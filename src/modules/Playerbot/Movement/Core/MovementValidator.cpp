/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "MovementValidator.h"
#include "Player.h"
#include "Map.h"

#include "VMapFactory.h"
#include "VMapDefinitions.h"
#include "PhaseShift.h"
#include "Log.h"
#include "World.h"
#include <random>

namespace Playerbot
{
    MovementValidator::MovementValidator()
        : _stuckDetectionEnabled(true),
          _stuckThreshold(MovementConstants::STUCK_THRESHOLD),
          _stuckCheckInterval(MovementConstants::STUCK_CHECK_INTERVAL),
          _maxStuckCounter(MovementConstants::MAX_STUCK_COUNTER),
          _dangerousTerrainBuffer(2.0f),
          _voidThreshold(-500.0f),
          _maxFallDistance(60.0f),
          _totalValidations(0),
          _totalFailures(0),
          _totalStuckDetections(0),
          _totalUnstuckAttempts(0)
    {
        // Initialize known danger zones (could be loaded from database)
        // Example: Lava in Molten Core
        _dangerZones.push_back({409, Position(-7516.0f, -1040.0f, 180.0f), 100.0f,
                               TerrainType::TERRAIN_LAVA});
    }

    MovementValidator::~MovementValidator()
    {
        // Cleanup
    }

    bool MovementValidator::ValidateDestination(Player* bot, Position const& destination) const
    {
        if (!bot || !bot->GetMap())
            return false;

        _totalValidations.fetch_add(1);

        Map* map = bot->GetMap();
        // Check if destination is in void
    if (IsVoidPosition(map, destination))
        {
            TC_LOG_DEBUG("playerbot.movement", "Destination in void for bot %s",
                bot->GetName().c_str());
            _totalFailures.fetch_add(1);
            return false;
        }
        // Check if destination is in dangerous terrain
    if (IsDangerousTerrain(map, destination))
        {
            TC_LOG_DEBUG("playerbot.movement", "Destination in dangerous terrain for bot %s",
                bot->GetName().c_str());
            _totalFailures.fetch_add(1);
            return false;
        }

        // Check if destination requires flying and bot can't fly
    if (RequiresFlying(map, destination) && !bot->CanFly())
        {
            TC_LOG_DEBUG("playerbot.movement", "Destination requires flying for bot %s",
                bot->GetName().c_str());
            _totalFailures.fetch_add(1);
            return false;
        }
        // Check if destination is reachable (basic LOS check)
    if (!HasLineOfSight(map, bot->GetPosition(), destination))
        {
            // LOS blocked doesn't mean unreachable if we can path around
            // This is just a quick check, pathfinding will do detailed validation
            TC_LOG_DEBUG("playerbot.movement", "No direct LOS to destination for bot %s",
                bot->GetName().c_str());
        }

        // Check height difference for non-flying movement
    if (!bot->CanFly())
        {
            float currentZ = bot->GetPositionZ();
            float destZ = destination.GetPositionZ();
            float groundZ = destZ;

            if (GetGroundHeight(map, destination.GetPositionX(),
                              destination.GetPositionY(), groundZ))
            {
                float heightDiff = ::std::abs(destZ - groundZ);
                if (heightDiff > 10.0f) // Destination is too far from ground
                {
                    TC_LOG_DEBUG("playerbot.movement",
                        "Destination too high above ground (%.2f) for bot %s",
                        heightDiff, bot->GetName().c_str());
                    _totalFailures.fetch_add(1);
                    return false;
                }

                // Check if we'd need to fall too far
                float fallDistance = currentZ - groundZ;
                if (fallDistance > 0 && !IsSafeFall(bot, fallDistance))
                {
                    TC_LOG_DEBUG("playerbot.movement",
                        "Unsafe fall distance (%.2f) to destination for bot %s",
                        fallDistance, bot->GetName().c_str());
                    _totalFailures.fetch_add(1);
                    return false;
                }
            }
        }

        return true;
    }

    bool MovementValidator::ValidatePath(Player* bot, MovementPath const& path) const
    {
        if (!bot || !path.IsValid())
            return false;

        _totalValidations.fetch_add(1);

        // Validate each segment
    for (size_t i = 0; i < path.nodes.size() - 1; ++i)
        {
            if (!ValidatePathSegment(bot, path.nodes[i].position, path.nodes[i + 1].position))
            {
                _totalFailures.fetch_add(1);
                return false;
            }
        }

        // Check total path length
    if (path.totalLength > MovementConstants::DISTANCE_VERY_FAR * 3)
        {
            TC_LOG_DEBUG("playerbot.movement", "Path too long (%.2f) for bot %s",
                path.totalLength, bot->GetName().c_str());
            _totalFailures.fetch_add(1);
            return false;
        }

        return true;
    }

    bool MovementValidator::ValidatePathSegment(Player* bot, Position const& start,
                                               Position const& end) const
    {
        if (!bot || !bot->GetMap())
            return false;

        Map* map = bot->GetMap();
        // Check segment length
        float distance = start.GetExactDist(&end);
        if (distance > MovementConstants::DISTANCE_FAR)
        {
            TC_LOG_DEBUG("playerbot.movement", "Path segment too long (%.2f)",
                distance);
            return false;
        }

        // Check for collision
    if (CheckCollision(map, start, end))
        {
            TC_LOG_DEBUG("playerbot.movement", "Collision detected in path segment");
            return false;
        }

        // Check terrain along segment (sample at intervals)
        uint32 samples = static_cast<uint32>(distance / 5.0f) + 1;
        for (uint32 i = 0; i <= samples; ++i)
        {
            float t = static_cast<float>(i) / samples;
            Position samplePos;
            samplePos.m_positionX = start.GetPositionX() + t * (end.GetPositionX() - start.GetPositionX());
            samplePos.m_positionY = start.GetPositionY() + t * (end.GetPositionY() - start.GetPositionY());
            samplePos.m_positionZ = start.GetPositionZ() + t * (end.GetPositionZ() - start.GetPositionZ());

            if (IsDangerousTerrain(map, samplePos))
                return false;
        }

        return true;
    }
    bool MovementValidator::IsDangerousTerrain(Map* map, Position const& position) const
    {
        if (!map)
            return false;

        // Check known danger zones
    for (auto const& zone : _dangerZones)
        {
            if (zone.mapId == map->GetId() &&
                zone.center.GetExactDist(&position) <= zone.radius)
            {
                return true;
            }
        }
        // Check liquid type at position
        PhaseShift phaseShift;
        LiquidData liquidData;
        ZLiquidStatus liquidStatus = map->GetLiquidStatus(phaseShift,
            position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(),
            map_liquidHeaderTypeFlags::AllLiquids, &liquidData);

        if (liquidStatus != LIQUID_MAP_NO_WATER)
        {
            // Check if it's lava or slime
    if (liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Magma) ||
                liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Slime))
                return true;
        }

        return false;
    }
    bool MovementValidator::IsVoidPosition(Map* map, Position const& position) const
    {
        if (!map)
            return true;

        float groundZ = position.GetPositionZ();
        if (!GetGroundHeight(map, position.GetPositionX(), position.GetPositionY(), groundZ))
            return true;

        // Check if ground is too far below (void)
    if (groundZ < _voidThreshold)
            return true;

        // Check if position is outside map bounds using line of sight check
        PhaseShift phaseShift;
        if (!map->isInLineOfSight(phaseShift, position.GetPositionX(), position.GetPositionY(),
            position.GetPositionZ() + 2.0f, position.GetPositionX(),
            position.GetPositionY(), position.GetPositionZ() - 100.0f,
            LINEOFSIGHT_CHECK_VMAP, VMAP::ModelIgnoreFlags::Nothing))
        {
            return true;
        }

        return false;
    }

    bool MovementValidator::IsStuck(Player* bot)
    {
        if (!bot || !_stuckDetectionEnabled)
            return false;

        ObjectGuid guid = bot->GetGUID();
        auto now = ::std::chrono::steady_clock::now();

        ::std::lock_guard lock(_dataLock);
        auto& data = _stuckData[guid];

        // Check if enough time has passed since last check
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            now - data.lastCheck).count();
        if (elapsed < _stuckCheckInterval)
            return data.isStuck;

        data.lastCheck = now;

        // Get current position
        Position currentPos = bot->GetPosition();
        // Calculate distance moved
        float distance = data.lastPosition.GetExactDist(&currentPos);
        data.totalDistanceMoved += distance;

        // Check if stuck
    if (distance < _stuckThreshold && bot->isMoving())
        {
            data.stuckCounter++;

            if (data.stuckCounter >= _maxStuckCounter)
            {
                if (!data.isStuck)
                {
                    data.isStuck = true;
                    data.stuckStartTime = now;
                    _totalStuckDetections.fetch_add(1);
                    TC_LOG_DEBUG("playerbot.movement", "Bot %s is stuck at position (%.2f, %.2f, %.2f)",
                        bot->GetName().c_str(), currentPos.GetPositionX(),
                        currentPos.GetPositionY(), currentPos.GetPositionZ());
                }
                return true;
            }
        }
        else
        {
            // Moving normally, reset counter
    if (data.stuckCounter > 0)
                data.stuckCounter--;
            if (data.isStuck && distance > _stuckThreshold * 2)
            {
                data.Reset();
                TC_LOG_DEBUG("playerbot.movement", "Bot %s is no longer stuck",
                    bot->GetName().c_str());
            }

            // Update last valid position if we've moved significantly
    if (distance > _stuckThreshold)
                data.lastValidPosition = currentPos;
        }

        data.lastPosition = currentPos;
        return data.isStuck;
    }

    bool MovementValidator::HandleStuck(Player* bot)
    {
        if (!bot)
            return false;

        ObjectGuid guid = bot->GetGUID();
        ::std::lock_guard lock(_dataLock);
        auto& data = _stuckData[guid];

        if (!data.isStuck)
            return true;

        data.unstuckAttempts++;
        _totalUnstuckAttempts.fetch_add(1);

        // Try different unstuck strategies based on attempt count
        Position unstuckPos;
        bool foundPosition = false;

        if (data.unstuckAttempts <= 3)
        {
            // Strategy 1: Move backward
            float angle = bot->GetOrientation() + static_cast<float>(M_PI);
            float distance = 5.0f + data.unstuckAttempts * 2.0f;
            unstuckPos = bot->GetNearPosition(distance, angle);
            foundPosition = ValidateDestination(bot, unstuckPos);
        }
        else if (data.unstuckAttempts <= 6)
        {
            // Strategy 2: Move to random direction
            foundPosition = CalculateUnstuckPosition(bot, unstuckPos);
        }
        else
        {
            // Strategy 3: Teleport to last valid position
    if (data.lastValidPosition.GetPositionX() != 0)
            {
                unstuckPos = data.lastValidPosition;
                foundPosition = true;
            }
        }

        if (foundPosition)
        {
            TC_LOG_DEBUG("playerbot.movement", "Attempting unstuck for bot %s to position (%.2f, %.2f, %.2f)",
                bot->GetName().c_str(), unstuckPos.GetPositionX(),
                unstuckPos.GetPositionY(), unstuckPos.GetPositionZ());

            // Reset stuck state
            data.Reset();
            return true;
        }

        // Give up after too many attempts
    if (data.unstuckAttempts > 10)
        {
            TC_LOG_WARN("playerbot.movement", "Failed to unstuck bot %s after %u attempts",
                bot->GetName().c_str(), data.unstuckAttempts);
            data.Reset();
            return false;
        }

        return false;
    }

    void MovementValidator::ResetStuckCounter(Player* bot)
    {
        if (!bot)
            return;

        ::std::lock_guard lock(_dataLock);
        auto it = _stuckData.find(bot->GetGUID());
        if (it != _stuckData.end())
        {
            it->second.Reset();
        }
    }

    bool MovementValidator::IsInWater(Map* map, Position const& position) const
    {
        if (!map)
            return false;

        PhaseShift phaseShift;
        LiquidData liquidData;
        ZLiquidStatus liquidStatus = map->GetLiquidStatus(phaseShift,
            position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(),
            map_liquidHeaderTypeFlags::AllLiquids, &liquidData);

        return liquidStatus != LIQUID_MAP_NO_WATER &&
               !liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Magma) &&
               !liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Slime);
    }

    bool MovementValidator::RequiresFlying(Map* map, Position const& position) const
    {
        if (!map)
            return false;

        // Check if position is high above ground
        float groundZ = position.GetPositionZ();
        if (GetGroundHeight(map, position.GetPositionX(), position.GetPositionY(), groundZ))
        {
            float heightAboveGround = position.GetPositionZ() - groundZ;
            return heightAboveGround > 30.0f;
        }

        return false;
    }

    bool MovementValidator::IsIndoors(Map* map, Position const& position) const
    {
        if (!map)
            return false;

        // TrinityCore 12.0 doesn't have IsIndoors() method
        // Alternative: Check if there's a roof above by doing a LOS check upward
        PhaseShift phaseShift;
        bool hasRoof = !map->isInLineOfSight(phaseShift,
            position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(),
            position.GetPositionX(), position.GetPositionY(), position.GetPositionZ() + 50.0f,
            LINEOFSIGHT_CHECK_VMAP, VMAP::ModelIgnoreFlags::Nothing);

        return hasRoof;
    }

    float MovementValidator::GetSafeFallDistance(Player* bot) const
    {
        if (!bot)
            return 0.0f;

        float safeDist = _maxFallDistance;

        // Check for fall damage reduction auras
    if (bot->HasAuraType(SPELL_AURA_SAFE_FALL))
            safeDist *= 2.0f;

        // Check for slow fall
    if (bot->HasAuraType(SPELL_AURA_FEATHER_FALL))
            safeDist = 1000.0f; // Effectively no fall damage

        return safeDist;
    }

    bool MovementValidator::HasLineOfSight(Map* map, Position const& start,
                                          Position const& end) const
    {
        if (!map)
            return false;

        PhaseShift phaseShift;
        return map->isInLineOfSight(phaseShift, start.GetPositionX(), start.GetPositionY(),
            start.GetPositionZ() + 2.0f, end.GetPositionX(), end.GetPositionY(),
            end.GetPositionZ() + 2.0f, LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::Nothing);
    }

    void MovementValidator::SetStuckParameters(float threshold, uint32 checkInterval,
                                              uint32 maxCounter)
    {
        _stuckThreshold = threshold;
        _stuckCheckInterval = checkInterval;
        _maxStuckCounter = maxCounter;
    }

    void MovementValidator::GetStatistics(uint32& validations, uint32& failures,
                                         uint32& stuckDetections) const
    {
        validations = _totalValidations.load();
        failures = _totalFailures.load();
        stuckDetections = _totalStuckDetections.load();
    }

    void MovementValidator::ResetStatistics()
    {
        _totalValidations.store(0);
        _totalFailures.store(0);
        _totalStuckDetections.store(0);
        _totalUnstuckAttempts.store(0);
    }

    TerrainType MovementValidator::GetTerrainType(Map* map, Position const& position) const
    {
        if (!map)
            return TerrainType::TERRAIN_GROUND;

        TerrainType terrain = TerrainType::TERRAIN_GROUND;

        // Check water
    if (IsInWater(map, position))
            terrain = TerrainType::TERRAIN_WATER;

        // Check indoor
    if (IsIndoors(map, position))
            terrain = static_cast<TerrainType>(terrain | TerrainType::TERRAIN_INDOOR);
        else
            terrain = static_cast<TerrainType>(terrain | TerrainType::TERRAIN_OUTDOOR);

        // Check if in air
    if (RequiresFlying(map, position))
            terrain = static_cast<TerrainType>(terrain | TerrainType::TERRAIN_AIR);

        return terrain;
    }

    bool MovementValidator::GetGroundHeight(Map* map, float x, float y, float& z) const
    {
        if (!map)
            return false;

        PhaseShift phaseShift;
        float groundZ = map->GetHeight(phaseShift, x, y, z, true, 100.0f);
        if (groundZ > INVALID_HEIGHT)
        {
            z = groundZ;
            return true;
        }

        return false;
    }

    bool MovementValidator::IsSafeFall(Player* bot, float fallDistance) const
    {
        if (!bot)
            return false;

        return fallDistance <= GetSafeFallDistance(bot);
    }

    bool MovementValidator::CalculateUnstuckPosition(Player* bot, Position& unstuckPos) const
    {
        if (!bot)
            return false;

        ::std::random_device rd;
        ::std::mt19937 gen(rd());
        ::std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * static_cast<float>(M_PI));
        ::std::uniform_real_distribution<float> distDist(5.0f, 15.0f);

        // Try random positions around the bot
    for (int i = 0; i < 8; ++i)
        {
            float angle = angleDist(gen);
            float distance = distDist(gen);

            unstuckPos = bot->GetNearPosition(distance, angle);

            if (ValidateDestination(bot, unstuckPos))
                return true;
        }

        return false;
    }

    bool MovementValidator::CheckCollision(Map* map, Position const& start,
                                          Position const& end) const
    {
        if (!map)
            return false;

        // Use VMAP to check for collision
        PhaseShift phaseShift;
        return !map->isInLineOfSight(phaseShift, start.GetPositionX(), start.GetPositionY(),
            start.GetPositionZ() + 2.0f, end.GetPositionX(), end.GetPositionY(),
            end.GetPositionZ() + 2.0f, LINEOFSIGHT_CHECK_VMAP, VMAP::ModelIgnoreFlags::Nothing);
    }
}