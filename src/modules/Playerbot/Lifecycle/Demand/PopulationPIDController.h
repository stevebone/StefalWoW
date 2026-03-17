/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * Population PID Controller
 *
 * Implements a PID (Proportional-Integral-Derivative) controller for smoothing
 * bot population management. Replaces the reactive deficit-based spawning with
 * a control-theory approach that eliminates oscillation and provides stable
 * convergence to target population levels.
 *
 * Integration: Sits between DemandCalculator and PopulationLifecycleController.
 * DemandCalculator calculates raw deficit; PID controller smooths the spawn/retire
 * rate to achieve stable population convergence.
 *
 * PID Formula:
 *   output = Kp * error + Ki * integral(error) + Kd * d(error)/dt
 *
 * Where:
 *   error = targetPopulation - currentPopulation
 *   Kp = proportional gain (immediate response to error)
 *   Ki = integral gain (corrects sustained steady-state error)
 *   Kd = derivative gain (dampens rate of change, prevents overshoot)
 */

#pragma once

#include "Define.h"
#include <chrono>
#include <deque>
#include <mutex>

namespace Playerbot
{

/// Configuration for the PID controller
struct PIDControllerConfig
{
    // PID gains (tuned for bot population control)
    float Kp = 0.3f;       // Proportional: moderate response to deficit
    float Ki = 0.05f;      // Integral: slow correction of steady-state error
    float Kd = 0.1f;       // Derivative: dampen rapid changes

    // Anti-windup limits for the integral term
    float integralMin = -100.0f;   // Prevent negative windup
    float integralMax = 100.0f;    // Prevent positive windup

    // Output clamping
    float outputMin = -20.0f;  // Max retire rate (negative = retire)
    float outputMax = 30.0f;   // Max spawn rate (positive = spawn)

    // Deadband: ignore error within this range (prevents micro-adjustments)
    float deadband = 2.0f;     // +/- 2 bots = "close enough"

    // Update interval (seconds)
    float updateIntervalSec = 5.0f;   // PID recalculates every 5 seconds

    // Smoothing: exponential moving average on derivative term
    float derivativeSmoothing = 0.7f;  // 0 = no smoothing, 1 = full smoothing

    // Per-bracket PID (each expansion bracket gets its own PID state)
    bool perBracketPID = true;

    // Safety: hard limits that override PID output
    uint32 absoluteMaxSpawnsPerHour = 60;
    uint32 absoluteMaxRetirementsPerHour = 20;

    // Ramp-up: limit output rate increase to prevent sudden bursts
    float maxOutputChangePerUpdate = 5.0f;   // Max change in output per update
};

/// Internal state for a single PID loop
struct PIDState
{
    float error = 0.0f;             // Current error (target - actual)
    float previousError = 0.0f;     // Previous error for derivative
    float integral = 0.0f;          // Accumulated integral term
    float derivative = 0.0f;        // Current derivative term
    float smoothedDerivative = 0.0f;// Exponentially smoothed derivative
    float output = 0.0f;            // Current PID output
    float previousOutput = 0.0f;    // Previous output for rate limiting

    int32 targetPopulation = 0;     // Setpoint
    int32 currentPopulation = 0;    // Process variable

    std::chrono::steady_clock::time_point lastUpdateTime;
    bool initialized = false;

    /// Reset all state
    void Reset()
    {
        error = previousError = 0.0f;
        integral = derivative = smoothedDerivative = 0.0f;
        output = previousOutput = 0.0f;
        targetPopulation = currentPopulation = 0;
        initialized = false;
    }
};

/// Per-bracket PID output for the lifecycle controller
struct BracketPIDOutput
{
    uint32 bracketIndex = 0;        // 0=Starting, 1=Chromie, 2=DF, 3=TWW
    float pidOutput = 0.0f;         // Raw PID output
    int32 recommendedSpawns = 0;    // Positive = spawn this many
    int32 recommendedRetirements = 0; // Positive = retire this many
    float error = 0.0f;             // Current population error
    float integral = 0.0f;          // Integral term (for diagnostics)
    float derivative = 0.0f;        // Derivative term (for diagnostics)
};

/// Aggregate PID output across all brackets
struct PIDOutput
{
    int32 totalRecommendedSpawns = 0;
    int32 totalRecommendedRetirements = 0;
    float totalError = 0.0f;
    BracketPIDOutput brackets[4];
    std::chrono::steady_clock::time_point timestamp;
};

/// Population PID Controller
class TC_GAME_API PopulationPIDController
{
public:
    PopulationPIDController();
    ~PopulationPIDController() = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /// Initialize with config
    void Initialize(PIDControllerConfig const& config);

    /// Reset all PID state (e.g. after config change)
    void Reset();

    /// Load config from playerbots.conf
    void LoadConfig();

    // ========================================================================
    // Core PID Operation
    // ========================================================================

    /// Update the PID controller with current population data.
    /// Call this periodically (every analysisIntervalMs).
    /// @param bracketIndex Which expansion bracket (0-3)
    /// @param currentPop Current bot count in this bracket
    /// @param targetPop Target bot count for this bracket
    /// @return PID output for this bracket
    BracketPIDOutput UpdateBracket(uint32 bracketIndex, int32 currentPop, int32 targetPop);

    /// Compute aggregate PID output across all brackets.
    /// Call after updating all brackets individually.
    PIDOutput ComputeAggregate();

    /// Get the smoothed spawn count recommendation.
    /// Positive = spawn, negative = retire.
    /// @param maxSpawnRate Maximum spawns allowed (from rate limiter)
    /// @param maxRetireRate Maximum retirements allowed (from rate limiter)
    int32 GetSmoothedSpawnCount(uint32 maxSpawnRate, uint32 maxRetireRate) const;

    // ========================================================================
    // Queries
    // ========================================================================

    /// Get current config
    PIDControllerConfig const& GetConfig() const { return _config; }

    /// Get the last computed aggregate output
    PIDOutput const& GetLastOutput() const { return _lastOutput; }

    /// Get PID state for a bracket (for diagnostics)
    PIDState const& GetBracketState(uint32 bracketIndex) const;

    /// Is the system in steady state (all errors within deadband)?
    bool IsInSteadyState() const;

    /// Get a diagnostic string describing current PID state
    std::string GetDiagnosticString() const;

private:
    /// Run one iteration of the PID loop
    float ComputePID(PIDState& state, float error, float dt);

    /// Apply anti-windup to integral term
    void ApplyAntiWindup(PIDState& state);

    /// Apply output rate limiting
    float ApplyRateLimiting(PIDState& state, float rawOutput);

    /// Apply deadband to error
    float ApplyDeadband(float error) const;

    PIDControllerConfig _config;
    PIDState _bracketStates[4];     // One per expansion bracket
    PIDOutput _lastOutput;
    mutable std::mutex _mutex;
};

} // namespace Playerbot
