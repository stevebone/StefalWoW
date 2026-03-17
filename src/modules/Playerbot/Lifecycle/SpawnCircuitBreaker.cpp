/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SpawnCircuitBreaker.h"
#include "Config/PlayerbotConfig.h"
#include "Log.h"
#include "GameTime.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// CircuitBreakerConfig Implementation
// ============================================================================

void CircuitBreakerConfig::LoadFromConfig()
{
    openThresholdPercent = sPlayerbotConfig->GetFloat("Playerbot.CircuitBreaker.OpenThresholdPercent", 10.0f);
    closeThresholdPercent = sPlayerbotConfig->GetFloat("Playerbot.CircuitBreaker.CloseThresholdPercent", 5.0f);

    uint32 cooldownSeconds = sPlayerbotConfig->GetInt("Playerbot.CircuitBreaker.CooldownSeconds", 60);
    cooldownDuration = Milliseconds(cooldownSeconds * 1000);

    uint32 recoverySeconds = sPlayerbotConfig->GetInt("Playerbot.CircuitBreaker.RecoveryWindowSeconds", 120);
    recoveryDuration = Milliseconds(recoverySeconds * 1000);

    uint32 windowSeconds = sPlayerbotConfig->GetInt("Playerbot.CircuitBreaker.FailureWindowSeconds", 60);
    slidingWindowDuration = Milliseconds(windowSeconds * 1000);

    minimumAttempts = sPlayerbotConfig->GetInt("Playerbot.CircuitBreaker.MinimumSampleSize", 20);

    TC_LOG_INFO("module.playerbot.circuit",
        "CircuitBreaker config loaded: Open={:.1f}%, Close={:.1f}%, Cooldown={}s, Recovery={}s, Window={}s",
        openThresholdPercent, closeThresholdPercent, cooldownSeconds, recoverySeconds, windowSeconds);
}

// ============================================================================
// SpawnCircuitBreaker Implementation
// ============================================================================

bool SpawnCircuitBreaker::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("module.playerbot.circuit", "Initializing SpawnCircuitBreaker...");

    // Load configuration
    _config.LoadFromConfig();

    // Initialize state
    _state = CircuitState::CLOSED;
    _stateEntryTime = GameTime::Now();
    _lastDurationUpdate = GameTime::Now();

    _initialized = true;
    TC_LOG_INFO("module.playerbot.circuit", " SpawnCircuitBreaker initialized successfully");
    return true;
}

void SpawnCircuitBreaker::Update(uint32 /*diff*/)
{
    if (!_initialized)
        return;

    // Update state duration tracking
    UpdateStateDurations();

    // Clean old entries from sliding window
    CleanSlidingWindow();

    // Check for state transitions
    switch (_state)
    {
        case CircuitState::CLOSED:
        {
            // Check if failure rate exceeds threshold → OPEN
            float failureRate = CalculateFailureRate();
            if (_attemptWindow.size() >= _config.minimumAttempts &&
                failureRate >= _config.openThresholdPercent)
            {
                TransitionTo(CircuitState::OPEN,
                    fmt::format("Failure rate {:.1f}% exceeds threshold {:.1f}%",
                    failureRate, _config.openThresholdPercent));
            }
            break;
        }

        case CircuitState::OPEN:
        {
            // Check if cooldown elapsed → HALF_OPEN
    if (CanTransitionToHalfOpen())
            {
                TransitionTo(CircuitState::HALF_OPEN, "Cooldown period elapsed, testing recovery");
                _consecutiveFailures = 0;
            }
            break;
        }

        case CircuitState::HALF_OPEN:
        {
            // Check if recovery successful → CLOSED
    if (CanTransitionToClosed())
            {
                TransitionTo(CircuitState::CLOSED, "Recovery successful, failure rate below threshold");
            }
            // Check for failure during recovery → OPEN
            else if (_consecutiveFailures >= 3)
            {
                TransitionTo(CircuitState::OPEN,
                    fmt::format("{} consecutive failures during recovery", _consecutiveFailures));
            }
            break;
        }
    }
}

void SpawnCircuitBreaker::RecordAttempt()
{
    if (!_initialized)
        return;

    ++_totalAttempts;
    _lastAttemptTime = GameTime::Now();

    TC_LOG_TRACE("module.playerbot.circuit",
        "Circuit breaker: Attempt recorded (total: {}, state: {})",
        _totalAttempts, GetCircuitStateName(_state));
}

void SpawnCircuitBreaker::RecordSuccess()
{
    if (!_initialized)
        return;

    ++_totalSuccesses;
    _consecutiveFailures = 0;

    // Add success to sliding window
    AttemptRecord record;
    record.timestamp = GameTime::Now();
    record.success = true;
    _attemptWindow.push_back(record);

    TC_LOG_TRACE("module.playerbot.circuit",
        "Circuit breaker: Success recorded (total: {}, failure rate: {:.1f}%)",
        _totalSuccesses, GetFailureRate());
}

void SpawnCircuitBreaker::RecordFailure(::std::string_view reason)
{
    if (!_initialized)
        return;

    ++_totalFailures;
    ++_consecutiveFailures;

    // Add failure to sliding window
    AttemptRecord record;
    record.timestamp = GameTime::Now();
    record.success = false;
    _attemptWindow.push_back(record);

    float failureRate = GetFailureRate();

    TC_LOG_DEBUG("module.playerbot.circuit",
        "Circuit breaker: Failure recorded (total: {}, consecutive: {}, failure rate: {:.1f}%){}{}",
        _totalFailures, _consecutiveFailures, failureRate,
        reason.empty() ? "" : " - Reason: ",
        reason);

    // In HALF_OPEN state, immediate transition to OPEN on failure
    if (_state == CircuitState::HALF_OPEN && _consecutiveFailures >= 1)
    {
        TransitionTo(CircuitState::OPEN, "Failure during recovery testing");
    }
}

bool SpawnCircuitBreaker::AllowSpawn()
{
    if (!_initialized)
        return true;  // Default allow if not initialized
    switch (_state)
    {
        case CircuitState::CLOSED:
            return true;  // Normal operation

        case CircuitState::HALF_OPEN:
        {
            // Rate-limited spawning in half-open state (1 attempt per 5 seconds)
            TimePoint now = GameTime::Now();
            Milliseconds timeSinceLastAttempt = ::std::chrono::duration_cast<Milliseconds>(
                now - _lastAttemptTime);

            return timeSinceLastAttempt >= Milliseconds(5000);
        }

        case CircuitState::OPEN:
            return false;  // Blocked

        default:
            return true;
    }
}

float SpawnCircuitBreaker::GetFailureRate() const
{
    return CalculateFailureRate();
}

CircuitBreakerMetrics SpawnCircuitBreaker::GetMetrics() const
{
    CircuitBreakerMetrics metrics;

    metrics.state = _state;
    metrics.currentFailureRate = CalculateFailureRate();
    metrics.consecutiveFailures = _consecutiveFailures;
    metrics.totalAttempts = _totalAttempts;
    metrics.totalFailures = _totalFailures;
    metrics.lastStateChange = _stateEntryTime;

    // Calculate next retry time for OPEN state
    if (_state == CircuitState::OPEN)
    {
        metrics.nextRetryTime = _stateEntryTime + _config.cooldownDuration;
    }

    metrics.timeInClosed = _closedDuration;
    metrics.timeInHalfOpen = _halfOpenDuration;
    metrics.timeInOpen = _openDuration;

    return metrics;
}

void SpawnCircuitBreaker::Reset()
{
    TC_LOG_WARN("module.playerbot.circuit",
        "Circuit breaker MANUALLY RESET to CLOSED state (was {})",
        GetCircuitStateName(_state));

    _state = CircuitState::CLOSED;
    _stateEntryTime = GameTime::Now();
    _consecutiveFailures = 0;
    _attemptWindow.clear();
}

void SpawnCircuitBreaker::TransitionTo(CircuitState newState, ::std::string_view reason)
{
    if (newState == _state)
        return;

    CircuitState oldState = _state;
    _state = newState;
    _stateEntryTime = GameTime::Now();

    // Log state transition
    TC_LOG_WARN("module.playerbot.circuit",
        " Circuit breaker state transition: {} → {} - {}",
        GetCircuitStateName(oldState),
        GetCircuitStateName(newState),
        reason);

    // Reset consecutive failures on transitions
    if (newState == CircuitState::HALF_OPEN || newState == CircuitState::CLOSED)
        _consecutiveFailures = 0;
}

float SpawnCircuitBreaker::CalculateFailureRate() const
{
    if (_attemptWindow.empty())
        return 0.0f;

    uint32 failures = 0;
    uint32 attempts = 0;

    for (const AttemptRecord& record : _attemptWindow)
    {
        ++attempts;
        if (!record.success)
            ++failures;
    }

    return (float)(failures * 100.0 / attempts);
}

void SpawnCircuitBreaker::CleanSlidingWindow()
{
    TimePoint now = GameTime::Now();
    TimePoint cutoffTime = now - _config.slidingWindowDuration;

    // Remove entries older than sliding window duration
    while (!_attemptWindow.empty() &&
           _attemptWindow.front().timestamp < cutoffTime)
    {
        _attemptWindow.pop_front();
    }
}

bool SpawnCircuitBreaker::CanTransitionToHalfOpen() const
{
    if (_state != CircuitState::OPEN)
        return false;

    TimePoint now = GameTime::Now();
    Milliseconds timeInOpen = ::std::chrono::duration_cast<Milliseconds>(
        now - _stateEntryTime);

    return timeInOpen >= _config.cooldownDuration;
}

bool SpawnCircuitBreaker::CanTransitionToClosed() const
{
    if (_state != CircuitState::HALF_OPEN)
        return false;

    TimePoint now = GameTime::Now();
    Milliseconds timeInHalfOpen = ::std::chrono::duration_cast<Milliseconds>(
        now - _stateEntryTime);

    // Must be in half-open for recovery duration
    if (timeInHalfOpen < _config.recoveryDuration)
        return false;

    // Check if failure rate is below close threshold
    float failureRate = CalculateFailureRate();
    return failureRate < _config.closeThresholdPercent;
}

void SpawnCircuitBreaker::UpdateStateDurations()
{
    TimePoint now = GameTime::Now();
    Milliseconds elapsed = ::std::chrono::duration_cast<Milliseconds>(
        now - _lastDurationUpdate);

    switch (_state)
    {
        case CircuitState::CLOSED:
            _closedDuration += elapsed;
            break;
        case CircuitState::HALF_OPEN:
            _halfOpenDuration += elapsed;
            break;
        case CircuitState::OPEN:
            _openDuration += elapsed;
            break;
    }

    _lastDurationUpdate = now;
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* GetCircuitStateName(CircuitState state)
{
    switch (state)
    {
        case CircuitState::CLOSED:
            return "CLOSED";
        case CircuitState::HALF_OPEN:
            return "HALF_OPEN";
        case CircuitState::OPEN:
            return "OPEN";
        default:
            return "UNKNOWN";
    }
}

} // namespace Playerbot
