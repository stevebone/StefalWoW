/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AdaptiveBehaviorManager.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "GameTime.h"
#include "../Decision/DecisionFusionSystem.h"
#include "../Common/ActionScoringEngine.h"
#include "Player.h"
#include "Group.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Timer.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

AdaptiveBehaviorManager::AdaptiveBehaviorManager(::Player* bot) :
    _bot(bot),
    _activeStrategies(STRATEGY_NONE),
    _activeProfile(nullptr),
    _lastProfileSwitch(0),
    _profileSwitchCount(0),
    _lastStrategyUpdate(0),
    _strategySwitchCount(0),
    _updateTimer(0),
    _lastUpdateTime(0),
    _totalUpdateTime(0),
    _updateCount(0),
    _compositionCacheTime(0),
    _roleCacheTime(0),
    _rolesInitialized(false)
{
    InitializeDefaultProfiles();
    // CRITICAL: Do NOT call AssignRoles() in constructor!
    // Bot's inventory and internal state may not be fully loaded yet during
    // ClassAI/BotAI construction. GetGearScore() accesses item templates which
    // can crash if inventory is not loaded. AssignRoles is deferred to first
    // Update() when bot IsInWorld().
}

AdaptiveBehaviorManager::~AdaptiveBehaviorManager() = default;

void AdaptiveBehaviorManager::InitializeDefaultProfiles()
{
    CreateEmergencyTankProfile();
    CreateAOEProfile();
    CreateSurvivalProfile();
    CreateBurstProfile();
    CreateResourceConservationProfile();
}

void AdaptiveBehaviorManager::CreateEmergencyTankProfile()
{
    BehaviorProfile profile;
    profile.name = "EmergencyTank";
    profile.priority = AdaptiveBehaviorPriority::EMERGENCY;
    profile.strategyFlags = STRATEGY_EMERGENCY_TANK | STRATEGY_DEFENSIVE | STRATEGY_USE_COOLDOWNS;
    profile.minDuration = 5000;
    profile.maxDuration = 30000;
    profile.cooldown = 60000;

    profile.condition = [](const CombatMetrics& metrics, CombatSituation situation) {
        return situation == CombatSituation::TANK_DEAD ||
               (!metrics.tankAlive && (metrics.eliteCount > 0 || metrics.bossCount > 0));
    };

    profile.applyFunction = [this](::Player* bot, uint32 flags) {
        // Emergency tank activation logic
        // SAFETY: Use GetGUID().GetCounter() instead of GetName() - Player string data may be freed during destruction
        TC_LOG_DEBUG("bot.playerbot", "Bot {} activating emergency tank mode", bot->GetGUID().GetCounter());
        ActivateStrategy(flags);
    };

    RegisterProfile(profile);
}

void AdaptiveBehaviorManager::CreateAOEProfile()
{
    BehaviorProfile profile;
    profile.name = "AOEMode";
    profile.priority = AdaptiveBehaviorPriority::HIGH;
    profile.strategyFlags = STRATEGY_AOE_FOCUS | STRATEGY_AGGRESSIVE;
    profile.minDuration = 3000;
    profile.maxDuration = 20000;
    profile.cooldown = 5000;

    profile.condition = [](const CombatMetrics& metrics, CombatSituation situation) {
        return situation == CombatSituation::AOE_HEAVY ||
               metrics.enemyCount >= 4 ||
               (metrics.enemyCount >= 3 && metrics.nearestEnemyDistance <= 8.0f);
    };

    profile.applyFunction = [this](::Player* bot, uint32 flags) {
        // SAFETY: Use GetGUID().GetCounter() instead of GetName() - Player string data may be freed during destruction
        TC_LOG_DEBUG("bot.playerbot", "Bot {} activating AOE mode", bot->GetGUID().GetCounter());
        ActivateStrategy(flags);
    };

    RegisterProfile(profile);
}

void AdaptiveBehaviorManager::CreateSurvivalProfile()
{
    BehaviorProfile profile;
    profile.name = "Survival";
    profile.priority = AdaptiveBehaviorPriority::CRITICAL;
    profile.strategyFlags = STRATEGY_SURVIVAL | STRATEGY_DEFENSIVE | STRATEGY_USE_CONSUMABLES | STRATEGY_USE_COOLDOWNS;
    profile.minDuration = 5000;
    profile.maxDuration = 15000;
    profile.cooldown = 30000;

    profile.condition = [](const CombatMetrics& metrics, CombatSituation situation) {
        return situation == CombatSituation::DEFENSIVE ||
               situation == CombatSituation::WIPE_IMMINENT ||
               metrics.personalHealthPercent < 30.0f ||
               (metrics.averageGroupHealth < 40.0f && !metrics.healerAlive);
    };

    profile.applyFunction = [this](::Player* bot, uint32 flags) {
        // SAFETY: Use GetGUID().GetCounter() instead of GetName() - Player string data may be freed during destruction
        TC_LOG_DEBUG("bot.playerbot", "Bot {} activating survival mode", bot->GetGUID().GetCounter());
        ActivateStrategy(flags);
    };

    RegisterProfile(profile);
}

void AdaptiveBehaviorManager::CreateBurstProfile()
{
    BehaviorProfile profile;
    profile.name = "BurstPhase";
    profile.priority = AdaptiveBehaviorPriority::HIGH;
    profile.strategyFlags = STRATEGY_BURST_DAMAGE | STRATEGY_AGGRESSIVE | STRATEGY_USE_COOLDOWNS | STRATEGY_USE_CONSUMABLES;
    profile.minDuration = 10000;
    profile.maxDuration = 30000;
    profile.cooldown = 120000;

    profile.condition = [](const CombatMetrics& metrics, CombatSituation situation) {
        return situation == CombatSituation::BURST_NEEDED ||
               (metrics.bossCount > 0 && metrics.enrageTimer > 0 && metrics.enrageTimer < 30000);
    };

    profile.applyFunction = [this](::Player* bot, uint32 flags) {
        // SAFETY: Use GetGUID().GetCounter() instead of GetName() - Player string data may be freed during destruction
        TC_LOG_DEBUG("bot.playerbot", "Bot {} activating burst phase", bot->GetGUID().GetCounter());
        ActivateStrategy(flags);
    };

    RegisterProfile(profile);
}

void AdaptiveBehaviorManager::CreateResourceConservationProfile()
{
    BehaviorProfile profile;
    profile.name = "ResourceConservation";
    profile.priority = AdaptiveBehaviorPriority::NORMAL;
    profile.strategyFlags = STRATEGY_CONSERVE_MANA | STRATEGY_SAVE_COOLDOWNS;
    profile.minDuration = 10000;
    profile.maxDuration = 60000;
    profile.cooldown = 20000;

    profile.condition = [](const CombatMetrics& metrics, CombatSituation situation) {
        return metrics.manaPercent < 30.0f ||
               (metrics.combatDuration < 30000 && metrics.bossCount > 0); // Save resources early in boss fight
    };

    profile.applyFunction = [this](::Player* bot, uint32 flags) {
        // SAFETY: Use GetGUID().GetCounter() instead of GetName() - Player string data may be freed during destruction
        TC_LOG_DEBUG("bot.playerbot", "Bot {} activating resource conservation", bot->GetGUID().GetCounter());
        ActivateStrategy(flags);
    };

    RegisterProfile(profile);
}

void AdaptiveBehaviorManager::Update(uint32 diff, const CombatMetrics& metrics, CombatSituation situation)
{
    uint32 startTime = GameTime::GetGameTimeMS();

    // Deferred role initialization - only when bot is fully in world
    // This cannot be done in constructor as inventory is not yet loaded
    if (!_rolesInitialized && _bot && _bot->IsInWorld())
    {
        AssignRoles();
        _rolesInitialized = true;
    }

    _updateTimer += diff;

    // Update every 200ms for responsiveness
    if (_updateTimer >= 200)
    {
        UpdateBehavior(metrics, situation);
        UpdateProfiles(diff, metrics, situation);
        AdaptToComposition();

        // Apply emergency strategies if needed
        ApplyEmergencyStrategies(metrics);

        // Apply positioning strategies based on situation
        ApplyPositioningStrategies(situation);

        // Apply resource management strategies
        ApplyResourceStrategies(metrics);

        _updateTimer = 0;
    }

    // Track performance
    _lastUpdateTime = GameTime::GetGameTimeMS() - startTime;
    _totalUpdateTime += _lastUpdateTime;
    _updateCount++;
}

void AdaptiveBehaviorManager::UpdateBehavior(const CombatMetrics& metrics, CombatSituation situation)
{
    // Clear expired strategies
    uint32 newStrategies = _activeStrategies;

    // Apply offensive or defensive strategies based on metrics
    if (metrics.averageGroupHealth > 70.0f && metrics.enemyCount > 0)
    {
        ApplyOffensiveStrategies(metrics);
    }
    else if (metrics.averageGroupHealth < 50.0f)
    {
        ApplyDefensiveStrategies(metrics);
    }

    // Track strategy changes
    if (newStrategies != _activeStrategies)
    {
        _strategySwitchCount++;
        _lastStrategyUpdate = GameTime::GetGameTimeMS();
    }
}

void AdaptiveBehaviorManager::UpdateProfiles(uint32 diff, const CombatMetrics& metrics, CombatSituation situation)
{
    BehaviorProfile* highestPriorityProfile = nullptr;
    AdaptiveBehaviorPriority highestPriority = AdaptiveBehaviorPriority::LOW;

    for (BehaviorProfile& profile : _profiles)
    {
        // Update active time
    if (profile.isActive)
        {
            profile.activeTime += diff;

            // Check if profile should deactivate (exceeded max duration)
    if (profile.activeTime >= profile.maxDuration)
            {
                RemoveProfile(profile);
                continue;
            }

            // Check if profile should remain active (minimum duration not met)
    if (profile.activeTime < profile.minDuration)
                continue;
        }

        // Check cooldown
    if (profile.lastActivated > 0 && profile.cooldown > 0)
        {
            if (GameTime::GetGameTimeMS() - profile.lastActivated < profile.cooldown)
                continue;
        }

        // Evaluate activation condition
        EvaluateProfileActivation(profile, metrics, situation);

        // Track highest priority profile
    if (profile.condition && profile.condition(metrics, situation))
        {
            if (profile.priority > highestPriority)
            {
                highestPriority = profile.priority;
                highestPriorityProfile = &profile;
            }
        }
    }

    // Switch to highest priority profile if different from current
    if (highestPriorityProfile && highestPriorityProfile != _activeProfile)
    {
        if (_activeProfile)
            RemoveProfile(*_activeProfile);

        ApplyProfile(*highestPriorityProfile);
        _activeProfile = highestPriorityProfile;
        _lastProfileSwitch = GameTime::GetGameTimeMS();
        _profileSwitchCount++;
    }
}

void AdaptiveBehaviorManager::EvaluateProfileActivation(BehaviorProfile& profile, const CombatMetrics& metrics, CombatSituation situation)
{
    if (!profile.condition)
        return;

    bool shouldActivate = profile.condition(metrics, situation);

    if (shouldActivate && !profile.isActive)
    {
        // Check cooldown
    if (profile.lastActivated > 0 && profile.cooldown > 0)
        {
            if (GameTime::GetGameTimeMS() - profile.lastActivated < profile.cooldown)
                return;
        }

        ApplyProfile(profile);
    }
    else if (!shouldActivate && profile.isActive && profile.activeTime >= profile.minDuration)
    {
        RemoveProfile(profile);
    }
}

void AdaptiveBehaviorManager::ApplyProfile(BehaviorProfile& profile)
{
    if (profile.applyFunction)
        profile.applyFunction(_bot, profile.strategyFlags);

    profile.isActive = true;
    profile.lastActivated = GameTime::GetGameTimeMS();
    profile.activeTime = 0;
    // SAFETY: Use GetGUID().GetCounter() instead of GetName() - Player string data may be freed during destruction
    TC_LOG_DEBUG("bot.playerbot", "Bot {} activated behavior profile: {}", _bot->GetGUID().GetCounter(), profile.name);
}

void AdaptiveBehaviorManager::RemoveProfile(BehaviorProfile& profile)
{
    DeactivateStrategy(profile.strategyFlags);
    profile.isActive = false;
    profile.activeTime = 0;
    // SAFETY: Use GetGUID().GetCounter() instead of GetName() - Player string data may be freed during destruction
    TC_LOG_DEBUG("bot.playerbot", "Bot {} deactivated behavior profile: {}", _bot->GetGUID().GetCounter(), profile.name);
}

void AdaptiveBehaviorManager::AdaptToComposition()
{
    // Update composition if cache is old
    if (GameTime::GetGameTimeMS() - _compositionCacheTime > 5000)
    {
        UpdateGroupComposition();
        _compositionCacheTime = GameTime::GetGameTimeMS();
    }

    // Adapt behavior based on composition
    if (!IsOptimalComposition())
    {
        // Missing tank - someone may need to emergency tank
    if (_groupComposition.tanks == 0 && CanPerformRole(BotRole::TANK))
        {
            ActivateStrategy(STRATEGY_EMERGENCY_TANK);
        }

        // Missing healer - activate self-preservation
    if (_groupComposition.healers == 0)
        {
            ActivateStrategy(STRATEGY_SURVIVAL | STRATEGY_USE_CONSUMABLES);
        }

        // Too many melee - some should stay ranged
    if (_groupComposition.meleeDPS > _groupComposition.rangedDPS + 2)
        {
            if (GetPrimaryRole() == BotRole::MELEE_DPS && CanPerformRole(BotRole::RANGED_DPS))
            {
                ActivateStrategy(STRATEGY_STAY_RANGED);
            }
        }
    }
}

void AdaptiveBehaviorManager::AssignRoles()
{
    BotRole primary = DetermineOptimalRole();
    BotRole secondary = DetermineSecondaryRole();

    _roleAssignment.primaryRole = primary;
    _roleAssignment.secondaryRole = secondary;
    _roleAssignment.roleEffectiveness = CalculateRoleScore(primary);
    _roleAssignment.rolePriority = GetRolePriority(primary);
    _roleAssignment.assignedTime = GameTime::GetGameTimeMS();
    _roleAssignment.isTemporary = false;

    // SAFETY: Use GetGUID().GetCounter() instead of GetName() - Player string data may be freed during destruction
    TC_LOG_DEBUG("bot.playerbot", "Bot {} assigned roles - Primary: {}, Secondary: {}",
        _bot->GetGUID().GetCounter(), GetRoleName(primary), GetRoleName(secondary));
}

void AdaptiveBehaviorManager::ActivateStrategy(uint32 flags)
{
    uint32 oldStrategies = _activeStrategies;
    _activeStrategies |= flags;

    if (oldStrategies != _activeStrategies)
    {
        _strategySwitchCount++;

        // Track activation time for each strategy
    for (uint32 i = 0; i < 32; ++i)
        {
            uint32 flag = 1 << i;
            if ((flags & flag) && !(oldStrategies & flag))
            {
                _strategyActiveTimes[flag] = GameTime::GetGameTimeMS();
            }
        }
    }
}

void AdaptiveBehaviorManager::DeactivateStrategy(uint32 flags)
{
    uint32 oldStrategies = _activeStrategies;
    _activeStrategies &= ~flags;

    if (oldStrategies != _activeStrategies)
    {
        _strategySwitchCount++;
    }
}

void AdaptiveBehaviorManager::RegisterProfile(const BehaviorProfile& profile)
{
    _profiles.push_back(profile);
}

void AdaptiveBehaviorManager::ActivateProfile(const ::std::string& name)
{
    for (BehaviorProfile& profile : _profiles)
    {
        if (profile.name == name)
        {
            ApplyProfile(profile);
            return;
        }
    }
}

void AdaptiveBehaviorManager::DeactivateProfile(const ::std::string& name)
{
    for (BehaviorProfile& profile : _profiles)
    {
        if (profile.name == name && profile.isActive)
        {
            RemoveProfile(profile);
            return;
        }
    }
}

bool AdaptiveBehaviorManager::IsProfileActive(const ::std::string& name) const
{
    for (const BehaviorProfile& profile : _profiles)
    {
        if (profile.name == name)
            return profile.isActive;
    }
    return false;
}

const BehaviorProfile* AdaptiveBehaviorManager::GetActiveProfile() const
{
    return _activeProfile;
}

::std::vector<::std::string> AdaptiveBehaviorManager::GetActiveProfileNames() const
{
    ::std::vector<::std::string> names;
    for (const BehaviorProfile& profile : _profiles)
    {
        if (profile.isActive)
            names.push_back(profile.name);
    }
    return names;
}

bool AdaptiveBehaviorManager::CanPerformRole(BotRole role) const
{
    Classes botClass = GetBotClass();
    switch (role)
    {
        case BotRole::TANK:
            return botClass == CLASS_WARRIOR || botClass == CLASS_PALADIN ||
                   botClass == CLASS_DEATH_KNIGHT || botClass == CLASS_DRUID;

        case BotRole::HEALER:
            return botClass == CLASS_PRIEST || botClass == CLASS_DRUID ||
                   botClass == CLASS_SHAMAN || botClass == CLASS_PALADIN;
        case BotRole::MELEE_DPS:
            return botClass == CLASS_WARRIOR || botClass == CLASS_ROGUE ||
                   botClass == CLASS_DEATH_KNIGHT || botClass == CLASS_PALADIN ||
                   botClass == CLASS_SHAMAN || botClass == CLASS_DRUID;

        case BotRole::RANGED_DPS:
            return botClass == CLASS_HUNTER || botClass == CLASS_MAGE ||
                   botClass == CLASS_WARLOCK || botClass == CLASS_PRIEST ||
                   botClass == CLASS_SHAMAN || botClass == CLASS_DRUID;

        case BotRole::CROWD_CONTROL:
            return botClass == CLASS_MAGE || botClass == CLASS_ROGUE ||
                   botClass == CLASS_HUNTER || botClass == CLASS_WARLOCK;
        default:
            return true;
    }
}

float AdaptiveBehaviorManager::GetRoleEffectiveness(BotRole role) const
{
    return CalculateRoleScore(role);
}

void AdaptiveBehaviorManager::ForceRole(BotRole role, bool temporary)
{
    _roleAssignment.primaryRole = role;
    _roleAssignment.isTemporary = temporary;
    _roleAssignment.assignedTime = GameTime::GetGameTimeMS();

    // SAFETY: Use GetGUID().GetCounter() instead of GetName() - Player string data may be freed during destruction
    TC_LOG_DEBUG("bot.playerbot", "Bot {} forced to role: {} (temporary: {})",
        _bot->GetGUID().GetCounter(), GetRoleName(role), temporary);
}

void AdaptiveBehaviorManager::UpdateGroupComposition()
{
    _groupComposition.Reset();

    ::Group* group = _bot->GetGroup();
    if (!group)
    {
        _groupComposition.totalMembers = 1;
        _groupComposition.alive = _bot->IsAlive() ? 1 : 0;
        return;
    }

    for (GroupReference const& groupRef : group->GetMembers())
    {
        ::Player* member = groupRef.GetSource();
        if (!member)
            continue;

        _groupComposition.totalMembers++;

        if (member->IsAlive())
            _groupComposition.alive++;
        else
            _groupComposition.dead++;

        // Classify member role based on class/spec
        Classes memberClass = static_cast<Classes>(member->GetClass());
        switch (memberClass)
        {
            case CLASS_WARRIOR:
                _groupComposition.tanks++;
                break;

            case CLASS_DEATH_KNIGHT:
                _groupComposition.tanks++;
                break;

            case CLASS_PALADIN:
                // Paladin can be tank, healer, or DPS
                _groupComposition.tanks++;
                _groupComposition.healers++;
                break;

            case CLASS_PRIEST:
                _groupComposition.healers++;
                break;

            case CLASS_DRUID:
                // Druid is hybrid - count as both tank and healer potential
                _groupComposition.tanks++;
                _groupComposition.healers++;
                break;

            case CLASS_SHAMAN:
                _groupComposition.healers++;
                _groupComposition.meleeDPS++;
                break;

            case CLASS_ROGUE:
                _groupComposition.meleeDPS++;
                break;

            case CLASS_HUNTER:
                _groupComposition.rangedDPS++;
                break;

            case CLASS_MAGE:
                _groupComposition.rangedDPS++;
                break;

            case CLASS_WARLOCK:
                _groupComposition.rangedDPS++;
                break;

            default:
                break;
        }

        // Check for special abilities
    if (memberClass == CLASS_SHAMAN || memberClass == CLASS_MAGE)
            _groupComposition.hasBloodlust = true;

        if (memberClass == CLASS_DRUID)
            _groupComposition.hasBattleRes = true;
    }
}

bool AdaptiveBehaviorManager::IsOptimalComposition() const
{
    // Optimal composition has at least 1 tank, 1 healer, and DPS
    return _groupComposition.tanks >= 1 &&
           _groupComposition.healers >= 1 &&
           (_groupComposition.meleeDPS + _groupComposition.rangedDPS) >= 1;
}

bool AdaptiveBehaviorManager::NeedsRoleSwitch() const
{
    // Check if role switch would benefit group
    if (!IsOptimalComposition())
    {
        if (_groupComposition.tanks == 0 && CanPerformRole(BotRole::TANK))
            return true;

        if (_groupComposition.healers == 0 && CanPerformRole(BotRole::HEALER))
            return true;
    }

    return false;
}

bool AdaptiveBehaviorManager::ShouldEmergencyTank() const
{
    return IsStrategyActive(STRATEGY_EMERGENCY_TANK) ||
           (!_groupComposition.tanks && CanPerformRole(BotRole::TANK));
}

bool AdaptiveBehaviorManager::ShouldEmergencyHeal() const
{
    return IsStrategyActive(STRATEGY_EMERGENCY_HEAL) ||
           (!_groupComposition.healers && CanPerformRole(BotRole::HEALER));
}

bool AdaptiveBehaviorManager::ShouldUseDefensiveCooldowns() const
{
    return IsStrategyActive(STRATEGY_DEFENSIVE) ||
           IsStrategyActive(STRATEGY_SURVIVAL);
}

bool AdaptiveBehaviorManager::ShouldUseOffensiveCooldowns() const
{
    return IsStrategyActive(STRATEGY_BURST_DAMAGE) ||
           (IsStrategyActive(STRATEGY_USE_COOLDOWNS) && !IsStrategyActive(STRATEGY_SAVE_COOLDOWNS));
}

bool AdaptiveBehaviorManager::ShouldSaveResources() const
{
    return IsStrategyActive(STRATEGY_CONSERVE_MANA) ||
           IsStrategyActive(STRATEGY_SAVE_COOLDOWNS);
}

uint32 AdaptiveBehaviorManager::GetAverageUpdateTime() const
{
    if (_updateCount == 0)
        return 0;
    return _totalUpdateTime / _updateCount;
}

void AdaptiveBehaviorManager::RecordDecisionOutcome(const ::std::string& decision, bool success)
{
    DecisionOutcome& outcome = _decisionHistory[decision];
    if (success)
        outcome.successCount++;
    else
        outcome.failureCount++;

    uint32 total = outcome.successCount + outcome.failureCount;
    if (total > 0)
        outcome.successRate = static_cast<float>(outcome.successCount) / total * 100.0f;
}

float AdaptiveBehaviorManager::GetDecisionSuccessRate(const ::std::string& decision) const
{
    auto it = _decisionHistory.find(decision);
    if (it != _decisionHistory.end())
        return it->second.successRate;
    return 0.0f;
}

void AdaptiveBehaviorManager::AdjustBehaviorWeights()
{
    // Analyze decision history and adjust profile priorities
    for (BehaviorProfile& profile : _profiles)
    {
        float successRate = GetDecisionSuccessRate(profile.name);

        // Adjust priority based on success rate
    if (successRate > 80.0f && profile.priority < AdaptiveBehaviorPriority::CRITICAL)
        {
            profile.priority = static_cast<AdaptiveBehaviorPriority>(static_cast<uint8>(profile.priority) + 1);
        }
        else if (successRate < 40.0f && profile.priority > AdaptiveBehaviorPriority::LOW)
        {
            profile.priority = static_cast<AdaptiveBehaviorPriority>(static_cast<uint8>(profile.priority) - 1);
        }
    }
}

void AdaptiveBehaviorManager::UpdateRoleAssignment()
{
    if (GameTime::GetGameTimeMS() - _roleAssignment.assignedTime < 30000)
        return; // Don't switch roles too frequently
    if (NeedsRoleSwitch())
    {
        AssignRoles();
    }
}

BotRole AdaptiveBehaviorManager::DetermineOptimalRole() const
{
    Classes botClass = GetBotClass();

    // Check group needs first
    if (_groupComposition.tanks == 0 && CanPerformRole(BotRole::TANK))
        return BotRole::TANK;

    if (_groupComposition.healers == 0 && CanPerformRole(BotRole::HEALER))
        return BotRole::HEALER;

    // Default role based on class
    switch (botClass)
    {
        case CLASS_WARRIOR:
            return HasTankSpec() ? BotRole::TANK : BotRole::MELEE_DPS;
        case CLASS_PALADIN:
            if (HasTankSpec()) return BotRole::TANK;
            if (HasHealSpec()) return BotRole::HEALER;
            return BotRole::MELEE_DPS;
        case CLASS_PRIEST:
            return BotRole::HEALER;
        case CLASS_ROGUE:
            return BotRole::MELEE_DPS;
        case CLASS_HUNTER:
            return BotRole::RANGED_DPS;
        case CLASS_SHAMAN:
            return HasHealSpec() ? BotRole::HEALER : BotRole::MELEE_DPS;
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return BotRole::RANGED_DPS;
        case CLASS_DRUID:
            if (HasTankSpec()) return BotRole::TANK;
            if (HasHealSpec()) return BotRole::HEALER;
            return BotRole::MELEE_DPS;
        case CLASS_DEATH_KNIGHT:
            return HasTankSpec() ? BotRole::TANK : BotRole::MELEE_DPS;
        default:
            return BotRole::MELEE_DPS;
    }
}

BotRole AdaptiveBehaviorManager::DetermineSecondaryRole() const
{
    BotRole primary = _roleAssignment.primaryRole;
    Classes botClass = GetBotClass();

    // Determine secondary role based on class flexibility
    switch (botClass)
    {
        case CLASS_PALADIN:
            if (primary == BotRole::TANK) return BotRole::OFF_HEALER;
            if (primary == BotRole::HEALER) return BotRole::OFF_TANK;
            return BotRole::SUPPORT;

        case CLASS_DRUID:
            if (primary == BotRole::TANK) return BotRole::OFF_HEALER;
            if (primary == BotRole::HEALER) return BotRole::RANGED_DPS;
            return BotRole::HYBRID;

        case CLASS_SHAMAN:
            if (primary == BotRole::HEALER) return BotRole::RANGED_DPS;
            return BotRole::OFF_HEALER;

        case CLASS_PRIEST:
            if (primary == BotRole::HEALER) return BotRole::RANGED_DPS;
            return BotRole::SUPPORT;

        case CLASS_WARRIOR:
            if (primary == BotRole::TANK) return BotRole::MELEE_DPS;
            return BotRole::OFF_TANK;

        case CLASS_DEATH_KNIGHT:
            if (primary == BotRole::TANK) return BotRole::MELEE_DPS;
            return BotRole::OFF_TANK;

        default:
            return BotRole::NONE;
    }
}

float AdaptiveBehaviorManager::CalculateRoleScore(BotRole role) const
{
    float score = 50.0f; // Base score

    Classes botClass = GetBotClass();

    // Class-specific bonuses
    switch (role)
    {
        case BotRole::TANK:
            if (botClass == CLASS_WARRIOR || botClass == CLASS_PALADIN ||
                botClass == CLASS_DEATH_KNIGHT)
                score += 30.0f;
            if (HasTankSpec())
                score += 20.0f;
            break;

        case BotRole::HEALER:
            if (botClass == CLASS_PRIEST || botClass == CLASS_DRUID ||
                botClass == CLASS_SHAMAN || botClass == CLASS_PALADIN)
                score += 30.0f;
            if (HasHealSpec())
                score += 20.0f;
            break;

        case BotRole::MELEE_DPS:
            if (botClass == CLASS_ROGUE || botClass == CLASS_WARRIOR ||
                botClass == CLASS_DEATH_KNIGHT)
                score += 30.0f;
            break;

        case BotRole::RANGED_DPS:
            if (botClass == CLASS_HUNTER || botClass == CLASS_MAGE ||
                botClass == CLASS_WARLOCK)
                score += 30.0f;
            break;

        default:
            break;
    }

    // Gear score bonus (simplified)
    score += GetGearScore() / 100.0f;

    return ::std::min(100.0f, score);
}

bool AdaptiveBehaviorManager::IsRoleNeeded(BotRole role) const
{
    switch (role)
    {
        case BotRole::TANK:
            return _groupComposition.tanks == 0;
        case BotRole::HEALER:
            return _groupComposition.healers == 0;
        case BotRole::MELEE_DPS:
            return _groupComposition.meleeDPS < 3;
        case BotRole::RANGED_DPS:
            return _groupComposition.rangedDPS < 3;
        default:
            return false;
    }
}

uint32 AdaptiveBehaviorManager::GetRolePriority(BotRole role) const
{
    // Priority: Tank > Healer > DPS
    switch (role)
    {
        case BotRole::TANK: return 1;
        case BotRole::HEALER: return 2;
        case BotRole::OFF_TANK: return 3;
        case BotRole::OFF_HEALER: return 4;
        case BotRole::MELEE_DPS:
        case BotRole::RANGED_DPS: return 5;
        default: return 999;
    }
}

void AdaptiveBehaviorManager::ApplyEmergencyStrategies(const CombatMetrics& metrics)
{
    // Emergency tank if needed
    if (metrics.tankAlive == false && CanPerformRole(BotRole::TANK))
    {
        ActivateStrategy(STRATEGY_EMERGENCY_TANK | STRATEGY_DEFENSIVE);
    }

    // Emergency healing if needed
    if (metrics.averageGroupHealth < 40.0f && CanPerformRole(BotRole::HEALER))
    {
        ActivateStrategy(STRATEGY_EMERGENCY_HEAL);
    }

    // Survival mode if health critical
    if (metrics.personalHealthPercent < 30.0f)
    {
        ActivateStrategy(STRATEGY_SURVIVAL | STRATEGY_USE_COOLDOWNS | STRATEGY_USE_CONSUMABLES);
    }
}

void AdaptiveBehaviorManager::ApplyOffensiveStrategies(const CombatMetrics& metrics)
{
    // Burst if boss is low or enraging
    if (metrics.bossCount > 0 && (metrics.enrageTimer < 30000 || metrics.lowestGroupHealth > 70.0f))
    {
        ActivateStrategy(STRATEGY_BURST_DAMAGE | STRATEGY_USE_COOLDOWNS);
    }

    // AOE if many enemies
    if (metrics.enemyCount >= 3)
    {
        ActivateStrategy(STRATEGY_AOE_FOCUS);
    }
    else
    {
        ActivateStrategy(STRATEGY_SINGLE_TARGET);
    }

    // Aggressive if safe
    if (metrics.averageGroupHealth > 80.0f && metrics.tankAlive && metrics.healerAlive)
    {
        ActivateStrategy(STRATEGY_AGGRESSIVE);
    }
}

void AdaptiveBehaviorManager::ApplyDefensiveStrategies(const CombatMetrics& metrics)
{
    // Defensive if taking heavy damage
    if (metrics.personalHealthPercent < 50.0f || metrics.incomingDPS > 0)
    {
        ActivateStrategy(STRATEGY_DEFENSIVE);
    }

    // Save cooldowns if early in fight
    if (metrics.combatDuration < 30000 && metrics.bossCount > 0)
    {
        ActivateStrategy(STRATEGY_SAVE_COOLDOWNS);
    }

    // Conserve mana if low
    if (metrics.manaPercent < 40.0f)
    {
        ActivateStrategy(STRATEGY_CONSERVE_MANA);
    }
}

void AdaptiveBehaviorManager::ApplyPositioningStrategies(CombatSituation situation)
{
    switch (situation)
    {
        case CombatSituation::SPREAD:
            ActivateStrategy(STRATEGY_SPREAD | STRATEGY_MOBILITY);
            DeactivateStrategy(STRATEGY_STACK);
            break;

        case CombatSituation::STACK:
            ActivateStrategy(STRATEGY_STACK);
            DeactivateStrategy(STRATEGY_SPREAD);
            break;
        case CombatSituation::KITE:
            ActivateStrategy(STRATEGY_KITE | STRATEGY_MOBILITY | STRATEGY_STAY_RANGED);
            DeactivateStrategy(STRATEGY_STAY_MELEE);
            break;

        default:
            // Normal positioning based on role
    if (IsDPSRole(GetPrimaryRole()))
            {
                if (GetPrimaryRole() == BotRole::MELEE_DPS)
                    ActivateStrategy(STRATEGY_STAY_MELEE);
                else
                    ActivateStrategy(STRATEGY_STAY_RANGED);
            }
            break;
    }
}

void AdaptiveBehaviorManager::ApplyResourceStrategies(const CombatMetrics& metrics)
{
    // Use consumables in critical situations
    if (metrics.personalHealthPercent < 40.0f ||
        (metrics.bossCount > 0 && metrics.enrageTimer < 60000))
    {
        ActivateStrategy(STRATEGY_USE_CONSUMABLES);
    }

    // Manage cooldowns based on combat phase
    if (metrics.bossCount > 0)
    {
        if (metrics.combatDuration < 30000)
        {
            // Early in fight, save cooldowns
            ActivateStrategy(STRATEGY_SAVE_COOLDOWNS);
            DeactivateStrategy(STRATEGY_USE_COOLDOWNS);
        }
        else if (metrics.enrageTimer < 120000)
        {
            // Use cooldowns when needed
            ActivateStrategy(STRATEGY_USE_COOLDOWNS);
            DeactivateStrategy(STRATEGY_SAVE_COOLDOWNS);
        }
    }
    else
    {
        // Normal enemies, use cooldowns freely
    if (metrics.enemyCount >= 3 || metrics.eliteCount > 0)
        {
            ActivateStrategy(STRATEGY_USE_COOLDOWNS);
        }
    }
}

Classes AdaptiveBehaviorManager::GetBotClass() const
{
    return static_cast<Classes>(_bot->GetClass());
}

uint32 AdaptiveBehaviorManager::GetBotSpec() const
{
    // Simplified spec detection - would need talent inspection in production
    return 0;
}

bool AdaptiveBehaviorManager::HasTankSpec() const
{
    // Simplified - would check actual spec/talents
    Classes botClass = GetBotClass();
    return botClass == CLASS_WARRIOR || botClass == CLASS_PALADIN ||
           botClass == CLASS_DEATH_KNIGHT || botClass == CLASS_DRUID;
}

bool AdaptiveBehaviorManager::HasHealSpec() const
{
    // Simplified - would check actual spec/talents
    Classes botClass = GetBotClass();
    return botClass == CLASS_PRIEST || botClass == CLASS_PALADIN ||
           botClass == CLASS_DRUID || botClass == CLASS_SHAMAN;
}

bool AdaptiveBehaviorManager::HasCrowdControl() const
{
    Classes botClass = GetBotClass();
    return botClass == CLASS_MAGE || botClass == CLASS_ROGUE ||
           botClass == CLASS_HUNTER || botClass == CLASS_WARLOCK ||
           botClass == CLASS_DRUID || botClass == CLASS_SHAMAN;
}

float AdaptiveBehaviorManager::GetGearScore() const
{
    // Full implementation: Calculate comprehensive gear score from equipped items
    if (!_bot)
        return 3000.0f;  // Baseline if no bot

    float totalScore = 0.0f;
    uint32 slotCount = 0;
    uint32 itemLevelSum = 0;

    // Slot importance multipliers (TWW 12.0 weights)
    static const std::unordered_map<uint8, float> slotMultipliers = {
        {EQUIPMENT_SLOT_HEAD, 1.2f},
        {EQUIPMENT_SLOT_NECK, 0.9f},
        {EQUIPMENT_SLOT_SHOULDERS, 1.1f},
        {EQUIPMENT_SLOT_BODY, 0.0f},       // Shirt - no stats
        {EQUIPMENT_SLOT_CHEST, 1.2f},
        {EQUIPMENT_SLOT_WAIST, 1.0f},
        {EQUIPMENT_SLOT_LEGS, 1.2f},
        {EQUIPMENT_SLOT_FEET, 1.0f},
        {EQUIPMENT_SLOT_WRISTS, 0.9f},
        {EQUIPMENT_SLOT_HANDS, 1.0f},
        {EQUIPMENT_SLOT_FINGER1, 0.85f},
        {EQUIPMENT_SLOT_FINGER2, 0.85f},
        {EQUIPMENT_SLOT_TRINKET1, 1.1f},
        {EQUIPMENT_SLOT_TRINKET2, 1.1f},
        {EQUIPMENT_SLOT_BACK, 0.9f},
        {EQUIPMENT_SLOT_MAINHAND, 1.5f},   // Weapons are highly weighted
        {EQUIPMENT_SLOT_OFFHAND, 1.2f},
        {EQUIPMENT_SLOT_RANGED, 1.0f}      // Legacy slot
    };

    // Quality multipliers
    static const std::unordered_map<uint32, float> qualityMultipliers = {
        {ITEM_QUALITY_POOR, 0.5f},
        {ITEM_QUALITY_NORMAL, 0.7f},
        {ITEM_QUALITY_UNCOMMON, 0.85f},
        {ITEM_QUALITY_RARE, 1.0f},
        {ITEM_QUALITY_EPIC, 1.15f},
        {ITEM_QUALITY_LEGENDARY, 1.3f},
        {ITEM_QUALITY_ARTIFACT, 1.4f}
    };

    // Iterate through all equipment slots
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        ItemTemplate const* itemTemplate = item->GetTemplate();
        if (!itemTemplate)
            continue;

        // Skip items with no stats (shirts, tabards)
        auto slotIt = slotMultipliers.find(slot);
        if (slotIt == slotMultipliers.end() || slotIt->second == 0.0f)
            continue;

        float slotMultiplier = slotIt->second;

        // Get item level
        uint32 itemLevel = itemTemplate->GetBaseItemLevel();
        itemLevelSum += itemLevel;

        // Get quality multiplier
        float qualityMult = 1.0f;
        auto qualIt = qualityMultipliers.find(itemTemplate->GetQuality());
        if (qualIt != qualityMultipliers.end())
            qualityMult = qualIt->second;

        // Calculate slot score: itemLevel * slotWeight * qualityMult
        float slotScore = static_cast<float>(itemLevel) * slotMultiplier * qualityMult;

        // Bonus for gem sockets (TWW 12.0: each socket adds ~2% value)
        uint32 socketCount = 0;
        for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
        {
            if (itemTemplate->GetSocketColor(i) != SocketColor(0))
                socketCount++;
        }
        if (socketCount > 0)
            slotScore *= (1.0f + 0.02f * socketCount);

        // Bonus for enchantments (enchanted items get 5% boost)
        if (item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) != 0)
            slotScore *= 1.05f;

        totalScore += slotScore;
        slotCount++;
    }

    // Calculate average item level for reference
    float avgItemLevel = slotCount > 0 ? static_cast<float>(itemLevelSum) / slotCount : 0.0f;

    // Base score formula: (total weighted score / expected slots) * 100
    // Expected filled slots for a fully geared character: ~16 (excluding shirt, tabard)
    constexpr float EXPECTED_SLOT_COUNT = 16.0f;
    float normalizedScore = slotCount > 0 ? (totalScore / EXPECTED_SLOT_COUNT) * 100.0f : 0.0f;

    // Add average item level contribution (ilvl 400+ = 4000+ gear score)
    float ilvlContribution = avgItemLevel * 10.0f;

    // Final gear score combines weighted slot scores and item level
    float finalScore = normalizedScore + ilvlContribution;

    // Clamp to reasonable bounds (0 to 10000)
    finalScore = std::clamp(finalScore, 0.0f, 10000.0f);

    // SAFETY: Use GetGUID().GetCounter() instead of GetName().c_str() - Player string data may be freed during destruction
    TC_LOG_DEBUG("playerbot", "AdaptiveBehaviorManager::GetGearScore - Bot {}: "
        "slots={}, avgIlvl={:.1f}, weightedScore={:.1f}, final={:.1f}",
        _bot->GetGUID().GetCounter(), slotCount, avgItemLevel, normalizedScore, finalScore);

    return finalScore;
}

void AdaptiveBehaviorManager::Reset()
{
    _activeStrategies = STRATEGY_NONE;
    _activeProfile = nullptr;
    _profileSwitchCount = 0;
    _strategySwitchCount = 0;
    _strategyActiveTimes.clear();
    _decisionHistory.clear();
    _updateTimer = 0;

    for (BehaviorProfile& profile : _profiles)
    {
        profile.isActive = false;
        profile.activeTime = 0;
        profile.lastActivated = 0;
    }

    AssignRoles();
}

void AdaptiveBehaviorManager::ClearProfiles()
{
    _profiles.clear();
    _activeProfile = nullptr;
}

void AdaptiveBehaviorManager::ResetStrategies()
{
    _activeStrategies = STRATEGY_NONE;
    _strategyActiveTimes.clear();
}

// ============================================================================
// DECISION FUSION INTEGRATION
// ============================================================================

Playerbot::bot::ai::DecisionVote AdaptiveBehaviorManager::GetRecommendedAction(Unit* target, Playerbot::bot::ai::CombatContext context) const
{
    using namespace Playerbot::bot::ai;

    // Create vote based on active profile and strategies
    DecisionVote vote;
    vote.source = DecisionSource::ADAPTIVE_BEHAVIOR;
    vote.target = target;
    vote.confidence = 0.0f;
    vote.urgency = 0.0f;

    if (!_bot || !_activeProfile)
    {
        vote.reasoning = "No active profile";
        return vote;
    }

    // Base confidence on profile effectiveness
    vote.confidence = 0.5f;  // Moderate baseline confidence

    // Evaluate urgency based on combat context
    float healthPct = _bot->GetHealthPct();
    if (healthPct < 20.0f)
    {
        vote.urgency = 0.95f;
        vote.reasoning = "Emergency: Low health, prioritize survival";
    }
    else if (healthPct < 40.0f)
    {
        vote.urgency = 0.7f;
        vote.reasoning = "Warning: Moderate health, consider defensive options";
    }
    else if (target && target->GetHealthPct() < 20.0f)
    {
        vote.urgency = 0.8f;
        vote.reasoning = "Execute phase: Target low health, prioritize finishing blows";
    }
    else
    {
        vote.urgency = 0.4f;
        vote.reasoning = "Normal combat: Standard priority";
    }

    // Adjust confidence based on active strategies
    if (_activeStrategies & STRATEGY_AGGRESSIVE)
    {
        vote.confidence += 0.1f;
        vote.reasoning += "; Aggressive strategy active";
    }
    if (_activeStrategies & STRATEGY_DEFENSIVE)
    {
        vote.confidence += 0.15f;
        vote.reasoning += "; Defensive strategy active";
    }
    if (_activeStrategies & STRATEGY_BURST_DAMAGE)
    {
        vote.confidence += 0.1f;
        vote.urgency += 0.2f;  // Burst increases urgency
        vote.reasoning += "; Burst strategy active";
    }

    // Clamp values
    vote.confidence = std::clamp(vote.confidence, 0.0f, 1.0f);
    vote.urgency = std::clamp(vote.urgency, 0.0f, 1.0f);

    // Action ID would typically come from the active profile's recommended action
    // For now, return 0 to indicate "defer to other systems for specific action"
    vote.actionId = 0;

    return vote;
}

} // namespace Playerbot