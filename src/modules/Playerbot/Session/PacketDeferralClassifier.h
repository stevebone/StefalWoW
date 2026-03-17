/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef PACKET_DEFERRAL_CLASSIFIER_H
#define PACKET_DEFERRAL_CLASSIFIER_H

#include "Define.h"
#include "Opcodes.h"
#include <atomic>
#include <unordered_set>

namespace Playerbot {

/**
 * @brief Classifies packets by whether they require main thread execution
 *
 * PURPOSE: Minimize main thread load by only deferring packets that modify
 *          game state in ways that can cause race conditions with Map::Update()
 *
 * DESIGN PHILOSOPHY:
 * - Worker thread safe: Read-only operations, queries, client state updates
 * - Main thread required: Game state modifications (spells, items, auras, movement)
 *
 * PERFORMANCE TARGET:
 * - Classification: O(1) hash lookup, <5 CPU cycles
 * - Deferral rate: ~15-20% of packets (80-85% stay on worker threads)
 *
 * ROOT CAUSE OF AURA CRASH:
 * CMSG_CAST_SPELL processed on bot worker thread while Map::Update() runs
 * on map worker thread → Race condition in AuraApplication::_HandleEffect
 *
 * @author TrinityCore Playerbot Integration Team
 * @date 2025-11-04
 * @version 1.0.0 (Crash Fix - Selective Deferral)
 */
class TC_GAME_API PacketDeferralClassifier
{
public:
    /**
     * @brief Check if opcode requires main thread execution
     * @param opcode The client opcode to classify
     * @return true if packet MUST be deferred to main thread
     * @return false if packet is safe to process on worker thread
     *
     * Thread Safety: This function is thread-safe (read-only access to const set)
     * Performance: O(1) hash lookup, ~3-5 CPU cycles
     */
    static bool RequiresMainThread(OpcodeClient opcode);

    /**
     * @brief Get human-readable reason for deferral decision
     * @param opcode The client opcode to explain
     * @return const char* explaining why packet requires main thread (or nullptr)
     */
    static const char* GetDeferralReason(OpcodeClient opcode);

    /**
     * @brief Get statistics on deferral classification
     * @param totalPackets OUT: Total packets classified
     * @param deferredPackets OUT: Packets requiring deferral
     * @param workerPackets OUT: Packets safe for worker threads
     */
    static void GetStatistics(uint64& totalPackets, uint64& deferredPackets, uint64& workerPackets);

private:
    /**
     * CATEGORY 1: GAME STATE MODIFICATION (MUST DEFER)
     * These opcodes modify persistent game state that Map::Update() concurrently accesses
     */

    // Spell Casting & Aura Application
    // CRITICAL: Spell::_cast → Unit::_ApplyAura → AuraApplication::_HandleEffect
    // Race Condition: Multiple threads applying same aura → Assertion failure (crash)
    static const ::std::unordered_set<OpcodeClient> s_spellOpcodes;

    // Item Usage & Equipment
    // Modifies inventory state, triggers spell casts, can apply auras
    static const ::std::unordered_set<OpcodeClient> s_itemOpcodes;

    // Resurrection & Death Recovery
    // CRITICAL: Modifies player state (alive/dead), removes corpse, applies auras
    // Race Condition: Map::Update may be iterating corpses while removal happens
    static const ::std::unordered_set<OpcodeClient> s_resurrectionOpcodes;

    // Movement & Position Changes
    // Modifies spatial grid, triggers area triggers, can apply auras
    static const ::std::unordered_set<OpcodeClient> s_movementOpcodes;

    // Combat & Targeting
    // Modifies combat state, threat tables, target selection
    static const ::std::unordered_set<OpcodeClient> s_combatOpcodes;

    // Quest & Objective Updates
    // Modifies quest state, triggers quest rewards (items, spells)
    static const ::std::unordered_set<OpcodeClient> s_questOpcodes;

    // Group & Raid Operations
    // Modifies group composition, roles, loot distribution
    static const ::std::unordered_set<OpcodeClient> s_groupOpcodes;

    // Trade & Economy
    // Modifies inventory, gold, mailbox state
    static const ::std::unordered_set<OpcodeClient> s_tradeOpcodes;

    /**
     * CATEGORY 2: WORKER THREAD SAFE (NO DEFERRAL)
     * These opcodes only read state or modify client-local data
     */

    // Examples (implicit - anything NOT in above sets):
    // - CMSG_PING, CMSG_TIME_SYNC_RESPONSE (network state)
    // - CMSG_CHAT_MESSAGE (chat log, no game state)
    // - CMSG_WHO (query only, no modifications)
    // - CMSG_QUERY_* (database queries, read-only)
    // - CMSG_*_ACK (acknowledgments, no side effects)

    // Statistics tracking (atomic for thread safety)
    static ::std::atomic<uint64> s_totalClassified;
    static ::std::atomic<uint64> s_deferredCount;
    static ::std::atomic<uint64> s_workerCount;
};

} // namespace Playerbot

#endif // PACKET_DEFERRAL_CLASSIFIER_H
