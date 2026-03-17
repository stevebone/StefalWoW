/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_SOCIAL_EVENTS_H
#define PLAYERBOT_SOCIAL_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>

namespace Playerbot
{

enum class SocialEventType : uint8
{
    MESSAGE_CHAT = 0,
    EMOTE_RECEIVED,
    TEXT_EMOTE_RECEIVED,
    GUILD_INVITE_RECEIVED,
    GUILD_EVENT_RECEIVED,
    TRADE_STATUS_CHANGED,
    MAX_SOCIAL_EVENT
};

enum class SocialEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

enum class ChatMsg : uint8
{
    CHAT_MSG_SAY = 0,
    CHAT_MSG_PARTY = 1,
    CHAT_MSG_RAID = 2,
    CHAT_MSG_GUILD = 3,
    CHAT_MSG_WHISPER = 4,
    CHAT_MSG_YELL = 6,
    CHAT_MSG_CHANNEL = 17
};

enum class Language : uint8
{
    LANG_UNIVERSAL = 0,
    LANG_ORCISH = 1,
    LANG_COMMON = 7
};

struct SocialEvent
{
    using EventType = SocialEventType;
    using Priority = SocialEventPriority;

    SocialEventType type;
    SocialEventPriority priority;
    ObjectGuid playerGuid;
    ObjectGuid targetGuid;
    std::string message;
    ChatMsg chatType;
    Language language;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    // Additional fields for various event types
    std::string senderName;
    std::string channel;
    uint32 emoteId = 0;
    uint32 achievementId = 0;
    uint64 guildId = 0;
    uint8 tradeStatus = 0;

    bool IsValid() const;
    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }
    std::string ToString() const;

    bool operator<(SocialEvent const& other) const
    {
        return priority > other.priority;
    }

    // Helper constructors
    static SocialEvent ChatReceived(ObjectGuid player, ObjectGuid target, std::string msg, ChatMsg type);
    static SocialEvent WhisperReceived(ObjectGuid player, ObjectGuid target, std::string msg);
    static SocialEvent GroupInvite(ObjectGuid player, ObjectGuid inviter);
    static SocialEvent MessageChat(ObjectGuid player, ObjectGuid target, std::string senderName, std::string msg, ChatMsg type, Language lang, std::string channel, uint32 achievementId);
    static SocialEvent EmoteReceived(ObjectGuid player, ObjectGuid target, uint32 emoteId);
    static SocialEvent TextEmoteReceived(ObjectGuid player, ObjectGuid target, uint32 emoteId);
    static SocialEvent GuildInviteReceived(ObjectGuid player, ObjectGuid target, std::string inviterName, uint64 guildId);
    static SocialEvent GuildEventReceived(ObjectGuid player, uint64 guildId, std::string message);
    static SocialEvent TradeStatusChanged(ObjectGuid partner, ObjectGuid player, uint8 status);
};

} // namespace Playerbot

#endif // PLAYERBOT_SOCIAL_EVENTS_H
