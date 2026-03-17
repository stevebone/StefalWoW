/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "StartupSpawnOrchestrator.h"
#include "Config/PlayerbotConfig.h"
#include "Log.h"
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// StartupOrchestratorConfig Implementation
// ============================================================================

void StartupOrchestratorConfig::LoadFromConfig()
{
    enablePhasedStartup = sPlayerbotConfig->GetBool("Playerbot.Startup.EnablePhased", true);
    enableParallelLoading = sPlayerbotConfig->GetBool("Playerbot.Startup.EnableParallelLoading", false);
    maxConcurrentDbLoads = sPlayerbotConfig->GetInt("Playerbot.Startup.MaxConcurrentDbLoads", 10);
    initialDelaySeconds = sPlayerbotConfig->GetInt("Playerbot.Startup.InitialDelaySeconds", 5);

    // Initialize default phases (can be overridden by config)
    InitializeDefaultPhases();

    TC_LOG_INFO("module.playerbot.orchestrator",
        "StartupSpawnOrchestrator config loaded: Phased={}, ParallelLoading={}, InitialDelay={}s, Phases={}",
        enablePhasedStartup, enableParallelLoading, initialDelaySeconds, phases.size());
}

void StartupOrchestratorConfig::InitializeDefaultPhases()
{
    phases.clear();

    // Phase 1: CRITICAL_BOTS (0-2 min)
    PhaseConfig phase1;
    phase1.phase = StartupPhase::CRITICAL_BOTS;
    phase1.targetPriority = SpawnPriority::CRITICAL;
    phase1.minDurationSeconds = 0;
    phase1.maxDurationSeconds = 120;  // 2 minutes max
    phase1.targetBotsToSpawn = sPlayerbotConfig->GetInt("Playerbot.Startup.Phase1.TargetBots", 100);
    TC_LOG_INFO("module.playerbot.orchestrator", "🔧 CONFIG DEBUG: Phase1.TargetBots = {} (from Playerbot.Startup.Phase1.TargetBots)", phase1.targetBotsToSpawn);
    phase1.spawnRateMultiplier = 1.5f;  // Hardcoded default (not in config)
    phase1.allowEarlyTransition = true;
    phases.push_back(phase1);

    // Phase 2: HIGH_PRIORITY (2-5 min)
    PhaseConfig phase2;
    phase2.phase = StartupPhase::HIGH_PRIORITY;
    phase2.targetPriority = SpawnPriority::HIGH;
    phase2.minDurationSeconds = 30;   // Min 30s in this phase
    phase2.maxDurationSeconds = 180;  // 3 minutes max
    phase2.targetBotsToSpawn = sPlayerbotConfig->GetInt("Playerbot.Startup.Phase2.TargetBots", 500);
    phase2.spawnRateMultiplier = 1.2f;  // Hardcoded default (not in config)
    phase2.allowEarlyTransition = true;
    phases.push_back(phase2);

    // Phase 3: NORMAL_BOTS (5-15 min)
    PhaseConfig phase3;
    phase3.phase = StartupPhase::NORMAL_BOTS;
    phase3.targetPriority = SpawnPriority::NORMAL;
    phase3.minDurationSeconds = 60;   // Min 1 min in this phase
    phase3.maxDurationSeconds = 600;  // 10 minutes max
    phase3.targetBotsToSpawn = sPlayerbotConfig->GetInt("Playerbot.Startup.Phase3.TargetBots", 3000);
    phase3.spawnRateMultiplier = 1.0f;  // Hardcoded default (not in config)
    phase3.allowEarlyTransition = true;
    phases.push_back(phase3);

    // Phase 4: LOW_PRIORITY (15-30 min)
    PhaseConfig phase4;
    phase4.phase = StartupPhase::LOW_PRIORITY;
    phase4.targetPriority = SpawnPriority::LOW;
    phase4.minDurationSeconds = 60;   // Min 1 min in this phase
    phase4.maxDurationSeconds = 900;  // 15 minutes max
    phase4.targetBotsToSpawn = sPlayerbotConfig->GetInt("Playerbot.Startup.Phase4.TargetBots", 1400);
    phase4.spawnRateMultiplier = 0.8f;  // Hardcoded default (not in config)
    phase4.allowEarlyTransition = true;
    phases.push_back(phase4);
}

// ============================================================================
// StartupSpawnOrchestrator Implementation
// ============================================================================

bool StartupSpawnOrchestrator::Initialize(SpawnPriorityQueue* priorityQueue, AdaptiveSpawnThrottler* throttler)
{
    if (_initialized)
        return true;

    if (!priorityQueue || !throttler)
    {
        TC_LOG_ERROR("module.playerbot.orchestrator",
            "StartupSpawnOrchestrator::Initialize() called with null dependencies");
        return false;
    }

    TC_LOG_INFO("module.playerbot.orchestrator", "Initializing StartupSpawnOrchestrator...");

    // Store dependencies
    _priorityQueue = priorityQueue;
    _throttler = throttler;

    // Load configuration
    _config.LoadFromConfig();

    // Initialize state
    _currentPhase = StartupPhase::IDLE;
    _botsSpawnedThisPhase = 0;
    _botsSpawnedTotal = 0;
    _startupBegun = false;

    _initialized = true;
    TC_LOG_INFO("module.playerbot.orchestrator", " StartupSpawnOrchestrator initialized successfully");
    return true;
}

void StartupSpawnOrchestrator::Update(uint32 /*diff*/)
{
    if (!_initialized || !_startupBegun)
        return;

    // Check if should transition to next phase
    if (ShouldTransitionPhase())
    {
        // Determine next phase
        StartupPhase nextPhase = StartupPhase::COMPLETED;

        switch (_currentPhase)
        {
            case StartupPhase::IDLE:
                nextPhase = StartupPhase::CRITICAL_BOTS;
                break;
            case StartupPhase::CRITICAL_BOTS:
                nextPhase = StartupPhase::HIGH_PRIORITY;
                break;
            case StartupPhase::HIGH_PRIORITY:
                nextPhase = StartupPhase::NORMAL_BOTS;
                break;
            case StartupPhase::NORMAL_BOTS:
                nextPhase = StartupPhase::LOW_PRIORITY;
                break;
            case StartupPhase::LOW_PRIORITY:
                nextPhase = StartupPhase::COMPLETED;
                break;
            case StartupPhase::COMPLETED:
                return;  // Already complete
        }

        TransitionToPhase(nextPhase);
    }
}

void StartupSpawnOrchestrator::BeginStartup()
{
    if (_startupBegun)
    {
        TC_LOG_WARN("module.playerbot.orchestrator", "BeginStartup() called but startup already begun");
        return;
    }

    TC_LOG_INFO("module.playerbot.orchestrator",
        " Beginning phased startup sequence (initial delay: {}s)",
        _config.initialDelaySeconds);

    _startupBeginTime = GameTime::Now();
    _startupBegun = true;

    // Start with initial delay, then transition to CRITICAL_BOTS
    if (_config.initialDelaySeconds > 0)
    {
        // DESIGN: Initial delay is intentionally skipped — transitioning immediately to CRITICAL_BOTS.
        // A delay timer could be added here if server maps need extra init time, but in practice
        // the ModuleManager callback timing already ensures the world is fully loaded.
    }

    TransitionToPhase(StartupPhase::CRITICAL_BOTS);
}

bool StartupSpawnOrchestrator::ShouldSpawnNext() const
{
    if (!_initialized || !_startupBegun)
        return false;

    // ========================================================================
    // CRITICAL FIX: Allow runtime spawning after startup completes
    // ========================================================================
    // Previously, this returned false when COMPLETED, which blocked ALL spawning
    // including runtime requests (BG queue, party invites, etc.).
    // Now we allow spawning in COMPLETED phase if there are items in the queue.
    // ========================================================================
    if (_currentPhase == StartupPhase::COMPLETED)
    {
        // Allow runtime spawning - just check throttler and queue
        if (_throttler && !_throttler->CanSpawnNow())
            return false;

        // In COMPLETED phase, allow any queued requests to spawn
        if (_priorityQueue && !_priorityQueue->IsEmpty())
        {
            TC_LOG_TRACE("module.playerbot.orchestrator",
                "Runtime spawn allowed in COMPLETED phase (queue has {} items)",
                _priorityQueue->GetTotalQueueSize());
            return true;
        }
        return false;
    }

    // Check if throttler allows spawning
    if (_throttler && !_throttler->CanSpawnNow())
        return false;

    // Get current phase config
    const PhaseConfig* phaseConfig = GetCurrentPhaseConfig();
    if (!phaseConfig)
        return false;

    // Check if queue has requests for current priority
    if (_priorityQueue)
    {
        // Check if any requests exist at current priority
        size_t requestsAtPriority = _priorityQueue->GetQueueSize(phaseConfig->targetPriority);
        if (requestsAtPriority == 0)
        {
            TC_LOG_TRACE("module.playerbot.orchestrator",
                "No more requests at priority {} in phase {}",
                GetSpawnPriorityName(phaseConfig->targetPriority),
                GetStartupPhaseName(_currentPhase));
            return false;
        }
    }

    return true;
}

void StartupSpawnOrchestrator::OnBotSpawned()
{
    if (!_initialized)
        return;

    ++_botsSpawnedThisPhase;
    ++_botsSpawnedTotal;

    TC_LOG_TRACE("module.playerbot.orchestrator",
        "Bot spawned in {} phase ({} this phase, {} total)",
        GetStartupPhaseName(_currentPhase),
        _botsSpawnedThisPhase,
        _botsSpawnedTotal);
}

OrchestratorMetrics StartupSpawnOrchestrator::GetMetrics() const
{
    OrchestratorMetrics metrics;

    metrics.currentPhase = _currentPhase;
    metrics.botsSpawnedThisPhase = _botsSpawnedThisPhase;
    metrics.botsSpawnedTotal = _botsSpawnedTotal;
    metrics.isStartupComplete = (_currentPhase == StartupPhase::COMPLETED);

    if (_priorityQueue)
        metrics.botsRemainingInQueue = static_cast<uint32>(_priorityQueue->GetTotalQueueSize());

    if (_startupBegun)
    {
        metrics.totalElapsedTime = ::std::chrono::duration_cast<Milliseconds>(
            GameTime::Now() - _startupBeginTime);

        if (_currentPhase != StartupPhase::IDLE && _currentPhase != StartupPhase::COMPLETED)
        {
            metrics.timeInCurrentPhase = ::std::chrono::duration_cast<Milliseconds>(
                GameTime::Now() - _phaseStartTime);
        }
    }

    // Calculate phase progress
    const PhaseConfig* phaseConfig = GetCurrentPhaseConfig();
    if (phaseConfig && phaseConfig->targetBotsToSpawn > 0)
    {
        metrics.currentPhaseProgress = ::std::min(1.0f,
            static_cast<float>(_botsSpawnedThisPhase) / phaseConfig->targetBotsToSpawn);
    }

    metrics.overallProgress = CalculateOverallProgress();

    return metrics;
}

void StartupSpawnOrchestrator::ForceNextPhase()
{
    TC_LOG_WARN("module.playerbot.orchestrator",
        "ForceNextPhase() called - manually transitioning from {}",
        GetStartupPhaseName(_currentPhase));

    StartupPhase nextPhase = StartupPhase::COMPLETED;

    switch (_currentPhase)
    {
        case StartupPhase::IDLE:
            nextPhase = StartupPhase::CRITICAL_BOTS;
            break;
        case StartupPhase::CRITICAL_BOTS:
            nextPhase = StartupPhase::HIGH_PRIORITY;
            break;
        case StartupPhase::HIGH_PRIORITY:
            nextPhase = StartupPhase::NORMAL_BOTS;
            break;
        case StartupPhase::NORMAL_BOTS:
            nextPhase = StartupPhase::LOW_PRIORITY;
            break;
        case StartupPhase::LOW_PRIORITY:
        case StartupPhase::COMPLETED:
            nextPhase = StartupPhase::COMPLETED;
            break;
    }

    TransitionToPhase(nextPhase);
}

void StartupSpawnOrchestrator::AbortStartup()
{
    TC_LOG_WARN("module.playerbot.orchestrator",
        "AbortStartup() called - aborting startup from phase {}",
        GetStartupPhaseName(_currentPhase));

    TransitionToPhase(StartupPhase::COMPLETED);
}

void StartupSpawnOrchestrator::TransitionToPhase(StartupPhase newPhase)
{
    if (newPhase == _currentPhase)
        return;

    StartupPhase oldPhase = _currentPhase;
    _currentPhase = newPhase;
    _phaseStartTime = GameTime::Now();
    _botsSpawnedThisPhase = 0;

    const char* emoji = "";
    if (newPhase == StartupPhase::COMPLETED)
        emoji = "";
    else if (newPhase == StartupPhase::CRITICAL_BOTS)
        emoji = "";
    else if (newPhase == StartupPhase::HIGH_PRIORITY)
        emoji = "";
    else if (newPhase == StartupPhase::NORMAL_BOTS)
        emoji = "";
    else if (newPhase == StartupPhase::LOW_PRIORITY)
        emoji = "";

    TC_LOG_INFO("module.playerbot.orchestrator",
        "{} Startup phase transition: {} → {} (bots spawned: {})",
        emoji,
        GetStartupPhaseName(oldPhase),
        GetStartupPhaseName(newPhase),
        _botsSpawnedTotal);
}

bool StartupSpawnOrchestrator::ShouldTransitionPhase() const
{
    if (_currentPhase == StartupPhase::IDLE || _currentPhase == StartupPhase::COMPLETED)
        return false;

    const PhaseConfig* phaseConfig = GetCurrentPhaseConfig();
    if (!phaseConfig)
        return true;  // Invalid config, transition anyway

    Milliseconds timeInPhase = ::std::chrono::duration_cast<Milliseconds>(
        GameTime::Now() - _phaseStartTime);

    // Must meet minimum duration
    if (timeInPhase < Milliseconds(phaseConfig->minDurationSeconds * 1000))
        return false;

    // Check transition conditions
    bool targetReached = (_botsSpawnedThisPhase >= phaseConfig->targetBotsToSpawn);
    bool maxDurationReached = (timeInPhase >= Milliseconds(phaseConfig->maxDurationSeconds * 1000));
    bool noPendingRequests = false;

    if (_priorityQueue)
    {
        size_t requestsAtPriority = _priorityQueue->GetQueueSize(phaseConfig->targetPriority);
        noPendingRequests = (requestsAtPriority == 0);
    }

    // Transition if:
    // - Max duration reached, OR
    // - (Target reached OR no pending requests) AND early transition allowed
    return maxDurationReached ||
           ((targetReached || noPendingRequests) && phaseConfig->allowEarlyTransition);
}

const PhaseConfig* StartupSpawnOrchestrator::GetCurrentPhaseConfig() const
{
    for (const PhaseConfig& phase : _config.phases)
    {
        if (phase.phase == _currentPhase)
            return &phase;
    }

    return nullptr;
}

float StartupSpawnOrchestrator::CalculateOverallProgress() const
{
    if (_currentPhase == StartupPhase::IDLE)
        return 0.0f;

    if (_currentPhase == StartupPhase::COMPLETED)
        return 1.0f;

    // Calculate total target bots across all phases
    uint32 totalTargetBots = 0;
    for (const PhaseConfig& phase : _config.phases)
        totalTargetBots += phase.targetBotsToSpawn;

    if (totalTargetBots == 0)
        return 0.0f;

    // Calculate progress as: botsSpawned / totalTarget
    return ::std::min(1.0f, static_cast<float>(_botsSpawnedTotal) / totalTargetBots);
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* GetStartupPhaseName(StartupPhase phase)
{
    switch (phase)
    {
        case StartupPhase::IDLE:
            return "IDLE";
        case StartupPhase::CRITICAL_BOTS:
            return "CRITICAL_BOTS";
        case StartupPhase::HIGH_PRIORITY:
            return "HIGH_PRIORITY";
        case StartupPhase::NORMAL_BOTS:
            return "NORMAL_BOTS";
        case StartupPhase::LOW_PRIORITY:
            return "LOW_PRIORITY";
        case StartupPhase::COMPLETED:
            return "COMPLETED";
        default:
            return "UNKNOWN";
    }
}

} // namespace Playerbot
