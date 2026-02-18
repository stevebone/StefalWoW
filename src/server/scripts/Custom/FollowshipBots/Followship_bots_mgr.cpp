#include "Followship_bots_mgr.h"
#include "Followship_bots_db.h"
#include "Followship_bots.h"
#include "PhasingHandler.h"
#include "CharmInfo.h"

#include "Followship_bots_stats_handler.h"

FSBMgr* FSBMgr::Get()
{
    static FSBMgr instance;
    return &instance;
}

// ==================== PERSISTENT LAYER ================================================= //
void FSBMgr::LoadAllPersistentBots()
{
    std::vector<PlayerBotData> allBots;
    if (!FSBUtilsDB::LoadAllPersistentBotsFromDB(allBots))
        return;

    _playerBotsPersistent.clear();

    for (auto const& bot : allBots)
    {
        _playerBotsPersistent[bot.owner].push_back(bot);
    }

    TC_LOG_INFO("scripts.ai.fsb",
        "FSB Mgr: Loaded {} persistent bots for {} players",
        allBots.size(),
        _playerBotsPersistent.size());
}

bool FSBMgr::StorePersistentBot(Creature* bot, Player* player, uint64 hireExpiry)
{
    if (!bot || !player)
        return false;

    uint64 ownerGuid = player->GetGUID().GetCounter();
    uint64 spawnId = bot->GetSpawnId();
    uint32 entry = bot->GetEntry();

    // Build persistent record
    PlayerBotData data;
    data.botId = 0; // if you auto-increment in DB, this will be filled on reload
    data.spawnId = spawnId;
    data.entry = entry;
    data.owner = ownerGuid;
    data.hireExpiry = hireExpiry;
    data.runtimeGuid = bot->GetGUID();

    // Save to DB first
    if (!FSBUtilsDB::SaveBotToDB(bot, player, hireExpiry))
    {
        TC_LOG_ERROR("scripts.ai.fsb",
            "FSB Mgr: Failed to save bot {} for player {} to DB",
            entry, player->GetName());
        return false;
    }

    // Insert into persistent container
    _playerBotsPersistent[ownerGuid].push_back(data);

    TC_LOG_DEBUG("scripts.ai.fsb",
        "FSB Mgr: Stored persistent bot {} for player {} (spawnId {}). Player has now {} bots.",
        entry, player->GetName(), spawnId, _playerBotsPersistent[ownerGuid].size());

    return true;
}

// Load persistent bots - once for player - this is used at first login for example
void FSBMgr::LoadPersistentPlayerBots(Player* player)
{
    if (!player)
        return;

    uint64 guid = player->GetGUID().GetCounter();

    // Already loaded from DB?
    if (_playerBotsPersistent.contains(guid) && _playerBotsPersistent[guid].size() != 0)
    {
        TC_LOG_DEBUG("scripts.ai.fsb", "LoadPlayerBots: already cached for {} and playerBots size: {}", player->GetName(), _playerBotsPersistent[guid].size());
        return;
    }

    std::vector<PlayerBotData> bots;
    if (!FSBUtilsDB::LoadBotsForPlayer(guid, bots))
        return;

    _playerBotsPersistent[guid] = std::move(bots);

    TC_LOG_DEBUG("scripts.ai.fsb",
        "FSB Mgr: Loaded persistent bots for player {} with size {}", player->GetName(), _playerBotsPersistent[guid].size());
}

void FSBMgr::RemovePersistentExpiredPlayerBots(Player* player)
{
    if (!player)
        return;

    uint64 guid = player->GetGUID().GetCounter();

    // Look up persistent bots for this player
    auto it = _playerBotsPersistent.find(guid);
    if (it == _playerBotsPersistent.end())
        return;

    auto& bots = it->second;

    for (auto botIt = bots.begin(); botIt != bots.end(); )
    {
        if (IsBotExpired(*botIt))
        {
            uint32 botEntry = botIt->entry;

            // Remove from DB
            FSBUtilsDB::DeleteBotByEntry(botEntry, guid);

            // Remove from persistent container
            botIt = bots.erase(botIt);

            TC_LOG_DEBUG("scripts.ai.fsb",
                "FSB Mgr: Removed expired bot entry {} for player {}", botEntry, player->GetName());
        }
        else
        {
            ++botIt;
        }
    }

    // If the player now has zero bots, you may optionally erase the key entirely:
    if (bots.empty())
        _playerBotsPersistent.erase(guid);
}

bool FSBMgr::RemovePersistentBot(uint64 playerGuid, uint32 botEntry)
{
    auto it = _playerBotsPersistent.find(playerGuid);
    if (it == _playerBotsPersistent.end())
        return false;

    auto& bots = it->second;

    // Remove from persistent container
    std::erase_if(bots, [&](auto const& b) { return b.entry == botEntry; });

    // Remove from DB
    FSBUtilsDB::DeleteBotByEntry(botEntry, playerGuid);

    TC_LOG_INFO("scripts.ai.fsb",
        "FSB Mgr: Removed 1 persistent bot with entry {} for player guid {}", botEntry, playerGuid);

    // Optional: remove empty entry
    if (bots.empty())
        _playerBotsPersistent.erase(playerGuid);

    return true;
}

void FSBMgr::SpawnPlayerBots(Player* player)
{
    if (!player)
        return;

    // Get persistent bots for this player
    auto* bots = FSBMgr::Get()->GetPersistentBotsForPlayer(player);
    if (!bots || bots->empty())
    {
        TC_LOG_DEBUG("scripts.ai.fsb",
            "FSB: OnMapChanged - Player {} has no persistent bots", player->GetName());
        return;
    }

    uint64 now = time(nullptr);

    for (auto& botData : *bots)
    {
        Creature* bot = nullptr;

        // 1. Try to find the bot by spawnId on this map
        if (botData.spawnId != 0)
            bot = player->GetMap()->GetCreatureBySpawnId(botData.spawnId);

        // 2. If in dungeon and not found, try nearest fallback
        if (!bot && player->GetMap()->IsDungeon())
        {
            bot = player->FindNearestCreature(botData.entry, 500.f, true);
            if (!bot)
                bot = player->FindNearestCreature(botData.entry, 500.f, false);
        }

        // 3. If still not found ? spawn a temporary runtime bot
        if (!bot)
        {
            Position pos = player->GetPosition();
            bot = player->SummonCreature(botData.entry, pos, TEMPSUMMON_MANUAL_DESPAWN, 0s);

            if (!bot)
            {
                TC_LOG_ERROR("scripts.ai.fsb",
                    "FSB: OnMapChanged - Failed to summon bot entry {} for player {}",
                    botData.entry, player->GetName());
                continue;
            }

            botData.runtimeGuid = bot->GetGUID();

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

        // 4. Restore ownership (AI, follow, hire time left)
        uint32 hireTimeLeft = botData.hireExpiry > 0
            ? uint32(botData.hireExpiry - now)
            : 0;

        FSBMgr::Get()->RestoreBotOwnership(player, bot, hireTimeLeft);
    }
}

// -------------------- Bot Ownership --------------------
void FSBMgr::RegisterBotSpawn(Creature* bot, Player* owner)
{
    if (!bot || !owner)
        return;

    auto botsPtr = GetPersistentBotsForPlayer(owner);
    if (!botsPtr)
        return;

    for (auto& botData : *botsPtr)
        if (botData.entry == bot->GetEntry())
            botData.runtimeGuid = bot->GetGUID();
}

void FSBMgr::RestoreBotOwnership(Player* player, Creature* bot, uint32 hireTimeLeft)
{
    if (!player || !bot)
        return;

    // Set owner
    bot->SetOwnerGUID(player->GetGUID());

    bot->AI()->SetData(FSB_DATA_HIRED, 1);
    bot->AI()->SetData(FSB_DATA_HIRE_TIME_LEFT, hireTimeLeft);

    FSBMgr::Get()->RegisterBotSpawn(bot, player);


    PhasingHandler::ResetPhaseShift(bot);
    bot->SetStandState(UNIT_STAND_STATE_STAND);
}

// ==================== GETTER METHODS ==================================================== //
std::vector<PlayerBotData>* FSBMgr::GetPersistentBotsForPlayer(Player* player)
{
    if (!player)
        return nullptr;

    uint64 guid = player->GetGUID().GetCounter();
    auto it = _playerBotsPersistent.find(guid);

    if (it == _playerBotsPersistent.end())
        return nullptr;

    return &it->second;
}

PlayerBotData* FSBMgr::GetPersistentBotBySpawnId(uint64 spawnId)
{
    for (auto& [playerGuid, bots] : _playerBotsPersistent)
    {
        for (auto& bot : bots)
        {
            if (bot.spawnId == spawnId)
                return &bot;
        }
    }
    return nullptr;
}

Player* FSBMgr::GetBotOwner(Unit* unit)
{
    if (!unit)
        return nullptr;

    if (Unit* owner = unit->GetOwner())
        return owner->ToPlayer();

    return nullptr;
}

// ==================== CHECK METHODS ==================================================== //
bool FSBMgr::IsBotExpired(PlayerBotData const& bot)
{
    return bot.hireExpiry > 0 && bot.hireExpiry < static_cast<uint64>(time(nullptr));
}

bool FSBMgr::IsPersistentBotExpired(uint64 ownerGuid, uint64 botEntry)
{
    auto it = _playerBotsPersistent.find(ownerGuid);
    if (it == _playerBotsPersistent.end())
        return false;

    for (auto const& bot : it->second)
    {
        if (bot.entry == botEntry)
            return bot.hireExpiry > 0 && bot.hireExpiry < uint64(time(nullptr));
    }

    return false;
}

uint64 FSBMgr::GetBotExpireTime(uint32 durationHours)
{
    return uint64(time(nullptr)) + uint64(durationHours) * 60 * 60;
}

// -------------------- Player Queries --------------------
bool FSBMgr::CheckPlayerHasBotWithEntry(Player* player, uint32 entry)
{
    auto* bots = GetPersistentBotsForPlayer(player);
    if (!bots)
        return false;

    return std::any_of(bots->begin(), bots->end(), [entry](PlayerBotData const& b) { return b.entry == entry; });
}

bool FSBMgr::IsBotOwnedByPlayer(Player* player, Creature* bot)
{
    if (!player || !bot)
        return false;

    uint64 ownerGuid = player->GetGUID().GetCounter();
    uint64 spawnId = bot->GetSpawnId();

    auto it = _playerBotsPersistent.find(ownerGuid);
    if (it == _playerBotsPersistent.end())
        return false;

    for (auto const& data : it->second)
    {
        if (data.spawnId == spawnId)
            return true;
    }

    return false;
}

bool FSBMgr::IsBotOwned(Creature* bot)
{
    if (!bot)
        return false;

    uint64 spawnId = bot->GetSpawnId();

    for (auto const& [ownerGuid, bots] : _playerBotsPersistent)
    {
        for (auto const& data : bots)
        {
            if (data.spawnId == spawnId)
                return true;
        }
    }

    return false;
}


// ==================== MANAGEMENT METHODS ==================================================== //
void FSBMgr::HirePersistentBot(Player* player, Creature* bot, uint32 hireDurationHours)
{
    if (!bot || !player)
        return;

    // Calculate expiry
    // Permanent hire comes with duration 0
    uint64 hireExpiry = 0;
    if (hireDurationHours != 0)
        hireExpiry = GetBotExpireTime(hireDurationHours);

    StorePersistentBot(bot, player, hireExpiry);

    // 3?? Restore ownership + AI state
    uint32 hireTimeLeft = hireDurationHours * 3600;
    RestoreBotOwnership(player, bot, hireTimeLeft);

    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Player {} hired bot {} (entry {}) until {}",
        player->GetName(), bot->GetName(), bot->GetEntry(), hireExpiry);


}

void FSBMgr::DismissPersistentBot(Creature* bot)
{
    if (!bot)
        return;

    Player* player = GetBotOwner(bot);

    if (!player)
        return;

    uint32 playerGuidLow = player->GetGUID().GetCounter();
    uint32 botEntry = bot->GetEntry();

    bot->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
    bot->StopMoving();
    bot->GetMotionMaster()->Clear();
    if (player)
    {
        std::string msg = FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::Fire, "");
        bot->Say(msg, LANG_UNIVERSAL);
    }

    RemovePersistentBot(playerGuidLow, botEntry);

    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Dismissed bot {} for player {}", bot->GetName(), player->GetName());
}

void FSBMgr::SetInitialBotState(Creature* bot)
{
    if (!bot)
        return;

    bot->SetBot(true);
    bot->setActive(true);

    if (bot->HasUnitState(UNIT_STAND_STATE_SIT))
        bot->SetStandState(UNIT_STAND_STATE_STAND);

    auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
    if (!baseAI)
        return;

    baseAI->botHired = false;
    baseAI->botHasDemon = false;
    baseAI->botMoveState = FSB_MOVE_STATE_IDLE;
    baseAI->botFollowDistance = frand(2.f, 8.f);
    baseAI->botFollowAngle = frand(0.0f, float(M_PI * 2.0f));
    auto& botClass = baseAI->botClass;
    auto& botRace = baseAI->botRace;
    auto& botStats = baseAI->botStats;

    // Initial Flags and States
    bot->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
    bot->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);

    bot->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
    bot->SetReactState(REACT_DEFENSIVE);
    bot->SetFaction(FSB_FACTION_HUMAN);
    //creature->SetFaction(12); // it is actually template

    SetBotClassAndRace(bot, botClass, botRace);
    FSBStats::ApplyBotBaseClassStats(bot, botClass);
    botStats = FSBBotStats();
}

void FSBMgr::SetBotClassAndRace(Creature* creature, FSB_Class& outClass, FSB_Race& outRace)
{
    if (!creature)
        return;

    bool found = GetBotClassAndRaceForEntry(
        creature->GetEntry(),
        outClass,
        outRace
    );

    if (!found)
    {
        TC_LOG_WARN(
            "scripts.ai.fsb",
            "FSB: No class/race mapping found for creature entry {}",
            creature->GetEntry()
        );
        return;
    }

    TC_LOG_DEBUG(
        "scripts.ai.fsb",
        "FSB: Class set to {} and Race set to {} for bot with entry {}",
        outClass,
        outRace,
        creature->GetEntry()
    );
}

bool FSBMgr::GetBotClassAndRaceForEntry(uint32 entry, FSB_Class& outClass, FSB_Race& outRace)
{
    for (auto const& map : BotEntryClassTable)
    {
        if (map.entry == entry)
        {
            outClass = map.botClass;
            outRace = map.botRace;
            return true;
        }
    }

    outClass = FSB_Class::None;
    outRace = FSB_Race::None;
    return false;
}










