#pragma once

#include <cstdint>

#include "Player.h"
#include "Creature.h"

#include "Followship_bots_defines.h"

namespace FSBUtilsDB
{
    bool LoadAllPersistentBotsFromDB(std::vector<PlayerBotData>& outBots);

    bool LoadBotOwners(std::unordered_map<ObjectGuid::LowType, ObjectGuid::LowType>& botOwners);
    bool LoadBotsForPlayer(uint64 playerGuidLow, std::vector<PlayerBotData>& outBots);
    
    bool SaveBotToDB(Creature* bot, Player* player, uint64 hireExpiry);
    bool DeleteBotByEntry(uint32 bot_entry, uint32 player_guid);    
}
