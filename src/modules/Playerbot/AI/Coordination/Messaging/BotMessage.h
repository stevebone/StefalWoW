/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_BOT_MESSAGE_H
#define PLAYERBOT_BOT_MESSAGE_H

#include "MessageTypes.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <chrono>
#include <string>
#include <sstream>
#include <optional>

namespace Playerbot
{

/**
 * @brief Bot-to-bot message for group coordination
 *
 * This structure represents a message sent between bots within a group.
 * Messages are used for claiming actions (interrupts, dispels), announcing
 * status changes (CD usage, death), requesting help, and issuing commands.
 *
 * Thread Safety: This is a value type - safe to copy and pass between threads.
 */
struct BotMessage
{
    // ========================================================================
    // Core Message Fields
    // ========================================================================
    BotMessageType type;        // What kind of message
    MessageScope scope;         // Who should receive it
    ObjectGuid senderGuid;      // Who sent the message
    ObjectGuid groupGuid;       // Which group/raid this belongs to
    uint32 messageId;           // Unique ID for this message
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    // ========================================================================
    // Claim-specific Fields
    // ========================================================================
    ClaimPriority claimPriority;  // For claim resolution
    ClaimStatus claimStatus;      // Current claim status

    // ========================================================================
    // Target/Subject Fields
    // ========================================================================
    ObjectGuid targetGuid;      // Target of the action (e.g., who to dispel)
    uint32 spellId;             // Spell ID (for interrupt, dispel, CD usage)
    uint32 auraId;              // Aura ID (for dispel claims)
    uint32 durationMs;          // Duration in milliseconds
    float value;                // Generic value (healthPct, urgency, etc)

    // ========================================================================
    // Position Fields (for movement commands)
    // ========================================================================
    std::optional<Position> position;

    // ========================================================================
    // Role Targeting
    // ========================================================================
    uint8 targetRole;           // For ROLE_BROADCAST (0=tank, 1=healer, 2=dps)
    uint8 subGroup;             // For SUBGROUP_BROADCAST (1-8)

    // ========================================================================
    // Methods
    // ========================================================================

    bool IsValid() const
    {
        if (type >= BotMessageType::MAX_MESSAGE_TYPE)
            return false;
        if (senderGuid.IsEmpty())
            return false;
        if (timestamp.time_since_epoch().count() == 0)
            return false;
        return true;
    }

    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }

    bool IsClaim() const
    {
        return IsClaimMessage(type);
    }

    bool IsAnnouncement() const
    {
        return IsAnnouncementMessage(type);
    }

    bool IsRequest() const
    {
        return IsRequestMessage(type);
    }

    bool IsCommand() const
    {
        return IsCommandMessage(type);
    }

    std::string ToString() const
    {
        std::ostringstream oss;
        oss << "[BotMessage] Type: " << GetMessageTypeName(type)
            << ", From: " << senderGuid.ToString();
        if (!targetGuid.IsEmpty())
            oss << ", Target: " << targetGuid.ToString();
        if (spellId > 0)
            oss << ", Spell: " << spellId;
        if (durationMs > 0)
            oss << ", Duration: " << durationMs << "ms";
        return oss.str();
    }

    // Priority comparison for queue ordering
    bool operator<(BotMessage const& other) const
    {
        // Higher priority claims come first (lower numeric value = higher priority)
        if (IsClaim() && other.IsClaim())
            return claimPriority > other.claimPriority;
        // Commands have highest priority
        if (IsCommand() && !other.IsCommand())
            return true;
        if (!IsCommand() && other.IsCommand())
            return false;
        // Then requests
        if (IsRequest() && !other.IsRequest())
            return true;
        if (!IsRequest() && other.IsRequest())
            return false;
        // Default: by timestamp (older first)
        return timestamp > other.timestamp;
    }

    // ========================================================================
    // Static Factory Methods
    // ========================================================================

    static BotMessage ClaimInterrupt(ObjectGuid sender, ObjectGuid group, ObjectGuid target,
                                     uint32 spellId, ClaimPriority priority)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CLAIM_INTERRUPT;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = target;
        msg.spellId = spellId;
        msg.claimPriority = priority;
        msg.claimStatus = ClaimStatus::PENDING;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(200);  // 200ms claim window
        return msg;
    }

    static BotMessage ClaimDispel(ObjectGuid sender, ObjectGuid group, ObjectGuid target,
                                  uint32 auraId, ClaimPriority priority)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CLAIM_DISPEL;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = target;
        msg.auraId = auraId;
        msg.claimPriority = priority;
        msg.claimStatus = ClaimStatus::PENDING;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(200);
        return msg;
    }

    static BotMessage ClaimDefensiveCD(ObjectGuid sender, ObjectGuid group, ObjectGuid target,
                                        uint32 spellId, ClaimPriority priority)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CLAIM_DEFENSIVE_CD;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = target;
        msg.spellId = spellId;
        msg.claimPriority = priority;
        msg.claimStatus = ClaimStatus::PENDING;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(200);
        return msg;
    }

    static BotMessage ClaimCC(ObjectGuid sender, ObjectGuid group, ObjectGuid target,
                              uint32 spellId, ClaimPriority priority)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CLAIM_CC;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = target;
        msg.spellId = spellId;
        msg.claimPriority = priority;
        msg.claimStatus = ClaimStatus::PENDING;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(200);
        return msg;
    }

    static BotMessage ClaimSoak(ObjectGuid sender, ObjectGuid group, ObjectGuid target,
                                ClaimPriority priority)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CLAIM_SOAK;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = target;
        msg.claimPriority = priority;
        msg.claimStatus = ClaimStatus::PENDING;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(200);
        return msg;
    }

    static BotMessage RequestExternalCD(ObjectGuid sender, ObjectGuid group, ObjectGuid target, float urgency)
    {
        BotMessage msg{};
        msg.type = BotMessageType::REQUEST_EXTERNAL_CD;
        msg.scope = MessageScope::ROLE_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = target;
        msg.targetRole = 1;  // Healers primarily provide externals
        msg.value = urgency;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(2000);
        return msg;
    }

    static BotMessage AnnounceCDUsage(ObjectGuid sender, ObjectGuid group,
                                       uint32 spellId, uint32 durationMs)
    {
        BotMessage msg{};
        msg.type = BotMessageType::ANNOUNCE_CD_USAGE;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.spellId = spellId;
        msg.durationMs = durationMs;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(5000);
        return msg;
    }

    static BotMessage AnnounceBurstWindow(ObjectGuid sender, ObjectGuid group, uint32 durationMs)
    {
        BotMessage msg{};
        msg.type = BotMessageType::ANNOUNCE_BURST_WINDOW;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.durationMs = durationMs;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(durationMs);
        return msg;
    }

    static BotMessage RequestHeal(ObjectGuid sender, ObjectGuid group, float healthPct, float urgency)
    {
        BotMessage msg{};
        msg.type = BotMessageType::REQUEST_HEAL;
        msg.scope = MessageScope::ROLE_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = sender;  // Requesting heal for self
        msg.targetRole = 1;  // Healers
        msg.value = urgency;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(2000);
        return msg;
    }

    static BotMessage RequestTankSwap(ObjectGuid sender, ObjectGuid group, ObjectGuid bossGuid,
                                      uint8 debuffStacks, ClaimPriority priority = ClaimPriority::HIGH)
    {
        BotMessage msg{};
        msg.type = BotMessageType::REQUEST_TANK_SWAP;
        msg.scope = MessageScope::ROLE_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = bossGuid;
        msg.targetRole = 0;  // Tanks
        msg.claimPriority = priority;
        msg.value = static_cast<float>(debuffStacks);
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(3000);
        return msg;
    }

    static BotMessage CommandFocusTarget(ObjectGuid sender, ObjectGuid group, ObjectGuid target)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CMD_FOCUS_TARGET;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = target;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(10000);
        return msg;
    }

    static BotMessage CommandStack(ObjectGuid sender, ObjectGuid group, Position const& pos)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CMD_STACK;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.position = pos;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(5000);
        return msg;
    }

    static BotMessage CommandSpread(ObjectGuid sender, ObjectGuid group)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CMD_SPREAD;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(5000);
        return msg;
    }

    static BotMessage CommandBloodlust(ObjectGuid sender, ObjectGuid group)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CMD_BLOODLUST;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(1000);
        return msg;
    }

    static BotMessage CommandWipeRecovery(ObjectGuid sender, ObjectGuid group)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CMD_WIPE_RECOVERY;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(30000);
        return msg;
    }

    static BotMessage CommandStopDps(ObjectGuid sender, ObjectGuid group)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CMD_STOP_DPS;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(5000);
        return msg;
    }

    static BotMessage CommandUseDefensives(ObjectGuid sender, ObjectGuid group)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CMD_USE_DEFENSIVES;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(3000);
        return msg;
    }

    static BotMessage ClaimResurrect(ObjectGuid sender, ObjectGuid group, ObjectGuid target,
                                     ClaimPriority priority = ClaimPriority::MEDIUM)
    {
        BotMessage msg{};
        msg.type = BotMessageType::CLAIM_RESURRECT;
        msg.scope = MessageScope::GROUP_BROADCAST;
        msg.senderGuid = sender;
        msg.groupGuid = group;
        msg.targetGuid = target;
        msg.claimPriority = priority;
        msg.timestamp = std::chrono::steady_clock::now();
        msg.expiryTime = msg.timestamp + std::chrono::milliseconds(10000);
        return msg;
    }
};

} // namespace Playerbot

#endif // PLAYERBOT_BOT_MESSAGE_H
