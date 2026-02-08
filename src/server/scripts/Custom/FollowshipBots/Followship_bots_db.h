#pragma once

#include <cstdint>

#include "Player.h"
#include "Creature.h"

struct PlayerBotData
{
    ObjectGuid::LowType owner   = 0;
    uint64 botId                = 0;       // logical ID from DB
    ObjectGuid::LowType spawnId = 0; // spawn id
    uint32 entry                = 0;
    uint64 hireExpiry           = 0;
    ObjectGuid runtimeGuid      = ObjectGuid::Empty;

    PlayerBotData() = default;
};

namespace FSBUtilsDB
{
    bool LoadBotOwners(std::unordered_map<ObjectGuid::LowType, ObjectGuid::LowType>& botOwners);
    bool LoadBotsForPlayer(uint64 playerGuidLow, std::vector<PlayerBotData>& outBots);
    
    bool SaveBotToDB(Creature* bot, Player* player, uint64 hireExpiry);
    bool DeleteBotByEntry(uint32 bot_entry, uint32 player_guid);

    
}
