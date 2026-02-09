
#include "Followship_bots_db.h"



namespace FSBMgr
{
    void BotManagerInit();

    void StorePlayerBot(Player* player, PlayerBotData& botData, bool saveToDB);
    std::vector<PlayerBotData>* GetBotsForPlayer(Player* player);
    bool RemovePlayerBot(Player* player, uint32 botEntry, bool isTemp);
    

    void HandleBotHire(Player* player, Creature* bot, uint32 hireDurationHours);
    void DismissBot(Player* player, Creature* bot);
    void RestoreBotOwnership(Player* player, Creature* bot, uint32 hireTimeLeft);

    uint64 GetBotExpireTime(uint32 durationHours);
    bool IsBotExpired(PlayerBotData const& bot);
    void RemoveExpiredBots(Player* player);

    void RemoveTempBots(Player* player);

    bool CheckPlayerHasBotWithEntry(Player* player, uint32 entry);
    bool IsBotOwnedByPlayer(Player* player, Creature* bot);
    bool IsBotOwned(Creature* bot);

    Player* GetBotOwner(Unit* unit);

    void AddBotOwner(ObjectGuid::LowType spawnId, ObjectGuid::LowType ownerGuid);
    void RemoveBotOwner(ObjectGuid::LowType spawnId);
}
