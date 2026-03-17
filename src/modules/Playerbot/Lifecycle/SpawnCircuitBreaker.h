/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_SPAWN_CIRCUIT_BREAKER_H
#define PLAYERBOT_SPAWN_CIRCUIT_BREAKER_H

#include "Define.h"
#include "Duration.h"
#include <deque>
#include <string>

namespace Playerbot
{
    /**
     * @brief Circuit breaker states for spawn failure handling
     *
     * Implements the Circuit Breaker pattern to prevent cascading
     * failures during bot spawning.
     *
     * State Transitions:
     * CLOSED → OPEN: >10% failure rate over 1 minute
     * OPEN → HALF_OPEN: After 60-second cooldown
     * HALF_OPEN → CLOSED: <5% failure rate for 2 minutes
     * HALF_OPEN → OPEN: Failure detected during recovery
     */
    enum class CircuitState : uint8
    {
        CLOSED,      ///< Normal operation, spawning allowed
        HALF_OPEN,   ///< Testing recovery, limited spawning (1 per 5 seconds)
        OPEN         ///< Failure detected, spawning blocked
    };

    /**
     * @brief Circuit breaker metrics for monitoring
     */
    struct TC_GAME_API CircuitBreakerMetrics
    {
        CircuitState state = CircuitState::CLOSED;
        float currentFailureRate = 0.0f;      ///< Current failure rate (0-100%)
        uint32 consecutiveFailures = 0;       ///< Consecutive failures in HALF_OPEN
        uint32 totalAttempts = 0;             ///< Total spawn attempts tracked
        uint32 totalFailures = 0;             ///< Total spawn failures tracked
        TimePoint lastStateChange;            ///< When state last changed
        TimePoint nextRetryTime;              ///< When OPEN → HALF_OPEN transition allowed

        Milliseconds timeInClosed = Milliseconds(0);
        Milliseconds timeInHalfOpen = Milliseconds(0);
        Milliseconds timeInOpen = Milliseconds(0);
    };

    /**
     * @brief Circuit breaker configuration
     */
    struct TC_GAME_API CircuitBreakerConfig
    {
        float openThresholdPercent = 10.0f;    ///< Failure rate to open circuit (%)
        float closeThresholdPercent = 5.0f;    ///< Failure rate to close from half-open (%)
        Milliseconds cooldownDuration = Milliseconds(60000);  ///< OPEN cooldown (60s)
        Milliseconds recoveryDuration = Milliseconds(120000); ///< HALF_OPEN test period (120s)
        Milliseconds slidingWindowDuration = Milliseconds(60000); ///< Failure rate window (60s)
        uint32 minimumAttempts = 10;          ///< Minimum attempts before circuit can open

        /**
         * @brief Load configuration from playerbots.conf
         */
        void LoadFromConfig();
    };

    /**
     * @brief Circuit breaker for spawn failure prevention
     *
     * Implements the Circuit Breaker pattern to detect and prevent
     * cascading failures during bot spawning. Automatically blocks
     * spawning when failure rate exceeds threshold and allows gradual
     * recovery after cooldown period.
     *
     * Features:
     * - Automatic failure detection (>10% failure rate)
     * - Cooldown period after opening (60 seconds)
     * - Gradual recovery testing (half-open state)
     * - Comprehensive metrics and logging
     * - Manual reset capability for emergency recovery
     *
     * Performance:
     * - Overhead: O(1) per spawn attempt
     * - Memory: ~4KB (sliding window buffer)
     * - Update frequency: Every spawn attempt
     *
     * Thread Safety:
     * - Not thread-safe, must be called from world update thread
     * - Designed for single-threaded access pattern
     *
     * @example
     * @code
     * SpawnCircuitBreaker breaker;
     * breaker.Initialize();
     *
     * // Before spawning
     * if (breaker.AllowSpawn())
     * {
     *     breaker.RecordAttempt();
     *     bool success = SpawnBot();
     *
     *     if (success)
     *         breaker.RecordSuccess();
     *     else
     *         breaker.RecordFailure();
     * }
     * else
     * {
     *     TC_LOG_WARN("...", "Circuit breaker is OPEN, spawning blocked");
     * }
     * @endcode
     */
    class TC_GAME_API SpawnCircuitBreaker
    {
    public:
        SpawnCircuitBreaker() = default;
        ~SpawnCircuitBreaker() = default;

        /**
         * @brief Initialize circuit breaker
         * @return true on successful initialization
         */
        bool Initialize();

        /**
         * @brief Update circuit breaker state (called periodically)
         * @param diff Milliseconds since last update
         */
        void Update(uint32 diff);

        /**
         * @brief Record spawn attempt
         */
        void RecordAttempt();

        /**
         * @brief Record spawn success
         */
        void RecordSuccess();

        /**
         * @brief Record spawn failure
         * @param reason Optional failure reason for logging
         */
        void RecordFailure(::std::string_view reason = "");

        /**
         * @brief Check if spawn is allowed in current state
         * @return true if spawning allowed, false if circuit is OPEN
         *
         * Returns:
         * - CLOSED: true (normal operation)
         * - HALF_OPEN: true if enough time passed since last attempt (rate-limited)
         * - OPEN: false (blocked until cooldown expires)
         */
        bool AllowSpawn();

        /**
         * @brief Get current circuit state
         * @return CircuitState (CLOSED/HALF_OPEN/OPEN)
         */
        CircuitState GetState() const { return _state; }

        /**
         * @brief Get current failure rate
         * @return float Failure rate percentage (0-100%)
         */
        float GetFailureRate() const;

        /**
         * @brief Get circuit breaker metrics
         * @return CircuitBreakerMetrics Current state and statistics
         */
        CircuitBreakerMetrics GetMetrics() const;

        /**
         * @brief Manually reset circuit breaker to CLOSED state
         *
         * Emergency override to force circuit closed. Use with caution.
         * Typically used after manual server fixes or during testing.
         */
        void Reset();

        /**
         * @brief Check if circuit breaker is initialized
         * @return true if initialized
         */
        bool IsInitialized() const { return _initialized; }

    private:
        /**
         * @brief Transition circuit to new state
         * @param newState State to transition to
         * @param reason Reason for transition (for logging)
         */
        void TransitionTo(CircuitState newState, ::std::string_view reason);

        /**
         * @brief Calculate failure rate from sliding window
         * @return float Failure rate percentage (0-100%)
         */
        float CalculateFailureRate() const;

        /**
         * @brief Clean old entries from sliding window
         */
        void CleanSlidingWindow();

        /**
         * @brief Check if transition from OPEN to HALF_OPEN is allowed
         * @return true if cooldown period has elapsed
         */
        bool CanTransitionToHalfOpen() const;

        /**
         * @brief Check if transition from HALF_OPEN to CLOSED is allowed
         * @return true if recovery period passed with low failure rate
         */
        bool CanTransitionToClosed() const;

        /**
         * @brief Record state duration metrics
         */
        void UpdateStateDurations();

    private:
        /**
         * @brief Spawn attempt result (for sliding window)
         */
        struct AttemptRecord
        {
            TimePoint timestamp;
            bool success;
        };

        // Configuration
        CircuitBreakerConfig _config;

        // Current state
        CircuitState _state = CircuitState::CLOSED;
        TimePoint _stateEntryTime;           ///< When current state was entered
        TimePoint _lastAttemptTime;          ///< Last spawn attempt time (for half-open rate limiting)

        // Sliding window for failure rate calculation
        ::std::deque<AttemptRecord> _attemptWindow;

        // Metrics
        uint32 _totalAttempts = 0;
        uint32 _totalFailures = 0;
        uint32 _totalSuccesses = 0;
        uint32 _consecutiveFailures = 0;    ///< Consecutive failures in HALF_OPEN state

        // State duration tracking
        TimePoint _lastDurationUpdate;
        Milliseconds _closedDuration = Milliseconds(0);
        Milliseconds _halfOpenDuration = Milliseconds(0);
        Milliseconds _openDuration = Milliseconds(0);

        // Initialization
        bool _initialized = false;
    };

    /**
     * @brief Get string name for CircuitState enum
     * @param state Circuit state enum value
     * @return String representation ("CLOSED", "HALF_OPEN", "OPEN")
     */
    TC_GAME_API const char* GetCircuitStateName(CircuitState state);

} // namespace Playerbot

#endif // PLAYERBOT_SPAWN_CIRCUIT_BREAKER_H
