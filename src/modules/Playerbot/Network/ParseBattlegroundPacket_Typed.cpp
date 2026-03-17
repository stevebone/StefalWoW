/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file ParseBattlegroundPacket_Typed.cpp
 * @brief Typed packet handlers for Battleground/Arena packets
 *
 * Handles BG queue status, invitations, and auto-acceptance for bots.
 * Part of the JIT bot creation system - detects queue states and triggers
 * bot creation when shortages are detected.
 *
 * Packet Types Handled:
 * - SMSG_BATTLEFIELD_STATUS_QUEUED - Player entered queue
 * - SMSG_BATTLEFIELD_STATUS_NEED_CONFIRMATION - BG ready, need acceptance
 * - SMSG_BATTLEFIELD_STATUS_ACTIVE - Player is in active BG
 * - SMSG_BATTLEFIELD_STATUS_FAILED - Queue failed
 * - SMSG_BATTLEFIELD_STATUS_NONE - Queue cleared
 */

#include "PlayerbotPacketSniffer.h"
#include "WorldSession.h"
#include "Player.h"
#include "BattlegroundPackets.h"
#include "Core/Events/EventDispatcher.h"
#include "Core/Events/BotEventTypes.h"
#include "Core/Events/QueueEventData.h"
#include "Lifecycle/Instance/QueueStatePoller.h"
#include "Lifecycle/Instance/JITBotFactory.h"
#include "PvP/BGBotManager.h"
#include "PvP/ArenaBotManager.h"
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
 * @brief Extract BG type ID from queue ID (QueueID is packed)
 * The QueueID contains battleground type and other info
 */
static uint32 ExtractBGTypeFromQueueID(uint64 queueId)
{
    // QueueID structure varies - the low bits typically contain the BG type
    // This is a simplification; actual extraction depends on TC version
    return static_cast<uint32>(queueId & 0xFFFF);
}

/**
 * @brief Determine if this is an arena queue based on team size
 */
static bool IsArenaQueue(uint8 teamSize)
{
    return teamSize == 2 || teamSize == 3 || teamSize == 5;
}

// ================================================================================================
// TYPED PACKET HANDLERS
// ================================================================================================

/**
 * @brief SMSG_BATTLEFIELD_STATUS_QUEUED - Player/Bot entered queue
 *
 * Triggered when a player enters the BG/Arena queue.
 * For bots: Track queue state for JIT coordination
 * For all: Register active queue for polling
 */
void ParseTypedBattlefieldStatusQueued(WorldSession* session, WorldPackets::Battleground::BattlefieldStatusQueued const& packet)
{
    if (!session)
        return;

    Player* player = session->GetPlayer();
    if (!player)
        return;

    bool isBot = IsBot(player);

    // Extract queue information from header
    uint8 teamSize = packet.Hdr.TeamSize;
    uint32 instanceId = packet.Hdr.InstanceID;
    uint8 bracketMin = packet.Hdr.RangeMin;
    uint8 bracketMax = packet.Hdr.RangeMax;

    // Get BG type from queue IDs (first queue ID if available)
    uint32 bgTypeId = 0;
    if (!packet.Hdr.QueueID.empty())
        bgTypeId = ExtractBGTypeFromQueueID(packet.Hdr.QueueID[0]);

    // Determine bracket ID from level range
    BattlegroundBracketId bracketId = static_cast<BattlegroundBracketId>(
        (bracketMin - 10) / 10);  // Simplified bracket calculation

    TC_LOG_DEBUG("playerbot.packets", "BG Status Queued: {} {} for BG type {} (teamSize={}, bracket={}-{}), wait={}ms",
        isBot ? "Bot" : "Player",
        player->GetName(),
        bgTypeId,
        teamSize,
        bracketMin, bracketMax,
        packet.WaitTime);

    // Create and dispatch queue join event
    Events::BotEvent event(StateMachine::EventType::BG_QUEUE_JOIN, player->GetGUID());

    Events::QueueJoinEventData eventData;
    eventData.contentType = IsArenaQueue(teamSize) ? Events::ContentType::Arena : Events::ContentType::Battleground;
    eventData.contentId = bgTypeId;
    eventData.bracketId = static_cast<uint32>(bracketId);
    eventData.playerGuid = player->GetGUID();
    eventData.playerTeam = static_cast<Team>(player->GetTeam());
    eventData.playerLevel = player->GetLevel();
    eventData.isBot = isBot;
    eventData.isGroupLeader = packet.AsGroup;
    eventData.timestamp = time(nullptr);

    event.eventData = std::make_any<Events::QueueJoinEventData const*>(&eventData);

    // Register queue as active for polling (human players trigger this)
    if (!isBot && bgTypeId != 0)
    {
        if (IsArenaQueue(teamSize))
        {
            // Arena queue - less relevant for polling as arenas pop quickly
            TC_LOG_DEBUG("playerbot.jit", "Human joined arena queue: type={}, teamSize={}",
                bgTypeId, teamSize);
        }
        else
        {
            // BG queue - register for shortage polling
            BattlegroundTypeId bgType = static_cast<BattlegroundTypeId>(bgTypeId);
            sQueueStatePoller->RegisterActiveBGQueue(bgType, bracketId);

            TC_LOG_INFO("playerbot.jit", "Human joined BG queue: {} queued for BG {} bracket {}",
                player->GetName(), bgTypeId, static_cast<uint32>(bracketId));
        }
    }
}

/**
 * @brief SMSG_BATTLEFIELD_STATUS_NEED_CONFIRMATION - BG/Arena is ready, need acceptance
 *
 * Triggered when the matchmaking system has found enough players and the
 * BG/Arena is ready to start. Players must accept within the timeout.
 *
 * For bots: Auto-accept the invitation
 * For all: Dispatch invitation event
 */
void ParseTypedBattlefieldStatusNeedConfirmation(WorldSession* session, WorldPackets::Battleground::BattlefieldStatusNeedConfirmation const& packet)
{
    if (!session)
        return;

    Player* player = session->GetPlayer();
    if (!player)
        return;

    bool isBot = IsBot(player);

    // Extract queue information
    uint8 teamSize = packet.Hdr.TeamSize;
    uint32 instanceId = packet.Hdr.InstanceID;
    uint32 mapId = packet.Mapid;
    uint32 timeout = packet.Timeout;

    // Get BG type from queue IDs
    uint32 bgTypeId = 0;
    if (!packet.Hdr.QueueID.empty())
        bgTypeId = ExtractBGTypeFromQueueID(packet.Hdr.QueueID[0]);

    TC_LOG_DEBUG("playerbot.packets", "BG Status NeedConfirmation: {} {} for BG type {} (map={}, instance={}, timeout={}s)",
        isBot ? "Bot" : "Player",
        player->GetName(),
        bgTypeId,
        mapId,
        instanceId,
        timeout);

    // Create and dispatch invitation event
    Events::BotEvent event(
        IsArenaQueue(teamSize) ? StateMachine::EventType::ARENA_INVITATION_RECEIVED
                               : StateMachine::EventType::BG_INVITATION_RECEIVED,
        player->GetGUID());

    Events::QueueInvitationEventData eventData;
    eventData.contentType = IsArenaQueue(teamSize) ? Events::ContentType::Arena : Events::ContentType::Battleground;
    eventData.contentId = bgTypeId;
    eventData.instanceId = instanceId;
    eventData.playerGuid = player->GetGUID();
    eventData.timeout = timeout;
    eventData.isBot = isBot;
    eventData.timestamp = time(nullptr);

    event.eventData = std::make_any<Events::QueueInvitationEventData const*>(&eventData);

    // Auto-accept invitation for bots
    if (isBot)
    {
        TC_LOG_INFO("playerbot.jit", "Bot {} received BG invitation - auto-accepting (BG={}, instance={})",
            player->GetName(), bgTypeId, instanceId);

        if (IsArenaQueue(teamSize))
        {
            // Arena invitation
            sArenaBotManager->OnInvitationReceived(player->GetGUID(), instanceId);
        }
        else
        {
            // BG invitation
            sBGBotManager->OnInvitationReceived(player->GetGUID(), instanceId);
        }
    }
}

/**
 * @brief SMSG_BATTLEFIELD_STATUS_ACTIVE - Player is in an active BG/Arena
 *
 * Triggered when the player has entered an active battleground or arena.
 * Used to track bot participation and cleanup queue state.
 */
void ParseTypedBattlefieldStatusActive(WorldSession* session, WorldPackets::Battleground::BattlefieldStatusActive const& packet)
{
    if (!session)
        return;

    Player* player = session->GetPlayer();
    if (!player)
        return;

    bool isBot = IsBot(player);

    // Extract information
    uint8 teamSize = packet.Hdr.TeamSize;
    uint32 instanceId = packet.Hdr.InstanceID;
    uint32 mapId = packet.Mapid;
    int8 arenaFaction = packet.ArenaFaction;

    // Get BG type from queue IDs
    uint32 bgTypeId = 0;
    if (!packet.Hdr.QueueID.empty())
        bgTypeId = ExtractBGTypeFromQueueID(packet.Hdr.QueueID[0]);

    TC_LOG_DEBUG("playerbot.packets", "BG Status Active: {} {} entered BG type {} (map={}, instance={}, startTimer={})",
        isBot ? "Bot" : "Player",
        player->GetName(),
        bgTypeId,
        mapId,
        instanceId,
        packet.StartTimer);

    // Create and dispatch entered event
    Events::BotEvent event(StateMachine::EventType::BG_ENTERED, player->GetGUID());

    Events::InstanceEnteredEventData eventData;
    eventData.contentType = IsArenaQueue(teamSize) ? Events::ContentType::Arena : Events::ContentType::Battleground;
    eventData.contentId = bgTypeId;
    eventData.instanceId = instanceId;
    eventData.playerGuid = player->GetGUID();
    eventData.isBot = isBot;
    eventData.timestamp = time(nullptr);

    event.eventData = std::make_any<Events::InstanceEnteredEventData const*>(&eventData);
}

/**
 * @brief SMSG_BATTLEFIELD_STATUS_FAILED - Queue failed
 *
 * Triggered when a BG/Arena queue fails (timeout, error, etc.)
 * Allows cleanup of queue tracking state.
 */
void ParseTypedBattlefieldStatusFailed(WorldSession* session, WorldPackets::Battleground::BattlefieldStatusFailed const& packet)
{
    if (!session)
        return;

    Player* player = session->GetPlayer();
    if (!player)
        return;

    bool isBot = IsBot(player);

    TC_LOG_DEBUG("playerbot.packets", "BG Status Failed: {} {} - queue={}, reason={}",
        isBot ? "Bot" : "Player",
        player->GetName(),
        packet.QueueID,
        packet.Reason);

    // Extract BG type for queue cleanup
    uint32 bgTypeId = ExtractBGTypeFromQueueID(packet.QueueID);

    // Create leave event
    Events::BotEvent event(StateMachine::EventType::BG_QUEUE_LEAVE, player->GetGUID());

    Events::QueueLeaveEventData eventData;
    eventData.contentType = Events::ContentType::Battleground;
    eventData.contentId = bgTypeId;
    eventData.playerGuid = player->GetGUID();
    eventData.isBot = isBot;
    eventData.leaveReason = 3;  // 3 = error
    eventData.timestamp = time(nullptr);

    event.eventData = std::make_any<Events::QueueLeaveEventData const*>(&eventData);
}

/**
 * @brief SMSG_BATTLEFIELD_STATUS_NONE - Queue cleared
 *
 * Triggered when a player is no longer in any queue for a specific slot.
 * Used to clean up queue tracking state.
 */
void ParseTypedBattlefieldStatusNone(WorldSession* session, WorldPackets::Battleground::BattlefieldStatusNone const& packet)
{
    if (!session)
        return;

    Player* player = session->GetPlayer();
    if (!player)
        return;

    bool isBot = IsBot(player);

    TC_LOG_DEBUG("playerbot.packets", "BG Status None: {} {} - queue cleared",
        isBot ? "Bot" : "Player",
        player->GetName());

    // Create leave event
    Events::BotEvent event(StateMachine::EventType::BG_QUEUE_LEAVE, player->GetGUID());

    Events::QueueLeaveEventData eventData;
    eventData.contentType = Events::ContentType::Battleground;
    eventData.contentId = 0;  // Unknown - queue was cleared
    eventData.playerGuid = player->GetGUID();
    eventData.isBot = isBot;
    eventData.leaveReason = 0;  // 0 = cancelled/cleared
    eventData.timestamp = time(nullptr);

    event.eventData = std::make_any<Events::QueueLeaveEventData const*>(&eventData);
}

// ================================================================================================
// HANDLER REGISTRATION
// Called from PlayerbotPacketSniffer::Initialize()
// ================================================================================================

void RegisterBattlegroundPacketHandlers()
{
    // Queue status handlers
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Battleground::BattlefieldStatusQueued>(
        &ParseTypedBattlefieldStatusQueued);

    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Battleground::BattlefieldStatusNeedConfirmation>(
        &ParseTypedBattlefieldStatusNeedConfirmation);

    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Battleground::BattlefieldStatusActive>(
        &ParseTypedBattlefieldStatusActive);

    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Battleground::BattlefieldStatusFailed>(
        &ParseTypedBattlefieldStatusFailed);

    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Battleground::BattlefieldStatusNone>(
        &ParseTypedBattlefieldStatusNone);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered 5 Battleground packet typed handlers");
}

} // namespace Playerbot
