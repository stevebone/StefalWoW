/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * ServerLoadMonitor: Monitors server tick performance and provides dynamic
 * reaction delay scaling for bots. When the server is under load, bots react
 * slower (100ms->500ms) to reduce CPU pressure. When idle, bots react faster
 * for more responsive gameplay.
 *
 * Uses BotPerformanceMonitor tick data as the source of truth.
 * Updated once per server tick (not per bot).
 */

#pragma once

#include "Define.h"
#include <atomic>

namespace Playerbot
{

/// Server load levels for reaction delay scaling
enum class ServerLoadLevel : uint8
{
    IDLE = 0,       // Server is barely loaded (<30% target tick)
    LIGHT = 1,      // Light load (30-60% target tick)
    NORMAL = 2,     // Normal load (60-80% target tick)
    HEAVY = 3,      // Heavy load (80-100% target tick)
    OVERLOADED = 4  // Overloaded (>100% target tick)
};

/// Configuration for the server load monitor
struct ServerLoadConfig
{
    // Tick time thresholds (as percentage of target tick time)
    float idleThreshold = 0.30f;        // Below 30% = IDLE
    float lightThreshold = 0.60f;       // 30-60% = LIGHT
    float normalThreshold = 0.80f;      // 60-80% = NORMAL
    float heavyThreshold = 1.00f;       // 80-100% = HEAVY
    // Above 100% = OVERLOADED

    // Reaction delay range (milliseconds)
    uint32 minReactionDelayMs = 100;    // Minimum delay (IDLE/LIGHT)
    uint32 normalReactionDelayMs = 200; // Normal delay
    uint32 heavyReactionDelayMs = 350;  // Heavy load delay
    uint32 maxReactionDelayMs = 500;    // Maximum delay (OVERLOADED)

    // Update interval for recalculating load level
    uint32 updateIntervalMs = 5000;     // Recalculate every 5 seconds

    // Smoothing: how fast to transition between load levels
    float smoothingFactor = 0.3f;       // 0.0 = instant, 1.0 = never change
};

class TC_GAME_API ServerLoadMonitor
{
public:
    static ServerLoadMonitor* instance()
    {
        static ServerLoadMonitor inst;
        return &inst;
    }

    /// Initialize with default configuration.
    void Initialize();

    /// Update load metrics. Called from main Update() loop.
    /// @param diff Milliseconds since last call
    void Update(uint32 diff);

    // ========================================================================
    // Queries (thread-safe via atomics)
    // ========================================================================

    /// Get the current server load level.
    ServerLoadLevel GetLoadLevel() const { return _loadLevel.load(std::memory_order_relaxed); }

    /// Get the recommended reaction delay for bots (milliseconds).
    /// Scales smoothly between min and max based on server load.
    uint32 GetReactionDelay() const { return _reactionDelayMs.load(std::memory_order_relaxed); }

    /// Get the current tick time ratio (0.0 = idle, 1.0 = at target, >1.0 = overloaded).
    float GetTickTimeRatio() const { return _tickTimeRatio.load(std::memory_order_relaxed); }

    /// Get a human-readable load level string.
    static char const* LoadLevelToString(ServerLoadLevel level);

    // ========================================================================
    // Configuration
    // ========================================================================

    void SetConfig(ServerLoadConfig const& config) { _config = config; }
    ServerLoadConfig const& GetConfig() const { return _config; }

private:
    ServerLoadMonitor();
    ~ServerLoadMonitor() = default;
    ServerLoadMonitor(ServerLoadMonitor const&) = delete;
    ServerLoadMonitor& operator=(ServerLoadMonitor const&) = delete;

    void RecalculateLoad();
    uint32 CalculateReactionDelay(float tickRatio) const;
    ServerLoadLevel ClassifyLoad(float tickRatio) const;

    ServerLoadConfig _config;
    uint32 _timeSinceLastUpdate{0};
    float _smoothedTickRatio{0.0f};

    // Atomic outputs for thread-safe reads
    std::atomic<ServerLoadLevel> _loadLevel{ServerLoadLevel::NORMAL};
    std::atomic<uint32> _reactionDelayMs{200};
    std::atomic<float> _tickTimeRatio{0.5f};

    bool _initialized{false};
};

#define sServerLoadMonitor ServerLoadMonitor::instance()

} // namespace Playerbot
