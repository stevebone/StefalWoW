
#include "Followship_bots_mgr.h"
#include "Followship_bots.h"

namespace FSBMgr
{
    // playerGuidLow -> vector of PlayerBotData
    std::unordered_map<uint64, std::vector<PlayerBotData>> _playerBots;

    void StorePlayerBot(Player* player, PlayerBotData& botData, bool saveToDB)
    {
        ASSERT(botData.spawnId != 0);        // sanity
        ASSERT(botData.entry != 0);
        ASSERT(player);

        uint64 playerGuidLow = player->GetGUID().GetCounter();
        auto& vec = _playerBots[playerGuidLow];

        // Check if bot already exists for this player
        for (auto& existing : vec)
        {
            if (existing.botId == botData.botId)
            {
                // Update existing entry
                existing = botData;

                TC_LOG_DEBUG("scripts.ai.fsb", "FSBMgr: Updated bot {} for player {}.", botData.spawnId, playerGuidLow);

                return;
            }
        }

        if (saveToDB && botData.botId == 0)
        {
            if (Creature* bot = player->GetMap()->GetCreatureBySpawnId(botData.spawnId))
            {
                botData.botId = FSBUtilsDB::SaveBotToDB(bot, player, botData.hireExpiry);
            }
        }

        ASSERT(!saveToDB || botData.botId != 0);

        vec.push_back(botData);

        TC_LOG_DEBUG("scripts.ai.fsb",
            "FSBMgr: Stored bot entry={} spawnId={} for player {}",
            botData.entry, botData.spawnId, player->GetName());

        
    }

    std::vector<PlayerBotData>* GetBotsForPlayer(Player* player)
    {
        uint64 playerGuidLow = player->GetGUID().GetCounter();

        auto it = _playerBots.find(playerGuidLow);
        if (it == _playerBots.end())
        {
            std::vector<PlayerBotData> bots;
            if (!FSBUtilsDB::LoadBotsForPlayer(playerGuidLow, bots))
                return nullptr; // real DB error

            it = _playerBots.emplace(playerGuidLow, std::move(bots)).first;
        }

        return &it->second;
    }

    bool RemovePlayerBot(Player* player, uint32 botEntry, bool isTemp)
    {
        if (!player)
            return false;

        uint32 playerGuidLow = player->GetGUID().GetCounter();

        auto it = _playerBots.find(playerGuidLow);
        if (it == _playerBots.end())
            return false;

        auto& vec = it->second;

        auto vit = std::find_if(vec.begin(), vec.end(),
            [botEntry](PlayerBotData const& b)
            {
                return b.entry == botEntry;
            });

        if (vit == vec.end())
            return false;

        // Remove from DB if requested
        if (!isTemp)
        {
            FSBUtilsDB::DeleteBotByEntry(botEntry, playerGuidLow);

            TC_LOG_DEBUG("scripts.ai.fsb",
                "FSBMgr: Removed DB bot entry {} (botId {}) for player {}",
                botEntry, vit->botId, player->GetName());
        }
        else
        {
            TC_LOG_DEBUG("scripts.ai.fsb",
                "FSBMgr: Removed runtime bot entry {} for player {}",
                botEntry, player->GetName());
        }

        if(!isTemp)
            vec.erase(vit);

        return true;
    }

    uint64 GetBotExpireTime(uint32 durationHours)
    {
        uint64 now = time(nullptr); // seconds since 1970
        return uint64(now) + uint64(durationHours) * 60 * 60;
    }

    bool IsBotExpired(PlayerBotData const& bot)
    {
        return bot.hireExpiry > 0 && bot.hireExpiry < time(nullptr);
    }

    void RemoveExpiredBots(Player* player)
    {
        auto* bots = GetBotsForPlayer(player);
        if (!bots)
            return;

        for (auto it = bots->begin(); it != bots->end(); )
        {
            if (IsBotExpired(*it))
            {
                uint32 botEntry = it->entry;
                uint32 playerGuid = player->GetGUID().GetCounter();
                it = bots->erase(it);
                FSBUtilsDB::DeleteBotByEntry(botEntry, playerGuid);
            }
            else
                ++it;
        }
    }

    void RestoreBotOwnership(Player* player, Creature* bot, uint32 hireTimeLeft)
    {
        if (!player || !bot)
            return;

        // Teleport if too far away
        if (bot->GetMapId() == player->GetMapId() && bot->GetDistance(player) > 100.0f)
        {
            bot->NearTeleportTo(
                player->GetPositionX(),
                player->GetPositionY(),
                player->GetPositionZ(),
                player->GetOrientation());

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Teleported bot {} to player {} due to distance > 100.", bot->GetName(), player->GetName());

        }

        // Set owner + creator GUIDs
        bot->SetOwnerGUID(player->GetGUID());

        bot->AI()->SetData(FSB_DATA_HIRED, 1);
        bot->AI()->SetData(FSB_DATA_HIRE_TIME_LEFT, hireTimeLeft);
    }

    void HandleBotHire(Player* player, Creature* bot, uint32 hireDurationHours)
    {
        ASSERT(player && bot);

        // Calculate expiry
        uint64 hireExpiry = FSBMgr::GetBotExpireTime(hireDurationHours);

        // 1?? Save to DB
        //FSBUtilsDB::SaveBotToDB(bot, player, hireExpiry);

        // 2?? Update memory
        PlayerBotData botData;
        botData.botId = 0; // or actual DB primary key if known
        botData.spawnId = bot->GetSpawnId();
        botData.entry = bot->GetEntry();
        botData.hireExpiry = hireExpiry;
        botData.runtimeGuid = ObjectGuid::Empty;

        StorePlayerBot(player, botData, true);

        // 3?? Restore ownership + AI state
        uint32 hireTimeLeft = hireDurationHours * 3600;
        RestoreBotOwnership(player, bot, hireTimeLeft);
        //FSBMgr::RegisterActiveBot(bot);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Player {} hired bot {} (entry {}) until {}",
            player->GetName(), bot->GetName(), bot->GetEntry(), hireExpiry);
    }

    // Dismiss a bot from a player
    void DismissBot(Player* player, Creature* bot)
    {
        if (!player || !bot)
            return;

        uint32 playerGuid = player->GetGUID().GetCounter();
        auto* bots = FSBMgr::GetBotsForPlayer(player);
        if (!bots)
            return;

        // Match using botId stored in memory
        auto it = std::find_if(bots->begin(), bots->end(),
            [bot](PlayerBotData const& b)
            {
                // Use entry to match bots since it is common to db and temp bots
                return (b.entry != 0 && (b.entry == bot->GetEntry()));
            });

        if (it != bots->end())
        {
            uint32 botEntry = it->entry;
            bots->erase(it);

            if (botEntry != 0)
                FSBUtilsDB::DeleteBotByEntry(botEntry, playerGuid);  // delete DB record regardless of temp/DB

            TC_LOG_DEBUG("scripts.ai.fsb",
                "FSB: Dismissed bot {} (entry {}) for player {}", bot->GetName(), botEntry, player->GetName());
        }

        // Always despawn
        //bot->DespawnOrUnsummon();
    }

    void DespawnTempBots(Player* player)
    {
        ASSERT(player);

        // Get all bots for the player (persistent table)
        auto* bots = GetBotsForPlayer(player);

        if (!bots || bots->empty())
            return;

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: OnLogout - Found {} bots for player {}",
            bots->size(), player->GetName());

        for (PlayerBotData& botData : *bots)
        {
            if (!botData.runtimeGuid)
                continue;

            // runtimeGuid is already a full ObjectGuid
            if (Creature* bot = ObjectAccessor::GetCreature(*player, botData.runtimeGuid))
            {
                TC_LOG_DEBUG("scripts.ai.fsb",
                    "FSB: OnLogout - Despawning bot {} for player {}",
                    bot->GetName(), player->GetName());

                FSBMgr::RemovePlayerBot(player, botData.entry, true);
                bot->DespawnOrUnsummon();

            }

            //botData.runtimeGuid.Clear(); // reset so next login/map change won't try to despawn same creature
            
        }
    }

}
