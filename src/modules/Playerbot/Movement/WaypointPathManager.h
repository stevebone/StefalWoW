/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * WAYPOINT PATH MANAGER
 *
 * Enterprise-grade waypoint path management for bot navigation.
 * Leverages TrinityCore 12.0's MovePath() support for players (commit 1db1a0e57f).
 *
 * Features:
 * - Dynamic runtime path creation (no database required)
 * - Path templates for common use cases (patrol, gathering, quest routes)
 * - Path caching and reuse
 * - Integration with TrinityCore waypoint_path database
 * - Automatic path cleanup for bot-specific paths
 *
 * Usage:
 *   // Create a patrol path
 *   uint32 pathId = sWaypointPathMgr->CreatePatrolPath(bot, waypoints);
 *   BotMovementUtil::MoveAlongPath(bot, pathId, true, true);
 *
 *   // Later, clean up
 *   sWaypointPathMgr->CleanupBotPaths(bot->GetGUID());
 */

#pragma once

#include "Define.h"
#include "Position.h"
#include "ObjectGuid.h"
#include "GameTime.h"
#include "Log.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <shared_mutex>
#include <atomic>

namespace Playerbot
{

// ============================================================================
// PATH TYPES
// ============================================================================

enum class BotPathType : uint8
{
    CUSTOM = 0,         // Custom path created at runtime
    PATROL,             // Looping patrol path
    GATHERING,          // Resource gathering route (mining, herbalism)
    QUEST_ROUTE,        // Path to quest objective
    DUNGEON_ROUTE,      // Path through dungeon
    ESCAPE_ROUTE,       // Emergency escape path
    FORMATION,          // Group formation movement
    BOSS_MECHANIC,      // Boss encounter movement pattern
    COUNT
};

// ============================================================================
// WAYPOINT DATA
// ============================================================================

/**
 * Single waypoint in a path.
 * Compatible with TrinityCore's WaypointPath structure.
 */
struct BotWaypoint
{
    uint32 id{0};           // Waypoint ID within path (0-indexed)
    float x{0.0f};          // X coordinate
    float y{0.0f};          // Y coordinate
    float z{0.0f};          // Z coordinate
    float orientation{0.0f}; // Facing direction (optional)
    uint32 delay{0};        // Delay at this waypoint in ms (0 = no delay)
    uint32 moveType{0};     // 0 = walk, 1 = run, 2 = fly
    uint32 action{0};       // Optional action to perform at waypoint
    float velocity{0.0f};   // Speed override (0 = default)

    BotWaypoint() = default;
    BotWaypoint(float _x, float _y, float _z, float _orientation = 0.0f, uint32 _delay = 0)
        : x(_x), y(_y), z(_z), orientation(_orientation), delay(_delay) {}

    Position GetPosition() const { return Position(x, y, z, orientation); }
};

// ============================================================================
// PATH DATA
// ============================================================================

/**
 * Complete waypoint path definition.
 */
struct BotWaypointPath
{
    uint32 pathId{0};                       // Unique path identifier
    BotPathType type{BotPathType::CUSTOM};  // Path type
    ObjectGuid ownerGuid;                   // Bot that created/owns this path (empty for shared)
    uint32 mapId{0};                        // Map ID for this path
    std::string name;                       // Optional path name for debugging
    std::vector<BotWaypoint> waypoints;     // Waypoints in order
    bool isRepeatable{false};               // Loop when reaching end
    bool forceWalk{false};                  // Always walk this path
    uint32 createdTime{0};                  // GameTime when path was created
    uint32 lastUsedTime{0};                 // GameTime when path was last used
    uint32 useCount{0};                     // Number of times path was used

    [[nodiscard]] bool IsEmpty() const { return waypoints.empty(); }
    [[nodiscard]] size_t Size() const { return waypoints.size(); }
    [[nodiscard]] bool IsExpired(uint32 maxAgeMs) const;
    [[nodiscard]] bool IsOwnedBy(ObjectGuid guid) const { return ownerGuid == guid; }
    [[nodiscard]] bool IsShared() const { return ownerGuid.IsEmpty(); }
};

// ============================================================================
// PATH BUILDER
// ============================================================================

/**
 * Fluent builder for creating waypoint paths.
 */
class BotWaypointPathBuilder
{
public:
    explicit BotWaypointPathBuilder(uint32 mapId)
    {
        _path.mapId = mapId;
        _path.createdTime = GameTime::GetGameTimeMS();
    }

    BotWaypointPathBuilder& SetType(BotPathType type)
    {
        _path.type = type;
        return *this;
    }

    BotWaypointPathBuilder& SetOwner(ObjectGuid guid)
    {
        _path.ownerGuid = guid;
        return *this;
    }

    BotWaypointPathBuilder& SetName(std::string name)
    {
        _path.name = std::move(name);
        return *this;
    }

    BotWaypointPathBuilder& SetRepeatable(bool repeatable)
    {
        _path.isRepeatable = repeatable;
        return *this;
    }

    BotWaypointPathBuilder& SetForceWalk(bool forceWalk)
    {
        _path.forceWalk = forceWalk;
        return *this;
    }

    BotWaypointPathBuilder& AddWaypoint(float x, float y, float z, float orientation = 0.0f, uint32 delay = 0)
    {
        BotWaypoint wp;
        wp.id = static_cast<uint32>(_path.waypoints.size());
        wp.x = x;
        wp.y = y;
        wp.z = z;
        wp.orientation = orientation;
        wp.delay = delay;
        _path.waypoints.push_back(wp);
        return *this;
    }

    BotWaypointPathBuilder& AddWaypoint(Position const& pos, uint32 delay = 0)
    {
        return AddWaypoint(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
                          pos.GetOrientation(), delay);
    }

    BotWaypointPathBuilder& AddWaypoints(std::vector<Position> const& positions)
    {
        for (auto const& pos : positions)
            AddWaypoint(pos);
        return *this;
    }

    [[nodiscard]] BotWaypointPath Build() { return std::move(_path); }

private:
    BotWaypointPath _path;
};

// ============================================================================
// WAYPOINT PATH MANAGER
// ============================================================================

/**
 * Central manager for bot waypoint paths.
 *
 * Thread Safety:
 * - All public methods are thread-safe (use read-write locks)
 * - Paths can be created/accessed from multiple threads safely
 *
 * Path ID Allocation:
 * - Database paths: IDs from waypoint_path table (typically 1-999999)
 * - Runtime paths: IDs starting at RUNTIME_PATH_ID_START (1000000+)
 * - This prevents collision with database-defined paths
 */
class TC_GAME_API WaypointPathManager
{
public:
    static constexpr uint32 RUNTIME_PATH_ID_START = 1000000;
    static constexpr uint32 DEFAULT_PATH_EXPIRY_MS = 300000; // 5 minutes

    static WaypointPathManager* Instance();

    // ========================================================================
    // PATH CREATION
    // ========================================================================

    /**
     * Create a custom path from waypoints.
     *
     * @param mapId Map ID for the path
     * @param waypoints List of positions
     * @param ownerGuid Bot that owns this path (empty for shared)
     * @param repeatable Loop when reaching end
     * @return Path ID (use with BotMovementUtil::MoveAlongPath)
     */
    uint32 CreatePath(uint32 mapId, std::vector<Position> const& waypoints,
                      ObjectGuid ownerGuid = ObjectGuid::Empty, bool repeatable = false);

    /**
     * Create a patrol path (always repeatable, walk speed).
     *
     * @param bot Bot that will use this path
     * @param waypoints Patrol waypoints
     * @return Path ID
     */
    uint32 CreatePatrolPath(Player* bot, std::vector<Position> const& waypoints);

    /**
     * Create a gathering route (mining, herbalism, etc.).
     *
     * @param bot Bot that will use this path
     * @param nodePositions Resource node positions
     * @return Path ID
     */
    uint32 CreateGatheringPath(Player* bot, std::vector<Position> const& nodePositions);

    /**
     * Create a quest route path.
     *
     * @param bot Bot that will use this path
     * @param waypoints Path to quest objective
     * @return Path ID
     */
    uint32 CreateQuestPath(Player* bot, std::vector<Position> const& waypoints);

    /**
     * Create an escape route (fast movement away from danger).
     *
     * @param bot Bot that will use this path
     * @param waypoints Escape waypoints
     * @return Path ID
     */
    uint32 CreateEscapePath(Player* bot, std::vector<Position> const& waypoints);

    /**
     * Register a path built with BotWaypointPathBuilder.
     *
     * @param path Complete path definition
     * @return Path ID
     */
    uint32 RegisterPath(BotWaypointPath path);

    // ========================================================================
    // PATH RETRIEVAL
    // ========================================================================

    /**
     * Get a path by ID.
     *
     * @param pathId Path ID
     * @return Path pointer (nullptr if not found)
     */
    BotWaypointPath const* GetPath(uint32 pathId) const;

    /**
     * Get all paths owned by a bot.
     *
     * @param ownerGuid Bot GUID
     * @return Vector of path IDs
     */
    std::vector<uint32> GetBotPaths(ObjectGuid ownerGuid) const;

    /**
     * Get all paths of a specific type.
     *
     * @param type Path type
     * @return Vector of path IDs
     */
    std::vector<uint32> GetPathsByType(BotPathType type) const;

    /**
     * Check if a path exists.
     *
     * @param pathId Path ID
     * @return true if path exists
     */
    bool HasPath(uint32 pathId) const;

    // ========================================================================
    // PATH MANAGEMENT
    // ========================================================================

    /**
     * Mark a path as used (updates lastUsedTime and useCount).
     *
     * @param pathId Path ID
     */
    void MarkPathUsed(uint32 pathId);

    /**
     * Remove a specific path.
     *
     * @param pathId Path ID
     * @return true if path was removed
     */
    bool RemovePath(uint32 pathId);

    /**
     * Remove all paths owned by a bot.
     *
     * @param ownerGuid Bot GUID
     * @return Number of paths removed
     */
    uint32 CleanupBotPaths(ObjectGuid ownerGuid);

    /**
     * Remove expired paths (not used recently).
     *
     * @param maxAgeMs Maximum age in milliseconds (default 5 minutes)
     * @return Number of paths removed
     */
    uint32 CleanupExpiredPaths(uint32 maxAgeMs = DEFAULT_PATH_EXPIRY_MS);

    /**
     * Remove all runtime paths.
     */
    void ClearAllRuntimePaths();

    // ========================================================================
    // PATH CONVERSION (for TrinityCore integration)
    // ========================================================================

    /**
     * Convert a BotWaypointPath to TrinityCore's WaypointPath format.
     * This is used internally by BotMovementUtil::MoveAlongPath().
     *
     * @param pathId Path ID
     * @return WaypointPath pointer (caller takes ownership, nullptr if not found)
     */
    struct WaypointPath* ConvertToTrinityPath(uint32 pathId) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    [[nodiscard]] uint32 GetRuntimePathCount() const;
    [[nodiscard]] uint32 GetTotalWaypointCount() const;
    [[nodiscard]] uint32 GetPathUsageCount(uint32 pathId) const;

private:
    WaypointPathManager() = default;
    ~WaypointPathManager() = default;

    WaypointPathManager(WaypointPathManager const&) = delete;
    WaypointPathManager& operator=(WaypointPathManager const&) = delete;

    uint32 AllocatePathId();

    mutable std::shared_mutex _pathMutex;
    std::unordered_map<uint32, BotWaypointPath> _runtimePaths;
    std::atomic<uint32> _nextPathId{RUNTIME_PATH_ID_START};
};

#define sWaypointPathMgr WaypointPathManager::Instance()

// ============================================================================
// INLINE IMPLEMENTATIONS
// ============================================================================

inline bool BotWaypointPath::IsExpired(uint32 maxAgeMs) const
{
    uint32 now = GameTime::GetGameTimeMS();
    return (now - lastUsedTime) > maxAgeMs && (now - createdTime) > maxAgeMs;
}

} // namespace Playerbot
