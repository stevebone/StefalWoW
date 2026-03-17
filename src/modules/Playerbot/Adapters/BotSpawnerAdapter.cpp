/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotSpawnerAdapter.h"
#include "Lifecycle/BotSpawnOrchestrator.h"
#include "Lifecycle/BotSpawner.h"
#include "Lifecycle/BotCharacterSelector.h"
#include "Lifecycle/BotPerformanceMonitor.h"
#include "Lifecycle/BotResourcePool.h"
#include "Lifecycle/BotPopulationManager.h"
#include "Session/BotSessionFactory.h"
#include "Config/PlayerbotConfig.h"
#include "Logging/Log.h"
#include <chrono>

namespace Playerbot
{

// =====================================================
// BotSpawnerAdapter Implementation
// =====================================================

BotSpawnerAdapter::BotSpawnerAdapter()
{
    TC_LOG_DEBUG("module.playerbot.adapter",
        "BotSpawnerAdapter: Creating adapter for orchestrator-based spawning");
}

BotSpawnerAdapter::~BotSpawnerAdapter()
{
    Shutdown();
}

bool BotSpawnerAdapter::Initialize()
{
    TC_LOG_INFO("module.playerbot.adapter",
        "BotSpawnerAdapter: Initializing orchestrator adapter");

    if (!InitializeOrchestrator())
    {
        TC_LOG_ERROR("module.playerbot.adapter",
            "BotSpawnerAdapter: Failed to initialize orchestrator");
        return false;
    }

    ConfigureOrchestrator();
    _enabled = sPlayerbotConfig->GetBool("Playerbot.Enable", false);

    TC_LOG_INFO("module.playerbot.adapter",
        "BotSpawnerAdapter: Successfully initialized (enabled: {})", _enabled);
    return true;
}

void BotSpawnerAdapter::Shutdown()
{
    TC_LOG_INFO("module.playerbot.adapter",
        "BotSpawnerAdapter: Shutting down adapter and orchestrator");

    if (_orchestrator)
    {
        _orchestrator->Shutdown();
        _orchestrator.reset();
    }
}

void BotSpawnerAdapter::Update(uint32 diff)
{
    if (!_enabled || !_orchestrator)
        return;

    auto start = ::std::chrono::high_resolution_clock::now();
    _orchestrator->Update(diff);
    auto end = ::std::chrono::high_resolution_clock::now();

    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start).count();
    RecordApiCall(duration);
}

bool BotSpawnerAdapter::SpawnBot(SpawnRequest const& request)
{
    if (!_enabled || !_orchestrator)
        return false;

    auto start = ::std::chrono::high_resolution_clock::now();
    ++_stats.callsToSpawnBot;

    bool result = _orchestrator->SpawnBot(request);

    auto end = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start).count();
    RecordApiCall(duration);

    return result;
}

uint32 BotSpawnerAdapter::SpawnBots(::std::vector<SpawnRequest> const& requests)
{
    if (!_enabled || !_orchestrator)
        return 0;

    auto start = ::std::chrono::high_resolution_clock::now();
    ++_stats.callsToSpawnBots;

    uint32 result = _orchestrator->SpawnBots(requests);

    auto end = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start).count();
    RecordApiCall(duration);

    return result;
}

void BotSpawnerAdapter::SpawnToPopulationTarget()
{
    if (!_enabled || !_orchestrator)
        return;

    _orchestrator->SpawnToPopulationTarget();
}

void BotSpawnerAdapter::UpdatePopulationTargets()
{
    if (!_enabled || !_orchestrator)
        return;

    _orchestrator->UpdatePopulationTargets();
}

bool BotSpawnerAdapter::DespawnBot(ObjectGuid guid, ::std::string const& reason)
{
    if (!_enabled || !_orchestrator)
        return false;

    ++_stats.callsToDespawnBot;
    return _orchestrator->DespawnBot(guid, reason);
}

void BotSpawnerAdapter::DespawnBot(ObjectGuid guid, bool forced)
{
    if (!_enabled || !_orchestrator)
        return;

    ++_stats.callsToDespawnBot;
    _orchestrator->DespawnBot(guid, forced);
}

uint32 BotSpawnerAdapter::GetActiveBotCount() const
{
    if (!_enabled || !_orchestrator)
        return 0;

    ++_stats.queryCalls;
    return _orchestrator->GetActiveBotCount();
}

uint32 BotSpawnerAdapter::GetActiveBotCount(uint32 zoneId) const
{
    if (!_enabled || !_orchestrator)
        return 0;

    ++_stats.queryCalls;
    return _orchestrator->GetActiveBotCount(zoneId);
}

bool BotSpawnerAdapter::CanSpawnMore() const
{
    if (!_enabled || !_orchestrator)
        return false;

    ++_stats.queryCalls;
    return _orchestrator->CanSpawnMore();
}

bool BotSpawnerAdapter::CanSpawnInZone(uint32 zoneId) const
{
    if (!_enabled || !_orchestrator)
        return false;

    ++_stats.queryCalls;
    return _orchestrator->CanSpawnInZone(zoneId);
}

void BotSpawnerAdapter::SetMaxBots(uint32 maxBots)
{
    _maxBots = maxBots;
    if (_orchestrator)
        _orchestrator->SetMaxBots(maxBots);
}

void BotSpawnerAdapter::SetBotToPlayerRatio(float ratio)
{
    _botToPlayerRatio = ratio;
    if (_orchestrator)
        _orchestrator->SetBotToPlayerRatio(ratio);
}

bool BotSpawnerAdapter::IsEnabled() const
{
    return _enabled;
}

void BotSpawnerAdapter::SetEnabled(bool enabled)
{
    _enabled = enabled;
    TC_LOG_INFO("module.playerbot.adapter",
        "BotSpawnerAdapter: Adapter enabled state changed to: {}", enabled);
}

void BotSpawnerAdapter::ResetAdapterStats()
{
    _stats = AdapterStats{};
    TC_LOG_DEBUG("module.playerbot.adapter",
        "BotSpawnerAdapter: Performance statistics reset");
}

bool BotSpawnerAdapter::InitializeOrchestrator()
{
    try
    {
        _orchestrator = ::std::make_unique<BotSpawnOrchestrator>();
        return _orchestrator->Initialize();
    }
    catch (::std::exception const& ex)
    {
        TC_LOG_ERROR("module.playerbot.adapter",
            "BotSpawnerAdapter: Exception during orchestrator initialization: {}", ex.what());
        return false;
    }
}

void BotSpawnerAdapter::ConfigureOrchestrator()
{
    if (!_orchestrator)
        return;

    _orchestrator->SetMaxBots(_maxBots);
    _orchestrator->SetBotToPlayerRatio(_botToPlayerRatio);

    TC_LOG_DEBUG("module.playerbot.adapter",
        "BotSpawnerAdapter: Orchestrator configured with maxBots={}, ratio={}",
        _maxBots, _botToPlayerRatio);
}

void BotSpawnerAdapter::RecordApiCall(uint64 durationMicroseconds)
{
    // Simple moving average for performance tracking
    if (_stats.avgCallDurationUs == 0)
        _stats.avgCallDurationUs = durationMicroseconds;
    else
        _stats.avgCallDurationUs = (_stats.avgCallDurationUs + durationMicroseconds) / 2;
}

// === CONFIGURATION METHODS ===

void BotSpawnerAdapter::LoadConfig()
{
    // Load configuration from database or config file
    // For now, use defaults from member variables
    TC_LOG_DEBUG("module.playerbot.adapter", "BotSpawnerAdapter: Configuration loaded");
}

SpawnConfig const& BotSpawnerAdapter::GetConfig() const
{
    return _config;
}

void BotSpawnerAdapter::SetConfig(SpawnConfig const& config)
{
    _config = config;
    TC_LOG_DEBUG("module.playerbot.adapter", "BotSpawnerAdapter: Configuration updated");
}

// === ZONE/MAP MANAGEMENT METHODS ===
// These delegate to BotSpawner singleton which has full implementations

void BotSpawnerAdapter::DespawnAllBots()
{
    if (!_enabled)
        return;

    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
    {
        spawner->DespawnAllBots();
        TC_LOG_INFO("module.playerbot.adapter", "BotSpawnerAdapter::DespawnAllBots() - Delegated to BotSpawner");
    }
}

void BotSpawnerAdapter::UpdateZonePopulation(uint32 zoneId, uint32 mapId)
{
    if (!_enabled)
        return;

    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        spawner->UpdateZonePopulation(zoneId, mapId);
}

void BotSpawnerAdapter::UpdateZonePopulationSafe(uint32 zoneId, uint32 mapId)
{
    if (!_enabled)
        return;

    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        spawner->UpdateZonePopulationSafe(zoneId, mapId);
}

ZonePopulation BotSpawnerAdapter::GetZonePopulation(uint32 zoneId) const
{
    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        return spawner->GetZonePopulation(zoneId);
    return ZonePopulation{};
}

::std::vector<ZonePopulation> BotSpawnerAdapter::GetAllZonePopulations() const
{
    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        return spawner->GetAllZonePopulations();
    return {};
}

// === BOT QUERY METHODS ===
// These delegate to BotSpawner singleton which has full implementations

bool BotSpawnerAdapter::IsBotActive(ObjectGuid guid) const
{
    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        return spawner->IsBotActive(guid);
    return false;
}

uint32 BotSpawnerAdapter::GetActiveBotCount(uint32 mapId, bool useMapId) const
{
    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        return spawner->GetActiveBotCount(mapId, useMapId);
    return 0;
}

::std::vector<ObjectGuid> BotSpawnerAdapter::GetActiveBotsInZone(uint32 zoneId) const
{
    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        return spawner->GetActiveBotsInZone(zoneId);
    return {};
}

bool BotSpawnerAdapter::CanSpawnOnMap(uint32 mapId) const
{
    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        return spawner->CanSpawnOnMap(mapId);
    return false;
}

// === ADVANCED SPAWNING METHODS ===
// These delegate to BotSpawner singleton which has full implementations

bool BotSpawnerAdapter::CreateAndSpawnBot(uint32 masterAccountId, uint8 classId, uint8 race, uint8 gender, ::std::string const& name, ObjectGuid& outCharacterGuid)
{
    if (!_enabled)
        return false;

    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
    {
        bool result = spawner->CreateAndSpawnBot(masterAccountId, classId, race, gender, name, outCharacterGuid);
        if (result)
        {
            TC_LOG_INFO("module.playerbot.adapter",
                "BotSpawnerAdapter::CreateAndSpawnBot() - Successfully created bot '{}' (GUID: {})",
                name, outCharacterGuid.ToString());
        }
        return result;
    }
    return false;
}

// === STATISTICS METHODS ===
// These delegate to BotSpawner singleton which has full implementations

SpawnStats const& BotSpawnerAdapter::GetStats() const
{
    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
    {
        // Sync from BotSpawner's stats
        _spawnStats = spawner->GetStats();
    }
    return _spawnStats;
}

void BotSpawnerAdapter::ResetStats()
{
    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        spawner->ResetStats();

    // Also reset adapter's local copy
    _spawnStats = SpawnStats{};
    TC_LOG_DEBUG("module.playerbot.adapter", "BotSpawnerAdapter::ResetStats() - Statistics reset");
}

// === PLAYER INTERACTION METHODS ===
// These delegate to BotSpawner singleton which has full implementations

void BotSpawnerAdapter::OnPlayerLogin()
{
    if (!_enabled)
        return;

    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
    {
        spawner->OnPlayerLogin();
        TC_LOG_DEBUG("module.playerbot.adapter", "BotSpawnerAdapter::OnPlayerLogin() - Delegated to BotSpawner");
    }
}

void BotSpawnerAdapter::CheckAndSpawnForPlayers()
{
    if (!_enabled)
        return;

    BotSpawner* spawner = BotSpawner::instance();
    if (spawner)
        spawner->CheckAndSpawnForPlayers();
}

// =====================================================
// LegacyBotSpawnerAdapter Implementation
// =====================================================

LegacyBotSpawnerAdapter::LegacyBotSpawnerAdapter()
{
    TC_LOG_DEBUG("module.playerbot.adapter",
        "LegacyBotSpawnerAdapter: Creating adapter for legacy spawning");
}

LegacyBotSpawnerAdapter::~LegacyBotSpawnerAdapter()
{
    Shutdown();
}

bool LegacyBotSpawnerAdapter::Initialize()
{
    TC_LOG_INFO("module.playerbot.adapter",
        "LegacyBotSpawnerAdapter: Initializing legacy adapter");

    try
    {
        // Use singleton instance instead of creating new instance
        _legacySpawner = BotSpawner::instance();
        if (!_legacySpawner)
        {
            TC_LOG_ERROR("module.playerbot.adapter",
                "LegacyBotSpawnerAdapter: Failed to get BotSpawner singleton instance");
            return false;
        }
        return _legacySpawner->Initialize();
    }
    catch (::std::exception const& ex)
    {
        TC_LOG_ERROR("module.playerbot.adapter",
            "LegacyBotSpawnerAdapter: Exception during initialization: {}", ex.what());
        return false;
    }
}

void LegacyBotSpawnerAdapter::Shutdown()
{
    if (_legacySpawner)
    {
        _legacySpawner->Shutdown();
        _legacySpawner = nullptr; // Clear reference to singleton
    }
}

void LegacyBotSpawnerAdapter::Update(uint32 diff)
{
    if (_legacySpawner)
        _legacySpawner->Update(diff);
}

bool LegacyBotSpawnerAdapter::SpawnBot(SpawnRequest const& request)
{
    if (!_legacySpawner)
        return false;

    return _legacySpawner->SpawnBot(request);
}

uint32 LegacyBotSpawnerAdapter::SpawnBots(::std::vector<SpawnRequest> const& requests)
{
    if (!_legacySpawner)
        return 0;

    return _legacySpawner->SpawnBots(requests);
}

void LegacyBotSpawnerAdapter::SpawnToPopulationTarget()
{
    if (_legacySpawner)
        _legacySpawner->SpawnToPopulationTarget();
}

void LegacyBotSpawnerAdapter::UpdatePopulationTargets()
{
    if (_legacySpawner)
        _legacySpawner->UpdatePopulationTargets();
}

bool LegacyBotSpawnerAdapter::DespawnBot(ObjectGuid guid, ::std::string const& reason)
{
    if (!_legacySpawner)
        return false;

    return _legacySpawner->DespawnBot(guid, reason);
}

void LegacyBotSpawnerAdapter::DespawnBot(ObjectGuid guid, bool forced)
{
    if (_legacySpawner)
        _legacySpawner->DespawnBot(guid, forced);
}

uint32 LegacyBotSpawnerAdapter::GetActiveBotCount() const
{
    return _legacySpawner ? _legacySpawner->GetActiveBotCount() : 0;
}

uint32 LegacyBotSpawnerAdapter::GetActiveBotCount(uint32 zoneId) const
{
    return _legacySpawner ? _legacySpawner->GetActiveBotCount(zoneId) : 0;
}

bool LegacyBotSpawnerAdapter::CanSpawnMore() const
{
    return _legacySpawner ? _legacySpawner->CanSpawnMore() : false;
}

bool LegacyBotSpawnerAdapter::CanSpawnInZone(uint32 zoneId) const
{
    return _legacySpawner ? _legacySpawner->CanSpawnInZone(zoneId) : false;
}

void LegacyBotSpawnerAdapter::SetMaxBots(uint32 maxBots)
{
    if (_legacySpawner)
        _legacySpawner->SetMaxBots(maxBots);
}

void LegacyBotSpawnerAdapter::SetBotToPlayerRatio(float ratio)
{
    if (_legacySpawner)
        _legacySpawner->SetBotToPlayerRatio(ratio);
}

bool LegacyBotSpawnerAdapter::IsEnabled() const
{
    return _legacySpawner ? _legacySpawner->IsEnabled() : false;
}

void LegacyBotSpawnerAdapter::SetEnabled(bool enabled)
{
    if (_legacySpawner)
        _legacySpawner->SetEnabled(enabled);
}

// === CONFIGURATION METHODS ===

void LegacyBotSpawnerAdapter::LoadConfig()
{
    if (_legacySpawner)
        _legacySpawner->LoadConfig();
}

SpawnConfig const& LegacyBotSpawnerAdapter::GetConfig() const
{
    if (_legacySpawner)
        _config = _legacySpawner->GetConfig();
    return _config;
}

void LegacyBotSpawnerAdapter::SetConfig(SpawnConfig const& config)
{
    _config = config;
    if (_legacySpawner)
        _legacySpawner->SetConfig(config);
}

// === ZONE/MAP MANAGEMENT METHODS ===

void LegacyBotSpawnerAdapter::DespawnAllBots()
{
    if (_legacySpawner)
        _legacySpawner->DespawnAllBots();
}

void LegacyBotSpawnerAdapter::UpdateZonePopulation(uint32 zoneId, uint32 mapId)
{
    if (_legacySpawner)
        _legacySpawner->UpdateZonePopulation(zoneId, mapId);
}

void LegacyBotSpawnerAdapter::UpdateZonePopulationSafe(uint32 zoneId, uint32 mapId)
{
    if (_legacySpawner)
        _legacySpawner->UpdateZonePopulationSafe(zoneId, mapId);
}

ZonePopulation LegacyBotSpawnerAdapter::GetZonePopulation(uint32 zoneId) const
{
    return _legacySpawner ? _legacySpawner->GetZonePopulation(zoneId) : ZonePopulation{};
}

::std::vector<ZonePopulation> LegacyBotSpawnerAdapter::GetAllZonePopulations() const
{
    return _legacySpawner ? _legacySpawner->GetAllZonePopulations() : ::std::vector<ZonePopulation>{};
}

// === BOT QUERY METHODS ===

bool LegacyBotSpawnerAdapter::IsBotActive(ObjectGuid guid) const
{
    return _legacySpawner ? _legacySpawner->IsBotActive(guid) : false;
}

uint32 LegacyBotSpawnerAdapter::GetActiveBotCount(uint32 mapId, bool useMapId) const
{
    return _legacySpawner ? _legacySpawner->GetActiveBotCount(mapId, useMapId) : 0;
}

::std::vector<ObjectGuid> LegacyBotSpawnerAdapter::GetActiveBotsInZone(uint32 zoneId) const
{
    return _legacySpawner ? _legacySpawner->GetActiveBotsInZone(zoneId) : ::std::vector<ObjectGuid>{};
}

bool LegacyBotSpawnerAdapter::CanSpawnOnMap(uint32 mapId) const
{
    return _legacySpawner ? _legacySpawner->CanSpawnOnMap(mapId) : false;
}

// === ADVANCED SPAWNING METHODS ===

bool LegacyBotSpawnerAdapter::CreateAndSpawnBot(uint32 masterAccountId, uint8 classId, uint8 race, uint8 gender, ::std::string const& name, ObjectGuid& outCharacterGuid)
{
    return _legacySpawner ? _legacySpawner->CreateAndSpawnBot(masterAccountId, classId, race, gender, name, outCharacterGuid) : false;
}

// === STATISTICS METHODS ===

SpawnStats const& LegacyBotSpawnerAdapter::GetStats() const
{
    if (_legacySpawner)
        _stats = _legacySpawner->GetStats();
    return _stats;
}

void LegacyBotSpawnerAdapter::ResetStats()
{
    if (_legacySpawner)
        _legacySpawner->ResetStats();
    _stats = SpawnStats{};
}

// === PLAYER INTERACTION METHODS ===

void LegacyBotSpawnerAdapter::OnPlayerLogin()
{
    if (_legacySpawner)
        _legacySpawner->OnPlayerLogin();
}

void LegacyBotSpawnerAdapter::CheckAndSpawnForPlayers()
{
    if (_legacySpawner)
        _legacySpawner->CheckAndSpawnForPlayers();
}

// =====================================================
// BotSpawnerFactory Implementation
// =====================================================

::std::unique_ptr<BotSpawnerAdapter> BotSpawnerFactory::CreateSpawner(SpawnerType type)
{
    if (type == SpawnerType::AUTO)
        type = DetectBestSpawnerType();

    TC_LOG_INFO("module.playerbot.factory",
        "BotSpawnerFactory: Creating spawner of type: {}", GetSpawnerTypeName(type));

    // Note: After DI cleanup, factory always returns BotSpawnerAdapter
    // Legacy mode is handled internally by the adapter
    return ::std::make_unique<BotSpawnerAdapter>();
}

BotSpawnerFactory::SpawnerType BotSpawnerFactory::DetectBestSpawnerType()
{
    // Check if orchestrator is available and should be used
    if (IsOrchestratorAvailable() && !ShouldUseLegacySpawner())
    {
        TC_LOG_DEBUG("module.playerbot.factory",
            "BotSpawnerFactory: Auto-detected ORCHESTRATED spawner");
        return SpawnerType::ORCHESTRATED;
    }

    TC_LOG_DEBUG("module.playerbot.factory",
        "BotSpawnerFactory: Auto-detected LEGACY spawner");
    return SpawnerType::LEGACY;
}

::std::string BotSpawnerFactory::GetSpawnerTypeName(SpawnerType type)
{
    switch (type)
    {
        case SpawnerType::ORCHESTRATED: return "ORCHESTRATED";
        case SpawnerType::LEGACY: return "LEGACY";
        case SpawnerType::AUTO: return "AUTO";
        default: return "UNKNOWN";
    }
}

bool BotSpawnerFactory::IsOrchestratorAvailable()
{
    // Check if all required components for orchestrator are available
    return sPlayerbotConfig->GetBool("Playerbot.UseOrchestrator", true);
}

bool BotSpawnerFactory::ShouldUseLegacySpawner()
{
    // Check configuration flag to force legacy mode
    return sPlayerbotConfig->GetBool("Playerbot.ForceLegacyMode", false);
}

} // namespace Playerbot