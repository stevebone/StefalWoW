/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotSpawnOrchestrator.h"
#include "BotResourcePool.h"
#include "BotPerformanceMonitor.h"
#include "BotPopulationManager.h"
#include "BotCharacterSelector.h"
#include "BotSessionFactory.h"
#include "BotSpawnEventBus.h"
#include "BotSpawner.h"
#include "Session/BotSession.h"
#include "../AI/BotAI.h"  // P1 FIX: Required for unique_ptr<BotAI> in BotSession.h
#include "Logging/Log.h"
#include "Player.h"

namespace Playerbot
{

bool BotSpawnOrchestrator::Initialize()
{
    TC_LOG_INFO("module.playerbot.orchestrator",
        "Initializing BotSpawnOrchestrator with component-based architecture");

    // Initialize all components
    _resourcePool = ::std::make_unique<BotResourcePool>();
    if (!_resourcePool->Initialize())
    {
        TC_LOG_ERROR("module.playerbot.orchestrator", "Failed to initialize BotResourcePool");
        return false;
    }

    _performanceMonitor = ::std::unique_ptr<BotPerformanceMonitor>(BotPerformanceMonitor::instance());
    if (!_performanceMonitor->Initialize())
    {
        TC_LOG_ERROR("module.playerbot.orchestrator", "Failed to initialize BotPerformanceMonitor");
        return false;
    }

    _populationManager = ::std::make_unique<BotPopulationManager>();
    if (!_populationManager->Initialize())
    {
        TC_LOG_ERROR("module.playerbot.orchestrator", "Failed to initialize BotPopulationManager");
        return false;
    }

    _characterSelector = ::std::make_unique<BotCharacterSelector>();
    if (!_characterSelector->Initialize())
    {
        TC_LOG_ERROR("module.playerbot.orchestrator", "Failed to initialize BotCharacterSelector");
        return false;
    }

    _sessionFactory = ::std::make_unique<BotSessionFactory>();
    if (!_sessionFactory->Initialize())
    {
        TC_LOG_ERROR("module.playerbot.orchestrator", "Failed to initialize BotSessionFactory");
        return false;
    }

    // Subscribe to spawn events for coordination
    sBotSpawnEventBus->Subscribe(BotSpawnEventType::CHARACTER_SELECTED,
        [this](::std::shared_ptr<BotSpawnEvent> event) {
            auto charEvent = ::std::static_pointer_cast<CharacterSelectedEvent>(event);
            OnCharacterSelected(charEvent->characterGuid, charEvent->originalRequest);
        });

    sBotSpawnEventBus->Subscribe(BotSpawnEventType::SESSION_CREATED,
        [this](::std::shared_ptr<BotSpawnEvent> event) {
            auto sessionEvent = ::std::static_pointer_cast<SessionCreatedEvent>(event);
            OnSessionCreated(sessionEvent->session, sessionEvent->originalRequest);
        });

    sBotSpawnEventBus->Subscribe(BotSpawnEventType::SPAWN_COMPLETED,
        [this](::std::shared_ptr<BotSpawnEvent> event) {
            auto completedEvent = ::std::static_pointer_cast<SpawnCompletedEvent>(event);
            // Handle spawn completion statistics and cleanup
        });

    _enabled.store(true);

    TC_LOG_INFO("module.playerbot.orchestrator", "BotSpawnOrchestrator initialized successfully");
    return true;
}

void BotSpawnOrchestrator::Shutdown()
{
    TC_LOG_INFO("module.playerbot.orchestrator", "Shutting down BotSpawnOrchestrator");

    _enabled.store(false);

    // Shutdown components in reverse order
    if (_sessionFactory)
        _sessionFactory->Shutdown();

    if (_characterSelector)
        _characterSelector->Shutdown();

    if (_populationManager)
        _populationManager->Shutdown();

    if (_performanceMonitor)
        _performanceMonitor->Shutdown();

    if (_resourcePool)
        _resourcePool->Shutdown();

    // Log final statistics
    TC_LOG_INFO("module.playerbot.orchestrator",
        "BotSpawnOrchestrator shutdown complete. Active spawn requests: {}",
        _activeSpawnRequests.load());
}

void BotSpawnOrchestrator::Update(uint32 diff)
{
    if (!_enabled.load())
        return;

    // Update all components
    if (_resourcePool)
        _resourcePool->Update(diff);

    if (_performanceMonitor)
        _performanceMonitor->Update(diff);

    if (_populationManager)
        _populationManager->Update(diff);

    // Check for population rebalancing needs
    if (_populationManager)
    {
        auto underpopulatedZones = _populationManager->GetUnderpopulatedZones();
        if (!underpopulatedZones.empty())
        {
            // Trigger population adjustment
            SpawnToPopulationTarget();
        }
    }
}

// === MAIN SPAWNING INTERFACE ===

bool BotSpawnOrchestrator::SpawnBot(SpawnRequest const& request)
{
    if (!_enabled.load())
        return false;

    // PERFORMANCE MONITORING: Start tracking this spawn operation
    auto timer = _performanceMonitor->CreateSpawnTimer();

    // Check resource availability
    if (!_resourcePool->CanAllocateSession())
    {
        TC_LOG_WARN("module.playerbot.orchestrator",
            "Cannot spawn bot - no available sessions in resource pool");
        return false;
    }

    // Check population limits
    if (!_populationManager->CanSpawnInZone(request.zoneId, request.maxBotsPerZone))
    {
        TC_LOG_DEBUG("module.playerbot.orchestrator",
            "Cannot spawn bot in zone {} - population limit reached", request.zoneId);
        return false;
    }

    // Start async spawn workflow via events
    _activeSpawnRequests.fetch_add(1);
    ProcessSpawnRequest(request);

    return true;
}

uint32 BotSpawnOrchestrator::SpawnBots(::std::vector<SpawnRequest> const& requests)
{
    if (!_enabled.load())
        return 0;

    uint32 successfulSpawns = 0;

    // BATCH OPTIMIZATION: Use batch character selection for multiple requests
    _characterSelector->ProcessBatchSelection(requests,
        [this, &successfulSpawns](::std::vector<ObjectGuid> selectedCharacters) {
            // Process each selected character
    for (size_t i = 0; i < selectedCharacters.size(); ++i)
            {
                if (!selectedCharacters[i].IsEmpty())
                {
                    // Continue spawn workflow for this character
                    successfulSpawns++;
                }
            }
        });

    return successfulSpawns;
}

void BotSpawnOrchestrator::SpawnToPopulationTarget()
{
    if (!_populationManager)
        return;

    auto underpopulatedZones = _populationManager->GetUnderpopulatedZones();

    for (uint32 zoneId : underpopulatedZones)
    {
        auto zonePopulation = _populationManager->GetZonePopulation(zoneId);
        uint32 botsNeeded = zonePopulation.targetBotCount - zonePopulation.botCount;

        // Create spawn requests for needed bots
    for (uint32 i = 0; i < botsNeeded && i < 10; ++i) // Limit to 10 per update cycle
        {
            SpawnRequest request;
            request.zoneId = zoneId;
            request.mapId = zonePopulation.mapId;
            request.minLevel = zonePopulation.minLevel;
            request.maxLevel = zonePopulation.maxLevel;

            SpawnBot(request);
        }
    }
}

void BotSpawnOrchestrator::UpdatePopulationTargets()
{
    if (_populationManager)
        _populationManager->CalculateZoneTargets();
}

bool BotSpawnOrchestrator::DespawnBot(ObjectGuid guid, ::std::string const& reason)
{
    if (!_enabled.load())
        return false;

    // Return session to resource pool
    if (_resourcePool)
        _resourcePool->ReturnSession(guid);

    // Update population tracking
    if (_populationManager)
    {
        // Find which zone this bot was in and update count
        // This would require additional tracking in population manager
    }

    TC_LOG_DEBUG("module.playerbot.orchestrator",
        "Despawned bot {} - Reason: {}", guid.ToString(), reason);

    return true;
}

void BotSpawnOrchestrator::DespawnBot(ObjectGuid guid, bool forced)
{
    ::std::string reason = forced ? "forced_shutdown" : "normal_despawn";
    DespawnBot(guid, reason);
}

// === INFORMATION QUERIES ===

uint32 BotSpawnOrchestrator::GetActiveBotCount() const
{
    return _populationManager ? _populationManager->GetTotalBotCount() : 0;
}

uint32 BotSpawnOrchestrator::GetActiveBotCount(uint32 zoneId) const
{
    return _populationManager ? _populationManager->GetBotCountInZone(zoneId) : 0;
}

bool BotSpawnOrchestrator::CanSpawnMore() const
{
    if (!_enabled.load())
        return false;

    return _resourcePool && _resourcePool->CanAllocateSession() &&
           _performanceMonitor && _performanceMonitor->IsPerformanceHealthy();
}

bool BotSpawnOrchestrator::CanSpawnInZone(uint32 zoneId) const
{
    if (!_populationManager)
        return false;

    return _populationManager->CanSpawnInZone(zoneId, 50); // Default limit
}

// === ASYNC SPAWN WORKFLOW ===

void BotSpawnOrchestrator::ProcessSpawnRequest(SpawnRequest const& request)
{
    // Publish spawn request event to start async workflow
    sBotSpawnEventBus->PublishSpawnRequest(request,
        [this, request](bool success, ObjectGuid characterGuid) {
            if (success && !characterGuid.IsEmpty())
            {
                // Character selection completed successfully
                // The event system will handle the next steps
            }
            else
            {
                // Handle spawn failure
                _activeSpawnRequests.fetch_sub(1);
                HandleSpawnFailure(request, "character_selection_failed");
            }
        });
}

void BotSpawnOrchestrator::OnCharacterSelected(ObjectGuid characterGuid, SpawnRequest const& request)
{
    // Start session creation phase
    auto session = _sessionFactory->CreateBotSession(characterGuid, request);
    if (session)
    {
        // Add session to resource pool
        _resourcePool->AddSession(session);

        // Publish session created event
        sBotSpawnEventBus->PublishSessionCreated(session, request);
    }
    else
    {
        HandleSpawnFailure(request, "session_creation_failed");
    }
}

void BotSpawnOrchestrator::OnSessionCreated(::std::shared_ptr<BotSession> session, SpawnRequest const& request)
{
    // Final spawn completion
    ObjectGuid botGuid = session->GetPlayer() ? session->GetPlayer()->GetGUID() : ObjectGuid::Empty;

    if (!botGuid.IsEmpty())
    {
        // Update population tracking
        _populationManager->AddBotToZone(request.zoneId, botGuid);

        // Publish spawn completed event
        sBotSpawnEventBus->PublishSpawnCompleted(botGuid, true, "spawn_successful");

        TC_LOG_DEBUG("module.playerbot.orchestrator",
            "Successfully spawned bot {} in zone {}", botGuid.ToString(), request.zoneId);
    }
    else
    {
        HandleSpawnFailure(request, "player_creation_failed");
    }

    _activeSpawnRequests.fetch_sub(1);
}

void BotSpawnOrchestrator::OnSpawnCompleted(bool success, ObjectGuid guid, SpawnRequest const& request)
{
    if (success)
    {
        TC_LOG_INFO("module.playerbot.orchestrator",
            "Bot spawn completed successfully: {}", guid.ToString());
    }
    else
    {
        HandleSpawnFailure(request, "spawn_workflow_failed");
    }
}

// === ERROR HANDLING ===

void BotSpawnOrchestrator::HandleSpawnFailure(SpawnRequest const& request, ::std::string const& reason)
{
    TC_LOG_WARN("module.playerbot.orchestrator",
        "Spawn failure for zone {} - Reason: {}", request.zoneId, reason);

    // Attempt recovery if appropriate
    if (reason == "resource_exhaustion" && _resourcePool)
    {
        // Trigger resource pool cleanup
        _resourcePool->CleanupIdleSessions();
    }

    // Publish spawn failed event
    sBotSpawnEventBus->PublishSpawnCompleted(ObjectGuid::Empty, false, reason);
}

bool BotSpawnOrchestrator::AttemptSpawnRecovery(SpawnRequest const& request)
{
    // Simple recovery strategies
    if (_resourcePool && _resourcePool->GetAvailableSessionCount() == 0)
    {
        // Try to free up resources
        _resourcePool->CleanupIdleSessions();
        return _resourcePool->CanAllocateSession();
    }

    return false;
}

// === CONFIGURATION ===

void BotSpawnOrchestrator::SetMaxBots(uint32 maxBots)
{
    if (_populationManager)
        _populationManager->SetMaxBots(maxBots);
}

void BotSpawnOrchestrator::SetBotToPlayerRatio(float ratio)
{
    if (_populationManager)
        _populationManager->SetBotToPlayerRatio(ratio);
}

} // namespace Playerbot