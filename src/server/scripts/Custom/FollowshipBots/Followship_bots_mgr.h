
#include "Followship_bots_db.h"
#include "Followship_bots_utils.h"

struct FSBEntryRaceClassMap
{
    uint32 entry;
    FSB_Class botClass;
    FSB_Race botRace;
};

static constexpr FSBEntryRaceClassMap BotEntryClassTable[] =
{
    // PRIESTS
    { 141508,   FSB_Class::Priest,          FSB_Race::Human },      // Stormwind Priest
    { 375,      FSB_Class::Priest,          FSB_Race::Human },      // Priestess Anetta
    { 16502,    FSB_Class::Priest,          FSB_Race::Draenei },    // Zalduun

    // WARRIORS
    { 911, FSB_Class::Warrior,              FSB_Race::Human },      // Llane Beshere
    { 50595, FSB_Class::Warrior,            FSB_Race::Human },      // Stormwind Defender
    { 46405, FSB_Class::Warrior,            FSB_Race::Human },      // Stockade Guard
    { 16503, FSB_Class::Warrior,            FSB_Race::Draenei },     // Kore

    // MAGES
    { 198, FSB_Class::Mage,                 FSB_Race::Human },      // Khelden Bremen
    { 16500, FSB_Class::Mage,               FSB_Race::Draenei },     // Valaatu

    // PALADINS
    { 925, FSB_Class::Paladin,              FSB_Race::Human },      // Brother Sammuel
    { 16501, FSB_Class::Paladin,            FSB_Race::Draenei },    // Aurelon (Draenei)

    // WARLOCKS
    { 459,   FSB_Class::Warlock,            FSB_Race::Human },      // Drusilla la Salle

    // ROGUES
    //{ 90030, FSB_Class::Rogue },
};

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
