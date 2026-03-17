/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * Population PID Controller implementation.
 */

#include "PopulationPIDController.h"
#include "Log.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Playerbot
{

// ============================================================================
// Constructor
// ============================================================================

PopulationPIDController::PopulationPIDController()
{
    _lastOutput = {};
}

// ============================================================================
// Lifecycle
// ============================================================================

void PopulationPIDController::Initialize(PIDControllerConfig const& config)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _config = config;
    Reset();

    TC_LOG_INFO("module.playerbot", "PopulationPIDController initialized: Kp={:.2f}, Ki={:.2f}, Kd={:.2f}, "
        "deadband={:.1f}, update interval={:.1f}s",
        _config.Kp, _config.Ki, _config.Kd, _config.deadband, _config.updateIntervalSec);
}

void PopulationPIDController::Reset()
{
    for (auto& state : _bracketStates)
        state.Reset();

    _lastOutput = {};
}

void PopulationPIDController::LoadConfig()
{
    // PID gains can be loaded from playerbots.conf
    // For now, using defaults. This method exists for future integration with
    // sConfigMgr->GetOption<float>("Playerbot.PID.Kp", 0.3f) etc.
    // The config is set during Initialize() from PopulationLifecycleController.
}

// ============================================================================
// Core PID Operation
// ============================================================================

BracketPIDOutput PopulationPIDController::UpdateBracket(
    uint32 bracketIndex, int32 currentPop, int32 targetPop)
{
    std::lock_guard<std::mutex> lock(_mutex);

    BracketPIDOutput result;
    result.bracketIndex = bracketIndex;

    if (bracketIndex >= 4)
    {
        TC_LOG_ERROR("module.playerbot", "PopulationPIDController: Invalid bracket index {}", bracketIndex);
        return result;
    }

    PIDState& state = _bracketStates[bracketIndex];
    auto now = std::chrono::steady_clock::now();

    // Calculate delta time
    float dt = _config.updateIntervalSec;
    if (state.initialized)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - state.lastUpdateTime);
        dt = static_cast<float>(elapsed.count()) / 1000.0f;

        // Guard against too-small or too-large dt
        if (dt < 0.1f)
            dt = 0.1f;
        if (dt > 60.0f)
            dt = 60.0f;
    }

    // Store measurements
    state.targetPopulation = targetPop;
    state.currentPopulation = currentPop;
    state.lastUpdateTime = now;

    // Calculate error (positive = need more bots, negative = too many)
    float rawError = static_cast<float>(targetPop - currentPop);

    // Apply deadband
    float error = ApplyDeadband(rawError);

    // Run PID
    float pidOutput = ComputePID(state, error, dt);

    // Store results
    result.pidOutput = pidOutput;
    result.error = state.error;
    result.integral = state.integral;
    result.derivative = state.smoothedDerivative;

    // Convert PID output to spawn/retire recommendations
    if (pidOutput > 0.0f)
    {
        result.recommendedSpawns = static_cast<int32>(std::ceil(pidOutput));
        result.recommendedRetirements = 0;
    }
    else if (pidOutput < 0.0f)
    {
        result.recommendedSpawns = 0;
        result.recommendedRetirements = static_cast<int32>(std::ceil(-pidOutput));
    }
    else
    {
        result.recommendedSpawns = 0;
        result.recommendedRetirements = 0;
    }

    state.initialized = true;
    return result;
}

PIDOutput PopulationPIDController::ComputeAggregate()
{
    std::lock_guard<std::mutex> lock(_mutex);

    PIDOutput output;
    output.timestamp = std::chrono::steady_clock::now();
    output.totalRecommendedSpawns = 0;
    output.totalRecommendedRetirements = 0;
    output.totalError = 0.0f;

    for (uint32 i = 0; i < 4; ++i)
    {
        PIDState const& state = _bracketStates[i];
        BracketPIDOutput& bracket = output.brackets[i];

        bracket.bracketIndex = i;
        bracket.error = state.error;
        bracket.integral = state.integral;
        bracket.derivative = state.smoothedDerivative;
        bracket.pidOutput = state.output;

        if (state.output > 0.0f)
        {
            bracket.recommendedSpawns = static_cast<int32>(std::ceil(state.output));
            bracket.recommendedRetirements = 0;
        }
        else if (state.output < 0.0f)
        {
            bracket.recommendedSpawns = 0;
            bracket.recommendedRetirements = static_cast<int32>(std::ceil(-state.output));
        }

        output.totalRecommendedSpawns += bracket.recommendedSpawns;
        output.totalRecommendedRetirements += bracket.recommendedRetirements;
        output.totalError += state.error;
    }

    _lastOutput = output;
    return output;
}

int32 PopulationPIDController::GetSmoothedSpawnCount(
    uint32 maxSpawnRate, uint32 maxRetireRate) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_lastOutput.totalRecommendedSpawns > 0)
    {
        return std::min(_lastOutput.totalRecommendedSpawns,
                        static_cast<int32>(maxSpawnRate));
    }
    else if (_lastOutput.totalRecommendedRetirements > 0)
    {
        return -std::min(_lastOutput.totalRecommendedRetirements,
                         static_cast<int32>(maxRetireRate));
    }

    return 0;
}

// ============================================================================
// Queries
// ============================================================================

PIDState const& PopulationPIDController::GetBracketState(uint32 bracketIndex) const
{
    static PIDState emptyState;
    if (bracketIndex >= 4)
        return emptyState;
    return _bracketStates[bracketIndex];
}

bool PopulationPIDController::IsInSteadyState() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto const& state : _bracketStates)
    {
        if (!state.initialized)
            continue;

        // Check if error is within deadband
        float absError = std::abs(static_cast<float>(state.targetPopulation - state.currentPopulation));
        if (absError > _config.deadband)
            return false;
    }

    return true;
}

std::string PopulationPIDController::GetDiagnosticString() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    static const char* bracketNames[] = { "Starting", "Chromie", "DF", "TWW" };
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    oss << "PID Controller State (Kp=" << _config.Kp
        << " Ki=" << _config.Ki
        << " Kd=" << _config.Kd << "):\n";

    for (uint32 i = 0; i < 4; ++i)
    {
        PIDState const& s = _bracketStates[i];
        if (!s.initialized)
            continue;

        oss << "  [" << bracketNames[i] << "] "
            << "pop=" << s.currentPopulation << "/" << s.targetPopulation
            << " err=" << s.error
            << " I=" << s.integral
            << " D=" << s.smoothedDerivative
            << " out=" << s.output
            << (std::abs(s.error) <= _config.deadband ? " (steady)" : "")
            << "\n";
    }

    oss << "  Total: spawns=" << _lastOutput.totalRecommendedSpawns
        << " retires=" << _lastOutput.totalRecommendedRetirements
        << " err=" << _lastOutput.totalError;

    return oss.str();
}

// ============================================================================
// Private: PID Computation
// ============================================================================

float PopulationPIDController::ComputePID(PIDState& state, float error, float dt)
{
    state.previousError = state.error;
    state.error = error;

    // ---- Proportional term ----
    float proportional = _config.Kp * error;

    // ---- Integral term ----
    // Only accumulate if error is outside deadband (prevents integral creep at target)
    if (std::abs(error) > 0.0f)
    {
        state.integral += error * dt;
    }
    ApplyAntiWindup(state);
    float integral = _config.Ki * state.integral;

    // ---- Derivative term ----
    if (dt > 0.0f)
    {
        state.derivative = (error - state.previousError) / dt;
    }
    else
    {
        state.derivative = 0.0f;
    }

    // Exponential moving average smoothing on derivative to reduce noise
    float alpha = _config.derivativeSmoothing;
    state.smoothedDerivative = alpha * state.smoothedDerivative + (1.0f - alpha) * state.derivative;
    float derivative = _config.Kd * state.smoothedDerivative;

    // ---- Compute raw output ----
    float rawOutput = proportional + integral + derivative;

    // ---- Clamp output ----
    rawOutput = std::clamp(rawOutput, _config.outputMin, _config.outputMax);

    // ---- Rate limit output ----
    rawOutput = ApplyRateLimiting(state, rawOutput);

    state.previousOutput = state.output;
    state.output = rawOutput;

    return rawOutput;
}

void PopulationPIDController::ApplyAntiWindup(PIDState& state)
{
    // Clamp integral to prevent windup
    state.integral = std::clamp(state.integral, _config.integralMin, _config.integralMax);

    // Back-calculation anti-windup: if output is saturated, stop integrating
    // in the direction of saturation
    float testOutput = _config.Kp * state.error + _config.Ki * state.integral;
    if (testOutput > _config.outputMax && state.error > 0.0f)
    {
        // Output would exceed max, don't accumulate more positive integral
        state.integral -= state.error * _config.updateIntervalSec * 0.5f;
        state.integral = std::max(state.integral, _config.integralMin);
    }
    else if (testOutput < _config.outputMin && state.error < 0.0f)
    {
        // Output would exceed min (negative), don't accumulate more negative integral
        state.integral -= state.error * _config.updateIntervalSec * 0.5f;
        state.integral = std::min(state.integral, _config.integralMax);
    }
}

float PopulationPIDController::ApplyRateLimiting(PIDState& state, float rawOutput)
{
    if (!state.initialized)
        return rawOutput;

    float maxChange = _config.maxOutputChangePerUpdate;
    float change = rawOutput - state.previousOutput;

    if (change > maxChange)
        return state.previousOutput + maxChange;
    else if (change < -maxChange)
        return state.previousOutput - maxChange;

    return rawOutput;
}

float PopulationPIDController::ApplyDeadband(float error) const
{
    if (std::abs(error) <= _config.deadband)
        return 0.0f;

    // Shift error so deadband acts as a "zero zone"
    if (error > 0.0f)
        return error - _config.deadband;
    else
        return error + _config.deadband;
}

} // namespace Playerbot
