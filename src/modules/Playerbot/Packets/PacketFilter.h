/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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

#ifndef _PLAYERBOT_PACKET_FILTER_H
#define _PLAYERBOT_PACKET_FILTER_H

#include "Define.h"
#include "Opcodes.h"
#include <unordered_set>
#include <unordered_map>

class WorldSession;
class WorldPacket;

namespace Playerbot
{

/**
 * @brief Packet filtering and routing for bot-safe operations
 *
 * Purpose: Ensures bot-generated packets don't interfere with player clients
 * and routes packets correctly through the TrinityCore packet handling system.
 *
 * Architecture:
 * - Whitelist-based opcode filtering (deny by default)
 * - Priority-based packet processing (resurrection > combat > buffs)
 * - Integration with BotSession::Update() for main thread processing
 *
 * Performance:
 * - Opcode lookup: O(1) hash table lookup
 * - Priority lookup: O(1) hash table lookup
 * - Total overhead: <0.01ms per packet
 *
 * Thread Safety:
 * - All methods are const and use const static data
 * - Safe to call from both worker threads and main thread
 *
 * @author TrinityCore Playerbot Integration Team
 * @date 2025-10-30
 * @version 1.0.0 (Phase 0 - Week 2)
 */
class TC_GAME_API PacketFilter
{
public:
    /**
     * @brief Check if packet should be processed for this session
     *
     * Applies filtering rules to determine if packet is safe for bot processing.
     *
     * @param session WorldSession (bot or player)
     * @param opcode Client opcode from packet
     * @param packet Raw packet data (for future content-based filtering)
     * @return true if packet should be processed, false if filtered
     *
     * Example usage:
     * @code
     * if (PacketFilter::ShouldProcessPacket(botSession, opcode, *packet))
     *     ProcessPacket(packet);
     * else
     *     TC_LOG_DEBUG("playerbot.packets", "Filtered opcode {}", opcode);
     * @endcode
     */
    static bool ShouldProcessPacket(
        WorldSession const* session,
        OpcodeClient opcode,
        WorldPacket const& packet);

    /**
     * @brief Check if opcode is safe for bot usage
     *
     * Validates against whitelist of bot-safe opcodes. Opcodes not in whitelist
     * are blocked to prevent bots from triggering unintended game mechanics.
     *
     * @param opcode Client opcode to validate
     * @return true if opcode is in bot-safe whitelist
     *
     * Example usage:
     * @code
     * if (!PacketFilter::IsBotSafeOpcode(opcode))
     * {
     *     TC_LOG_ERROR("playerbot.packets", "Unsafe opcode {} blocked", opcode);
     *     return;
     * }
     * @endcode
     */
    static bool IsBotSafeOpcode(OpcodeClient opcode);

    /**
     * @brief Get packet processing priority
     *
     * Returns priority value for packet queue ordering:
     * - 0 = Highest priority (resurrection, death recovery)
     * - 10 = Normal priority (combat spells)
     * - 50 = Low priority (buff management)
     * - 100 = Lowest priority (default for unlisted opcodes)
     *
     * @param opcode Client opcode
     * @return Priority value (lower = higher priority)
     *
     * Example usage (future priority queue):
     * @code
     * uint8 priority = PacketFilter::GetPacketPriority(opcode);
     * _priorityQueue.Push(packet, priority);
     * @endcode
     */
    static uint8 GetPacketPriority(OpcodeClient opcode);

    /**
     * @brief Get human-readable opcode name for logging
     *
     * @param opcode Client opcode
     * @return Opcode name string (e.g., "CMSG_CAST_SPELL")
     */
    static char const* GetOpcodeName(OpcodeClient opcode);

    /**
     * @brief Get statistics on filtered packets
     *
     * @return Total number of packets filtered since server start
     */
    static uint64 GetTotalFiltered();

    /**
     * @brief Get statistics on processed packets
     *
     * @return Total number of packets processed since server start
     */
    static uint64 GetTotalProcessed();

private:
    /**
     * @brief Whitelist of bot-safe opcodes
     *
     * Only opcodes in this set are allowed for bot usage. This prevents bots
     * from accidentally triggering admin commands, economy exploits, or other
     * unintended game mechanics.
     *
     * Current whitelist (Phase 0):
     * - CMSG_CAST_SPELL - Spell casting
     * - CMSG_CANCEL_CAST - Cancel spell cast
     * - CMSG_CANCEL_AURA - Remove buff/debuff
     * - CMSG_CANCEL_CHANNELLING - Stop channeling
     * - CMSG_CANCEL_AUTO_REPEAT_SPELL - Stop wand/auto-shot
     * - CMSG_RECLAIM_CORPSE - Resurrect at corpse
     * - CMSG_REPOP_REQUEST - Release spirit
     * - CMSG_MOVE_TELEPORT_ACK - Acknowledge teleport (resurrection)
     *
     * Future additions (Phase 1+):
     * - CMSG_SETACTIONBUTTON - Action bar management
     * - CMSG_SWAP_INV_ITEM - Inventory management
     * - CMSG_AUTOEQUIP_ITEM - Auto-equip items
     * - CMSG_ACCEPT_TRADE - Trading
     * - CMSG_QUEST_GIVER_ACCEPT_QUEST - Quest acceptance
     */
    static ::std::unordered_set<OpcodeClient> const _botSafeOpcodes;

    /**
     * @brief Priority mapping for packet processing
     *
     * Lower values = higher priority (processed first)
     *
     * Priority Tiers:
     * - 0-9: Critical (resurrection, death recovery)
     * - 10-19: High (combat spells, interrupts)
     * - 20-49: Normal (movement, targeting)
     * - 50-99: Low (buff management, inventory)
     * - 100: Default (unlisted opcodes)
     */
    static ::std::unordered_map<OpcodeClient, uint8> const _opcodePriorities;

    // Statistics (mutable for const methods)
    static ::std::atomic<uint64> _totalFiltered;
    static ::std::atomic<uint64> _totalProcessed;
};

} // namespace Playerbot

#endif // _PLAYERBOT_PACKET_FILTER_H
