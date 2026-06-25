/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 */

#include "Followship_bots_ai_base.h"
#include "Followship_bots_chat_handler.h"
#include "Handlers/Followship_bots_dungeon_handler.h"

FSB_BaseAI::~FSB_BaseAI()
{
    FSBChatMgr::Get()->UnregisterActiveBot(me);
    delete botDungeonData;
}

void FSB_BaseAI::AddChatMemory(uint32 channelId, std::string const& sender, std::string const& msg, bool isPlayer)
{
    BotChatMemoryEntry entry;
    entry.channelId = channelId;
    entry.senderName = sender;
    entry.message = msg;
    entry.isPlayer = isPlayer;
    entry.timestamp = getMSTime();
    botChatMemory.push_back(std::move(entry));

    uint32 maxSize = FollowshipBotsConfig::configFSBBotChatMemorySize;
    if (maxSize < 1)
        maxSize = 10;

    while (botChatMemory.size() > maxSize)
        botChatMemory.pop_front();
}

std::deque<BotChatMemoryEntry> const& FSB_BaseAI::GetChatMemory() const
{
    return botChatMemory;
}
