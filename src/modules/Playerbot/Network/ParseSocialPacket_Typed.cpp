/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotPacketSniffer.h"
#include "Core/Events/GenericEventBus.h"
#include "Social/SocialEvents.h"
#include "ChatPackets.h"
#include "GuildPackets.h"
#include "TradePackets.h"
#include "Player.h"
#include "WorldSession.h"
#include "Log.h"

namespace Playerbot
{

/**
 * @brief SMSG_CHAT / SMSG_MESSAGECHAT - Chat message received
 */
void ParseTypedChat(WorldSession* session, WorldPackets::Chat::Chat const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    SocialEvent event = SocialEvent::MessageChat(
        packet.SenderGUID,
        bot->GetGUID(),
        packet.SenderName,
        packet.ChatText,
        static_cast<ChatMsg>(packet.SlashCmd),  // WoW 12.0: SlashCmd is uint8, cast to ChatMsg enum
        static_cast<Language>(packet._Language),
        packet._Channel,
        packet.AchievementID
    );

    EventBus<SocialEvent>::instance()->PublishEvent(event);

    TC_LOG_TRACE("playerbot.packets", "Bot {} received CHAT (typed): from={}, type={}, msg={}",
        bot->GetName(), packet.SenderName, packet.SlashCmd, packet.ChatText.substr(0, 50));
}

/**
 * @brief SMSG_EMOTE - Emote received
 */
void ParseTypedEmote(WorldSession* session, WorldPackets::Chat::Emote const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    SocialEvent event = SocialEvent::EmoteReceived(
        packet.Guid,
        bot->GetGUID(),
        packet.EmoteID
    );

    EventBus<SocialEvent>::instance()->PublishEvent(event);

    TC_LOG_TRACE("playerbot.packets", "Bot {} received EMOTE (typed): from={}, emote={}",
        bot->GetName(), packet.Guid.ToString(), packet.EmoteID);
}

/**
 * @brief SMSG_TEXT_EMOTE - Text emote received
 */
void ParseTypedTextEmote(WorldSession* session, WorldPackets::Chat::STextEmote const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    SocialEvent event = SocialEvent::TextEmoteReceived(
        packet.SourceGUID,
        bot->GetGUID(),
        packet.EmoteID
    );

    EventBus<SocialEvent>::instance()->PublishEvent(event);
    TC_LOG_TRACE("playerbot.packets", "Bot {} received TEXT_EMOTE (typed): from={}, emote={}",
        bot->GetName(), packet.SourceGUID.ToString(), packet.EmoteID);
}

/**
 * @brief SMSG_GUILD_INVITE - Guild invite received
 */
void ParseTypedGuildInvite(WorldSession* session, WorldPackets::Guild::GuildInvite const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Get inviter name from session or packet data
    ::std::string inviterName = packet.InviterName;

    SocialEvent event = SocialEvent::GuildInviteReceived(
        packet.InviterVirtualRealmAddress > 0 ? ObjectGuid::Create<HighGuid::Player>(packet.InviterVirtualRealmAddress) : ObjectGuid::Empty,
        bot->GetGUID(),
        inviterName,
        packet.GuildGUID.GetCounter()
    );

    EventBus<SocialEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received GUILD_INVITE (typed): inviter={}, guild={}",
        bot->GetName(), inviterName, packet.GuildGUID.ToString());
}

/**
 * @brief SMSG_GUILD_EVENT - Guild event received (multiple types)
 */
void ParseTypedGuildEvent(WorldSession* session, WorldPackets::Guild::GuildEventPresenceChange const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    Guild* guild = bot->GetGuild();
    if (!guild)
        return;

    ::std::string message = packet.Name + (packet.LoggedOn ? " has come online" : " has gone offline");

    SocialEvent event = SocialEvent::GuildEventReceived(
        bot->GetGUID(),
        guild->GetId(),
        message
    );

    EventBus<SocialEvent>::instance()->PublishEvent(event);

    TC_LOG_TRACE("playerbot.packets", "Bot {} received GUILD_EVENT (typed): guild={}, msg={}",
        bot->GetName(), guild->GetId(), message);
}

/**
 * @brief SMSG_TRADE_STATUS - Trade status changed
 */
void ParseTypedTradeStatus(WorldSession* session, WorldPackets::Trade::TradeStatus const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    SocialEvent event = SocialEvent::TradeStatusChanged(
        packet.Partner,  // WoW 12.0: Field is Partner, not PartnerGuid
        bot->GetGUID(),
        static_cast<uint8>(packet.Status)
    );

    EventBus<SocialEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received TRADE_STATUS (typed): partner={}, status={}",
        bot->GetName(), packet.Partner.ToString(), static_cast<uint32>(packet.Status));
}

/**
 * @brief Register all social packet typed handlers
 */
void RegisterSocialPacketHandlers()
{
    // Register chat handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Chat::Chat>(&ParseTypedChat);

    // Register emote handlers
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Chat::Emote>(&ParseTypedEmote);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Chat::STextEmote>(&ParseTypedTextEmote);

    // Register guild handlers
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Guild::GuildInvite>(&ParseTypedGuildInvite);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Guild::GuildEventPresenceChange>(&ParseTypedGuildEvent);

    // Register trade handler
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Trade::TradeStatus>(&ParseTypedTradeStatus);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} Social packet typed handlers", 6);
}

} // namespace Playerbot
