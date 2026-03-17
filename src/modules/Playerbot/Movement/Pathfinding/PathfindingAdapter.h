/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITY_PLAYERBOT_PATHFINDING_ADAPTER_H
#define TRINITY_PLAYERBOT_PATHFINDING_ADAPTER_H

#include "../Core/MovementTypes.h"
#include "Threading/LockHierarchy.h"
#include "PathGenerator.h"
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include <queue>

class Player;
class Unit;
class Map;

namespace Playerbot
{
    /**
     * @class PathfindingAdapter
     * @brief Wrapper for TrinityCore's PathGenerator with caching and optimization
     *
     * This class provides an interface to TrinityCore's pathfinding system
     * with additional features like path caching, asynchronous calculation,
     * and performance optimization.
     */
    class TC_GAME_API PathfindingAdapter
    {
    public:
        /**
         * @brief Constructor
         */
        PathfindingAdapter();

        /**
         * @brief Destructor
         */
        ~PathfindingAdapter();

        /**
         * @brief Initialize the pathfinding adapter
         * @param cacheSize Maximum number of cached paths
         * @param cacheDuration Cache validity duration in milliseconds
         * @return True if initialization successful
         */
        bool Initialize(uint32 cacheSize = 100, uint32 cacheDuration = 5000);

        /**
         * @brief Shutdown and cleanup
         */
        void Shutdown();

        /**
         * @brief Calculate path from bot to destination
         * @param bot The bot player
         * @param destination Target position
         * @param path Output path
         * @param forceDirect Force direct path without pathfinding
         * @return True if path found
         */
        bool CalculatePath(Player* bot, Position const& destination,
                         MovementPath& path, bool forceDirect = false);

        /**
         * @brief Calculate path to a unit
         * @param bot The bot player
         * @param target Target unit
         * @param path Output path
         * @param range Stop distance from target
         * @return True if path found
         */
        bool CalculatePathToUnit(Player* bot, Unit* target,
                                MovementPath& path, float range = 0.0f);

        /**
         * @brief Calculate formation path
         * @param bot The bot player
         * @param leader Formation leader
         * @param offset Formation offset
         * @param path Output path
         * @return True if path found
         */
        bool CalculateFormationPath(Player* bot, Unit* leader,
                                   Position const& offset, MovementPath& path);

        /**
         * @brief Calculate flee path
         * @param bot The bot player
         * @param threat Threat to flee from
         * @param distance Flee distance
         * @param path Output path
         * @return True if path found
         */
        bool CalculateFleePath(Player* bot, Unit* threat,
                             float distance, MovementPath& path);

        /**
         * @brief Check if cached path exists
         * @param bot The bot player
         * @param destination Target position
         * @return True if valid cached path exists
         */
        bool HasCachedPath(Player* bot, Position const& destination) const;

        /**
         * @brief Get cached path if available
         * @param bot The bot player
         * @param destination Target position
         * @param path Output path
         * @return True if cached path found and valid
         */
        bool GetCachedPath(Player* bot, Position const& destination,
                         MovementPath& path) const;

        /**
         * @brief Clear path cache for a bot
         * @param bot The bot player
         */
        void ClearCache(Player* bot);

        /**
         * @brief Clear entire path cache
         */
        void ClearAllCache();

        /**
         * @brief Set path generation parameters
         * @param maxNodes Maximum nodes to search
         * @param straightDistance Max distance for straight path
         * @param maxSearchDistance Maximum search distance
         */
        void SetPathParameters(uint32 maxNodes = 3000,
                             float straightDistance = 10.0f,
                             float maxSearchDistance = 100.0f);

        /**
         * @brief Enable or disable path smoothing
         * @param enable True to enable smoothing
         */
        void EnablePathSmoothing(bool enable) { _enableSmoothing = enable; }

        /**
         * @brief Enable or disable path caching
         * @param enable True to enable caching
         */
        void EnableCaching(bool enable) { _enableCaching = enable; }

        /**
         * @brief Set cache parameters
         * @param maxSize Maximum cache size
         * @param duration Cache duration in milliseconds
         */
        void SetCacheParameters(uint32 maxSize, uint32 duration);

        /**
         * @brief Get cache statistics
         * @param hits Number of cache hits
         * @param misses Number of cache misses
         * @param evictions Number of cache evictions
         */
        void GetCacheStatistics(uint32& hits, uint32& misses, uint32& evictions) const;

        /**
         * @brief Get pathfinding statistics
         * @param totalPaths Total paths generated
         * @param avgTime Average generation time in microseconds
         * @param maxTime Maximum generation time in microseconds
         */
        void GetPathStatistics(uint32& totalPaths, uint32& avgTime, uint32& maxTime) const;

        /**
         * @brief Reset all statistics
         */
        void ResetStatistics();

        /**
         * @brief Check if position is walkable
         * @param map The map
         * @param position Position to check
         * @return True if position is walkable
         */
        bool IsWalkablePosition(Map* map, Position const& position) const;

        /**
         * @brief Get nearest walkable position
         * @param map The map
         * @param position Starting position
         * @param walkable Output walkable position
         * @param searchRange Search range
         * @return True if walkable position found
         */
        bool GetNearestWalkablePosition(Map* map, Position const& position,
                                       Position& walkable, float searchRange = 20.0f) const;

    private:
        /**
         * @brief Internal path calculation
         * @param generator Path generator to use
         * @param start Start position
         * @param end End position
         * @param bot The bot unit
         * @return True if path calculated
         */
        bool InternalCalculatePath(PathGenerator& generator,
                                 Position const& start,
                                 Position const& end,
                                 Unit const* bot);

        /**
         * @brief Convert PathGenerator path to MovementPath
         * @param generator Source path generator
         * @param path Output movement path
         */
        void ConvertPath(PathGenerator const& generator, MovementPath& path);

        /**
         * @brief Optimize path by removing unnecessary waypoints
         * @param path Path to optimize
         */
        void OptimizePath(MovementPath& path);

        /**
         * @brief Cache a calculated path
         * @param bot The bot player
         * @param destination Target position
         * @param path Path to cache
         */
        void CachePath(Player* bot, Position const& destination,
                      MovementPath const& path);

        /**
         * @brief Clean expired cache entries
         */
        void CleanExpiredCache();

        /**
         * @brief Calculate cache key
         * @param botGuid Bot GUID
         * @param destination Target position
         * @return Cache key
         */
        uint64 CalculateCacheKey(ObjectGuid botGuid, Position const& destination) const;

        /**
         * @brief Check if two positions are close enough to reuse path
         * @param pos1 First position
         * @param pos2 Second position
         * @param threshold Distance threshold
         * @return True if positions are close enough
         */
        bool ArePositionsClose(Position const& pos1, Position const& pos2,
                             float threshold = 2.0f) const;

        // Path cache entry
        struct CachedPath
        {
            MovementPath path;
            Position destination;
            ::std::chrono::steady_clock::time_point timestamp;
            uint32 hitCount;
            bool isValid;

            CachedPath() : hitCount(0), isValid(false)
            {
                timestamp = ::std::chrono::steady_clock::now();
            }

            bool IsExpired(uint32 duration) const
            {
                auto now = ::std::chrono::steady_clock::now();
                auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                    now - timestamp).count();
                return elapsed > duration;
            }
        };

        // Cache management
        mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::MOVEMENT_ARBITER> _cacheLock;
        ::std::unordered_map<uint64, CachedPath> _pathCache;
        ::std::queue<uint64> _cacheOrder; // LRU tracking

        // Configuration
        bool _enableCaching;
        bool _enableSmoothing;
        uint32 _maxCacheSize;
        uint32 _cacheDuration;
        uint32 _maxPathNodes;
        float _straightPathDistance;
        float _maxSearchDistance;

        // Statistics
        mutable ::std::atomic<uint32> _cacheHits;
        mutable ::std::atomic<uint32> _cacheMisses;
        mutable ::std::atomic<uint32> _cacheEvictions;
        ::std::atomic<uint32> _totalPathsGenerated;
        ::std::atomic<uint64> _totalGenerationTime;
        ::std::atomic<uint32> _maxGenerationTime;

        // Performance tracking
        ::std::chrono::steady_clock::time_point _lastCacheClean;
        uint32 _cacheCleanInterval;

        // Thread pool for async path calculation (future enhancement)
        // std::unique_ptr<ThreadPool> _threadPool;
    };
}

#endif // TRINITY_PLAYERBOT_PATHFINDING_ADAPTER_H