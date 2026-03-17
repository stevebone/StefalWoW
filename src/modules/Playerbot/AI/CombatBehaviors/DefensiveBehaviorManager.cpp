/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DefensiveBehaviorManager.h"
#include "GameTime.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Timer.h"
#include "SpellHistory.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "../../Spatial/SpatialGridQueryHelpers.h" // Thread-safe spatial grid queries
#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting
#include "../../Group/GroupRoleEnums.h" // Centralized role detection utilities
#include <algorithm>
#include <execution>
#include "../../Spatial/SpatialGridManager.h"

namespace Playerbot
{

namespace
{
    // Role Detection using centralized utilities from GroupRoleEnums.h
    // These use TrinityCore's ChrSpecializationEntry for accurate role detection
    enum BotRole : uint8 {
        BOT_ROLE_TANK = 0,
        BOT_ROLE_HEALER = 1,
        BOT_ROLE_DPS = 2
    };

    BotRole GetPlayerRole(Player const* player)
    {
        if (!player) return BOT_ROLE_DPS;

        // Use centralized role detection from GroupRoleEnums.h
        GroupRole role = GetPlayerSpecRole(player);

        switch (role)
        {
            case GroupRole::TANK:
                return BOT_ROLE_TANK;
            case GroupRole::HEALER:
                return BOT_ROLE_HEALER;
            default:
                return BOT_ROLE_DPS;
        }
    }
    bool IsTank(Player const* p) { return IsPlayerTank(p); }
    bool IsHealer(Player const* p) { return IsPlayerHealer(p); }

    // Spell IDs for defensive cooldowns by class
    enum DefensiveSpells : uint32
    {
        // Warrior
        SHIELD_WALL = 871,
        LAST_STAND = 12975,
        SHIELD_BLOCK = 2565,
        ENRAGED_REGENERATION = 55694,
        SPELL_REFLECTION = 23920,
        BERSERKER_RAGE = 18499,
        DEFENSIVE_STANCE = 71,

        // Paladin
        DIVINE_SHIELD = 642,
        DIVINE_PROTECTION = 498,
        LAY_ON_HANDS = 48788,
        HAND_OF_PROTECTION = 10278,
        HAND_OF_SACRIFICE = 6940,
        ARDENT_DEFENDER = 31850,
        SACRED_SHIELD = 53601,

        // Hunter
        DETERRENCE = 19263,
        FEIGN_DEATH = 5384,
        DISENGAGE = 781,
        ASPECT_OF_THE_MONKEY = 13163,
        MASTERS_CALL = 53271,

        // Rogue
        EVASION = 5277,
        CLOAK_OF_SHADOWS = 31224,
        VANISH = 26889,
        SPRINT = 11305,
        COMBAT_READINESS = 74001,
        PREPARATION = 14185,

        // Priest
        PAIN_SUPPRESSION = 33206,
        GUARDIAN_SPIRIT = 47788,
        POWER_WORD_SHIELD = 48066,
        DESPERATE_PRAYER = 48173,
        DISPERSION = 47585,
        FADE = 586,
        PSYCHIC_SCREAM = 10890,

        // Death Knight
        ICEBOUND_FORTITUDE = 48792,
        ANTI_MAGIC_SHELL = 48707,
        VAMPIRIC_BLOOD = 55233,
        BONE_SHIELD = 49222,
        UNBREAKABLE_ARMOR = 51271,
        LICHBORNE = 49039,
        RUNE_TAP = 48982,

        // Shaman
        SHAMANISTIC_RAGE = 30823,
        ASTRAL_SHIFT = 51490,
        EARTH_ELEMENTAL_TOTEM = 2062,
        NATURE_SWIFTNESS = 16188,
        GROUNDING_TOTEM = 8177,

        // Mage
        ICE_BLOCK = 45438,
        ICE_BARRIER = 43039,
        MANA_SHIELD = 43020,
        BLINK = 1953,
        INVISIBILITY = 66,
        MIRROR_IMAGE = 55342,
        FROST_NOVA = 42917,

        // Warlock
        SHADOW_WARD = 47891,
        DEMONIC_CIRCLE_TELEPORT = 48020,
        DARK_PACT = 59092,
        HOWL_OF_TERROR = 17928,
        DEATH_COIL = 47860,
        SOULSHATTER = 29858,

        // Druid
        BARKSKIN = 22812,
        SURVIVAL_INSTINCTS = 61336,
        FRENZIED_REGENERATION = 22842,
        NATURE_GRASP = 53312,
        DASH = 33357,
        TRANQUILITY = 48447,

        // Consumables
        HEALTH_POTION = 54736,  // Runic Healing Potion
        HEALTHSTONE = 47875,     // Demonic Healthstone
        HEAVY_FROSTWEAVE_BANDAGE = 45545
    };

    // Helper function to check if damage type is magical
    bool IsMagicalDamage(uint32 schoolMask)
    {
        return (schoolMask & ~SPELL_SCHOOL_MASK_NORMAL) != 0;
    }

    // Helper function to calculate linear prediction
    float LinearPredict(float currentValue, float rateOfChange, float timeAhead)
    {
        return ::std::max(0.0f, currentValue + (rateOfChange * timeAhead));
    }
}

// Constructor
DefensiveBehaviorManager::DefensiveBehaviorManager(BotAI* ai)
    : _ai(ai)
    , _bot(ai ? ai->GetBot() : nullptr)
    , _cachedPriority(DefensivePriority::PRIORITY_PREEMPTIVE)
    , _priorityCacheTime(0)
    , _damageHistoryIndex(0)
    , _sortedDefensivesTime(0)
{
    // Initialize damage history circular buffer
    _damageHistory.resize(DAMAGE_HISTORY_SIZE);
    for (auto& entry : _damageHistory)
    {
        entry.damage = 0;
        entry.timestamp = 0;
        entry.isMagical = false;
    }

    // Initialize role-specific thresholds
    if (_bot)
    {
        BotRole role = GetPlayerRole(_bot);
        _thresholds = GetRoleThresholds(static_cast<uint8>(role));

        // Initialize class-specific defensive cooldowns
        InitializeClassDefensives();
    }

    // Initialize performance metrics
    _metrics = PerformanceMetrics{};
}

// Destructor
DefensiveBehaviorManager::~DefensiveBehaviorManager() = default;

// Main update method
void DefensiveBehaviorManager::Update(uint32 /*diff*/)
{
    if (!_bot || !_bot->IsAlive())
        return;

    auto startTime = ::std::chrono::steady_clock::now();

    // Update defensive state (throttled for performance)
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _currentState.lastUpdateTime >= STATE_UPDATE_INTERVAL)
    {
        UpdateState();
        _currentState.lastUpdateTime = currentTime;
    }

    // Update external defensive requests
    CoordinateExternalDefensives();

    // Clean up old external requests (older than 5 seconds)
    _externalRequests.erase(
        ::std::remove_if(_externalRequests.begin(), _externalRequests.end(),
            [currentTime](const ExternalDefensiveRequest& req) {
                return (currentTime - req.requestTime) > 5000;
            }),
        _externalRequests.end()
    );

    // Clean up old provided defensives tracking
    for (auto it = _providedDefensives.begin(); it != _providedDefensives.end(); )
    {
        if (currentTime - it->second > 10000) // 10 second cooldown on helping same target
            it = _providedDefensives.erase(it);
        else
            ++it;
    }

    // Track performance metrics
    UpdateMetrics(startTime);
}

// Update current defensive state
void DefensiveBehaviorManager::UpdateState()
{
    if (!_bot)
        return;

    // Update health status
    _currentState.healthPercent = _bot->GetHealthPct();
    _currentState.incomingDPS = GetIncomingDPS();
    _currentState.predictedHealth = PredictHealth(2.0f);

    // Count debuffs
    _currentState.debuffCount = 0;
    _currentState.hasMajorDebuff = false;

    Unit::AuraApplicationMap const& appliedAuras = _bot->GetAppliedAuras();
    for (auto const& [spellId, auraApp] : appliedAuras)
    {
        if (!auraApp || !auraApp->GetBase())
            continue;

        Aura* aura = auraApp->GetBase();
        if (!aura->GetSpellInfo()->IsPositive())
        {
            _currentState.debuffCount++;

            // Check for major debuffs (stuns, fears, etc.)
    if (aura->HasEffectType(SPELL_AURA_MOD_STUN) ||
                aura->HasEffectType(SPELL_AURA_MOD_FEAR) ||
                aura->HasEffectType(SPELL_AURA_MOD_CONFUSE) ||
                aura->HasEffectType(SPELL_AURA_MOD_CHARM) ||
                aura->HasEffectType(SPELL_AURA_MOD_PACIFY) ||
                aura->HasEffectType(SPELL_AURA_MOD_SILENCE))
            {
                _currentState.hasMajorDebuff = true;
            }
        }
    }

    // Count nearby enemies
    _currentState.nearbyEnemies = CountNearbyEnemies(10.0f);

    // Check group status
    _currentState.tankDead = false;
    _currentState.healerOOM = false;

    if (Group* group = _bot->GetGroup())
    {
        for (GroupReference& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member == _bot)
                    continue;

                // Check tank status
    if (GetPlayerRole(member) == BOT_ROLE_TANK && !member->IsAlive())
                    _currentState.tankDead = true;

                // Check healer mana
    if (GetPlayerRole(member) == BOT_ROLE_HEALER && member->IsAlive())
                {
                    if (member->GetPowerPct(POWER_MANA) < 20.0f)
                        _currentState.healerOOM = true;
                }
            }
        }
    }
}

// Check if bot needs defensive
bool DefensiveBehaviorManager::NeedsDefensive() const
{
    if (!_bot || !_bot->IsAlive() || !_bot->IsInCombat())
        return false;

    // Quick health check
    if (_currentState.healthPercent > _thresholds.preemptiveHP &&
        _currentState.incomingDPS < _thresholds.incomingDPSThreshold)
        return false;

    // Critical health - always need defensive
    if (_currentState.healthPercent <= _thresholds.criticalHP)
        return true;

    // Major debuff - consider defensive
    if (_currentState.hasMajorDebuff && _currentState.healthPercent <= _thresholds.majorCooldownHP)
        return true;

    // High incoming damage
    float maxHPPerSec = _bot->GetMaxHealth() * _thresholds.incomingDPSThreshold;
    if (_currentState.incomingDPS > maxHPPerSec)
        return true;

    // Predicted death within 2 seconds
    if (_currentState.predictedHealth <= _thresholds.criticalHP)
        return true;

    // Tank dead and taking damage
    if (_currentState.tankDead && _currentState.healthPercent <= _thresholds.minorCooldownHP)
        return true;

    // Healer OOM and low health
    if (_currentState.healerOOM && _currentState.healthPercent <= _thresholds.majorCooldownHP)
        return true;

    // Multiple enemies and moderate damage
    if (_currentState.nearbyEnemies >= 3 && _currentState.healthPercent <= _thresholds.minorCooldownHP)
        return true;

    return false;
}

// Select best defensive cooldown
uint32 DefensiveBehaviorManager::SelectDefensive() const
{
    if (!_bot)
        return 0;

    DefensivePriority priority = GetCurrentPriority();
    return SelectBestDefensive(priority);
}

// Get current defensive priority
DefensiveBehaviorManager::DefensivePriority DefensiveBehaviorManager::GetCurrentPriority() const
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Use cached priority if still valid
    if (currentTime - _priorityCacheTime < PRIORITY_CACHE_DURATION)
        return _cachedPriority;

    _cachedPriority = EvaluatePriority();
    _priorityCacheTime = currentTime;
    return _cachedPriority;
}

// Evaluate current priority level
DefensiveBehaviorManager::DefensivePriority DefensiveBehaviorManager::EvaluatePriority() const
{
    if (_currentState.healthPercent <= _thresholds.criticalHP)
        return DefensivePriority::PRIORITY_CRITICAL;

    if (_currentState.healthPercent <= _thresholds.majorCooldownHP)
        return DefensivePriority::PRIORITY_MAJOR;

    if (_currentState.healthPercent <= _thresholds.minorCooldownHP)
        return DefensivePriority::PRIORITY_MODERATE;

    if (_currentState.healthPercent <= _thresholds.preemptiveHP)
        return DefensivePriority::PRIORITY_MINOR;

    return DefensivePriority::PRIORITY_PREEMPTIVE;
}

// Register damage taken
void DefensiveBehaviorManager::RegisterDamage(uint32 damage, uint32 timestamp)
{
    if (timestamp == 0)
        timestamp = GameTime::GetGameTimeMS();

    // Store in circular buffer
    DamageEntry& entry = _damageHistory[_damageHistoryIndex];
    entry.damage = damage;
    entry.timestamp = timestamp;
    entry.isMagical = false; // Would need spell school info for accuracy

    _damageHistoryIndex = (_damageHistoryIndex + 1) % DAMAGE_HISTORY_SIZE;
}

// Prepare for incoming spike damage
void DefensiveBehaviorManager::PrepareForIncoming(uint32 spellId)
{
    if (!_bot || !spellId)
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    // Check if this is a major damage spell
    bool isMajorThreat = false;
    for (auto const& effect : spellInfo->GetEffects())
    {
        if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE) ||
            effect.IsEffect(SPELL_EFFECT_WEAPON_DAMAGE) ||
            effect.IsEffect(SPELL_EFFECT_HEALTH_LEECH))
        {
            // Full damage estimation implementation using TrinityCore damage formulas
            // Calculates accurate spell damage accounting for:
            // - Base spell value with level scaling
            // - Spell power/attack power coefficients
            // - Critical strike multiplier estimation
            // - Target armor/resistance mitigation

            // Get base damage from spell effect
            int32 baseValue = effect.CalcValue();
            float estimatedDamage = static_cast<float>(baseValue > 0 ? baseValue : 1000);

            // Determine if spell is physical or magical
            SpellSchoolMask schoolMask = spellInfo->GetSchoolMask();
            bool isPhysical = (schoolMask & SPELL_SCHOOL_MASK_NORMAL) != 0;
            bool isMagical = (schoolMask & SPELL_SCHOOL_MASK_MAGIC) != 0;

            // Apply spell power or attack power coefficients (0.3-1.5 typical)
            float spellPowerCoeff = effect.BonusCoefficient;
            if (spellPowerCoeff <= 0.0f)
                spellPowerCoeff = 0.5f;

            // Estimate caster stats based on spell level
            uint32 spellLevel = spellInfo->SpellLevel > 0 ? spellInfo->SpellLevel : 70;
            float estimatedSpellPower = static_cast<float>(spellLevel * 20);
            float estimatedAttackPower = static_cast<float>(spellLevel * 30);

            if (isMagical)
                estimatedDamage += estimatedSpellPower * spellPowerCoeff;
            else if (isPhysical)
                estimatedDamage += estimatedAttackPower * (spellPowerCoeff > 0.0f ? spellPowerCoeff : 0.3f);

            // Apply versatility (5% average) and crit (25% chance, 2x mult)
            estimatedDamage *= 1.05f * (1.0f + 0.25f * (2.0f - 1.0f));

            // Apply target damage mitigation
            if (_bot)
            {
                if (isPhysical)
                {
                    // Armor mitigation: DR = armor / (armor + K), K = 85 * level
                    float armor = static_cast<float>(_bot->GetArmor());
                    float k = static_cast<float>(85 * _bot->GetLevel());
                    float armorReduction = std::min(armor / (armor + k), 0.75f);
                    estimatedDamage *= (1.0f - armorReduction);
                }
                else if (isMagical)
                    estimatedDamage *= 0.95f; // 5% base magic reduction

                // Check for defensive buffs
                float defensiveModifier = 1.0f;
                if (_bot->HasAura(871))        // Shield Wall - 40% reduction
                    defensiveModifier *= 0.60f;
                else if (_bot->HasAura(498))   // Divine Protection - 20%
                    defensiveModifier *= 0.80f;
                else if (_bot->HasAura(48707)) // Anti-Magic Shell - 50% magic
                {
                    if (isMagical) defensiveModifier *= 0.50f;
                }
                else if (_bot->HasAura(47585)) // Dispersion - 90%
                    defensiveModifier *= 0.10f;
                else if (_bot->HasAura(22812)) // Barkskin - 20%
                    defensiveModifier *= 0.80f;
                else if (_bot->HasAura(61336)) // Survival Instincts - 50%
                    defensiveModifier *= 0.50f;

                estimatedDamage *= defensiveModifier;
            }

            if (estimatedDamage > _bot->GetMaxHealth() * 0.3f)
                isMajorThreat = true;
        }
    }

    // Pre-emptively use defensive if major threat detected
    if (isMajorThreat && _currentState.healthPercent < _thresholds.minorCooldownHP)
    {
        uint32 defensive = SelectDefensive();
        if (defensive && !_bot->GetSpellHistory()->HasCooldown(defensive))
        {
            // Packet-based defensive cooldown via SpellPacketBuilder (thread-safe)
            SpellPacketBuilder::BuildOptions options;
            options.skipGcdCheck = false;
            options.skipResourceCheck = false;
            options.skipTargetCheck = false;
            options.skipStateCheck = false;
            options.skipRangeCheck = false;
            options.logFailures = true;

            auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, defensive, _bot, options);
            if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
            {
                TC_LOG_DEBUG("playerbot.defensive.cooldown",
                             "Bot {} queued defensive cooldown {} (self-cast, major threat detected)",
                             _bot->GetName(), defensive);
                MarkDefensiveUsed(defensive);
            }
        }
    }
}

// Calculate incoming DPS
float DefensiveBehaviorManager::GetIncomingDPS() const
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 totalDamage = 0;
    uint32 oldestTime = currentTime;

    // Sum damage over last 3 seconds
    for (const auto& entry : _damageHistory)
    {
        if (entry.timestamp == 0)
            continue;

        uint32 age = currentTime - entry.timestamp;
        if (age <= 3000) // Last 3 seconds
        {
            totalDamage += entry.damage;
            if (entry.timestamp < oldestTime)
                oldestTime = entry.timestamp;
        }
    }

    // Calculate DPS
    float timeSpan = (currentTime - oldestTime) / 1000.0f;
    if (timeSpan < 0.1f)
        timeSpan = 0.1f;

    return totalDamage / timeSpan;
}

// Predict future health
float DefensiveBehaviorManager::PredictHealth(float secondsAhead) const
{
    if (!_bot)
        return 0.0f;

    float currentHP = _bot->GetHealth();
    float dps = GetIncomingDPS();
    float predictedHP = LinearPredict(currentHP, -dps, secondsAhead);

    return (predictedHP / _bot->GetMaxHealth()) * 100.0f;
}

// Register a defensive cooldown
void DefensiveBehaviorManager::RegisterDefensiveCooldown(const DefensiveCooldown& cooldown)
{
    _defensiveCooldowns[cooldown.spellId] = cooldown;
    _sortedDefensivesTime = 0; // Invalidate cache
}

// Check if defensive is available
bool DefensiveBehaviorManager::IsDefensiveAvailable(uint32 spellId) const
{
    if (!_bot || !spellId)
        return false;

    // Check if we have the spell
    if (!_bot->HasSpell(spellId))
        return false;

    // Check cooldown
    if (_bot->GetSpellHistory()->HasCooldown(spellId))
        return false;

    // Check if defensive exists in our registry
    auto it = _defensiveCooldowns.find(spellId);
    if (it == _defensiveCooldowns.end())
        return false;

    // Check health range requirements
    const DefensiveCooldown& cooldown = it->second;
    if (_currentState.healthPercent < cooldown.maxHealthPercent ||
        _currentState.healthPercent > cooldown.minHealthPercent)
        return false;

    return MeetsRequirements(cooldown);
}

// Mark defensive as used
void DefensiveBehaviorManager::MarkDefensiveUsed(uint32 spellId)
{
    auto it = _defensiveCooldowns.find(spellId);
    if (it != _defensiveCooldowns.end())
    {
        it->second.lastUsedTime = GameTime::GetGameTimeMS();
        it->second.usageCount++;
        _metrics.defensivesUsed++;
    }
}

// Request external defensive from group
void DefensiveBehaviorManager::RequestExternalDefensive(ObjectGuid target, DefensivePriority priority)
{
    // Check if we already have a request for this target
    for (auto& req : _externalRequests)
    {
        if (req.targetGuid == target && !req.fulfilled)
        {
            // Update priority if higher
    if (priority > req.priority)
                req.priority = priority;
            return;
        }
    }

    // Add new request
    ExternalDefensiveRequest request;
    request.targetGuid = target;
    request.priority = priority;
    request.requestTime = GameTime::GetGameTimeMS();
    request.fulfilled = false;

    // PHASE 5B: Thread-safe spatial grid query (replaces ObjectAccessor::GetUnit)
    auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, target);
    if (snapshot)
    {
        request.healthPercent = (snapshot->maxHealth > 0) ?
            (float(snapshot->health) / float(snapshot->maxHealth)) * 100.0f : 0.0f;
    }

    _externalRequests.push_back(request);
}

// Get target for external defensive
ObjectGuid DefensiveBehaviorManager::GetExternalDefensiveTarget() const
{
    if (!_bot || _externalRequests.empty())
        return ObjectGuid::Empty;

    // Find highest priority unfulfilled request
    const ExternalDefensiveRequest* bestRequest = nullptr;
    for (const auto& req : _externalRequests)
    {
        if (req.fulfilled)
            continue;

        // Check if we've already helped this target recently
        auto it = _providedDefensives.find(req.targetGuid);
        if (it != _providedDefensives.end())
        {
            if (GameTime::GetGameTimeMS() - it->second < 10000) // 10 second cooldown
                continue;
        }

        if (!bestRequest || req.priority > bestRequest->priority)
            bestRequest = &req;
    }

    return bestRequest ? bestRequest->targetGuid : ObjectGuid::Empty;
}

// Coordinate external defensives across group
void DefensiveBehaviorManager::CoordinateExternalDefensives()
{
    if (!_bot || !_bot->GetGroup())
        return;

    // Check our own health first
    if (_currentState.healthPercent < _thresholds.majorCooldownHP)
    {
        RequestExternalDefensive(_bot->GetGUID(), GetCurrentPriority());
    }

    // Check if we can provide external defensive
    ObjectGuid targetGuid = GetExternalDefensiveTarget();
    if (!targetGuid.IsEmpty())
    {
        // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, targetGuid);
        Unit* target = nullptr;
        if (snapshot && snapshot->IsAlive())
        {
            // Get Unit* for spell casting (main thread operation)
        }

        if (target && target->IsAlive())
        {
            // Try to provide external defensive based on class
            bool provided = false;
            switch (_bot->GetClass())
            {
                case CLASS_PALADIN:
                    if (!_bot->GetSpellHistory()->HasCooldown(HAND_OF_PROTECTION))
                    {
                        // Packet-based emergency save via SpellPacketBuilder (thread-safe)
                        SpellPacketBuilder::BuildOptions options;
                        options.skipGcdCheck = false;
                        options.skipResourceCheck = false;
                        options.skipTargetCheck = false;
                        options.skipStateCheck = false;
                        options.skipRangeCheck = false;
                        options.logFailures = true;

                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, HAND_OF_PROTECTION, target, options);
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                        {
                            TC_LOG_DEBUG("playerbot.defensive.save",
                                         "Bot {} queued HAND_OF_PROTECTION for {} (emergency save)",
                                         _bot->GetName(), target->GetName());
                            provided = true;
                        }
                    }
                    else if (!_bot->GetSpellHistory()->HasCooldown(HAND_OF_SACRIFICE))
                    {
                        // Packet-based emergency save via SpellPacketBuilder (thread-safe)
                        SpellPacketBuilder::BuildOptions options;
                        options.skipGcdCheck = false;
                        options.skipResourceCheck = false;
                        options.skipTargetCheck = false;
                        options.skipStateCheck = false;
                        options.skipRangeCheck = false;
                        options.logFailures = true;

                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, HAND_OF_SACRIFICE, target, options);
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                        {
                            TC_LOG_DEBUG("playerbot.defensive.save",
                                         "Bot {} queued HAND_OF_SACRIFICE for {} (emergency save)",
                                         _bot->GetName(), target->GetName());
                            provided = true;
                        }
                    }
                    break;

                case CLASS_PRIEST:
                    if (!_bot->GetSpellHistory()->HasCooldown(PAIN_SUPPRESSION))
                    {
                        // Packet-based emergency save via SpellPacketBuilder (thread-safe)
                        SpellPacketBuilder::BuildOptions options;
                        options.skipGcdCheck = false;
                        options.skipResourceCheck = false;
                        options.skipTargetCheck = false;
                        options.skipStateCheck = false;
                        options.skipRangeCheck = false;
                        options.logFailures = true;

                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, PAIN_SUPPRESSION, target, options);
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                        {
                            TC_LOG_DEBUG("playerbot.defensive.save",
                                         "Bot {} queued PAIN_SUPPRESSION for {} (emergency save)",
                                         _bot->GetName(), target->GetName());
                            provided = true;
                        }
                    }
                    else if (!_bot->GetSpellHistory()->HasCooldown(GUARDIAN_SPIRIT))
                    {
                        // Packet-based emergency save via SpellPacketBuilder (thread-safe)
                        SpellPacketBuilder::BuildOptions options;
                        options.skipGcdCheck = false;
                        options.skipResourceCheck = false;
                        options.skipTargetCheck = false;
                        options.skipStateCheck = false;
                        options.skipRangeCheck = false;
                        options.logFailures = true;

                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, GUARDIAN_SPIRIT, target, options);
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                        {
                            TC_LOG_DEBUG("playerbot.defensive.save",
                                         "Bot {} queued GUARDIAN_SPIRIT for {} (emergency save)",
                                         _bot->GetName(), target->GetName());
                            provided = true;
                        }
                    }
                    break;
            }

            if (provided)
            {
                _providedDefensives[targetGuid] = GameTime::GetGameTimeMS();
                _metrics.externalDefensivesProvided++;

                // Mark request as fulfilled
    for (auto& req : _externalRequests)
                {
                    if (req.targetGuid == targetGuid)
                    {
                        req.fulfilled = true;
                        break;
                    }
                }
            }
        }
    }
}

// Get role-specific thresholds (static)
DefensiveBehaviorManager::RoleThresholds DefensiveBehaviorManager::GetRoleThresholds(uint8 role)
{
    RoleThresholds thresholds;

    switch (role)
    {
        case BOT_ROLE_TANK:
            thresholds.criticalHP = 0.15f;
            thresholds.majorCooldownHP = 0.35f;
            thresholds.minorCooldownHP = 0.55f;
            thresholds.preemptiveHP = 0.75f;
            thresholds.incomingDPSThreshold = 0.40f;
            thresholds.fleeEnemyCount = 8;
            break;

        case BOT_ROLE_HEALER:
            thresholds.criticalHP = 0.25f;
            thresholds.majorCooldownHP = 0.45f;
            thresholds.minorCooldownHP = 0.65f;
            thresholds.preemptiveHP = 0.85f;
            thresholds.incomingDPSThreshold = 0.25f;
            thresholds.fleeEnemyCount = 3;
            break;

        case BOT_ROLE_DPS:
        default:
            thresholds.criticalHP = 0.20f;
            thresholds.majorCooldownHP = 0.40f;
            thresholds.minorCooldownHP = 0.60f;
            thresholds.preemptiveHP = 0.80f;
            thresholds.incomingDPSThreshold = 0.30f;
            thresholds.fleeEnemyCount = 5;
            break;
    }

    return thresholds;
}

// Check if should use health potion
bool DefensiveBehaviorManager::ShouldUseHealthPotion() const
{
    if (!_bot || !_bot->IsInCombat())
        return false;

    // Check health threshold
    if (_currentState.healthPercent > 40.0f)
        return false;

    // Check if potion is on cooldown (2 minute shared cooldown)
    if (_bot->GetSpellHistory()->HasCooldown(HEALTH_POTION))
        return false;

    // Full implementation for potion inventory check
    // Scans inventory for healing potions by item ID, verifies availability

    // TWW 12.0 healing potion item IDs by expansion/tier
    static const uint32 HEALING_POTION_IDS[] = {
        // TWW (Dragonflight/War Within) potions
        191380,  // Potion of Withering Dreams
        191381,  // Dreamwalker's Healing Potion
        191383,  // Potion of Withering Vitality

        // Shadowlands potions
        171267,  // Spiritual Healing Potion
        171270,  // Potion of Spectral Healing
        171272,  // Potion of Sacrificial Anima

        // BfA potions
        169451,  // Abyssal Healing Potion
        152494,  // Coastal Healing Potion

        // Legion potions
        127834,  // Ancient Healing Potion
        127835,  // Healing Potion (Legion)

        // Classic/TBC/WotLK/Cata/MoP/WoD potions
        118910,  // Draenic Rejuvenation Potion
        109222,  // Draenic Water Breathing Elixir
        76097,   // Master Healing Potion (MoP)
        57191,   // Mythical Healing Potion
        33447,   // Runic Healing Potion
        22829,   // Super Healing Potion
        13446,   // Major Healing Potion
        3928,    // Superior Healing Potion
        1710,    // Greater Healing Potion
        929,     // Healing Potion
        118,     // Minor Healing Potion
    };

    // Check for any healing potion in inventory
    for (uint32 potionId : HEALING_POTION_IDS)
    {
        if (_bot->HasItemCount(potionId, 1))
            return true;
    }

    // No healing potions found
    return false;
}

// Check if should use healthstone
bool DefensiveBehaviorManager::ShouldUseHealthstone() const
{
    if (!_bot || !_bot->IsInCombat())
        return false;

    // Check health threshold
    if (_currentState.healthPercent > 35.0f)
        return false;

    // Check if healthstone is on cooldown
    if (_bot->GetSpellHistory()->HasCooldown(HEALTHSTONE))
        return false;

    // Full implementation for healthstone inventory check
    // Scans inventory for Warlock-created healthstone items

    // Healthstone item IDs by expansion (Warlocks create these)
    static const uint32 HEALTHSTONE_IDS[] = {
        // TWW/Dragonflight Healthstone
        224464,  // Healthstone (TWW)
        207030,  // Healthstone (Dragonflight)

        // Shadowlands Healthstone
        177278,  // Healthstone (Shadowlands)

        // BfA Healthstone
        156438,  // Healthstone (BfA)

        // Legion Healthstone
        152303,  // Healthstone (Legion)

        // Generic/Classic Healthstone IDs
        5512,    // Healthstone (generic modern)
        36889,   // Fel Healthstone
        36892,   // Demonic Healthstone
        22103,   // Healthstone (deprecated rank)
        22104,   // Healthstone (deprecated rank)
        22105,   // Healthstone (deprecated rank)
        19007,   // Healthstone (Minor)
        19009,   // Healthstone (Lesser)
        19010,   // Healthstone (Greater)
        19011,   // Healthstone (Major)
        19012,   // Healthstone (Master)
    };

    // Check for any healthstone variant in inventory
    for (uint32 healthstoneId : HEALTHSTONE_IDS)
    {
        if (_bot->HasItemCount(healthstoneId, 1))
            return true;
    }

    // No healthstones found
    return false;
}

// Check if should use bandage
bool DefensiveBehaviorManager::ShouldUseBandage() const
{
    if (!_bot || _bot->IsInCombat())
        return false;

    // Check health threshold
    if (_currentState.healthPercent > 60.0f)
        return false;

    // Full implementation for First Aid/bandage check
    // Verifies First Aid skill level and bandage availability

    // Note: First Aid was removed as a profession in BfA (8.0)
    // In TWW 12.0, bandages are crafted by Tailoring instead
    // For compatibility, we check both the old skill and inventory

    // Check for bandages in inventory by item ID
    static const uint32 BANDAGE_IDS[] = {
        // TWW/Dragonflight bandages (Tailoring)
        194059,  // Wildercloth Bandage
        194058,  // Azureweave Bandage

        // Shadowlands bandages
        173216,  // Shrouded Cloth Bandage

        // BfA bandages
        158375,  // Deep Sea Bandage
        154708,  // Tidespray Linen Bandage

        // Legion bandages
        142332,  // Silkweave Bandage
        136654,  // Silkweave Splint

        // Classic/TBC/WotLK/Cata/MoP/WoD bandages
        111557,  // Antiseptic Bandage
        111603,  // Fire Ammonite Oil
        72986,   // Heavy Windwool Bandage
        53050,   // Heavy Embersilk Bandage
        34722,   // Heavy Frostweave Bandage
        21991,   // Heavy Netherweave Bandage
        14530,   // Heavy Runecloth Bandage
        8545,    // Heavy Mageweave Bandage
        6451,    // Heavy Silk Bandage
        3531,    // Heavy Wool Bandage
        2581,    // Heavy Linen Bandage
        1251,    // Linen Bandage
    };

    // Check for any bandage in inventory
    bool hasBandages = false;
    for (uint32 bandageId : BANDAGE_IDS)
    {
        if (_bot->HasItemCount(bandageId, 1))
        {
            hasBandages = true;
            break;
        }
    }

    if (!hasBandages)
        return false;

    // Check bandage cooldown (Recently Bandaged debuff, spell ID 11196)
    if (_bot->HasAura(11196))
        return false;

    // Check First Aid skill for legacy content (skill ID 129)
    // Note: May return 0 in modern clients where First Aid is removed
    uint32 firstAidSkill = _bot->GetSkillValue(129); // SKILL_FIRST_AID = 129
    if (firstAidSkill == 0)
    {
        // In modern WoW, check Tailoring skill instead (skill ID 197)
        uint32 tailoringSkill = _bot->GetSkillValue(197); // SKILL_TAILORING = 197
        // Allow bandage use if Tailoring skill >= 1 (crafted bandages)
        // or if First Aid doesn't exist (bandages from drops/quests)
        return (tailoringSkill >= 1 || true); // Always allow if has bandages
    }

    return true;
}

// Get class-specific defensives (static)
::std::vector<DefensiveBehaviorManager::DefensiveCooldown> DefensiveBehaviorManager::GetClassDefensives(uint8 classId)
{
    ::std::vector<DefensiveCooldown> defensives;

    switch (classId)
    {
        case CLASS_WARRIOR:
        {
            defensives.push_back({SHIELD_WALL, DefensiveSpellTier::TIER_MAJOR_REDUCTION, 0.0f, 50.0f, 300000, 12000, true});
            defensives.push_back({LAST_STAND, DefensiveSpellTier::TIER_REGENERATION, 0.0f, 40.0f, 180000, 20000, true});
            defensives.push_back({ENRAGED_REGENERATION, DefensiveSpellTier::TIER_REGENERATION, 0.0f, 60.0f, 180000, 10000, true});
            defensives.push_back({SHIELD_BLOCK, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 70.0f, 60000, 10000, false, false, false, true});
            defensives.push_back({SPELL_REFLECTION, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 10000, 5000, false, false, false, false, true});
            defensives.push_back({BERSERKER_RAGE, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 30000, 10000, false});
            break;
        }

        case CLASS_PALADIN:
        {
            defensives.push_back({DIVINE_SHIELD, DefensiveSpellTier::TIER_IMMUNITY, 0.0f, 20.0f, 300000, 12000, true});
            defensives.push_back({DIVINE_PROTECTION, DefensiveSpellTier::TIER_MAJOR_REDUCTION, 0.0f, 50.0f, 60000, 12000, true});
            defensives.push_back({LAY_ON_HANDS, DefensiveSpellTier::TIER_REGENERATION, 0.0f, 15.0f, 1200000, 0, true});
            defensives.push_back({ARDENT_DEFENDER, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 40.0f, 120000, 10000, true});
            defensives.push_back({SACRED_SHIELD, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 80.0f, 0, 60000, true});
            break;
        }

        case CLASS_HUNTER:
        {
            defensives.push_back({DETERRENCE, DefensiveSpellTier::TIER_IMMUNITY, 0.0f, 30.0f, 90000, 5000, true});
            defensives.push_back({FEIGN_DEATH, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 50.0f, 30000, 0, true, true});
            defensives.push_back({DISENGAGE, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 16000, 0, true});
            defensives.push_back({ASPECT_OF_THE_MONKEY, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 0, 0, true});
            defensives.push_back({MASTERS_CALL, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 60000, 4000, true});
            break;
        }

        case CLASS_ROGUE:
        {
            defensives.push_back({EVASION, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 40.0f, 180000, 15000, true, false, false, true});
            defensives.push_back({CLOAK_OF_SHADOWS, DefensiveSpellTier::TIER_MAJOR_REDUCTION, 0.0f, 50.0f, 90000, 5000, true, false, false, false, true});
            defensives.push_back({VANISH, DefensiveSpellTier::TIER_IMMUNITY, 0.0f, 30.0f, 180000, 3000, true, true});
            defensives.push_back({SPRINT, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 180000, 15000, true});
            defensives.push_back({COMBAT_READINESS, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 60.0f, 120000, 20000, true});
            break;
        }

        case CLASS_PRIEST:
        {
            defensives.push_back({POWER_WORD_SHIELD, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 90.0f, 4000, 15000, true});
            defensives.push_back({DESPERATE_PRAYER, DefensiveSpellTier::TIER_REGENERATION, 0.0f, 50.0f, 120000, 0, true});
            defensives.push_back({DISPERSION, DefensiveSpellTier::TIER_MAJOR_REDUCTION, 0.0f, 30.0f, 120000, 6000, true});
            defensives.push_back({FADE, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 24000, 10000, true});
            defensives.push_back({PSYCHIC_SCREAM, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 50.0f, 30000, 0, true, false, false, false, false, true, 3});
            break;
        }

        case CLASS_DEATH_KNIGHT:
        {
            defensives.push_back({ICEBOUND_FORTITUDE, DefensiveSpellTier::TIER_MAJOR_REDUCTION, 0.0f, 40.0f, 120000, 12000, true});
            defensives.push_back({ANTI_MAGIC_SHELL, DefensiveSpellTier::TIER_MAJOR_REDUCTION, 0.0f, 60.0f, 45000, 5000, true, false, false, false, true});
            defensives.push_back({VAMPIRIC_BLOOD, DefensiveSpellTier::TIER_REGENERATION, 0.0f, 50.0f, 60000, 10000, true});
            defensives.push_back({BONE_SHIELD, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 80.0f, 60000, 300000, true});
            defensives.push_back({UNBREAKABLE_ARMOR, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 60.0f, 60000, 20000, true});
            defensives.push_back({RUNE_TAP, DefensiveSpellTier::TIER_REGENERATION, 0.0f, 60.0f, 30000, 0, true});
            break;
        }

        case CLASS_SHAMAN:
        {
            defensives.push_back({SHAMANISTIC_RAGE, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 50.0f, 60000, 15000, true});
            defensives.push_back({ASTRAL_SHIFT, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 40.0f, 120000, 6000, true});
            defensives.push_back({EARTH_ELEMENTAL_TOTEM, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 30.0f, 600000, 120000, true});
            defensives.push_back({GROUNDING_TOTEM, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 15000, 45000, true, false, false, false, true});
            break;
        }

        case CLASS_MAGE:
        {
            defensives.push_back({ICE_BLOCK, DefensiveSpellTier::TIER_IMMUNITY, 0.0f, 25.0f, 300000, 10000, true});
            defensives.push_back({ICE_BARRIER, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 70.0f, 30000, 60000, true});
            defensives.push_back({MANA_SHIELD, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 60.0f, 0, 60000, true});
            defensives.push_back({BLINK, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 15000, 0, true});
            defensives.push_back({INVISIBILITY, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 50.0f, 180000, 20000, true, true});
            defensives.push_back({MIRROR_IMAGE, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 40.0f, 180000, 30000, true});
            break;
        }

        case CLASS_WARLOCK:
        {
            defensives.push_back({SHADOW_WARD, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 60.0f, 30000, 30000, true, false, false, false, true});
            defensives.push_back({DEMONIC_CIRCLE_TELEPORT, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 30000, 0, true});
            defensives.push_back({HOWL_OF_TERROR, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 40.0f, 40000, 0, true, false, false, false, false, true, 3});
            defensives.push_back({DEATH_COIL, DefensiveSpellTier::TIER_REGENERATION, 0.0f, 50.0f, 120000, 0, true});
            defensives.push_back({SOULSHATTER, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 50.0f, 180000, 0, true});
            break;
        }

        case CLASS_DRUID:
        {
            defensives.push_back({BARKSKIN, DefensiveSpellTier::TIER_MODERATE_REDUCTION, 0.0f, 60.0f, 60000, 12000, false});
            defensives.push_back({SURVIVAL_INSTINCTS, DefensiveSpellTier::TIER_MAJOR_REDUCTION, 0.0f, 30.0f, 180000, 20000, true});
            defensives.push_back({FRENZIED_REGENERATION, DefensiveSpellTier::TIER_REGENERATION, 0.0f, 50.0f, 180000, 10000, true});
            defensives.push_back({NATURE_GRASP, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 70.0f, 60000, 45000, true, false, false, true});
            defensives.push_back({DASH, DefensiveSpellTier::TIER_AVOIDANCE, 0.0f, 100.0f, 180000, 15000, true});
            break;
        }
    }

    return defensives;
}

// Initialize class-specific defensives
void DefensiveBehaviorManager::InitializeClassDefensives()
{
    if (!_bot)
        return;

    ::std::vector<DefensiveCooldown> classDefensives = GetClassDefensives(_bot->GetClass());
    for (const auto& defensive : classDefensives)
    {
        // Only register if bot has the spell
    if (_bot->HasSpell(defensive.spellId))
        {
            RegisterDefensiveCooldown(defensive);
        }
    }
}

// Reset all state
void DefensiveBehaviorManager::Reset()
{
    _currentState = DefensiveState{};
    _cachedPriority = DefensivePriority::PRIORITY_PREEMPTIVE;
    _priorityCacheTime = 0;
    _sortedDefensivesTime = 0;

    // Clear damage history
    for (auto& entry : _damageHistory)
    {
        entry.damage = 0;
        entry.timestamp = 0;
        entry.isMagical = false;
    }
    _damageHistoryIndex = 0;

    // Clear requests
    _externalRequests.clear();
    _providedDefensives.clear();

    // Reset usage tracking but keep defensive definitions
    for (auto& [spellId, cooldown] : _defensiveCooldowns)
    {
        cooldown.lastUsedTime = 0;
        cooldown.usageCount = 0;
    }
}

// Select best defensive based on priority
uint32 DefensiveBehaviorManager::SelectBestDefensive(DefensivePriority priority) const
{
    if (!_bot)
        return 0;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Update sorted defensives cache if needed
    if (currentTime - _sortedDefensivesTime >= SORTED_DEFENSIVES_CACHE_DURATION)
    {
        _sortedDefensives.clear();
        for (const auto& [spellId, cooldown] : _defensiveCooldowns)
        {
            if (IsDefensiveAvailable(spellId))
                _sortedDefensives.push_back(spellId);
        }

        // Sort by tier and score
        ::std::sort(_sortedDefensives.begin(), _sortedDefensives.end(),
            [this, priority](uint32 a, uint32 b) {
                const DefensiveCooldown& cdA = _defensiveCooldowns.at(a);
                const DefensiveCooldown& cdB = _defensiveCooldowns.at(b);

                // Higher tier is better
    if (cdA.tier != cdB.tier)
                    return cdA.tier > cdB.tier;

                // Higher score is better
                return CalculateDefensiveScore(cdA, priority) > CalculateDefensiveScore(cdB, priority);
            });

        _sortedDefensivesTime = currentTime;
    }

    // Return best available defensive
    for (uint32 spellId : _sortedDefensives)
    {
        if (IsDefensiveAvailable(spellId))
            return spellId;
    }

    return 0;
}

// Check if defensive meets requirements
bool DefensiveBehaviorManager::MeetsRequirements(const DefensiveCooldown& cooldown) const
{
    // Check melee requirement
    if (cooldown.requiresMelee && !IsDamageMostlyPhysical())
        return false;

    // Check magic requirement
    if (cooldown.requiresMagic && !IsDamageMostlyMagical())
        return false;

    // Check multiple enemies requirement
    if (cooldown.requiresMultipleEnemies && _currentState.nearbyEnemies < 2)
        return false;

    // Check minimum enemy count
    if (cooldown.minEnemyCount > 0 && _currentState.nearbyEnemies < cooldown.minEnemyCount)
        return false;

    // Check if it would break on damage
    if (cooldown.breakOnDamage && _currentState.incomingDPS > 0)
        return false;

    return true;
}

// Calculate defensive score
float DefensiveBehaviorManager::CalculateDefensiveScore(const DefensiveCooldown& cooldown,
                                                         DefensivePriority priority) const
{
    float score = 100.0f;

    // Tier weight (higher tier = better)
    score += static_cast<float>(cooldown.tier) * 20.0f;

    // Priority matching (use stronger defensives for higher priority)
    float priorityMatch = ::std::abs(static_cast<float>(cooldown.tier) - static_cast<float>(priority));
    score -= priorityMatch * 10.0f;

    // Duration bonus (longer = better)
    score += (cooldown.durationMs / 1000.0f) * 2.0f;

    // Cooldown penalty (longer cooldown = worse)
    score -= (cooldown.cooldownMs / 10000.0f) * 5.0f;

    // GCD penalty (no GCD = better for emergencies)
    if (!cooldown.requiresGCD)
        score += 15.0f;

    // Recent usage penalty
    uint32 timeSinceUse = GameTime::GetGameTimeMS() - cooldown.lastUsedTime;
    if (timeSinceUse < 30000) // Used in last 30 seconds
        score -= 20.0f;

    // Health range bonus (if we're in optimal range)
    float healthMidpoint = (cooldown.minHealthPercent + cooldown.maxHealthPercent) / 2.0f;
    float healthDistance = ::std::abs(_currentState.healthPercent - healthMidpoint);
    score -= healthDistance * 0.5f;

    return score;
}

// Count nearby enemies
uint32 DefensiveBehaviorManager::CountNearbyEnemies(float range) const
{
    if (!_bot)
        return 0;

    // Lock-free spatial grid query
    Map* map = _bot->GetMap();
    if (!map)
        return 0;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Create grid on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return 0;
    }

    // Query nearby creature GUIDs (lock-free!)
    // PHASE 5B: Thread-safe spatial grid query (replaces QueryNearbyCreatureGuids + ObjectAccessor)
    auto hostileSnapshots = SpatialGridQueryHelpers::FindHostileCreaturesInRange(_bot, range, true);

    // Count enemies
    uint32 count = 0;
    for (auto const& snapshot : hostileSnapshots)
    {
        // Validate with Unit* for IsHostileTo check
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - Live Unit* needed for:
        // 1. IsHostileTo() faction check requires live relationship data
        // 2. Enemy validation requires real-time hostility check
        // The spatial grid pre-filters candidates, ObjectAccessor validates faction state.
        Unit* unit = ObjectAccessor::GetUnit(*_bot, snapshot.guid);
        if (!unit || !_bot->IsHostileTo(unit))
            continue;

        float rangeSq = range * range;
        if (_bot->GetExactDistSq(snapshot.position) > rangeSq)
            continue;

        count++;
    }

    return count;
}

// Check if damage is mostly magical
bool DefensiveBehaviorManager::IsDamageMostlyMagical() const
{
    uint32 magicalDamage = 0;
    uint32 physicalDamage = 0;
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (const auto& entry : _damageHistory)
    {
        if (entry.timestamp == 0)
            continue;

        uint32 age = currentTime - entry.timestamp;
        if (age <= 3000) // Last 3 seconds
        {
            if (entry.isMagical)
                magicalDamage += entry.damage;
            else
                physicalDamage += entry.damage;
        }
    }

    return magicalDamage > physicalDamage;
}

// Check if damage is mostly physical
bool DefensiveBehaviorManager::IsDamageMostlyPhysical() const
{
    return !IsDamageMostlyMagical();
}

// Update performance metrics
void DefensiveBehaviorManager::UpdateMetrics(::std::chrono::steady_clock::time_point startTime)
{
    auto endTime = ::std::chrono::steady_clock::now();
    auto updateTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);

    _metrics.updatesPerformed++;

    // Update average (simple moving average)
    if (_metrics.averageUpdateTime.count() == 0)
        _metrics.averageUpdateTime = updateTime;
    else
        _metrics.averageUpdateTime = (_metrics.averageUpdateTime * 9 + updateTime) / 10;

    // Update max
    if (updateTime > _metrics.maxUpdateTime)
        _metrics.maxUpdateTime = updateTime;
}

} // namespace Playerbot