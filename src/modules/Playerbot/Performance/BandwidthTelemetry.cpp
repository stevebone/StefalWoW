/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BandwidthTelemetry.h"
#include "Log.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

BandwidthTelemetry& BandwidthTelemetry::Instance()
{
    static BandwidthTelemetry instance;
    return instance;
}

// ============================================================================
// RECORDING
// ============================================================================

void BandwidthTelemetry::RecordPacket(ObjectGuid botGuid, PacketDirection direction,
                                       uint32 opcode, uint32 sizeBytes)
{
    // Record per-bot stats
    {
        ::std::shared_lock lock(_botMutex);
        auto it = _botStats.find(botGuid);
        if (it != _botStats.end())
        {
            if (direction == PacketDirection::INBOUND)
            {
                it->second->packetsIn.fetch_add(1, ::std::memory_order_relaxed);
                it->second->bytesIn.fetch_add(sizeBytes, ::std::memory_order_relaxed);
            }
            else
            {
                it->second->packetsOut.fetch_add(1, ::std::memory_order_relaxed);
                it->second->bytesOut.fetch_add(sizeBytes, ::std::memory_order_relaxed);
            }
        }
    }

    // Record per-opcode stats
    {
        ::std::shared_lock lock(_opcodeMutex);
        auto it = _opcodeStats.find(opcode);
        if (it != _opcodeStats.end())
        {
            it->second->Record(sizeBytes);
        }
        else
        {
            lock.unlock();
            ::std::unique_lock writeLock(_opcodeMutex);
            // Double-check after acquiring write lock
            auto it2 = _opcodeStats.find(opcode);
            if (it2 == _opcodeStats.end())
            {
                auto stats = ::std::make_unique<OpcodeStats>();
                stats->Record(sizeBytes);
                _opcodeStats[opcode] = ::std::move(stats);
            }
            else
            {
                it2->second->Record(sizeBytes);
            }
        }
    }
}

void BandwidthTelemetry::RecordFilteredPacket(ObjectGuid botGuid, uint32 /*opcode*/,
                                                uint32 sizeBytes)
{
    ::std::shared_lock lock(_botMutex);
    auto it = _botStats.find(botGuid);
    if (it != _botStats.end())
    {
        it->second->packetsFiltered.fetch_add(1, ::std::memory_order_relaxed);
        it->second->bytesFiltered.fetch_add(sizeBytes, ::std::memory_order_relaxed);
    }
}

// ============================================================================
// BOT LIFECYCLE
// ============================================================================

void BandwidthTelemetry::RegisterBot(ObjectGuid botGuid)
{
    ::std::unique_lock lock(_botMutex);
    if (_botStats.find(botGuid) == _botStats.end())
    {
        _botStats[botGuid] = ::std::make_unique<BotBandwidthStats>();
    }
}

void BandwidthTelemetry::UnregisterBot(ObjectGuid botGuid)
{
    ::std::unique_lock lock(_botMutex);
    _botStats.erase(botGuid);
}

// ============================================================================
// QUERIES
// ============================================================================

BotBandwidthStats const* BandwidthTelemetry::GetBotStats(ObjectGuid botGuid) const
{
    ::std::shared_lock lock(_botMutex);
    auto it = _botStats.find(botGuid);
    if (it != _botStats.end())
        return it->second.get();
    return nullptr;
}

BandwidthSummary BandwidthTelemetry::GetSummary() const
{
    BandwidthSummary summary;

    // Aggregate per-bot stats
    {
        ::std::shared_lock lock(_botMutex);
        summary.activeBotCount = static_cast<uint32>(_botStats.size());

        for (auto const& [guid, stats] : _botStats)
        {
            summary.totalPacketsIn += stats->packetsIn.load(::std::memory_order_relaxed);
            summary.totalPacketsOut += stats->packetsOut.load(::std::memory_order_relaxed);
            summary.totalBytesIn += stats->bytesIn.load(::std::memory_order_relaxed);
            summary.totalBytesOut += stats->bytesOut.load(::std::memory_order_relaxed);
            summary.totalPacketsFiltered += stats->packetsFiltered.load(::std::memory_order_relaxed);
            summary.totalBytesFiltered += stats->bytesFiltered.load(::std::memory_order_relaxed);
        }
    }

    if (summary.activeBotCount > 0)
    {
        uint64 totalPackets = summary.totalPacketsIn + summary.totalPacketsOut;
        uint64 totalBytes = summary.totalBytesIn + summary.totalBytesOut;
        summary.avgPacketsPerBot = static_cast<float>(totalPackets) / summary.activeBotCount;
        summary.avgBytesPerBot = static_cast<float>(totalBytes) / summary.activeBotCount;

        uint64 totalPossibleBytes = totalBytes + summary.totalBytesFiltered;
        if (totalPossibleBytes > 0)
        {
            summary.filterSavingsPercent =
                static_cast<float>(summary.totalBytesFiltered) / totalPossibleBytes * 100.0f;
        }
    }

    // Get top opcodes
    {
        ::std::shared_lock lock(_opcodeMutex);
        ::std::vector<BandwidthSummary::OpcodeEntry> entries;
        entries.reserve(_opcodeStats.size());

        for (auto const& [opcode, stats] : _opcodeStats)
        {
            BandwidthSummary::OpcodeEntry entry;
            entry.opcode = opcode;
            entry.count = stats->count.load(::std::memory_order_relaxed);
            entry.bytes = stats->totalBytes.load(::std::memory_order_relaxed);
            entries.push_back(entry);
        }

        // Sort by bytes descending
        ::std::sort(entries.begin(), entries.end(),
            [](auto const& a, auto const& b) { return a.bytes > b.bytes; });

        // Take top 10
        size_t limit = ::std::min<size_t>(entries.size(), 10);
        summary.topOpcodes.assign(entries.begin(), entries.begin() + limit);
    }

    return summary;
}

uint32 BandwidthTelemetry::GetTrackedBotCount() const
{
    ::std::shared_lock lock(_botMutex);
    return static_cast<uint32>(_botStats.size());
}

::std::string BandwidthTelemetry::FormatReport() const
{
    BandwidthSummary summary = GetSummary();

    ::std::ostringstream oss;
    oss << "=== Bandwidth Telemetry Report ===\n";
    oss << "  Active Bots: " << summary.activeBotCount << "\n";
    oss << "  Packets In:  " << summary.totalPacketsIn << "\n";
    oss << "  Packets Out: " << summary.totalPacketsOut << "\n";
    oss << "  Bytes In:    " << FormatBytes(summary.totalBytesIn) << "\n";
    oss << "  Bytes Out:   " << FormatBytes(summary.totalBytesOut) << "\n";
    oss << "  Filtered:    " << summary.totalPacketsFiltered << " packets ("
        << FormatBytes(summary.totalBytesFiltered) << " saved)\n";
    oss << ::std::fixed << ::std::setprecision(1);
    oss << "  Filter Savings: " << summary.filterSavingsPercent << "%\n";
    oss << "  Avg Packets/Bot: " << summary.avgPacketsPerBot << "\n";
    oss << "  Avg Bytes/Bot: " << FormatBytes(static_cast<uint64>(summary.avgBytesPerBot)) << "\n";

    if (!summary.topOpcodes.empty())
    {
        oss << "  Top Opcodes by Volume:\n";
        for (auto const& entry : summary.topOpcodes)
        {
            oss << "    0x" << ::std::hex << ::std::setw(6) << ::std::setfill('0')
                << entry.opcode << ::std::dec
                << " - " << entry.count << " packets, "
                << FormatBytes(entry.bytes) << "\n";
        }
    }

    return oss.str();
}

::std::vector<::std::pair<ObjectGuid, uint64>> BandwidthTelemetry::GetTopBotsByBandwidth(
    uint32 count) const
{
    ::std::shared_lock lock(_botMutex);

    ::std::vector<::std::pair<ObjectGuid, uint64>> result;
    result.reserve(_botStats.size());

    for (auto const& [guid, stats] : _botStats)
    {
        result.emplace_back(guid, stats->GetTotalBytes());
    }

    ::std::sort(result.begin(), result.end(),
        [](auto const& a, auto const& b) { return a.second > b.second; });

    if (result.size() > count)
        result.resize(count);

    return result;
}

// ============================================================================
// MAINTENANCE
// ============================================================================

void BandwidthTelemetry::ResetAll()
{
    {
        ::std::unique_lock lock(_botMutex);
        for (auto& [guid, stats] : _botStats)
            stats->Reset();
    }
    {
        ::std::unique_lock lock(_opcodeMutex);
        _opcodeStats.clear();
    }
    TC_LOG_INFO("module.playerbot", "BandwidthTelemetry: All counters reset");
}

void BandwidthTelemetry::ResetBot(ObjectGuid botGuid)
{
    ::std::shared_lock lock(_botMutex);
    auto it = _botStats.find(botGuid);
    if (it != _botStats.end())
        it->second->Reset();
}

void BandwidthTelemetry::Cleanup()
{
    // Remove entries with zero traffic (stale bots)
    ::std::unique_lock lock(_botMutex);
    uint32 removed = 0;

    for (auto it = _botStats.begin(); it != _botStats.end();)
    {
        if (it->second->GetTotalPackets() == 0)
        {
            it = _botStats.erase(it);
            ++removed;
        }
        else
        {
            ++it;
        }
    }

    if (removed > 0)
    {
        TC_LOG_DEBUG("module.playerbot", "BandwidthTelemetry: Cleaned up {} stale entries",
            removed);
    }
}

// ============================================================================
// HELPERS
// ============================================================================

::std::string BandwidthTelemetry::FormatBytes(uint64 bytes)
{
    ::std::ostringstream oss;
    oss << ::std::fixed << ::std::setprecision(1);

    if (bytes >= 1073741824ULL) // >= 1 GB
        oss << (static_cast<double>(bytes) / 1073741824.0) << " GB";
    else if (bytes >= 1048576ULL) // >= 1 MB
        oss << (static_cast<double>(bytes) / 1048576.0) << " MB";
    else if (bytes >= 1024ULL) // >= 1 KB
        oss << (static_cast<double>(bytes) / 1024.0) << " KB";
    else
        oss << bytes << " B";

    return oss.str();
}

} // namespace Playerbot
