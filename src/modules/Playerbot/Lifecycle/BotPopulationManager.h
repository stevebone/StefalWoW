/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>

namespace Playerbot
{

// Zone population data structure
struct ZonePopulation
{
    uint32 zoneId;
    uint32 mapId;
    uint32 playerCount;
    uint32 botCount;
    uint32 targetBotCount;
    uint8 minLevel;
    uint8 maxLevel;
    float botDensity;
    ::std::chrono::system_clock::time_point lastUpdate;
};

/**
 * @class BotPopulationManager
 * @brief Manages zone population tracking and bot distribution
 *
 * SINGLE RESPONSIBILITY: Handles all zone population logic extracted
 * from the monolithic BotSpawner class.
 *
 * Responsibilities:
 * - Track bot and player populations per zone
 * - Calculate optimal bot distribution targets
 * - Maintain population density ratios
 * - Provide efficient population queries
 * - Handle dynamic population rebalancing
 *
 * Performance Features:
 * - Lock-free population counters for hot paths
 * - Cached population calculations
 * - Batched population updates
 * - Minimal memory footprint per zone
 */
class TC_GAME_API BotPopulationManager
{
public:
    BotPopulationManager();
    ~BotPopulationManager() = default;

    // Lifecycle
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // === POPULATION TRACKING ===
    void UpdateZonePopulation(uint32 zoneId, uint32 mapId);
    void AddBotToZone(uint32 zoneId, ObjectGuid botGuid);
    void RemoveBotFromZone(uint32 zoneId, ObjectGuid botGuid);

    // === POPULATION QUERIES ===
    ZonePopulation GetZonePopulation(uint32 zoneId) const;
    ::std::vector<ZonePopulation> GetAllZonePopulations() const;
    uint32 GetBotCountInZone(uint32 zoneId) const;
    uint32 GetTotalBotCount() const { return _totalBotCount.load(::std::memory_order_acquire); }

    // === TARGET CALCULATIONS ===
    void CalculateZoneTargets();
    uint32 CalculateTargetBotCount(ZonePopulation const& zone) const;
    ::std::vector<uint32> GetUnderpopulatedZones() const;
    ::std::vector<uint32> GetOverpopulatedZones() const;

    // === POPULATION LIMITS ===
    bool CanSpawnInZone(uint32 zoneId, uint32 maxBotsPerZone) const;
    bool CanSpawnOnMap(uint32 mapId, uint32 maxBotsPerMap) const;
    bool IsZoneAtCapacity(uint32 zoneId) const;

    // === CONFIGURATION ===
    void SetBotToPlayerRatio(float ratio) { _botToPlayerRatio = ratio; }
    void SetMaxBotsPerZone(uint32 maxBots) { _maxBotsPerZone = maxBots; }
    void SetMaxBotsPerMap(uint32 maxBots) { _maxBotsPerMap = maxBots; }
    void SetMaxBots(uint32 maxBots) { _maxBotsPerZone = maxBots; }

    float GetBotToPlayerRatio() const { return _botToPlayerRatio; }

    // === PERFORMANCE OPTIMIZATION ===
    void UpdatePopulationCache();
    void InvalidateCache(uint32 zoneId);

private:
    // Population data management
    void UpdateZoneData(uint32 zoneId, uint32 mapId);
    void CleanupStaleData();
    bool IsDataStale(ZonePopulation const& zone) const;

    // Lock-free counters for hot path operations
    ::std::atomic<uint32> _totalBotCount{0};
    ::std::atomic<uint32> _totalPlayerCount{0};

    // Zone population tracking (requires synchronization)
    mutable OrderedRecursiveMutex<LockOrder::BOT_SPAWNER> _populationMutex;
    ::std::unordered_map<uint32, ZonePopulation> _zonePopulations; // zoneId -> population data
    ::std::unordered_map<uint32, ::std::vector<ObjectGuid>> _botsByZone; // zoneId -> bot guids

    // Map-level tracking for map limits
    ::std::unordered_map<uint32, uint32> _botsPerMap; // mapId -> bot count

    // Configuration
    float _botToPlayerRatio = 2.0f;
    uint32 _maxBotsPerZone = 50;
    uint32 _maxBotsPerMap = 200;

    // Performance optimization
    struct PopulationCache
    {
        ::std::unordered_map<uint32, uint32> zoneBotCounts;
        ::std::chrono::steady_clock::time_point lastUpdate;
        bool isValid = false;
    };
    mutable PopulationCache _cache;
    mutable OrderedRecursiveMutex<LockOrder::BOT_SPAWNER> _cacheMutex;

    // Timing
    ::std::chrono::steady_clock::time_point _lastUpdate;
    static constexpr uint32 UPDATE_INTERVAL_MS = 10000; // 10 seconds
    static constexpr uint32 CACHE_VALIDITY_MS = 5000;   // 5 seconds
    static constexpr uint32 DATA_STALE_THRESHOLD_MS = 300000; // 5 minutes

    // Non-copyable
    BotPopulationManager(BotPopulationManager const&) = delete;
    BotPopulationManager& operator=(BotPopulationManager const&) = delete;
};

} // namespace Playerbot