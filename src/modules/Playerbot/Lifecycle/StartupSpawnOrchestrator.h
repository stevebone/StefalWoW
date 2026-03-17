/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_STARTUP_SPAWN_ORCHESTRATOR_H
#define PLAYERBOT_STARTUP_SPAWN_ORCHESTRATOR_H

#include "Define.h"
#include "Duration.h"
#include "SpawnPriorityQueue.h"
#include "AdaptiveSpawnThrottler.h"
#include <vector>

namespace Playerbot
{
    /**
     * @brief Startup spawn phases for graduated bot spawning
     *
     * Phases are executed sequentially to prevent server overload
     * during initial bot population.
     */
    enum class StartupPhase : uint8
    {
        IDLE = 0,              ///< Not started or completed
        CRITICAL_BOTS = 1,     ///< Phase 1: Guild leaders, raid leaders (0-2 min)
        HIGH_PRIORITY = 2,     ///< Phase 2: Party members, friends (2-5 min)
        NORMAL_BOTS = 3,       ///< Phase 3: Standard bots (5-15 min)
        LOW_PRIORITY = 4,      ///< Phase 4: Background/filler bots (15-30 min)
        COMPLETED = 5          ///< All phases complete
    };

    /**
     * @brief Configuration for a single startup phase
     */
    struct TC_GAME_API PhaseConfig
    {
        StartupPhase phase = StartupPhase::IDLE;
        SpawnPriority targetPriority = SpawnPriority::NORMAL;

        uint32 minDurationSeconds = 0;        ///< Minimum time to spend in this phase
        uint32 maxDurationSeconds = 0;        ///< Maximum time to spend in this phase
        uint32 targetBotsToSpawn = 0;         ///< Target number of bots to spawn
        float spawnRateMultiplier = 1.0f;     ///< Spawn rate multiplier for this phase

        bool allowEarlyTransition = true;     ///< Can transition early if quota met
    };

    /**
     * @brief Configuration for startup spawn orchestration
     */
    struct TC_GAME_API StartupOrchestratorConfig
    {
        bool enablePhasedStartup = true;      ///< Master enable for phased startup
        bool enableParallelLoading = false;   ///< Allow database preloading during spawning

        uint32 maxConcurrentDbLoads = 10;     ///< Max concurrent character loads from DB
        uint32 initialDelaySeconds = 5;       ///< Delay before starting Phase 1

        ::std::vector<PhaseConfig> phases;      ///< Phase configurations

        /**
         * @brief Load configuration from playerbots.conf
         */
        void LoadFromConfig();

        /**
         * @brief Initialize default phase configurations
         */
        void InitializeDefaultPhases();
    };

    /**
     * @brief Orchestrator metrics for monitoring startup progress
     */
    struct TC_GAME_API OrchestratorMetrics
    {
        StartupPhase currentPhase = StartupPhase::IDLE;
        Milliseconds timeInCurrentPhase = Milliseconds(0);
        Milliseconds totalElapsedTime = Milliseconds(0);

        uint32 botsSpawnedThisPhase = 0;
        uint32 botsSpawnedTotal = 0;
        uint32 botsRemainingInQueue = 0;

        float currentPhaseProgress = 0.0f;     ///< Progress in current phase (0.0-1.0)
        float overallProgress = 0.0f;          ///< Overall startup progress (0.0-1.0)

        bool isStartupComplete = false;
    };

    /**
     * @brief Startup spawn orchestrator - Phased bot spawning manager
     *
     * Manages graduated bot spawning during server startup to prevent
     * resource spikes and database overload. Spawns bots in 4 priority-based
     * phases with adaptive timing and rate control.
     *
     * Startup Phases:
     * 1. CRITICAL_BOTS (0-2 min): Guild leaders, raid leaders
     *    - Spawns essential leadership bots first
     *    - Fast spawn rate (15-20 bots/sec)
     *    - Early world population
     *
     * 2. HIGH_PRIORITY (2-5 min): Party members, friends
     *    - Spawns social network bots
     *    - Medium spawn rate (10-15 bots/sec)
     *    - Builds group foundations
     *
     * 3. NORMAL_BOTS (5-15 min): Standard bots
     *    - Bulk bot population
     *    - Normal spawn rate (5-10 bots/sec)
     *    - Main world population
     *
     * 4. LOW_PRIORITY (15-30 min): Background/filler bots
     *    - Final population bots
     *    - Slow spawn rate (2-5 bots/sec)
     *    - World atmosphere
     *
     * Features:
     * - Phased spawning prevents resource spikes
     * - Adaptive phase transitions based on progress/time
     * - Respects adaptive throttler and circuit breaker
     * - Database preloading support (parallel character loading)
     * - Progress tracking and metrics
     *
     * Performance:
     * - Overhead: O(1) per update
     * - Memory: ~8KB
     * - Update frequency: Every world update tick
     *
     * Thread Safety:
     * - Not thread-safe, must be called from world update thread
     *
     * @example
     * @code
     * StartupSpawnOrchestrator orchestrator;
     * orchestrator.Initialize(priorityQueue, throttler);
     * orchestrator.BeginStartup();
     *
     * // In world update loop
     * orchestrator.Update(diff);
     *
     * if (orchestrator.ShouldSpawnNext())
     * {
     *     PrioritySpawnRequest request = priorityQueue.DequeueNextRequest();
     *     // ... spawn bot ...
     * }
     * @endcode
     */
    class TC_GAME_API StartupSpawnOrchestrator
    {
    public:
        StartupSpawnOrchestrator() = default;
        ~StartupSpawnOrchestrator() = default;

        /**
         * @brief Initialize orchestrator with dependencies
         * @param priorityQueue Pointer to SpawnPriorityQueue instance
         * @param throttler Pointer to AdaptiveSpawnThrottler instance
         * @return true on successful initialization
         */
        bool Initialize(SpawnPriorityQueue* priorityQueue, AdaptiveSpawnThrottler* throttler);

        /**
         * @brief Update orchestrator state (called every world tick)
         * @param diff Milliseconds since last update
         */
        void Update(uint32 diff);

        /**
         * @brief Begin phased startup sequence
         *
         * Transitions from IDLE to CRITICAL_BOTS phase after initial delay
         */
        void BeginStartup();

        /**
         * @brief Check if should spawn next bot in current phase
         * @return true if spawn should proceed
         *
         * Considers:
         * - Current phase allows spawning
         * - Throttler allows spawning
         * - Queue has requests for current priority
         */
        bool ShouldSpawnNext() const;

        /**
         * @brief Notify orchestrator of successful bot spawn
         *
         * Updates spawn counters and checks phase transition
         */
        void OnBotSpawned();

        /**
         * @brief Get current startup phase
         * @return StartupPhase Current phase
         */
        StartupPhase GetCurrentPhase() const { return _currentPhase; }

        /**
         * @brief Check if startup sequence is complete
         * @return bool true if all phases finished
         */
        bool IsStartupComplete() const { return _currentPhase == StartupPhase::COMPLETED; }

        /**
         * @brief Get orchestrator metrics
         * @return OrchestratorMetrics Current state and progress
         */
        OrchestratorMetrics GetMetrics() const;

        /**
         * @brief Force transition to next phase
         *
         * Useful for testing or manual phase control
         */
        void ForceNextPhase();

        /**
         * @brief Abort startup and transition to COMPLETED
         *
         * Emergency abort - useful for shutdown scenarios
         */
        void AbortStartup();

        /**
         * @brief Check if orchestrator is initialized
         * @return true if initialized
         */
        bool IsInitialized() const { return _initialized; }

    private:
        /**
         * @brief Transition to specified phase
         * @param newPhase Phase to transition to
         */
        void TransitionToPhase(StartupPhase newPhase);

        /**
         * @brief Check if current phase should transition to next
         * @return bool true if transition conditions met
         *
         * Transition triggers:
         * - Minimum duration elapsed AND
         *   - Target spawn count reached OR
         *   - Maximum duration elapsed OR
         *   - No more requests for current priority
         */
        bool ShouldTransitionPhase() const;

        /**
         * @brief Get configuration for current phase
         * @return PhaseConfig* Pointer to phase config (null if invalid)
         */
        const PhaseConfig* GetCurrentPhaseConfig() const;

        /**
         * @brief Calculate overall startup progress
         * @return float Progress (0.0-1.0)
         */
        float CalculateOverallProgress() const;

    private:
        // Configuration
        StartupOrchestratorConfig _config;

        // Dependencies (not owned)
        SpawnPriorityQueue* _priorityQueue = nullptr;
        AdaptiveSpawnThrottler* _throttler = nullptr;

        // Current state
        StartupPhase _currentPhase = StartupPhase::IDLE;
        TimePoint _phaseStartTime;
        TimePoint _startupBeginTime;

        // Phase metrics
        uint32 _botsSpawnedThisPhase = 0;
        uint32 _botsSpawnedTotal = 0;

        // State
        bool _initialized = false;
        bool _startupBegun = false;
    };

    /**
     * @brief Get string name for StartupPhase enum
     * @param phase Startup phase enum value
     * @return String representation
     */
    TC_GAME_API const char* GetStartupPhaseName(StartupPhase phase);

} // namespace Playerbot

#endif // PLAYERBOT_STARTUP_SPAWN_ORCHESTRATOR_H
