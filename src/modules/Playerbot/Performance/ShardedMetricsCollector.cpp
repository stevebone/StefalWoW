/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ShardedMetricsCollector.h"
#include "Log.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace Playerbot
{

// ============================================================================
// LIFECYCLE
// ============================================================================

bool ShardedMetricsCollector::Initialize()
{
    if (_initialized.load(::std::memory_order_relaxed))
        return true;

    TC_LOG_INFO("module.playerbot", "ShardedMetricsCollector: Initializing {} shards...", SHARD_COUNT);

    // Reset all shard diagnostic counters
    for (uint32 i = 0; i < SHARD_COUNT; ++i)
        _shardRecordCounts[i].store(0, ::std::memory_order_relaxed);

    _totalRegisteredBots.store(0, ::std::memory_order_relaxed);
    _totalRecordCount.store(0, ::std::memory_order_relaxed);

    _initialized.store(true, ::std::memory_order_release);
    _enabled.store(true, ::std::memory_order_release);

    TC_LOG_INFO("module.playerbot", "ShardedMetricsCollector: Initialized with {} shards, "
                "shared_mutex per shard (read-parallel, write-exclusive)",
                SHARD_COUNT);

    return true;
}

void ShardedMetricsCollector::Shutdown()
{
    TC_LOG_INFO("module.playerbot", "ShardedMetricsCollector: Shutting down...");

    _enabled.store(false, ::std::memory_order_release);

    // Clear all shards
    for (uint32 i = 0; i < SHARD_COUNT; ++i)
    {
        ::std::unique_lock lock(_shards[i].mutex);
        _shards[i].botMetrics.clear();
    }

    _totalRegisteredBots.store(0, ::std::memory_order_relaxed);
    _totalRecordCount.store(0, ::std::memory_order_relaxed);
    _initialized.store(false, ::std::memory_order_release);

    TC_LOG_INFO("module.playerbot", "ShardedMetricsCollector: Shutdown complete");
}

// ============================================================================
// METRIC RECORDING
// ============================================================================

void ShardedMetricsCollector::RecordMetric(uint32_t botGuid, BotMetricCategory category, uint64_t value)
{
    if (!_enabled.load(::std::memory_order_relaxed))
        return;

    if (static_cast<uint32>(category) >= METRIC_CATEGORY_COUNT)
        return;

    uint32 shardIdx = GetShardIndex(botGuid);
    MetricShard& shard = _shards[shardIdx];

    // Fast path: try with shared (read) lock first to find existing entry
    {
        ::std::shared_lock readLock(shard.mutex);
        auto it = shard.botMetrics.find(botGuid);
        if (it != shard.botMetrics.end())
        {
            // Bot exists in shard - record atomically (no write lock needed)
            it->second.categories[static_cast<uint32>(category)].Record(value);
            _shardRecordCounts[shardIdx].fetch_add(1, ::std::memory_order_relaxed);
            _totalRecordCount.fetch_add(1, ::std::memory_order_relaxed);
            return;
        }
    }

    // Slow path: bot not registered yet, need exclusive lock to insert
    {
        ::std::unique_lock writeLock(shard.mutex);
        // Double-check after acquiring write lock
        auto [it, inserted] = shard.botMetrics.try_emplace(botGuid);
        if (inserted)
        {
            it->second.botGuid = botGuid;
            _totalRegisteredBots.fetch_add(1, ::std::memory_order_relaxed);
        }
        it->second.categories[static_cast<uint32>(category)].Record(value);
        _shardRecordCounts[shardIdx].fetch_add(1, ::std::memory_order_relaxed);
        _totalRecordCount.fetch_add(1, ::std::memory_order_relaxed);
    }
}

// ============================================================================
// BOT LIFECYCLE
// ============================================================================

void ShardedMetricsCollector::RegisterBot(uint32_t botGuid)
{
    uint32 shardIdx = GetShardIndex(botGuid);
    MetricShard& shard = _shards[shardIdx];

    ::std::unique_lock lock(shard.mutex);
    auto [it, inserted] = shard.botMetrics.try_emplace(botGuid);
    if (inserted)
    {
        it->second.botGuid = botGuid;
        _totalRegisteredBots.fetch_add(1, ::std::memory_order_relaxed);
    }
}

void ShardedMetricsCollector::UnregisterBot(uint32_t botGuid)
{
    uint32 shardIdx = GetShardIndex(botGuid);
    MetricShard& shard = _shards[shardIdx];

    ::std::unique_lock lock(shard.mutex);
    auto it = shard.botMetrics.find(botGuid);
    if (it != shard.botMetrics.end())
    {
        shard.botMetrics.erase(it);
        _totalRegisteredBots.fetch_sub(1, ::std::memory_order_relaxed);
    }
}

void ShardedMetricsCollector::ResetBotMetrics(uint32_t botGuid)
{
    uint32 shardIdx = GetShardIndex(botGuid);
    MetricShard& shard = _shards[shardIdx];

    ::std::shared_lock lock(shard.mutex);
    auto it = shard.botMetrics.find(botGuid);
    if (it != shard.botMetrics.end())
        it->second.Reset();
}

void ShardedMetricsCollector::ResetAllMetrics()
{
    for (uint32 i = 0; i < SHARD_COUNT; ++i)
    {
        ::std::shared_lock lock(_shards[i].mutex);
        for (auto& [guid, entry] : _shards[i].botMetrics)
            entry.Reset();

        _shardRecordCounts[i].store(0, ::std::memory_order_relaxed);
    }
    _totalRecordCount.store(0, ::std::memory_order_relaxed);
}

// ============================================================================
// QUERIES
// ============================================================================

bool ShardedMetricsCollector::GetBotMetrics(uint32_t botGuid, BotMetricsSnapshot& outSnapshot) const
{
    uint32 shardIdx = GetShardIndex(botGuid);
    MetricShard const& shard = _shards[shardIdx];

    ::std::shared_lock lock(shard.mutex);
    auto it = shard.botMetrics.find(botGuid);
    if (it == shard.botMetrics.end())
        return false;

    outSnapshot.botGuid = botGuid;
    for (uint32 c = 0; c < METRIC_CATEGORY_COUNT; ++c)
    {
        outSnapshot.categories[c] = SnapshotStats(
            static_cast<BotMetricCategory>(c),
            it->second.categories[c]);
    }

    return true;
}

bool ShardedMetricsCollector::GetBotCategoryMetric(
    uint32_t botGuid, BotMetricCategory category, MetricCategorySnapshot& outSnapshot) const
{
    if (static_cast<uint32>(category) >= METRIC_CATEGORY_COUNT)
        return false;

    uint32 shardIdx = GetShardIndex(botGuid);
    MetricShard const& shard = _shards[shardIdx];

    ::std::shared_lock lock(shard.mutex);
    auto it = shard.botMetrics.find(botGuid);
    if (it == shard.botMetrics.end())
        return false;

    outSnapshot = SnapshotStats(category, it->second.categories[static_cast<uint32>(category)]);
    return true;
}

GlobalMetricsSnapshot ShardedMetricsCollector::GetGlobalSnapshot() const
{
    GlobalMetricsSnapshot snapshot;
    snapshot.snapshotTimestamp = static_cast<uint64_t>(
        ::std::chrono::duration_cast<::std::chrono::microseconds>(
            ::std::chrono::steady_clock::now().time_since_epoch()).count());

    // Initialize aggregated stats
    for (uint32 c = 0; c < METRIC_CATEGORY_COUNT; ++c)
    {
        snapshot.aggregated[c].category = static_cast<BotMetricCategory>(c);
        snapshot.aggregated[c].minValue = UINT64_MAX;
    }

    uint32_t totalBots = 0;

    // Iterate all shards, taking shared locks
    for (uint32 i = 0; i < SHARD_COUNT; ++i)
    {
        ::std::shared_lock lock(_shards[i].mutex);
        for (auto const& [guid, entry] : _shards[i].botMetrics)
        {
            totalBots++;
            for (uint32 c = 0; c < METRIC_CATEGORY_COUNT; ++c)
            {
                auto const& catStats = entry.categories[c];
                auto& agg = snapshot.aggregated[c];

                uint64_t count = catStats.sampleCount.load(::std::memory_order_relaxed);
                if (count == 0)
                    continue;

                uint64_t total = catStats.totalValue.load(::std::memory_order_relaxed);
                uint64_t mn = catStats.minValue.load(::std::memory_order_relaxed);
                uint64_t mx = catStats.maxValue.load(::std::memory_order_relaxed);
                uint64_t last = catStats.lastValue.load(::std::memory_order_relaxed);

                agg.sampleCount += count;
                agg.totalValue += total;
                if (mn < agg.minValue)
                    agg.minValue = mn;
                if (mx > agg.maxValue)
                    agg.maxValue = mx;
                agg.lastValue = last; // Last seen value (arbitrary across bots)
            }
        }
    }

    snapshot.totalBots = totalBots;

    // Calculate averages
    for (uint32 c = 0; c < METRIC_CATEGORY_COUNT; ++c)
    {
        auto& agg = snapshot.aggregated[c];
        if (agg.sampleCount > 0)
            agg.average = static_cast<double>(agg.totalValue) / agg.sampleCount;
        if (agg.minValue == UINT64_MAX)
            agg.minValue = 0;
    }

    return snapshot;
}

uint32_t ShardedMetricsCollector::GetRegisteredBotCount() const
{
    return _totalRegisteredBots.load(::std::memory_order_relaxed);
}

::std::vector<uint32_t> ShardedMetricsCollector::GetRegisteredBots() const
{
    ::std::vector<uint32_t> bots;
    bots.reserve(_totalRegisteredBots.load(::std::memory_order_relaxed));

    for (uint32 i = 0; i < SHARD_COUNT; ++i)
    {
        ::std::shared_lock lock(_shards[i].mutex);
        for (auto const& [guid, entry] : _shards[i].botMetrics)
            bots.push_back(guid);
    }

    return bots;
}

::std::vector<uint32_t> ShardedMetricsCollector::GetBotsExceedingThreshold(
    BotMetricCategory category, uint64_t thresholdAvg) const
{
    if (static_cast<uint32>(category) >= METRIC_CATEGORY_COUNT)
        return {};

    ::std::vector<uint32_t> exceeding;
    uint32 catIdx = static_cast<uint32>(category);

    for (uint32 i = 0; i < SHARD_COUNT; ++i)
    {
        ::std::shared_lock lock(_shards[i].mutex);
        for (auto const& [guid, entry] : _shards[i].botMetrics)
        {
            auto const& stats = entry.categories[catIdx];
            double avg = stats.GetAverage();
            if (avg > static_cast<double>(thresholdAvg))
                exceeding.push_back(guid);
        }
    }

    return exceeding;
}

// ============================================================================
// REPORTING
// ============================================================================

void ShardedMetricsCollector::GenerateReport(::std::string& outReport) const
{
    GlobalMetricsSnapshot snapshot = GetGlobalSnapshot();

    ::std::ostringstream oss;
    oss << "=== Sharded Metrics Report ===\n";
    oss << "Total Bots: " << snapshot.totalBots << "\n";
    oss << "Total Records: " << _totalRecordCount.load(::std::memory_order_relaxed) << "\n";
    oss << "Shard Count: " << SHARD_COUNT << "\n\n";

    oss << "Per-Category Aggregates:\n";
    oss << ::std::setw(20) << ::std::left << "Category"
        << ::std::setw(12) << ::std::right << "Samples"
        << ::std::setw(12) << "Average"
        << ::std::setw(12) << "Min"
        << ::std::setw(12) << "Max"
        << ::std::setw(12) << "Last"
        << "\n";
    oss << ::std::string(80, '-') << "\n";

    for (uint32 c = 0; c < METRIC_CATEGORY_COUNT; ++c)
    {
        auto const& agg = snapshot.aggregated[c];
        if (agg.sampleCount == 0)
            continue;

        oss << ::std::setw(20) << ::std::left << GetMetricCategoryName(static_cast<BotMetricCategory>(c))
            << ::std::setw(12) << ::std::right << agg.sampleCount
            << ::std::setw(12) << ::std::fixed << ::std::setprecision(1) << agg.average
            << ::std::setw(12) << agg.minValue
            << ::std::setw(12) << agg.maxValue
            << ::std::setw(12) << agg.lastValue
            << "\n";
    }

    // Shard distribution
    oss << "\nShard Distribution (top 10 busiest):\n";
    struct ShardInfo { uint32 idx; uint64_t records; uint32 bots; };
    ::std::vector<ShardInfo> shardInfos;
    shardInfos.reserve(SHARD_COUNT);

    for (uint32 i = 0; i < SHARD_COUNT; ++i)
    {
        uint32 botCount = 0;
        {
            ::std::shared_lock lock(_shards[i].mutex);
            botCount = static_cast<uint32>(_shards[i].botMetrics.size());
        }
        shardInfos.push_back({i, _shardRecordCounts[i].load(::std::memory_order_relaxed), botCount});
    }

    ::std::sort(shardInfos.begin(), shardInfos.end(),
        [](ShardInfo const& a, ShardInfo const& b) { return a.records > b.records; });

    for (uint32 i = 0; i < ::std::min(10u, static_cast<uint32>(shardInfos.size())); ++i)
    {
        auto const& si = shardInfos[i];
        if (si.records == 0)
            break;
        oss << "  Shard " << ::std::setw(3) << si.idx
            << ": " << ::std::setw(8) << si.records << " records, "
            << ::std::setw(4) << si.bots << " bots\n";
    }

    outReport = oss.str();
}

void ShardedMetricsCollector::ExportToCSV(const ::std::string& filename) const
{
    ::std::ofstream file(filename);
    if (!file.is_open())
    {
        TC_LOG_ERROR("module.playerbot", "ShardedMetricsCollector: Failed to open {} for export", filename);
        return;
    }

    // Header
    file << "botGuid";
    for (uint32 c = 0; c < METRIC_CATEGORY_COUNT; ++c)
    {
        ::std::string name = GetMetricCategoryName(static_cast<BotMetricCategory>(c));
        file << "," << name << "_samples"
             << "," << name << "_avg"
             << "," << name << "_min"
             << "," << name << "_max"
             << "," << name << "_last";
    }
    file << "\n";

    // Data rows
    for (uint32 i = 0; i < SHARD_COUNT; ++i)
    {
        ::std::shared_lock lock(_shards[i].mutex);
        for (auto const& [guid, entry] : _shards[i].botMetrics)
        {
            file << guid;
            for (uint32 c = 0; c < METRIC_CATEGORY_COUNT; ++c)
            {
                auto const& stats = entry.categories[c];
                uint64_t count = stats.sampleCount.load(::std::memory_order_relaxed);
                double avg = stats.GetAverage();
                uint64_t mn = stats.minValue.load(::std::memory_order_relaxed);
                uint64_t mx = stats.maxValue.load(::std::memory_order_relaxed);
                uint64_t last = stats.lastValue.load(::std::memory_order_relaxed);

                if (mn == UINT64_MAX)
                    mn = 0;

                file << "," << count
                     << "," << ::std::fixed << ::std::setprecision(1) << avg
                     << "," << mn
                     << "," << mx
                     << "," << last;
            }
            file << "\n";
        }
    }

    file.close();
    TC_LOG_INFO("module.playerbot", "ShardedMetricsCollector: Exported metrics to {}", filename);
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

::std::vector<ShardedMetricsCollector::ShardDiagnostics> ShardedMetricsCollector::GetShardDiagnostics() const
{
    ::std::vector<ShardDiagnostics> diags;
    diags.reserve(SHARD_COUNT);

    for (uint32 i = 0; i < SHARD_COUNT; ++i)
    {
        ShardDiagnostics d;
        d.shardIndex = i;
        d.recordCount = _shardRecordCounts[i].load(::std::memory_order_relaxed);

        {
            ::std::shared_lock lock(_shards[i].mutex);
            d.botCount = static_cast<uint32>(_shards[i].botMetrics.size());
        }

        diags.push_back(d);
    }

    return diags;
}

// ============================================================================
// INTERNAL
// ============================================================================

MetricCategorySnapshot ShardedMetricsCollector::SnapshotStats(
    BotMetricCategory cat, AtomicMetricStats const& stats)
{
    MetricCategorySnapshot snap;
    snap.category = cat;
    snap.sampleCount = stats.sampleCount.load(::std::memory_order_relaxed);
    snap.totalValue = stats.totalValue.load(::std::memory_order_relaxed);
    snap.minValue = stats.minValue.load(::std::memory_order_relaxed);
    snap.maxValue = stats.maxValue.load(::std::memory_order_relaxed);
    snap.lastValue = stats.lastValue.load(::std::memory_order_relaxed);

    if (snap.minValue == UINT64_MAX)
        snap.minValue = 0;

    snap.average = snap.sampleCount > 0
        ? static_cast<double>(snap.totalValue) / snap.sampleCount
        : 0.0;

    return snap;
}

} // namespace Playerbot
