/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InstanceFarmingManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Map.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

InstanceFarmingManager::InstanceFarmingManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 10000, "InstanceFarmingManager")  // 10 second update
{
}

bool InstanceFarmingManager::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    InitializeInstanceDatabase();
    RefreshLockouts();
    _lastLockoutRefresh = std::chrono::steady_clock::now();

    return true;
}

void InstanceFarmingManager::OnShutdown()
{
    if (_currentSession.isActive)
    {
        StopSession("Shutdown");
    }

    _instanceDatabase.clear();
    _lockouts.clear();
}

void InstanceFarmingManager::OnUpdate(uint32 /*elapsed*/)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    // Refresh lockouts periodically
    auto now = std::chrono::steady_clock::now();
    auto timeSinceRefresh = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastLockoutRefresh).count();
    if (timeSinceRefresh >= LOCKOUT_REFRESH_INTERVAL_MS)
    {
        RefreshLockouts();
        _lastLockoutRefresh = now;
    }

    // Update instance progress
    if (_currentSession.isActive)
    {
        UpdateInstanceProgress();
    }
}

void InstanceFarmingManager::InitializeInstanceDatabase()
{
    // Add classic raid mounts
    {
        FarmableInstance onyxia;
        onyxia.mapId = 249;
        onyxia.name = "Onyxia's Lair";
        onyxia.minLevel = 30;
        onyxia.maxPlayers = 25;
        onyxia.isRaid = true;
        onyxia.hasWeeklyLockout = true;
        onyxia.mountDrops = {69395};  // Reins of the Onyxian Drake
        onyxia.estimatedGoldValue = 500000;  // ~50g
        onyxia.estimatedClearTimeMs = 300000;  // 5 minutes
        AddFarmableInstance(onyxia);
    }

    {
        FarmableInstance tempestKeep;
        tempestKeep.mapId = 550;
        tempestKeep.name = "The Eye (Tempest Keep)";
        tempestKeep.minLevel = 30;
        tempestKeep.maxPlayers = 25;
        tempestKeep.isRaid = true;
        tempestKeep.hasWeeklyLockout = true;
        tempestKeep.mountDrops = {32458};  // Ashes of Al'ar
        tempestKeep.estimatedGoldValue = 2000000;  // ~200g
        tempestKeep.estimatedClearTimeMs = 600000;  // 10 minutes
        AddFarmableInstance(tempestKeep);
    }

    {
        FarmableInstance stratholme;
        stratholme.mapId = 329;
        stratholme.name = "Stratholme";
        stratholme.minLevel = 15;
        stratholme.maxPlayers = 5;
        stratholme.isRaid = false;
        stratholme.hasWeeklyLockout = false;
        stratholme.mountDrops = {13335};  // Deathcharger's Reins
        stratholme.estimatedGoldValue = 100000;  // ~10g
        stratholme.estimatedClearTimeMs = 600000;  // 10 minutes
        AddFarmableInstance(stratholme);
    }

    {
        FarmableInstance utgardePinnacle;
        utgardePinnacle.mapId = 575;
        utgardePinnacle.name = "Utgarde Pinnacle";
        utgardePinnacle.minLevel = 20;
        utgardePinnacle.maxPlayers = 5;
        utgardePinnacle.isRaid = false;
        utgardePinnacle.hasWeeklyLockout = false;
        utgardePinnacle.mountDrops = {44151};  // Reins of the Blue Proto-Drake
        utgardePinnacle.estimatedGoldValue = 150000;  // ~15g
        utgardePinnacle.estimatedClearTimeMs = 600000;  // 10 minutes
        AddFarmableInstance(utgardePinnacle);
    }

    {
        FarmableInstance stonecoreTimed;
        stonecoreTimed.mapId = 725;
        stonecoreTimed.name = "The Stonecore";
        stonecoreTimed.minLevel = 30;
        stonecoreTimed.maxPlayers = 5;
        stonecoreTimed.isRaid = false;
        stonecoreTimed.hasWeeklyLockout = false;
        stonecoreTimed.mountDrops = {63043};  // Reins of the Vitreous Stone Drake
        stonecoreTimed.estimatedGoldValue = 200000;  // ~20g
        stonecoreTimed.estimatedClearTimeMs = 900000;  // 15 minutes
        AddFarmableInstance(stonecoreTimed);
    }

    {
        FarmableInstance vortexPinnacle;
        vortexPinnacle.mapId = 657;
        vortexPinnacle.name = "The Vortex Pinnacle";
        vortexPinnacle.minLevel = 30;
        vortexPinnacle.maxPlayers = 5;
        vortexPinnacle.isRaid = false;
        vortexPinnacle.hasWeeklyLockout = false;
        vortexPinnacle.mountDrops = {63040};  // Reins of the Drake of the North Wind
        vortexPinnacle.estimatedGoldValue = 200000;  // ~20g
        vortexPinnacle.estimatedClearTimeMs = 600000;  // 10 minutes
        AddFarmableInstance(vortexPinnacle);
    }

    TC_LOG_DEBUG("module.playerbot.dungeon",
        "InstanceFarmingManager: Initialized {} farmable instances",
        _instanceDatabase.size());
}

void InstanceFarmingManager::AddFarmableInstance(FarmableInstance const& instance)
{
    _instanceDatabase[instance.mapId] = instance;
}

bool InstanceFarmingManager::IsInInstance() const
{
    if (!GetBot())
        return false;

    Map* map = GetBot()->GetMap();
    return map && (map->IsDungeon() || map->IsRaid());
}

std::vector<FarmableInstance> InstanceFarmingManager::GetFarmableInstances(FarmingContentType type) const
{
    std::vector<FarmableInstance> result;

    for (auto const& [mapId, instance] : _instanceDatabase)
    {
        bool include = false;

        switch (type)
        {
            case FarmingContentType::NONE:
            case FarmingContentType::MIXED:
                include = true;
                break;
            case FarmingContentType::MOUNT:
                include = instance.HasMounts();
                break;
            case FarmingContentType::TRANSMOG:
                include = instance.HasTransmog();
                break;
            case FarmingContentType::PET:
                include = instance.HasPets();
                break;
            case FarmingContentType::GOLD:
                include = instance.estimatedGoldValue > 0;
                break;
            default:
                include = true;
                break;
        }

        if (include)
        {
            result.push_back(instance);
        }
    }

    return result;
}

std::vector<FarmableInstance> InstanceFarmingManager::GetInstancesWithMount(uint32 mountSpellId) const
{
    std::vector<FarmableInstance> result;

    for (auto const& [mapId, instance] : _instanceDatabase)
    {
        for (uint32 mount : instance.mountDrops)
        {
            if (mount == mountSpellId)
            {
                result.push_back(instance);
                break;
            }
        }
    }

    return result;
}

std::vector<FarmableInstance> InstanceFarmingManager::GetRecommendedInstances(uint32 maxCount) const
{
    std::vector<FarmableInstance> available = GetAvailableInstances();

    // Sort by priority:
    // 1. Instances with mounts (if prioritizing mounts)
    // 2. Gold per hour efficiency
    // 3. Time to clear
    std::sort(available.begin(), available.end(),
        [this](FarmableInstance const& a, FarmableInstance const& b) {
            if (_prioritizeMounts)
            {
                if (a.HasMounts() != b.HasMounts())
                    return a.HasMounts();
            }

            // Compare gold per hour
            float gphA = a.estimatedClearTimeMs > 0 ?
                static_cast<float>(a.estimatedGoldValue) * 3600000.0f / a.estimatedClearTimeMs : 0;
            float gphB = b.estimatedClearTimeMs > 0 ?
                static_cast<float>(b.estimatedGoldValue) * 3600000.0f / b.estimatedClearTimeMs : 0;

            return gphA > gphB;
        });

    if (available.size() > maxCount)
    {
        available.resize(maxCount);
    }

    return available;
}

FarmableInstance InstanceFarmingManager::GetInstanceInfo(uint32 mapId) const
{
    auto it = _instanceDatabase.find(mapId);
    if (it != _instanceDatabase.end())
    {
        return it->second;
    }

    return FarmableInstance();
}

bool InstanceFarmingManager::IsInstanceLocked(uint32 mapId, InstanceDifficulty difficulty) const
{
    uint64 key = (static_cast<uint64>(mapId) << 32) | static_cast<uint64>(difficulty);
    auto it = _lockouts.find(key);
    return it != _lockouts.end() && it->second.IsLocked();
}

InstanceLockout InstanceFarmingManager::GetLockout(uint32 mapId, InstanceDifficulty difficulty) const
{
    uint64 key = (static_cast<uint64>(mapId) << 32) | static_cast<uint64>(difficulty);
    auto it = _lockouts.find(key);
    if (it != _lockouts.end())
    {
        return it->second;
    }

    return InstanceLockout();
}

std::vector<InstanceLockout> InstanceFarmingManager::GetAllLockouts() const
{
    std::vector<InstanceLockout> result;
    result.reserve(_lockouts.size());

    for (auto const& [key, lockout] : _lockouts)
    {
        if (lockout.IsLocked())
        {
            result.push_back(lockout);
        }
    }

    return result;
}

std::vector<FarmableInstance> InstanceFarmingManager::GetAvailableInstances() const
{
    std::vector<FarmableInstance> result;

    for (auto const& [mapId, instance] : _instanceDatabase)
    {
        // Check level requirement
        if (GetBot() && GetBot()->GetLevel() < instance.minLevel)
            continue;

        // Check lockout
        if (instance.hasWeeklyLockout)
        {
            if (IsInstanceLocked(mapId, instance.difficulty))
                continue;
        }

        // Check if bot can solo
        if (!CanSoloInstance(instance))
            continue;

        result.push_back(instance);
    }

    return result;
}

void InstanceFarmingManager::RefreshLockouts()
{
    if (!GetBot())
        return;

    _lockouts.clear();

    // Query player's instance lockouts from TrinityCore
    // Would iterate through Player's bound instances and populate lockout map

    TC_LOG_DEBUG("module.playerbot.dungeon",
        "InstanceFarmingManager: Refreshed {} lockouts for bot {}",
        _lockouts.size(),
        GetBot()->GetName());
}

bool InstanceFarmingManager::StartSession(
    FarmingContentType goalType,
    std::vector<FarmingGoal> const& specificGoals)
{
    if (_currentSession.isActive)
    {
        TC_LOG_DEBUG("module.playerbot.dungeon",
            "InstanceFarmingManager: Session already active for bot {}",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    _currentSession.Reset();
    _currentSession.isActive = true;
    _currentSession.startTime = std::chrono::steady_clock::now();
    _currentSession.primaryGoal = goalType;

    if (!specificGoals.empty())
    {
        _currentSession.activeGoals = specificGoals;
    }
    else
    {
        // Auto-generate goals based on type
        if (goalType == FarmingContentType::MOUNT || goalType == FarmingContentType::MIXED)
        {
            auto missingMounts = GetMissingMounts();
            for (auto const& goal : missingMounts)
            {
                _currentSession.activeGoals.push_back(goal);
            }
        }
    }

    // Select first instance
    _currentSession.currentInstance = SelectNextInstance();

    TC_LOG_DEBUG("module.playerbot.dungeon",
        "InstanceFarmingManager: Started session for bot {}, goal: {}, targets: {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        static_cast<uint32>(goalType),
        _currentSession.activeGoals.size());

    return true;
}

void InstanceFarmingManager::StopSession(std::string const& reason)
{
    if (!_currentSession.isActive)
        return;

    _statistics.totalFarmingTimeMs += _currentSession.GetElapsedMs();
    _statistics.totalInstancesCleared += _currentSession.instancesCleared;
    _statistics.totalBossesKilled += _currentSession.bossesKilled;
    _statistics.totalGoldEarned += _currentSession.goldEarned;
    _statistics.totalMountsAcquired += _currentSession.mountsAcquired;
    _statistics.totalTransmogsAcquired += _currentSession.transmogsAcquired;

    TC_LOG_DEBUG("module.playerbot.dungeon",
        "InstanceFarmingManager: Stopped session for bot {}, reason: {}, instances: {}, gold: {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        reason.empty() ? "none" : reason.c_str(),
        _currentSession.instancesCleared,
        _currentSession.goldEarned);

    _currentSession.Reset();
}

bool InstanceFarmingManager::QueueInstance(FarmableInstance const& instance)
{
    if (!_currentSession.isActive)
        return false;

    _currentSession.currentInstance = instance;
    return true;
}

void InstanceFarmingManager::AddGoal(FarmingGoal const& goal)
{
    _currentSession.activeGoals.push_back(goal);
}

void InstanceFarmingManager::RemoveGoal(uint32 itemId)
{
    _currentSession.activeGoals.erase(
        std::remove_if(_currentSession.activeGoals.begin(), _currentSession.activeGoals.end(),
            [itemId](FarmingGoal const& g) { return g.targetItemId == itemId; }),
        _currentSession.activeGoals.end());
}

void InstanceFarmingManager::MarkGoalAcquired(uint32 itemId)
{
    for (auto& goal : _currentSession.activeGoals)
    {
        if (goal.targetItemId == itemId)
        {
            goal.isAcquired = true;
            NotifyCallback(goal.type, itemId, true);

            if (goal.type == FarmingContentType::MOUNT)
            {
                _currentSession.mountsAcquired++;
            }
            else if (goal.type == FarmingContentType::TRANSMOG)
            {
                _currentSession.transmogsAcquired++;
            }

            break;
        }
    }
}

std::vector<FarmingGoal> InstanceFarmingManager::GetMissingMounts() const
{
    std::vector<FarmingGoal> missingMounts;

    // Would query MountManager for missing mounts and cross-reference
    // with instances that can drop them

    for (auto const& [mapId, instance] : _instanceDatabase)
    {
        for (uint32 mountSpellId : instance.mountDrops)
        {
            // Check if bot already has this mount (would query MountManager)
            bool hasMountAlready = false;  // Placeholder

            if (!hasMountAlready)
            {
                FarmingGoal goal(FarmingContentType::MOUNT, mountSpellId, instance.name + " Mount");
                goal.instancesWithDrop.push_back(mapId);
                missingMounts.push_back(goal);
            }
        }
    }

    return missingMounts;
}

void InstanceFarmingManager::UpdateInstanceProgress()
{
    // Check if we've completed current instance
    // Would integrate with dungeon/raid completion tracking

    // Check session limits
    if (_currentSession.instancesCleared >= _maxInstancesPerSession)
    {
        StopSession("Max instances reached");
    }
}

FarmableInstance InstanceFarmingManager::SelectNextInstance() const
{
    auto recommended = GetRecommendedInstances(1);
    if (!recommended.empty())
    {
        return recommended[0];
    }

    return FarmableInstance();
}

bool InstanceFarmingManager::CanSoloInstance(FarmableInstance const& instance) const
{
    if (!GetBot())
        return false;

    // Level check - bot should be significantly higher level than instance
    uint32 botLevel = GetBot()->GetLevel();
    uint32 levelDiff = botLevel > instance.minLevel ? botLevel - instance.minLevel : 0;

    // Generally need to be 10+ levels higher to solo dungeons, 20+ for raids
    if (instance.isRaid)
    {
        return levelDiff >= 20;
    }

    return levelDiff >= 10;
}

void InstanceFarmingManager::NotifyCallback(FarmingContentType type, uint32 itemId, bool acquired)
{
    if (_callback)
    {
        _callback(type, itemId, acquired);
    }
}

} // namespace Playerbot
