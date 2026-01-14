#include "Followship_bots_mgr.h"
#include "Followship_bots.h"
#include "Followship_bots_priest.h"

class followship_bots_player : public PlayerScript
{
public:
    followship_bots_player() : PlayerScript("followship_bots_player") { }
    
    void OnLogout(Player* player) override
    {
        FSBMgr::RemoveTempBots(player);
    }

    void OnMapChanged(Player* player) override
    {
        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: OnMapChanged - Triggered");

        FSBMgr::RemoveExpiredBots(player);
        FSBMgr::RemoveTempBots(player);

        auto* bots = FSBMgr::GetBotsForPlayer(player);
        if (!bots || bots->empty())
        {
            TC_LOG_DEBUG("scripts.ai.fsb",
                "FSB: OnMapChanged - Player {} has no bots", player->GetName());
            return;
        }

        uint64 now = time(nullptr);

        for (auto it = bots->begin(); it != bots->end(); )
        {
            PlayerBotData& botData = *it;

            uint32 hireTimeLeft = botData.hireExpiry > 0
                ? uint32(botData.hireExpiry - now)
                : 0;

            Creature* bot = nullptr;

            // Try to find DB bot by spawnId
            if (botData.spawnId != 0)
                bot = player->GetMap()->GetCreatureBySpawnId(botData.spawnId);

            // If not found ? spawn runtime temp
            if (!bot)
            {
                Position pos = player->GetPosition();
                bot = player->SummonCreature(botData.entry, pos, TEMPSUMMON_MANUAL_DESPAWN, 0s);

                if (!bot)
                {
                    TC_LOG_ERROR("scripts.ai.fsb",
                        "FSB: OnMapChanged - Failed to summon bot entry {} for player {}",
                        botData.entry, player->GetName());
                    ++it;
                    continue;
                }

                botData.runtimeGuid = bot->GetGUID();
                // Mark as temp and register
                FSBMgr::StorePlayerBot(player, botData, false);

                TC_LOG_DEBUG("scripts.ai.fsb",
                    "FSB: OnMapChanged - Spawned TEMP bot {} for player {} on new map",
                    bot->GetName(), player->GetName());
            }
            else
            {
                TC_LOG_DEBUG("scripts.ai.fsb",
                    "FSB: OnMapChanged - Found existing DB bot {} on new map for player {}",
                    bot->GetName(), player->GetName());
            }

            // Register runtime and restore ownership
            FSBMgr::RestoreBotOwnership(player, bot, hireTimeLeft);

            ++it;
        }
    }
};

void AddSC_followship_bots_player()
{
    new followship_bots_player();
}
