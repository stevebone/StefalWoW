/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file ParseLFGPacket_Typed.cpp
 * @brief Typed packet handlers for LFG (Looking For Group) packets
 *
 * Handles LFG queue status, proposals, and auto-acceptance for bots.
 * Part of the JIT bot creation system - detects role shortages and triggers
 * bot creation when queues need more tanks/healers/DPS.
 *
 * Packet Types Handled:
 * - SMSG_LFG_QUEUE_STATUS - Queue wait times and role needs
 * - SMSG_LFG_PROPOSAL_UPDATE - Dungeon found, need acceptance
 * - SMSG_LFG_UPDATE_STATUS - General queue status updates
 * - SMSG_LFG_JOIN_RESULT - Result of join attempt
 */

#include "PlayerbotPacketSniffer.h"
#include "WorldSession.h"
#include "Player.h"
#include "LFGPackets.h"
#include "LFGMgr.h"
#include "Core/Events/EventDispatcher.h"
#include "Core/Events/BotEventTypes.h"
#include "Core/Events/QueueEventData.h"
#include "Lifecycle/Instance/QueueStatePoller.h"
#include "Lifecycle/Instance/JITBotFactory.h"
#include "LFG/LFGBotManager.h"
#include "Session/BotPacketRelay.h"
#include "Log.h"

namespace Playerbot
{

// ================================================================================================
// HELPER FUNCTIONS
// ================================================================================================

/**
 * @brief Check if player is a bot
 */
static bool IsBot(Player* player)
{
    return BotPacketRelay::IsBot(player);
}

/**
 * @brief Convert LFG slot to dungeon ID
 * The slot contains dungeon ID information
 */
static uint32 ExtractDungeonIdFromSlot(uint32 slot)
{
    // LFG slot format: dungeonId | (type << 24)
    // For most purposes, the slot IS the dungeon ID
    return slot & 0xFFFFFF;
}

// ================================================================================================
// TYPED PACKET HANDLERS
// ================================================================================================

/**
 * @brief SMSG_LFG_QUEUE_STATUS - Queue status with role counts
 *
 * Triggered periodically while in LFG queue. Contains:
 * - Wait time estimates per role
 * - Number of each role still needed (LastNeeded[3])
 *
 * This is the PRIMARY source for detecting LFG role shortages.
 * When LastNeeded indicates missing roles, we can trigger JIT bot creation.
 */
void ParseTypedLFGQueueStatus(WorldSession* session, WorldPackets::LFG::LFGQueueStatus const& packet)
{
    if (!session)
        return;

    Player* player = session->GetPlayer();
    if (!player)
        return;

    bool isBot = IsBot(player);

    // Extract dungeon info from slot
    uint32 dungeonId = ExtractDungeonIdFromSlot(packet.Slot);

    // Role needs from LastNeeded array
    // Index 0 = Tank, 1 = Healer, 2 = DPS
    uint8 tanksNeeded = packet.LastNeeded[0];
    uint8 healersNeeded = packet.LastNeeded[1];
    uint8 dpsNeeded = packet.LastNeeded[2];

    TC_LOG_DEBUG("playerbot.packets", "LFG Queue Status: {} {} - Dungeon={}, Wait={}ms, Needed: T={} H={} D={}",
        isBot ? "Bot" : "Player",
        player->GetName(),
        dungeonId,
        packet.QueuedTime,
        tanksNeeded, healersNeeded, dpsNeeded);

    // Create queue status update event
    Events::BotEvent event(StateMachine::EventType::LFG_QUEUE_UPDATE, player->GetGUID());

    Events::QueueStatusUpdateEventData eventData;
    eventData.contentType = Events::ContentType::Dungeon;
    eventData.contentId = dungeonId;
    eventData.tankCount = 1 - tanksNeeded;   // Approximate current count
    eventData.healerCount = 1 - healersNeeded;
    eventData.dpsCount = 3 - dpsNeeded;
    eventData.minPlayers = 5;
    eventData.maxPlayers = 5;
    eventData.estimatedWaitTime = packet.AvgWaitTime;
    eventData.timestamp = time(nullptr);

    event.eventData = std::make_any<Events::QueueStatusUpdateEventData const*>(&eventData);

    // If there's a shortage and this is a human player, register for JIT polling
    bool hasShortage = (tanksNeeded > 0 || healersNeeded > 0 || dpsNeeded > 0);
    if (hasShortage && !isBot)
    {
        // Register this dungeon queue as active for polling
        sQueueStatePoller->RegisterActiveLFGQueue(dungeonId);

        // Create and dispatch shortage event for JIT handling
        Events::BotEvent shortageEvent(StateMachine::EventType::LFG_QUEUE_SHORTAGE, player->GetGUID());

        Events::QueueShortageEventData shortageData;
        shortageData.contentType = Events::ContentType::Dungeon;
        shortageData.contentId = dungeonId;
        shortageData.tankNeeded = tanksNeeded;
        shortageData.healerNeeded = healersNeeded;
        shortageData.dpsNeeded = dpsNeeded;
        shortageData.priority = 7;  // LFG gets high priority
        shortageData.timestamp = time(nullptr);
        shortageData.triggerPlayerGuid = player->GetGUID();

        shortageEvent.eventData = std::make_any<Events::QueueShortageEventData const*>(&shortageData);

        TC_LOG_INFO("playerbot.jit", "LFG Queue Shortage detected: Dungeon={}, Need: T={} H={} D={}",
            dungeonId, tanksNeeded, healersNeeded, dpsNeeded);
    }
}

/**
 * @brief SMSG_LFG_PROPOSAL_UPDATE - Dungeon group found
 *
 * Triggered when the matchmaking system has found a complete group.
 * All players must accept the proposal within the timeout.
 *
 * For bots: Auto-accept the proposal
 * For all: Dispatch proposal event
 */
void ParseTypedLFGProposalUpdate(WorldSession* session, WorldPackets::LFG::LFGProposalUpdate const& packet)
{
    if (!session)
        return;

    Player* player = session->GetPlayer();
    if (!player)
        return;

    bool isBot = IsBot(player);

    // Extract dungeon info
    uint32 dungeonId = ExtractDungeonIdFromSlot(packet.Slot);
    uint32 proposalId = packet.ProposalID;
    int8 state = packet.State;

    TC_LOG_DEBUG("playerbot.packets", "LFG Proposal Update: {} {} - Proposal={}, Dungeon={}, State={}, Players={}",
        isBot ? "Bot" : "Player",
        player->GetName(),
        proposalId,
        dungeonId,
        state,
        packet.Players.size());

    // Create proposal event
    Events::BotEvent event(StateMachine::EventType::LFG_PROPOSAL, player->GetGUID());

    Events::LFGProposalEventData eventData;
    eventData.proposalId = proposalId;
    eventData.dungeonId = dungeonId;
    eventData.proposalState = static_cast<uint8>(state);
    eventData.playerGuid = player->GetGUID();
    eventData.isBot = isBot;
    eventData.timestamp = time(nullptr);

    // Determine player's role from the proposal
    for (auto const& proposalPlayer : packet.Players)
    {
        if (proposalPlayer.Me)
        {
            eventData.playerRole = proposalPlayer.Roles;
            break;
        }
    }

    event.eventData = std::make_any<Events::LFGProposalEventData const*>(&eventData);

    // Auto-accept proposal for bots
    // State 0 = LFG_PROPOSAL_INITIATING (need response)
    // State 1 = LFG_PROPOSAL_FAILED
    // State 2 = LFG_PROPOSAL_SUCCESS
    if (isBot && state == 0)
    {
        // Check if bot hasn't already responded
        bool alreadyResponded = false;
        for (auto const& proposalPlayer : packet.Players)
        {
            if (proposalPlayer.Me && proposalPlayer.Responded)
            {
                alreadyResponded = true;
                break;
            }
        }

        if (!alreadyResponded)
        {
            TC_LOG_INFO("playerbot.jit", "Bot {} auto-accepting LFG proposal {} for dungeon {}",
                player->GetName(), proposalId, dungeonId);

            // Use LFGMgr to accept the proposal
            sLFGMgr->UpdateProposal(proposalId, player->GetGUID(), true);
        }
    }
}

/**
 * @brief SMSG_LFG_UPDATE_STATUS - General LFG status update
 *
 * Triggered for various LFG state changes:
 * - Joined queue
 * - Left queue
 * - Queued status changed
 */
void ParseTypedLFGUpdateStatus(WorldSession* session, WorldPackets::LFG::LFGUpdateStatus const& packet)
{
    if (!session)
        return;

    Player* player = session->GetPlayer();
    if (!player)
        return;

    bool isBot = IsBot(player);

    TC_LOG_DEBUG("playerbot.packets", "LFG Update Status: {} {} - Joined={}, Queued={}, Reason={}, Slots={}",
        isBot ? "Bot" : "Player",
        player->GetName(),
        packet.Joined,
        packet.Queued,
        packet.Reason,
        packet.Slots.size());

    // Determine event type based on state
    StateMachine::EventType eventType;
    if (packet.Joined && packet.Queued)
    {
        eventType = StateMachine::EventType::LFG_QUEUE_JOIN;
    }
    else if (!packet.Queued)
    {
        eventType = StateMachine::EventType::LFG_QUEUE_LEAVE;
    }
    else
    {
        eventType = StateMachine::EventType::LFG_QUEUE_UPDATE;
    }

    Events::BotEvent event(eventType, player->GetGUID());

    // Process each slot (dungeon) in the queue
    for (uint32 slot : packet.Slots)
    {
        uint32 dungeonId = ExtractDungeonIdFromSlot(slot);

        if (packet.Joined && packet.Queued && !isBot)
        {
            // Human player joined queue - register for JIT polling
            sQueueStatePoller->RegisterActiveLFGQueue(dungeonId);

            TC_LOG_INFO("playerbot.jit", "Human joined LFG queue: {} queued for dungeon {}",
                player->GetName(), dungeonId);
        }
        else if (!packet.Queued)
        {
            // Left queue - unregister from polling
            sQueueStatePoller->UnregisterActiveLFGQueue(dungeonId);
        }
    }
}

/**
 * @brief SMSG_LFG_JOIN_RESULT - Result of LFG join attempt
 *
 * Triggered after attempting to join the LFG queue.
 * Result 0 = success, other values indicate failure reasons.
 */
void ParseTypedLFGJoinResult(WorldSession* session, WorldPackets::LFG::LFGJoinResult const& packet)
{
    if (!session)
        return;

    Player* player = session->GetPlayer();
    if (!player)
        return;

    bool isBot = IsBot(player);

    TC_LOG_DEBUG("playerbot.packets", "LFG Join Result: {} {} - Result={}, Detail={}",
        isBot ? "Bot" : "Player",
        player->GetName(),
        packet.Result,
        packet.ResultDetail);

    // If join failed, dispatch event
    if (packet.Result != 0)
    {
        Events::BotEvent event(StateMachine::EventType::LFG_QUEUE_LEAVE, player->GetGUID());

        Events::QueueLeaveEventData eventData;
        eventData.contentType = Events::ContentType::Dungeon;
        eventData.playerGuid = player->GetGUID();
        eventData.isBot = isBot;
        eventData.leaveReason = 3;  // Error
        eventData.timestamp = time(nullptr);

        event.eventData = std::make_any<Events::QueueLeaveEventData const*>(&eventData);
    }
}

// ================================================================================================
// HANDLER REGISTRATION
// Called from PlayerbotPacketSniffer::Initialize()
// ================================================================================================

void RegisterLFGPacketHandlers()
{
    // Queue status handler - primary source for shortage detection
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::LFG::LFGQueueStatus>(
        &ParseTypedLFGQueueStatus);

    // Proposal update handler - auto-accept for bots
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::LFG::LFGProposalUpdate>(
        &ParseTypedLFGProposalUpdate);

    // General status update handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::LFG::LFGUpdateStatus>(
        &ParseTypedLFGUpdateStatus);

    // Join result handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::LFG::LFGJoinResult>(
        &ParseTypedLFGJoinResult);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered 4 LFG packet typed handlers");
}

} // namespace Playerbot
