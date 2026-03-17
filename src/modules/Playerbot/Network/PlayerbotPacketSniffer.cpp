/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotPacketSniffer.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Player.h"
#include "PlayerBotHooks.h"
#include "Log.h"
#include <sstream>
#include <iomanip>

namespace Playerbot
{

// Static member definitions moved to header with inline to fix DLL linkage (C2491)

void PlayerbotPacketSniffer::Initialize()
{
    if (_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "PlayerbotPacketSniffer: Initializing typed packet interception system...");

    // Register typed packet handlers (WoW 12.0 - all packet handling via typed handlers)
    RegisterGroupPacketHandlers();
    RegisterCombatPacketHandlers();
    RegisterCooldownPacketHandlers();
    RegisterAuraPacketHandlers();
    RegisterLootPacketHandlers();
    RegisterQuestPacketHandlers();
    RegisterResourcePacketHandlers();
    RegisterSocialPacketHandlers();
    RegisterAuctionPacketHandlers();
    RegisterNPCPacketHandlers();
    RegisterInstancePacketHandlers();

    // Initialize statistics
    _totalPackets.store(0);
    for (auto& counter : _categoryPackets)
        counter.store(0);
    _totalProcessTimeUs.store(0);
    _peakProcessTimeUs.store(0);
    _startTime = ::std::chrono::steady_clock::now();

    _initialized = true;

    TC_LOG_INFO("module.playerbot", "PlayerbotPacketSniffer: Initialized with {} typed handlers",
        _typedPacketHandlers.size());
}

void PlayerbotPacketSniffer::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "PlayerbotPacketSniffer: Shutting down...");

    DumpStatistics();

    _typedPacketHandlers.clear();
    _initialized = false;

    TC_LOG_INFO("module.playerbot", "PlayerbotPacketSniffer: Shutdown complete");
}

void PlayerbotPacketSniffer::OnPacketSend(WorldSession* session, WorldPacket const& packet)
{
    // Legacy opcode-based entry point. All packet handling now occurs through
    // OnTypedPacket<T>() which receives strongly-typed packets before serialization.
    // This method is retained as a public API entry point but only tracks stats.

    if (!_initialized || !session)
        return;

    Player* player = session->GetPlayer();
    if (!player || !PlayerBotHooks::IsPlayerBot(player))
        return;

    _totalPackets.fetch_add(1, ::std::memory_order_relaxed);
}

// Statistics implementation
::std::string PlayerbotPacketSniffer::Statistics::ToString() const
{
    ::std::ostringstream ss;

    ss << "\n=== PlayerbotPacketSniffer Statistics ===\n";
    ss << "Total Packets Processed: " << totalPacketsProcessed << "\n";
    ss << "Average Processing Time: " << avgProcessTimeUs << " μs\n";
    ss << "Peak Processing Time: " << peakProcessTimeUs << " μs\n";

    auto uptime = ::std::chrono::duration_cast<::std::chrono::seconds>(
        ::std::chrono::steady_clock::now() - startTime).count();
    ss << "Uptime: " << uptime << " seconds\n";

    ss << "\nPackets per Category:\n";
    const char* categoryNames[] = {
        "GROUP", "COMBAT", "COOLDOWN", "LOOT", "QUEST", "AURA",
        "RESOURCE", "SOCIAL", "AUCTION", "NPC", "INSTANCE",
        "BATTLEGROUND", "LFG", "UNKNOWN"
    };

    for (uint8 i = 0; i < static_cast<uint8>(PacketCategory::MAX_CATEGORY); ++i)
    {
        if (packetsPerCategory[i] > 0)
        {
            ss << "  " << ::std::setw(12) << ::std::left << categoryNames[i]
               << ": " << ::std::setw(10) << ::std::right << packetsPerCategory[i];

            if (totalPacketsProcessed > 0)
            {
                float percentage = (float)packetsPerCategory[i] / totalPacketsProcessed * 100.0f;
                ss << " (" << ::std::fixed << ::std::setprecision(2) << percentage << "%)";
            }
            ss << "\n";
        }
    }

    return ss.str();
}

void PlayerbotPacketSniffer::Statistics::Reset()
{
    totalPacketsProcessed = 0;
    packetsPerCategory.fill(0);
    avgProcessTimeUs = 0;
    peakProcessTimeUs = 0;
    startTime = ::std::chrono::steady_clock::now();
}

PlayerbotPacketSniffer::Statistics PlayerbotPacketSniffer::GetStatistics()
{
    Statistics stats;
    stats.totalPacketsProcessed = _totalPackets.load(::std::memory_order_relaxed);

    for (uint8 i = 0; i < static_cast<uint8>(PacketCategory::MAX_CATEGORY); ++i)
        stats.packetsPerCategory[i] = _categoryPackets[i].load(::std::memory_order_relaxed);

    uint64_t totalTime = _totalProcessTimeUs.load(::std::memory_order_relaxed);
    stats.avgProcessTimeUs = stats.totalPacketsProcessed > 0 ?
        totalTime / stats.totalPacketsProcessed : 0;

    stats.peakProcessTimeUs = _peakProcessTimeUs.load(::std::memory_order_relaxed);
    stats.startTime = _startTime;

    return stats;
}

void PlayerbotPacketSniffer::DumpStatistics()
{
    Statistics stats = GetStatistics();
    TC_LOG_INFO("module.playerbot", "{}", stats.ToString());
}

} // namespace Playerbot
