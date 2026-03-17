/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotMemoryManager.h"
#include "BotPerformanceMonitor.h"
#include "Log.h"
#include "Util.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#include <malloc.h>
#endif

namespace Playerbot
{

// CategoryMemoryStats Implementation
void CategoryMemoryStats::RecordAllocation(uint64_t size)
{
    totalAllocated.fetch_add(size);
    currentUsage.fetch_add(size);
    allocationCount.fetch_add(1);

    // Update peak usage
    uint64_t current = currentUsage.load();
    uint64_t peak = peakUsage.load();
    while (current > peak && !peakUsage.compare_exchange_weak(peak, current)) {}

    lastAllocation.store(::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count());
}

void CategoryMemoryStats::RecordDeallocation(uint64_t size)
{
    totalDeallocated.fetch_add(size);

    uint64_t currentUsageValue = currentUsage.load();
    if (currentUsageValue >= size)
        currentUsage.fetch_sub(size);
    else
        currentUsage.store(0);

    deallocationCount.fetch_add(1);

    lastDeallocation.store(::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count());
}

void CategoryMemoryStats::Reset()
{
    totalAllocated.store(0);
    totalDeallocated.store(0);
    currentUsage.store(0);
    peakUsage.store(0);
    allocationCount.store(0);
    deallocationCount.store(0);
    lastAllocation.store(0);
    lastDeallocation.store(0);
}

double CategoryMemoryStats::GetFragmentationRatio() const
{
    uint64_t allocated = totalAllocated.load();
    uint64_t deallocated = totalDeallocated.load();

    if (allocated == 0)
        return 0.0;

    return static_cast<double>(deallocated) / allocated;
}

double CategoryMemoryStats::GetAllocationRate() const
{
    uint64_t now = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();
    uint64_t firstAlloc = lastAllocation.load();

    if (firstAlloc == 0 || now <= firstAlloc)
        return 0.0;

    uint64_t timeSpan = now - firstAlloc;
    if (timeSpan == 0)
        return 0.0;

    return static_cast<double>(allocationCount.load()) * 1000000.0 / timeSpan; // Per second
}

// BotMemoryProfile Implementation
uint64_t BotMemoryProfile::GetTotalUsage() const
{
    uint64_t total = 0;
    for (const auto& stats : categoryStats)
    {
        total += stats.currentUsage.load();
    }
    return total;
}

double BotMemoryProfile::CalculateEfficiency() const
{
    uint64_t totalAllocated = 0;
    uint64_t totalCurrent = 0;

    for (const auto& stats : categoryStats)
    {
        totalAllocated += stats.totalAllocated.load();
        totalCurrent += stats.currentUsage.load();
    }

    if (totalAllocated == 0)
        return 1.0;

    return static_cast<double>(totalCurrent) / totalAllocated;
}

void BotMemoryProfile::UpdateMemoryMetrics()
{
    uint64_t currentTotal = GetTotalUsage();
    totalMemoryUsage.store(currentTotal);

    uint64_t peak = peakMemoryUsage.load();
    while (currentTotal > peak && !peakMemoryUsage.compare_exchange_weak(peak, currentTotal)) {}

    memoryEfficiency.store(CalculateEfficiency());

    // Calculate average fragmentation across categories
    double totalFragmentation = 0.0;
    uint32_t activeCategories = 0;

    for (const auto& stats : categoryStats)
    {
        if (stats.totalAllocated.load() > 0)
        {
            totalFragmentation += stats.GetFragmentationRatio();
            activeCategories++;
        }
    }

    if (activeCategories > 0)
        fragmentationRatio.store(totalFragmentation / activeCategories);

    lastMemoryCheck.store(::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count());
}

// SystemMemoryAnalytics Implementation
void SystemMemoryAnalytics::UpdateSystemMetrics()
{
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    totalSystemMemory.store(memInfo.ullTotalPhys);
    availableSystemMemory.store(memInfo.ullAvailPhys);

    double usagePercent = 100.0 - (static_cast<double>(memInfo.ullAvailPhys) / memInfo.ullTotalPhys * 100.0);
    systemMemoryUsagePercent.store(usagePercent);
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long pageSize = sysconf(_SC_PAGE_SIZE);
    long availPages = sysconf(_SC_AVPHYS_PAGES);

    if (pages > 0 && pageSize > 0)
    {
        totalSystemMemory.store(static_cast<uint64_t>(pages) * pageSize);
        availableSystemMemory.store(static_cast<uint64_t>(availPages) * pageSize);

        double usagePercent = 100.0 - (static_cast<double>(availPages) / pages * 100.0);
        systemMemoryUsagePercent.store(usagePercent);
    }
#endif
}

double SystemMemoryAnalytics::CalculateMemoryPressure() const
{
    return systemMemoryUsagePercent.load() / 100.0;
}

bool SystemMemoryAnalytics::IsMemoryPressureHigh() const
{
    return CalculateMemoryPressure() > 0.8; // 80% threshold
}

// TrackedPtr template specializations
template<typename T>
void TrackedPtr<T>::RecordAllocation()
{
    if (sMemoryManager.IsEnabled())
    {
        sMemoryManager.RecordAllocation(_ptr, sizeof(T), _category, _botGuid, "TrackedPtr");
    }
}

template<typename T>
void TrackedPtr<T>::RecordDeallocation()
{
    if (sMemoryManager.IsEnabled())
    {
        sMemoryManager.RecordDeallocation(_ptr, sizeof(T), _category, _botGuid);
    }
}

// MemoryPool template specializations
template<typename T, size_t PoolSize>
void MemoryPool<T, PoolSize>::RecordPoolAllocation()
{
    if (sMemoryManager.IsEnabled())
    {
        sMemoryManager.RecordAllocation(nullptr, sizeof(T), _category, _botGuid, "MemoryPool");
    }
}

template<typename T, size_t PoolSize>
void MemoryPool<T, PoolSize>::RecordPoolDeallocation()
{
    if (sMemoryManager.IsEnabled())
    {
        sMemoryManager.RecordDeallocation(nullptr, sizeof(T), _category, _botGuid);
    }
}

// BotMemoryManager Implementation
bool BotMemoryManager::Initialize()
{
    TC_LOG_INFO("playerbot", "Initializing Bot Memory Manager...");

    // Update initial system metrics
    _systemAnalytics.UpdateSystemMetrics();

    // Start maintenance thread
    _maintenanceThread = ::std::thread([this] { PerformMemoryMaintenance(); });

    _enabled.store(true);

    TC_LOG_INFO("playerbot", "Bot Memory Manager initialized successfully");
    return true;
}

void BotMemoryManager::Shutdown()
{
    TC_LOG_INFO("playerbot", "Shutting down Bot Memory Manager...");

    _enabled.store(false);
    _shutdownRequested.store(true);

    // Stop maintenance thread
    _maintenanceCondition.notify_all();
    if (_maintenanceThread.joinable())
        _maintenanceThread.join();

    // Perform final cleanup
    PerformGarbageCollection();

    TC_LOG_INFO("playerbot", "Bot Memory Manager shut down successfully");
}

void BotMemoryManager::RegisterBot(uint32_t botGuid)
{
    ::std::lock_guard lock(_profilesMutex);
    _botProfiles[botGuid] = BotMemoryProfile(botGuid);

    TC_LOG_DEBUG("playerbot", "Registered bot {} for memory tracking", botGuid);
}

void BotMemoryManager::UnregisterBot(uint32_t botGuid)
{
    {
        ::std::lock_guard lock(_profilesMutex);
        _botProfiles.erase(botGuid);
    }

    // Clean up any tracked allocations for this bot
    {
        ::std::lock_guard allocLock(_allocationsMutex);
        auto it = _activeAllocations.begin();
        while (it != _activeAllocations.end())
        {
            if (it->second.botGuid == botGuid)
                it = _activeAllocations.erase(it);
            else
                ++it;
        }
    }

    TC_LOG_DEBUG("playerbot", "Unregistered bot {} from memory tracking", botGuid);
}

void BotMemoryManager::RecordAllocation(void* address, uint64_t size, MemoryCategory category, uint32_t botGuid, const ::std::string& context)
{
    if (!_enabled.load())
        return;

    // Update bot profile
    {
        ::std::lock_guard lock(_profilesMutex);
        auto it = _botProfiles.find(botGuid);
        if (it != _botProfiles.end())
        {
            it->second.categoryStats[static_cast<size_t>(category)].RecordAllocation(size);
            it->second.totalMemoryUsage.fetch_add(size);

            // Update peak usage
            uint64_t current = it->second.totalMemoryUsage.load();
            uint64_t peak = it->second.peakMemoryUsage.load();
            while (current > peak && !it->second.peakMemoryUsage.compare_exchange_weak(peak, current)) {}
        }
    }

    // Track allocation for leak detection
    if (_leakDetectionEnabled.load() && address)
    {
        ::std::lock_guard allocLock(_allocationsMutex);
        _activeAllocations.emplace(address, MemoryLeakEntry(address, size, category, botGuid, context));

        // Limit tracking entries to prevent excessive memory usage
    if (_activeAllocations.size() > MAX_LEAK_ENTRIES)
        {
            auto oldest = ::std::min_element(_activeAllocations.begin(), _activeAllocations.end(),
                [](const auto& a, const auto& b) { return a.second.allocationTime < b.second.allocationTime; });
            if (oldest != _activeAllocations.end())
                _activeAllocations.erase(oldest);
        }
    }

    // Record in performance monitor
    sPerfMonitor.RecordMemoryUsage(botGuid, size, GetCategoryName(category));
}

void BotMemoryManager::RecordDeallocation(void* address, uint64_t size, MemoryCategory category, uint32_t botGuid)
{
    if (!_enabled.load())
        return;

    // Update bot profile
    {
        ::std::lock_guard lock(_profilesMutex);
        auto it = _botProfiles.find(botGuid);
        if (it != _botProfiles.end())
        {
            it->second.categoryStats[static_cast<size_t>(category)].RecordDeallocation(size);

            uint64_t currentUsage = it->second.totalMemoryUsage.load();
            if (currentUsage >= size)
                it->second.totalMemoryUsage.fetch_sub(size);
            else
                it->second.totalMemoryUsage.store(0);
        }
    }

    // Remove from leak tracking
    if (_leakDetectionEnabled.load() && address)
    {
        ::std::lock_guard allocLock(_allocationsMutex);
        _activeAllocations.erase(address);
    }
}

void BotMemoryManager::OptimizeBotMemory(uint32_t botGuid)
{
    if (!_optimizationEnabled.load())
        return;

    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it == _botProfiles.end())
        return;

    BotMemoryProfile& profile = it->second;
    uint64_t memoryBefore = profile.GetTotalUsage();

    // Flush less critical caches
    FlushBotCache(botGuid);

    // Perform defragmentation if needed
    if (profile.fragmentationRatio.load() > FRAGMENTATION_THRESHOLD)
    {
        ConsolidateFragmentedMemory();
    }

    uint64_t memoryAfter = profile.GetTotalUsage();
    if (memoryBefore > memoryAfter)
    {
        uint64_t reclaimed = memoryBefore - memoryAfter;
        profile.memoryOptimizations.fetch_add(1);
        _totalMemoryReclaimed.fetch_add(reclaimed);

        TC_LOG_DEBUG("playerbot", "Optimized memory for bot {}: reclaimed {} bytes", botGuid, reclaimed);
    }

    profile.UpdateMemoryMetrics();
}

void BotMemoryManager::PerformGarbageCollection()
{
    uint64_t startTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    uint64_t memoryBefore = GetTotalMemoryUsage();

    // Perform cleanup operations
    OptimizeCacheSize();
    CompactMemory();

    uint64_t memoryAfter = GetTotalMemoryUsage();
    uint64_t gcTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count() - startTime;

    // Update system analytics
    {
        ::std::lock_guard lock(_systemAnalyticsMutex);
        _systemAnalytics.garbageCollectionEvents.fetch_add(1);
        _systemAnalytics.totalGcTime.fetch_add(gcTime);

        if (memoryBefore > memoryAfter)
        {
            uint64_t reclaimed = memoryBefore - memoryAfter;
            _systemAnalytics.memoryReclaimed.fetch_add(reclaimed);
        }
    }

    TC_LOG_DEBUG("playerbot", "Garbage collection completed: {} bytes reclaimed in {}μs",
                memoryBefore > memoryAfter ? memoryBefore - memoryAfter : 0, gcTime);
}

void BotMemoryManager::CompactMemory()
{
#ifdef _WIN32
    // Windows memory compaction
    SetProcessWorkingSetSize(GetCurrentProcess(), SIZE_T(-1), SIZE_T(-1));
#else
    // Linux memory compaction
    malloc_trim(0);
#endif
}

uint64_t BotMemoryManager::ReclaimUnusedMemory()
{
    uint64_t memoryBefore = GetTotalMemoryUsage();

    // Flush all caches
    for (int category = 0; category < static_cast<int>(MemoryCategory::UNKNOWN); ++category)
    {
        FlushCache(static_cast<MemoryCategory>(category));
    }

    // Perform system-level memory reclamation
    CompactMemory();

    uint64_t memoryAfter = GetTotalMemoryUsage();
    return memoryBefore > memoryAfter ? memoryBefore - memoryAfter : 0;
}

BotMemoryProfile BotMemoryManager::GetBotMemoryProfile(uint32_t botGuid) const
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    return it != _botProfiles.end() ? it->second : BotMemoryProfile();
}

SystemMemoryAnalytics BotMemoryManager::GetSystemAnalytics() const
{
    ::std::lock_guard lock(_systemAnalyticsMutex);
    return _systemAnalytics;
}

::std::vector<uint32_t> BotMemoryManager::GetHighMemoryUsageBots(uint32_t count) const
{
    ::std::lock_guard lock(_profilesMutex);

    ::std::vector<::std::pair<uint32_t, uint64_t>> botUsage;
    for (const auto& [botGuid, profile] : _botProfiles)
    {
        botUsage.emplace_back(botGuid, profile.GetTotalUsage());
    }

    // Sort by memory usage (descending)
    ::std::sort(botUsage.begin(), botUsage.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });

    ::std::vector<uint32_t> result;
    result.reserve(::std::min(static_cast<size_t>(count), botUsage.size()));

    for (size_t i = 0; i < ::std::min(static_cast<size_t>(count), botUsage.size()); ++i)
    {
        result.push_back(botUsage[i].first);
    }

    return result;
}

void BotMemoryManager::DetectMemoryLeaks()
{
    if (!_leakDetectionEnabled.load())
        return;

    uint64_t now = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    ::std::vector<MemoryLeakEntry> suspectedLeaks;

    {
        ::std::lock_guard lock(_allocationsMutex);

        for (const auto& [address, entry] : _activeAllocations)
        {
            // Consider allocations older than 10 minutes as potential leaks
    if (now - entry.allocationTime > 600000000) // 10 minutes in microseconds
            {
                suspectedLeaks.push_back(entry);
            }
        }
    }

    if (!suspectedLeaks.empty())
    {
        TC_LOG_WARN("playerbot", "Detected {} potential memory leaks", suspectedLeaks.size());

        // Update bot leak counts
        ::std::lock_guard profileLock(_profilesMutex);
        for (const auto& leak : suspectedLeaks)
        {
            auto it = _botProfiles.find(leak.botGuid);
            if (it != _botProfiles.end())
            {
                it->second.memoryLeakCount.fetch_add(1);
            }
        }
    }

    _lastLeakDetection.store(now);
}

::std::vector<MemoryLeakEntry> BotMemoryManager::GetSuspectedLeaks() const
{
    ::std::lock_guard lock(_allocationsMutex);

    uint64_t now = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now().time_since_epoch()).count();

    ::std::vector<MemoryLeakEntry> leaks;

    for (const auto& [address, entry] : _activeAllocations)
    {
        if (now - entry.allocationTime > 600000000) // 10 minutes
        {
            leaks.push_back(entry);
        }
    }

    return leaks;
}

void BotMemoryManager::ReportMemoryLeaks() const
{
    auto leaks = GetSuspectedLeaks();

    if (leaks.empty())
    {
        TC_LOG_INFO("playerbot", "No memory leaks detected");
        return;
    }

    TC_LOG_WARN("playerbot", "Memory leak report - {} suspected leaks:", leaks.size());

    for (const auto& leak : leaks)
    {
        TC_LOG_WARN("playerbot", "  Bot {}: {} bytes in category {} ({})",
                   leak.botGuid, leak.size, static_cast<int>(leak.category), leak.context);
    }
}

void BotMemoryManager::FlushCache(MemoryCategory category)
{
    // Implementation would flush caches for specific category
    // This is a placeholder for cache management logic

    ::std::lock_guard lock(_profilesMutex);
    for (auto& [botGuid, profile] : _botProfiles)
    {
        // Reset cache-related statistics for the category
        size_t categoryIndex = static_cast<size_t>(category);
        if (categoryIndex < profile.categoryStats.size())
        {
            auto& stats = profile.categoryStats[categoryIndex];
            // Only flush temporary/cache data, not persistent allocations
    if (category == MemoryCategory::CACHE_DATA ||
                category == MemoryCategory::TEMPORARY_DATA ||
                category == MemoryCategory::DATABASE_CACHE)
            {
                uint64_t currentUsage = stats.currentUsage.load();
                stats.totalDeallocated.fetch_add(currentUsage);
                stats.currentUsage.store(0);
            }
        }
    }
}

void BotMemoryManager::FlushBotCache(uint32_t botGuid)
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    if (it == _botProfiles.end())
        return;

    BotMemoryProfile& profile = it->second;

    // Flush cache categories
    ::std::vector<MemoryCategory> cacheCategories = {
        MemoryCategory::CACHE_DATA,
        MemoryCategory::TEMPORARY_DATA,
        MemoryCategory::DATABASE_CACHE
    };

    for (auto category : cacheCategories)
    {
        size_t categoryIndex = static_cast<size_t>(category);
        if (categoryIndex < profile.categoryStats.size())
        {
            auto& stats = profile.categoryStats[categoryIndex];
            uint64_t currentUsage = stats.currentUsage.load();
            if (currentUsage > 0)
            {
                stats.totalDeallocated.fetch_add(currentUsage);
                stats.currentUsage.store(0);
                profile.totalMemoryUsage.fetch_sub(currentUsage);
            }
        }
    }
}

void BotMemoryManager::OptimizeCacheSize()
{
    // Analyze cache hit ratios and adjust cache sizes accordingly
    ::std::lock_guard lock(_profilesMutex);

    for (auto& [botGuid, profile] : _botProfiles)
    {
        double cacheHitRatio = profile.cacheHitRatio.load();

        // If cache hit ratio is low, reduce cache size
    if (cacheHitRatio < 0.5)
        {
            FlushBotCache(botGuid);
        }

        profile.UpdateMemoryMetrics();
    }
}

void BotMemoryManager::HandleMemoryPressure()
{
    if (!IsMemoryPressureHigh())
        return;

    TC_LOG_WARN("playerbot", "High memory pressure detected - performing emergency cleanup");

    {
        ::std::lock_guard lock(_systemAnalyticsMutex);
        _systemAnalytics.memoryPressureEvents.fetch_add(1);
    }

    // Aggressive memory cleanup
    for (int category = 0; category < static_cast<int>(MemoryCategory::UNKNOWN); ++category)
    {
        if (category == static_cast<int>(MemoryCategory::CACHE_DATA) ||
            category == static_cast<int>(MemoryCategory::TEMPORARY_DATA))
        {
            FlushCache(static_cast<MemoryCategory>(category));
        }
    }

    // Force garbage collection
    PerformGarbageCollection();

    // Optimize high-usage bots
    auto highUsageBots = GetHighMemoryUsageBots(10);
    for (uint32_t botGuid : highUsageBots)
    {
        OptimizeBotMemory(botGuid);
    }
}

bool BotMemoryManager::IsMemoryPressureHigh() const
{
    return _systemAnalytics.CalculateMemoryPressure() > _memoryPressureThreshold.load();
}

void BotMemoryManager::GenerateMemoryReport(::std::string& report, uint32_t botGuid) const
{
    ::std::ostringstream oss;

    if (botGuid == 0)
    {
        // System-wide memory report
        oss << "=== Bot Memory Manager Report ===\n";
        oss << "Generated at: " << TimeToTimestampStr(time(nullptr)) << "\n\n";

        SystemMemoryAnalytics analytics = GetSystemAnalytics();

        oss << "System Memory Overview:\n";
        oss << "- Total System Memory: " << analytics.totalSystemMemory.load() / (1024 * 1024) << " MB\n";
        oss << "- Available Memory: " << analytics.availableSystemMemory.load() / (1024 * 1024) << " MB\n";
        oss << "- System Usage: " << ::std::fixed << ::std::setprecision(1) << analytics.systemMemoryUsagePercent.load() << "%\n";
        oss << "- Bot Memory Usage: " << analytics.totalBotMemory.load() / (1024 * 1024) << " MB\n";
        oss << "- Memory Pressure: " << (analytics.IsMemoryPressureHigh() ? "HIGH" : "NORMAL") << "\n\n";

        oss << "System Performance:\n";
        oss << "- Memory Pressure Events: " << analytics.memoryPressureEvents.load() << "\n";
        oss << "- Garbage Collections: " << analytics.garbageCollectionEvents.load() << "\n";
        oss << "- Total GC Time: " << analytics.totalGcTime.load() / 1000 << " ms\n";
        oss << "- Memory Reclaimed: " << analytics.memoryReclaimed.load() / (1024 * 1024) << " MB\n";
        oss << "- Optimizations Performed: " << analytics.optimizationsPerformed.load() << "\n\n";

        // Top memory usage bots
        auto highUsageBots = GetHighMemoryUsageBots(5);
        if (!highUsageBots.empty())
        {
            oss << "Top Memory Users:\n";
            for (uint32_t guid : highUsageBots)
            {
                BotMemoryProfile profile = GetBotMemoryProfile(guid);
                oss << "- Bot " << guid << ": " << profile.GetTotalUsage() / (1024 * 1024) << " MB"
                    << " (Efficiency: " << ::std::fixed << ::std::setprecision(1) << profile.memoryEfficiency.load() * 100 << "%)\n";
            }
        }
    }
    else
    {
        // Individual bot memory report
        BotMemoryProfile profile = GetBotMemoryProfile(botGuid);
        if (profile.botGuid == 0)
        {
            oss << "Bot " << botGuid << " not found in memory tracking.\n";
        }
        else
        {
            oss << "=== Bot Memory Report ===\n";
            oss << "Bot GUID: " << profile.botGuid << "\n";
            oss << "Total Memory Usage: " << profile.GetTotalUsage() / 1024 << " KB\n";
            oss << "Peak Memory Usage: " << profile.peakMemoryUsage.load() / 1024 << " KB\n";
            oss << "Memory Efficiency: " << ::std::fixed << ::std::setprecision(1) << profile.memoryEfficiency.load() * 100 << "%\n";
            oss << "Fragmentation Ratio: " << ::std::fixed << ::std::setprecision(1) << profile.fragmentationRatio.load() * 100 << "%\n";
            oss << "Cache Hit Ratio: " << ::std::fixed << ::std::setprecision(1) << profile.cacheHitRatio.load() * 100 << "%\n";
            oss << "Memory Leaks Detected: " << profile.memoryLeakCount.load() << "\n";
            oss << "Optimizations Performed: " << profile.memoryOptimizations.load() << "\n\n";

            oss << "Memory Usage by Category:\n";
            const char* categoryNames[] = {
                "Bot AI State", "Performance Metrics", "Combat Data", "Movement Data",
                "Spell Data", "Inventory Data", "Social Data", "Quest Data",
                "Cache Data", "Temporary Data", "Networking Data", "Database Cache", "Unknown"
            };

            for (size_t i = 0; i < profile.categoryStats.size(); ++i)
            {
                const auto& stats = profile.categoryStats[i];
                if (stats.currentUsage.load() > 0)
                {
                    oss << "- " << categoryNames[i] << ": " << stats.currentUsage.load() / 1024 << " KB"
                        << " (Peak: " << stats.peakUsage.load() / 1024 << " KB)\n";
                }
            }
        }
    }

    report = oss.str();
}

uint64_t BotMemoryManager::GetTotalMemoryUsage() const
{
    ::std::lock_guard lock(_profilesMutex);

    uint64_t total = 0;
    for (const auto& [botGuid, profile] : _botProfiles)
    {
        total += profile.GetTotalUsage();
    }

    return total;
}

uint64_t BotMemoryManager::GetBotMemoryUsage(uint32_t botGuid) const
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    return it != _botProfiles.end() ? it->second.GetTotalUsage() : 0;
}

double BotMemoryManager::GetMemoryEfficiency(uint32_t botGuid) const
{
    ::std::lock_guard lock(_profilesMutex);
    auto it = _botProfiles.find(botGuid);
    return it != _botProfiles.end() ? it->second.memoryEfficiency.load() : 0.0;
}

uint32_t BotMemoryManager::GetActiveAllocations() const
{
    ::std::lock_guard lock(_allocationsMutex);
    return static_cast<uint32_t>(_activeAllocations.size());
}

// Internal methods
void BotMemoryManager::PerformMemoryMaintenance()
{
    while (!_shutdownRequested.load())
    {
        ::std::unique_lock<::std::recursive_mutex> lock(_maintenanceMutex);
        _maintenanceCondition.wait_for(lock, ::std::chrono::microseconds(DEFAULT_MAINTENANCE_INTERVAL_US),
                                     [this] { return _shutdownRequested.load(); });

        if (_shutdownRequested.load())
            break;

        // Update system metrics
        _systemAnalytics.UpdateSystemMetrics();

        // Perform periodic maintenance
        uint64_t now = ::std::chrono::duration_cast<::std::chrono::microseconds>(
            ::std::chrono::steady_clock::now().time_since_epoch()).count();

        // Garbage collection
    if (now - _lastOptimization.load() >= _garbageCollectionInterval.load())
        {
            PerformGarbageCollection();
            _lastOptimization.store(now);
        }

        // Leak detection
    if (now - _lastLeakDetection.load() >= _leakDetectionInterval.load())
        {
            DetectMemoryLeaks();
        }

        // Handle memory pressure
    if (IsMemoryPressureHigh())
        {
            HandleMemoryPressure();
        }

        // Update all bot memory metrics
        UpdateMemoryStatistics();
    }
}

void BotMemoryManager::UpdateMemoryStatistics()
{
    ::std::lock_guard lock(_profilesMutex);

    for (auto& [botGuid, profile] : _botProfiles)
    {
        profile.UpdateMemoryMetrics();
    }

    // Update system analytics
    {
        ::std::lock_guard systemLock(_systemAnalyticsMutex);
        _systemAnalytics.totalBotMemory.store(GetTotalMemoryUsage());

        if (_systemAnalytics.totalSystemMemory.load() > 0)
        {
            double botUsagePercent = static_cast<double>(_systemAnalytics.totalBotMemory.load()) /
                                   _systemAnalytics.totalSystemMemory.load() * 100.0;
            _systemAnalytics.botMemoryUsagePercent.store(botUsagePercent);
        }
    }
}

void BotMemoryManager::ConsolidateFragmentedMemory()
{
    // Implementation would perform memory defragmentation
    // This is a placeholder for more advanced memory management
    CompactMemory();

    _totalOptimizations.fetch_add(1);
}

// Utility functions
::std::string GetCategoryName(MemoryCategory category)
{
    switch (category)
    {
        case MemoryCategory::BOT_AI_STATE: return "Bot AI State";
        case MemoryCategory::PERFORMANCE_METRICS: return "Performance Metrics";
        case MemoryCategory::COMBAT_DATA: return "Combat Data";
        case MemoryCategory::MOVEMENT_DATA: return "Movement Data";
        case MemoryCategory::SPELL_DATA: return "Spell Data";
        case MemoryCategory::INVENTORY_DATA: return "Inventory Data";
        case MemoryCategory::SOCIAL_DATA: return "Social Data";
        case MemoryCategory::QUEST_DATA: return "Quest Data";
        case MemoryCategory::CACHE_DATA: return "Cache Data";
        case MemoryCategory::TEMPORARY_DATA: return "Temporary Data";
        case MemoryCategory::NETWORKING_DATA: return "Networking Data";
        case MemoryCategory::DATABASE_CACHE: return "Database Cache";
        case MemoryCategory::UNKNOWN: return "Unknown";
        default: return "Invalid";
    }
}

} // namespace Playerbot