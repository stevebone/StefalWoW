/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_MESSAGE_TYPES_H
#define PLAYERBOT_MESSAGE_TYPES_H

#include "Define.h"

namespace Playerbot
{

/**
 * @brief Bot-to-bot message types for group coordination
 *
 * These message types enable direct communication between bots
 * for coordinating actions like interrupts, dispels, and defensive CDs.
 */
enum class BotMessageType : uint8
{
    // ========================================================================
    // CLAIMS - "I will handle this"
    // ========================================================================
    CLAIM_INTERRUPT,        // I will interrupt this cast
    CLAIM_DISPEL,           // I will dispel this target
    CLAIM_DEFENSIVE_CD,     // I will use my external CD on this target
    CLAIM_CC,               // I will CC this target
    CLAIM_SOAK,             // I will soak this mechanic
    CLAIM_RESURRECT,        // I will resurrect this target

    // ========================================================================
    // ANNOUNCEMENTS - "Info for everyone"
    // ========================================================================
    ANNOUNCE_CD_USAGE,      // I used CD X (spellId, duration)
    ANNOUNCE_BURST_WINDOW,  // Burst window open for X seconds
    ANNOUNCE_POSITION,      // I'm moving to position X
    ANNOUNCE_DEATH,         // I died (killerGuid)
    ANNOUNCE_RESURRECT,     // I'm resurrecting target X
    ANNOUNCE_LOW_RESOURCE,  // My mana/health is low
    ANNOUNCE_CC_APPLIED,    // I CC'd target for X seconds

    // ========================================================================
    // REQUESTS - "I need help"
    // ========================================================================
    REQUEST_HEAL,           // I need healing (urgency, healthPct)
    REQUEST_EXTERNAL_CD,    // I need an external defensive CD
    REQUEST_TANK_SWAP,      // I need a tank swap (debuff stacks)
    REQUEST_RESCUE,         // I'm stuck/OOM/etc, need assistance
    REQUEST_INTERRUPT,      // I can't interrupt, someone else please do it

    // ========================================================================
    // COMMANDS - "Everyone do X" (from leader/coordinator)
    // ========================================================================
    CMD_FOCUS_TARGET,       // Everyone focus this target
    CMD_SPREAD,             // Everyone spread out
    CMD_STACK,              // Everyone stack up
    CMD_MOVE_TO,            // Everyone move to this position
    CMD_USE_DEFENSIVES,     // Everyone use defensive CDs
    CMD_BLOODLUST,          // Use Bloodlust/Heroism now
    CMD_STOP_DPS,           // Stop all DPS (phase transition, etc)
    CMD_WIPE_RECOVERY,      // Begin wipe recovery sequence

    MAX_MESSAGE_TYPE
};

/**
 * @brief Message delivery scope
 */
enum class MessageScope : uint8
{
    GROUP_BROADCAST,        // To all bots in the group/raid
    ROLE_BROADCAST,         // To all bots with a specific role (Tank/Healer/DPS)
    SUBGROUP_BROADCAST,     // To all bots in a raid subgroup (1-8)
    DIRECT,                 // To a specific bot
    NEARBY_BROADCAST,       // To all bots within X yards
};

/**
 * @brief Claim priority for conflict resolution
 */
enum class ClaimPriority : uint8
{
    CRITICAL = 0,           // Highest priority (must-interrupt, healer for dispel)
    HIGH = 1,               // High priority (shortest CD available)
    MEDIUM = 2,             // Normal priority
    LOW = 3,                // Low priority (fallback)
};

/**
 * @brief Claim status
 */
enum class ClaimStatus : uint8
{
    PENDING,                // Claim submitted, awaiting resolution
    GRANTED,                // Claim accepted - proceed with action
    DENIED,                 // Claim rejected - someone else claimed it
    EXPIRED,                // Claim timed out
    RELEASED,               // Claim voluntarily released (bot died, OOM, etc)
};

/**
 * @brief Get string name for message type (for logging)
 */
inline char const* GetMessageTypeName(BotMessageType type)
{
    switch (type)
    {
        case BotMessageType::CLAIM_INTERRUPT: return "CLAIM_INTERRUPT";
        case BotMessageType::CLAIM_DISPEL: return "CLAIM_DISPEL";
        case BotMessageType::CLAIM_DEFENSIVE_CD: return "CLAIM_DEFENSIVE_CD";
        case BotMessageType::CLAIM_CC: return "CLAIM_CC";
        case BotMessageType::CLAIM_SOAK: return "CLAIM_SOAK";
        case BotMessageType::CLAIM_RESURRECT: return "CLAIM_RESURRECT";
        case BotMessageType::ANNOUNCE_CD_USAGE: return "ANNOUNCE_CD_USAGE";
        case BotMessageType::ANNOUNCE_BURST_WINDOW: return "ANNOUNCE_BURST_WINDOW";
        case BotMessageType::ANNOUNCE_POSITION: return "ANNOUNCE_POSITION";
        case BotMessageType::ANNOUNCE_DEATH: return "ANNOUNCE_DEATH";
        case BotMessageType::ANNOUNCE_RESURRECT: return "ANNOUNCE_RESURRECT";
        case BotMessageType::ANNOUNCE_LOW_RESOURCE: return "ANNOUNCE_LOW_RESOURCE";
        case BotMessageType::ANNOUNCE_CC_APPLIED: return "ANNOUNCE_CC_APPLIED";
        case BotMessageType::REQUEST_HEAL: return "REQUEST_HEAL";
        case BotMessageType::REQUEST_EXTERNAL_CD: return "REQUEST_EXTERNAL_CD";
        case BotMessageType::REQUEST_TANK_SWAP: return "REQUEST_TANK_SWAP";
        case BotMessageType::REQUEST_RESCUE: return "REQUEST_RESCUE";
        case BotMessageType::REQUEST_INTERRUPT: return "REQUEST_INTERRUPT";
        case BotMessageType::CMD_FOCUS_TARGET: return "CMD_FOCUS_TARGET";
        case BotMessageType::CMD_SPREAD: return "CMD_SPREAD";
        case BotMessageType::CMD_STACK: return "CMD_STACK";
        case BotMessageType::CMD_MOVE_TO: return "CMD_MOVE_TO";
        case BotMessageType::CMD_USE_DEFENSIVES: return "CMD_USE_DEFENSIVES";
        case BotMessageType::CMD_BLOODLUST: return "CMD_BLOODLUST";
        case BotMessageType::CMD_STOP_DPS: return "CMD_STOP_DPS";
        case BotMessageType::CMD_WIPE_RECOVERY: return "CMD_WIPE_RECOVERY";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Check if message type is a claim
 */
inline bool IsClaimMessage(BotMessageType type)
{
    return type >= BotMessageType::CLAIM_INTERRUPT && type <= BotMessageType::CLAIM_RESURRECT;
}

/**
 * @brief Check if message type is an announcement
 */
inline bool IsAnnouncementMessage(BotMessageType type)
{
    return type >= BotMessageType::ANNOUNCE_CD_USAGE && type <= BotMessageType::ANNOUNCE_CC_APPLIED;
}

/**
 * @brief Check if message type is a request
 */
inline bool IsRequestMessage(BotMessageType type)
{
    return type >= BotMessageType::REQUEST_HEAL && type <= BotMessageType::REQUEST_INTERRUPT;
}

/**
 * @brief Check if message type is a command
 */
inline bool IsCommandMessage(BotMessageType type)
{
    return type >= BotMessageType::CMD_FOCUS_TARGET && type <= BotMessageType::CMD_WIPE_RECOVERY;
}

} // namespace Playerbot

#endif // PLAYERBOT_MESSAGE_TYPES_H
