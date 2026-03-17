/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotPacketSimulator.h"
#include "BotSession.h"
#include "../AI/BotAI.h"  // P1 FIX: Required for unique_ptr<BotAI> in BotSession.h
#include "WorldSession.h"
#include "Player.h"
#include "Log.h"
#include "Timer.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "UnitDefines.h"

// TrinityCore packet structures
#include "AuthenticationPackets.h"
#include "MovementPackets.h"
#include "MiscPackets.h"
#include "GameTime.h"

namespace Playerbot
{

BotPacketSimulator::BotPacketSimulator(BotSession* session)
    : _session(session)
{
    // Initialize simulated client time with current server time
    _simulatedClientTime = GameTime::GetGameTimeMS();
}

void BotPacketSimulator::SimulateQueuedMessagesEnd()
{

    Player* bot = _session->GetPlayer();

    // CRITICAL: This packet is sent by real clients after receiving SMSG_RESUME_COMMS
    // It triggers time synchronization via HandleTimeSync with SPECIAL_RESUME_COMMS_TIME_SYNC_COUNTER
    // See: MovementHandler.cpp:838-841
    TC_LOG_DEBUG("module.playerbot.packet",
        "Bot {} simulating CMSG_QUEUED_MESSAGES_END (timestamp: {})",
        bot->GetName(), _simulatedClientTime);

    // Create WorldPacket with CMSG_QUEUED_MESSAGES_END opcode
    WorldPacket packet(CMSG_QUEUED_MESSAGES_END);
    packet << _simulatedClientTime;  // Write timestamp
    // Construct packet structure from WorldPacket
    WorldPackets::Auth::QueuedMessagesEnd queuedMessagesEnd(::std::move(packet));
    queuedMessagesEnd.Read();  // Extract data from WorldPacket
    // Call the handler directly (packet forging)
    _session->HandleQueuedMessagesEnd(queuedMessagesEnd);

    TC_LOG_INFO("module.playerbot.packet",
        " Bot {} successfully simulated CMSG_QUEUED_MESSAGES_END - time sync initialized",
        bot->GetName());
}

void BotPacketSimulator::SimulateMoveInitActiveMoverComplete()
{

    Player* bot = _session->GetPlayer();

    // CRITICAL: This packet is sent by real clients after receiving SMSG_MOVE_INIT_ACTIVE_MOVER
    // It triggers:
    // 1. Time synchronization via HandleTimeSync with SPECIAL_INIT_ACTIVE_MOVER_TIME_SYNC_COUNTER
    // 2. Sets PLAYER_LOCAL_FLAG_OVERRIDE_TRANSPORT_SERVER_TIME automatically (MovementHandler.cpp:886)
    // 3. Updates player visibility via UpdateObjectVisibility()
    //
    // Without this packet, Player::CanNeverSee() returns TRUE because the flag is missing,
    // making bots unable to see any objects in the world.
    // See: MovementHandler.cpp:843-848
    TC_LOG_DEBUG("module.playerbot.packet",
        "Bot {} simulating CMSG_MOVE_INIT_ACTIVE_MOVER_COMPLETE (ticks: {})",
        bot->GetName(), _simulatedClientTime);

    // Create WorldPacket with CMSG_MOVE_INIT_ACTIVE_MOVER_COMPLETE opcode
    WorldPacket packet(CMSG_MOVE_INIT_ACTIVE_MOVER_COMPLETE);
    packet << _simulatedClientTime;  // Write ticks
    // Construct packet structure from WorldPacket
    WorldPackets::Movement::MoveInitActiveMoverComplete moveInitComplete(::std::move(packet));
    moveInitComplete.Read();  // Extract data from WorldPacket
    // Call the handler directly (packet forging)
    _session->HandleMoveInitActiveMoverComplete(moveInitComplete);

    TC_LOG_INFO("module.playerbot.packet",
        " Bot {} successfully simulated CMSG_MOVE_INIT_ACTIVE_MOVER_COMPLETE - visibility enabled, flag set automatically",
        bot->GetName());
}

void BotPacketSimulator::SimulateTimeSyncResponse(uint32 counter)
{
    if (!_session)
        return;

    Player* bot = _session->GetPlayer();
    if (!bot || !bot->IsInWorld())
        return;

    // Create WorldPacket with CMSG_TIME_SYNC_RESPONSE opcode
    WorldPacket packet(CMSG_TIME_SYNC_RESPONSE);
    packet << counter;               // Write SequenceIndex (first)
    packet << _simulatedClientTime;  // Write ClientTime (second)

    // Construct packet structure from WorldPacket
    WorldPackets::Misc::TimeSyncResponse timeSyncResponse(::std::move(packet));
    timeSyncResponse.Read();  // Extract data from WorldPacket

    // Call the handler directly
    _session->HandleTimeSyncResponse(timeSyncResponse);
    TC_LOG_TRACE("module.playerbot.packet",
        "Bot {} simulated CMSG_TIME_SYNC_RESPONSE (counter: {}, clientTime: {})",
        bot->GetName(), counter, _simulatedClientTime);

    // Increment client time for next sync (simulate passage of time)
    _simulatedClientTime += TIME_SYNC_INTERVAL;
}

void BotPacketSimulator::EnablePeriodicTimeSync()
{
    _periodicTimeSyncEnabled = true;
    _timeSyncCounter = 0;

    if (Player* bot = _session ? _session->GetPlayer() : nullptr)
    {
        TC_LOG_DEBUG("module.playerbot.packet",
            "Bot {} periodic time sync enabled (interval: {}ms)",
            bot->GetName(), TIME_SYNC_INTERVAL);
    }
}

void BotPacketSimulator::DisablePeriodicTimeSync()
{
    _periodicTimeSyncEnabled = false;

    if (Player* bot = _session ? _session->GetPlayer() : nullptr)
    {
        TC_LOG_DEBUG("module.playerbot.packet",
            "Bot {} periodic time sync disabled",
            bot->GetName());
    }
}

// ============================================================================
// REACTIVE PACKET INTERCEPTION
// ============================================================================

void BotPacketSimulator::OnPacketSent(uint32 opcode)
{
    // Map outgoing SMSG opcodes to atomic ACK flags.
    // Only opcodes that require a client ACK response are handled here.
    // Thread-safe: fetch_or is atomic and lock-free on all platforms.
    uint16_t flag = ACK_NONE;

    switch (opcode)
    {
        // Speed change SMSGs (9 UnitMoveType values)
        case SMSG_MOVE_SET_WALK_SPEED:        flag = ACK_SPEED_WALK; break;
        case SMSG_MOVE_SET_RUN_SPEED:         flag = ACK_SPEED_RUN; break;
        case SMSG_MOVE_SET_RUN_BACK_SPEED:    flag = ACK_SPEED_RUN_BACK; break;
        case SMSG_MOVE_SET_SWIM_SPEED:        flag = ACK_SPEED_SWIM; break;
        case SMSG_MOVE_SET_SWIM_BACK_SPEED:   flag = ACK_SPEED_SWIM_BACK; break;
        case SMSG_MOVE_SET_TURN_RATE:         flag = ACK_SPEED_TURN_RATE; break;
        case SMSG_MOVE_SET_FLIGHT_SPEED:      flag = ACK_SPEED_FLIGHT; break;
        case SMSG_MOVE_SET_FLIGHT_BACK_SPEED: flag = ACK_SPEED_FLIGHT_BACK; break;
        case SMSG_MOVE_SET_PITCH_RATE:        flag = ACK_SPEED_PITCH_RATE; break;

        // Teleport SMSGs
        case SMSG_MOVE_TELEPORT:              flag = ACK_TELEPORT_NEAR; break;
        case SMSG_NEW_WORLD:                  flag = ACK_WORLDPORT; break;

        // Knockback
        case SMSG_MOVE_KNOCK_BACK:            flag = ACK_KNOCKBACK; break;

        // Force magnitude
        case SMSG_MOVE_SET_MOD_MOVEMENT_FORCE_MAGNITUDE: flag = ACK_FORCE_MAGNITUDE; break;

        default:
            return; // Not an ACK-requiring opcode, skip atomic operation
    }

    _pendingAcks.fetch_or(flag, std::memory_order_relaxed);
}

// ============================================================================
// MOVEMENT ACK PACKETS
// ============================================================================

uint32 BotPacketSimulator::GetSpeedAckOpcodeForMoveType(uint8 moveType)
{
    // Maps UnitMoveType enum to the corresponding CMSG speed change ACK opcode
    // See: MovementHandler.cpp:549-560 for the reverse mapping
    switch (moveType)
    {
        case MOVE_WALK:        return CMSG_MOVE_FORCE_WALK_SPEED_CHANGE_ACK;
        case MOVE_RUN:         return CMSG_MOVE_FORCE_RUN_SPEED_CHANGE_ACK;
        case MOVE_RUN_BACK:    return CMSG_MOVE_FORCE_RUN_BACK_SPEED_CHANGE_ACK;
        case MOVE_SWIM:        return CMSG_MOVE_FORCE_SWIM_SPEED_CHANGE_ACK;
        case MOVE_SWIM_BACK:   return CMSG_MOVE_FORCE_SWIM_BACK_SPEED_CHANGE_ACK;
        case MOVE_TURN_RATE:   return CMSG_MOVE_FORCE_TURN_RATE_CHANGE_ACK;
        case MOVE_FLIGHT:      return CMSG_MOVE_FORCE_FLIGHT_SPEED_CHANGE_ACK;
        case MOVE_FLIGHT_BACK: return CMSG_MOVE_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK;
        case MOVE_PITCH_RATE:  return CMSG_MOVE_FORCE_PITCH_RATE_CHANGE_ACK;
        default:               return 0;
    }
}

void BotPacketSimulator::SimulateSpeedChangeAck(uint8 mtype)
{
    if (!_session)
        return;

    Player* bot = _session->GetPlayer();
    if (!bot || !bot->IsInWorld())
        return;

    // Drain all pending ACKs for this specific speed type
    // The handler skips all but the last ACK (when counter reaches 0), then validates speed.
    // We process them all at once since each handler call decrements the counter.
    while (bot->m_forced_speed_changes[mtype] > 0)
    {
        uint32 opcode = GetSpeedAckOpcodeForMoveType(mtype);
        if (opcode == 0)
            break;

        float currentSpeed = bot->GetSpeed(static_cast<UnitMoveType>(mtype));

        // Directly populate the packet struct fields instead of serializing through WorldPacket
        // (operator<<(ByteBuffer&, MovementInfo const&) is not declared in any header)
        WorldPacket data(static_cast<OpcodeClient>(opcode), 0);
        WorldPackets::Movement::MovementSpeedAck ack(::std::move(data));
        ack.Ack.Status = bot->m_movementInfo;
        ack.Ack.Status.guid = bot->GetGUID();
        ack.Ack.AckIndex = 0;
        ack.Speed = currentSpeed;

        _session->HandleForceSpeedChangeAck(ack);

        TC_LOG_DEBUG("module.playerbot.packet",
            "Bot {} simulated speed ACK: mtype={}, speed={:.2f}, remaining={}",
            bot->GetName(), mtype, currentSpeed, bot->m_forced_speed_changes[mtype]);
    }
}

void BotPacketSimulator::SimulateTeleportNearAck()
{
    if (!_session)
        return;

    Player* bot = _session->GetPlayer();
    if (!bot || !bot->IsBeingTeleportedNear())
        return;

    TC_LOG_DEBUG("module.playerbot.packet",
        "Bot {} simulating CMSG_MOVE_TELEPORT_ACK (near teleport)",
        bot->GetName());

    // Forge the teleport ACK packet
    // MoveTeleportAck::Read() expects: MoverGUID + AckIndex + MoveTime
    WorldPacket data(CMSG_MOVE_TELEPORT_ACK, 8 + 4 + 4);
    data << bot->GetGUID();                      // MoverGUID
    data << int32(0);                            // AckIndex (not validated by handler)
    data << int32(GameTime::GetGameTimeMS());    // MoveTime (not validated by handler)

    WorldPackets::Movement::MoveTeleportAck ackPacket(::std::move(data));
    ackPacket.Read();

    _session->HandleMoveTeleportAck(ackPacket);

    TC_LOG_DEBUG("module.playerbot.packet",
        "Bot {} teleport near ACK processed successfully",
        bot->GetName());
}

void BotPacketSimulator::SimulateWorldportAck()
{
    if (!_session)
        return;

    Player* bot = _session->GetPlayer();
    if (!bot || !bot->IsBeingTeleportedFar())
        return;

    TC_LOG_DEBUG("module.playerbot.packet",
        "Bot {} simulating HandleMoveWorldportAck (far teleport)",
        bot->GetName());

    // HandleMoveWorldportAck() is a server-side convenience method that takes no parameters.
    // It handles all far-teleport completion: map change, position update, initial packets.
    _session->HandleMoveWorldportAck();

    TC_LOG_DEBUG("module.playerbot.packet",
        "Bot {} worldport ACK processed successfully",
        bot->GetName());
}

void BotPacketSimulator::SimulateKnockbackAck()
{
    if (!_session)
        return;

    Player* bot = _session->GetPlayer();
    if (!bot || !bot->IsInWorld())
        return;

    TC_LOG_DEBUG("module.playerbot.packet",
        "Bot {} simulating CMSG_MOVE_KNOCK_BACK_ACK",
        bot->GetName());

    // Directly populate the packet struct fields instead of serializing through WorldPacket
    // (operator<<(ByteBuffer&, MovementInfo const&) is not declared in any header)
    WorldPacket data(CMSG_MOVE_KNOCK_BACK_ACK, 0);
    WorldPackets::Movement::MoveKnockBackAck ackPacket(::std::move(data));
    ackPacket.Ack.Status = bot->m_movementInfo;
    ackPacket.Ack.Status.guid = bot->GetGUID();
    ackPacket.Ack.AckIndex = 0;
    // Speeds left empty (Optional not set) - handler only uses Ack.Status

    _session->HandleMoveKnockBackAck(ackPacket);

    TC_LOG_DEBUG("module.playerbot.packet",
        "Bot {} knockback ACK processed, movement update broadcast to nearby players",
        bot->GetName());
}

void BotPacketSimulator::SimulateMovementForceMagnitudeAck()
{
    if (!_session)
        return;

    Player* bot = _session->GetPlayer();
    if (!bot || !bot->IsInWorld())
        return;

    // Drain all pending force magnitude ACKs
    // The handler skips all but the last ACK (when counter reaches 0), then validates magnitude.
    while (bot->m_movementForceModMagnitudeChanges > 0)
    {
        // Calculate expected magnitude from MovementForces
        float expectedMagnitude = 1.0f;
        if (MovementForces const* movementForces = bot->GetMovementForces())
            expectedMagnitude = movementForces->GetModMagnitude();

        // Directly populate the packet struct fields instead of serializing through WorldPacket
        // (handler reuses MovementSpeedAck, Speed field = magnitude)
        WorldPacket data(CMSG_MOVE_SET_MOD_MOVEMENT_FORCE_MAGNITUDE_ACK, 0);
        WorldPackets::Movement::MovementSpeedAck ack(::std::move(data));
        ack.Ack.Status = bot->m_movementInfo;
        ack.Ack.Status.guid = bot->GetGUID();
        ack.Ack.AckIndex = 0;
        ack.Speed = expectedMagnitude;

        _session->HandleMoveSetModMovementForceMagnitudeAck(ack);

        TC_LOG_DEBUG("module.playerbot.packet",
            "Bot {} simulated force magnitude ACK: magnitude={:.2f}, remaining={}",
            bot->GetName(), expectedMagnitude, bot->m_movementForceModMagnitudeChanges);
    }
}

// ============================================================================
// UPDATE LOOP
// ============================================================================

void BotPacketSimulator::ProcessQueuedAcks()
{
    // Atomically read-and-clear all pending flags (lock-free)
    uint16_t acks = _pendingAcks.exchange(0, std::memory_order_relaxed);
    if (acks == ACK_NONE)
        return;

    // Speed change ACKs (bits 0-8, one per UnitMoveType)
    if (acks & ACK_SPEED_WALK)        SimulateSpeedChangeAck(MOVE_WALK);
    if (acks & ACK_SPEED_RUN)         SimulateSpeedChangeAck(MOVE_RUN);
    if (acks & ACK_SPEED_RUN_BACK)    SimulateSpeedChangeAck(MOVE_RUN_BACK);
    if (acks & ACK_SPEED_SWIM)        SimulateSpeedChangeAck(MOVE_SWIM);
    if (acks & ACK_SPEED_SWIM_BACK)   SimulateSpeedChangeAck(MOVE_SWIM_BACK);
    if (acks & ACK_SPEED_TURN_RATE)   SimulateSpeedChangeAck(MOVE_TURN_RATE);
    if (acks & ACK_SPEED_FLIGHT)      SimulateSpeedChangeAck(MOVE_FLIGHT);
    if (acks & ACK_SPEED_FLIGHT_BACK) SimulateSpeedChangeAck(MOVE_FLIGHT_BACK);
    if (acks & ACK_SPEED_PITCH_RATE)  SimulateSpeedChangeAck(MOVE_PITCH_RATE);

    // Teleport ACKs
    if (acks & ACK_TELEPORT_NEAR)     SimulateTeleportNearAck();
    if (acks & ACK_WORLDPORT)         SimulateWorldportAck();

    // Knockback ACK
    if (acks & ACK_KNOCKBACK)         SimulateKnockbackAck();

    // Force magnitude ACK
    if (acks & ACK_FORCE_MAGNITUDE)   SimulateMovementForceMagnitudeAck();
}

void BotPacketSimulator::Update(uint32 diff)
{
    if (!_session || !_session->GetPlayer() || !_session->GetPlayer()->IsInWorld())
        return;

    // --- Reactive ACK processing (runs every tick, O(1) when idle) ---
    ProcessQueuedAcks();

    // --- Periodic time sync (runs every TIME_SYNC_INTERVAL ms) ---
    if (!_periodicTimeSyncEnabled)
        return;

    _timeSyncCounter += diff;

    if (_timeSyncCounter >= TIME_SYNC_INTERVAL)
    {
        // Simulate time sync response with incremented counter
        static uint32 globalTimeSyncCounter = 1000; // Start at 1000 to avoid special counters
        SimulateTimeSyncResponse(globalTimeSyncCounter++);

        _timeSyncCounter = 0;
    }
}

} // namespace Playerbot
