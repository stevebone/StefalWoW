/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5: Performance Optimization - Profiler Implementation
 */

#include "Profiler.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {
namespace Performance {

Profiler::ScopedTimer::ScopedTimer(const ::std::string& section)
    : _section(section)
    , _enabled(Profiler::Instance().IsEnabled())
{
    if (_enabled)
    {
        _start = ::std::chrono::steady_clock::now();
    }
}

Profiler::ScopedTimer::~ScopedTimer()
{
    if (_enabled)
    {
        auto end = ::std::chrono::steady_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - _start).count();
        Profiler::Instance().RecordSection(_section, duration);
    }
}

void Profiler::RecordSection(const ::std::string& section, uint64 microseconds)
{
    ::std::lock_guard lock(_mutex);

    auto& data = _sections[section];
    data.totalTime.fetch_add(microseconds, ::std::memory_order_relaxed);
    data.callCount.fetch_add(1, ::std::memory_order_relaxed);

    // Update min/max
    uint64 currentMin = data.minTime.load(::std::memory_order_relaxed);
    while (microseconds < currentMin && !data.minTime.compare_exchange_weak(currentMin, microseconds)) {}

    uint64 currentMax = data.maxTime.load(::std::memory_order_relaxed);
    while (microseconds > currentMax && !data.maxTime.compare_exchange_weak(currentMax, microseconds)) {}
}

Profiler::ProfileResults Profiler::GetResults() const
{
    ::std::lock_guard lock(_mutex);

    ProfileResults results;
    for (const auto& [name, data] : _sections)
    {
        results.sections[name] = data.GetSnapshot();
    }
    return results;
}

void Profiler::Reset()
{
    ::std::lock_guard lock(_mutex);
    _sections.clear();
}

Profiler& Profiler::Instance()
{
    static Profiler instance;
    return instance;
}

} // namespace Performance
} // namespace Playerbot
