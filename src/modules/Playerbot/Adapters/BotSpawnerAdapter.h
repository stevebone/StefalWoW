/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Lifecycle/BotSpawner.h"  // For SpawnConfig and SpawnStats definitions
#include "Define.h"
#include <memory>

namespace Playerbot
{

// Forward declarations
class BotSpawnOrchestrator;

/**
 * @class BotSpawnerAdapter
 * @brief Adapter that makes BotSpawnOrchestrator implement IBotSpawner interface
 *
 * ADAPTER PATTERN: Provides backward compatibility by adapting the new
 * orchestrator-based architecture to the legacy BotSpawner interface.
 *
 * Benefits:
 * - Maintains API compatibility during refactoring
 * - Allows gradual migration to new architecture
 * - Provides clean interface for external consumers
 * - Enables dependency injection and testing
 * - Isolates interface changes from implementation changes
 *
 * Usage:
 * ```cpp
 * auto spawner = std::make_unique<BotSpawnerAdapter>();
 * spawner->Initialize();
 * spawner->SpawnBot(request);  // Delegates to orchestrator
 * ```
 */
class TC_GAME_API BotSpawnerAdapter 
{
public:
    BotSpawnerAdapter();
    ~BotSpawnerAdapter();

    // === LIFECYCLE (IBotSpawner) ===
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // === SPAWNING INTERFACE (IBotSpawner) ===
    bool SpawnBot(SpawnRequest const& request);
    uint32 SpawnBots(::std::vector<SpawnRequest> const& requests);

    // === POPULATION MANAGEMENT (IBotSpawner) ===
    void SpawnToPopulationTarget();
    void UpdatePopulationTargets();
    bool DespawnBot(ObjectGuid guid, ::std::string const& reason);
    void DespawnBot(ObjectGuid guid, bool forced = false);

    // === QUERIES (IBotSpawner) ===
    uint32 GetActiveBotCount() const;
    uint32 GetActiveBotCount(uint32 zoneId) const;
    bool CanSpawnMore() const;
    bool CanSpawnInZone(uint32 zoneId) const;

    // === CONFIGURATION (IBotSpawner) ===
    void LoadConfig();
    SpawnConfig const& GetConfig() const;
    void SetConfig(SpawnConfig const& config);
    void SetMaxBots(uint32 maxBots);
    void SetBotToPlayerRatio(float ratio);
    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    // === ZONE/MAP MANAGEMENT (IBotSpawner) ===
    void DespawnAllBots();
    void UpdateZonePopulation(uint32 zoneId, uint32 mapId);
    void UpdateZonePopulationSafe(uint32 zoneId, uint32 mapId);
    ZonePopulation GetZonePopulation(uint32 zoneId) const;
    ::std::vector<ZonePopulation> GetAllZonePopulations() const;

    // === BOT QUERIES (IBotSpawner) ===
    bool IsBotActive(ObjectGuid guid) const;
    uint32 GetActiveBotCount(uint32 mapId, bool useMapId) const;
    ::std::vector<ObjectGuid> GetActiveBotsInZone(uint32 zoneId) const;
    bool CanSpawnOnMap(uint32 mapId) const;

    // === ADVANCED SPAWNING (IBotSpawner) ===
    bool CreateAndSpawnBot(uint32 masterAccountId, uint8 classId, uint8 race, uint8 gender, ::std::string const& name, ObjectGuid& outCharacterGuid);

    // === STATISTICS (IBotSpawner) ===
    SpawnStats const& GetStats() const;
    void ResetStats();

    // === PLAYER INTERACTION (IBotSpawner) ===
    void OnPlayerLogin();
    void CheckAndSpawnForPlayers();

    // === ADAPTER-SPECIFIC METHODS ===
    BotSpawnOrchestrator* GetOrchestrator() const { return _orchestrator.get(); }

    // Performance metrics access
    struct AdapterStats
    {
        uint32 callsToSpawnBot = 0;
        uint32 callsToSpawnBots = 0;
        uint32 callsToDespawnBot = 0;
        uint32 queryCalls = 0;
        uint64 avgCallDurationUs = 0;
    };

    AdapterStats const& GetAdapterStats() const { return _stats; }
    void ResetAdapterStats();

private:
    // === ORCHESTRATOR DELEGATION ===
    ::std::unique_ptr<BotSpawnOrchestrator> _orchestrator;

    // === CONFIGURATION STATE ===
    bool _enabled = true;
    uint32 _maxBots = 1000;
    float _botToPlayerRatio = 2.0f;
    mutable SpawnConfig _config;
    mutable SpawnStats _spawnStats;

    // === PERFORMANCE TRACKING ===
    mutable AdapterStats _stats;
    void RecordApiCall(uint64 durationMicroseconds);

    // === INITIALIZATION HELPERS ===
    bool InitializeOrchestrator();
    void ConfigureOrchestrator();

    // Non-copyable
    BotSpawnerAdapter(BotSpawnerAdapter const&) = delete;
    BotSpawnerAdapter& operator=(BotSpawnerAdapter const&) = delete;
};

/**
 * @class LegacyBotSpawnerAdapter
 * @brief Adapter for legacy BotSpawner class during migration
 *
 * This adapter allows the old BotSpawner to work with the new interface
 * during the transition period. It can be removed once migration is complete.
 */
class TC_GAME_API LegacyBotSpawnerAdapter 
{
public:
    LegacyBotSpawnerAdapter();
    ~LegacyBotSpawnerAdapter();

    // IBotSpawner implementation delegating to legacy BotSpawner
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    bool SpawnBot(SpawnRequest const& request);
    uint32 SpawnBots(::std::vector<SpawnRequest> const& requests);

    void SpawnToPopulationTarget();
    void UpdatePopulationTargets();
    bool DespawnBot(ObjectGuid guid, ::std::string const& reason);
    void DespawnBot(ObjectGuid guid, bool forced = false);

    uint32 GetActiveBotCount() const;
    uint32 GetActiveBotCount(uint32 zoneId) const;
    bool CanSpawnMore() const;
    bool CanSpawnInZone(uint32 zoneId) const;

    void SetMaxBots(uint32 maxBots);
    void SetBotToPlayerRatio(float ratio);
    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    // Additional IBotSpawner methods
    void LoadConfig();
    SpawnConfig const& GetConfig() const;
    void SetConfig(SpawnConfig const& config);
    void DespawnAllBots();
    void UpdateZonePopulation(uint32 zoneId, uint32 mapId);
    void UpdateZonePopulationSafe(uint32 zoneId, uint32 mapId);
    ZonePopulation GetZonePopulation(uint32 zoneId) const;
    ::std::vector<ZonePopulation> GetAllZonePopulations() const;
    bool IsBotActive(ObjectGuid guid) const;
    uint32 GetActiveBotCount(uint32 mapId, bool useMapId) const;
    ::std::vector<ObjectGuid> GetActiveBotsInZone(uint32 zoneId) const;
    bool CanSpawnOnMap(uint32 mapId) const;
    bool CreateAndSpawnBot(uint32 masterAccountId, uint8 classId, uint8 race, uint8 gender, ::std::string const& name, ObjectGuid& outCharacterGuid);
    SpawnStats const& GetStats() const;
    void ResetStats();
    void OnPlayerLogin();
    void CheckAndSpawnForPlayers();

private:
    class BotSpawner* _legacySpawner = nullptr; // Non-owning pointer to singleton
    bool _migrationMode = true;  // Flag to indicate we're in migration mode
    mutable SpawnConfig _config;
    mutable SpawnStats _stats;
};

/**
 * @class BotSpawnerFactory
 * @brief Factory for creating appropriate bot spawner implementations
 *
 * Allows runtime selection of spawner implementation based on configuration.
 */
class TC_GAME_API BotSpawnerFactory
{
public:
    enum class SpawnerType
    {
        LEGACY,      // Use old BotSpawner implementation
        ORCHESTRATED, // Use new orchestrator-based implementation
        AUTO         // Auto-detect based on configuration
    };

    static ::std::unique_ptr<BotSpawnerAdapter> CreateSpawner(SpawnerType type = SpawnerType::AUTO);
    static SpawnerType DetectBestSpawnerType();
    static ::std::string GetSpawnerTypeName(SpawnerType type);

private:
    static bool IsOrchestratorAvailable();
    static bool ShouldUseLegacySpawner();
};

} // namespace Playerbot