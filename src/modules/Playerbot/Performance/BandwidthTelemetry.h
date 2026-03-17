/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * BANDWIDTH TELEMETRY
 *
 * Tracks network bandwidth consumption by bot sessions to identify
 * packet-heavy bots and optimize network usage. Since bots don't have
 * real network connections, they still generate server-side packet
 * processing overhead. This telemetry helps identify:
 *
 *   - Bots generating excessive update packets
 *   - Packet types dominating bandwidth
 *   - Per-zone and per-activity bandwidth patterns
 *   - Opportunities for packet batching/deferral
 *   - Impact of packet filtering optimizations
 *
 * Architecture:
 *   - Singleton collecting metrics from all bot sessions
 *   - Per-bot counters (packets sent/received, bytes)
 *   - Per-opcode breakdown for identifying hot opcodes
 *   - Rolling window averages for trend detection
 *   - Thread-safe via sharded counters (per-bot, no global lock)
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <vector>

namespace Playerbot
{

// ============================================================================
// PACKET DIRECTION
// ============================================================================

enum class PacketDirection : uint8
{
    INBOUND     = 0,    // Client -> Server (simulated for bots)
    OUTBOUND    = 1     // Server -> Client (sent to bot session)
};

// ============================================================================
// PER-BOT BANDWIDTH STATS
// ============================================================================

struct BotBandwidthStats
{
    ::std::atomic<uint64> packetsIn{0};
    ::std::atomic<uint64> packetsOut{0};
    ::std::atomic<uint64> bytesIn{0};
    ::std::atomic<uint64> bytesOut{0};
    ::std::atomic<uint64> packetsFiltered{0};   // Packets dropped by filter
    ::std::atomic<uint64> bytesFiltered{0};     // Bytes saved by filtering

    void Reset()
    {
        packetsIn.store(0, ::std::memory_order_relaxed);
        packetsOut.store(0, ::std::memory_order_relaxed);
        bytesIn.store(0, ::std::memory_order_relaxed);
        bytesOut.store(0, ::std::memory_order_relaxed);
        packetsFiltered.store(0, ::std::memory_order_relaxed);
        bytesFiltered.store(0, ::std::memory_order_relaxed);
    }

    uint64 GetTotalPackets() const
    {
        return packetsIn.load(::std::memory_order_relaxed) +
               packetsOut.load(::std::memory_order_relaxed);
    }

    uint64 GetTotalBytes() const
    {
        return bytesIn.load(::std::memory_order_relaxed) +
               bytesOut.load(::std::memory_order_relaxed);
    }
};

// ============================================================================
// PER-OPCODE STATS
// ============================================================================

struct OpcodeStats
{
    ::std::atomic<uint64> count{0};
    ::std::atomic<uint64> totalBytes{0};

    void Record(uint32 bytes)
    {
        count.fetch_add(1, ::std::memory_order_relaxed);
        totalBytes.fetch_add(bytes, ::std::memory_order_relaxed);
    }
};

// ============================================================================
// GLOBAL BANDWIDTH SUMMARY
// ============================================================================

struct BandwidthSummary
{
    uint64 totalPacketsIn{0};
    uint64 totalPacketsOut{0};
    uint64 totalBytesIn{0};
    uint64 totalBytesOut{0};
    uint64 totalPacketsFiltered{0};
    uint64 totalBytesFiltered{0};
    uint32 activeBotCount{0};
    float avgPacketsPerBot{0.0f};
    float avgBytesPerBot{0.0f};
    float filterSavingsPercent{0.0f};

    // Top opcodes by volume
    struct OpcodeEntry
    {
        uint32 opcode{0};
        uint64 count{0};
        uint64 bytes{0};
    };
    ::std::vector<OpcodeEntry> topOpcodes;
};

// ============================================================================
// BANDWIDTH TELEMETRY (SINGLETON)
// ============================================================================

class BandwidthTelemetry
{
public:
    static BandwidthTelemetry& Instance();

    // ========================================================================
    // RECORDING
    // ========================================================================

    /// Record a packet sent/received for a bot
    void RecordPacket(ObjectGuid botGuid, PacketDirection direction,
                      uint32 opcode, uint32 sizeBytes);

    /// Record a packet that was filtered (not sent)
    void RecordFilteredPacket(ObjectGuid botGuid, uint32 opcode,
                               uint32 sizeBytes);

    // ========================================================================
    // BOT LIFECYCLE
    // ========================================================================

    /// Register a new bot for tracking
    void RegisterBot(ObjectGuid botGuid);

    /// Unregister a bot (cleanup)
    void UnregisterBot(ObjectGuid botGuid);

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Get bandwidth stats for a specific bot
    BotBandwidthStats const* GetBotStats(ObjectGuid botGuid) const;

    /// Get global bandwidth summary
    BandwidthSummary GetSummary() const;

    /// Get number of tracked bots
    uint32 GetTrackedBotCount() const;

    /// Get formatted report
    ::std::string FormatReport() const;

    /// Get top N bots by bandwidth consumption
    ::std::vector<::std::pair<ObjectGuid, uint64>> GetTopBotsByBandwidth(
        uint32 count = 10) const;

    // ========================================================================
    // MAINTENANCE
    // ========================================================================

    /// Reset all counters
    void ResetAll();

    /// Reset counters for a specific bot
    void ResetBot(ObjectGuid botGuid);

    /// Cleanup stale entries
    void Cleanup();

private:
    BandwidthTelemetry() = default;
    ~BandwidthTelemetry() = default;
    BandwidthTelemetry(BandwidthTelemetry const&) = delete;
    BandwidthTelemetry& operator=(BandwidthTelemetry const&) = delete;

    // Per-bot stats (sharded by GUID for low contention)
    mutable ::std::shared_mutex _botMutex;
    ::std::unordered_map<ObjectGuid, ::std::unique_ptr<BotBandwidthStats>> _botStats;

    // Per-opcode stats (global)
    mutable ::std::shared_mutex _opcodeMutex;
    ::std::unordered_map<uint32, ::std::unique_ptr<OpcodeStats>> _opcodeStats;

    // Helper
    static ::std::string FormatBytes(uint64 bytes);
};

} // namespace Playerbot
