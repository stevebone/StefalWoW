/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file ContentRequirements.cpp
 * @brief Implementation of content requirements database
 */

#include "ContentRequirements.h"
#include "BattlegroundMgr.h"
#include "Database/PlayerbotDatabase.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "Log.h"
#include "Random.h"
#include "World.h"
#include <fmt/format.h>

namespace Playerbot
{

// ============================================================================
// CONTENT REQUIREMENT
// ============================================================================

std::string ContentRequirement::ToString() const
{
    if (type == InstanceType::Battleground || type == InstanceType::Arena)
    {
        return fmt::format("ContentRequirement[ID={}, Name={}, Type={}, Players={}, PerFaction={}]",
            contentId, contentName, InstanceTypeToString(type), maxPlayers, playersPerFaction);
    }
    else
    {
        return fmt::format("ContentRequirement[ID={}, Name={}, Type={}, T/H/D={}/{}/{}, GS={}]",
            contentId, contentName, InstanceTypeToString(type),
            recommendedTanks, recommendedHealers, recommendedDPS, recommendedGearScore);
    }
}

// ============================================================================
// BOTS NEEDED
// ============================================================================

std::string BotsNeeded::ToString() const
{
    if (allianceNeeded > 0 || hordeNeeded > 0)
    {
        return fmt::format("BotsNeeded[Alliance={}, Horde={}, Total={}, MinGS={}]",
            allianceNeeded, hordeNeeded, totalNeeded, minGearScore);
    }
    else
    {
        return fmt::format("BotsNeeded[T={}, H={}, D={}, Total={}, MinGS={}]",
            tanksNeeded, healersNeeded, dpsNeeded, totalNeeded, minGearScore);
    }
}

// ============================================================================
// SINGLETON
// ============================================================================

ContentRequirementDatabase* ContentRequirementDatabase::Instance()
{
    static ContentRequirementDatabase instance;
    return &instance;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool ContentRequirementDatabase::Initialize()
{
    if (_initialized.load())
    {
        TC_LOG_WARN("playerbot.content", "ContentRequirementDatabase::Initialize - Already initialized");
        return true;
    }

    TC_LOG_INFO("playerbot.content", "ContentRequirementDatabase::Initialize - Starting initialization");

    // Create default requirements first
    CreateDefaultDungeons();
    CreateDefaultRaids();
    CreateDefaultBattlegrounds();
    CreateDefaultArenas();

    // Then load/override from database
    LoadFromDatabase();

    _initialized.store(true);

    TC_LOG_INFO("playerbot.content", "ContentRequirementDatabase::Initialize - Loaded {} dungeons, {} raids, {} battlegrounds, {} arenas",
        _dungeons.size(), _raids.size(), _battlegrounds.size(), _arenas.size());

    return true;
}

void ContentRequirementDatabase::Shutdown()
{
    if (!_initialized.load())
        return;

    TC_LOG_INFO("playerbot.content", "ContentRequirementDatabase::Shutdown - Shutting down");

    std::unique_lock<std::shared_mutex> lock(_mutex);
    _dungeons.clear();
    _raids.clear();
    _battlegrounds.clear();
    _arenas.clear();

    _initialized.store(false);
}

void ContentRequirementDatabase::Reload()
{
    TC_LOG_INFO("playerbot.content", "ContentRequirementDatabase::Reload - Reloading requirements");

    std::unique_lock<std::shared_mutex> lock(_mutex);
    _dungeons.clear();
    _raids.clear();
    _battlegrounds.clear();
    _arenas.clear();

    lock.unlock();

    CreateDefaultDungeons();
    CreateDefaultRaids();
    CreateDefaultBattlegrounds();
    CreateDefaultArenas();
    LoadFromDatabase();

    TC_LOG_INFO("playerbot.content", "ContentRequirementDatabase::Reload - Reloaded {} dungeons, {} raids, {} battlegrounds, {} arenas",
        _dungeons.size(), _raids.size(), _battlegrounds.size(), _arenas.size());
}

// ============================================================================
// REQUIREMENT ACCESS
// ============================================================================

ContentRequirement const* ContentRequirementDatabase::GetDungeonRequirement(uint32 dungeonId) const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    auto it = _dungeons.find(dungeonId);
    if (it != _dungeons.end())
        return &it->second;

    // Fall back to generic dungeon template (ID 0) for unknown dungeons
    auto genericIt = _dungeons.find(0);
    if (genericIt != _dungeons.end())
    {
        TC_LOG_DEBUG("playerbot.content", "GetDungeonRequirement - Using generic template for dungeon {}",
            dungeonId);
        return &genericIt->second;
    }

    return nullptr;
}

ContentRequirement const* ContentRequirementDatabase::GetRaidRequirement(uint32 raidId) const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    auto it = _raids.find(raidId);
    return it != _raids.end() ? &it->second : nullptr;
}

ContentRequirement const* ContentRequirementDatabase::GetBattlegroundRequirement(uint32 bgTypeId) const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);

    // Random Battleground (32) - pick a random actual BG
    if (bgTypeId == 32)
    {
        return GetRandomBattlegroundRequirement_Locked();
    }

    auto it = _battlegrounds.find(bgTypeId);
    return it != _battlegrounds.end() ? &it->second : nullptr;
}

ContentRequirement const* ContentRequirementDatabase::GetRandomBattlegroundRequirement_Locked() const
{
    // Pick a random BG from all loaded battleground requirements
    if (_battlegrounds.empty())
        return nullptr;

    uint32 randomIndex = urand(0, static_cast<uint32>(_battlegrounds.size() - 1));
    auto it = _battlegrounds.begin();
    std::advance(it, randomIndex);

    TC_LOG_INFO("playerbot.content", "Random BG selected: {} ({}v{})",
        it->second.contentName, it->second.playersPerFaction, it->second.playersPerFaction);
    return &it->second;
}

ContentRequirement const* ContentRequirementDatabase::GetArenaRequirement(uint32 arenaType) const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    auto it = _arenas.find(arenaType);
    return it != _arenas.end() ? &it->second : nullptr;
}

ContentRequirement const* ContentRequirementDatabase::GetRequirement(InstanceType type, uint32 contentId) const
{
    switch (type)
    {
        case InstanceType::Dungeon: return GetDungeonRequirement(contentId);
        case InstanceType::Raid: return GetRaidRequirement(contentId);
        case InstanceType::Battleground: return GetBattlegroundRequirement(contentId);
        case InstanceType::Arena: return GetArenaRequirement(contentId);
        default: return nullptr;
    }
}

// ============================================================================
// BOTS CALCULATION
// ============================================================================

BotsNeeded ContentRequirementDatabase::CalculateBotsNeeded(
    ContentRequirement const* requirement,
    GroupState const& groupState) const
{
    BotsNeeded result;

    if (!requirement)
        return result;

    result.minGearScore = requirement->minGearScore;

    // PvP content - both factions
    if (requirement->requiresBothFactions)
    {
        uint32 perFaction = requirement->playersPerFaction;

        // Calculate Alliance bots needed
        if (groupState.alliancePlayers < perFaction)
            result.allianceNeeded = perFaction - groupState.alliancePlayers;

        // Calculate Horde bots needed
        if (groupState.hordePlayers < perFaction)
            result.hordeNeeded = perFaction - groupState.hordePlayers;

        result.totalNeeded = result.allianceNeeded + result.hordeNeeded;
    }
    // Arena without both factions requirement (teammates only)
    else if (requirement->type == InstanceType::Arena)
    {
        CalculateOptimalRoles(requirement, groupState, result);
    }
    // PvE content - role-based
    else
    {
        CalculateOptimalRoles(requirement, groupState, result);
    }

    return result;
}

BotsNeeded ContentRequirementDatabase::CalculateDungeonBots(
    uint32 dungeonId,
    GroupState const& groupState) const
{
    ContentRequirement const* req = GetDungeonRequirement(dungeonId);
    return CalculateBotsNeeded(req, groupState);
}

BotsNeeded ContentRequirementDatabase::CalculateRaidBots(
    uint32 raidId,
    GroupState const& groupState) const
{
    ContentRequirement const* req = GetRaidRequirement(raidId);
    return CalculateBotsNeeded(req, groupState);
}

BotsNeeded ContentRequirementDatabase::CalculateBattlegroundBots(
    uint32 bgTypeId,
    GroupState const& groupState) const
{
    ContentRequirement const* req = GetBattlegroundRequirement(bgTypeId);
    return CalculateBotsNeeded(req, groupState);
}

BotsNeeded ContentRequirementDatabase::CalculateArenaBots(
    uint32 arenaType,
    GroupState const& groupState,
    bool needOpponents) const
{
    ContentRequirement const* req = GetArenaRequirement(arenaType);
    if (!req)
        return BotsNeeded{};

    BotsNeeded result = CalculateBotsNeeded(req, groupState);

    // For arena, also calculate opponents if needed
    if (needOpponents)
    {
        Faction opponentFaction = (groupState.leaderFaction == Faction::Alliance) ?
            Faction::Horde : Faction::Alliance;

        // Opponents need same team size
        uint32 teamSize = req->maxPlayers;
        if (opponentFaction == Faction::Alliance)
            result.allianceNeeded += teamSize;
        else
            result.hordeNeeded += teamSize;

        result.totalNeeded += teamSize;
    }

    return result;
}

// ============================================================================
// QUERIES
// ============================================================================

std::vector<ContentRequirement const*> ContentRequirementDatabase::GetAllDungeons() const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    std::vector<ContentRequirement const*> result;
    result.reserve(_dungeons.size());
    for (auto const& [id, req] : _dungeons)
        result.push_back(&req);
    return result;
}

std::vector<ContentRequirement const*> ContentRequirementDatabase::GetAllRaids() const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    std::vector<ContentRequirement const*> result;
    result.reserve(_raids.size());
    for (auto const& [id, req] : _raids)
        result.push_back(&req);
    return result;
}

std::vector<ContentRequirement const*> ContentRequirementDatabase::GetAllBattlegrounds() const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    std::vector<ContentRequirement const*> result;
    result.reserve(_battlegrounds.size());
    for (auto const& [id, req] : _battlegrounds)
        result.push_back(&req);
    return result;
}

std::vector<ContentRequirement const*> ContentRequirementDatabase::GetAllArenas() const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    std::vector<ContentRequirement const*> result;
    result.reserve(_arenas.size());
    for (auto const& [id, req] : _arenas)
        result.push_back(&req);
    return result;
}

std::vector<ContentRequirement const*> ContentRequirementDatabase::GetDungeonsForLevel(uint32 level) const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    std::vector<ContentRequirement const*> result;
    for (auto const& [id, req] : _dungeons)
    {
        if (level >= req.minLevel && level <= req.maxLevel)
            result.push_back(&req);
    }
    return result;
}

std::vector<ContentRequirement const*> ContentRequirementDatabase::GetBattlegroundsForLevel(uint32 level) const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    std::vector<ContentRequirement const*> result;
    for (auto const& [id, req] : _battlegrounds)
    {
        if (level >= req.minLevel && level <= req.maxLevel)
            result.push_back(&req);
    }
    return result;
}

// ============================================================================
// STATISTICS
// ============================================================================

uint32 ContentRequirementDatabase::GetTotalCount() const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return static_cast<uint32>(_dungeons.size() + _raids.size() +
        _battlegrounds.size() + _arenas.size());
}

void ContentRequirementDatabase::PrintStatistics() const
{
    TC_LOG_INFO("playerbot.content", "=== ContentRequirementDatabase Statistics ===");
    TC_LOG_INFO("playerbot.content", "Dungeons: {}", _dungeons.size());
    TC_LOG_INFO("playerbot.content", "Raids: {}", _raids.size());
    TC_LOG_INFO("playerbot.content", "Battlegrounds: {}", _battlegrounds.size());
    TC_LOG_INFO("playerbot.content", "Arenas: {}", _arenas.size());
    TC_LOG_INFO("playerbot.content", "Total: {}", GetTotalCount());
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void ContentRequirementDatabase::LoadFromDatabase()
{
    TC_LOG_DEBUG("playerbot.content", "ContentRequirementDatabase::LoadFromDatabase - Loading from database");

    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT content_id, content_name, instance_type, min_players, max_players, "
        "min_level, max_level, recommended_level, min_tanks, max_tanks, recommended_tanks, "
        "min_healers, max_healers, recommended_healers, min_dps, max_dps, recommended_dps, "
        "min_gear_score, recommended_gear_score, requires_both_factions, players_per_faction, "
        "estimated_duration_minutes, difficulty "
        "FROM playerbot_content_requirements");

    if (!result)
    {
        TC_LOG_DEBUG("playerbot.content", "ContentRequirementDatabase::LoadFromDatabase - No database entries found");
        return;
    }

    uint32 loadedCount = 0;
    do
    {
        Field* fields = result->Fetch();

        ContentRequirement req;
        req.contentId = fields[0].GetUInt32();
        req.contentName = fields[1].GetString();
        req.type = static_cast<InstanceType>(fields[2].GetUInt8());
        req.minPlayers = fields[3].GetUInt32();
        req.maxPlayers = fields[4].GetUInt32();
        req.minLevel = fields[5].GetUInt32();
        req.maxLevel = fields[6].GetUInt32();
        req.recommendedLevel = fields[7].GetUInt32();
        req.minTanks = fields[8].GetUInt32();
        req.maxTanks = fields[9].GetUInt32();
        req.recommendedTanks = fields[10].GetUInt32();
        req.minHealers = fields[11].GetUInt32();
        req.maxHealers = fields[12].GetUInt32();
        req.recommendedHealers = fields[13].GetUInt32();
        req.minDPS = fields[14].GetUInt32();
        req.maxDPS = fields[15].GetUInt32();
        req.recommendedDPS = fields[16].GetUInt32();
        req.minGearScore = fields[17].GetUInt32();
        req.recommendedGearScore = fields[18].GetUInt32();
        req.requiresBothFactions = fields[19].GetBool();
        req.playersPerFaction = fields[20].GetUInt32();
        req.estimatedDurationMinutes = fields[21].GetUInt32();
        req.difficulty = fields[22].GetUInt8();

        AddRequirement(std::move(req));
        ++loadedCount;

    } while (result->NextRow());

    TC_LOG_INFO("playerbot.content", "ContentRequirementDatabase::LoadFromDatabase - Loaded {} requirements from database",
        loadedCount);
}

void ContentRequirementDatabase::CreateDefaultDungeons()
{
    TC_LOG_INFO("playerbot.content", "ContentRequirementDatabase::CreateDefaultDungeons - Loading from LFGDungeons.db2");

    uint32 loadedCount = 0;

    // Load all dungeons from DB2
    for (uint32 i = 0; i < sLFGDungeonsStore.GetNumRows(); ++i)
    {
        LFGDungeonsEntry const* dungeon = sLFGDungeonsStore.LookupEntry(i);
        if (!dungeon)
            continue;

        // TypeID 1 = Dungeon (5-man), TypeID 2 = Raid
        // Skip non-dungeon entries (raids handled separately)
        if (dungeon->TypeID != 1)
            continue;

        ContentRequirement req;
        req.contentId = dungeon->ID;
        req.contentName = dungeon->Name[LOCALE_enUS];
        req.type = InstanceType::Dungeon;
        req.minPlayers = 1;
        req.maxPlayers = 5;

        // Level requirements - use a reasonable range based on expansion
        // ExpansionLevel: 0=Classic, 1=TBC, 2=WotLK, etc.
        switch (dungeon->ExpansionLevel)
        {
            case 0: req.minLevel = 10; req.maxLevel = 60; req.recommendedLevel = 60; break;
            case 1: req.minLevel = 58; req.maxLevel = 70; req.recommendedLevel = 70; break;
            case 2: req.minLevel = 68; req.maxLevel = 80; req.recommendedLevel = 80; break;
            case 3: req.minLevel = 80; req.maxLevel = 85; req.recommendedLevel = 85; break;
            case 4: req.minLevel = 85; req.maxLevel = 90; req.recommendedLevel = 90; break;
            case 5: req.minLevel = 90; req.maxLevel = 100; req.recommendedLevel = 100; break;
            case 6: req.minLevel = 98; req.maxLevel = 110; req.recommendedLevel = 110; break;
            case 7: req.minLevel = 110; req.maxLevel = 120; req.recommendedLevel = 120; break;
            case 8: req.minLevel = 50; req.maxLevel = 60; req.recommendedLevel = 60; break;
            case 9: req.minLevel = 60; req.maxLevel = 70; req.recommendedLevel = 70; break;
            case 10: req.minLevel = 70; req.maxLevel = 80; req.recommendedLevel = 80; break;
            default: req.minLevel = 1; req.maxLevel = 80; req.recommendedLevel = 80; break;
        }

        // Role requirements from DB2
        req.minTanks = dungeon->MinCountTank > 0 ? dungeon->MinCountTank : 1;
        req.maxTanks = dungeon->CountTank > 0 ? dungeon->CountTank : 1;
        req.recommendedTanks = dungeon->CountTank > 0 ? dungeon->CountTank : 1;

        req.minHealers = dungeon->MinCountHealer > 0 ? dungeon->MinCountHealer : 1;
        req.maxHealers = dungeon->CountHealer > 0 ? dungeon->CountHealer : 1;
        req.recommendedHealers = dungeon->CountHealer > 0 ? dungeon->CountHealer : 1;

        req.minDPS = dungeon->MinCountDamage > 0 ? dungeon->MinCountDamage : 3;
        req.maxDPS = dungeon->CountDamage > 0 ? dungeon->CountDamage : 3;
        req.recommendedDPS = dungeon->CountDamage > 0 ? dungeon->CountDamage : 3;

        // Gear requirements from DB2
        req.minGearScore = static_cast<uint32>(dungeon->MinGear);
        req.recommendedGearScore = static_cast<uint32>(dungeon->MinGear);

        req.estimatedDurationMinutes = 30;
        req.difficulty = dungeon->DifficultyID;

        AddRequirement(std::move(req));
        ++loadedCount;
    }

    TC_LOG_INFO("playerbot.content", "ContentRequirementDatabase::CreateDefaultDungeons - Loaded {} dungeons from DB2",
        loadedCount);

    // Generic dungeon template for any dungeons not in DB2
    ContentRequirement genericDungeon;
    genericDungeon.contentId = 0;  // Special ID for generic fallback
    genericDungeon.contentName = "Generic Dungeon";
    genericDungeon.type = InstanceType::Dungeon;
    genericDungeon.minPlayers = 1;
    genericDungeon.maxPlayers = 5;
    genericDungeon.minLevel = 1;
    genericDungeon.maxLevel = 80;
    genericDungeon.recommendedLevel = 80;
    genericDungeon.minTanks = 1;
    genericDungeon.maxTanks = 1;
    genericDungeon.recommendedTanks = 1;
    genericDungeon.minHealers = 1;
    genericDungeon.maxHealers = 1;
    genericDungeon.recommendedHealers = 1;
    genericDungeon.minDPS = 3;
    genericDungeon.maxDPS = 3;
    genericDungeon.recommendedDPS = 3;
    genericDungeon.minGearScore = 0;
    genericDungeon.recommendedGearScore = 400;
    genericDungeon.estimatedDurationMinutes = 30;

    AddRequirement(std::move(genericDungeon));
}

void ContentRequirementDatabase::CreateDefaultRaids()
{
    TC_LOG_DEBUG("playerbot.content", "ContentRequirementDatabase::CreateDefaultRaids - Creating defaults");

    // The War Within Raids

    // Nerub-ar Palace (10/25 Normal/Heroic)
    ContentRequirement nerubarPalace10;
    nerubarPalace10.contentId = 2657;
    nerubarPalace10.contentName = "Nerub-ar Palace (10)";
    nerubarPalace10.type = InstanceType::Raid;
    nerubarPalace10.minPlayers = 1;
    nerubarPalace10.maxPlayers = 10;
    nerubarPalace10.minLevel = 80;
    nerubarPalace10.maxLevel = 80;
    nerubarPalace10.recommendedLevel = 80;
    nerubarPalace10.minTanks = 2;
    nerubarPalace10.maxTanks = 2;
    nerubarPalace10.recommendedTanks = 2;
    nerubarPalace10.minHealers = 2;
    nerubarPalace10.maxHealers = 3;
    nerubarPalace10.recommendedHealers = 2;
    nerubarPalace10.minDPS = 5;
    nerubarPalace10.maxDPS = 6;
    nerubarPalace10.recommendedDPS = 6;
    nerubarPalace10.minGearScore = 590;
    nerubarPalace10.recommendedGearScore = 610;
    nerubarPalace10.estimatedDurationMinutes = 120;

    AddRequirement(std::move(nerubarPalace10));

    // 25-man version
    ContentRequirement nerubarPalace25;
    nerubarPalace25.contentId = 2658;
    nerubarPalace25.contentName = "Nerub-ar Palace (25)";
    nerubarPalace25.type = InstanceType::Raid;
    nerubarPalace25.minPlayers = 1;
    nerubarPalace25.maxPlayers = 25;
    nerubarPalace25.minLevel = 80;
    nerubarPalace25.maxLevel = 80;
    nerubarPalace25.recommendedLevel = 80;
    nerubarPalace25.minTanks = 2;
    nerubarPalace25.maxTanks = 3;
    nerubarPalace25.recommendedTanks = 2;
    nerubarPalace25.minHealers = 5;
    nerubarPalace25.maxHealers = 7;
    nerubarPalace25.recommendedHealers = 6;
    nerubarPalace25.minDPS = 15;
    nerubarPalace25.maxDPS = 18;
    nerubarPalace25.recommendedDPS = 17;
    nerubarPalace25.minGearScore = 595;
    nerubarPalace25.recommendedGearScore = 615;
    nerubarPalace25.estimatedDurationMinutes = 180;

    AddRequirement(std::move(nerubarPalace25));

    // Classic 40-man raid template (for legacy content)
    ContentRequirement classic40man;
    classic40man.contentId = 9999;  // Generic ID for 40-man
    classic40man.contentName = "Classic 40-man Raid";
    classic40man.type = InstanceType::Raid;
    classic40man.minPlayers = 1;
    classic40man.maxPlayers = 40;
    classic40man.minLevel = 60;
    classic40man.maxLevel = 80;
    classic40man.recommendedLevel = 60;
    classic40man.minTanks = 4;
    classic40man.maxTanks = 5;
    classic40man.recommendedTanks = 4;
    classic40man.minHealers = 10;
    classic40man.maxHealers = 12;
    classic40man.recommendedHealers = 10;
    classic40man.minDPS = 23;
    classic40man.maxDPS = 26;
    classic40man.recommendedDPS = 26;
    classic40man.minGearScore = 0;
    classic40man.recommendedGearScore = 300;
    classic40man.estimatedDurationMinutes = 240;

    AddRequirement(std::move(classic40man));
}

void ContentRequirementDatabase::CreateDefaultBattlegrounds()
{
    TC_LOG_DEBUG("playerbot.content", "ContentRequirementDatabase::CreateDefaultBattlegrounds - Populating from BattlemasterList DB2");

    uint32 count = 0;

    for (BattlemasterListEntry const* entry : sBattlemasterListStore)
    {
        if (!entry)
            continue;

        // Only battlegrounds, not arenas
        if (entry->GetType() != BattlemasterType::Battleground)
            continue;

        // Skip meta/random queue entries — these are virtual queues, not real BGs
        if (BattlegroundMgr::IsRandomBattleground(entry->ID)
            || entry->ID == BATTLEGROUND_AA)
            continue;

        // Skip internal-only, obsolete, brawls, rated-only
        EnumFlag<BattlemasterListFlags> flags = entry->GetFlags();
        if (flags.HasFlag(BattlemasterListFlags::InternalOnly)
            || flags.HasFlag(BattlemasterListFlags::ObsoleteDoNotList)
            || flags.HasFlag(BattlemasterListFlags::IsBrawl)
            || flags.HasFlag(BattlemasterListFlags::RatedOnly))
            continue;

        // Must have valid player count (MaxPlayers is per-team in BattlemasterList)
        if (entry->MaxPlayers <= 0)
            continue;

        uint32 playersPerTeam = static_cast<uint32>(entry->MaxPlayers);
        uint32 totalPlayers = playersPerTeam * 2;

        ContentRequirement req;
        req.contentId = entry->ID;
        req.contentName = entry->Name[sWorld->GetDefaultDbcLocale()];
        req.type = InstanceType::Battleground;
        req.minPlayers = totalPlayers;
        req.maxPlayers = totalPlayers;
        req.minLevel = std::max(static_cast<int>(entry->MinLevel), 10);
        req.maxLevel = std::max(static_cast<int>(entry->MaxLevel), 80);
        req.requiresBothFactions = true;
        req.playersPerFaction = playersPerTeam;

        // Estimate duration based on team size: epic BGs take longer
        if (playersPerTeam >= 40)
            req.estimatedDurationMinutes = 45;
        else if (playersPerTeam >= 15)
            req.estimatedDurationMinutes = 20;
        else
            req.estimatedDurationMinutes = 15;

        TC_LOG_DEBUG("playerbot.content",
            "  BG {}: '{}' ({}v{}, levels {}-{})",
            entry->ID, req.contentName, playersPerTeam, playersPerTeam,
            req.minLevel, req.maxLevel);

        AddRequirement(std::move(req));
        ++count;
    }

    TC_LOG_INFO("module.playerbot",
        "ContentRequirementDatabase: Loaded {} battleground types from BattlemasterList DB2", count);

    // NOTE: Random Battleground (32) is handled dynamically in GetBattlegroundRequirement()
    // by selecting a random actual BG (AV, WSG, AB, etc.) so proper team sizes are used.
}

void ContentRequirementDatabase::CreateDefaultArenas()
{
    TC_LOG_DEBUG("playerbot.content", "ContentRequirementDatabase::CreateDefaultArenas - Creating defaults");

    // 2v2 Arena
    ContentRequirement arena2v2;
    arena2v2.contentId = 2;
    arena2v2.contentName = "2v2 Arena";
    arena2v2.type = InstanceType::Arena;
    arena2v2.minPlayers = 1;
    arena2v2.maxPlayers = 2;
    arena2v2.minLevel = 70;
    arena2v2.maxLevel = 80;
    arena2v2.recommendedLevel = 80;
    arena2v2.minTanks = 0;
    arena2v2.maxTanks = 1;
    arena2v2.recommendedTanks = 0;
    arena2v2.minHealers = 0;
    arena2v2.maxHealers = 1;
    arena2v2.recommendedHealers = 1;
    arena2v2.minDPS = 1;
    arena2v2.maxDPS = 2;
    arena2v2.recommendedDPS = 1;
    arena2v2.minGearScore = 580;
    arena2v2.recommendedGearScore = 620;
    arena2v2.requiresBothFactions = false;  // Teammates are same faction
    arena2v2.estimatedDurationMinutes = 10;

    AddRequirement(std::move(arena2v2));

    // 3v3 Arena
    ContentRequirement arena3v3;
    arena3v3.contentId = 3;
    arena3v3.contentName = "3v3 Arena";
    arena3v3.type = InstanceType::Arena;
    arena3v3.minPlayers = 1;
    arena3v3.maxPlayers = 3;
    arena3v3.minLevel = 70;
    arena3v3.maxLevel = 80;
    arena3v3.recommendedLevel = 80;
    arena3v3.minTanks = 0;
    arena3v3.maxTanks = 0;
    arena3v3.recommendedTanks = 0;
    arena3v3.minHealers = 1;
    arena3v3.maxHealers = 1;
    arena3v3.recommendedHealers = 1;
    arena3v3.minDPS = 2;
    arena3v3.maxDPS = 2;
    arena3v3.recommendedDPS = 2;
    arena3v3.minGearScore = 590;
    arena3v3.recommendedGearScore = 625;
    arena3v3.requiresBothFactions = false;
    arena3v3.estimatedDurationMinutes = 15;

    AddRequirement(std::move(arena3v3));

    // Solo Shuffle (new arena type)
    ContentRequirement soloShuffle;
    soloShuffle.contentId = 6;  // Solo Shuffle arena type
    soloShuffle.contentName = "Solo Shuffle";
    soloShuffle.type = InstanceType::Arena;
    soloShuffle.minPlayers = 1;
    soloShuffle.maxPlayers = 6;  // 3v3 with shuffled teams
    soloShuffle.minLevel = 70;
    soloShuffle.maxLevel = 80;
    soloShuffle.recommendedLevel = 80;
    soloShuffle.minTanks = 0;
    soloShuffle.maxTanks = 0;
    soloShuffle.recommendedTanks = 0;
    soloShuffle.minHealers = 2;  // 2 healers, rotated
    soloShuffle.maxHealers = 2;
    soloShuffle.recommendedHealers = 2;
    soloShuffle.minDPS = 4;  // 4 DPS, rotated
    soloShuffle.maxDPS = 4;
    soloShuffle.recommendedDPS = 4;
    soloShuffle.minGearScore = 595;
    soloShuffle.recommendedGearScore = 630;
    soloShuffle.requiresBothFactions = false;
    soloShuffle.estimatedDurationMinutes = 20;

    AddRequirement(std::move(soloShuffle));
}

void ContentRequirementDatabase::AddRequirement(ContentRequirement requirement)
{
    if (!requirement.IsValid())
    {
        TC_LOG_WARN("playerbot.content", "ContentRequirementDatabase::AddRequirement - Invalid requirement: {}",
            requirement.contentName);
        return;
    }

    std::unique_lock<std::shared_mutex> lock(_mutex);

    switch (requirement.type)
    {
        case InstanceType::Dungeon:
            _dungeons[requirement.contentId] = std::move(requirement);
            break;
        case InstanceType::Raid:
            _raids[requirement.contentId] = std::move(requirement);
            break;
        case InstanceType::Battleground:
            _battlegrounds[requirement.contentId] = std::move(requirement);
            break;
        case InstanceType::Arena:
            _arenas[requirement.contentId] = std::move(requirement);
            break;
        default:
            TC_LOG_WARN("playerbot.content", "ContentRequirementDatabase::AddRequirement - Unknown type");
            break;
    }
}

void ContentRequirementDatabase::CalculateOptimalRoles(
    ContentRequirement const* req,
    GroupState const& groupState,
    BotsNeeded& result) const
{
    if (!req)
        return;

    // Calculate tanks needed
    if (groupState.tanks < req->recommendedTanks)
        result.tanksNeeded = req->recommendedTanks - groupState.tanks;

    // Calculate healers needed
    if (groupState.healers < req->recommendedHealers)
        result.healersNeeded = req->recommendedHealers - groupState.healers;

    // Calculate DPS needed
    if (groupState.dps < req->recommendedDPS)
        result.dpsNeeded = req->recommendedDPS - groupState.dps;

    // Cap at max allowed
    if (result.tanksNeeded > (req->maxTanks - groupState.tanks))
        result.tanksNeeded = (req->maxTanks > groupState.tanks) ? req->maxTanks - groupState.tanks : 0;

    if (result.healersNeeded > (req->maxHealers - groupState.healers))
        result.healersNeeded = (req->maxHealers > groupState.healers) ? req->maxHealers - groupState.healers : 0;

    if (result.dpsNeeded > (req->maxDPS - groupState.dps))
        result.dpsNeeded = (req->maxDPS > groupState.dps) ? req->maxDPS - groupState.dps : 0;

    // Calculate total
    result.totalNeeded = result.tanksNeeded + result.healersNeeded + result.dpsNeeded;

    // Ensure we don't exceed max players
    uint32 maxBotsAllowed = req->maxPlayers - groupState.totalPlayers;
    if (result.totalNeeded > maxBotsAllowed)
    {
        // Prioritize: Tank > Healer > DPS
        while (result.totalNeeded > maxBotsAllowed && result.dpsNeeded > 0)
        {
            --result.dpsNeeded;
            --result.totalNeeded;
        }
        while (result.totalNeeded > maxBotsAllowed && result.healersNeeded > req->minHealers)
        {
            --result.healersNeeded;
            --result.totalNeeded;
        }
        while (result.totalNeeded > maxBotsAllowed && result.tanksNeeded > req->minTanks)
        {
            --result.tanksNeeded;
            --result.totalNeeded;
        }
    }
}

} // namespace Playerbot
