/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "PacketDeferralClassifier.h"
#include "Log.h"

namespace Playerbot {

// Statistics (atomic for thread safety)
::std::atomic<uint64> PacketDeferralClassifier::s_totalClassified{0};
::std::atomic<uint64> PacketDeferralClassifier::s_deferredCount{0};
::std::atomic<uint64> PacketDeferralClassifier::s_workerCount{0};

// ============================================================================
// CATEGORY 1: SPELL CASTING & AURA APPLICATION (CRITICAL - RACE CONDITION FIX)
// ============================================================================
const ::std::unordered_set<OpcodeClient> PacketDeferralClassifier::s_spellOpcodes = {
    CMSG_CAST_SPELL,                        // PRIMARY CRASH SOURCE
    CMSG_CANCEL_AURA,                       // Aura removal (race with application)
    CMSG_CANCEL_AUTO_REPEAT_SPELL,          // Auto-attack cancel
    CMSG_CANCEL_CAST,                       // Spell cancel (modifies cast state)
    CMSG_CANCEL_CHANNELLING,                // Channel cancel
    CMSG_CANCEL_GROWTH_AURA,                // Growth aura cancel
    CMSG_CANCEL_MOUNT_AURA,                 // Mount cancel (aura removal)
    CMSG_PET_CAST_SPELL,                    // Pet spell cast (applies auras)
    CMSG_PET_CANCEL_AURA,                   // Pet aura cancel
    CMSG_TOTEM_DESTROYED,                   // Totem destruction (can trigger spell effects)
    // NOTE: CMSG_CAST_SPELL_EMBEDDED does not exist in 12.0 — embedded casts
    // are handled internally by the spell system, no client opcode needed.
};

// ============================================================================
// CATEGORY 2: ITEM USAGE & EQUIPMENT (MODIFIES INVENTORY + TRIGGERS SPELLS)
// ============================================================================
const ::std::unordered_set<OpcodeClient> PacketDeferralClassifier::s_itemOpcodes = {
    CMSG_USE_ITEM,                          // Can trigger spell cast → aura application
    CMSG_LOOT_ITEM,                         // Modifies inventory (12.0 opcode)
    CMSG_SWAP_INV_ITEM,                     // Equipment swap (can trigger on-equip effects)
    CMSG_SWAP_ITEM,                         // Bag slot swap
    CMSG_AUTO_EQUIP_ITEM,                   // Auto-equip (triggers on-equip auras)
    CMSG_AUTO_EQUIP_ITEM_SLOT,              // Equip to specific slot
    CMSG_AUTO_STORE_BAG_ITEM,               // Store in bag
    CMSG_AUTOBANK_ITEM,                     // Bank storage
    CMSG_AUTOSTORE_BANK_ITEM,               // Retrieve from bank
    CMSG_DESTROY_ITEM,                      // Item destruction
    CMSG_SPLIT_ITEM,                        // Item splitting
    CMSG_READ_ITEM,                         // Reading item (can trigger quest completion)
    CMSG_OPEN_ITEM,                         // Opening item (loot generation)
    CMSG_WRAP_ITEM,                         // Gift wrapping
};

// ============================================================================
// CATEGORY 3: RESURRECTION & DEATH RECOVERY (CRITICAL - CORPSE MANIPULATION)
// ============================================================================
const ::std::unordered_set<OpcodeClient> PacketDeferralClassifier::s_resurrectionOpcodes = {
    CMSG_RECLAIM_CORPSE,                    // Corpse reclaim (modifies corpse list)
    CMSG_REPOP_REQUEST,                     // Release spirit (creates corpse, applies Ghost aura)
    CMSG_RESURRECT_RESPONSE,                // Accept resurrection (modifies alive/dead state)
    CMSG_AREA_SPIRIT_HEALER_QUEUE,          // Spirit healer queue
    CMSG_AREA_SPIRIT_HEALER_QUERY,          // Spirit healer query (can trigger resurrection)
};

// ============================================================================
// CATEGORY 4: MOVEMENT & POSITION (TRIGGERS AREA AURAS + SPATIAL GRID UPDATES)
// ============================================================================
const ::std::unordered_set<OpcodeClient> PacketDeferralClassifier::s_movementOpcodes = {
    CMSG_MOVE_TELEPORT_ACK,                 // Teleport confirmation (updates position)
    // NOTE: CMSG_MOVE_WORLDPORT_ACK does not exist in 12.0 — map transfers
    // are handled via CMSG_SUSPEND_TOKEN_RESPONSE below.
    CMSG_AREA_TRIGGER,                      // Area trigger activation (can apply auras!)
    CMSG_SUSPEND_TOKEN_RESPONSE,            // Map transfer response
};

// ============================================================================
// CATEGORY 5: COMBAT & TARGETING (MODIFIES COMBAT STATE + THREAT)
// ============================================================================
const ::std::unordered_set<OpcodeClient> PacketDeferralClassifier::s_combatOpcodes = {
    CMSG_ATTACK_SWING,                      // Melee attack start
    CMSG_ATTACK_STOP,                       // Combat end
    CMSG_SET_SELECTION,                     // Target selection
    CMSG_PET_ACTION,                        // Pet attack/follow (modifies pet state)
    CMSG_DISMISS_CRITTER,                   // Critter dismiss
};

// ============================================================================
// CATEGORY 6: QUEST & OBJECTIVE UPDATES (TRIGGERS REWARDS + SPELL CASTS)
// ============================================================================
const ::std::unordered_set<OpcodeClient> PacketDeferralClassifier::s_questOpcodes = {
    CMSG_QUEST_GIVER_ACCEPT_QUEST,          // Quest accept (can add items, apply auras)
    CMSG_QUEST_GIVER_COMPLETE_QUEST,        // Quest complete (rewards: items, spells)
    CMSG_QUEST_GIVER_QUERY_QUEST,           // Quest query (can trigger completion check)
    CMSG_QUEST_GIVER_STATUS_QUERY,          // Quest status query
    CMSG_QUEST_GIVER_REQUEST_REWARD,        // Reward selection
    CMSG_QUEST_GIVER_CHOOSE_REWARD,         // Reward choice (adds items)
    CMSG_QUEST_LOG_REMOVE_QUEST,            // Quest abandon
    CMSG_PUSH_QUEST_TO_PARTY,               // Quest sharing
};

// ============================================================================
// CATEGORY 7: GROUP & RAID OPERATIONS (MODIFIES GROUP STATE)
// ============================================================================
const ::std::unordered_set<OpcodeClient> PacketDeferralClassifier::s_groupOpcodes = {
    CMSG_PARTY_INVITE,                      // Group invite (12.0 renamed to PARTY)
    CMSG_PARTY_INVITE_RESPONSE,             // Accept/decline invite
    CMSG_PARTY_UNINVITE,                    // Kick from group
    CMSG_SET_PARTY_LEADER,                  // Leader change
    CMSG_SET_PARTY_ASSIGNMENT,              // Role assignment (12.0)
    CMSG_SET_LOOT_METHOD,                   // Loot method change (12.0)
    CMSG_LOOT_ROLL,                         // Loot roll
    CMSG_READY_CHECK_RESPONSE,              // Ready check response
};

// ============================================================================
// CATEGORY 8: TRADE & ECONOMY (MODIFIES INVENTORY + GOLD)
// ============================================================================
const ::std::unordered_set<OpcodeClient> PacketDeferralClassifier::s_tradeOpcodes = {
    CMSG_INITIATE_TRADE,                    // Trade initiation
    CMSG_ACCEPT_TRADE,                      // Trade accept (transfers items/gold)
    CMSG_CANCEL_TRADE,                      // Trade cancel
    CMSG_SET_TRADE_GOLD,                    // Set gold amount
    CMSG_SET_TRADE_ITEM,                    // Set trade item
    CMSG_SEND_MAIL,                         // Mail send (modifies inventory)
    CMSG_MAIL_RETURN_TO_SENDER,             // Mail return
    CMSG_MAIL_TAKE_ITEM,                    // Mail item retrieval
    CMSG_MAIL_TAKE_MONEY,                   // Mail money retrieval
};

// ============================================================================
// PUBLIC API
// ============================================================================

bool PacketDeferralClassifier::RequiresMainThread(OpcodeClient opcode)
{
    // Increment statistics
    s_totalClassified.fetch_add(1, ::std::memory_order_relaxed);

    // Check all deferral categories
    bool requiresDefer =
        s_spellOpcodes.count(opcode) ||
        s_itemOpcodes.count(opcode) ||
        s_resurrectionOpcodes.count(opcode) ||
        s_movementOpcodes.count(opcode) ||
        s_combatOpcodes.count(opcode) ||
        s_questOpcodes.count(opcode) ||
        s_groupOpcodes.count(opcode) ||
        s_tradeOpcodes.count(opcode);

    // Update statistics
    if (requiresDefer)
        s_deferredCount.fetch_add(1, ::std::memory_order_relaxed);
    else
        s_workerCount.fetch_add(1, ::std::memory_order_relaxed);

    return requiresDefer;
}

const char* PacketDeferralClassifier::GetDeferralReason(OpcodeClient opcode)
{
    if (s_spellOpcodes.count(opcode))
        return "Spell casting/aura application - Race condition with Map::Update()";

    if (s_itemOpcodes.count(opcode))
        return "Item usage/inventory modification - Can trigger spell casts";

    if (s_resurrectionOpcodes.count(opcode))
        return "Resurrection/death recovery - Modifies corpse list and player state";

    if (s_movementOpcodes.count(opcode))
        return "Movement/position change - Triggers area auras and spatial updates";

    if (s_combatOpcodes.count(opcode))
        return "Combat/targeting - Modifies combat state and threat tables";

    if (s_questOpcodes.count(opcode))
        return "Quest/objective - Triggers rewards (items, spells, auras)";

    if (s_groupOpcodes.count(opcode))
        return "Group/raid operation - Modifies group composition and state";

    if (s_tradeOpcodes.count(opcode))
        return "Trade/economy - Modifies inventory and gold";

    return nullptr; // Worker thread safe
}

void PacketDeferralClassifier::GetStatistics(uint64& totalPackets, uint64& deferredPackets, uint64& workerPackets)
{
    totalPackets = s_totalClassified.load(::std::memory_order_relaxed);
    deferredPackets = s_deferredCount.load(::std::memory_order_relaxed);
    workerPackets = s_workerCount.load(::std::memory_order_relaxed);
}

} // namespace Playerbot
