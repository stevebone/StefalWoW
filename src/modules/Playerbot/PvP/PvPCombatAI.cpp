/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PvPCombatAI.h"
#include "PvPSpellUtils.h"  // WoW 12.0: PvP multiplier and SpellAttr16 support
#include "SpellHistory.h"
#include "GameTime.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "World.h"
#include <algorithm>
#include <cmath>
#include "../Spatial/SpatialGridManager.h"  // Spatial grid for deadlock fix


namespace Playerbot
{

// Static member initialization
PvPMetrics PvPCombatAI::_globalMetrics;

// ============================================================================
// SINGLETON
// ============================================================================

PvPCombatAI::PvPCombatAI(Player* bot)
    : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot.pvp", "PvPCombatAI: Attempted to create with null bot!");
        return;
    }

    TC_LOG_DEBUG("playerbot.pvp", "PvPCombatAI: Created for bot {} ({})",
                 _bot->GetName(), _bot->GetGUID().ToString());
}

PvPCombatAI::~PvPCombatAI()
{
    // CRITICAL: Do NOT access _bot->GetName() or GetGUID().ToString() during destructor!
    // During bot destruction, the Player's internal string data may already be
    // freed even if the pointer is valid. This causes ACCESS_VIOLATION in
    // the string formatting code (VCRUNTIME140.dll).
    //
    // If logging is needed, use GUID counter instead:
    // if (_bot)
    //     TC_LOG_DEBUG("playerbot.pvp",
    //         "PvPCombatAI destroyed for bot {}", _bot->GetGUID().GetCounter());
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void PvPCombatAI::Initialize()
{

    TC_LOG_INFO("playerbot", "PvPCombatAI: Initializing PvP combat systems...");

    // Initialize class-specific spell databases
    // Full implementation would load from DBC/DB2

    TC_LOG_INFO("playerbot", "PvPCombatAI: Initialization complete");
}

void PvPCombatAI::Update(uint32 /*diff*/)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Throttle updates (100ms for PvP responsiveness)
    if (_lastUpdateTime > 0)
    {
        uint32 timeSinceLastUpdate = currentTime - _lastUpdateTime;
        if (timeSinceLastUpdate < COMBAT_UPDATE_INTERVAL)
            return;
    }

    _lastUpdateTime = currentTime;

    // Get combat profile
    PvPCombatProfile profile = GetCombatProfile();
    PvPCombatState state = GetCombatState();
    // Not in combat - idle state
    if (!_bot->IsInCombat())
    {
        if (state != PvPCombatState::IDLE)
            SetCombatState(PvPCombatState::IDLE);
        return;
    }

    // Auto-interrupt enemy casts
    if (profile.autoInterrupt)
    {
        ::Unit* target = _bot->GetSelectedUnit();
        if (target && target->HasUnitState(UNIT_STATE_CASTING))
        {
            if (ShouldInterrupt(target))
                InterruptCast(target);
        }
    }

    // Auto-use defensive cooldowns
    if (profile.autoDefensiveCooldowns)
    {
        uint32 healthPct = _bot->GetHealthPct();
        if (healthPct < profile.defensiveHealthThreshold)
            UseDefensiveCooldown();
    }

    // Auto-trinket CC
    if (profile.autoTrinket)
    {
        if (_bot->HasUnitState(UNIT_STATE_CONTROLLED) ||
            _bot->HasUnitState(UNIT_STATE_STUNNED))
        {
            UseTrinket();
        }
    }

    // Auto-peel for allies
    if (profile.autoPeel)
    {
        ::Unit* allyNeedingPeel = FindAllyNeedingPeel();
        if (allyNeedingPeel)
            PeelForAlly(allyNeedingPeel);
    }
    // Target selection and offensive actions
    ::Unit* currentTarget = _bot->GetSelectedUnit();

    // Check if should switch target
    if (ShouldSwitchTarget())
    {
        ::Unit* newTarget = SelectBestTarget();
        if (newTarget && newTarget != currentTarget)
        {
            _bot->SetSelection(newTarget->GetGUID());
            currentTarget = newTarget;
        }
    }

    if (!currentTarget)
        return;

    // Execute CC chain if enabled
    if (profile.autoCCChain)
        ExecuteCCChain(currentTarget);

    // Execute offensive burst if target is low
    if (profile.autoOffensiveBurst)
    {
        if (ShouldBurstTarget(currentTarget))
            ExecuteOffensiveBurst(currentTarget);
    }
}

// ============================================================================
// TARGET SELECTION
// ============================================================================

::Unit* PvPCombatAI::SelectBestTarget() const
{
    if (!_bot)
        return nullptr;

    std::vector<::Unit*> enemies = GetEnemyPlayers(40.0f);
    if (enemies.empty())
        return nullptr;

    PvPCombatProfile profile = GetCombatProfile();

    // Assess threat for all enemies
    std::vector<std::pair<::Unit*, float>> threatScores;
    for (::Unit* enemy : enemies)
    {
        ThreatAssessment assessment = AssessThreat(enemy);
        threatScores.push_back({enemy, assessment.threatScore});
    }

    // Sort by threat score descending
    std::sort(threatScores.begin(), threatScores.end(),
        [](auto const& a, auto const& b) { return a.second > b.second; });

    return threatScores.empty() ? nullptr : threatScores[0].first;
}

ThreatAssessment PvPCombatAI::AssessThreat(::Unit* target) const
{
    ThreatAssessment assessment;
    assessment.targetGuid = target->GetGUID();
    assessment.healthPercent = target->GetHealthPct();
    assessment.distanceToPlayer = static_cast<uint32>(std::sqrt(_bot->GetExactDistSq(target))); // Calculate once from squared distance
    // Check if healer
    assessment.isHealer = IsHealer(target);

    // Check if caster
    assessment.isCaster = IsCaster(target);

    // Check if attacking ally
    assessment.isAttackingAlly = IsTargetAttackingAlly(target);

    // Estimate DPS
    assessment.damageOutput = EstimateDPS(target);

    // Calculate threat score
    assessment.threatScore = CalculateThreatScore(target);

    return assessment;
}

std::vector<::Unit*> PvPCombatAI::GetEnemyPlayers(float range) const
{
    std::vector<::Unit*> enemies;

    if (!_bot || !_bot->GetMap())
        return enemies;

    // Find all players in range using TrinityCore API
    std::vector<::Player*> players;
    _bot->GetPlayerListInGrid(players, range, true);

    // Filter to only hostile players
    for (::Player* player : players)
    {
        if (player && player != _bot && _bot->IsHostileTo(player) && !player->isDead())
            enemies.push_back(player);
    }

    return enemies;
}

std::vector<::Unit*> PvPCombatAI::GetEnemyHealers() const
{
    std::vector<::Unit*> enemies = GetEnemyPlayers(40.0f);
    std::vector<::Unit*> healers;

    for (::Unit* enemy : enemies)
    {
        if (IsHealer(enemy))
            healers.push_back(enemy);
    }

    return healers;
}

bool PvPCombatAI::ShouldSwitchTarget() const
{
    if (!_bot)
        return false;

    ::Unit* currentTarget = _bot->GetSelectedUnit();
    if (!currentTarget || currentTarget->isDead())
        return true;

    // Don't switch if current target is low health
    if (currentTarget->GetHealthPct() < 30)
        return false;

    // Check if a better target exists
    ::Unit* bestTarget = SelectBestTarget();
    if (!bestTarget || bestTarget == currentTarget)
        return false;

    // Switch if best target has significantly higher threat score
    ThreatAssessment currentAssessment = AssessThreat(currentTarget);
    ThreatAssessment bestAssessment = AssessThreat(bestTarget);

    return bestAssessment.threatScore > (currentAssessment.threatScore * 1.3f);
}

// ============================================================================
// CC CHAIN COORDINATION
// ============================================================================

bool PvPCombatAI::ExecuteCCChain(::Unit* target)
{
    if (!_bot || !target)
        return false;

    PvPCombatProfile profile = GetCombatProfile();
    if (!profile.autoCCChain)
        return false;

    // Get next CC ability
    uint32 ccSpellId = GetNextCCAbility(target);
    if (ccSpellId == 0)
        return false;

    // Check cooldown
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(ccSpellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Cast CC
    if (_bot->GetSpellHistory()->HasCooldown(ccSpellId))
        return false;

    // Full implementation: Cast spell using TrinityCore spell system
    TC_LOG_DEBUG("playerbot", "PvPCombatAI: Player {} casting CC spell {} on target {}",
        _bot->GetGUID().GetCounter(), ccSpellId, target->GetGUID().GetCounter());

    // Track CC usage
    ObjectGuid targetGuid = target->GetGUID();
    if (!_ccChains.count(targetGuid))
        _ccChains[targetGuid] = CCChain();

    _ccChains[targetGuid].targetGuid = targetGuid;
    _ccChains[targetGuid].lastCCTime = GameTime::GetGameTimeMS();

    // Update metrics
    
    _metrics.ccChainsExecuted++;
    _globalMetrics.ccChainsExecuted++;

    return true;
}

uint32 PvPCombatAI::GetNextCCAbility(::Unit* target) const
{
    if (!_bot || !target)
        return 0;

    // Get available CC types for player's class
    std::vector<CCType> availableCC = GetAvailableCCTypes();

    // Filter by diminishing returns
    for (CCType ccType : availableCC)
    {
        if (IsTargetCCImmune(target, ccType))
            continue;

        uint32 spellId = GetCCSpellId(ccType);
        if (spellId != 0 && !IsCCOnCooldown(ccType))
            return spellId;
    }

    return 0;
}

uint32 PvPCombatAI::GetDiminishingReturnsLevel(::Unit* target, CCType ccType) const
{
    if (!target)
        return 0;

    ObjectGuid targetGuid = target->GetGUID();
    if (!_ccChains.count(targetGuid))
        return 0;

    return _ccChains.at(targetGuid).diminishingReturnsLevel;
}

void PvPCombatAI::TrackCCUsed(::Unit* target, CCType ccType)
{
    if (!target)
        return;

    ObjectGuid targetGuid = target->GetGUID();
    if (!_ccChains.count(targetGuid))
        _ccChains[targetGuid] = CCChain();

    CCChain& chain = _ccChains[targetGuid];
    chain.ccSequence.push_back(ccType);
    chain.lastCCTime = GameTime::GetGameTimeMS();

    // Increase DR level
    if (chain.diminishingReturnsLevel < 3)
        chain.diminishingReturnsLevel++;

    // Reset DR after 18 seconds
    uint32 timeSinceLastCC = GameTime::GetGameTimeMS() - chain.lastCCTime;
    if (timeSinceLastCC > DR_RESET_TIME)
        chain.diminishingReturnsLevel = 0;
}

bool PvPCombatAI::IsTargetCCImmune(::Unit* target, CCType ccType) const
{
    if (!target)
        return true;

    // Check diminishing returns immunity (DR level 3)
    uint32 drLevel = GetDiminishingReturnsLevel(target, ccType);
    if (drLevel >= 3)
        return true;

    // Check for CC immunity buffs (Will of the Forsaken, Ice Block, etc.)
    // Full implementation: Query aura system

    return false;
}

// ============================================================================
// DEFENSIVE COOLDOWNS
// ============================================================================

bool PvPCombatAI::UseDefensiveCooldown()
{
    if (!_bot)
        return false;

    uint32 cdSpellId = GetBestDefensiveCooldown();
    if (cdSpellId == 0)
        return false;

    if (_bot->GetSpellHistory()->HasCooldown(cdSpellId))
        return false;

    TC_LOG_INFO("playerbot", "PvPCombatAI: Player {} using defensive cooldown {}",
        _bot->GetGUID().GetCounter(), cdSpellId);

    // Full implementation: Cast spell

    // Update metrics
    
    _metrics.defensivesUsed++;
    _globalMetrics.defensivesUsed++;

    return true;
}

uint32 PvPCombatAI::GetBestDefensiveCooldown() const
{
    if (!_bot)
        return 0;

    uint32 healthPct = _bot->GetHealthPct();
    // Use immunity if very low health
    if (healthPct < 20 && ShouldUseImmunity())
    {
        // Class-specific immunity spells
        switch (_bot->GetClass())
        {
            case CLASS_PALADIN: return 642;  // Divine Shield
            case CLASS_MAGE: return 45438;   // Ice Block
            case CLASS_HUNTER: return 186265; // Aspect of the Turtle
            default: break;
        }
    }

    // Get class-specific defensive cooldowns
    std::vector<uint32> defensives;
    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR: defensives = GetWarriorDefensiveCooldowns(); break;
        case CLASS_PALADIN: defensives = GetPaladinDefensiveCooldowns(); break;
        case CLASS_HUNTER: defensives = GetHunterDefensiveCooldowns(); break;
        case CLASS_ROGUE: defensives = GetRogueDefensiveCooldowns(); break;
        case CLASS_PRIEST: defensives = GetPriestDefensiveCooldowns(); break;
        case CLASS_DEATH_KNIGHT: defensives = GetDeathKnightDefensiveCooldowns(); break;
        case CLASS_SHAMAN: defensives = GetShamanDefensiveCooldowns(); break;
        case CLASS_MAGE: defensives = GetMageDefensiveCooldowns(); break;
        case CLASS_WARLOCK: defensives = GetWarlockDefensiveCooldowns(); break;
        case CLASS_MONK: defensives = GetMonkDefensiveCooldowns(); break;
        case CLASS_DRUID: defensives = GetDruidDefensiveCooldowns(); break;
        case CLASS_DEMON_HUNTER: defensives = GetDemonHunterDefensiveCooldowns(); break;
        case CLASS_EVOKER: defensives = GetEvokerDefensiveCooldowns(); break;
        default: return 0;
    }

    // Find first available defensive
    for (uint32 spellId : defensives)
    {
        if (!_bot->GetSpellHistory()->HasCooldown(spellId))
            return spellId;
    }

    return 0;
}

bool PvPCombatAI::ShouldUseImmunity() const
{
    if (!_bot)
        return false;

    // Use immunity if:
    // 1. Very low health (<20%)
    // 2. Multiple enemies attacking
    // 3. Under heavy burst damage

    uint32 healthPct = _bot->GetHealthPct();
    if (healthPct < 20)
        return true;

    std::vector<::Unit*> attackers = GetEnemyPlayers(10.0f);
    if (attackers.size() >= 2)
        return true;

    return false;
}

bool PvPCombatAI::UseTrinket()
{
    if (!_bot)
        return false;

    TC_LOG_DEBUG("playerbot", "PvPCombatAI: Player {} using PvP trinket",
        _bot->GetGUID().GetCounter());

    // Full implementation: Use trinket item (42292 or 208683)
    // PvP trinkets break CC and provide immunity

    return true;
}

// ============================================================================
// OFFENSIVE BURSTS
// ============================================================================

bool PvPCombatAI::ExecuteOffensiveBurst(::Unit* target)
{
    if (!_bot || !target)
        return false;

    TC_LOG_INFO("playerbot", "PvPCombatAI: Player {} executing offensive burst on target {}",
        _bot->GetGUID().GetCounter(), target->GetGUID().GetCounter());

    // Stack offensive cooldowns
    bool success = StackOffensiveCooldowns();

    if (success)
    {
        
        _metrics.burstsExecuted++;
        _globalMetrics.burstsExecuted++;
    }

    return success;
}

bool PvPCombatAI::ShouldBurstTarget(::Unit* target) const
{
    if (!_bot || !target)
        return false;

    PvPCombatProfile profile = GetCombatProfile();
    // Burst if target below threshold
    if (target->GetHealthPct() < profile.burstHealthThreshold)
        return true;

    // Burst if target is healer
    if (IsHealer(target))
        return true;

    return false;
}

std::vector<uint32> PvPCombatAI::GetOffensiveCooldowns() const
{
    if (!_bot)
        return {};

    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR: return GetWarriorOffensiveCooldowns();
        case CLASS_PALADIN: return GetPaladinOffensiveCooldowns();
        case CLASS_HUNTER: return GetHunterOffensiveCooldowns();
        case CLASS_ROGUE: return GetRogueOffensiveCooldowns();
        case CLASS_PRIEST: return GetPriestOffensiveCooldowns();
        case CLASS_DEATH_KNIGHT: return GetDeathKnightOffensiveCooldowns();
        case CLASS_SHAMAN: return GetShamanOffensiveCooldowns();
        case CLASS_MAGE: return GetMageOffensiveCooldowns();
        case CLASS_WARLOCK: return GetWarlockOffensiveCooldowns();
        case CLASS_MONK: return GetMonkOffensiveCooldowns();
        case CLASS_DRUID: return GetDruidOffensiveCooldowns();
        case CLASS_DEMON_HUNTER: return GetDemonHunterOffensiveCooldowns();
        case CLASS_EVOKER: return GetEvokerOffensiveCooldowns();
        default: return {};
    }
}
bool PvPCombatAI::StackOffensiveCooldowns()
{
    if (!_bot)
        return false;

    std::vector<uint32> cooldowns = GetOffensiveCooldowns();
    bool usedAny = false;

    for (uint32 spellId : cooldowns)
    {
        // Check if spell exists before checking cooldown (HasCooldown asserts on invalid spells)
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        if (!_bot->GetSpellHistory()->HasCooldown(spellId))
        {
            // Full implementation: Cast spell
            TC_LOG_DEBUG("playerbot", "PvPCombatAI: Using offensive CD {}", spellId);
            usedAny = true;
        }
    }

    return usedAny;
}

// ============================================================================
// INTERRUPT COORDINATION
// ============================================================================

bool PvPCombatAI::InterruptCast(::Unit* target)
{
    if (!_bot || !target)
        return false;

    uint32 interruptSpell = GetInterruptSpell();
    if (interruptSpell == 0)
        return false;

    if (_bot->GetSpellHistory()->HasCooldown(interruptSpell))
        return false;

    TC_LOG_INFO("playerbot", "PvPCombatAI: Player {} interrupting target {} cast",
        _bot->GetGUID().GetCounter(), target->GetGUID().GetCounter());

    // Full implementation: Cast interrupt spell

    // Update metrics
    
    _metrics.interruptsLanded++;
    _globalMetrics.interruptsLanded++;

    return true;
}

bool PvPCombatAI::ShouldInterrupt(::Unit* target) const
{
    if (!_bot || !target)
        return false;

    if (!target->HasUnitState(UNIT_STATE_CASTING))
        return false;

    // Full implementation: Check if spell is interruptible and high priority
    // Priority interrupts: Heals, CC, high damage spells

    return true;
}

uint32 PvPCombatAI::GetInterruptSpell() const
{
    if (!_bot)
        return 0;

    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR: return GetWarriorInterruptSpell();
        case CLASS_PALADIN: return 96231;  // Rebuke
        case CLASS_HUNTER: return 187650;  // Counter Shot
        case CLASS_ROGUE: return 1766;     // Kick
        case CLASS_PRIEST: return 15487;   // Silence (Shadow)
        case CLASS_DEATH_KNIGHT: return 47528; // Mind Freeze
        case CLASS_SHAMAN: return 57994;   // Wind Shear
        case CLASS_MAGE: return 2139;      // Counterspell
        case CLASS_WARLOCK: return 119910; // Spell Lock (pet)
        case CLASS_MONK: return 116705;    // Spear Hand Strike
        case CLASS_DRUID: return 106839;   // Skull Bash
        case CLASS_DEMON_HUNTER: return 183752; // Disrupt
        case CLASS_EVOKER: return 351338;  // Quell
        default: return 0;
    }
}

// ============================================================================
// PEEL MECHANICS
// ============================================================================

bool PvPCombatAI::PeelForAlly(::Unit* ally)
{
    if (!_bot || !ally)
        return false;

    uint32 peelSpell = GetPeelAbility();
    if (peelSpell == 0)
        return false;

    TC_LOG_INFO("playerbot", "PvPCombatAI: Player {} peeling for ally {}",
        _bot->GetGUID().GetCounter(), ally->GetGUID().GetCounter());

    // Full implementation: Cast peel ability (CC on attacker)

    // Update metrics
    
    _metrics.peelsPerformed++;
    _globalMetrics.peelsPerformed++;

    return true;
}

::Unit* PvPCombatAI::FindAllyNeedingPeel() const
{
    if (!_bot)
        return nullptr;

    Group* group = _bot->GetGroup();
    if (!group)
        return nullptr;

    // Find ally with lowest health being attacked
    ::Unit* allyNeedingPeel = nullptr;
    uint32 lowestHealth = 100;

    for (GroupReference const& itr : group->GetMembers())
    {
        ::Player* member = itr.GetSource();
        if (!member || member == _bot || !member->IsInWorld())
            continue;

        if (member->GetHealthPct() < lowestHealth &&
            member->IsInCombat())
        {
            lowestHealth = static_cast<uint32>(member->GetHealthPct());
            allyNeedingPeel = member;
        }
    }

    return allyNeedingPeel;
}

uint32 PvPCombatAI::GetPeelAbility() const
{
    if (!_bot)
        return 0;

    // Return class-specific peel abilities (CC, knockback, etc.)
    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR: return 5246;   // Intimidating Shout
        case CLASS_PALADIN: return 853;    // Hammer of Justice
        case CLASS_HUNTER: return 19577;   // Intimidation
        case CLASS_ROGUE: return 1833;     // Cheap Shot
        case CLASS_PRIEST: return 8122;    // Psychic Scream
        case CLASS_DEATH_KNIGHT: return 108194; // Asphyxiate
        case CLASS_SHAMAN: return 51514;   // Hex
        case CLASS_MAGE: return 118;       // Polymorph
        case CLASS_WARLOCK: return 5782;   // Fear
        case CLASS_MONK: return 119381;    // Leg Sweep
        case CLASS_DRUID: return 5211;     // Bash
        case CLASS_DEMON_HUNTER: return 179057; // Chaos Nova
        case CLASS_EVOKER: return 351338;  // Quell (can be used defensively)
        default: return 0;
    }
}

// ============================================================================
// COMBAT STATE
// ============================================================================

void PvPCombatAI::SetCombatState(PvPCombatState state)
{
    _combatState = state;
}

PvPCombatState PvPCombatAI::GetCombatState() const
{
    return _combatState;
}

// ============================================================================
// PROFILES
// ============================================================================

void PvPCombatAI::SetCombatProfile(PvPCombatProfile const& profile)
{
    _profile = profile;
}

PvPCombatProfile PvPCombatAI::GetCombatProfile() const
{
    return _profile;
}

// ============================================================================
// METRICS
// ============================================================================

PvPMetrics const& PvPCombatAI::GetMetrics() const
{
    return _metrics;
}

PvPMetrics const& PvPCombatAI::GetGlobalMetrics() const
{
    return _globalMetrics;
}
// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool PvPCombatAI::IsHealer(::Unit* unit) const
{
    if (!unit || !unit->IsPlayer())
        return false;

    ::Player* targetPlayer = unit->ToPlayer();
    ChrSpecialization spec = targetPlayer->GetPrimarySpecialization();
    // Check if player is in healing spec using TWW ChrSpecialization enum
    switch (spec)
    {
        case ChrSpecialization::PriestDiscipline:
        case ChrSpecialization::PriestHoly:
        case ChrSpecialization::PaladinHoly:
        case ChrSpecialization::ShamanRestoration:
        case ChrSpecialization::DruidRestoration:
        case ChrSpecialization::MonkMistweaver:
        case ChrSpecialization::EvokerPreservation:
            return true;
        default:
            return false;
    }
}

bool PvPCombatAI::IsCaster(::Unit* unit) const
{
    if (!unit || !unit->IsPlayer())
        return false;

    ::Player* targetPlayer = unit->ToPlayer();
    switch (targetPlayer->GetClass())
    {
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_DRUID: // Balance
        case CLASS_EVOKER:
            return true;
        default:
            return false;
    }
}

uint32 PvPCombatAI::EstimateDPS(::Unit* unit) const
{
    if (!unit)
        return 0;

    // WoW 12.0: Enhanced DPS estimation with PvP modifier awareness
    // Uses PvPSpellUtils to account for PvpMultiplier from SpellEffectEntry
    //
    // DPS estimation considers:
    // 1. Base attack power / spell power from stats
    // 2. Class-specific rotation damage potential
    // 3. PvP modifiers that reduce damage in PvP combat
    // 4. Current cooldown availability for burst windows

    uint32 baseDPS = 0;

    // Determine if this is PvP combat context
    bool isPvPCombat = PvPSpellUtils::IsInPvPCombat(unit);

    if (unit->IsPlayer())
    {
        Player const* player = unit->ToPlayer();
        uint8 playerClass = player->GetClass();

        // Get primary damage stat
        float attackPower = player->GetTotalAttackPowerValue(BASE_ATTACK);
        float spellPower = static_cast<float>(player->GetBaseSpellPowerBonus());

        // Class-specific DPS estimation
        // Base calculation using attack power or spell power depending on class
        switch (playerClass)
        {
            // Melee classes - use attack power
            case CLASS_WARRIOR:
            case CLASS_ROGUE:
            case CLASS_DEATH_KNIGHT:
            case CLASS_MONK:
            case CLASS_DEMON_HUNTER:
            case CLASS_PALADIN:
                baseDPS = static_cast<uint32>(attackPower * 0.6f);
                break;

            // Caster classes - use spell power
            case CLASS_MAGE:
            case CLASS_WARLOCK:
            case CLASS_PRIEST:
            case CLASS_EVOKER:
                baseDPS = static_cast<uint32>(spellPower * 0.8f);
                break;

            // Hybrid classes - use higher of AP/SP
            case CLASS_SHAMAN:
            case CLASS_DRUID:
            case CLASS_HUNTER:
                baseDPS = static_cast<uint32>(std::max(attackPower * 0.6f, spellPower * 0.8f));
                break;

            default:
                baseDPS = static_cast<uint32>(std::max(attackPower, spellPower) * 0.5f);
                break;
        }

        // Apply PvP reduction if in PvP combat
        // Most damage spells have a ~20-40% PvP reduction
        if (isPvPCombat)
        {
            // Apply average PvP damage reduction factor
            // This accounts for typical PvpMultiplier values (0.6-0.9)
            constexpr float AVG_PVP_DAMAGE_REDUCTION = 0.75f;
            baseDPS = static_cast<uint32>(baseDPS * AVG_PVP_DAMAGE_REDUCTION);
        }

        // Check for burst cooldowns that increase damage
        // If offensive CDs are active, estimate higher DPS
        if (HasBurstCooldownActive(player))
        {
            baseDPS = static_cast<uint32>(baseDPS * 1.5f);
        }

        // Ensure minimum DPS estimate
        if (baseDPS < 1000)
            baseDPS = 1000;
    }
    else
    {
        // Non-player unit - use base 5000 DPS estimate
        baseDPS = 5000;
    }

    return baseDPS;
}

bool PvPCombatAI::HasBurstCooldownActive(Player const* player) const
{
    if (!player)
        return false;

    // Check for common offensive burst cooldown auras
    // These are spell IDs for major offensive cooldowns that significantly increase damage
    static const std::vector<uint32> burstCooldowns = {
        // Warrior
        1719,    // Recklessness
        107574,  // Avatar

        // Paladin
        31884,   // Avenging Wrath

        // Mage
        12042,   // Arcane Power
        190319,  // Combustion
        12472,   // Icy Veins

        // Rogue
        13750,   // Adrenaline Rush
        121471,  // Shadow Blades

        // Death Knight
        51271,   // Pillar of Frost
        207289,  // Unholy Assault

        // Shaman
        191634,  // Stormkeeper

        // Warlock
        113860,  // Dark Soul

        // Monk
        137639,  // Storm, Earth, and Fire

        // Druid
        106951,  // Berserk
        194223,  // Celestial Alignment

        // Demon Hunter
        191427,  // Metamorphosis (Havoc)

        // Evoker
        375087,  // Dragonrage

        // Hunter
        19574,   // Bestial Wrath
        288613,  // Trueshot

        // Priest
        10060,   // Power Infusion
    };

    for (uint32 spellId : burstCooldowns)
    {
        if (player->HasAura(spellId))
            return true;
    }

    return false;
}

float PvPCombatAI::CalculateThreatScore(::Unit* target) const
{
    if (!_bot || !target)
        return 0.0f;

    float score = 50.0f; // Base score

    // Healer multiplier
    if (IsHealer(target))
        score *= HEALER_THREAT_MULTIPLIER;

    // Low health multiplier
    if (target->GetHealthPct() < 40)
        score *= LOW_HEALTH_THREAT_MULTIPLIER;

    // Attacking ally multiplier
    if (IsTargetAttackingAlly(target))
        score *= ATTACKING_ALLY_MULTIPLIER;

    // Distance penalty
    float distance = std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance
    if (distance > 30.0f)
        score *= 0.5f;

    // DPS multiplier
    uint32 dps = EstimateDPS(target);
    score += (dps / 100.0f);

    return score;
}

bool PvPCombatAI::IsInCCRange(::Unit* target, CCType ccType) const
{
    if (!_bot || !target)
        return false;

    float distance = std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance

    // Range check based on CC type
    switch (ccType)
    {
        case CCType::STUN:
        case CCType::INCAPACITATE:
            return distance <= 5.0f; // Melee range
        case CCType::ROOT:
        case CCType::SILENCE:
        case CCType::POLYMORPH:
            return distance <= 30.0f; // Ranged
        case CCType::FEAR:
            return distance <= 8.0f;
        default:
            return false;
    }
}

bool PvPCombatAI::HasCCAvailable(CCType ccType) const
{
    if (!_bot)
        return false;

    uint32 spellId = GetCCSpellId(ccType);
    return spellId != 0 && !_bot->GetSpellHistory()->HasCooldown(spellId);
}

uint32 PvPCombatAI::GetCCSpellId(CCType ccType) const
{
    if (!_bot)
        return 0;

    // Full CC spell database with DR categories and duration tracking
    // Current behavior: Returns basic CC spell ID per class without spec consideration
    // Full implementation should:
    // - Query player's active specialization to return spec-specific CC spells
    // - Handle talent-based CC abilities (e.g., Intimidating Shout vs Storm Bolt for Warriors)
    // - Check if spell is learned via HasSpell() before returning
    // - Map CCType to multiple possible spells per class (primary, secondary, tertiary options)
    // - Consider covenant/talent tree variations for current expansion
    // Reference: SpellMgr, Player::HasSpell(), ChrSpecialization mappings

    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR:
            if (ccType == CCType::STUN) return 46968; // Shockwave
            if (ccType == CCType::FEAR) return 5246;  // Intimidating Shout
            break;
        case CLASS_PALADIN:
            if (ccType == CCType::STUN) return 853;   // Hammer of Justice
            break;
        case CLASS_HUNTER:
            if (ccType == CCType::ROOT) return 3355;  // Freezing Trap
            break;
        case CLASS_ROGUE:
            if (ccType == CCType::STUN) return 1833;  // Cheap Shot
            break;
        case CLASS_MAGE:
            if (ccType == CCType::POLYMORPH) return 118; // Polymorph
            if (ccType == CCType::ROOT) return 122;   // Frost Nova
            break;
        default:
            break;
    }

    return 0;
}

bool PvPCombatAI::IsCCOnCooldown(CCType ccType) const
{
    if (!_bot)
        return true;

    uint32 spellId = GetCCSpellId(ccType);
    return spellId == 0 || _bot->GetSpellHistory()->HasCooldown(spellId);
}

std::vector<CCType> PvPCombatAI::GetAvailableCCTypes() const
{
    std::vector<CCType> ccTypes;

    if (!_bot)
        return ccTypes;

    // Return available CC types based on class
    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR:
            ccTypes.push_back(CCType::STUN);
            ccTypes.push_back(CCType::FEAR);
            break;
        case CLASS_PALADIN:
            ccTypes.push_back(CCType::STUN);
            break;
        case CLASS_HUNTER:
            ccTypes.push_back(CCType::ROOT);
            ccTypes.push_back(CCType::STUN);
            break;
        case CLASS_ROGUE:
            ccTypes.push_back(CCType::STUN);
            ccTypes.push_back(CCType::SILENCE);
            break;
        case CLASS_MAGE:
            ccTypes.push_back(CCType::POLYMORPH);
            ccTypes.push_back(CCType::ROOT);
            break;
        case CLASS_WARLOCK:
            ccTypes.push_back(CCType::FEAR);
            break;
        case CLASS_DRUID:
            ccTypes.push_back(CCType::ROOT);
            ccTypes.push_back(CCType::STUN);
            break;
        default:
            break;
    }

    return ccTypes;
}

bool PvPCombatAI::IsTargetAttackingAlly(::Unit* target) const
{
    if (!target || !_bot)
        return false;

    ::Unit* targetVictim = target->GetVictim();
    if (!targetVictim || !targetVictim->IsPlayer())
        return false;

    // Check if victim is in player's group
    Group* group = _bot->GetGroup();
    if (!group)
        return false;

    return group->IsMember(targetVictim->GetGUID());
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - WARRIOR
// ============================================================================

std::vector<uint32> PvPCombatAI::GetWarriorDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(871);    // Shield Wall
    cooldowns.push_back(97462);  // Rallying Cry
    cooldowns.push_back(18499);  // Berserker Rage
    cooldowns.push_back(23920);  // Spell Reflection
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetWarriorOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(1719);   // Recklessness
    cooldowns.push_back(107574); // Avatar
    cooldowns.push_back(46924);  // Bladestorm
    return cooldowns;
}

uint32 PvPCombatAI::GetWarriorInterruptSpell() const
{
    return 6552; // Pummel
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - PALADIN
// ============================================================================

std::vector<uint32> PvPCombatAI::GetPaladinDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(642);    // Divine Shield
    cooldowns.push_back(498);    // Divine Protection
    cooldowns.push_back(1022);   // Blessing of Protection
    cooldowns.push_back(633);    // Lay on Hands
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetPaladinOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(31884);  // Avenging Wrath
    cooldowns.push_back(231895); // Crusade
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - HUNTER
// ============================================================================

std::vector<uint32> PvPCombatAI::GetHunterDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(186265); // Aspect of the Turtle
    cooldowns.push_back(109304); // Exhilaration
    cooldowns.push_back(5384);   // Feign Death
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetHunterOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(19574);  // Bestial Wrath
    cooldowns.push_back(288613); // Trueshot
    cooldowns.push_back(266779); // Coordinated Assault
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - ROGUE
// ============================================================================

std::vector<uint32> PvPCombatAI::GetRogueDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(31224);  // Cloak of Shadows
    cooldowns.push_back(5277);   // Evasion
    cooldowns.push_back(1856);   // Vanish
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetRogueOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(13750);  // Adrenaline Rush
    cooldowns.push_back(121471); // Shadow Blades
    cooldowns.push_back(79140);  // Vendetta
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - PRIEST
// ============================================================================

std::vector<uint32> PvPCombatAI::GetPriestDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(47585);  // Dispersion
    cooldowns.push_back(33206);  // Pain Suppression
    cooldowns.push_back(19236);  // Desperate Prayer
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetPriestOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(10060);  // Power Infusion
    cooldowns.push_back(47540);  // Penance
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - DEATH KNIGHT
// ============================================================================

std::vector<uint32> PvPCombatAI::GetDeathKnightDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(48792);  // Icebound Fortitude
    cooldowns.push_back(48707);  // Anti-Magic Shell
    cooldowns.push_back(55233);  // Vampiric Blood
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetDeathKnightOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(51271);  // Pillar of Frost
    cooldowns.push_back(207289); // Unholy Assault
    cooldowns.push_back(152279); // Breath of Sindragosa
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - SHAMAN
// ============================================================================

std::vector<uint32> PvPCombatAI::GetShamanDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(108271); // Astral Shift
    cooldowns.push_back(108280); // Healing Tide Totem
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetShamanOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(51533);  // Feral Spirit
    cooldowns.push_back(191634); // Stormkeeper
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - MAGE
// ============================================================================

std::vector<uint32> PvPCombatAI::GetMageDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(45438);  // Ice Block
    cooldowns.push_back(55342);  // Mirror Image
    cooldowns.push_back(235219); // Cold Snap
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetMageOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(12042);  // Arcane Power
    cooldowns.push_back(190319); // Combustion
    cooldowns.push_back(12472);  // Icy Veins
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - WARLOCK
// ============================================================================

std::vector<uint32> PvPCombatAI::GetWarlockDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(104773); // Unending Resolve
    cooldowns.push_back(108416); // Dark Pact
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetWarlockOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(113860); // Dark Soul
    cooldowns.push_back(1122);   // Summon Infernal
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - MONK
// ============================================================================

std::vector<uint32> PvPCombatAI::GetMonkDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(122783); // Diffuse Magic
    cooldowns.push_back(122278); // Dampen Harm
    cooldowns.push_back(243435); // Fortifying Brew
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetMonkOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(137639); // Storm, Earth, and Fire
    cooldowns.push_back(152173); // Serenity
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - DRUID
// ============================================================================

std::vector<uint32> PvPCombatAI::GetDruidDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(22812);  // Barkskin
    cooldowns.push_back(61336);  // Survival Instincts
    cooldowns.push_back(108238); // Renewal
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetDruidOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(106951); // Berserk
    cooldowns.push_back(194223); // Celestial Alignment
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - DEMON HUNTER
// ============================================================================

std::vector<uint32> PvPCombatAI::GetDemonHunterDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(196555); // Netherwalk
    cooldowns.push_back(187827); // Metamorphosis
    cooldowns.push_back(198589); // Blur
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetDemonHunterOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(191427); // Metamorphosis (DPS)
    cooldowns.push_back(258920); // Immolation Aura
    return cooldowns;
}

// ============================================================================
// CLASS-SPECIFIC HELPERS - EVOKER
// ============================================================================

std::vector<uint32> PvPCombatAI::GetEvokerDefensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(363916); // Obsidian Scales
    cooldowns.push_back(374348); // Renewing Blaze
    return cooldowns;
}

std::vector<uint32> PvPCombatAI::GetEvokerOffensiveCooldowns() const
{
    std::vector<uint32> cooldowns;
    cooldowns.push_back(375087); // Dragonrage
    return cooldowns;
}

} // namespace Playerbot
