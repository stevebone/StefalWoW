#pragma once
#include "Player.h"
#include "Creature.h"
#include <cstdint>

struct PlayerBotData
{
    uint64 botId;       // logical ID from DB
    ObjectGuid::LowType spawnId; // spawn id
    uint32 entry;
    uint64 hireExpiry;
    ObjectGuid runtimeGuid;
};

namespace FSBUtilsDB
{
    bool LoadBotsForPlayer(uint64 playerGuidLow, std::vector<PlayerBotData>& outBots);
    
    bool SaveBotToDB(Creature* bot, Player* player, uint64 hireExpiry);
    bool DeleteBotByEntry(uint32 bot_entry, uint32 player_guid);

    
}
