/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerActivityTracker.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "Map.h"
#include "DB2Structure.h"
#include "Character/BotLevelDistribution.h"
#include <algorithm>
#include <numeric>

namespace Playerbot
{

PlayerActivityTracker* PlayerActivityTracker::Instance()
{
    static PlayerActivityTracker instance;
    return &instance;
}

bool PlayerActivityTracker::Initialize()
{
    if (_initialized.exchange(true))
        return true;

    LoadConfig();

    TC_LOG_INFO("playerbot.lifecycle", "PlayerActivityTracker initialized");
    return true;
}

void PlayerActivityTracker::Shutdown()
{
    if (!_initialized.exchange(false))
        return;

    _playerActivity.clear();

    TC_LOG_INFO("playerbot.lifecycle", "PlayerActivityTracker shutdown. "
        "Final tracked players: {}", GetTotalTrackedPlayers());
}

void PlayerActivityTracker::Update(uint32 diff)
{
    if (!_initialized || !_config.enabled)
        return;

    _updateAccumulator += diff;
    _cleanupAccumulator += diff;

    // Update active status periodically
    if (_updateAccumulator >= _config.updateIntervalMs)
    {
        _updateAccumulator = 0;
        UpdateActiveStatus();
    }

    // Cleanup stale entries
    if (_cleanupAccumulator >= _config.cleanupIntervalMs)
    {
        _cleanupAccumulator = 0;
        CleanupStaleEntries();
    }
}

void PlayerActivityTracker::LoadConfig()
{
    _config.enabled = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Activity.Enable", true);
    _config.staleThresholdSeconds = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Activity.StaleThresholdSeconds", 300);
    _config.updateIntervalMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Activity.UpdateIntervalMs", 10000);
    _config.cleanupIntervalMs = sPlayerbotConfig->GetInt(
        "Playerbot.Lifecycle.Activity.CleanupIntervalMs", 60000);
    _config.trackInstances = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Activity.TrackInstances", true);
    _config.trackBattlegrounds = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Activity.TrackBattlegrounds", true);
    _config.logActivityChanges = sPlayerbotConfig->GetBool(
        "Playerbot.Lifecycle.Activity.LogChanges", false);

    TC_LOG_INFO("playerbot.lifecycle", "PlayerActivityTracker config loaded: "
        "StaleThreshold={}s, UpdateInterval={}ms",
        _config.staleThresholdSeconds, _config.updateIntervalMs);
}

// ============================================================================
// EVENT TRACKING
// ============================================================================

void PlayerActivityTracker::OnPlayerLogin(Player* player)
{
    if (!_initialized || !player)
        return;

    PlayerActivity activity;
    activity.playerGuid = player->GetGUID();
    activity.playerName = player->GetName();
    activity.level = player->GetLevel();
    activity.playerClass = player->GetClass();
    activity.zoneId = player->GetZoneId();
    activity.areaId = player->GetAreaId();
    activity.mapId = player->GetMapId();
    activity.isInGroup = player->GetGroup() != nullptr;
    activity.isInInstance = player->GetMap() && player->GetMap()->IsDungeon();
    activity.isInBattleground = player->GetMap() && player->GetMap()->IsBattleground();
    activity.lastUpdate = std::chrono::system_clock::now();
    activity.isActive = true;

    _playerActivity.insert_or_assign(player->GetGUID(), activity);
    _countsDirty = true;

    if (_config.logActivityChanges)
    {
        TC_LOG_DEBUG("playerbot.lifecycle", "Player {} logged in at zone {} (level {})",
            player->GetName(), activity.zoneId, activity.level);
    }
}

void PlayerActivityTracker::OnPlayerLogout(Player* player)
{
    if (!_initialized || !player)
        return;

    _playerActivity.erase(player->GetGUID());
    _countsDirty = true;

    if (_config.logActivityChanges)
    {
        TC_LOG_DEBUG("playerbot.lifecycle", "Player {} logged out",
            player->GetName());
    }
}

void PlayerActivityTracker::OnPlayerZoneChange(Player* player, uint32 newZoneId, uint32 newAreaId)
{
    if (!_initialized || !player)
        return;

    ObjectGuid playerGuid = player->GetGUID();
    auto activity = _playerActivity.find(playerGuid);
    if (activity == _playerActivity.end())
    {
        // Player not tracked yet, create entry
        OnPlayerLogin(player);
        return;
    }

    uint32 oldZone = activity->second.zoneId;
    uint32 mapId = player->GetMapId();
    bool isInstance = player->GetMap() && player->GetMap()->IsDungeon();
    bool isBG = player->GetMap() && player->GetMap()->IsBattleground();
    auto now = std::chrono::system_clock::now();

    _playerActivity.modify_if(playerGuid, [&](auto& pair) {
        pair.second.zoneId = newZoneId;
        pair.second.areaId = newAreaId;
        pair.second.mapId = mapId;
        pair.second.isInInstance = isInstance;
        pair.second.isInBattleground = isBG;
        pair.second.lastUpdate = now;
        pair.second.isActive = true;
    });
    _countsDirty = true;

    if (_config.logActivityChanges && oldZone != newZoneId)
    {
        TC_LOG_DEBUG("playerbot.lifecycle", "Player {} moved from zone {} to zone {}",
            player->GetName(), oldZone, newZoneId);
    }
}

void PlayerActivityTracker::OnPlayerLevelUp(Player* player, uint32 newLevel)
{
    if (!_initialized || !player)
        return;

    ObjectGuid playerGuid = player->GetGUID();
    auto activity = _playerActivity.find(playerGuid);
    if (activity == _playerActivity.end())
        return;

    auto now = std::chrono::system_clock::now();
    _playerActivity.modify_if(playerGuid, [newLevel, now](auto& pair) {
        pair.second.level = newLevel;
        pair.second.lastUpdate = now;
        pair.second.isActive = true;
    });
    _countsDirty = true;

    if (_config.logActivityChanges)
    {
        TC_LOG_DEBUG("playerbot.lifecycle", "Player {} leveled up to {}",
            player->GetName(), newLevel);
    }
}

void PlayerActivityTracker::OnPlayerGroupChange(Player* player, bool isInGroup)
{
    if (!_initialized || !player)
        return;

    ObjectGuid playerGuid = player->GetGUID();
    auto activity = _playerActivity.find(playerGuid);
    if (activity == _playerActivity.end())
        return;

    auto now = std::chrono::system_clock::now();
    _playerActivity.modify_if(playerGuid, [isInGroup, now](auto& pair) {
        pair.second.isInGroup = isInGroup;
        pair.second.lastUpdate = now;
        pair.second.isActive = true;
    });
}

void PlayerActivityTracker::UpdatePlayerActivity(Player* player)
{
    if (!_initialized || !player)
        return;

    ObjectGuid playerGuid = player->GetGUID();
    auto activity = _playerActivity.find(playerGuid);
    if (activity == _playerActivity.end())
    {
        OnPlayerLogin(player);
        return;
    }

    uint32 zoneId = player->GetZoneId();
    uint32 areaId = player->GetAreaId();
    uint32 mapId = player->GetMapId();
    uint32 level = player->GetLevel();
    bool inGroup = player->GetGroup() != nullptr;
    bool isInstance = player->GetMap() && player->GetMap()->IsDungeon();
    bool isBG = player->GetMap() && player->GetMap()->IsBattleground();
    auto now = std::chrono::system_clock::now();

    _playerActivity.modify_if(playerGuid, [=](auto& pair) {
        pair.second.zoneId = zoneId;
        pair.second.areaId = areaId;
        pair.second.mapId = mapId;
        pair.second.level = level;
        pair.second.isInGroup = inGroup;
        pair.second.isInInstance = isInstance;
        pair.second.isInBattleground = isBG;
        pair.second.lastUpdate = now;
        pair.second.isActive = true;
    });
}

// ============================================================================
// ACTIVITY QUERIES
// ============================================================================

std::map<uint32, uint32> PlayerActivityTracker::GetPlayerCountByZone() const
{
    std::map<uint32, uint32> result;

    _playerActivity.for_each([&](auto const& pair) {
        if (pair.second.isActive)
        {
            ++result[pair.second.zoneId];
        }
    });

    return result;
}

std::map<ExpansionTier, uint32> PlayerActivityTracker::GetPlayerCountByBracket() const
{
    std::map<ExpansionTier, uint32> result;

    _playerActivity.for_each([&](auto const& pair) {
        if (pair.second.isActive)
        {
            ExpansionTier tier = GetTierForLevel(pair.second.level);
            ++result[tier];
        }
    });

    return result;
}

std::vector<uint32> PlayerActivityTracker::GetHighActivityZones(uint32 maxCount) const
{
    auto zoneCounts = GetPlayerCountByZone();

    // Convert to vector for sorting
    std::vector<std::pair<uint32, uint32>> sortedZones(
        zoneCounts.begin(), zoneCounts.end());

    // Sort by count descending
    std::sort(sortedZones.begin(), sortedZones.end(),
        [](auto const& a, auto const& b) { return a.second > b.second; });

    // Extract zone IDs
    std::vector<uint32> result;
    for (uint32 i = 0; i < maxCount && i < sortedZones.size(); ++i)
    {
        result.push_back(sortedZones[i].first);
    }

    return result;
}

std::vector<uint32> PlayerActivityTracker::GetZonesWithPlayersAtLevel(
    uint32 targetLevel, uint32 range) const
{
    std::map<uint32, uint32> matchingZones;  // zoneId -> count

    _playerActivity.for_each([&](auto const& pair) {
        if (!pair.second.isActive)
            return;

        int32 diff = static_cast<int32>(pair.second.level) - static_cast<int32>(targetLevel);
        if (std::abs(diff) <= static_cast<int32>(range))
        {
            ++matchingZones[pair.second.zoneId];
        }
    });

    // Sort by count
    std::vector<std::pair<uint32, uint32>> sorted(
        matchingZones.begin(), matchingZones.end());
    std::sort(sorted.begin(), sorted.end(),
        [](auto const& a, auto const& b) { return a.second > b.second; });

    std::vector<uint32> result;
    for (auto const& [zoneId, _] : sorted)
    {
        result.push_back(zoneId);
    }

    return result;
}

std::array<uint32, 81> PlayerActivityTracker::GetPlayerLevelDistribution() const
{
    std::array<uint32, 81> distribution = {};

    _playerActivity.for_each([&](auto const& pair) {
        if (pair.second.isActive && pair.second.level <= 80)
        {
            ++distribution[pair.second.level];
        }
    });

    return distribution;
}

ZoneActivitySummary PlayerActivityTracker::GetZoneActivitySummary(uint32 zoneId) const
{
    ZoneActivitySummary summary;
    summary.zoneId = zoneId;

    uint32 totalLevel = 0;
    uint32 minLevel = UINT32_MAX;
    uint32 maxLevel = 0;

    _playerActivity.for_each([&](auto const& pair) {
        if (pair.second.isActive && pair.second.zoneId == zoneId)
        {
            ++summary.playerCount;
            totalLevel += pair.second.level;
            minLevel = std::min(minLevel, pair.second.level);
            maxLevel = std::max(maxLevel, pair.second.level);
            ++summary.classCounts[pair.second.playerClass];
        }
    });

    if (summary.playerCount > 0)
    {
        summary.averageLevel = totalLevel / summary.playerCount;
        summary.minLevel = minLevel;
        summary.maxLevel = maxLevel;
        summary.hasActivePlayers = true;
    }

    return summary;
}

BracketActivitySummary PlayerActivityTracker::GetBracketActivitySummary(ExpansionTier tier) const
{
    BracketActivitySummary summary;
    summary.tier = tier;

    std::set<uint32> zones;

    _playerActivity.for_each([&](auto const& pair) {
        if (pair.second.isActive && GetTierForLevel(pair.second.level) == tier)
        {
            ++summary.playerCount;
            zones.insert(pair.second.zoneId);
        }
    });

    summary.activeZones = std::vector<uint32>(zones.begin(), zones.end());

    // Calculate activity score (players * unique zones)
    summary.activityScore = static_cast<float>(summary.playerCount) *
                           (1.0f + 0.1f * summary.activeZones.size());

    return summary;
}

uint32 PlayerActivityTracker::GetActivePlayerCount() const
{
    if (_countsDirty)
    {
        uint32 count = 0;
        _playerActivity.for_each([&](auto const& pair) {
            if (pair.second.isActive)
                ++count;
        });
        _activePlayerCount = count;
        _countsDirty = false;
    }

    return _activePlayerCount.load();
}

bool PlayerActivityTracker::HasActivePlayersInZone(uint32 zoneId) const
{
    bool found = false;

    _playerActivity.for_each([&](auto const& pair) {
        if (!found && pair.second.isActive && pair.second.zoneId == zoneId)
        {
            found = true;
        }
    });

    return found;
}

PlayerActivity PlayerActivityTracker::GetPlayerActivity(ObjectGuid playerGuid) const
{
    auto activity = _playerActivity.find(playerGuid);
    return activity != _playerActivity.end() ? activity->second : PlayerActivity();
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void PlayerActivityTracker::SetConfig(ActivityTrackerConfig const& config)
{
    _config = config;
}

// ============================================================================
// STATISTICS
// ============================================================================

void PlayerActivityTracker::PrintActivityReport() const
{
    TC_LOG_INFO("playerbot.lifecycle", "=== Player Activity Report ===");
    TC_LOG_INFO("playerbot.lifecycle", "Total tracked: {}, Active: {}",
        GetTotalTrackedPlayers(), GetActivePlayerCount());

    // By bracket
    auto bracketCounts = GetPlayerCountByBracket();
    TC_LOG_INFO("playerbot.lifecycle", "By bracket: Starting={}, Chromie={}, DF={}, TWW={}",
        bracketCounts[ExpansionTier::Starting],
        bracketCounts[ExpansionTier::ChromieTime],
        bracketCounts[ExpansionTier::Dragonflight],
        bracketCounts[ExpansionTier::TheWarWithin]);

    // Top zones
    auto topZones = GetHighActivityZones(5);
    if (!topZones.empty())
    {
        std::string zonesStr;
        auto zoneCounts = GetPlayerCountByZone();
        for (size_t i = 0; i < topZones.size(); ++i)
        {
            if (i > 0) zonesStr += ", ";
            zonesStr += std::to_string(topZones[i]) + "(" +
                       std::to_string(zoneCounts[topZones[i]]) + ")";
        }
        TC_LOG_INFO("playerbot.lifecycle", "Top zones (id:count): {}", zonesStr);
    }
}

uint32 PlayerActivityTracker::GetTotalTrackedPlayers() const
{
    return static_cast<uint32>(_playerActivity.size());
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void PlayerActivityTracker::CleanupStaleEntries()
{
    auto now = std::chrono::system_clock::now();
    uint32 removed = 0;

    std::vector<ObjectGuid> toRemove;

    _playerActivity.for_each([&](auto const& pair) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - pair.second.lastUpdate);

        // Remove very stale entries (10x threshold)
        if (elapsed.count() > _config.staleThresholdSeconds * 10)
        {
            toRemove.push_back(pair.first);
        }
    });

    for (ObjectGuid guid : toRemove)
    {
        _playerActivity.erase(guid);
        ++removed;
    }

    if (removed > 0)
    {
        _countsDirty = true;
        TC_LOG_DEBUG("playerbot.lifecycle", "Cleaned up {} stale player activity entries",
            removed);
    }
}

bool PlayerActivityTracker::IsStale(PlayerActivity const& activity) const
{
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - activity.lastUpdate);

    return elapsed.count() > _config.staleThresholdSeconds;
}

void PlayerActivityTracker::UpdateActiveStatus()
{
    // Collect GUIDs and their new active status since for_each is const
    std::vector<std::pair<ObjectGuid, bool>> updates;
    _playerActivity.for_each([&](auto const& pair) {
        bool shouldBeActive = !IsStale(pair.second);
        if (pair.second.isActive != shouldBeActive)
        {
            updates.emplace_back(pair.first, shouldBeActive);
        }
    });

    // Apply the updates
    for (auto const& [guid, newStatus] : updates)
    {
        auto it = _playerActivity.find(guid);
        if (it != _playerActivity.end())
        {
            it->second.isActive = newStatus;
        }
    }

    _countsDirty = true;
}

ExpansionTier PlayerActivityTracker::GetTierForLevel(uint32 level) const
{
    BotLevelDistribution* dist = BotLevelDistribution::instance();
    if (!dist)
    {
        // Fallback
        if (level <= 10) return ExpansionTier::Starting;
        if (level <= 60) return ExpansionTier::ChromieTime;
        if (level <= 70) return ExpansionTier::Dragonflight;
        return ExpansionTier::TheWarWithin;
    }

    LevelBracket const* bracket = dist->GetBracketForLevel(level, TEAM_NEUTRAL);
    return bracket ? bracket->tier : ExpansionTier::Starting;
}

} // namespace Playerbot
