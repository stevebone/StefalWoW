/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SocialEvents.h"
#include <sstream>

namespace Playerbot
{

SocialEvent SocialEvent::ChatReceived(ObjectGuid player, ObjectGuid target, std::string msg, ChatMsg type)
{
    SocialEvent event;
    event.type = SocialEventType::MESSAGE_CHAT;
    event.priority = SocialEventPriority::MEDIUM;
    event.playerGuid = player;
    event.targetGuid = target;
    event.message = std::move(msg);
    event.chatType = type;
    event.language = Language::LANG_UNIVERSAL;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::seconds(30);
    return event;
}

SocialEvent SocialEvent::WhisperReceived(ObjectGuid player, ObjectGuid target, std::string msg)
{
    SocialEvent event;
    event.type = SocialEventType::MESSAGE_CHAT;
    event.priority = SocialEventPriority::HIGH;
    event.playerGuid = player;
    event.targetGuid = target;
    event.message = std::move(msg);
    event.chatType = ChatMsg::CHAT_MSG_WHISPER;
    event.language = Language::LANG_UNIVERSAL;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::seconds(30);
    return event;
}

SocialEvent SocialEvent::GroupInvite(ObjectGuid player, ObjectGuid inviter)
{
    SocialEvent event;
    event.type = SocialEventType::GUILD_INVITE_RECEIVED;
    event.priority = SocialEventPriority::HIGH;
    event.playerGuid = player;
    event.targetGuid = inviter;
    event.message = "";
    event.chatType = ChatMsg::CHAT_MSG_PARTY;
    event.language = Language::LANG_UNIVERSAL;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::seconds(60);
    return event;
}

SocialEvent SocialEvent::MessageChat(ObjectGuid player, ObjectGuid target, std::string senderName, std::string msg, ChatMsg type, Language lang, std::string channel, uint32 achievementId)
{
    SocialEvent event;
    event.type = SocialEventType::MESSAGE_CHAT;
    event.priority = (type == ChatMsg::CHAT_MSG_WHISPER) ? SocialEventPriority::HIGH : SocialEventPriority::MEDIUM;
    event.playerGuid = player;
    event.targetGuid = target;
    event.senderName = std::move(senderName);
    event.message = std::move(msg);
    event.chatType = type;
    event.language = lang;
    event.channel = std::move(channel);
    event.achievementId = achievementId;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::seconds(30);
    return event;
}

SocialEvent SocialEvent::EmoteReceived(ObjectGuid player, ObjectGuid target, uint32 emoteId)
{
    SocialEvent event;
    event.type = SocialEventType::EMOTE_RECEIVED;
    event.priority = SocialEventPriority::LOW;
    event.playerGuid = player;
    event.targetGuid = target;
    event.emoteId = emoteId;
    event.chatType = ChatMsg::CHAT_MSG_SAY;
    event.language = Language::LANG_UNIVERSAL;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::seconds(10);
    return event;
}

SocialEvent SocialEvent::TextEmoteReceived(ObjectGuid player, ObjectGuid target, uint32 emoteId)
{
    SocialEvent event;
    event.type = SocialEventType::TEXT_EMOTE_RECEIVED;
    event.priority = SocialEventPriority::LOW;
    event.playerGuid = player;
    event.targetGuid = target;
    event.emoteId = emoteId;
    event.chatType = ChatMsg::CHAT_MSG_SAY;
    event.language = Language::LANG_UNIVERSAL;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::seconds(10);
    return event;
}

SocialEvent SocialEvent::GuildInviteReceived(ObjectGuid player, ObjectGuid target, std::string inviterName, uint64 guildId)
{
    SocialEvent event;
    event.type = SocialEventType::GUILD_INVITE_RECEIVED;
    event.priority = SocialEventPriority::HIGH;
    event.playerGuid = player;
    event.targetGuid = target;
    event.senderName = std::move(inviterName);
    event.guildId = guildId;
    event.chatType = ChatMsg::CHAT_MSG_GUILD;
    event.language = Language::LANG_UNIVERSAL;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::seconds(60);
    return event;
}

SocialEvent SocialEvent::GuildEventReceived(ObjectGuid player, uint64 guildId, std::string message)
{
    SocialEvent event;
    event.type = SocialEventType::GUILD_EVENT_RECEIVED;
    event.priority = SocialEventPriority::MEDIUM;
    event.playerGuid = player;
    event.targetGuid = ObjectGuid::Empty;
    event.guildId = guildId;
    event.message = std::move(message);
    event.chatType = ChatMsg::CHAT_MSG_GUILD;
    event.language = Language::LANG_UNIVERSAL;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::seconds(30);
    return event;
}

SocialEvent SocialEvent::TradeStatusChanged(ObjectGuid partner, ObjectGuid player, uint8 status)
{
    SocialEvent event;
    event.type = SocialEventType::TRADE_STATUS_CHANGED;
    event.priority = SocialEventPriority::MEDIUM;
    event.playerGuid = player;
    event.targetGuid = partner;
    event.tradeStatus = status;
    event.chatType = ChatMsg::CHAT_MSG_SAY;
    event.language = Language::LANG_UNIVERSAL;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::seconds(30);
    return event;
}

bool SocialEvent::IsValid() const
{
    if (type >= SocialEventType::MAX_SOCIAL_EVENT)
        return false;
    if (timestamp.time_since_epoch().count() == 0)
        return false;
    if (playerGuid.IsEmpty())
        return false;
    return true;
}

std::string SocialEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[SocialEvent] Type: " << static_cast<uint32>(type)
        << ", Player: " << playerGuid.ToString()
        << ", Message: " << message.substr(0, 50);
    return oss.str();
}

} // namespace Playerbot
