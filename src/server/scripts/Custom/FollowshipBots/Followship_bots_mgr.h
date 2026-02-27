
#include "Followship_bots_db.h"
#include "Followship_bots_defines.h"
#include "Followship_bots_utils.h"

struct FSBEntryRaceClassMap
{
    uint32 entry;
    FSB_Class botClass;
    FSB_Race botRace;
    uint32 companionSpell;
    FSB_ChatterType chatterType;
    Gender gender;
    uint32 petSource;
};

static constexpr FSBEntryRaceClassMap BotEntryClassTable[] =
{
    // PRIESTS
    { 141508,   FSB_Class::Priest,          FSB_Race::Human, 0, FSB_ChatterType::Neutral, GENDER_UNKNOWN, 0},      // Stormwind Priest
    { 375,      FSB_Class::Priest,          FSB_Race::Human, 10675, FSB_ChatterType::Positive, GENDER_FEMALE, 0 },      // Priestess Anetta
    { 377,      FSB_Class::Priest,          FSB_Race::Human, 70613, FSB_ChatterType::Positive, GENDER_FEMALE, 0 },      // Priestess Josetta
    { 16502,    FSB_Class::Priest,          FSB_Race::Draenei, 0, FSB_ChatterType::Neutral, GENDER_MALE, 0},    // Zalduun
    { 3595,     FSB_Class::Priest,          FSB_Race::NightElf, 0, FSB_ChatterType::Positive, GENDER_FEMALE, 0},   // Shanda
    { 837,      FSB_Class::Priest,          FSB_Race::Dwarf, 0, FSB_ChatterType::Negative, GENDER_MALE, 0},      // Branstock Khalder

    // WARRIORS
    { 911,   FSB_Class::Warrior,            FSB_Race::Human, 0, FSB_ChatterType::Positive, GENDER_MALE, 0 },      // Llane Beshere
    { 912,   FSB_Class::Warrior,            FSB_Race::Dwarf, 0, FSB_ChatterType::Neutral, GENDER_MALE, 0 },      // Thran Khorman
    { 913,   FSB_Class::Warrior,            FSB_Race::Human, 171118, FSB_ChatterType::Positive, GENDER_FEMALE, 0 },      // Lyria Du Lac
    { 50595, FSB_Class::Warrior,            FSB_Race::Human, 0, FSB_ChatterType::Negative, GENDER_UNKNOWN, 0 },      // Stormwind Defender
    { 46405, FSB_Class::Warrior,            FSB_Race::Human, 0, FSB_ChatterType::Negative, GENDER_MALE, 0 },      // Stockade Guard
    { 16503, FSB_Class::Warrior,            FSB_Race::Draenei, 0, FSB_ChatterType::Neutral, GENDER_MALE, 0 },    // Kore
    { 3593,  FSB_Class::Warrior,            FSB_Race::NightElf, 15067, FSB_ChatterType::Positive, GENDER_FEMALE, 0 },   // Alyissia

    // MAGES
    { 198,   FSB_Class::Mage,               FSB_Race::Human, 10697, FSB_ChatterType::Positive, GENDER_MALE, 0 },      // Khelden Bremen
    { 328,   FSB_Class::Mage,               FSB_Race::Human, 10696, FSB_ChatterType::Neutral, GENDER_MALE, 0 },      // Zaldimar Wefhellt
    { 16500, FSB_Class::Mage,               FSB_Race::Draenei, 0, FSB_ChatterType::Negative, GENDER_FEMALE, 0 },    // Valaatu
    { 37121, FSB_Class::Mage,               FSB_Race::Dwarf, 0, FSB_ChatterType::Neutral, GENDER_FEMALE, 0 },      // Teegli Merrowith
    { 43006, FSB_Class::Mage,               FSB_Race::NightElf, 0, FSB_ChatterType::Positive, GENDER_FEMALE, 0 },   // Rhyanda

    // PALADINS
    { 925, FSB_Class::Paladin,              FSB_Race::Human, 0, FSB_ChatterType::Negative, GENDER_MALE, 0 },      // Brother Sammuel
    { 926, FSB_Class::Paladin,              FSB_Race::Dwarf, 0, FSB_ChatterType::Neutral, GENDER_MALE, 0 },      // Bromos Grummner
    { 927, FSB_Class::Paladin,              FSB_Race::Human, 0, FSB_ChatterType::Neutral, GENDER_MALE, 0 },      // Brother Wilhelm
    { 16501, FSB_Class::Paladin,            FSB_Race::Draenei, 408311, FSB_ChatterType::Positive, GENDER_MALE, 0 },    // Aurelon (Draenei)

    // WARLOCKS
    { 459,   FSB_Class::Warlock,            FSB_Race::Human, 153252, FSB_ChatterType::Positive, GENDER_FEMALE, 0 },      // Drusilla la Salle
    { 906,   FSB_Class::Warlock,            FSB_Race::Human, 0, FSB_ChatterType::Positive, GENDER_MALE, 0 },               // Maximillian Crowe
    { 6373,  FSB_Class::Warlock,            FSB_Race::Human, 0, FSB_ChatterType::Positive, GENDER_MALE, 0 },               // Dane Winslow
    { 6374,  FSB_Class::Warlock,            FSB_Race::Human, 0, FSB_ChatterType::Neutral, GENDER_FEMALE, 0 },      // Cylina Darkheart

    // DRUID
    { 3597,  FSB_Class::Druid,            FSB_Race::NightElf, 15067, FSB_ChatterType::Neutral, GENDER_MALE, 0 },   // Mardant

    // ROGUES
    { 915, FSB_Class::Rogue,                FSB_Race::Human, 0, FSB_ChatterType::Neutral, GENDER_MALE, 0 },     // Jorik Kerridan
    { 917, FSB_Class::Rogue,                FSB_Race::Human, 0, FSB_ChatterType::Negative, GENDER_FEMALE, 0 },     // Keryn Sylvius

    // HUNTER
    { 43278,  FSB_Class::Hunter,            FSB_Race::Human, 0, FSB_ChatterType::Negative, GENDER_FEMALE, 43292 /*Adele*/},   // Ashley
    { 46983,  FSB_Class::Hunter,            FSB_Race::Human, 0, FSB_ChatterType::Positive, GENDER_MALE, 46982 /*Sly*/},  //Benjamin Foxworthy

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

    void SetInitialBotState(Creature* bot);

    void SetBotClassAndRace(Creature* creature, FSB_Class& outClass, FSB_Race& outRace);
    bool GetBotClassAndRaceForEntry(uint32 entry, FSB_Class& outClass, FSB_Race& outRace);

    FSB_Class GetBotClassForEntry(uint32 entry);
    void SetBotClass(Creature* creature, FSB_Class& outClass);

    FSB_Race GetBotRaceForEntry(uint32 entry);
    void SetBotRace(Creature* creature, FSB_Race& outRace);

    uint32 GetAvailableRolesForClass(FSB_Class botClass);
    FSB_Roles GetRandomRoleForClass(FSB_Class botClass);

    FSB_ChatterType GetBotChatterTypeForEntry(uint32 entry);

    Gender GetBotGenderForEntry(uint32 entry);

    uint32 GetBotPetSourceForEntry(uint32 entry);

private:
    std::unordered_map<uint64 /*playerGuid*/, std::vector<PlayerBotData>> _playerBotsPersistent;
};
