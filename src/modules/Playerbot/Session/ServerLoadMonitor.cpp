/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * ServerLoadMonitor: Dynamic reaction delay based on server tick performance.
 */

#include "ServerLoadMonitor.h"
#include "BotPerformanceMonitor.h"
#include "Log.h"

namespace Playerbot
{

ServerLoadMonitor::ServerLoadMonitor()
    : _config()
    , _timeSinceLastUpdate(0)
    , _smoothedTickRatio(0.5f)
    , _loadLevel(ServerLoadLevel::NORMAL)
    , _reactionDelayMs(200)
    , _tickTimeRatio(0.5f)
    , _initialized(false)
{
}

void ServerLoadMonitor::Initialize()
{
    if (_initialized)
        return;

    _smoothedTickRatio = 0.5f;
    _loadLevel.store(ServerLoadLevel::NORMAL, std::memory_order_relaxed);
    _reactionDelayMs.store(_config.normalReactionDelayMs, std::memory_order_relaxed);
    _tickTimeRatio.store(0.5f, std::memory_order_relaxed);
    _initialized = true;

    TC_LOG_INFO("module.playerbot", "ServerLoadMonitor: Initialized (reaction delay range {}ms-{}ms, update interval {}ms)",
        _config.minReactionDelayMs, _config.maxReactionDelayMs, _config.updateIntervalMs);
}

void ServerLoadMonitor::Update(uint32 diff)
{
    if (!_initialized)
        return;

    _timeSinceLastUpdate += diff;
    if (_timeSinceLastUpdate < _config.updateIntervalMs)
        return;

    _timeSinceLastUpdate = 0;
    RecalculateLoad();
}

void ServerLoadMonitor::RecalculateLoad()
{
    // Get current tick performance from BotPerformanceMonitor
    auto const& metrics = BotPerformanceMonitor::instance()->GetMetrics();
    uint32 targetTickTime = BotPerformanceMonitor::instance()->GetTargetTickTime();

    if (targetTickTime == 0)
        targetTickTime = 150000; // 150ms default in microseconds

    // Calculate raw tick time ratio (0.0 = idle, 1.0 = at target, >1.0 = overloaded)
    float rawRatio = static_cast<float>(metrics.averageTickTime) / static_cast<float>(targetTickTime);

    // Apply exponential smoothing to avoid jitter
    _smoothedTickRatio = _smoothedTickRatio * _config.smoothingFactor
                       + rawRatio * (1.0f - _config.smoothingFactor);

    // Classify load level
    ServerLoadLevel newLevel = ClassifyLoad(_smoothedTickRatio);
    ServerLoadLevel oldLevel = _loadLevel.load(std::memory_order_relaxed);

    // Calculate reaction delay
    uint32 newDelay = CalculateReactionDelay(_smoothedTickRatio);

    // Update atomic outputs
    _tickTimeRatio.store(_smoothedTickRatio, std::memory_order_relaxed);
    _loadLevel.store(newLevel, std::memory_order_relaxed);
    _reactionDelayMs.store(newDelay, std::memory_order_relaxed);

    // Log level transitions
    if (newLevel != oldLevel)
    {
        TC_LOG_INFO("module.playerbot", "ServerLoadMonitor: Load level changed {} -> {} (ratio {:.2f}, delay {}ms, avgTick {}us)",
            LoadLevelToString(oldLevel), LoadLevelToString(newLevel),
            _smoothedTickRatio, newDelay, metrics.averageTickTime);
    }
}

ServerLoadLevel ServerLoadMonitor::ClassifyLoad(float tickRatio) const
{
    if (tickRatio < _config.idleThreshold)
        return ServerLoadLevel::IDLE;
    if (tickRatio < _config.lightThreshold)
        return ServerLoadLevel::LIGHT;
    if (tickRatio < _config.normalThreshold)
        return ServerLoadLevel::NORMAL;
    if (tickRatio < _config.heavyThreshold)
        return ServerLoadLevel::HEAVY;
    return ServerLoadLevel::OVERLOADED;
}

uint32 ServerLoadMonitor::CalculateReactionDelay(float tickRatio) const
{
    // Linear interpolation between min and max delay based on tick ratio
    // At ratio 0.0 -> minReactionDelay
    // At ratio 1.0 -> maxReactionDelay
    // Clamped to [min, max] range

    float t = tickRatio; // 0.0 to ~1.5+
    if (t < 0.0f) t = 0.0f;
    if (t > 1.5f) t = 1.5f;

    // Use piecewise linear for better control:
    // [0, 0.3)   -> minReactionDelay
    // [0.3, 0.6) -> interpolate min -> normal
    // [0.6, 0.8) -> normal
    // [0.8, 1.0) -> interpolate normal -> heavy
    // [1.0, 1.5] -> interpolate heavy -> max

    uint32 delay;
    if (t < _config.idleThreshold)
    {
        delay = _config.minReactionDelayMs;
    }
    else if (t < _config.lightThreshold)
    {
        float fraction = (t - _config.idleThreshold) / (_config.lightThreshold - _config.idleThreshold);
        delay = _config.minReactionDelayMs + static_cast<uint32>(
            fraction * static_cast<float>(_config.normalReactionDelayMs - _config.minReactionDelayMs));
    }
    else if (t < _config.normalThreshold)
    {
        delay = _config.normalReactionDelayMs;
    }
    else if (t < _config.heavyThreshold)
    {
        float fraction = (t - _config.normalThreshold) / (_config.heavyThreshold - _config.normalThreshold);
        delay = _config.normalReactionDelayMs + static_cast<uint32>(
            fraction * static_cast<float>(_config.heavyReactionDelayMs - _config.normalReactionDelayMs));
    }
    else
    {
        // Overloaded: scale from heavy to max
        float fraction = (t - _config.heavyThreshold) / 0.5f; // 0.5 range above heavy
        if (fraction > 1.0f) fraction = 1.0f;
        delay = _config.heavyReactionDelayMs + static_cast<uint32>(
            fraction * static_cast<float>(_config.maxReactionDelayMs - _config.heavyReactionDelayMs));
    }

    return delay;
}

char const* ServerLoadMonitor::LoadLevelToString(ServerLoadLevel level)
{
    switch (level)
    {
        case ServerLoadLevel::IDLE:       return "IDLE";
        case ServerLoadLevel::LIGHT:      return "LIGHT";
        case ServerLoadLevel::NORMAL:     return "NORMAL";
        case ServerLoadLevel::HEAVY:      return "HEAVY";
        case ServerLoadLevel::OVERLOADED: return "OVERLOADED";
        default:                          return "UNKNOWN";
    }
}

} // namespace Playerbot
