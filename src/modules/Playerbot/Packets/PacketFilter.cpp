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

#include "PacketFilter.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Log.h"
#include "Opcodes.h"

namespace Playerbot
{

// Static member initialization
::std::atomic<uint64> PacketFilter::_totalFiltered{0};
::std::atomic<uint64> PacketFilter::_totalProcessed{0};

/**
 * Bot-safe opcode whitelist
 *
 * SECURITY: Only opcodes explicitly listed here can be queued by bots.
 * This prevents bots from triggering admin commands, economy exploits,
 * or other unintended game mechanics.
 *
 * Phase 0 Whitelist (Spell Casting):
 * - Spell casting and cancellation opcodes
 * - Death recovery and resurrection opcodes
 * - Movement acknowledgment for resurrection teleport
 *
 * Future Phases:
 * - Phase 1: Action bar, inventory, equipment
 * - Phase 2: Trading, mail, auction house
 * - Phase 3: Questing, NPCs, gossip
 * - Phase 4: Group, raid, PvP
 */
::std::unordered_set<OpcodeClient> const PacketFilter::_botSafeOpcodes = {
    // Spell casting (Phase 0 - Week 1)
    CMSG_CAST_SPELL,                  // Primary spell casting
    CMSG_CANCEL_CAST,                 // Interrupt current cast
    CMSG_CANCEL_AURA,                 // Remove buff/debuff from self
    CMSG_CANCEL_CHANNELLING,          // Stop channeling spell (Blizzard, Mind Flay, etc.)
    CMSG_CANCEL_AUTO_REPEAT_SPELL,    // Stop auto-attacking spell (Wand, Shoot)

    // Death recovery and resurrection (Phase 0 - Existing)
    CMSG_RECLAIM_CORPSE,              // Resurrect at corpse location
    CMSG_REPOP_REQUEST,               // Release spirit to graveyard
    CMSG_MOVE_TELEPORT_ACK,           // Acknowledge teleport (required for resurrection)

    // Pet spells (Phase 0 - Week 3)
    CMSG_PET_CAST_SPELL,              // Pet spell casting
    CMSG_PET_CANCEL_AURA,             // Pet buff removal

    // Future Phase 1: Action bar and inventory management
    // CMSG_SETACTIONBUTTON,          // Set action bar button
    // CMSG_SWAP_INV_ITEM,            // Swap inventory items
    // CMSG_AUTOEQUIP_ITEM,           // Auto-equip item
    // CMSG_AUTOSTORE_ITEM,           // Auto-store item in bag
    // CMSG_USE_ITEM,                 // Use consumable item

    // Future Phase 2: Social and economy
    // CMSG_ACCEPT_TRADE,             // Accept trade
    // CMSG_INITIATE_TRADE,           // Initiate trade
    // CMSG_MAIL_SEND,                // Send mail
    // CMSG_AUCTION_PLACE_BID,        // Place auction bid

    // Future Phase 3: Questing and world interaction
    // CMSG_QUEST_GIVER_ACCEPT_QUEST, // Accept quest
    // CMSG_QUEST_GIVER_COMPLETE_QUEST, // Complete quest
    // CMSG_GOSSIP_SELECT_OPTION,     // Select gossip option
    // CMSG_NPC_TEXT_QUERY,           // Query NPC text

    // Future Phase 4: Group and PvP
    // CMSG_GROUP_ACCEPT,             // Accept group invite
    // CMSG_BATTLEFIELD_JOIN,         // Join battleground
    // CMSG_ARENA_TEAM_ACCEPT,        // Accept arena team invite
};

/**
 * Opcode priority mapping
 *
 * PERFORMANCE: Lower priority values are processed first in priority queue.
 * This ensures critical packets (resurrection) are never delayed by
 * non-critical packets (buff management).
 *
 * Priority Tiers:
 * - 0-9: CRITICAL - Death recovery, resurrection (must be immediate)
 * - 10-19: HIGH - Combat spells, interrupts (affects combat outcome)
 * - 20-49: NORMAL - Movement, targeting (general gameplay)
 * - 50-99: LOW - Buff management, inventory (can be delayed)
 * - 100: DEFAULT - Unlisted opcodes (lowest priority)
 *
 * Queue Behavior (Future Priority Queue):
 * - Resurrection packet jumps to front of 1000-packet queue
 * - Combat spell processed before buff removal
 * - Inventory management waits until combat packets clear
 */
::std::unordered_map<OpcodeClient, uint8> const PacketFilter::_opcodePriorities = {
    // CRITICAL PRIORITY (0-9) - Death recovery
    {CMSG_RECLAIM_CORPSE, 0},         // Highest - bot stuck until resurrected
    {CMSG_REPOP_REQUEST, 1},          // Very high - release spirit to continue
    {CMSG_MOVE_TELEPORT_ACK, 2},      // High - required for resurrection teleport

    // HIGH PRIORITY (10-19) - Combat effectiveness
    {CMSG_CAST_SPELL, 10},            // Normal combat spell
    {CMSG_PET_CAST_SPELL, 10},        // Pet combat spell
    {CMSG_CANCEL_CAST, 15},           // Interrupt (movement, counterspell target)
    {CMSG_CANCEL_CHANNELLING, 15},    // Stop channeling (movement, new target)

    // LOW PRIORITY (50-99) - Non-critical maintenance
    {CMSG_CANCEL_AURA, 50},           // Buff management (not time-sensitive)
    {CMSG_PET_CANCEL_AURA, 50},       // Pet buff management
    {CMSG_CANCEL_AUTO_REPEAT_SPELL, 50}, // Stop wand/shoot (low impact)

    // Future Phase 1: Inventory and action bars
    // {CMSG_SETACTIONBUTTON, 80},    // Action bar setup (very low priority)
    // {CMSG_SWAP_INV_ITEM, 70},      // Inventory management
    // {CMSG_USE_ITEM, 30},           // Item usage (potions = higher priority)

    // Future Phase 2: Social and economy
    // {CMSG_ACCEPT_TRADE, 60},       // Trade completion
    // {CMSG_MAIL_SEND, 90},          // Mail (async, low priority)

    // DEFAULT: 100 (unlisted opcodes get lowest priority)
};

/**
 * Check if packet should be processed
 *
 * FILTERING LOGIC:
 * 1. Validate session is not nullptr
 * 2. Check if opcode is in bot-safe whitelist
 * 3. [Future] Check session-specific filters (bot vs player)
 * 4. [Future] Content-based filtering (packet data validation)
 *
 * @param session WorldSession (bot or player)
 * @param opcode Client opcode from packet
 * @param packet Raw packet data (for future content filtering)
 * @return true if packet should be processed
 */
bool PacketFilter::ShouldProcessPacket(
    WorldSession const* session,
    OpcodeClient opcode,
    WorldPacket const& packet)
{
    // Validate session
    if (!session)
    {
        TC_LOG_ERROR("playerbot.packets",
            "PacketFilter::ShouldProcessPacket - nullptr session for opcode {}",
            GetOpcodeName(opcode));
        _totalFiltered.fetch_add(1, ::std::memory_order_relaxed);
        return false;
    }

    // Check whitelist
    if (!IsBotSafeOpcode(opcode))
    {
        TC_LOG_DEBUG("playerbot.packets",
            "Filtered unsafe opcode {} for session {} (player {})",
            GetOpcodeName(opcode),
            session->GetAccountId(),
            session->GetPlayerName());
        _totalFiltered.fetch_add(1, ::std::memory_order_relaxed);
        return false;
    }

    // [Future] Session-specific filtering
    // if (session->IsBot() && RequiresPlayerPermission(opcode))
    //     return false;

    // [Future] Content-based filtering
    // if (!ValidatePacketContent(opcode, packet))
    //     return false;

    // Packet passed all filters
    _totalProcessed.fetch_add(1, ::std::memory_order_relaxed);

    TC_LOG_TRACE("playerbot.packets",
        "Allowed opcode {} for session {} (player {})",
        GetOpcodeName(opcode),
        session->GetAccountId(),
        session->GetPlayerName());

    return true;
}

/**
 * Check if opcode is in bot-safe whitelist
 *
 * PERFORMANCE: O(1) hash table lookup
 * THREAD-SAFETY: Reads from const static data (no locking needed)
 *
 * @param opcode Client opcode to validate
 * @return true if opcode is whitelisted
 */
bool PacketFilter::IsBotSafeOpcode(OpcodeClient opcode)
{
    return _botSafeOpcodes.count(opcode) > 0;
}

/**
 * Get packet processing priority
 *
 * PERFORMANCE: O(1) hash table lookup with default fallback
 * THREAD-SAFETY: Reads from const static data (no locking needed)
 *
 * Priority Guide:
 * - 0 = CRITICAL (resurrection) - Process immediately
 * - 10 = HIGH (combat) - Process within 1 update cycle
 * - 50 = LOW (buffs) - Process within 5 update cycles
 * - 100 = DEFAULT (unlisted) - Process when queue empties
 *
 * @param opcode Client opcode
 * @return Priority value (0 = highest, 100 = lowest)
 */
uint8 PacketFilter::GetPacketPriority(OpcodeClient opcode)
{
    auto it = _opcodePriorities.find(opcode);
    if (it != _opcodePriorities.end())
        return it->second;

    // Default priority for unlisted opcodes
    return 100;
}

/**
 * Get human-readable opcode name
 *
 * Uses TrinityCore's built-in opcode name lookup.
 *
 * @param opcode Client opcode
 * @return Opcode name string (e.g., "CMSG_CAST_SPELL")
 */
char const* PacketFilter::GetOpcodeName(OpcodeClient opcode)
{
    return GetOpcodeNameForLogging(opcode);
}

/**
 * Get total packets filtered since server start
 *
 * THREAD-SAFETY: Atomic load with relaxed ordering
 * (exact count not critical, approximate is fine)
 *
 * @return Total filtered packet count
 */
uint64 PacketFilter::GetTotalFiltered()
{
    return _totalFiltered.load(::std::memory_order_relaxed);
}

/**
 * Get total packets processed since server start
 *
 * THREAD-SAFETY: Atomic load with relaxed ordering
 *
 * @return Total processed packet count
 */
uint64 PacketFilter::GetTotalProcessed()
{
    return _totalProcessed.load(::std::memory_order_relaxed);
}

} // namespace Playerbot
