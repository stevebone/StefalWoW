/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_PACKET_SNIFFER_H
#define PLAYERBOT_PACKET_SNIFFER_H

#include "Define.h"
#include <atomic>
#include <unordered_map>
#include <functional>
#include <array>
#include <chrono>
#include <typeindex>

class WorldSession;
class WorldPacket;

namespace Playerbot
{

// Forward declarations for typed packet handler registration
void RegisterGroupPacketHandlers();
void RegisterCombatPacketHandlers();
void RegisterCooldownPacketHandlers();
void RegisterAuraPacketHandlers();
void RegisterLootPacketHandlers();
void RegisterQuestPacketHandlers();
void RegisterResourcePacketHandlers();
void RegisterSocialPacketHandlers();
void RegisterAuctionPacketHandlers();
void RegisterNPCPacketHandlers();
void RegisterInstancePacketHandlers();
void RegisterBattlegroundPacketHandlers();
void RegisterLFGPacketHandlers();

/**
 * @enum PacketCategory
 * @brief Categorizes packets for specialized parsing
 */
enum class PacketCategory : uint8
{
    GROUP       = 0,   // Ready check, icons, markers, party updates
    COMBAT      = 1,   // Spell casts, damage, interrupts, combat log
    COOLDOWN    = 2,   // Spell/item cooldowns, resets
    LOOT        = 3,   // Loot rolls, money, items
    QUEST       = 4,   // Quest dialogue, progress, rewards
    AURA        = 5,   // Buffs, debuffs, dispels
    RESOURCE    = 6,   // Mana, health, power updates
    SOCIAL      = 7,   // Chat, guild, trade
    AUCTION     = 8,   // AH operations
    NPC         = 9,   // Gossip, vendors, trainers
    INSTANCE    = 10,  // Dungeon, raid, scenario
    BATTLEGROUND = 11, // BG/Arena queue, status, invitations
    LFG         = 12,  // LFG queue, proposals, roles
    UNKNOWN     = 13,  // Uncategorized packets

    MAX_CATEGORY
};

/**
 * @class PlayerbotPacketSniffer
 * @brief Centralized packet interception system for playerbot event detection
 *
 * This class provides a single integration point for intercepting network packets
 * sent to bot players, enabling real-time event detection for systems that lack
 * public TrinityCore APIs.
 *
 * **Architecture (WoW 12.0 Typed Packet Pipeline)**:
 * ```
 * WorldSession::SendPacket<T>() → OnTypedPacket<T>() → TypedPacketHandler
 *                                                              ↓
 *                                                    Specialized Event Buses
 *                                                              ↓
 *                                                    BotAI Event Subscribers
 * ```
 *
 * Typed handlers are registered per-category in Parse*Packet_Typed.cpp files
 * via RegisterTypedHandler<T>(), enabling full access to strongly-typed packet
 * data before serialization.
 *
 * The legacy OnPacketSend() opcode-based path is retained for stats tracking only.
 *
 * **Design Principles**:
 * - Typed packet interception (full data access before serialization)
 * - Zero impact on real players (bot-only processing)
 * - Performance optimized (<0.01% CPU per bot)
 * - Thread-safe packet processing
 *
 * @note Only processes packets for bots (zero overhead for real players)
 */
class TC_GAME_API PlayerbotPacketSniffer
{
public:
    /**
     * Main packet interception hook
     * Called from WorldSession::SendPacket() for bot players only
     *
     * @param session Bot's world session
     * @param packet The packet being sent to the bot
     */
    static void OnPacketSend(WorldSession* session, WorldPacket const& packet);

    /**
     * Typed packet interception hook (WoW 12.0 Solution)
     * Called from WorldSession::SendPacket<PacketType>() template overload
     * Receives typed packet BEFORE serialization, enabling full data access
     *
     * @param session Bot's world session
     * @param typedPacket The strongly-typed packet object
     */
    template<typename PacketType>
    static void OnTypedPacket(WorldSession* session, PacketType const& typedPacket);

    /**
     * Initialize the packet sniffer
     * Called during module initialization
     */
    static void Initialize();

    /**
     * Shutdown the packet sniffer
     * Called during module shutdown
     */
    static void Shutdown();

    /**
     * Performance statistics
     */
    struct Statistics
    {
        uint64_t totalPacketsProcessed;
        ::std::array<uint64_t, static_cast<uint8>(PacketCategory::MAX_CATEGORY)> packetsPerCategory;
        uint64_t avgProcessTimeUs;
        uint64_t peakProcessTimeUs;
        ::std::chrono::steady_clock::time_point startTime;

        ::std::string ToString() const;
        void Reset();
    };

    /**
     * Get current statistics
     * @return Current performance statistics
     */
    static Statistics GetStatistics();

    /**
     * Dump statistics to log
     */
    static void DumpStatistics();

private:
    static inline bool _initialized = false;

    // Performance tracking
    static inline ::std::atomic<uint64_t> _totalPackets{0};
    static inline ::std::array<::std::atomic<uint64_t>, static_cast<uint8>(PacketCategory::MAX_CATEGORY)> _categoryPackets{};
    static inline ::std::atomic<uint64_t> _totalProcessTimeUs{0};
    static inline ::std::atomic<uint64_t> _peakProcessTimeUs{0};
    static inline ::std::chrono::steady_clock::time_point _startTime;

    // Typed packet handlers (WoW 12.0)
    using TypedPacketHandler = ::std::function<void(WorldSession*, void const*)>;
    static inline ::std::unordered_map<::std::type_index, TypedPacketHandler> _typedPacketHandlers;

    template<typename PacketType>
    static void RegisterTypedHandler(void (*handler)(WorldSession*, PacketType const&));

    // Register category-specific typed handlers (defined in Parse*Packet_Typed.cpp files)
    friend void RegisterGroupPacketHandlers();
    friend void RegisterCombatPacketHandlers();
    friend void RegisterCooldownPacketHandlers();
    friend void RegisterAuraPacketHandlers();
    friend void RegisterLootPacketHandlers();
    friend void RegisterQuestPacketHandlers();
    friend void RegisterResourcePacketHandlers();
    friend void RegisterSocialPacketHandlers();
    friend void RegisterAuctionPacketHandlers();
    friend void RegisterNPCPacketHandlers();
    friend void RegisterInstancePacketHandlers();
    friend void RegisterBattlegroundPacketHandlers();
    friend void RegisterLFGPacketHandlers();
};

// Template implementation (must be in header)
template<typename PacketType>
void PlayerbotPacketSniffer::OnTypedPacket(WorldSession* session, PacketType const& typedPacket)
{
    if (!_initialized || !session)
        return;

    // Dispatch to registered typed handler
    ::std::type_index typeIdx(typeid(PacketType));
    auto it = _typedPacketHandlers.find(typeIdx);
    if (it != _typedPacketHandlers.end())
        it->second(session, &typedPacket);
}

template<typename PacketType>
void PlayerbotPacketSniffer::RegisterTypedHandler(void (*handler)(WorldSession*, PacketType const&))
{
    _typedPacketHandlers[::std::type_index(typeid(PacketType))] =
        [handler](WorldSession* session, void const* packet) {
            handler(session, *static_cast<PacketType const*>(packet));
        };
}

} // namespace Playerbot

#endif // PLAYERBOT_PACKET_SNIFFER_H
