/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotSubsystemRegistry.h"
#include "Log.h"
#include <algorithm>
#include <chrono>
#include <numeric>

namespace Playerbot
{

PlayerbotSubsystemRegistry* PlayerbotSubsystemRegistry::instance()
{
    static PlayerbotSubsystemRegistry inst;
    return &inst;
}

void PlayerbotSubsystemRegistry::RegisterSubsystem(std::unique_ptr<IPlayerbotSubsystem> subsystem)
{
    if (!subsystem)
        return;

    SubsystemEntry entry;
    entry.info = subsystem->GetInfo();
    entry.subsystem = std::move(subsystem);

    TC_LOG_DEBUG("module.playerbot", "SubsystemRegistry: Registered '{}'  init={} update={} shutdown={}",
        entry.info.name, entry.info.initOrder, entry.info.updateOrder, entry.info.shutdownOrder);

    _metrics[entry.info.name] = SubsystemMetrics{ entry.info.name };
    _subsystems.push_back(std::move(entry));
    _updateOrderCached = false;
}

// ============================================================================
// INITIALIZE ALL
// ============================================================================

bool PlayerbotSubsystemRegistry::InitializeAll(std::string const& moduleVersion)
{
    // Build sorted index by initOrder (skip entries with initOrder == 0)
    std::vector<size_t> initOrder;
    for (size_t i = 0; i < _subsystems.size(); ++i)
    {
        if (_subsystems[i].info.initOrder > 0)
            initOrder.push_back(i);
    }
    std::sort(initOrder.begin(), initOrder.end(), [this](size_t a, size_t b)
    {
        return _subsystems[a].info.initOrder < _subsystems[b].info.initOrder;
    });

    uint32 totalCount = static_cast<uint32>(initOrder.size());
    uint32 okCount = 0;
    uint32 warnCount = 0;
    uint32 failCount = 0;

    // Banner header
    TC_LOG_INFO("module.playerbot", " ");
    TC_LOG_INFO("module.playerbot",
        "======================================================================");
    TC_LOG_INFO("module.playerbot",
        "  Playerbot Module v{} initializing...", moduleVersion);
    TC_LOG_INFO("module.playerbot",
        "======================================================================");
    TC_LOG_INFO("module.playerbot", " Initializing {} subsystems...", totalCount);

    auto totalStart = std::chrono::high_resolution_clock::now();

    for (size_t idx : initOrder)
    {
        auto& entry = _subsystems[idx];
        auto const& info = entry.info;

        auto start = std::chrono::high_resolution_clock::now();
        bool success = false;

        try
        {
            success = entry.subsystem->Initialize();
        }
        catch (std::exception const& ex)
        {
            TC_LOG_ERROR("module.playerbot", "  EXCEPTION initializing '{}': {}", info.name, ex.what());
            success = false;
        }

        auto end = std::chrono::high_resolution_clock::now();
        uint64 elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        double elapsedMs = static_cast<double>(elapsedUs) / 1000.0;

        // Store metrics
        auto& metrics = _metrics[info.name];
        metrics.totalInitTimeUs = elapsedUs;

        if (success)
        {
            ++okCount;
            TC_LOG_INFO("module.playerbot", "   + {:<32} [{:>6.0f}ms]", info.name, elapsedMs);
        }
        else
        {
            switch (info.priority)
            {
                case SubsystemPriority::CRITICAL:
                    ++failCount;
                    _lastError = "Failed to initialize critical subsystem: " + info.name;
                    TC_LOG_ERROR("module.playerbot", "   X {:<32} [{:>6.0f}ms]  CRITICAL FAILURE", info.name, elapsedMs);
                    TC_LOG_ERROR("module.playerbot",
                        "----------------------------------------------------------------------");
                    TC_LOG_ERROR("module.playerbot",
                        " ABORT: Critical subsystem '{}' failed. Module cannot start.", info.name);
                    TC_LOG_ERROR("module.playerbot",
                        "======================================================================");
                    return false;

                case SubsystemPriority::HIGH:
                    ++warnCount;
                    TC_LOG_WARN("module.playerbot", "   ! {:<32} [{:>6.0f}ms]  (non-critical)", info.name, elapsedMs);
                    break;

                case SubsystemPriority::NORMAL:
                case SubsystemPriority::LOW:
                default:
                    ++failCount;
                    TC_LOG_WARN("module.playerbot", "   X {:<32} [{:>6.0f}ms]  (failed)", info.name, elapsedMs);
                    break;
            }
        }
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    uint64 totalUs = std::chrono::duration_cast<std::chrono::microseconds>(totalEnd - totalStart).count();
    double totalMs = static_cast<double>(totalUs) / 1000.0;

    // Summary
    TC_LOG_INFO("module.playerbot",
        "----------------------------------------------------------------------");
    TC_LOG_INFO("module.playerbot",
        " Result: {}/{} OK | {} warnings | {} failed | {:.0f}ms total",
        okCount, totalCount, warnCount, failCount, totalMs);
    TC_LOG_INFO("module.playerbot",
        "======================================================================");
    TC_LOG_INFO("module.playerbot",
        "  Playerbot Module v{} ready.", moduleVersion);
    TC_LOG_INFO("module.playerbot",
        "======================================================================");
    TC_LOG_INFO("module.playerbot", " ");

    return true;
}

// ============================================================================
// UPDATE ALL
// ============================================================================

void PlayerbotSubsystemRegistry::UpdateAll(uint32 diff)
{
    // Build cached update order on first call
    if (!_updateOrderCached)
    {
        _updateOrder.clear();
        for (size_t i = 0; i < _subsystems.size(); ++i)
        {
            if (_subsystems[i].info.updateOrder > 0)
                _updateOrder.push_back(i);
        }
        std::sort(_updateOrder.begin(), _updateOrder.end(), [this](size_t a, size_t b)
        {
            return _subsystems[a].info.updateOrder < _subsystems[b].info.updateOrder;
        });
        _updateOrderCached = true;
    }

    auto totalStart = std::chrono::high_resolution_clock::now();

    // Per-subsystem timing for this cycle
    struct CycleTiming
    {
        std::string name;
        uint64 timeUs;
    };
    std::vector<CycleTiming> cycleTimings;
    cycleTimings.reserve(_updateOrder.size());

    for (size_t idx : _updateOrder)
    {
        auto& entry = _subsystems[idx];
        auto start = std::chrono::high_resolution_clock::now();

        entry.subsystem->Update(diff);

        auto end = std::chrono::high_resolution_clock::now();
        uint64 elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // Update metrics
        auto& metrics = _metrics[entry.info.name];
        metrics.lastUpdateTimeUs = elapsedUs;
        metrics.totalUpdateTimeUs += elapsedUs;
        metrics.updateCount++;
        if (elapsedUs > metrics.maxUpdateTimeUs)
            metrics.maxUpdateTimeUs = elapsedUs;

        cycleTimings.push_back({ entry.info.name, elapsedUs });
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    uint64 totalUs = std::chrono::duration_cast<std::chrono::microseconds>(totalEnd - totalStart).count();

    // Performance warning if total exceeds threshold
    if (totalUs > UPDATE_WARN_THRESHOLD_US)
    {
        double totalMs = static_cast<double>(totalUs) / 1000.0;

        // Sort by time descending for top-5
        std::sort(cycleTimings.begin(), cycleTimings.end(),
            [](CycleTiming const& a, CycleTiming const& b) { return a.timeUs > b.timeUs; });

        std::string top5;
        uint32 count = 0;
        for (auto const& ct : cycleTimings)
        {
            if (count >= 5)
                break;
            if (!top5.empty())
                top5 += ", ";
            top5 += ct.name + ":" + std::to_string(ct.timeUs / 1000) + "ms";
            ++count;
        }

        TC_LOG_WARN("module.playerbot.performance",
            "PERFORMANCE: UpdateAll took {:.2f}ms  Top-5: {}", totalMs, top5);
    }
}

// ============================================================================
// SHUTDOWN ALL
// ============================================================================

void PlayerbotSubsystemRegistry::ShutdownAll()
{
    // Build sorted index by shutdownOrder (skip entries with shutdownOrder == 0)
    std::vector<size_t> shutdownOrder;
    for (size_t i = 0; i < _subsystems.size(); ++i)
    {
        if (_subsystems[i].info.shutdownOrder > 0)
            shutdownOrder.push_back(i);
    }
    std::sort(shutdownOrder.begin(), shutdownOrder.end(), [this](size_t a, size_t b)
    {
        return _subsystems[a].info.shutdownOrder < _subsystems[b].info.shutdownOrder;
    });

    TC_LOG_INFO("module.playerbot", "Shutting down {} subsystems...",
        static_cast<uint32>(shutdownOrder.size()));

    for (size_t idx : shutdownOrder)
    {
        auto& entry = _subsystems[idx];

        TC_LOG_INFO("module.playerbot", "  Shutting down {}...", entry.info.name);

        try
        {
            entry.subsystem->Shutdown();
        }
        catch (std::exception const& ex)
        {
            TC_LOG_ERROR("module.playerbot", "  EXCEPTION shutting down '{}': {}", entry.info.name, ex.what());
        }

        TC_LOG_DEBUG("module.playerbot", "  {} shutdown complete", entry.info.name);
    }

    TC_LOG_INFO("module.playerbot", "All subsystems shut down.");
}

// ============================================================================
// METRICS
// ============================================================================

SubsystemMetrics const* PlayerbotSubsystemRegistry::GetMetrics(std::string const& name) const
{
    auto it = _metrics.find(name);
    return it != _metrics.end() ? &it->second : nullptr;
}

std::vector<SubsystemMetrics> PlayerbotSubsystemRegistry::GetAllMetrics() const
{
    std::vector<SubsystemMetrics> result;
    result.reserve(_metrics.size());
    for (auto const& [name, metrics] : _metrics)
        result.push_back(metrics);
    return result;
}

} // namespace Playerbot
