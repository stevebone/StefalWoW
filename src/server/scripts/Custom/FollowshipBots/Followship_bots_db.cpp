#include "Followship_bots_db.h"
#include "DatabaseEnv.h"
#include "CharacterDatabase.h"

namespace FSBUtilsDB
{
    bool LoadBotsForPlayer(uint64 playerGuidLow, std::vector<PlayerBotData>& outBots)
    {
        outBots.clear();

        QueryResult result = CharacterDatabase.PQuery(
            "SELECT bot_id, bot_guid, bot_entry, hire_expiry_time "
            "FROM followship_bot_owners "
            "WHERE player_guid = {}",
            playerGuidLow);

        if (!result)
            return true; // no bots is valid

        do
        {
            Field* fields = result->Fetch();

            PlayerBotData data;
            data.botId = fields[0].GetUInt32();
            data.spawnId = fields[1].GetUInt64(); // rename!
            data.entry = fields[2].GetUInt32();
            data.hireExpiry = fields[3].GetUInt64();
            data.runtimeGuid = ObjectGuid::Empty;

            outBots.push_back(data);
        } while (result->NextRow());

        return true;
    }

    bool LoadBotOwners(std::unordered_map<ObjectGuid::LowType, ObjectGuid::LowType>& botOwners)
    {
        botOwners.clear();

        QueryResult result = CharacterDatabase.Query(
            "SELECT bot_guid, player_guid, hire_expiry_time FROM followship_bot_owners");

        if (!result)
            return true; // no bots is valid

        do
        {
            Field* fields = result->Fetch();

            ObjectGuid::LowType spawnId = fields[0].GetUInt64(); // bot_guid
            ObjectGuid::LowType ownerGuid = fields[1].GetUInt32(); // player_guid
            uint64 hireExpiry = fields[2].GetUInt64();

            // Skip expired bots
            if (hireExpiry > 0 && hireExpiry <= static_cast<uint64>(time(nullptr)))
            {
                // Clean up DB entry
                CharacterDatabase.PExecute(
                    "DELETE FROM followship_bot_owners WHERE bot_guid = {}", spawnId);

                TC_LOG_DEBUG("scripts.ai.fsb",
                    "FSB Bot MGR: Removing expired bot {} (owner {})", spawnId, ownerGuid);

                continue;
            }


            botOwners[spawnId] = ownerGuid;

        } while (result->NextRow());

        return true;
    }

    bool SaveBotToDB(Creature* bot, Player* player, uint64 hireExpiry)
    {
        ASSERT(bot && player);

        ObjectGuid::LowType spawnId = bot->GetSpawnId();
        uint32 playerGuidLow = player->GetGUID().GetCounter();

        if (!spawnId)
        {
            TC_LOG_ERROR("scripts.ai.fsb",
                "FSB DB: Attempted to save non-DB bot (entry {})", bot->GetEntry());
            return false;
        }

        uint32 entry = bot->GetEntry();

        CharacterDatabase.PExecute(
            "INSERT INTO followship_bot_owners (bot_guid, bot_entry, player_guid, hire_expiry_time) "
            "VALUES ({}, {}, {}, {})",
            spawnId,
            entry,
            playerGuidLow,
            hireExpiry);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB DB: Bot: {} was saved to DB for player: {} with expiry time: {}", bot->GetName(), player->GetName(), hireExpiry);

        // 2?? Retry select until we get the bot_id
        uint32 botId = 0;
        for (int attempts = 0; attempts < 5; ++attempts)
        {
            QueryResult result = CharacterDatabase.PQuery(
                "SELECT bot_id FROM followship_bot_owners "
                "WHERE bot_guid = {} AND player_guid = {}",
                spawnId, playerGuidLow);

            if (result)
            {
                botId = result->Fetch()[0].GetUInt32();
                if (botId != 0)
                    break;
            }

            TC_LOG_WARN("scripts.ai.fsb",
                "FSB DB: bot_id not yet available for bot spawn {} player {}, retrying...",
                spawnId, player->GetName());

            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // small delay
        }

        if (!botId)
        {
            TC_LOG_ERROR("scripts.ai.fsb",
                "FSB DB: Failed to retrieve bot_id for bot_guid {} player {}", spawnId, player->GetName());
        }
        else
        {
            TC_LOG_DEBUG("scripts.ai.fsb",
                "FSB DB: Saved bot {} for player {} with bot_id {}", bot->GetName(), player->GetName(), botId);
        }

        return botId;
    }

    bool DeleteBotByEntry(uint32 bot_entry, uint32 player_guid)
    {
        if (!bot_entry || !player_guid)
            return false;

        CharacterDatabase.PExecute("DELETE FROM followship_bot_owners WHERE bot_entry = {} AND player_guid = {} ", bot_entry, player_guid);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB DB: Bot with bot_entry: {} was deleted", bot_entry);

        return true;
    }
}
