/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "WaypointPathManager.h"
#include "BotMovementUtil.h"
#include "Player.h"
#include "WaypointDefines.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

WaypointPathManager* WaypointPathManager::Instance()
{
    static WaypointPathManager instance;
    return &instance;
}

// ============================================================================
// PATH CREATION
// ============================================================================

uint32 WaypointPathManager::CreatePath(uint32 mapId, std::vector<Position> const& waypoints,
                                        ObjectGuid ownerGuid, bool repeatable)
{
    if (waypoints.empty())
    {
        TC_LOG_ERROR("module.playerbot.movement",
            "WaypointPathManager::CreatePath: Cannot create path with no waypoints");
        return 0;
    }

    BotWaypointPath path;
    path.pathId = AllocatePathId();
    path.mapId = mapId;
    path.ownerGuid = ownerGuid;
    path.type = BotPathType::CUSTOM;
    path.isRepeatable = repeatable;
    path.createdTime = GameTime::GetGameTimeMS();
    path.lastUsedTime = path.createdTime;

    for (size_t i = 0; i < waypoints.size(); ++i)
    {
        BotWaypoint wp;
        wp.id = static_cast<uint32>(i);
        wp.x = waypoints[i].GetPositionX();
        wp.y = waypoints[i].GetPositionY();
        wp.z = waypoints[i].GetPositionZ();
        wp.orientation = waypoints[i].GetOrientation();
        path.waypoints.push_back(wp);
    }

    std::unique_lock lock(_pathMutex);
    _runtimePaths[path.pathId] = std::move(path);

    TC_LOG_DEBUG("module.playerbot.movement",
        "WaypointPathManager: Created path {} with {} waypoints (owner: {}, map: {})",
        path.pathId, waypoints.size(), ownerGuid.ToString(), mapId);

    return _runtimePaths.rbegin()->second.pathId;
}

uint32 WaypointPathManager::CreatePatrolPath(Player* bot, std::vector<Position> const& waypoints)
{
    if (!bot || waypoints.empty())
        return 0;

    BotWaypointPathBuilder builder(bot->GetMapId());
    builder.SetType(BotPathType::PATROL)
           .SetOwner(bot->GetGUID())
           .SetName("Patrol_" + std::to_string(bot->GetGUID().GetCounter()))
           .SetRepeatable(true)
           .SetForceWalk(true);

    // Add waypoints with Z correction
    for (auto const& pos : waypoints)
    {
        Position correctedPos = pos;
        BotMovementUtil::CorrectPositionToGround(bot, correctedPos);
        builder.AddWaypoint(correctedPos);
    }

    BotWaypointPath path = builder.Build();
    return RegisterPath(std::move(path));
}

uint32 WaypointPathManager::CreateGatheringPath(Player* bot, std::vector<Position> const& nodePositions)
{
    if (!bot || nodePositions.empty())
        return 0;

    BotWaypointPathBuilder builder(bot->GetMapId());
    builder.SetType(BotPathType::GATHERING)
           .SetOwner(bot->GetGUID())
           .SetName("Gathering_" + std::to_string(bot->GetGUID().GetCounter()))
           .SetRepeatable(true)   // Gathering routes typically loop
           .SetForceWalk(false);  // Run between nodes for efficiency

    // Add waypoints with Z correction and small delay at each node
    for (auto const& pos : nodePositions)
    {
        Position correctedPos = pos;
        BotMovementUtil::CorrectPositionToGround(bot, correctedPos);
        builder.AddWaypoint(correctedPos, 2000);  // 2 second delay at each node for gathering
    }

    BotWaypointPath path = builder.Build();
    return RegisterPath(std::move(path));
}

uint32 WaypointPathManager::CreateQuestPath(Player* bot, std::vector<Position> const& waypoints)
{
    if (!bot || waypoints.empty())
        return 0;

    BotWaypointPathBuilder builder(bot->GetMapId());
    builder.SetType(BotPathType::QUEST_ROUTE)
           .SetOwner(bot->GetGUID())
           .SetName("Quest_" + std::to_string(bot->GetGUID().GetCounter()))
           .SetRepeatable(false)  // Quest routes are one-way
           .SetForceWalk(false);  // Run to quest objectives

    // Add waypoints with Z correction
    for (auto const& pos : waypoints)
    {
        Position correctedPos = pos;
        BotMovementUtil::CorrectPositionToGround(bot, correctedPos);
        builder.AddWaypoint(correctedPos);
    }

    BotWaypointPath path = builder.Build();
    return RegisterPath(std::move(path));
}

uint32 WaypointPathManager::CreateEscapePath(Player* bot, std::vector<Position> const& waypoints)
{
    if (!bot || waypoints.empty())
        return 0;

    BotWaypointPathBuilder builder(bot->GetMapId());
    builder.SetType(BotPathType::ESCAPE_ROUTE)
           .SetOwner(bot->GetGUID())
           .SetName("Escape_" + std::to_string(bot->GetGUID().GetCounter()))
           .SetRepeatable(false)
           .SetForceWalk(false);  // Run when escaping!

    // Add waypoints with Z correction
    for (auto const& pos : waypoints)
    {
        Position correctedPos = pos;
        BotMovementUtil::CorrectPositionToGround(bot, correctedPos);
        builder.AddWaypoint(correctedPos);
    }

    BotWaypointPath path = builder.Build();
    return RegisterPath(std::move(path));
}

uint32 WaypointPathManager::RegisterPath(BotWaypointPath path)
{
    if (path.waypoints.empty())
    {
        TC_LOG_ERROR("module.playerbot.movement",
            "WaypointPathManager::RegisterPath: Cannot register path with no waypoints");
        return 0;
    }

    // Allocate path ID if not set
    if (path.pathId == 0 || path.pathId < RUNTIME_PATH_ID_START)
        path.pathId = AllocatePathId();

    // Set creation time if not set
    if (path.createdTime == 0)
        path.createdTime = GameTime::GetGameTimeMS();
    path.lastUsedTime = path.createdTime;

    std::unique_lock lock(_pathMutex);
    uint32 pathId = path.pathId;
    _runtimePaths[pathId] = std::move(path);

    TC_LOG_DEBUG("module.playerbot.movement",
        "WaypointPathManager: Registered path {} (type: {}, waypoints: {})",
        pathId, static_cast<int>(_runtimePaths[pathId].type), _runtimePaths[pathId].waypoints.size());

    return pathId;
}

// ============================================================================
// PATH RETRIEVAL
// ============================================================================

BotWaypointPath const* WaypointPathManager::GetPath(uint32 pathId) const
{
    std::shared_lock lock(_pathMutex);

    auto it = _runtimePaths.find(pathId);
    if (it != _runtimePaths.end())
        return &it->second;

    return nullptr;
}

std::vector<uint32> WaypointPathManager::GetBotPaths(ObjectGuid ownerGuid) const
{
    std::vector<uint32> result;

    std::shared_lock lock(_pathMutex);
    for (auto const& [pathId, path] : _runtimePaths)
    {
        if (path.ownerGuid == ownerGuid)
            result.push_back(pathId);
    }

    return result;
}

std::vector<uint32> WaypointPathManager::GetPathsByType(BotPathType type) const
{
    std::vector<uint32> result;

    std::shared_lock lock(_pathMutex);
    for (auto const& [pathId, path] : _runtimePaths)
    {
        if (path.type == type)
            result.push_back(pathId);
    }

    return result;
}

bool WaypointPathManager::HasPath(uint32 pathId) const
{
    std::shared_lock lock(_pathMutex);
    return _runtimePaths.find(pathId) != _runtimePaths.end();
}

// ============================================================================
// PATH MANAGEMENT
// ============================================================================

void WaypointPathManager::MarkPathUsed(uint32 pathId)
{
    std::unique_lock lock(_pathMutex);

    auto it = _runtimePaths.find(pathId);
    if (it != _runtimePaths.end())
    {
        it->second.lastUsedTime = GameTime::GetGameTimeMS();
        it->second.useCount++;
    }
}

bool WaypointPathManager::RemovePath(uint32 pathId)
{
    std::unique_lock lock(_pathMutex);

    auto it = _runtimePaths.find(pathId);
    if (it != _runtimePaths.end())
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "WaypointPathManager: Removed path {} (used {} times)",
            pathId, it->second.useCount);
        _runtimePaths.erase(it);
        return true;
    }

    return false;
}

uint32 WaypointPathManager::CleanupBotPaths(ObjectGuid ownerGuid)
{
    uint32 removed = 0;

    std::unique_lock lock(_pathMutex);

    for (auto it = _runtimePaths.begin(); it != _runtimePaths.end(); )
    {
        if (it->second.ownerGuid == ownerGuid)
        {
            TC_LOG_DEBUG("module.playerbot.movement",
                "WaypointPathManager: Cleaning up path {} for bot {}",
                it->first, ownerGuid.ToString());
            it = _runtimePaths.erase(it);
            ++removed;
        }
        else
        {
            ++it;
        }
    }

    if (removed > 0)
    {
        TC_LOG_DEBUG("module.playerbot.movement",
            "WaypointPathManager: Cleaned up {} paths for bot {}",
            removed, ownerGuid.ToString());
    }

    return removed;
}

uint32 WaypointPathManager::CleanupExpiredPaths(uint32 maxAgeMs)
{
    uint32 removed = 0;

    std::unique_lock lock(_pathMutex);

    for (auto it = _runtimePaths.begin(); it != _runtimePaths.end(); )
    {
        if (it->second.IsExpired(maxAgeMs))
        {
            TC_LOG_DEBUG("module.playerbot.movement",
                "WaypointPathManager: Removing expired path {} (last used: {}ms ago)",
                it->first, GameTime::GetGameTimeMS() - it->second.lastUsedTime);
            it = _runtimePaths.erase(it);
            ++removed;
        }
        else
        {
            ++it;
        }
    }

    if (removed > 0)
    {
        TC_LOG_INFO("module.playerbot.movement",
            "WaypointPathManager: Cleaned up {} expired paths", removed);
    }

    return removed;
}

void WaypointPathManager::ClearAllRuntimePaths()
{
    std::unique_lock lock(_pathMutex);

    TC_LOG_INFO("module.playerbot.movement",
        "WaypointPathManager: Clearing all {} runtime paths", _runtimePaths.size());

    _runtimePaths.clear();
}

// ============================================================================
// PATH CONVERSION
// ============================================================================

WaypointPath* WaypointPathManager::ConvertToTrinityPath(uint32 pathId) const
{
    std::shared_lock lock(_pathMutex);

    auto it = _runtimePaths.find(pathId);
    if (it == _runtimePaths.end())
        return nullptr;

    BotWaypointPath const& botPath = it->second;

    // Create TrinityCore WaypointPath
    WaypointPath* tcPath = new WaypointPath();
    tcPath->Id = pathId;

    for (auto const& wp : botPath.waypoints)
    {
        WaypointNode node;
        node.Id = wp.id;
        node.X = wp.x;
        node.Y = wp.y;
        node.Z = wp.z;
        node.Orientation = wp.orientation;
        node.Delay = wp.delay;
        node.MoveType = static_cast<WaypointMoveType>(wp.moveType);
        node.Velocity = wp.velocity;
        // Note: EventId and EventChance left as defaults

        tcPath->Nodes.push_back(node);
    }

    return tcPath;
}

// ============================================================================
// STATISTICS
// ============================================================================

uint32 WaypointPathManager::GetRuntimePathCount() const
{
    std::shared_lock lock(_pathMutex);
    return static_cast<uint32>(_runtimePaths.size());
}

uint32 WaypointPathManager::GetTotalWaypointCount() const
{
    std::shared_lock lock(_pathMutex);

    uint32 total = 0;
    for (auto const& [pathId, path] : _runtimePaths)
        total += static_cast<uint32>(path.waypoints.size());

    return total;
}

uint32 WaypointPathManager::GetPathUsageCount(uint32 pathId) const
{
    std::shared_lock lock(_pathMutex);

    auto it = _runtimePaths.find(pathId);
    if (it != _runtimePaths.end())
        return it->second.useCount;

    return 0;
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

uint32 WaypointPathManager::AllocatePathId()
{
    return _nextPathId.fetch_add(1, std::memory_order_relaxed);
}

} // namespace Playerbot
