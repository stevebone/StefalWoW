/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PreBurstResourcePooling.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "GameTime.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR / INITIALIZATION
// ============================================================================

PreBurstResourcePooling::PreBurstResourcePooling(Player* bot)
    : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("module.playerbot", "PreBurstResourcePooling: Created with null bot!");
        return;
    }
}

void PreBurstResourcePooling::Initialize()
{
    if (!_bot || _initialized)
        return;

    LoadSpecBurstCooldowns();
    _initialized = true;

    TC_LOG_DEBUG("module.playerbot", "PreBurstResourcePooling: Initialized for bot {} with {} burst CDs tracked",
        _bot->GetName(), _trackedBurstCDs.size());
}

void PreBurstResourcePooling::Reset()
{
    _recommendation.Reset();
    _isInBurstWindow = false;
    _updateTimer = 0;

    // Reset tracking state on burst CDs
    for (auto& cd : _trackedBurstCDs)
    {
        cd.remainingMs = 0;
        cd.isReady = false;
        cd.isActive = false;
    }
}

// ============================================================================
// CORE UPDATE
// ============================================================================

void PreBurstResourcePooling::Update(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld() || !_bot->IsAlive() || !_bot->IsInCombat())
        return;

    if (!_initialized)
        Initialize();

    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;
    _updateTimer = 0;

    UpdateCooldownTracking();
    _isInBurstWindow = CheckBurstActive();
    CalculateRecommendation();
}

// ============================================================================
// POOLING QUERIES
// ============================================================================

bool PreBurstResourcePooling::ShouldSkipForPooling(uint32 /*spellId*/, float priority) const
{
    switch (_recommendation.state)
    {
        case PoolingState::NONE:
            return false;

        case PoolingState::LIGHT:
            // Only skip low-priority fillers
            return priority >= LIGHT_POOL_SKIP_THRESHOLD;

        case PoolingState::MODERATE:
            // Skip most abilities except high-priority
            return priority >= MODERATE_POOL_SKIP_THRESHOLD;

        case PoolingState::AGGRESSIVE:
            // Skip everything except absolute top priorities
            return priority >= AGGRESSIVE_POOL_SKIP_THRESHOLD;

        default:
            return false;
    }
}

// ============================================================================
// COOLDOWN TRACKING
// ============================================================================

void PreBurstResourcePooling::UpdateCooldownTracking()
{
    if (!_bot || !_bot->GetSpellHistory())
        return;

    SpellHistory const* history = _bot->GetSpellHistory();

    for (auto& cd : _trackedBurstCDs)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(cd.spellId, DIFFICULTY_NONE);
        if (!spellInfo)
        {
            cd.isReady = false;
            cd.remainingMs = 0;
            continue;
        }

        // Check if spell is known by this bot
        if (!_bot->HasSpell(cd.spellId))
        {
            cd.isReady = false;
            cd.remainingMs = 0;
            continue;
        }

        // Get remaining cooldown
        SpellHistory::Duration remaining = history->GetRemainingCooldown(spellInfo);
        cd.remainingMs = static_cast<uint32>(remaining.count());

        cd.isReady = (cd.remainingMs == 0);

        // Check if the burst buff is currently active on the bot
        cd.isActive = _bot->HasAura(cd.spellId);
    }
}

void PreBurstResourcePooling::CalculateRecommendation()
{
    _recommendation.Reset();

    if (_trackedBurstCDs.empty())
        return;

    float currentResource = GetCurrentResourcePercent();
    _recommendation.currentResourcePct = currentResource;

    // If we're in a burst window, no need to pool
    if (_isInBurstWindow)
        return;

    // Find the burst CD closest to coming off cooldown within our pooling window
    BurstCooldownInfo const* bestCandidate = nullptr;
    uint32 closestTimeMs = _poolingWindowMs + 1; // Sentinel

    for (auto const& cd : _trackedBurstCDs)
    {
        if (!cd.requiresResource)
            continue;

        // Skip CDs we don't have
        if (cd.remainingMs == 0 && !cd.isReady)
            continue;

        // If it's already ready, that's the closest possible
        if (cd.isReady && !cd.isActive)
        {
            bestCandidate = &cd;
            closestTimeMs = 0;
            break;
        }

        // Check if it's within our pooling window
        if (cd.remainingMs > 0 && cd.remainingMs <= _poolingWindowMs)
        {
            if (cd.remainingMs < closestTimeMs)
            {
                bestCandidate = &cd;
                closestTimeMs = cd.remainingMs;
            }
        }
    }

    if (!bestCandidate)
        return;

    // Determine target resource level
    float targetResource = bestCandidate->resourceThreshold;
    targetResource = std::clamp(targetResource, _minPoolTarget, _maxPoolTarget);

    _recommendation.poolingForSpellId = bestCandidate->spellId;
    _recommendation.targetResourcePct = targetResource;
    _recommendation.timeUntilBurstMs = closestTimeMs;

    // Calculate resource deficit
    float deficit = targetResource - currentResource;
    _recommendation.resourceDeficit = std::max(0.0f, deficit);
    _recommendation.atTargetResource = (deficit <= 0.05f); // Within 5% is "at target"

    // If we're already at target, light pooling to maintain
    if (_recommendation.atTargetResource)
    {
        _recommendation.state = PoolingState::LIGHT;
        _recommendation.poolingReason = "Maintaining resource for " + bestCandidate->name;
    }
    else
    {
        // Determine intensity based on time remaining and deficit
        _recommendation.state = DeterminePoolingState(closestTimeMs, currentResource, targetResource);

        if (closestTimeMs == 0)
            _recommendation.poolingReason = bestCandidate->name + " ready - pool to " +
                std::to_string(static_cast<int>(targetResource * 100)) + "%";
        else
            _recommendation.poolingReason = bestCandidate->name + " in " +
                std::to_string(closestTimeMs / 1000) + "s - pool to " +
                std::to_string(static_cast<int>(targetResource * 100)) + "%";
    }

    if (_recommendation.state != PoolingState::NONE)
    {
        TC_LOG_TRACE("module.playerbot", "PreBurstResourcePooling [{}]: {} (resource {:.0f}% -> {:.0f}%, burst in {}ms)",
            _bot->GetName(), _recommendation.poolingReason,
            currentResource * 100.0f, targetResource * 100.0f, closestTimeMs);
    }
}

PoolingState PreBurstResourcePooling::DeterminePoolingState(uint32 timeUntilBurstMs,
    float currentResourcePct, float targetResourcePct) const
{
    float deficit = targetResourcePct - currentResourcePct;

    // No deficit? Light pool to maintain
    if (deficit <= 0.05f)
        return PoolingState::LIGHT;

    // Burst is ready NOW and we're significantly below target
    if (timeUntilBurstMs == 0)
    {
        if (deficit > 0.30f)
            return PoolingState::AGGRESSIVE;
        if (deficit > 0.15f)
            return PoolingState::MODERATE;
        return PoolingState::LIGHT;
    }

    // Burst coming soon (0-2s): more aggressive pooling
    if (timeUntilBurstMs <= 2000)
    {
        if (deficit > 0.25f)
            return PoolingState::AGGRESSIVE;
        if (deficit > 0.10f)
            return PoolingState::MODERATE;
        return PoolingState::LIGHT;
    }

    // Burst coming (2-4s): moderate pooling
    if (timeUntilBurstMs <= 4000)
    {
        if (deficit > 0.30f)
            return PoolingState::MODERATE;
        return PoolingState::LIGHT;
    }

    // Burst in 4-5s: light pooling to start building
    return PoolingState::LIGHT;
}

// ============================================================================
// RESOURCE TRACKING
// ============================================================================

float PreBurstResourcePooling::GetCurrentResourcePercent() const
{
    if (!_bot)
        return 0.0f;

    // Determine primary resource type from class
    Powers powerType = _bot->GetPowerType();
    uint32 current = _bot->GetPower(powerType);
    uint32 maximum = _bot->GetMaxPower(powerType);

    if (maximum == 0)
        return 1.0f; // No resource to track (some specs don't have a primary resource)

    return static_cast<float>(current) / static_cast<float>(maximum);
}

bool PreBurstResourcePooling::CheckBurstActive() const
{
    for (auto const& cd : _trackedBurstCDs)
    {
        if (cd.isActive)
            return true;
    }
    return false;
}

// ============================================================================
// MANUAL BURST CD MANAGEMENT
// ============================================================================

void PreBurstResourcePooling::AddBurstCooldown(uint32 spellId, std::string name, float resourceThreshold)
{
    // Check if already tracked
    for (auto const& cd : _trackedBurstCDs)
    {
        if (cd.spellId == spellId)
            return;
    }

    BurstCooldownInfo info;
    info.spellId = spellId;
    info.name = std::move(name);
    info.resourceThreshold = resourceThreshold;
    info.requiresResource = true;

    // Try to get cooldown duration from SpellInfo
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (spellInfo)
    {
        // RecoveryTime is in milliseconds
        info.cooldownMs = spellInfo->RecoveryTime;
        if (info.cooldownMs == 0)
            info.cooldownMs = spellInfo->CategoryRecoveryTime;
    }

    _trackedBurstCDs.push_back(std::move(info));
}

// ============================================================================
// SPEC-SPECIFIC BURST CD LOADING
// ============================================================================

void PreBurstResourcePooling::LoadSpecBurstCooldowns()
{
    if (!_bot)
        return;

    ChrSpecialization spec = _bot->GetPrimarySpecialization();

    // Each spec has 1-3 major burst CDs that benefit from resource pooling.
    // Resource threshold indicates how full the resource should be before pressing.
    // These are the "big CDs" where entering with full resources is a major DPS gain.

    switch (spec)
    {
        // ====================================================================
        // WARRIOR
        // ====================================================================
        case ChrSpecialization::WarriorArms:
            AddBurstCooldown(107574, "Avatar", 0.85f);             // Colossus Smash window
            AddBurstCooldown(227847, "Bladestorm", 0.70f);         // Arms Bladestorm
            break;
        case ChrSpecialization::WarriorFury:
            AddBurstCooldown(1719, "Recklessness", 0.90f);         // Major burst CD, need full rage
            AddBurstCooldown(107574, "Avatar", 0.85f);
            break;

        // ====================================================================
        // PALADIN
        // ====================================================================
        case ChrSpecialization::PaladinRetribution:
            AddBurstCooldown(31884, "Avenging Wrath", 0.90f);      // Wings + dump holy power
            AddBurstCooldown(255937, "Wake of Ashes", 0.30f);       // Generates holy power, no pool needed
            break;

        // ====================================================================
        // HUNTER
        // ====================================================================
        case ChrSpecialization::HunterBeastMastery:
            AddBurstCooldown(19574, "Bestial Wrath", 0.80f);       // Pool focus before Bestial Wrath
            break;
        case ChrSpecialization::HunterMarksmanship:
            AddBurstCooldown(288613, "Trueshot", 0.85f);           // Pool focus before Trueshot
            break;
        case ChrSpecialization::HunterSurvival:
            AddBurstCooldown(360952, "Coordinated Assault", 0.80f);
            break;

        // ====================================================================
        // ROGUE
        // ====================================================================
        case ChrSpecialization::RogueAssassination:
            AddBurstCooldown(79140, "Vendetta", 0.90f);            // Pool energy + combo points
            break;
        case ChrSpecialization::RogueOutlaw:
            AddBurstCooldown(13750, "Adrenaline Rush", 0.85f);     // Pool energy before AR
            break;
        case ChrSpecialization::RogueSubtely:
            AddBurstCooldown(121471, "Shadow Blades", 0.90f);      // Pool energy before Shadow Blades
            AddBurstCooldown(277925, "Symbols of Death", 0.80f);
            break;

        // ====================================================================
        // PRIEST
        // ====================================================================
        case ChrSpecialization::PriestShadow:
            AddBurstCooldown(228260, "Void Eruption", 0.90f);      // Pool insanity before Voidform
            AddBurstCooldown(391109, "Dark Ascension", 0.85f);
            break;

        // ====================================================================
        // DEATH KNIGHT
        // ====================================================================
        case ChrSpecialization::DeathKnightFrost:
            AddBurstCooldown(51271, "Pillar of Frost", 0.85f);     // Pool runic power
            AddBurstCooldown(152279, "Breath of Sindragosa", 0.95f); // Needs max runic power
            break;
        case ChrSpecialization::DeathKnightUnholy:
            AddBurstCooldown(63560, "Dark Transformation", 0.80f);
            AddBurstCooldown(275699, "Apocalypse", 0.75f);
            break;

        // ====================================================================
        // SHAMAN
        // ====================================================================
        case ChrSpecialization::ShamanEnhancement:
            AddBurstCooldown(51533, "Feral Spirit", 0.85f);        // Pool maelstrom
            AddBurstCooldown(114051, "Ascendance", 0.90f);
            break;
        case ChrSpecialization::ShamanElemental:
            AddBurstCooldown(114050, "Ascendance", 0.85f);         // Pool maelstrom before Ascendance
            AddBurstCooldown(191634, "Stormkeeper", 0.80f);
            break;

        // ====================================================================
        // MAGE
        // ====================================================================
        case ChrSpecialization::MageFire:
            AddBurstCooldown(190319, "Combustion", 0.70f);         // Mana less important, but pool fire blast charges
            break;
        case ChrSpecialization::MageFrost:
            AddBurstCooldown(12472, "Icy Veins", 0.75f);
            break;
        case ChrSpecialization::MageArcane:
            AddBurstCooldown(365350, "Arcane Surge", 0.95f);       // Arcane needs FULL mana for surge
            AddBurstCooldown(12042, "Arcane Power", 0.90f);
            break;

        // ====================================================================
        // WARLOCK
        // ====================================================================
        case ChrSpecialization::WarlockAffliction:
            AddBurstCooldown(205180, "Summon Darkglare", 0.85f);   // Pool soul shards
            break;
        case ChrSpecialization::WarlockDemonology:
            AddBurstCooldown(265187, "Summon Demonic Tyrant", 0.90f); // Pool soul shards for wild imps
            break;
        case ChrSpecialization::WarlockDestruction:
            AddBurstCooldown(1122, "Summon Infernal", 0.85f);      // Pool soul shards
            break;

        // ====================================================================
        // MONK
        // ====================================================================
        case ChrSpecialization::MonkWindwalker:
            AddBurstCooldown(137639, "Storm Earth and Fire", 0.85f); // Pool energy + chi
            AddBurstCooldown(152173, "Serenity", 0.90f);            // Pool energy before Serenity
            break;

        // ====================================================================
        // DRUID
        // ====================================================================
        case ChrSpecialization::DruidBalance:
            AddBurstCooldown(194223, "Celestial Alignment", 0.85f); // Pool astral power
            AddBurstCooldown(102560, "Incarnation: Chosen of Elune", 0.85f);
            break;
        case ChrSpecialization::DruidFeral:
            AddBurstCooldown(106951, "Berserk", 0.90f);            // Pool energy + combo points
            AddBurstCooldown(102543, "Incarnation: Avatar of Ashamane", 0.90f);
            break;

        // ====================================================================
        // DEMON HUNTER
        // ====================================================================
        case ChrSpecialization::DemonHunterHavoc:
            AddBurstCooldown(191427, "Metamorphosis", 0.85f);      // Pool fury
            AddBurstCooldown(258920, "Immolation Aura", 0.70f);
            break;

        // ====================================================================
        // EVOKER
        // ====================================================================
        case ChrSpecialization::EvokerDevastation:
            AddBurstCooldown(375087, "Dragonrage", 0.85f);         // Pool essence
            break;
        case ChrSpecialization::EvokerAugmentation:
            AddBurstCooldown(395152, "Ebon Might", 0.80f);
            break;

        default:
            // Healing specs and tank specs generally don't need resource pooling for burst
            // Protection Warrior, Protection Paladin, etc. are excluded
            break;
    }
}

} // namespace Playerbot
