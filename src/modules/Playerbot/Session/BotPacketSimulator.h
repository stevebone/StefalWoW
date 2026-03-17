/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include <atomic>
#include <cstdint>

class WorldSession;

namespace Playerbot
{

class BotSession;

/**
 * @class BotPacketSimulator
 * @brief Simulates client->server packets that bots don't naturally send
 *
 * PROBLEM: Real players send acknowledgement packets during login/world entry
 * that drive critical server-side state transitions (time sync, object visibility,
 * movement initialization). Bots don't have network clients to send these packets,
 * causing various issues that previously required manual workarounds.
 *
 * SOLUTION: Forge these critical packets by calling the appropriate TrinityCore
 * packet handlers directly, making bot login indistinguishable from real players.
 *
 * ARCHITECTURE: Reactive event-driven ACK system.
 * BotSession::SendPacket() intercepts outgoing SMSG packets and calls
 * OnPacketSent() which sets atomic flags. Update() processes only flagged ACKs.
 * This replaces the old 100ms polling scanner with O(1) atomic checks.
 *
 * Thread safety: std::atomic<uint16_t> with fetch_or (set from any thread)
 * and exchange(0) (read-and-clear from main thread). Lock-free, no mutex.
 *
 * Critical Packets Simulated:
 * 1. CMSG_QUEUED_MESSAGES_END - Resume communication after async DB load
 *    - Triggers time synchronization
 *    - Required for login flow to proceed
 *    - Handler: WorldSession::HandleQueuedMessagesEnd()
 *
 * 2. CMSG_MOVE_INIT_ACTIVE_MOVER_COMPLETE - Acknowledge active mover initialization
 *    - Sets PLAYER_LOCAL_FLAG_OVERRIDE_TRANSPORT_SERVER_TIME automatically
 *    - Enables object visibility (fixes CanNeverSee() check)
 *    - Updates player visibility
 *    - Handler: WorldSession::HandleMoveInitActiveMoverComplete()
 *
 * 3. CMSG_TIME_SYNC_RESPONSE - Periodic time synchronization
 *    - Maintains clock delta calculations
 *    - Prevents movement prediction drift
 *    - Handler: WorldSession::HandleTimeSyncResponse()
 *
 * 4. Movement ACK packets - Speed changes, teleports, knockback, force magnitude
 *    - Prevents uint8 overflow of m_forced_speed_changes[mtype] counters
 *    - Clears teleport semaphores (IsBeingTeleportedNear/Far)
 *    - Broadcasts knockback movement updates to other players
 *    - Drains m_movementForceModMagnitudeChanges counter
 *
 * BENEFITS:
 * - Eliminates manual flag workarounds in bot login code
 * - Matches real player login flow exactly
 * - Automatic phase assignment via TrinityCore APIs
 * - Better long-term stability and maintainability
 * - Reactive ACKs: zero wasted scans, O(1) per tick when idle
 */
class TC_GAME_API BotPacketSimulator
{
public:
    explicit BotPacketSimulator(BotSession* session);
    ~BotPacketSimulator() = default;

    BotPacketSimulator(BotPacketSimulator const&) = delete;
    BotPacketSimulator& operator=(BotPacketSimulator const&) = delete;

    // ========================================================================
    // REACTIVE ACK FLAGS
    // ========================================================================

    /// Atomic bitfield flags for pending movement ACKs.
    /// Set by OnPacketSent() (any thread), cleared by ProcessQueuedAcks() (main thread).
    enum PendingAckFlags : uint16_t
    {
        ACK_NONE                = 0,
        ACK_SPEED_WALK          = 1 << 0,
        ACK_SPEED_RUN           = 1 << 1,
        ACK_SPEED_RUN_BACK      = 1 << 2,
        ACK_SPEED_SWIM          = 1 << 3,
        ACK_SPEED_SWIM_BACK     = 1 << 4,
        ACK_SPEED_TURN_RATE     = 1 << 5,
        ACK_SPEED_FLIGHT        = 1 << 6,
        ACK_SPEED_FLIGHT_BACK   = 1 << 7,
        ACK_SPEED_PITCH_RATE    = 1 << 8,
        ACK_TELEPORT_NEAR       = 1 << 9,
        ACK_WORLDPORT            = 1 << 10,
        ACK_KNOCKBACK           = 1 << 11,
        ACK_FORCE_MAGNITUDE     = 1 << 12,
    };

    // ========================================================================
    // LOGIN SEQUENCE PACKETS
    // ========================================================================

    /**
     * Simulate CMSG_QUEUED_MESSAGES_END packet
     * Called after SendInitialPacketsBeforeAddToMap() to resume communication
     * Triggers time synchronization and allows login to proceed
     */
    void SimulateQueuedMessagesEnd();

    /**
     * Simulate CMSG_MOVE_INIT_ACTIVE_MOVER_COMPLETE packet
     * Called after SendInitialPacketsAfterAddToMap() to complete mover initialization
     * Automatically sets PLAYER_LOCAL_FLAG_OVERRIDE_TRANSPORT_SERVER_TIME
     * Updates player visibility for object detection
     */
    void SimulateMoveInitActiveMoverComplete();

    /**
     * Simulate CMSG_TIME_SYNC_RESPONSE packet
     * Called periodically to maintain time synchronization
     * Prevents movement prediction drift over time
     */
    void SimulateTimeSyncResponse(uint32 counter);

    /**
     * Enable periodic time sync responses (every 10 seconds)
     * Called after bot successfully enters world
     */
    void EnablePeriodicTimeSync();

    /**
     * Disable periodic time sync (called during logout/cleanup)
     */
    void DisablePeriodicTimeSync();

    // ========================================================================
    // REACTIVE PACKET INTERCEPTION
    // ========================================================================

    /**
     * Called by BotSession::SendPacket() to notify the simulator of an outgoing SMSG.
     * Sets atomic flags for SMSG opcodes that require a client ACK response.
     * Thread-safe (lock-free atomic fetch_or). May be called from any thread.
     *
     * @param opcode The SMSG opcode being sent to the bot
     */
    void OnPacketSent(uint32 opcode);

    // ========================================================================
    // MOVEMENT ACK PACKETS
    // ========================================================================

    /**
     * Simulate speed change ACK for a specific UnitMoveType
     * Drains Player::m_forced_speed_changes[mtype] counter to prevent uint8 overflow
     * Forges CMSG_MOVE_FORCE_*_SPEED_CHANGE_ACK packet with the bot's current speed
     *
     * @param mtype UnitMoveType enum value (0-8)
     */
    void SimulateSpeedChangeAck(uint8 mtype);

    /**
     * Simulate CMSG_MOVE_TELEPORT_ACK for near teleports
     * Called when Player::IsBeingTeleportedNear() is true
     * Clears the teleport semaphore and updates bot position
     */
    void SimulateTeleportNearAck();

    /**
     * Simulate CMSG_WORLD_PORT_RESPONSE for far (cross-map) teleports
     * Called when Player::IsBeingTeleportedFar() is true
     * Calls HandleMoveWorldportAck() directly (no packet needed)
     */
    void SimulateWorldportAck();

    /**
     * Simulate CMSG_MOVE_KNOCK_BACK_ACK for knockback effects
     * Broadcasts MoveUpdateKnockBack to other players so they see the bot fly
     */
    void SimulateKnockbackAck();

    /**
     * Simulate CMSG_MOVE_SET_MOD_MOVEMENT_FORCE_MAGNITUDE_ACK
     * Drains Player::m_movementForceModMagnitudeChanges counter
     */
    void SimulateMovementForceMagnitudeAck();

    /**
     * Update method called by BotSession::Update()
     * Handles reactive ACK processing and periodic time sync
     */
    void Update(uint32 diff);

private:
    /**
     * Process all queued ACK flags set by OnPacketSent()
     * Atomically reads and clears all pending flags, then dispatches handlers.
     * Called every tick from Update(), but only does work if flags are set (O(1) check).
     */
    void ProcessQueuedAcks();

    /**
     * Get the CMSG opcode for a speed change ACK given a UnitMoveType
     * @param moveType UnitMoveType enum value (0-8)
     * @return The corresponding CMSG_MOVE_FORCE_*_SPEED_CHANGE_ACK opcode
     */
    static uint32 GetSpeedAckOpcodeForMoveType(uint8 moveType);

    BotSession* _session;                           ///< Owning bot session
    uint32 _simulatedClientTime = 0;                ///< Fake client timestamp (incremented each sync)
    bool _periodicTimeSyncEnabled = false;          ///< Whether periodic sync is active
    uint32 _timeSyncCounter = 0;                    ///< Accumulated time for periodic sync

    /// Atomic bitfield of pending ACK flags. Set by OnPacketSent() (any thread),
    /// read-and-cleared by ProcessQueuedAcks() (main thread). Lock-free.
    std::atomic<uint16_t> _pendingAcks{0};

    static constexpr uint32 TIME_SYNC_INTERVAL = 10000; ///< 10 seconds between syncs
};

} // namespace Playerbot
