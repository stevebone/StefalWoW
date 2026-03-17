/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * PHASE 1: Bot Packet Relay Infrastructure
 *
 * This system provides packet relay functionality to forward bot packets
 * to human players in the same group. Critical for:
 * - Combat log visibility (bot damage appearing in party frames)
 * - Party chat bidirectionality
 * - Party member state updates (health/mana/position)
 * - Emotes and social interactions
 *
 * Architecture:
 * - Module-only implementation (no core modifications)
 * - Uses TrinityCore's Group::BroadcastPacket() pattern
 * - Thread-safe packet queue management
 * - Whitelist-based packet filtering
 * - Performance optimized (<0.1% CPU overhead)
 *
 * Integration Points:
 * - BotSession::SendPacket() - Intercepts outgoing bot packets
 * - Group system - Enumerates human players for relay
 * - WorldSession::SendDirectMessage() - Delivers to humans
 */

#ifndef PLAYERBOT_BOT_PACKET_RELAY_H
#define PLAYERBOT_BOT_PACKET_RELAY_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>

class Player;
class Group;
class WorldPacket;
class WorldSession;

namespace Playerbot {

class BotSession;

/**
 * @brief Packet relay system for forwarding bot packets to human players
 *
 * Core functionality:
 * 1. Intercepts packets from BotSession::SendPacket()
 * 2. Filters packets based on whitelist (combat log, chat, party updates)
 * 3. Finds human players in bot's group
 * 4. Relays packets to human players via Player::SendDirectMessage()
 *
 * Thread Safety:
 * - All methods are thread-safe
 * - Uses std::recursive_mutex for packet queue protection
 * - Atomic flags for initialization state
 *
 * Performance:
 * - Early exit for non-group bots (O(1))
 * - Whitelist lookup is O(1) hash set
 * - No packet copying unless necessary
 * - Typical overhead: <0.1% CPU per bot
 */
class TC_GAME_API BotPacketRelay
{
public:
    // ========================================================================
    // INITIALIZATION & LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize packet relay system
     *
     * Call during module startup (PlayerbotModule::OnStartup)
     * Registers packet opcodes for relay, initializes whitelist
     */
    static void Initialize();

    /**
     * @brief Shutdown packet relay system
     *
     * Call during module shutdown (PlayerbotModule::OnShutdown)
     * Cleans up resources, flushes pending packets
     */
    static void Shutdown();

    /**
     * @brief Check if relay system is initialized
     * @return true if initialized and ready for use
     */
    static bool IsInitialized() { return _initialized.load(); }

    // ========================================================================
    // CORE RELAY FUNCTIONALITY
    // ========================================================================

    /**
     * @brief Relay packet from bot to human group members
     *
     * Main entry point for packet relay. Called from BotSession::SendPacket()
     *
     * Algorithm:
     * 1. Check if packet should be relayed (whitelist)
     * 2. Get bot's player object
     * 3. Find human players in bot's group
     * 4. Send packet to each human via SendDirectMessage()
     *
     * @param botSession Bot session that generated the packet
     * @param packet Packet to relay (not modified)
     *
     * Thread Safety: Full thread safety, can be called from any thread
     * Performance: O(n) where n = number of human players in group
     */
    static void RelayToGroupMembers(BotSession* botSession, WorldPacket const* packet);

    /**
     * @brief Relay packet to specific player
     *
     * Used for direct communication (whispers, targeted emotes, etc.)
     *
     * @param botSession Bot session sending the packet
     * @param packet Packet to relay
     * @param targetPlayer Target player to receive packet
     */
    static void RelayToPlayer(BotSession* botSession, WorldPacket const* packet, Player* targetPlayer);

    /**
     * @brief Relay packet to all players in bot's group (including other bots)
     *
     * Used for group-wide broadcasts (raid warnings, etc.)
     *
     * @param botSession Bot session sending the packet
     * @param packet Packet to relay
     * @param ignoreBot If true, don't send to the originating bot
     */
    static void BroadcastToGroup(BotSession* botSession, WorldPacket const* packet, bool ignoreBot = true);

    // ========================================================================
    // PACKET FILTERING
    // ========================================================================

    /**
     * @brief Check if packet should be relayed
     *
     * Uses whitelist of opcodes that are safe/useful to relay:
     * - Combat log packets (SMSG_SPELL_DAMAGE_SHIELD, etc.)
     * - Chat packets (SMSG_CHAT, SMSG_TEXT_EMOTE)
     * - Party packets (SMSG_PARTY_MEMBER_FULL_STATE, etc.)
     * - Social packets (emotes, etc.)
     *
     * @param packet Packet to check
     * @return true if packet should be relayed
     *
     * Performance: O(1) hash set lookup
     */
    static bool ShouldRelayPacket(WorldPacket const* packet);

    /**
     * @brief Check if specific opcode should be relayed
     * @param opcode Packet opcode to check
     * @return true if opcode is in relay whitelist
     */
    static bool ShouldRelayOpcode(uint32 opcode);

    /**
     * @brief Add opcode to relay whitelist
     * @param opcode Opcode to add
     *
     * Used for dynamic whitelist expansion
     */
    static void AddRelayOpcode(uint32 opcode);

    /**
     * @brief Remove opcode from relay whitelist
     * @param opcode Opcode to remove
     */
    static void RemoveRelayOpcode(uint32 opcode);

    /**
     * @brief Get all relay opcodes
     * @return Set of all opcodes that are relayed
     */
    static ::std::unordered_set<uint32> const& GetRelayOpcodes();

    // ========================================================================
    // GROUP MEMBER ENUMERATION
    // ========================================================================

    /**
     * @brief Get all human players in bot's group
     *
     * Filters group members to only return human players (not bots)
     * Used by relay system to determine packet recipients
     *
     * @param bot Bot player
     * @return Vector of human players in bot's group
     *
     * Performance: O(n) where n = group size (max 40)
     */
    static ::std::vector<Player*> GetHumanGroupMembers(Player* bot);

    /**
     * @brief Get all players in bot's group (including bots)
     * @param bot Bot player
     * @return Vector of all players in bot's group
     */
    static ::std::vector<Player*> GetAllGroupMembers(Player* bot);

    /**
     * @brief Check if player is a bot
     * @param player Player to check
     * @return true if player is a bot
     */
    static bool IsBot(Player const* player);

    /**
     * @brief Get bot's group
     * @param bot Bot player
     * @return Group pointer or nullptr if not in group
     */
    static Group* GetBotGroup(Player* bot);

    // ========================================================================
    // GROUP INTEGRATION
    // ========================================================================

    /**
     * @brief Initialize packet relay for bot when joining group
     *
     * Called from PlayerbotGroupScript::OnAddMember()
     * Sets up relay state, sends initial party updates
     *
     * @param bot Bot joining group
     * @param group Group being joined
     */
    static void InitializeForGroup(Player* bot, Group* group);

    /**
     * @brief Cleanup packet relay for bot when leaving group
     *
     * Called from PlayerbotGroupScript::OnRemoveMember()
     * Flushes pending packets, cleans up state
     *
     * @param bot Bot leaving group
     * @param group Group being left
     */
    static void CleanupForGroup(Player* bot, Group* group);

    // ========================================================================
    // STATISTICS & DEBUGGING
    // ========================================================================

    /**
     * @brief Statistics tracking for packet relay
     */
    struct RelayStatistics
    {
        ::std::atomic<uint64_t> totalPacketsRelayed;
        ::std::atomic<uint64_t> totalPacketsFiltered;
        ::std::atomic<uint64_t> totalRelayErrors;
        ::std::atomic<uint64_t> combatLogPackets;
        ::std::atomic<uint64_t> chatPackets;
        ::std::atomic<uint64_t> partyUpdatePackets;
        ::std::atomic<uint64_t> emotePackets;
    };

    /**
     * @brief Get relay statistics
     * @return Current statistics
     */
    static RelayStatistics const& GetStatistics();

    /**
     * @brief Reset statistics counters
     */
    static void ResetStatistics();

    /**
     * @brief Enable/disable detailed relay logging
     * @param enabled true to enable debug logging
     */
    static void SetDebugLogging(bool enabled);

    /**
     * @brief Check if debug logging is enabled
     * @return true if debug logging enabled
     */
    static bool IsDebugLoggingEnabled() { return _debugLogging.load(); }

private:
    // ========================================================================
    // INTERNAL IMPLEMENTATION
    // ========================================================================

    /**
     * @brief Initialize relay opcode whitelist
     *
     * Populates _relayOpcodes with combat log, chat, party update opcodes
     * Called during Initialize()
     */
    static void InitializeOpcodeWhitelist();

    /**
     * @brief Send packet to specific player with error handling
     * @param player Target player
     * @param packet Packet to send
     * @return true if successfully sent
     */
    static bool SendPacketToPlayer(Player* player, WorldPacket const* packet);

    /**
     * @brief Log relay event for debugging
     * @param bot Bot sending packet
     * @param packet Packet being relayed
     * @param recipients Number of recipients
     */
    static void LogRelayEvent(Player* bot, WorldPacket const* packet, size_t recipients);

    /**
     * @brief Update statistics for relayed packet
     * @param opcode Packet opcode
     * @param success true if relay succeeded
     */
    static void UpdateStatistics(uint32 opcode, bool success);

    /**
     * @brief Get packet type for statistics tracking
     * @param opcode Packet opcode
     * @return Packet category string
     */
    static char const* GetPacketCategory(uint32 opcode);

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Initialization state
    static inline ::std::atomic<bool> _initialized{false};

    // Relay opcode whitelist (thread-safe for reads after initialization)
    static inline ::std::unordered_set<uint32> _relayOpcodes;
    static inline Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _opcodesMutex;

    // Statistics
    static inline RelayStatistics _statistics;

    // Debug logging
    static inline ::std::atomic<bool> _debugLogging{false};

    // RACE CONDITION FIX: Deferred packet queue for early relay attempts
    // Packets sent before Initialize() completes are queued here and processed after initialization
    struct DeferredPacket
    {
        BotSession* botSession;
        ::std::unique_ptr<WorldPacket> packet;
        uint32 timestamp; // GameTime::GetGameTimeMS() when queued
    };
    static inline ::std::queue<DeferredPacket> _deferredPackets;
    static inline Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _deferredMutex;

    /**
     * @brief Process deferred packets that were queued before initialization
     * Called at end of Initialize() to replay queued packets
     */
    static void ProcessDeferredPackets();

    // Deleted constructors (static class)
    BotPacketRelay() = delete;
    ~BotPacketRelay() = delete;
    BotPacketRelay(BotPacketRelay const&) = delete;
    BotPacketRelay& operator=(BotPacketRelay const&) = delete;
};

} // namespace Playerbot

#endif // PLAYERBOT_BOT_PACKET_RELAY_H
