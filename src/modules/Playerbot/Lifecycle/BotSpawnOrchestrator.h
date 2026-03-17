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
#include "ObjectGuid.h"
#include "Lifecycle/SpawnRequest.h"
#include <atomic>
#include <memory>
#include <functional>

namespace Playerbot
{

// Forward declarations
class BotResourcePool;
class BotPerformanceMonitor;
class BotPopulationManager;
class BotCharacterSelector;
class BotSessionFactory;

/**
 * @class BotSpawnOrchestrator
 * @brief Coordinates bot spawning across specialized components
 *
 * ARCHITECTURE REFACTORING: Replaces the monolithic BotSpawner God Class
 * with a clean orchestrator pattern that delegates to focused components.
 *
 * Responsibilities:
 * - Orchestrates the spawn workflow across components
 * - Maintains the public API for backward compatibility
 * - Handles error recovery and fallback strategies
 * - Provides unified logging and monitoring integration
 *
 * Components:
 * - BotResourcePool: Session and resource management
 * - BotPopulationManager: Zone population tracking
 * - BotCharacterSelector: Async character selection logic
 * - BotSessionFactory: Session creation and configuration
 * - BotPerformanceMonitor: Real-time performance tracking
 */
class TC_GAME_API BotSpawnOrchestrator
{
public:
    BotSpawnOrchestrator() = default;
    ~BotSpawnOrchestrator() = default;

    // Lifecycle
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // Main spawning interface (maintains BotSpawner API compatibility)
    bool SpawnBot(SpawnRequest const& request);
    uint32 SpawnBots(::std::vector<SpawnRequest> const& requests);

    // Population management
    void SpawnToPopulationTarget();
    void UpdatePopulationTargets();
    void DespawnBot(ObjectGuid guid, bool forced = false);
    bool DespawnBot(ObjectGuid guid, ::std::string const& reason);

    // Information queries
    uint32 GetActiveBotCount() const;
    uint32 GetActiveBotCount(uint32 zoneId) const;
    bool CanSpawnMore() const;
    bool CanSpawnInZone(uint32 zoneId) const;

    // Configuration
    void SetMaxBots(uint32 maxBots);
    void SetBotToPlayerRatio(float ratio);

    // Component access for advanced usage
    BotResourcePool* GetResourcePool() const { return _resourcePool.get(); }
    BotPerformanceMonitor* GetPerformanceMonitor() const { return _performanceMonitor.get(); }
    BotPopulationManager* GetPopulationManager() const { return _populationManager.get(); }

private:
    // Async spawn workflow
    void ProcessSpawnRequest(SpawnRequest const& request);
    void OnCharacterSelected(ObjectGuid characterGuid, SpawnRequest const& request);
    void OnSessionCreated(::std::shared_ptr<class BotSession> session, SpawnRequest const& request);
    void OnSpawnCompleted(bool success, ObjectGuid guid, SpawnRequest const& request);

    // Error handling and recovery
    void HandleSpawnFailure(SpawnRequest const& request, ::std::string const& reason);
    bool AttemptSpawnRecovery(SpawnRequest const& request);

    // Component instances
    ::std::unique_ptr<BotResourcePool> _resourcePool;
    ::std::unique_ptr<BotPerformanceMonitor> _performanceMonitor;
    ::std::unique_ptr<BotPopulationManager> _populationManager;
    ::std::unique_ptr<BotCharacterSelector> _characterSelector;
    ::std::unique_ptr<BotSessionFactory> _sessionFactory;

    // State tracking
    ::std::atomic<bool> _enabled{true};
    ::std::atomic<uint32> _activeSpawnRequests{0};

    // Non-copyable
    BotSpawnOrchestrator(BotSpawnOrchestrator const&) = delete;
    BotSpawnOrchestrator& operator=(BotSpawnOrchestrator const&) = delete;
};

} // namespace Playerbot