
#include "Followship_bots_db.h"
#include "Followship_bots_defines.h"
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
    { 3595,     FSB_Class::Priest,          FSB_Race::NightElf },   // Shanda

    // WARRIORS
    { 911,   FSB_Class::Warrior,            FSB_Race::Human },      // Llane Beshere
    { 50595, FSB_Class::Warrior,            FSB_Race::Human },      // Stormwind Defender
    { 46405, FSB_Class::Warrior,            FSB_Race::Human },      // Stockade Guard
    { 16503, FSB_Class::Warrior,            FSB_Race::Draenei },    // Kore
    { 3593,  FSB_Class::Warrior,            FSB_Race::NightElf },   // Alyissia

    // MAGES
    { 198,   FSB_Class::Mage,               FSB_Race::Human },      // Khelden Bremen
    { 16500, FSB_Class::Mage,               FSB_Race::Draenei },     // Valaatu
    { 43006, FSB_Class::Mage,               FSB_Race::NightElf },   // Rhyanda

    // PALADINS
    { 925, FSB_Class::Paladin,              FSB_Race::Human },      // Brother Sammuel
    { 16501, FSB_Class::Paladin,            FSB_Race::Draenei },    // Aurelon (Draenei)

    // WARLOCKS
    { 459,   FSB_Class::Warlock,            FSB_Race::Human },      // Drusilla la Salle

    // DRUID
    { 3597,  FSB_Class::Druid,            FSB_Race::NightElf },   // Mardant

    // ROGUES
    //{ 90030, FSB_Class::Rogue },
};



class FSBMgr
{
public:
    static FSBMgr* Get();

    // Persistent Layer - with DB relation
    void LoadAllPersistentBots();
    bool StorePersistentBot(Creature* bot, Player* player, uint64 hireExpiry);

    void LoadPersistentPlayerBots(Player* player);
    void RemovePersistentExpiredPlayerBots(Player* player);
    bool RemovePersistentBot(uint64 playerGuid, uint32 botEntry);

    void SpawnPlayerBots(Player* player);

    // Getters
    std::vector<PlayerBotData>* GetPersistentBotsForPlayer(Player* player);
    PlayerBotData* GetPersistentBotBySpawnId(uint64 spawnId);
    Player* GetBotOwner(Unit* unit);

    // Checks the persistent container directly
    bool IsPersistentBotExpired(uint64 ownerGuid, uint64 botEntry);
    // Checks provided bot data
    bool IsBotExpired(PlayerBotData const& bot);


    // Bot Management
    void HirePersistentBot(Player* player, Creature* bot, uint32 hireDurationHours);
    void DismissPersistentBot(Creature* bot);

    void RegisterBotSpawn(Creature* bot, Player* owner);

    bool CheckPlayerHasBotWithEntry(Player* player, uint32 entry);
    bool IsBotOwnedByPlayer(Player* player, Creature* bot);
    bool IsBotOwned(Creature* bot);

    uint64 GetBotExpireTime(uint32 durationHours);
    
    void RestoreBotOwnership(Player* player, Creature* bot, uint32 hireTimeLeft);

private:
    std::unordered_map<uint64 /*playerGuid*/, std::vector<PlayerBotData>> _playerBotsPersistent;
};
