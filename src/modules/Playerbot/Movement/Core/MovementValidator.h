/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITY_PLAYERBOT_MOVEMENT_VALIDATOR_H
#define TRINITY_PLAYERBOT_MOVEMENT_VALIDATOR_H

#include "MovementTypes.h"
#include "Threading/LockHierarchy.h"
#include "Define.h"
#include <unordered_map>
#include <shared_mutex>
#include <chrono>

class Player;
class Map;

namespace Playerbot
{
    /**
     * @class MovementValidator
     * @brief Validates movement paths and detects stuck situations
     *
     * This class provides validation for movement destinations, paths,
     * and handles stuck detection and recovery.
     */
    class TC_GAME_API MovementValidator
    {
    public:
        /**
         * @brief Constructor
         */
        MovementValidator();

        /**
         * @brief Destructor
         */
        ~MovementValidator();

        /**
         * @brief Validate a destination is reachable and safe
         * @param bot The bot player
         * @param destination Target position
         * @return True if destination is valid
         */
        bool ValidateDestination(Player* bot, Position const& destination) const;

        /**
         * @brief Validate an entire path
         * @param bot The bot player
         * @param path The path to validate
         * @return True if path is valid
         */
        bool ValidatePath(Player* bot, MovementPath const& path) const;

        /**
         * @brief Validate a single path segment
         * @param bot The bot player
         * @param start Start position
         * @param end End position
         * @return True if segment is valid
         */
        bool ValidatePathSegment(Player* bot, Position const& start,
                               Position const& end) const;

        /**
         * @brief Check if position is in dangerous terrain
         * @param map The map
         * @param position Position to check
         * @return True if terrain is dangerous
         */
        bool IsDangerousTerrain(Map* map, Position const& position) const;

        /**
         * @brief Check if position is in void/unreachable
         * @param map The map
         * @param position Position to check
         * @return True if position is void
         */
        bool IsVoidPosition(Map* map, Position const& position) const;

        /**
         * @brief Check if bot is stuck
         * @param bot The bot player
         * @return True if stuck detected
         */
        bool IsStuck(Player* bot);

        /**
         * @brief Handle stuck situation
         * @param bot The bot player
         * @return True if unstuck successful
         */
        bool HandleStuck(Player* bot);

        /**
         * @brief Reset stuck counter for bot
         * @param bot The bot player
         */
        void ResetStuckCounter(Player* bot);

        /**
         * @brief Check if position is in water
         * @param map The map
         * @param position Position to check
         * @return True if in water
         */
        bool IsInWater(Map* map, Position const& position) const;

        /**
         * @brief Check if position requires flying
         * @param map The map
         * @param position Position to check
         * @return True if flying required
         */
        bool RequiresFlying(Map* map, Position const& position) const;

        /**
         * @brief Check if position is indoors
         * @param map The map
         * @param position Position to check
         * @return True if indoors
         */
        bool IsIndoors(Map* map, Position const& position) const;

        /**
         * @brief Calculate safe fall distance
         * @param bot The bot player
         * @return Maximum safe fall distance
         */
        float GetSafeFallDistance(Player* bot) const;

        /**
         * @brief Check line of sight between two positions
         * @param map The map
         * @param start Start position
         * @param end End position
         * @return True if line of sight exists
         */
        bool HasLineOfSight(Map* map, Position const& start,
                           Position const& end) const;

        /**
         * @brief Enable or disable stuck detection
         * @param enable True to enable
         */
        void EnableStuckDetection(bool enable) { _stuckDetectionEnabled = enable; }

        /**
         * @brief Set stuck detection parameters
         * @param threshold Movement threshold for stuck detection
         * @param checkInterval Interval between stuck checks in ms
         * @param maxCounter Maximum stuck count before giving up
         */
        void SetStuckParameters(float threshold, uint32 checkInterval, uint32 maxCounter);

        /**
         * @brief Get validation statistics
         * @param validations Total validations performed
         * @param failures Total validation failures
         * @param stuckDetections Total stuck detections
         */
        void GetStatistics(uint32& validations, uint32& failures,
                          uint32& stuckDetections) const;

        /**
         * @brief Reset all statistics
         */
        void ResetStatistics();

    private:
        /**
         * @brief Check terrain type at position
         * @param map The map
         * @param position Position to check
         * @return Terrain type flags
         */
        TerrainType GetTerrainType(Map* map, Position const& position) const;

        /**
         * @brief Calculate ground height at position
         * @param map The map
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate (modified with ground height)
         * @return True if ground height found
         */
        bool GetGroundHeight(Map* map, float x, float y, float& z) const;

        /**
         * @brief Check if fall distance is safe
         * @param bot The bot player
         * @param fallDistance Fall distance in yards
         * @return True if fall is safe
         */
        bool IsSafeFall(Player* bot, float fallDistance) const;

        /**
         * @brief Calculate unstuck position
         * @param bot The bot player
         * @param unstuckPos Output position for unstuck
         * @return True if unstuck position found
         */
        bool CalculateUnstuckPosition(Player* bot, Position& unstuckPos) const;

        /**
         * @brief Check collision between two points
         * @param map The map
         * @param start Start position
         * @param end End position
         * @return True if collision detected
         */
        bool CheckCollision(Map* map, Position const& start,
                          Position const& end) const;

        // Stuck detection data per bot
        struct StuckData
        {
            Position lastPosition;
            Position lastValidPosition;
            ::std::chrono::steady_clock::time_point lastCheck;
            ::std::chrono::steady_clock::time_point stuckStartTime;
            uint32 stuckCounter;
            uint32 unstuckAttempts;
            float totalDistanceMoved;
            bool isStuck;

            StuckData() : stuckCounter(0), unstuckAttempts(0),
                         totalDistanceMoved(0.0f), isStuck(false)
            {
                lastCheck = ::std::chrono::steady_clock::now();
                stuckStartTime = lastCheck;
            }

            void Reset()
            {
                stuckCounter = 0;
                unstuckAttempts = 0;
                totalDistanceMoved = 0.0f;
                isStuck = false;
                lastCheck = ::std::chrono::steady_clock::now();
                stuckStartTime = lastCheck;
            }
        };

        // Member variables
        mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::MOVEMENT_ARBITER> _dataLock;
        ::std::unordered_map<ObjectGuid, StuckData> _stuckData;

        // Configuration
        bool _stuckDetectionEnabled;
        float _stuckThreshold;
        uint32 _stuckCheckInterval;
        uint32 _maxStuckCounter;
        float _dangerousTerrainBuffer;
        float _voidThreshold;
        float _maxFallDistance;

        // Statistics
        mutable ::std::atomic<uint32> _totalValidations;
        mutable ::std::atomic<uint32> _totalFailures;
        mutable ::std::atomic<uint32> _totalStuckDetections;
        mutable ::std::atomic<uint32> _totalUnstuckAttempts;

        // Terrain danger zones (could be loaded from DB)
        struct DangerZone
        {
            uint32 mapId;
            Position center;
            float radius;
            TerrainType type;
        };
        ::std::vector<DangerZone> _dangerZones;
    };
}

#endif // TRINITY_PLAYERBOT_MOVEMENT_VALIDATOR_H