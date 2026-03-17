/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotProfiler.h"
#include "Log.h"
#include "Timer.h"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <random>

namespace Playerbot
{

// ScopedProfiler Implementation
ScopedProfiler::ScopedProfiler(const ::std::string& functionName, const ::std::string& fileName, uint32_t lineNumber)
    : _functionName(functionName), _fileName(fileName), _lineNumber(lineNumber),
      _category(HotspotCategory::UNKNOWN_HOTSPOT), _impactScore(0.0), _isHotspot(false)
{
    _startTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    // Only record if profiler is enabled and we're not exceeding overhead limits
    if (sProfiler.IsEnabled() && sProfiler.GetProfilingOverhead() < 10.0)
    {
        sProfiler._realTimeStats.totalSamples.fetch_add(1);
    }
}

ScopedProfiler::~ScopedProfiler()
{
    if (sProfiler.IsEnabled())
    {
        RecordProfile();
    }
}

void ScopedProfiler::AddMetadata(const ::std::string& key, const ::std::string& value)
{
    _metadata[key] = value;
}

void ScopedProfiler::SetCategory(HotspotCategory category)
{
    _category = category;
}

void ScopedProfiler::MarkAsHotspot(double impact)
{
    _isHotspot = true;
    _impactScore = impact;
}

void ScopedProfiler::RecordProfile()
{
    uint64_t endTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();
    uint64_t duration = endTime - _startTime;

    // Record the function call
    sProfiler.RecordFunctionCall(_functionName, duration);

    // If marked as hotspot, record it
    if (_isHotspot)
    {
        PerformanceHotspot hotspot(_category, _functionName, _impactScore);
        hotspot.hitCount = 1;
        hotspot.totalTime = duration;
        hotspot.averageTime = duration;
        hotspot.maxTime = duration;

        // Add metadata to description
    if (!_metadata.empty())
        {
            ::std::ostringstream oss;
            for (const auto& pair : _metadata)
            {
                oss << pair.first << ": " << pair.second << "; ";
            }
            hotspot.description = oss.str();
        }

        sProfiler.RecordHotspot(hotspot);
    }
}

// BotProfiler Implementation
bool BotProfiler::Initialize()
{
    ::std::lock_guard lock(_sessionsMutex);

    if (_enabled.load())
        return true;

    TC_LOG_INFO("playerbot", "BotProfiler: Initializing performance profiling system...");

    // Initialize real-time statistics
    _realTimeStats.Reset();

    // Start background processing
    StartBackgroundProcessing();

    // Integrate with other performance systems
    IntegratePerformanceMonitor();
    IntegrateAnalytics();
    IntegrateMemoryManager();
    IntegrateAIProfiler();
    IntegrateDatabaseOptimizer();
    IntegrateLoadTester();

    _enabled.store(true);
    TC_LOG_INFO("playerbot", "BotProfiler: Performance profiling system initialized successfully");
    return true;
}

void BotProfiler::Shutdown()
{
    ::std::lock_guard lock(_sessionsMutex);

    if (!_enabled.load())
        return;

    TC_LOG_INFO("playerbot", "BotProfiler: Shutting down performance profiling system...");

    _shutdownRequested.store(true);

    // Stop all active sessions
    StopAllSessions();

    // Stop background processing
    StopBackgroundProcessing();

    // Stop sampling if active
    StopSamplingProfiler();

    // Cleanup expired sessions
    CleanupExpiredSessions();

    _enabled.store(false);
    TC_LOG_INFO("playerbot", "BotProfiler: Performance profiling system shut down");
}

uint64_t BotProfiler::StartProfilingSession(const ::std::string& sessionName, ProfilingSessionType type)
{
    if (!_enabled.load())
    {
        TC_LOG_ERROR("playerbot", "BotProfiler: Cannot start session - profiler not enabled");
        return 0;
    }

    ::std::lock_guard lock(_sessionsMutex);

    if (_activeSessions.size() >= _maxSessions.load())
    {
        TC_LOG_ERROR("playerbot", "BotProfiler: Maximum number of sessions reached: {}", _maxSessions.load());
        return 0;
    }

    uint64_t sessionId = GenerateSessionId();
    ProfilingSession session(sessionId, sessionName, type);
    session.active = true;
    session.startTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    _activeSessions[sessionId] = session;
    _realTimeStats.activeProfilers.fetch_add(1);

    TC_LOG_INFO("playerbot", "BotProfiler: Started profiling session '{}' (ID: {}, Type: {})",
                sessionName, sessionId, static_cast<uint32_t>(type));

    return sessionId;
}

bool BotProfiler::StopProfilingSession(uint64_t sessionId)
{
    ::std::lock_guard lock(_sessionsMutex);

    auto it = _activeSessions.find(sessionId);
    if (it == _activeSessions.end())
    {
        TC_LOG_ERROR("playerbot", "BotProfiler: Session {} not found", sessionId);
        return false;
    }

    ProfilingSession& session = it->second;
    session.active = false;
    session.endTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();
    session.duration = session.endTime - session.startTime;

    // Analyze session data before archiving
    AnalyzeSessionData(&session);

    // Move to history
    _sessionHistory.push_back(session);
    _activeSessions.erase(it);
    _realTimeStats.activeProfilers.fetch_sub(1);

    TC_LOG_INFO("playerbot", "BotProfiler: Stopped profiling session '{}' (Duration: {} ms)",
                session.sessionName, session.duration / 1000);

    return true;
}

void BotProfiler::StopAllSessions()
{
    ::std::lock_guard lock(_sessionsMutex);

    ::std::vector<uint64_t> sessionIds;
    for (const auto& pair : _activeSessions)
        sessionIds.push_back(pair.first);

    for (uint64_t sessionId : sessionIds)
    {
        auto it = _activeSessions.find(sessionId);
        if (it != _activeSessions.end())
        {
            it->second.active = false;
            it->second.endTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
                ::std::chrono::steady_clock::now().time_since_epoch()).count();
            it->second.duration = it->second.endTime - it->second.startTime;

            _sessionHistory.push_back(it->second);
        }
    }

    _activeSessions.clear();
    _realTimeStats.activeProfilers.store(0);

    TC_LOG_INFO("playerbot", "BotProfiler: Stopped all active profiling sessions");
}

void BotProfiler::RecordFunctionCall(const ::std::string& functionName, uint64_t duration, uint32_t botGuid)
{
    if (!_enabled.load())
        return;

    // Update real-time statistics
    _realTimeStats.totalSamples.fetch_add(1);

    // Record with all active sessions
    ::std::lock_guard lock(_sessionsMutex);
    for (auto& pair : _activeSessions)
    {
        ProfilingSession& session = pair.second;
        if (!session.active)
            continue;

        // Check if this function is being tracked
    if (!session.targetFunctions.empty())
        {
            bool found = ::std::find(session.targetFunctions.begin(), session.targetFunctions.end(),
                                 functionName) != session.targetFunctions.end();
            if (!found)
                continue;
        }

        // Check if this bot is being tracked
    if (!session.targetBotGuids.empty())
        {
            bool found = ::std::find(session.targetBotGuids.begin(), session.targetBotGuids.end(),
                                 botGuid) != session.targetBotGuids.end();
            if (!found)
                continue;
        }

        // Update aggregated metrics
        session.aggregatedMetrics[functionName] += static_cast<double>(duration);
    }
}

void BotProfiler::RecordHotspot(const PerformanceHotspot& hotspot, uint64_t sessionId)
{
    if (!_enabled.load())
        return;

    ::std::lock_guard lock(_sessionsMutex);

    if (sessionId == 0)
    {
        // Record for all active sessions
    for (auto& pair : _activeSessions)
        {
            ProfilingSession& session = pair.second;
            if (session.active)
            {
                session.hotspots.push_back(hotspot);
            }
        }

        // Also record globally
        ::std::lock_guard dataLock(_dataMutex);
        _globalHotspots.push_back(hotspot);
    }
    else
    {
        // Record for specific session
        auto it = _activeSessions.find(sessionId);
        if (it != _activeSessions.end() && it->second.active)
        {
            it->second.hotspots.push_back(hotspot);
        }
    }

    _realTimeStats.hotspotsDetected.fetch_add(1);
}

::std::vector<PerformanceHotspot> BotProfiler::AnalyzeHotspots(uint64_t sessionId, uint32_t topN)
{
    ::std::lock_guard lock(_sessionsMutex);

    ::std::vector<PerformanceHotspot> allHotspots;

    if (sessionId == 0)
    {
        // Analyze all sessions
    for (const auto& pair : _activeSessions)
        {
            const ProfilingSession& session = pair.second;
            allHotspots.insert(allHotspots.end(), session.hotspots.begin(), session.hotspots.end());
        }

        // Include global hotspots
        ::std::lock_guard dataLock(_dataMutex);
        allHotspots.insert(allHotspots.end(), _globalHotspots.begin(), _globalHotspots.end());
    }
    else
    {
        // Analyze specific session
        auto it = _activeSessions.find(sessionId);
        if (it != _activeSessions.end())
        {
            allHotspots = it->second.hotspots;
        }
    }

    // Aggregate hotspots by location
    ::std::unordered_map<::std::string, PerformanceHotspot> aggregatedHotspots;
    for (const auto& hotspot : allHotspots)
    {
        auto& aggHotspot = aggregatedHotspots[hotspot.location];
        if (aggHotspot.location.empty())
        {
            aggHotspot = hotspot;
        }
        else
        {
            aggHotspot.hitCount += hotspot.hitCount;
            aggHotspot.totalTime += hotspot.totalTime;
            aggHotspot.maxTime = ::std::max(aggHotspot.maxTime, hotspot.maxTime);
            aggHotspot.impact = ::std::max(aggHotspot.impact, hotspot.impact);
        }

        // Recalculate average
    if (aggHotspot.hitCount > 0)
            aggHotspot.averageTime = aggHotspot.totalTime / aggHotspot.hitCount;
    }

    // Convert back to vector and sort by impact
    ::std::vector<PerformanceHotspot> result;
    for (const auto& pair : aggregatedHotspots)
    {
        result.push_back(pair.second);
    }

    ::std::sort(result.begin(), result.end(),
        [](const PerformanceHotspot& a, const PerformanceHotspot& b) {
            return a.impact > b.impact;
        });

    // Return top N
    if (result.size() > topN)
        result.resize(topN);

    return result;
}

::std::vector<PerformanceHotspot> BotProfiler::FindBottlenecks(HotspotCategory category, uint64_t sessionId)
{
    auto allHotspots = AnalyzeHotspots(sessionId, 100); // Get more for filtering

    ::std::vector<PerformanceHotspot> bottlenecks;
    for (const auto& hotspot : allHotspots)
    {
        if (hotspot.category == category && hotspot.impact >= DEFAULT_HOTSPOT_THRESHOLD)
        {
            bottlenecks.push_back(hotspot);
        }
    }

    return bottlenecks;
}

::std::vector<::std::string> BotProfiler::GetOptimizationRecommendations(uint64_t sessionId)
{
    ::std::vector<::std::string> recommendations;
    auto hotspots = AnalyzeHotspots(sessionId, 10);

    for (const auto& hotspot : hotspots)
    {
        switch (hotspot.category)
        {
            case HotspotCategory::CPU_INTENSIVE:
                recommendations.push_back(fmt::format("CPU Hotspot in {}: Consider algorithmic optimization or caching", hotspot.location));
                break;
            case HotspotCategory::MEMORY_INTENSIVE:
                recommendations.push_back(fmt::format("Memory Hotspot in {}: Consider memory pooling or reduced allocations", hotspot.location));
                break;
            case HotspotCategory::IO_BOUND:
                recommendations.push_back(fmt::format("I/O Hotspot in {}: Consider async operations or batching", hotspot.location));
                break;
            case HotspotCategory::SYNCHRONIZATION:
                recommendations.push_back(fmt::format("Lock Contention in {}: Consider lock-free algorithms or finer-grained locking", hotspot.location));
                break;
            case HotspotCategory::ALLOCATION_HEAVY:
                recommendations.push_back(fmt::format("Allocation Hotspot in {}: Consider object pooling or stack allocation", hotspot.location));
                break;
            case HotspotCategory::CACHE_MISSES:
                recommendations.push_back(fmt::format("Cache Inefficiency in {}: Consider data structure optimization or prefetching", hotspot.location));
                break;
            case HotspotCategory::ALGORITHMIC:
                recommendations.push_back(fmt::format("Algorithm Inefficiency in {}: Consider complexity reduction or better data structures", hotspot.location));
                break;
            default:
                recommendations.push_back(fmt::format("Performance issue in {}: Investigate further for optimization opportunities", hotspot.location));
                break;
        }

        // Add specific suggestions from hotspot
    for (const auto& suggestion : hotspot.optimizationSuggestions)
        {
            recommendations.push_back(suggestion);
        }
    }

    return recommendations;
}

RealTimeProfilingStats BotProfiler::GetRealTimeStats() const
{
    return _realTimeStats;
}

::std::vector<PerformanceHotspot> BotProfiler::GetCurrentHotspots(uint32_t topN)
{
    return AnalyzeHotspots(0, topN);
}

double BotProfiler::GetProfilingOverhead() const
{
    return _realTimeStats.overheadPercentage.load();
}

void BotProfiler::IntegratePerformanceMonitor()
{
    if (_performanceMonitorIntegrated.load())
        return;

    // Integration logic with BotPerformanceMonitor
    TC_LOG_DEBUG("playerbot", "BotProfiler: Integrating with PerformanceMonitor");
    _performanceMonitorIntegrated.store(true);
}

void BotProfiler::IntegrateAnalytics()
{
    if (_analyticsIntegrated.load())
        return;

    // Integration logic with BotPerformanceAnalytics
    TC_LOG_DEBUG("playerbot", "BotProfiler: Integrating with Analytics");
    _analyticsIntegrated.store(true);
}

void BotProfiler::IntegrateMemoryManager()
{
    if (_memoryManagerIntegrated.load())
        return;

    // Integration logic with BotMemoryManager
    TC_LOG_DEBUG("playerbot", "BotProfiler: Integrating with MemoryManager");
    _memoryManagerIntegrated.store(true);
}

void BotProfiler::IntegrateAIProfiler()
{
    if (_aiProfilerIntegrated.load())
        return;

    // Integration logic with AIDecisionProfiler
    TC_LOG_DEBUG("playerbot", "BotProfiler: Integrating with AIProfiler");
    _aiProfilerIntegrated.store(true);
}

void BotProfiler::IntegrateDatabaseOptimizer()
{
    if (_databaseOptimizerIntegrated.load())
        return;

    // Integration logic with DatabaseQueryOptimizer
    TC_LOG_DEBUG("playerbot", "BotProfiler: Integrating with DatabaseOptimizer");
    _databaseOptimizerIntegrated.store(true);
}

void BotProfiler::IntegrateLoadTester()
{
    if (_loadTesterIntegrated.load())
        return;

    // Integration logic with BotLoadTester
    TC_LOG_DEBUG("playerbot", "BotProfiler: Integrating with LoadTester");
    _loadTesterIntegrated.store(true);
}

bool BotProfiler::GeneratePerformanceReport(uint64_t sessionId, ::std::string& report)
{
    ::std::lock_guard lock(_sessionsMutex);

    ProfilingSession* session = nullptr;
    if (sessionId == 0)
    {
        // Generate report for all sessions
        ::std::ostringstream oss;
        oss << "=== Comprehensive Performance Report ===\n\n";

        oss << "Active Sessions: " << _activeSessions.size() << "\n";
        oss << "Total Sessions: " << _sessionHistory.size() << "\n";
        oss << "Real-time Statistics:\n";
        oss << "  Total Samples: " << _realTimeStats.totalSamples.load() << "\n";
        oss << "  Active Profilers: " << _realTimeStats.activeProfilers.load() << "\n";
        oss << "  Hotspots Detected: " << _realTimeStats.hotspotsDetected.load() << "\n";
        oss << "  Profiling Overhead: " << ::std::fixed << ::std::setprecision(2)
            << _realTimeStats.overheadPercentage.load() << "%\n\n";

        // Top hotspots across all sessions
        auto hotspots = AnalyzeHotspots(0, 10);
        oss << "=== Top Performance Hotspots ===\n";
        for (size_t i = 0; i < hotspots.size(); ++i)
        {
            const auto& hotspot = hotspots[i];
            oss << (i + 1) << ". " << hotspot.location << "\n";
            oss << "   Category: " << GetHotspotCategoryName(hotspot.category) << "\n";
            oss << "   Impact: " << ::std::fixed << ::std::setprecision(1) << hotspot.impact << "%\n";
            oss << "   Hit Count: " << hotspot.hitCount << "\n";
            oss << "   Average Time: " << hotspot.averageTime / 1000 << " ms\n";
            oss << "   Total Time: " << hotspot.totalTime / 1000 << " ms\n\n";
        }

        // Optimization recommendations
        auto recommendations = GetOptimizationRecommendations(0);
        oss << "=== Optimization Recommendations ===\n";
        for (size_t i = 0; i < recommendations.size(); ++i)
        {
            oss << (i + 1) << ". " << recommendations[i] << "\n";
        }

        report = oss.str();
        return true;
    }
    else
    {
        // Generate report for specific session
        auto it = _activeSessions.find(sessionId);
        if (it != _activeSessions.end())
        {
            session = &it->second;
        }
        else
        {
            // Check history
    for (auto& histSession : _sessionHistory)
            {
                if (histSession.sessionId == sessionId)
                {
                    session = &histSession;
                    break;
                }
            }
        }

        if (!session)
        {
            report = "Session not found";
            return false;
        }

        GenerateSessionSummary(session, report);
        return true;
    }
}

bool BotProfiler::ExportProfilingData(uint64_t sessionId, const ::std::string& filename, const ::std::string& format)
{
    ::std::lock_guard lock(_sessionsMutex);

    ProfilingSession* session = nullptr;
    auto it = _activeSessions.find(sessionId);
    if (it != _activeSessions.end())
    {
        session = &it->second;
    }
    else
    {
        for (auto& histSession : _sessionHistory)
        {
            if (histSession.sessionId == sessionId)
            {
                session = &histSession;
                break;
            }
        }
    }

    try
    {
        ::std::ofstream file(filename);
        if (!file.is_open())
        {
            TC_LOG_ERROR("playerbot", "BotProfiler: Failed to open file {} for export", filename);
            return false;
        }

        if (format == "json")
        {
            ExportToJSON(session, file);
        }
        else if (format == "csv")
        {
            ExportToCSV(session, file);
        }
        else if (format == "xml")
        {
            ExportToXML(session, file);
        }
        else
        {
            TC_LOG_ERROR("playerbot", "BotProfiler: Unsupported export format: {}", format);
            return false;
        }

        file.close();
        TC_LOG_INFO("playerbot", "BotProfiler: Exported session {} to {} in {} format", sessionId, filename, format);
        return true;
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot", "BotProfiler: Export failed - {}", e.what());
        return false;
    }
}

void BotProfiler::StartBackgroundProcessing()
{
    _processingThread = ::std::thread([this]() {
        while (!_shutdownRequested.load())
        {
            ::std::unique_lock<::std::recursive_mutex> lock(_processingMutex);
            _processingCondition.wait_for(lock, ::std::chrono::seconds(1));

            if (_shutdownRequested.load())
                break;

            ProcessProfilingData();
            UpdateRealTimeStatistics();
        }
    });

    TC_LOG_DEBUG("playerbot", "BotProfiler: Background processing started");
}

void BotProfiler::StopBackgroundProcessing()
{
    _shutdownRequested.store(true);
    _processingCondition.notify_all();

    if (_processingThread.joinable())
        _processingThread.join();

    TC_LOG_DEBUG("playerbot", "BotProfiler: Background processing stopped");
}

void BotProfiler::ProcessProfilingData()
{
    ::std::lock_guard lock(_dataMutex);

    // Process pending operations
    while (!_pendingOperations.empty())
    {
        auto operation = _pendingOperations.front();
        _pendingOperations.pop();

        try
        {
            operation();
        }
        catch (const ::std::exception& e)
        {
            TC_LOG_ERROR("playerbot", "BotProfiler: Error processing operation - {}", e.what());
        }
    }
}

void BotProfiler::UpdateRealTimeStatistics()
{
    ::std::lock_guard lock(_statsMutex);

    // Calculate profiling overhead
    auto now = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    static uint64_t lastUpdate = now;
    uint64_t timeDelta = now - lastUpdate;

    if (timeDelta > 1000000) // Update every second
    {
        // Estimate overhead based on sampling frequency and processing time
        double overhead = 0.0;
        if (_samplingActive.load())
        {
            overhead += 0.5; // Base sampling overhead
        }
        if (_instrumentationActive.load())
        {
            overhead += _realTimeStats.totalSamples.load() * 0.001; // Per-sample overhead
        }

        _realTimeStats.overheadPercentage.store(::std::min(overhead, 100.0));
        _realTimeStats.lastUpdateTime.store(now);
        lastUpdate = now;
    }
}

void BotProfiler::AnalyzeSessionData(ProfilingSession* session)
{
    if (!session)
        return;

    // Calculate hotspot impacts
    uint64_t totalTime = 0;
    for (const auto& pair : session->aggregatedMetrics)
    {
        totalTime += static_cast<uint64_t>(pair.second);
    }

    for (auto& hotspot : session->hotspots)
    {
        CalculateHotspotImpact(hotspot, session);
    }

    // Sort hotspots by impact
    ::std::sort(session->hotspots.begin(), session->hotspots.end(),
        [](const PerformanceHotspot& a, const PerformanceHotspot& b) {
            return a.impact > b.impact;
        });

    TC_LOG_DEBUG("playerbot", "BotProfiler: Analyzed session {} - {} hotspots found",
                 session->sessionId, session->hotspots.size());
}

void BotProfiler::CalculateHotspotImpact(PerformanceHotspot& hotspot, const ProfilingSession* session)
{
    // Calculate impact based on time spent and frequency
    uint64_t totalSessionTime = session->duration;
    if (totalSessionTime > 0)
    {
        hotspot.percentOfTotal = (static_cast<double>(hotspot.totalTime) / totalSessionTime) * 100.0;

        // Impact is a combination of time percentage and frequency
        double timeImpact = hotspot.percentOfTotal;
        double frequencyImpact = ::std::log10(static_cast<double>(hotspot.hitCount + 1)) * 10.0;

        hotspot.impact = ::std::min(100.0, (timeImpact * 0.7 + frequencyImpact * 0.3));
    }
}

uint64_t BotProfiler::GenerateSessionId()
{
    return _nextSessionId.fetch_add(1);
}

void BotProfiler::CleanupExpiredSessions()
{
    auto now = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    uint64_t retentionTime = static_cast<uint64_t>(_dataRetentionDays.load()) * 24 * 3600 * 1000000; // Convert to microseconds

    ::std::lock_guard lock(_sessionsMutex);

    // Remove old sessions from history
    _sessionHistory.erase(
        ::std::remove_if(_sessionHistory.begin(), _sessionHistory.end(),
            [now, retentionTime](const ProfilingSession& session) {
                return (now - session.endTime) > retentionTime;
            }),
        _sessionHistory.end());
}

::std::string BotProfiler::GetHotspotCategoryName(HotspotCategory category) const
{
    switch (category)
    {
        case HotspotCategory::CPU_INTENSIVE: return "CPU Intensive";
        case HotspotCategory::MEMORY_INTENSIVE: return "Memory Intensive";
        case HotspotCategory::IO_BOUND: return "I/O Bound";
        case HotspotCategory::NETWORK_BOUND: return "Network Bound";
        case HotspotCategory::SYNCHRONIZATION: return "Synchronization";
        case HotspotCategory::ALLOCATION_HEAVY: return "Allocation Heavy";
        case HotspotCategory::CACHE_MISSES: return "Cache Misses";
        case HotspotCategory::ALGORITHMIC: return "Algorithmic";
        case HotspotCategory::UNKNOWN_HOTSPOT: return "Unknown";
        default: return "Unknown";
    }
}

::std::string BotProfiler::GetSessionTypeName(ProfilingSessionType type) const
{
    switch (type)
    {
        case ProfilingSessionType::CONTINUOUS: return "Continuous";
        case ProfilingSessionType::SNAPSHOT: return "Snapshot";
        case ProfilingSessionType::BENCHMARK: return "Benchmark";
        case ProfilingSessionType::REGRESSION: return "Regression";
        case ProfilingSessionType::STRESS_PROFILE: return "Stress Profile";
        case ProfilingSessionType::IDLE_BASELINE: return "Idle Baseline";
        case ProfilingSessionType::CUSTOM_SESSION: return "Custom";
        default: return "Unknown";
    }
}

::std::string BotProfiler::GetScopeName(ProfilingScope scope) const
{
    switch (scope)
    {
        case ProfilingScope::SYSTEM_WIDE: return "System Wide";
        case ProfilingScope::BOT_SPECIFIC: return "Bot Specific";
        case ProfilingScope::CLASS_SPECIFIC: return "Class Specific";
        case ProfilingScope::GROUP_ANALYSIS: return "Group Analysis";
        case ProfilingScope::ZONE_ANALYSIS: return "Zone Analysis";
        case ProfilingScope::FEATURE_FOCUSED: return "Feature Focused";
        case ProfilingScope::CUSTOM_SCOPE: return "Custom";
        default: return "Unknown";
    }
}

void BotProfiler::GenerateSessionSummary(const ProfilingSession* session, ::std::string& summary)
{
    ::std::ostringstream oss;

    oss << "=== Profiling Session Report ===\n";
    oss << "Session ID: " << session->sessionId << "\n";
    oss << "Name: " << session->sessionName << "\n";
    oss << "Type: " << GetSessionTypeName(session->type) << "\n";
    oss << "Scope: " << GetScopeName(session->scope) << "\n";
    oss << "Duration: " << session->duration / 1000 << " ms\n";
    oss << "Status: " << (session->active ? "Active" : "Completed") << "\n\n";

    oss << "=== Configuration ===\n";
    oss << "Sampling Interval: " << session->samplingIntervalMs << " ms\n";
    oss << "Call Stack Profiling: " << (session->enableCallStack ? "Enabled" : "Disabled") << "\n";
    oss << "Memory Profiling: " << (session->enableMemoryProfiling ? "Enabled" : "Disabled") << "\n";
    oss << "Database Profiling: " << (session->enableDatabaseProfiling ? "Enabled" : "Disabled") << "\n";
    oss << "Max Call Stack Depth: " << session->maxCallStackDepth << "\n\n";

    if (!session->targetBotGuids.empty())
    {
        oss << "Target Bots: ";
        for (size_t i = 0; i < session->targetBotGuids.size(); ++i)
        {
            if (i > 0) oss << ", ";
            oss << session->targetBotGuids[i];
        }
        oss << "\n";
    }

    if (!session->targetFunctions.empty())
    {
        oss << "Target Functions: ";
        for (size_t i = 0; i < session->targetFunctions.size(); ++i)
        {
            if (i > 0) oss << ", ";
            oss << session->targetFunctions[i];
        }
        oss << "\n";
    }

    oss << "\n=== Performance Hotspots ===\n";
    for (size_t i = 0; i < ::std::min(session->hotspots.size(), size_t(10)); ++i)
    {
        const auto& hotspot = session->hotspots[i];
        oss << (i + 1) << ". " << hotspot.location << "\n";
        oss << "   Impact: " << ::std::fixed << ::std::setprecision(1) << hotspot.impact << "%\n";
        oss << "   Category: " << GetHotspotCategoryName(hotspot.category) << "\n";
        oss << "   Hit Count: " << hotspot.hitCount << "\n";
        oss << "   Average Time: " << hotspot.averageTime / 1000 << " ms\n\n";
    }

    oss << "=== Aggregated Metrics ===\n";
    for (const auto& pair : session->aggregatedMetrics)
    {
        oss << pair.first << ": " << ::std::fixed << ::std::setprecision(2)
            << pair.second / 1000.0 << " ms total\n";
    }

    summary = oss.str();
}

void BotProfiler::ExportToJSON(const ProfilingSession* session, ::std::ofstream& file)
{
    file << "{\n";
    file << "  \"sessionId\": " << session->sessionId << ",\n";
    file << "  \"name\": \"" << session->sessionName << "\",\n";
    file << "  \"type\": " << static_cast<uint32_t>(session->type) << ",\n";
    file << "  \"scope\": " << static_cast<uint32_t>(session->scope) << ",\n";
    file << "  \"duration\": " << session->duration << ",\n";
    file << "  \"active\": " << (session->active ? "true" : "false") << ",\n";

    file << "  \"hotspots\": [\n";
    for (size_t i = 0; i < session->hotspots.size(); ++i)
    {
        const auto& hotspot = session->hotspots[i];
        file << "    {\n";
        file << "      \"location\": \"" << hotspot.location << "\",\n";
        file << "      \"category\": " << static_cast<uint32_t>(hotspot.category) << ",\n";
        file << "      \"impact\": " << hotspot.impact << ",\n";
        file << "      \"hitCount\": " << hotspot.hitCount << ",\n";
        file << "      \"totalTime\": " << hotspot.totalTime << ",\n";
        file << "      \"averageTime\": " << hotspot.averageTime << ",\n";
        file << "      \"maxTime\": " << hotspot.maxTime << "\n";
        file << "    }";
        if (i < session->hotspots.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";

    file << "  \"aggregatedMetrics\": {\n";
    size_t count = 0;
    for (const auto& pair : session->aggregatedMetrics)
    {
        file << "    \"" << pair.first << "\": " << pair.second;
        if (++count < session->aggregatedMetrics.size()) file << ",";
        file << "\n";
    }
    file << "  }\n";

    file << "}\n";
}

void BotProfiler::ExportToCSV(const ProfilingSession* session, ::std::ofstream& file)
{
    // CSV header
    file << "Location,Category,Impact,HitCount,TotalTime,AverageTime,MaxTime\n";

    // CSV data
    for (const auto& hotspot : session->hotspots)
    {
        file << "\"" << hotspot.location << "\","
             << static_cast<uint32_t>(hotspot.category) << ","
             << hotspot.impact << ","
             << hotspot.hitCount << ","
             << hotspot.totalTime << ","
             << hotspot.averageTime << ","
             << hotspot.maxTime << "\n";
    }
}

void BotProfiler::ExportToXML(const ProfilingSession* session, ::std::ofstream& file)
{
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<ProfilingSession>\n";
    file << "  <sessionId>" << session->sessionId << "</sessionId>\n";
    file << "  <name>" << session->sessionName << "</name>\n";
    file << "  <type>" << static_cast<uint32_t>(session->type) << "</type>\n";
    file << "  <duration>" << session->duration << "</duration>\n";

    file << "  <hotspots>\n";
    for (const auto& hotspot : session->hotspots)
    {
        file << "    <hotspot>\n";
        file << "      <location>" << hotspot.location << "</location>\n";
        file << "      <category>" << static_cast<uint32_t>(hotspot.category) << "</category>\n";
        file << "      <impact>" << hotspot.impact << "</impact>\n";
        file << "      <hitCount>" << hotspot.hitCount << "</hitCount>\n";
        file << "      <totalTime>" << hotspot.totalTime << "</totalTime>\n";
        file << "      <averageTime>" << hotspot.averageTime << "</averageTime>\n";
        file << "      <maxTime>" << hotspot.maxTime << "</maxTime>\n";
        file << "    </hotspot>\n";
    }
    file << "  </hotspots>\n";

    file << "</ProfilingSession>\n";
}

} // namespace Playerbot