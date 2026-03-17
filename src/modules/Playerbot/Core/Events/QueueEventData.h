/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file QueueEventData.h
 * @brief Event data structures for queue-related events
 *
 * These structures are used to pass typed data through the EventDispatcher
 * system for BG, LFG, and Arena queue events. They enable the JIT bot
 * creation system to receive rich event data from multiple sources:
 * - QueueStatePoller (periodic polling)
 * - Script hooks (PlayerbotBGScript, etc.)
 * - Packet handlers (typed packet interception)
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"  // BattlegroundTypeId, Team
#include "DBCEnums.h"       // BattlegroundBracketId
#include <ctime>

namespace Playerbot
{
namespace Events
{

// ============================================================================
// CONTENT TYPE ENUM
// ============================================================================

/**
 * @brief Type of instanced content
 */
enum class ContentType : uint8
{
    Battleground = 0,
    Dungeon      = 1,
    Raid         = 2,
    Arena        = 3,
    Delve        = 4
};

// ============================================================================
// QUEUE SHORTAGE EVENT DATA
// ============================================================================

/**
 * @brief Data for queue shortage events
 *
 * Published when QueueStatePoller or packet handlers detect that a queue
 * has insufficient players to start. The QueueShortageSubscriber listens
 * for these events and triggers JIT bot creation.
 *
 * Event Type: BG_QUEUE_SHORTAGE, LFG_QUEUE_SHORTAGE, ARENA_QUEUE_SHORTAGE
 */
struct TC_GAME_API QueueShortageEventData
{
    // Content identification
    ContentType contentType;
    uint32 contentId;          // BG type, dungeon ID, or arena type
    uint32 bracketId;          // Level bracket

    // Faction-based shortages (BG/Arena)
    uint32 allianceInQueue;
    uint32 hordeInQueue;
    uint32 allianceNeeded;
    uint32 hordeNeeded;

    // Role-based shortages (LFG)
    uint32 tankInQueue;
    uint32 healerInQueue;
    uint32 dpsInQueue;
    uint32 tankNeeded;
    uint32 healerNeeded;
    uint32 dpsNeeded;

    // Event metadata
    uint8 priority;            // 1-10 (1 = highest priority)
    time_t timestamp;
    ObjectGuid triggerPlayerGuid;  // Human player that triggered detection

    QueueShortageEventData()
        : contentType(ContentType::Battleground)
        , contentId(0)
        , bracketId(0)
        , allianceInQueue(0)
        , hordeInQueue(0)
        , allianceNeeded(0)
        , hordeNeeded(0)
        , tankInQueue(0)
        , healerInQueue(0)
        , dpsInQueue(0)
        , tankNeeded(0)
        , healerNeeded(0)
        , dpsNeeded(0)
        , priority(5)
        , timestamp(0)
    {}

    /**
     * @brief Check if this is a faction-based shortage (BG/Arena)
     */
    bool IsFactionBased() const
    {
        return contentType == ContentType::Battleground ||
               contentType == ContentType::Arena;
    }

    /**
     * @brief Check if this is a role-based shortage (LFG)
     */
    bool IsRoleBased() const
    {
        return contentType == ContentType::Dungeon ||
               contentType == ContentType::Raid ||
               contentType == ContentType::Delve;
    }

    /**
     * @brief Get total bots needed (all factions/roles)
     */
    uint32 GetTotalNeeded() const
    {
        if (IsFactionBased())
            return allianceNeeded + hordeNeeded;
        return tankNeeded + healerNeeded + dpsNeeded;
    }
};

// ============================================================================
// QUEUE JOIN EVENT DATA
// ============================================================================

/**
 * @brief Data for player joining queue events
 *
 * Published when a human player joins a BG/LFG/Arena queue.
 * Used to trigger immediate polling and JIT checks.
 *
 * Event Type: BG_QUEUE_JOIN, LFG_QUEUE_JOIN, ARENA_QUEUE_JOIN
 */
struct TC_GAME_API QueueJoinEventData
{
    ContentType contentType;
    uint32 contentId;
    uint32 bracketId;
    ObjectGuid playerGuid;
    Team playerTeam;
    uint8 playerRole;          // For LFG: 0=tank, 1=healer, 2=dps
    uint8 playerLevel;
    bool isBot;
    bool isGroupLeader;
    time_t timestamp;

    QueueJoinEventData()
        : contentType(ContentType::Battleground)
        , contentId(0)
        , bracketId(0)
        , playerTeam(static_cast<Team>(0))
        , playerRole(2)  // Default: DPS
        , playerLevel(0)
        , isBot(false)
        , isGroupLeader(false)
        , timestamp(0)
    {}
};

// ============================================================================
// QUEUE LEAVE EVENT DATA
// ============================================================================

/**
 * @brief Data for player leaving queue events
 *
 * Published when a player leaves a queue (cancel, timeout, or BG start).
 * Used to update active queue tracking.
 *
 * Event Type: BG_QUEUE_LEAVE, LFG_QUEUE_LEAVE, ARENA_QUEUE_LEAVE
 */
struct TC_GAME_API QueueLeaveEventData
{
    ContentType contentType;
    uint32 contentId;
    uint32 bracketId;
    ObjectGuid playerGuid;
    bool isBot;
    uint8 leaveReason;         // 0=cancelled, 1=timeout, 2=accepted, 3=error
    time_t timestamp;

    QueueLeaveEventData()
        : contentType(ContentType::Battleground)
        , contentId(0)
        , bracketId(0)
        , isBot(false)
        , leaveReason(0)
        , timestamp(0)
    {}
};

// ============================================================================
// QUEUE INVITATION EVENT DATA
// ============================================================================

/**
 * @brief Data for BG/Arena invitation events
 *
 * Published when a player receives an invitation to join a BG or Arena.
 * Bots use this to auto-accept invitations.
 *
 * Event Type: BG_INVITATION_RECEIVED, ARENA_INVITATION_RECEIVED
 */
struct TC_GAME_API QueueInvitationEventData
{
    ContentType contentType;
    uint32 contentId;
    uint32 instanceId;         // BG instance GUID
    ObjectGuid playerGuid;
    uint32 timeout;            // Seconds until invitation expires
    bool isBot;
    time_t timestamp;

    QueueInvitationEventData()
        : contentType(ContentType::Battleground)
        , contentId(0)
        , instanceId(0)
        , timeout(0)
        , isBot(false)
        , timestamp(0)
    {}
};

// ============================================================================
// LFG PROPOSAL EVENT DATA
// ============================================================================

/**
 * @brief Data for LFG group proposal events
 *
 * Published when an LFG group is formed and proposal is sent.
 * Bots use this to auto-accept proposals.
 *
 * Event Type: LFG_PROPOSAL
 */
struct TC_GAME_API LFGProposalEventData
{
    uint32 proposalId;
    uint32 dungeonId;
    ObjectGuid playerGuid;
    uint8 proposalState;       // 0=pending, 1=accepted, 2=declined
    uint8 playerRole;
    bool isBot;
    time_t timestamp;

    LFGProposalEventData()
        : proposalId(0)
        , dungeonId(0)
        , proposalState(0)
        , playerRole(2)
        , isBot(false)
        , timestamp(0)
    {}
};

// ============================================================================
// QUEUE STATUS UPDATE EVENT DATA
// ============================================================================

/**
 * @brief Data for queue status updates
 *
 * Published periodically with current queue composition.
 * Used for monitoring and statistics.
 *
 * Event Type: BG_QUEUE_UPDATE, LFG_QUEUE_UPDATE, ARENA_QUEUE_UPDATE
 */
struct TC_GAME_API QueueStatusUpdateEventData
{
    ContentType contentType;
    uint32 contentId;
    uint32 bracketId;

    // Queue composition
    uint32 allianceCount;
    uint32 hordeCount;
    uint32 tankCount;
    uint32 healerCount;
    uint32 dpsCount;

    // Requirements
    uint32 minPlayers;
    uint32 maxPlayers;

    // Estimated wait time (seconds)
    uint32 estimatedWaitTime;

    time_t timestamp;

    QueueStatusUpdateEventData()
        : contentType(ContentType::Battleground)
        , contentId(0)
        , bracketId(0)
        , allianceCount(0)
        , hordeCount(0)
        , tankCount(0)
        , healerCount(0)
        , dpsCount(0)
        , minPlayers(0)
        , maxPlayers(0)
        , estimatedWaitTime(0)
        , timestamp(0)
    {}
};

// ============================================================================
// INSTANCE ENTERED EVENT DATA
// ============================================================================

/**
 * @brief Data for player entering instance events
 *
 * Published when a player enters BG/Dungeon/Raid/Arena.
 * Used to track active instances and cleanup queue tracking.
 *
 * Event Type: BG_ENTERED, DUNGEON_ENTERED, ARENA_ENTERED
 */
struct TC_GAME_API InstanceEnteredEventData
{
    ContentType contentType;
    uint32 contentId;
    uint32 instanceId;
    ObjectGuid playerGuid;
    bool isBot;
    time_t timestamp;

    InstanceEnteredEventData()
        : contentType(ContentType::Battleground)
        , contentId(0)
        , instanceId(0)
        , isBot(false)
        , timestamp(0)
    {}
};

} // namespace Events
} // namespace Playerbot
