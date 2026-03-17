/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotTemplateRepository.h"
#include "Config/PlayerbotConfig.h"
#include "Database/PlayerbotDatabase.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "Globals/ObjectMgr.h"
#include "Log.h"
#include "World.h"
#include <random>
#include <sstream>

namespace Playerbot
{

// ============================================================================
// WoW CLASS/SPEC CONSTANTS
// ============================================================================

namespace Classes
{
    constexpr uint8 WARRIOR = 1;
    constexpr uint8 PALADIN = 2;
    constexpr uint8 HUNTER = 3;
    constexpr uint8 ROGUE = 4;
    constexpr uint8 PRIEST = 5;
    constexpr uint8 DEATH_KNIGHT = 6;
    constexpr uint8 SHAMAN = 7;
    constexpr uint8 MAGE = 8;
    constexpr uint8 WARLOCK = 9;
    constexpr uint8 MONK = 10;
    constexpr uint8 DRUID = 11;
    constexpr uint8 DEMON_HUNTER = 12;
    constexpr uint8 EVOKER = 13;
}

namespace Races
{
    // Alliance
    constexpr uint8 HUMAN = 1;
    constexpr uint8 DWARF = 3;
    constexpr uint8 NIGHT_ELF = 4;
    constexpr uint8 GNOME = 7;
    constexpr uint8 DRAENEI = 11;
    constexpr uint8 WORGEN = 22;
    constexpr uint8 PANDAREN_ALLIANCE = 25;
    constexpr uint8 VOID_ELF = 29;
    constexpr uint8 LIGHTFORGED = 30;
    constexpr uint8 DARK_IRON = 34;
    constexpr uint8 KUL_TIRAN = 32;
    constexpr uint8 MECHAGNOME = 37;
    constexpr uint8 DRACTHYR_ALLIANCE = 52;
    constexpr uint8 EARTHEN_ALLIANCE = 85;

    // Horde
    constexpr uint8 ORC = 2;
    constexpr uint8 UNDEAD = 5;
    constexpr uint8 TAUREN = 6;
    constexpr uint8 TROLL = 8;
    constexpr uint8 GOBLIN = 9;
    constexpr uint8 BLOOD_ELF = 10;
    constexpr uint8 PANDAREN_HORDE = 26;
    constexpr uint8 NIGHTBORNE = 27;
    constexpr uint8 HIGHMOUNTAIN = 28;
    constexpr uint8 MAGHAR = 36;
    constexpr uint8 ZANDALARI = 31;
    constexpr uint8 VULPERA = 35;
    constexpr uint8 DRACTHYR_HORDE = 70;
    constexpr uint8 EARTHEN_HORDE = 84;
}

// Class names for logging
static const char* GetClassName(uint8 classId)
{
    switch (classId)
    {
        case Classes::WARRIOR:      return "Warrior";
        case Classes::PALADIN:      return "Paladin";
        case Classes::HUNTER:       return "Hunter";
        case Classes::ROGUE:        return "Rogue";
        case Classes::PRIEST:       return "Priest";
        case Classes::DEATH_KNIGHT: return "DeathKnight";
        case Classes::SHAMAN:       return "Shaman";
        case Classes::MAGE:         return "Mage";
        case Classes::WARLOCK:      return "Warlock";
        case Classes::MONK:         return "Monk";
        case Classes::DRUID:        return "Druid";
        case Classes::DEMON_HUNTER: return "DemonHunter";
        case Classes::EVOKER:       return "Evoker";
        default:                    return "Unknown";
    }
}

// Get spec name from DB2 store
static std::string GetSpecName(uint32 specId)
{
    if (ChrSpecializationEntry const* spec = sChrSpecializationStore.LookupEntry(specId))
    {
        // Name is a LocalizedString with Str[] array - use locale 0 (enUS)
        if (spec->Name.Str[0] && spec->Name.Str[0][0] != '\0')
            return spec->Name.Str[0];
    }

    // Fallback spec names if DB2 not available
    static std::unordered_map<uint32, std::string> fallbackNames = {
        // Warrior
        {71, "Arms"}, {72, "Fury"}, {73, "Protection"},
        // Paladin
        {65, "Holy"}, {66, "Protection"}, {70, "Retribution"},
        // Hunter
        {253, "BeastMastery"}, {254, "Marksmanship"}, {255, "Survival"},
        // Rogue
        {259, "Assassination"}, {260, "Outlaw"}, {261, "Subtlety"},
        // Priest
        {256, "Discipline"}, {257, "Holy"}, {258, "Shadow"},
        // Death Knight
        {250, "Blood"}, {251, "Frost"}, {252, "Unholy"},
        // Shaman
        {262, "Elemental"}, {263, "Enhancement"}, {264, "Restoration"},
        // Mage
        {62, "Arcane"}, {63, "Fire"}, {64, "Frost"},
        // Warlock
        {265, "Affliction"}, {266, "Demonology"}, {267, "Destruction"},
        // Monk
        {268, "Brewmaster"}, {270, "Mistweaver"}, {269, "Windwalker"},
        // Druid
        {102, "Balance"}, {103, "Feral"}, {104, "Guardian"}, {105, "Restoration"},
        // Demon Hunter
        {577, "Havoc"}, {581, "Vengeance"},
        // Evoker
        {1467, "Devastation"}, {1468, "Preservation"}, {1473, "Augmentation"}
    };

    auto it = fallbackNames.find(specId);
    return it != fallbackNames.end() ? it->second : fmt::format("Spec{}", specId);
}

// ============================================================================
// TALENT TEMPLATE IMPLEMENTATION
// ============================================================================

std::vector<uint8> TalentTemplate::Serialize() const
{
    std::vector<uint8> data;

    // Version byte for future compatibility
    data.push_back(1);

    // Write spec ID (4 bytes)
    data.push_back(specId & 0xFF);
    data.push_back((specId >> 8) & 0xFF);
    data.push_back((specId >> 16) & 0xFF);
    data.push_back((specId >> 24) & 0xFF);

    // Write talent count (2 bytes)
    uint16 count = static_cast<uint16>(talentIds.size());
    data.push_back(count & 0xFF);
    data.push_back((count >> 8) & 0xFF);

    // Write talent IDs (4 bytes each)
    for (uint32 talentId : talentIds)
    {
        data.push_back(talentId & 0xFF);
        data.push_back((talentId >> 8) & 0xFF);
        data.push_back((talentId >> 16) & 0xFF);
        data.push_back((talentId >> 24) & 0xFF);
    }

    // Write PvP talent count and IDs
    uint16 pvpCount = static_cast<uint16>(pvpTalentIds.size());
    data.push_back(pvpCount & 0xFF);
    data.push_back((pvpCount >> 8) & 0xFF);

    for (uint32 pvpTalentId : pvpTalentIds)
    {
        data.push_back(pvpTalentId & 0xFF);
        data.push_back((pvpTalentId >> 8) & 0xFF);
        data.push_back((pvpTalentId >> 16) & 0xFF);
        data.push_back((pvpTalentId >> 24) & 0xFF);
    }

    // Write hero talent tree ID (4 bytes)
    data.push_back(heroTalentTreeId & 0xFF);
    data.push_back((heroTalentTreeId >> 8) & 0xFF);
    data.push_back((heroTalentTreeId >> 16) & 0xFF);
    data.push_back((heroTalentTreeId >> 24) & 0xFF);

    return data;
}

TalentTemplate TalentTemplate::Deserialize(std::vector<uint8> const& data)
{
    TalentTemplate result;
    if (data.size() < 7) // version + specId + talent count
        return result;

    size_t offset = 0;

    // Read version
    uint8 version = data[offset++];
    if (version != 1)
    {
        TC_LOG_WARN("playerbot.template", "Unknown talent template version: {}", version);
        return result;
    }

    // Read spec ID
    result.specId = data[offset] | (data[offset + 1] << 8) |
                    (data[offset + 2] << 16) | (data[offset + 3] << 24);
    offset += 4;

    // Read talent count
    if (offset + 2 > data.size())
        return result;
    uint16 count = data[offset] | (data[offset + 1] << 8);
    offset += 2;

    // Read talent IDs
    for (uint16 i = 0; i < count && offset + 4 <= data.size(); ++i)
    {
        uint32 talentId = data[offset] | (data[offset + 1] << 8) |
                          (data[offset + 2] << 16) | (data[offset + 3] << 24);
        result.talentIds.push_back(talentId);
        offset += 4;
    }

    // Read PvP talent count
    if (offset + 2 <= data.size())
    {
        uint16 pvpCount = data[offset] | (data[offset + 1] << 8);
        offset += 2;

        for (uint16 i = 0; i < pvpCount && offset + 4 <= data.size(); ++i)
        {
            uint32 pvpTalentId = data[offset] | (data[offset + 1] << 8) |
                                 (data[offset + 2] << 16) | (data[offset + 3] << 24);
            result.pvpTalentIds.push_back(pvpTalentId);
            offset += 4;
        }
    }

    // Read hero talent tree ID
    if (offset + 4 <= data.size())
    {
        result.heroTalentTreeId = data[offset] | (data[offset + 1] << 8) |
                                  (data[offset + 2] << 16) | (data[offset + 3] << 24);
    }

    return result;
}

// ============================================================================
// ACTION BAR TEMPLATE IMPLEMENTATION
// ============================================================================

std::vector<uint8> ActionBarTemplate::Serialize() const
{
    std::vector<uint8> data;

    // Version byte
    data.push_back(1);

    // Write button count (2 bytes)
    uint16 count = static_cast<uint16>(buttons.size());
    data.push_back(count & 0xFF);
    data.push_back((count >> 8) & 0xFF);

    // Write each button (10 bytes each)
    for (auto const& button : buttons)
    {
        data.push_back(button.actionBar);
        data.push_back(button.slot);
        data.push_back(button.actionType & 0xFF);
        data.push_back((button.actionType >> 8) & 0xFF);
        data.push_back((button.actionType >> 16) & 0xFF);
        data.push_back((button.actionType >> 24) & 0xFF);
        data.push_back(button.actionId & 0xFF);
        data.push_back((button.actionId >> 8) & 0xFF);
        data.push_back((button.actionId >> 16) & 0xFF);
        data.push_back((button.actionId >> 24) & 0xFF);
    }

    return data;
}

ActionBarTemplate ActionBarTemplate::Deserialize(std::vector<uint8> const& data)
{
    ActionBarTemplate result;
    if (data.size() < 3) // version + count
        return result;

    size_t offset = 0;

    // Read version
    uint8 version = data[offset++];
    if (version != 1)
    {
        TC_LOG_WARN("playerbot.template", "Unknown action bar template version: {}", version);
        return result;
    }

    // Read button count
    uint16 count = data[offset] | (data[offset + 1] << 8);
    offset += 2;

    // Read buttons
    for (uint16 i = 0; i < count && offset + 10 <= data.size(); ++i)
    {
        ActionBarButton button;
        button.actionBar = data[offset++];
        button.slot = data[offset++];
        button.actionType = data[offset] | (data[offset + 1] << 8) |
                            (data[offset + 2] << 16) | (data[offset + 3] << 24);
        offset += 4;
        button.actionId = data[offset] | (data[offset + 1] << 8) |
                          (data[offset + 2] << 16) | (data[offset + 3] << 24);
        offset += 4;
        result.buttons.push_back(button);
    }

    return result;
}

// ============================================================================
// BOT TEMPLATE IMPLEMENTATION
// ============================================================================

uint8 BotTemplate::GetRandomRace(Faction faction) const
{
    auto const& races = (faction == Faction::Alliance) ? allianceRaces : hordeRaces;
    if (races.empty())
        return (faction == Faction::Alliance) ? Races::HUMAN : Races::ORC;

    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, races.size() - 1);
    return races[dist(gen)];
}

GearSetTemplate const* BotTemplate::GetGearSetForItemLevel(uint32 targetItemLevel) const
{
    if (gearSets.empty())
        return nullptr;

    GearSetTemplate const* closest = nullptr;
    int32 closestDiff = INT32_MAX;

    for (auto const& [level, gearSet] : gearSets)
    {
        int32 diff = std::abs(static_cast<int32>(level) - static_cast<int32>(targetItemLevel));
        if (diff < closestDiff)
        {
            closestDiff = diff;
            closest = &gearSet;
        }
    }

    return closest;
}

bool BotTemplate::IsValid() const
{
    return templateId != 0 &&
           playerClass != 0 &&
           specId != 0 &&
           (!allianceRaces.empty() || !hordeRaces.empty());
}

void BotTemplate::PreSerialize()
{
    talentBlob = talents.Serialize();
    actionBarBlob = actionBars.Serialize();
}

std::string BotTemplate::ToString() const
{
    return fmt::format("[Template: {} ({}_{}_{}) Role={} GearSets={} Talents={} Actions={}]",
        templateId, className, specName, BotRoleToString(role),
        BotRoleToString(role), gearSets.size(),
        talents.talentIds.size(), actionBars.buttons.size());
}

// ============================================================================
// TEMPLATE REPOSITORY SINGLETON
// ============================================================================

BotTemplateRepository* BotTemplateRepository::Instance()
{
    static BotTemplateRepository instance;
    return &instance;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

bool BotTemplateRepository::Initialize()
{
    if (_initialized.load())
    {
        TC_LOG_WARN("playerbot.template", "BotTemplateRepository already initialized");
        return true;
    }

    // Check if template system is enabled
    if (!sPlayerbotConfig->GetBool("Playerbot.Instance.Template.Enable", true))
    {
        TC_LOG_INFO("playerbot.template", "Bot Template Repository is disabled by config");
        _initialized.store(true);
        return true;
    }

    TC_LOG_INFO("playerbot.template", "Initializing Bot Template Repository...");

    // Load configuration options
    bool loadFromDb = sPlayerbotConfig->GetBool("Playerbot.Instance.Template.LoadFromDatabase", true);
    bool createDefaults = sPlayerbotConfig->GetBool("Playerbot.Instance.Template.CreateDefaults", true);
    bool preSerialize = sPlayerbotConfig->GetBool("Playerbot.Instance.Template.PreSerialize", true);
    _logUsage = sPlayerbotConfig->GetBool("Playerbot.Instance.Template.LogUsage", false);
    _logCreation = sPlayerbotConfig->GetBool("Playerbot.Instance.Template.LogCreation", true);

    // Parse gear level configuration
    std::string gearLevelsStr = sPlayerbotConfig->GetString("Playerbot.Instance.Template.GearLevels",
        "400,450,480,510,545,580");
    std::stringstream ss(gearLevelsStr);
    std::string level;
    _gearLevels.clear();
    while (std::getline(ss, level, ','))
    {
        try
        {
            _gearLevels.push_back(static_cast<uint32>(std::stoi(level)));
        }
        catch (...)
        {
            TC_LOG_WARN("playerbot.template", "Invalid gear level in config: {}", level);
        }
    }
    if (_gearLevels.empty())
        _gearLevels = {400, 450, 480, 510, 545, 580};

    _defaultGearLevel = sPlayerbotConfig->GetInt("Playerbot.Instance.Template.DefaultGearLevel", 450);
    _scaleGearToContent = sPlayerbotConfig->GetBool("Playerbot.Instance.Template.ScaleGearToContent", true);

    if (_logCreation)
    {
        TC_LOG_INFO("playerbot.template", "Template config: LoadFromDB={}, CreateDefaults={}, PreSerialize={}",
            loadFromDb, createDefaults, preSerialize);
        TC_LOG_INFO("playerbot.template", "Gear levels: {} levels, default iLvl={}, scale={}",
            _gearLevels.size(), _defaultGearLevel, _scaleGearToContent);
    }

    // Try to load templates from database first
    if (loadFromDb)
        LoadFromDatabase();

    // If no templates loaded from database, create defaults
    if (_templates.empty() && createDefaults)
    {
        TC_LOG_INFO("playerbot.template", "No templates in database, creating default templates...");
        CreateDefaultTemplates();

        // Save the newly created templates to database for next time
        if (loadFromDb)
            SaveToDatabase();
    }

    // Pre-serialize templates if enabled
    if (preSerialize)
    {
        std::shared_lock lock(_mutex);
        for (auto& [id, tmpl] : _templates)
        {
            tmpl->PreSerialize();
        }
    }

    _initialized.store(true);

    TC_LOG_INFO("playerbot.template", "Bot Template Repository initialized with {} templates",
        GetTemplateCount());

    return true;
}

void BotTemplateRepository::Shutdown()
{
    if (!_initialized.load())
        return;

    TC_LOG_INFO("playerbot.template", "Shutting down Bot Template Repository...");

    std::unique_lock lock(_mutex);
    _templates.clear();
    _classSpecIndex.clear();
    _classRoleIndex.clear();
    _roleIndex.clear();

    _initialized.store(false);
}

void BotTemplateRepository::LoadFromDatabase()
{
    TC_LOG_INFO("playerbot.template", "Loading bot templates from database...");

    if (!sPlayerbotDatabase->IsConnected())
    {
        TC_LOG_WARN("playerbot.template", "Playerbot database not connected, skipping database load");
        return;
    }

    uint32 loadedTemplates = 0;
    uint32 loadedGearSets = 0;
    uint32 loadedTalents = 0;
    uint32 loadedActions = 0;

    // ==========================================================================
    // STEP 1: Load spec info for all known specs
    // ==========================================================================
    std::unordered_map<uint32, std::tuple<std::string, uint8, uint8>> specInfoMap;

    QueryResult specResult = sPlayerbotDatabase->Query(
        "SELECT spec_id, spec_name, class_id, role FROM playerbot_spec_info WHERE enabled = 1");

    if (specResult)
    {
        do
        {
            Field* fields = specResult->Fetch();
            uint32 specId = fields[0].GetUInt32();
            std::string specName = fields[1].GetString();
            uint8 classId = fields[2].GetUInt8();
            std::string roleStr = fields[3].GetString();
            BotRole role = StringToBotRole(roleStr);
            specInfoMap[specId] = std::make_tuple(specName, classId, static_cast<uint8>(role));
        } while (specResult->NextRow());

        TC_LOG_INFO("playerbot.template", "Loaded {} spec definitions from database", specInfoMap.size());
    }

    // ==========================================================================
    // STEP 2: Load class/race matrix
    // ==========================================================================
    std::unordered_map<uint64, std::vector<uint8>> classRaceMatrix; // (class << 8 | faction) -> races

    QueryResult raceResult = sPlayerbotDatabase->Query(
        "SELECT class_id, race_id, faction FROM playerbot_class_race_matrix WHERE enabled = 1 ORDER BY weight DESC");

    if (raceResult)
    {
        uint32 validEntries = 0;

        do
        {
            Field* fields = raceResult->Fetch();
            uint8 classId = fields[0].GetUInt8();
            uint8 raceId = fields[1].GetUInt8();
            std::string factionStr = fields[2].GetString();

            // Convert faction string to numeric value: 0 = Alliance, 1 = Horde
            uint8 faction = (factionStr == "HORDE" || factionStr == "Horde") ? 1 : 0;

            uint64 key = (static_cast<uint64>(classId) << 8) | faction;
            classRaceMatrix[key].push_back(raceId);
            ++validEntries;

            TC_LOG_TRACE("playerbot.template",
                "Loaded class/race matrix: class={} race={} faction={}",
                classId, raceId, factionStr);
        } while (raceResult->NextRow());

        TC_LOG_INFO("playerbot.template", "Loaded class/race matrix: {} entries", validEntries);
    }

    // ==========================================================================
    // STEP 3: Load template definitions
    // ==========================================================================
    QueryResult templateResult = sPlayerbotDatabase->Query(
        "SELECT template_id, template_name, class_id, spec_id, role, enabled, "
        "talent_blob, actionbar_blob, priority_weight "
        "FROM playerbot_bot_templates WHERE enabled = 1 ORDER BY template_id");

    if (!templateResult)
    {
        TC_LOG_INFO("playerbot.template", "No templates found in database");
        return;
    }

    std::unique_lock lock(_mutex);

    do
    {
        Field* fields = templateResult->Fetch();

        auto tmpl = std::make_unique<BotTemplate>();
        tmpl->templateId = fields[0].GetUInt32();
        tmpl->templateName = fields[1].GetString();
        tmpl->playerClass = fields[2].GetUInt8();
        tmpl->specId = fields[3].GetUInt32();
        // Note: Database stores role as TINYINT (0=Tank, 1=Healer, 2=DPS), not string
        tmpl->role = static_cast<BotRole>(fields[4].GetUInt8());

        // VALIDATION: Skip invalid templates with class_id=0 (not a valid WoW class)
        // This can happen due to corrupt database entries
        if (tmpl->playerClass == 0 || tmpl->playerClass > 13)
        {
            TC_LOG_ERROR("playerbot.template",
                "SKIPPING INVALID TEMPLATE: id={}, name='{}' has invalid class_id={} (valid: 1-13). "
                "Run fix_corrupt_template_entry.sql to clean up database.",
                tmpl->templateId, tmpl->templateName, tmpl->playerClass);
            continue;
        }

        // Also validate spec_id is not 0
        if (tmpl->specId == 0)
        {
            TC_LOG_ERROR("playerbot.template",
                "SKIPPING INVALID TEMPLATE: id={}, name='{}' has invalid spec_id=0. "
                "Run fix_corrupt_template_entry.sql to clean up database.",
                tmpl->templateId, tmpl->templateName);
            continue;
        }

        // Get class and spec names
        tmpl->className = GetClassName(tmpl->playerClass);
        tmpl->specName = GetSpecName(tmpl->specId);

        // Load talent blob if stored directly
        std::string talentBlobStr = fields[6].GetString();
        if (!talentBlobStr.empty())
        {
            // Decode hex string to binary
            std::vector<uint8> blob;
            for (size_t i = 0; i < talentBlobStr.length(); i += 2)
            {
                std::string byteStr = talentBlobStr.substr(i, 2);
                blob.push_back(static_cast<uint8>(std::stoul(byteStr, nullptr, 16)));
            }
            tmpl->talents = TalentTemplate::Deserialize(blob);
            tmpl->talentBlob = blob;
        }

        // Load action bar blob if stored directly
        std::string actionBlobStr = fields[7].GetString();
        if (!actionBlobStr.empty())
        {
            std::vector<uint8> blob;
            for (size_t i = 0; i < actionBlobStr.length(); i += 2)
            {
                std::string byteStr = actionBlobStr.substr(i, 2);
                blob.push_back(static_cast<uint8>(std::stoul(byteStr, nullptr, 16)));
            }
            tmpl->actionBars = ActionBarTemplate::Deserialize(blob);
            tmpl->actionBarBlob = blob;
        }

        // Get valid races for this class
        uint64 allianceKey = (static_cast<uint64>(tmpl->playerClass) << 8) | 0; // Alliance = 0
        uint64 hordeKey = (static_cast<uint64>(tmpl->playerClass) << 8) | 1;    // Horde = 1

        bool usedDBForAlliance = false, usedDBForHorde = false;
        auto alIt = classRaceMatrix.find(allianceKey);
        if (alIt != classRaceMatrix.end())
        {
            tmpl->allianceRaces = alIt->second;
            usedDBForAlliance = true;
        }

        auto hIt = classRaceMatrix.find(hordeKey);
        if (hIt != classRaceMatrix.end())
        {
            tmpl->hordeRaces = hIt->second;
            usedDBForHorde = true;
        }

        // If no races from database, use fallback
        bool usedFallbackForAlliance = false, usedFallbackForHorde = false;
        if (tmpl->allianceRaces.empty())
        {
            tmpl->allianceRaces = GetValidRaces(tmpl->playerClass, Faction::Alliance);
            usedFallbackForAlliance = true;
        }
        if (tmpl->hordeRaces.empty())
        {
            tmpl->hordeRaces = GetValidRaces(tmpl->playerClass, Faction::Horde);
            usedFallbackForHorde = true;
        }

        // Log race source info for tank templates to help debug JIT failure
        if (tmpl->role == BotRole::Tank && (tmpl->allianceRaces.empty() || tmpl->hordeRaces.empty()))
        {
            TC_LOG_ERROR("playerbot.template",
                "TANK {} (class={}) has EMPTY races after loading! Alliance: {} (DB={}, Fallback={}), Horde: {} (DB={}, Fallback={})",
                tmpl->templateName, tmpl->playerClass,
                tmpl->allianceRaces.size(), usedDBForAlliance, usedFallbackForAlliance,
                tmpl->hordeRaces.size(), usedDBForHorde, usedFallbackForHorde);
        }

        // DEBUG: Log ALL tank templates (role=0) specifically to debug JIT creation failure
        if (tmpl->role == BotRole::Tank)
        {
            TC_LOG_INFO("playerbot.template",
                "TANK Template loaded: {} (id={}, class={}, spec={}) - AllianceRaces={}, HordeRaces={}",
                tmpl->templateName, tmpl->templateId, tmpl->playerClass, tmpl->specId,
                tmpl->allianceRaces.size(), tmpl->hordeRaces.size());
        }
        // Also log first 3 templates regardless of role for general debugging
        else if (loadedTemplates < 3)
        {
            TC_LOG_INFO("playerbot.template",
                "Template {} ({}): class={}, role={}, allianceRaces={}, hordeRaces={}",
                tmpl->templateName, tmpl->templateId, tmpl->playerClass,
                static_cast<uint8>(tmpl->role),
                tmpl->allianceRaces.size(), tmpl->hordeRaces.size());
        }

        // Index the template
        uint64 classSpecKey = MakeTemplateKey(tmpl->playerClass, tmpl->specId);
        uint64 classRoleKey = MakeRoleKey(tmpl->playerClass, tmpl->role);

        _classSpecIndex[classSpecKey] = tmpl->templateId;
        _classRoleIndex[classRoleKey] = tmpl->templateId;
        _roleIndex[tmpl->role].push_back(tmpl->templateId);

        // Update next template ID
        if (tmpl->templateId >= _nextTemplateId.load())
            _nextTemplateId.store(tmpl->templateId + 1);

        _templates[tmpl->templateId] = std::move(tmpl);
        ++loadedTemplates;

    } while (templateResult->NextRow());

    // ==========================================================================
    // STEP 4: Load gear sets for each template
    // ==========================================================================
    QueryResult gearSetResult = sPlayerbotDatabase->Query(
        "SELECT gs.gear_set_id, gs.template_id, gs.target_ilvl, gs.actual_gear_score, gs.gear_set_name, "
        "gi.slot_id, gi.item_id, gi.item_level, gi.enchant_id, gi.gem1_id, gi.gem2_id, gi.gem3_id "
        "FROM playerbot_template_gear_sets gs "
        "LEFT JOIN playerbot_template_gear_items gi ON gs.gear_set_id = gi.gear_set_id "
        "WHERE gs.enabled = 1 "
        "ORDER BY gs.template_id, gs.target_ilvl, gi.slot_id");

    if (gearSetResult)
    {
        uint32 currentGearSetId = 0;
        uint32 currentTemplateId = 0;
        GearSetTemplate currentGearSet;

        do
        {
            Field* fields = gearSetResult->Fetch();

            uint32 gearSetId = fields[0].GetUInt32();
            uint32 templateId = fields[1].GetUInt32();

            // Check if this is a new gear set
            if (gearSetId != currentGearSetId)
            {
                // Save previous gear set if valid
                if (currentGearSetId != 0 && currentGearSet.IsValid())
                {
                    auto it = _templates.find(currentTemplateId);
                    if (it != _templates.end())
                    {
                        it->second->gearSets[currentGearSet.targetItemLevel] = currentGearSet;
                        ++loadedGearSets;
                    }
                }

                // Start new gear set
                currentGearSetId = gearSetId;
                currentTemplateId = templateId;
                currentGearSet = GearSetTemplate();
                currentGearSet.targetItemLevel = fields[2].GetUInt32();
                currentGearSet.actualGearScore = fields[3].GetUInt32();
            }

            // Load gear item if present
            if (!fields[5].IsNull())
            {
                uint8 slotId = fields[5].GetUInt8();
                if (slotId < 19)
                {
                    currentGearSet.slots[slotId].slotId = slotId;
                    currentGearSet.slots[slotId].itemId = fields[6].GetUInt32();
                    currentGearSet.slots[slotId].itemLevel = fields[7].GetUInt32();
                    currentGearSet.slots[slotId].enchantId = fields[8].GetUInt32();
                    currentGearSet.slots[slotId].gemIds[0] = fields[9].GetUInt32();
                    currentGearSet.slots[slotId].gemIds[1] = fields[10].GetUInt32();
                    currentGearSet.slots[slotId].gemIds[2] = fields[11].GetUInt32();
                }
            }

        } while (gearSetResult->NextRow());

        // Save last gear set
        if (currentGearSetId != 0 && currentGearSet.IsValid())
        {
            auto it = _templates.find(currentTemplateId);
            if (it != _templates.end())
            {
                it->second->gearSets[currentGearSet.targetItemLevel] = currentGearSet;
                ++loadedGearSets;
            }
        }
    }

    // ==========================================================================
    // STEP 5: Load talents for each template
    // ==========================================================================
    QueryResult talentResult = sPlayerbotDatabase->Query(
        "SELECT template_id, talent_tier, talent_column, talent_id, is_pvp_talent "
        "FROM playerbot_template_talents "
        "WHERE enabled = 1 "
        "ORDER BY template_id, is_pvp_talent, talent_tier, talent_column");

    if (talentResult)
    {
        uint32 currentTemplateId = 0;
        std::vector<uint32> pveTalents;
        std::vector<uint32> pvpTalents;

        do
        {
            Field* fields = talentResult->Fetch();

            uint32 templateId = fields[0].GetUInt32();

            // Check if switching to new template
            if (templateId != currentTemplateId)
            {
                // Save previous talents
                if (currentTemplateId != 0)
                {
                    auto it = _templates.find(currentTemplateId);
                    if (it != _templates.end())
                    {
                        it->second->talents.talentIds = pveTalents;
                        it->second->talents.pvpTalentIds = pvpTalents;
                        it->second->talents.specId = it->second->specId;
                        it->second->PreSerialize();
                        loadedTalents += pveTalents.size() + pvpTalents.size();
                    }
                }

                currentTemplateId = templateId;
                pveTalents.clear();
                pvpTalents.clear();
            }

            uint32 talentId = fields[3].GetUInt32();
            bool isPvP = fields[4].GetBool();

            if (isPvP)
                pvpTalents.push_back(talentId);
            else
                pveTalents.push_back(talentId);

        } while (talentResult->NextRow());

        // Save last template's talents
        if (currentTemplateId != 0)
        {
            auto it = _templates.find(currentTemplateId);
            if (it != _templates.end())
            {
                it->second->talents.talentIds = pveTalents;
                it->second->talents.pvpTalentIds = pvpTalents;
                it->second->talents.specId = it->second->specId;
                it->second->PreSerialize();
                loadedTalents += pveTalents.size() + pvpTalents.size();
            }
        }
    }

    // ==========================================================================
    // STEP 6: Load action bars for each template
    // ==========================================================================
    QueryResult actionResult = sPlayerbotDatabase->Query(
        "SELECT template_id, action_bar, slot, action_type, action_id "
        "FROM playerbot_template_actionbars "
        "WHERE enabled = 1 "
        "ORDER BY template_id, action_bar, slot");

    if (actionResult)
    {
        uint32 currentTemplateId = 0;
        std::vector<ActionBarButton> buttons;

        do
        {
            Field* fields = actionResult->Fetch();

            uint32 templateId = fields[0].GetUInt32();

            // Check if switching to new template
            if (templateId != currentTemplateId)
            {
                // Save previous action bars
                if (currentTemplateId != 0)
                {
                    auto it = _templates.find(currentTemplateId);
                    if (it != _templates.end())
                    {
                        it->second->actionBars.buttons = buttons;
                        it->second->PreSerialize();
                        loadedActions += buttons.size();
                    }
                }

                currentTemplateId = templateId;
                buttons.clear();
            }

            ActionBarButton button;
            button.actionBar = fields[1].GetUInt8();
            button.slot = fields[2].GetUInt8();
            button.actionType = fields[3].GetUInt32();
            button.actionId = fields[4].GetUInt32();
            buttons.push_back(button);

        } while (actionResult->NextRow());

        // Save last template's action bars
        if (currentTemplateId != 0)
        {
            auto it = _templates.find(currentTemplateId);
            if (it != _templates.end())
            {
                it->second->actionBars.buttons = buttons;
                it->second->PreSerialize();
                loadedActions += buttons.size();
            }
        }
    }

    TC_LOG_INFO("playerbot.template",
        "Database load complete: {} templates, {} gear sets, {} talents, {} action buttons",
        loadedTemplates, loadedGearSets, loadedTalents, loadedActions);

    // Log role distribution for debugging pool creation
    auto tankIt = _roleIndex.find(BotRole::Tank);
    auto healerIt = _roleIndex.find(BotRole::Healer);
    auto dpsIt = _roleIndex.find(BotRole::DPS);

    TC_LOG_INFO("playerbot.template",
        "Role distribution: Tank={}, Healer={}, DPS={}",
        tankIt != _roleIndex.end() ? tankIt->second.size() : 0,
        healerIt != _roleIndex.end() ? healerIt->second.size() : 0,
        dpsIt != _roleIndex.end() ? dpsIt->second.size() : 0);

    // Check for templates with valid races
    uint32 allianceValid = 0, hordeValid = 0;
    for (auto const& [id, tmpl] : _templates)
    {
        if (!tmpl->allianceRaces.empty()) ++allianceValid;
        if (!tmpl->hordeRaces.empty()) ++hordeValid;
    }
    TC_LOG_INFO("playerbot.template",
        "Templates with valid races: Alliance={}, Horde={}",
        allianceValid, hordeValid);
}

void BotTemplateRepository::SaveToDatabase()
{
    if (!sPlayerbotDatabase->IsConnected())
    {
        TC_LOG_WARN("playerbot.template", "Playerbot database not connected, skipping save");
        return;
    }

    TC_LOG_INFO("playerbot.template", "Saving {} templates to database...", _templates.size());

    std::shared_lock lock(_mutex);

    uint32 savedTemplates = 0;
    uint32 savedGearSets = 0;

    for (auto const& [id, tmpl] : _templates)
    {
        // =======================================================================
        // Save template definition
        // =======================================================================
        std::string talentBlobHex;
        for (uint8 byte : tmpl->talentBlob)
            talentBlobHex += fmt::format("{:02X}", byte);

        std::string actionBlobHex;
        for (uint8 byte : tmpl->actionBarBlob)
            actionBlobHex += fmt::format("{:02X}", byte);

        std::string insertSQL = fmt::format(
            "INSERT INTO playerbot_bot_templates "
            "(template_id, template_name, class_id, spec_id, role, enabled, talent_blob, actionbar_blob, priority_weight) "
            "VALUES ({}, '{}', {}, {}, {}, 1, '{}', '{}', 100) "
            "ON DUPLICATE KEY UPDATE "
            "template_name = VALUES(template_name), "
            "class_id = VALUES(class_id), "
            "spec_id = VALUES(spec_id), "
            "role = VALUES(role), "
            "talent_blob = VALUES(talent_blob), "
            "actionbar_blob = VALUES(actionbar_blob)",
            tmpl->templateId, tmpl->templateName, tmpl->playerClass, tmpl->specId,
            static_cast<uint8>(tmpl->role), talentBlobHex, actionBlobHex);

        if (sPlayerbotDatabase->Execute(insertSQL))
            ++savedTemplates;
        else
            TC_LOG_ERROR("playerbot.template", "Failed to save template {}", tmpl->templateId);

        // =======================================================================
        // Save gear sets
        // =======================================================================
        for (auto const& [ilvl, gearSet] : tmpl->gearSets)
        {
            std::string gearSetName = fmt::format("{}_iLvl{}", tmpl->templateName, ilvl);

            std::string gearSetSQL = fmt::format(
                "INSERT INTO playerbot_template_gear_sets "
                "(template_id, target_ilvl, actual_gear_score, gear_set_name, enabled) "
                "VALUES ({}, {}, {}, '{}', 1) "
                "ON DUPLICATE KEY UPDATE "
                "actual_gear_score = VALUES(actual_gear_score), "
                "gear_set_name = VALUES(gear_set_name)",
                tmpl->templateId, ilvl, gearSet.actualGearScore, gearSetName);

            if (sPlayerbotDatabase->Execute(gearSetSQL))
            {
                ++savedGearSets;

                // Get the gear_set_id for item insertion
                QueryResult idResult = sPlayerbotDatabase->Query(fmt::format(
                    "SELECT gear_set_id FROM playerbot_template_gear_sets "
                    "WHERE template_id = {} AND target_ilvl = {}",
                    tmpl->templateId, ilvl));

                if (idResult)
                {
                    uint32 gearSetId = idResult->Fetch()[0].GetUInt32();

                    // Save gear items
                    for (uint8 slot = 0; slot < 19; ++slot)
                    {
                        if (gearSet.slots[slot].itemId == 0)
                            continue;

                        std::string itemSQL = fmt::format(
                            "INSERT INTO playerbot_template_gear_items "
                            "(gear_set_id, slot_id, item_id, item_level, enchant_id, gem1_id, gem2_id, gem3_id) "
                            "VALUES ({}, {}, {}, {}, {}, {}, {}, {}) "
                            "ON DUPLICATE KEY UPDATE "
                            "item_id = VALUES(item_id), "
                            "item_level = VALUES(item_level), "
                            "enchant_id = VALUES(enchant_id)",
                            gearSetId, slot, gearSet.slots[slot].itemId,
                            gearSet.slots[slot].itemLevel, gearSet.slots[slot].enchantId,
                            gearSet.slots[slot].gemIds[0], gearSet.slots[slot].gemIds[1],
                            gearSet.slots[slot].gemIds[2]);

                        sPlayerbotDatabase->Execute(itemSQL);
                    }
                }
            }
        }

        // =======================================================================
        // Save talents
        // =======================================================================
        // First delete existing talents for this template
        sPlayerbotDatabase->Execute(fmt::format(
            "DELETE FROM playerbot_template_talents WHERE template_id = {}", tmpl->templateId));

        uint8 tier = 0;
        for (uint32 talentId : tmpl->talents.talentIds)
        {
            std::string talentSQL = fmt::format(
                "INSERT INTO playerbot_template_talents "
                "(template_id, talent_tier, talent_column, talent_id, is_pvp_talent, enabled) "
                "VALUES ({}, {}, 0, {}, 0, 1)",
                tmpl->templateId, tier++, talentId);
            sPlayerbotDatabase->Execute(talentSQL);
        }

        tier = 0;
        for (uint32 pvpTalentId : tmpl->talents.pvpTalentIds)
        {
            std::string pvpTalentSQL = fmt::format(
                "INSERT INTO playerbot_template_talents "
                "(template_id, talent_tier, talent_column, talent_id, is_pvp_talent, enabled) "
                "VALUES ({}, {}, 0, {}, 1, 1)",
                tmpl->templateId, tier++, pvpTalentId);
            sPlayerbotDatabase->Execute(pvpTalentSQL);
        }

        // =======================================================================
        // Save action bars
        // =======================================================================
        // First delete existing action bars for this template
        sPlayerbotDatabase->Execute(fmt::format(
            "DELETE FROM playerbot_template_actionbars WHERE template_id = {}", tmpl->templateId));

        for (auto const& button : tmpl->actionBars.buttons)
        {
            std::string actionSQL = fmt::format(
                "INSERT INTO playerbot_template_actionbars "
                "(template_id, action_bar, slot, action_type, action_id, enabled) "
                "VALUES ({}, {}, {}, {}, {}, 1)",
                tmpl->templateId, button.actionBar, button.slot,
                button.actionType, button.actionId);
            sPlayerbotDatabase->Execute(actionSQL);
        }
    }

    TC_LOG_INFO("playerbot.template",
        "Database save complete: {} templates, {} gear sets saved",
        savedTemplates, savedGearSets);
}

// ============================================================================
// TEMPLATE ACCESS
// ============================================================================

BotTemplate const* BotTemplateRepository::GetTemplate(uint8 playerClass, BotRole role) const
{
    std::shared_lock lock(_mutex);

    uint64 key = MakeRoleKey(playerClass, role);
    auto it = _classRoleIndex.find(key);
    if (it == _classRoleIndex.end())
        return nullptr;

    auto tmplIt = _templates.find(it->second);
    if (tmplIt == _templates.end())
        return nullptr;

    return tmplIt->second.get();
}

BotTemplate const* BotTemplateRepository::GetTemplate(uint8 playerClass, uint32 specId) const
{
    std::shared_lock lock(_mutex);

    uint64 key = MakeTemplateKey(playerClass, specId);
    auto it = _classSpecIndex.find(key);
    if (it == _classSpecIndex.end())
        return nullptr;

    auto tmplIt = _templates.find(it->second);
    if (tmplIt == _templates.end())
        return nullptr;

    return tmplIt->second.get();
}

BotTemplate const* BotTemplateRepository::GetTemplateById(uint32 templateId) const
{
    std::shared_lock lock(_mutex);

    auto it = _templates.find(templateId);
    if (it == _templates.end())
        return nullptr;

    return it->second.get();
}

std::vector<BotTemplate const*> BotTemplateRepository::GetTemplatesForRole(BotRole role) const
{
    std::shared_lock lock(_mutex);

    std::vector<BotTemplate const*> result;

    auto it = _roleIndex.find(role);
    if (it != _roleIndex.end())
    {
        for (uint32 id : it->second)
        {
            auto tmplIt = _templates.find(id);
            if (tmplIt != _templates.end())
                result.push_back(tmplIt->second.get());
        }
    }

    return result;
}

std::vector<BotTemplate const*> BotTemplateRepository::GetTemplatesForFaction(Faction faction) const
{
    std::shared_lock lock(_mutex);

    std::vector<BotTemplate const*> result;

    for (auto const& [id, tmpl] : _templates)
    {
        auto const& races = (faction == Faction::Alliance) ?
                           tmpl->allianceRaces : tmpl->hordeRaces;
        if (!races.empty())
            result.push_back(tmpl.get());
    }

    return result;
}

std::vector<BotTemplate const*> BotTemplateRepository::GetAllTemplates() const
{
    std::shared_lock lock(_mutex);

    std::vector<BotTemplate const*> result;
    result.reserve(_templates.size());

    for (auto const& [id, tmpl] : _templates)
        result.push_back(tmpl.get());

    return result;
}

uint32 BotTemplateRepository::GetTemplateCount() const
{
    std::shared_lock lock(_mutex);
    return static_cast<uint32>(_templates.size());
}

// ============================================================================
// TEMPLATE SELECTION
// ============================================================================

BotTemplate const* BotTemplateRepository::SelectBestTemplate(
    BotRole role, Faction faction, uint8 preferredClass) const
{
    std::shared_lock lock(_mutex);

    auto roleIt = _roleIndex.find(role);
    if (roleIt == _roleIndex.end() || roleIt->second.empty())
        return nullptr;

    // First try preferred class
    if (preferredClass != 0)
    {
        for (uint32 id : roleIt->second)
        {
            auto tmplIt = _templates.find(id);
            if (tmplIt != _templates.end())
            {
                auto const& tmpl = tmplIt->second;
                if (tmpl->playerClass == preferredClass)
                {
                    auto const& races = (faction == Faction::Alliance) ?
                                       tmpl->allianceRaces : tmpl->hordeRaces;
                    if (!races.empty())
                        return tmpl.get();
                }
            }
        }
    }

    // Return any matching template
    for (uint32 id : roleIt->second)
    {
        auto tmplIt = _templates.find(id);
        if (tmplIt != _templates.end())
        {
            auto const& tmpl = tmplIt->second;
            auto const& races = (faction == Faction::Alliance) ?
                               tmpl->allianceRaces : tmpl->hordeRaces;
            if (!races.empty())
                return tmpl.get();
        }
    }

    return nullptr;
}

BotTemplate const* BotTemplateRepository::SelectRandomTemplate(
    BotRole role, Faction faction) const
{
    auto templates = GetTemplatesForRole(role);
    if (templates.empty())
    {
        TC_LOG_ERROR("playerbot.template",
            "SelectRandomTemplate FAILED: No templates found for role {} (_roleIndex[{}] is empty or missing)",
            BotRoleToString(role), static_cast<uint8>(role));
        return nullptr;
    }

    TC_LOG_INFO("playerbot.template",
        "SelectRandomTemplate: Found {} templates for role {}, filtering for faction {}",
        templates.size(), BotRoleToString(role), FactionToString(faction));

    // Filter by faction
    std::vector<BotTemplate const*> validTemplates;
    for (auto const* tmpl : templates)
    {
        auto const& races = (faction == Faction::Alliance) ?
                           tmpl->allianceRaces : tmpl->hordeRaces;
        if (!races.empty())
        {
            validTemplates.push_back(tmpl);
        }
        else
        {
            TC_LOG_WARN("playerbot.template",
                "SelectRandomTemplate: Template {} ({}) rejected - no races for faction {} (Alliance: {}, Horde: {})",
                tmpl->templateName, tmpl->templateId, FactionToString(faction),
                tmpl->allianceRaces.size(), tmpl->hordeRaces.size());
        }
    }

    if (validTemplates.empty())
    {
        TC_LOG_ERROR("playerbot.template",
            "SelectRandomTemplate FAILED: All {} templates for role {} have empty races for faction {}!",
            templates.size(), BotRoleToString(role), FactionToString(faction));
        return nullptr;
    }

    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, validTemplates.size() - 1);
    return validTemplates[dist(gen)];
}

// ============================================================================
// STATISTICS
// ============================================================================

void BotTemplateRepository::RecordTemplateUsage(uint32 templateId, std::chrono::milliseconds creationTime)
{
    std::unique_lock lock(_mutex);

    auto it = _templates.find(templateId);
    if (it == _templates.end())
        return;

    auto& tmpl = it->second;

    // Update running average
    if (tmpl->usageCount == 0)
    {
        tmpl->avgCreationTime = creationTime;
    }
    else
    {
        auto avgMs = tmpl->avgCreationTime.count();
        avgMs = (avgMs * tmpl->usageCount + creationTime.count()) / (tmpl->usageCount + 1);
        tmpl->avgCreationTime = std::chrono::milliseconds(avgMs);
    }

    ++tmpl->usageCount;

    // Update statistics in database periodically
    if (tmpl->usageCount % 100 == 0)
    {
        sPlayerbotDatabase->Execute(fmt::format(
            "INSERT INTO playerbot_template_statistics "
            "(template_id, total_uses, avg_creation_time_ms) "
            "VALUES ({}, {}, {}) "
            "ON DUPLICATE KEY UPDATE "
            "total_uses = total_uses + 100, "
            "avg_creation_time_ms = {}",
            templateId, tmpl->usageCount, tmpl->avgCreationTime.count(),
            tmpl->avgCreationTime.count()));
    }
}

void BotTemplateRepository::PrintStatistics() const
{
    std::shared_lock lock(_mutex);

    TC_LOG_INFO("playerbot.template", "=== Bot Template Statistics ===");
    TC_LOG_INFO("playerbot.template", "Total templates: {}", _templates.size());

    uint32 totalUsage = 0;
    for (auto const& [id, tmpl] : _templates)
    {
        TC_LOG_INFO("playerbot.template", "  {} (ID {}): {} uses, avg {}ms, {} gear sets",
            tmpl->templateName, id, tmpl->usageCount,
            tmpl->avgCreationTime.count(), tmpl->gearSets.size());
        totalUsage += tmpl->usageCount;
    }

    TC_LOG_INFO("playerbot.template", "Total template uses: {}", totalUsage);

    // Print role distribution
    for (auto const& [role, ids] : _roleIndex)
    {
        TC_LOG_INFO("playerbot.template", "  {}: {} templates",
            BotRoleToString(role), ids.size());
    }
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void BotTemplateRepository::CreateDefaultTemplates()
{
    TC_LOG_INFO("playerbot.template", "Creating default bot templates...");

    // Warrior specs
    CreateClassTemplate(Classes::WARRIOR, 71, BotRole::DPS);     // Arms
    CreateClassTemplate(Classes::WARRIOR, 72, BotRole::DPS);     // Fury
    CreateClassTemplate(Classes::WARRIOR, 73, BotRole::Tank);    // Protection

    // Paladin specs
    CreateClassTemplate(Classes::PALADIN, 65, BotRole::Healer);  // Holy
    CreateClassTemplate(Classes::PALADIN, 66, BotRole::Tank);    // Protection
    CreateClassTemplate(Classes::PALADIN, 70, BotRole::DPS);     // Retribution

    // Hunter specs
    CreateClassTemplate(Classes::HUNTER, 253, BotRole::DPS);     // Beast Mastery
    CreateClassTemplate(Classes::HUNTER, 254, BotRole::DPS);     // Marksmanship
    CreateClassTemplate(Classes::HUNTER, 255, BotRole::DPS);     // Survival

    // Rogue specs
    CreateClassTemplate(Classes::ROGUE, 259, BotRole::DPS);      // Assassination
    CreateClassTemplate(Classes::ROGUE, 260, BotRole::DPS);      // Outlaw
    CreateClassTemplate(Classes::ROGUE, 261, BotRole::DPS);      // Subtlety

    // Priest specs
    CreateClassTemplate(Classes::PRIEST, 256, BotRole::Healer);  // Discipline
    CreateClassTemplate(Classes::PRIEST, 257, BotRole::Healer);  // Holy
    CreateClassTemplate(Classes::PRIEST, 258, BotRole::DPS);     // Shadow

    // Death Knight specs
    CreateClassTemplate(Classes::DEATH_KNIGHT, 250, BotRole::Tank);   // Blood
    CreateClassTemplate(Classes::DEATH_KNIGHT, 251, BotRole::DPS);    // Frost
    CreateClassTemplate(Classes::DEATH_KNIGHT, 252, BotRole::DPS);    // Unholy

    // Shaman specs
    CreateClassTemplate(Classes::SHAMAN, 262, BotRole::DPS);     // Elemental
    CreateClassTemplate(Classes::SHAMAN, 263, BotRole::DPS);     // Enhancement
    CreateClassTemplate(Classes::SHAMAN, 264, BotRole::Healer);  // Restoration

    // Mage specs
    CreateClassTemplate(Classes::MAGE, 62, BotRole::DPS);        // Arcane
    CreateClassTemplate(Classes::MAGE, 63, BotRole::DPS);        // Fire
    CreateClassTemplate(Classes::MAGE, 64, BotRole::DPS);        // Frost

    // Warlock specs
    CreateClassTemplate(Classes::WARLOCK, 265, BotRole::DPS);    // Affliction
    CreateClassTemplate(Classes::WARLOCK, 266, BotRole::DPS);    // Demonology
    CreateClassTemplate(Classes::WARLOCK, 267, BotRole::DPS);    // Destruction

    // Monk specs
    CreateClassTemplate(Classes::MONK, 268, BotRole::Tank);      // Brewmaster
    CreateClassTemplate(Classes::MONK, 270, BotRole::Healer);    // Mistweaver
    CreateClassTemplate(Classes::MONK, 269, BotRole::DPS);       // Windwalker

    // Druid specs
    CreateClassTemplate(Classes::DRUID, 102, BotRole::DPS);      // Balance
    CreateClassTemplate(Classes::DRUID, 103, BotRole::DPS);      // Feral
    CreateClassTemplate(Classes::DRUID, 104, BotRole::Tank);     // Guardian
    CreateClassTemplate(Classes::DRUID, 105, BotRole::Healer);   // Restoration

    // Demon Hunter specs
    CreateClassTemplate(Classes::DEMON_HUNTER, 577, BotRole::DPS);   // Havoc
    CreateClassTemplate(Classes::DEMON_HUNTER, 581, BotRole::Tank);  // Vengeance

    // Evoker specs
    CreateClassTemplate(Classes::EVOKER, 1467, BotRole::DPS);    // Devastation
    CreateClassTemplate(Classes::EVOKER, 1468, BotRole::Healer); // Preservation
    CreateClassTemplate(Classes::EVOKER, 1473, BotRole::DPS);    // Augmentation

    TC_LOG_INFO("playerbot.template", "Created {} default templates", GetTemplateCount());
}

void BotTemplateRepository::CreateClassTemplate(uint8 playerClass, uint32 specId, BotRole role)
{
    auto tmpl = std::make_unique<BotTemplate>();

    uint32 templateId = _nextTemplateId.fetch_add(1);

    tmpl->templateId = templateId;
    tmpl->playerClass = playerClass;
    tmpl->specId = specId;
    tmpl->role = role;
    tmpl->className = GetClassName(playerClass);
    tmpl->specName = GetSpecName(specId);
    tmpl->templateName = fmt::format("{}_{}", tmpl->className, tmpl->specName);

    // Get valid races
    tmpl->allianceRaces = GetValidRaces(playerClass, Faction::Alliance);
    tmpl->hordeRaces = GetValidRaces(playerClass, Faction::Horde);

    // Generate talent configuration
    GenerateTalents(*tmpl);

    // Generate action bar configuration
    GenerateActionBars(*tmpl);

    // Generate gear sets for different item levels
    GenerateGearSet(*tmpl, 400);  // Entry-level
    GenerateGearSet(*tmpl, 450);  // Mid-tier
    GenerateGearSet(*tmpl, 500);  // High-tier
    GenerateGearSet(*tmpl, 550);  // End-game

    // Pre-serialize for fast application
    tmpl->PreSerialize();

    // Index the template
    uint64 classSpecKey = MakeTemplateKey(playerClass, specId);
    uint64 classRoleKey = MakeRoleKey(playerClass, role);

    std::unique_lock lock(_mutex);

    _classSpecIndex[classSpecKey] = templateId;
    _classRoleIndex[classRoleKey] = templateId;
    _roleIndex[role].push_back(templateId);
    _templates[templateId] = std::move(tmpl);
}

void BotTemplateRepository::GenerateGearSet(BotTemplate& tmpl, uint32 itemLevel)
{
    GearSetTemplate gearSet;
    gearSet.targetItemLevel = itemLevel;
    gearSet.actualGearScore = itemLevel;

    // Determine armor type based on class
    uint8 armorType = 0; // 0=Cloth, 1=Leather, 2=Mail, 3=Plate
    switch (tmpl.playerClass)
    {
        case Classes::MAGE:
        case Classes::PRIEST:
        case Classes::WARLOCK:
            armorType = 0; // Cloth
            break;
        case Classes::ROGUE:
        case Classes::DRUID:
        case Classes::MONK:
        case Classes::DEMON_HUNTER:
            armorType = 1; // Leather
            break;
        case Classes::HUNTER:
        case Classes::SHAMAN:
        case Classes::EVOKER:
            armorType = 2; // Mail
            break;
        case Classes::WARRIOR:
        case Classes::PALADIN:
        case Classes::DEATH_KNIGHT:
            armorType = 3; // Plate
            break;
    }

    // Equipment slot mapping (TrinityCore EQUIPMENT_SLOT_xxx values)
    // 0=HEAD, 1=NECK, 2=SHOULDER, 3=BODY(shirt), 4=CHEST, 5=WAIST, 6=LEGS, 7=FEET,
    // 8=WRIST, 9=HANDS, 10=FINGER1, 11=FINGER2, 12=TRINKET1, 13=TRINKET2,
    // 14=BACK, 15=MAIN_HAND, 16=OFF_HAND, 17=RANGED/RELIC, 18=TABARD

    // For default templates, we create placeholder gear entries
    // Real gear will be populated from the database or by the admin
    for (uint8 slot = 0; slot < 19; ++slot)
    {
        gearSet.slots[slot].slotId = slot;
        gearSet.slots[slot].itemLevel = itemLevel;

        // Skip shirt (3), ranged (17), and tabard (18) for simplicity
        if (slot == 3 || slot == 17 || slot == 18)
            continue;

        // Item IDs would normally be looked up from item_template
        // For default templates, we leave itemId = 0 to indicate "needs configuration"
        // The BotCloneEngine will use the BotGearFactory to generate appropriate items
        gearSet.slots[slot].itemId = 0;
    }

    tmpl.gearSets[itemLevel] = std::move(gearSet);
}

void BotTemplateRepository::GenerateTalents(BotTemplate& tmpl)
{
    tmpl.talents.specId = tmpl.specId;
    tmpl.talents.specName = tmpl.specName;

    // For default templates, we create empty talent lists
    // These will be populated from the database or configured by admin
    // The talent system in modern WoW is complex with talent trees, loadouts, etc.

    // Hero talent tree placeholder
    tmpl.talents.heroTalentTreeId = 0;

    // Talent IDs would be populated from:
    // 1. Database (playerbot_template_talents)
    // 2. Default talent builds from game data
    // 3. Community-provided "meta" builds

    // For now, leave empty - the BotCloneEngine can use the player's
    // default talents or a talent manager to assign appropriate talents
}

void BotTemplateRepository::GenerateActionBars(BotTemplate& tmpl)
{
    // For default templates, we create empty action bar configs
    // These will be populated from the database or generated based on class/spec

    // Action bar layout generation is complex and depends on:
    // 1. Class abilities available at the target level
    // 2. Spec-specific abilities
    // 3. Rotation priority (main abilities on easy-to-reach slots)
    // 4. Cooldowns, trinkets, consumables

    // The BotAI system will handle actual ability usage, so action bars
    // are primarily for visual consistency when inspecting bots

    // Placeholder: Add basic auto-attack for all classes
    ActionBarButton autoAttack;
    autoAttack.actionBar = 0;
    autoAttack.slot = 0;
    autoAttack.actionType = 0; // ACTION_BUTTON_SPELL
    autoAttack.actionId = 6603; // Auto Attack spell ID
    tmpl.actionBars.buttons.push_back(autoAttack);
}

std::vector<uint8> BotTemplateRepository::GetValidRaces(uint8 playerClass, Faction faction) const
{
    std::vector<uint8> races;

    // WoW 12.0 (The War Within) class/race combinations
    // This is a comprehensive list including all allied races
    //
    // NOTE (2026-01-12): Earthen races (85=Alliance, 84=Horde) are DISABLED
    // because playercreateinfo table doesn't have proper entries for them yet.
    // Player::Create() fails with "No PlayerInfo for race X class Y" errors.
    // Re-enable when database is updated with Earthen starting positions.

    if (faction == Faction::Alliance)
    {
        switch (playerClass)
        {
            case Classes::WARRIOR:
                races = {Races::HUMAN, Races::DWARF, Races::NIGHT_ELF, Races::GNOME,
                        Races::DRAENEI, Races::WORGEN, Races::PANDAREN_ALLIANCE,
                        Races::VOID_ELF, Races::LIGHTFORGED, Races::DARK_IRON,
                        Races::KUL_TIRAN, Races::MECHAGNOME};
                break;
            case Classes::PALADIN:
                races = {Races::HUMAN, Races::DWARF, Races::DRAENEI,
                        Races::LIGHTFORGED, Races::DARK_IRON};
                break;
            case Classes::HUNTER:
                races = {Races::HUMAN, Races::DWARF, Races::NIGHT_ELF, Races::GNOME,
                        Races::DRAENEI, Races::WORGEN, Races::PANDAREN_ALLIANCE,
                        Races::VOID_ELF, Races::LIGHTFORGED, Races::DARK_IRON,
                        Races::KUL_TIRAN, Races::MECHAGNOME};
                break;
            case Classes::ROGUE:
                races = {Races::HUMAN, Races::DWARF, Races::NIGHT_ELF, Races::GNOME,
                        Races::WORGEN, Races::PANDAREN_ALLIANCE, Races::VOID_ELF,
                        Races::DARK_IRON, Races::KUL_TIRAN, Races::MECHAGNOME};
                break;
            case Classes::PRIEST:
                races = {Races::HUMAN, Races::DWARF, Races::NIGHT_ELF, Races::GNOME,
                        Races::DRAENEI, Races::WORGEN, Races::PANDAREN_ALLIANCE,
                        Races::VOID_ELF, Races::LIGHTFORGED, Races::DARK_IRON,
                        Races::KUL_TIRAN, Races::MECHAGNOME};
                break;
            case Classes::DEATH_KNIGHT:
                races = {Races::HUMAN, Races::DWARF, Races::NIGHT_ELF, Races::GNOME,
                        Races::DRAENEI, Races::WORGEN, Races::PANDAREN_ALLIANCE,
                        Races::VOID_ELF, Races::LIGHTFORGED, Races::DARK_IRON,
                        Races::KUL_TIRAN, Races::MECHAGNOME};
                break;
            case Classes::SHAMAN:
                races = {Races::DWARF, Races::DRAENEI, Races::PANDAREN_ALLIANCE,
                        Races::DARK_IRON, Races::KUL_TIRAN};
                break;
            case Classes::MAGE:
                races = {Races::HUMAN, Races::DWARF, Races::NIGHT_ELF, Races::GNOME,
                        Races::DRAENEI, Races::WORGEN, Races::PANDAREN_ALLIANCE,
                        Races::VOID_ELF, Races::LIGHTFORGED, Races::DARK_IRON,
                        Races::KUL_TIRAN, Races::MECHAGNOME};
                break;
            case Classes::WARLOCK:
                races = {Races::HUMAN, Races::DWARF, Races::GNOME, Races::WORGEN,
                        Races::VOID_ELF, Races::DARK_IRON, Races::MECHAGNOME};
                break;
            case Classes::MONK:
                races = {Races::HUMAN, Races::DWARF, Races::NIGHT_ELF, Races::GNOME,
                        Races::DRAENEI, Races::PANDAREN_ALLIANCE, Races::VOID_ELF,
                        Races::DARK_IRON, Races::KUL_TIRAN, Races::MECHAGNOME};
                break;
            case Classes::DRUID:
                races = {Races::NIGHT_ELF, Races::WORGEN, Races::KUL_TIRAN};
                break;
            case Classes::DEMON_HUNTER:
                races = {Races::NIGHT_ELF};
                break;
            case Classes::EVOKER:
                races = {Races::DRACTHYR_ALLIANCE};
                break;
            default:
                TC_LOG_ERROR("playerbot.template",
                    "GetValidRaces: Unknown Alliance class {} - no races available!", playerClass);
                break;
        }
    }
    else // Horde
    {
        switch (playerClass)
        {
            case Classes::WARRIOR:
                races = {Races::ORC, Races::UNDEAD, Races::TAUREN, Races::TROLL,
                        Races::GOBLIN, Races::BLOOD_ELF, Races::PANDAREN_HORDE,
                        Races::NIGHTBORNE, Races::HIGHMOUNTAIN, Races::MAGHAR,
                        Races::ZANDALARI, Races::VULPERA};
                break;
            case Classes::PALADIN:
                races = {Races::TAUREN, Races::BLOOD_ELF, Races::ZANDALARI};
                break;
            case Classes::HUNTER:
                races = {Races::ORC, Races::UNDEAD, Races::TAUREN, Races::TROLL,
                        Races::GOBLIN, Races::BLOOD_ELF, Races::PANDAREN_HORDE,
                        Races::NIGHTBORNE, Races::HIGHMOUNTAIN, Races::MAGHAR,
                        Races::ZANDALARI, Races::VULPERA};
                break;
            case Classes::ROGUE:
                races = {Races::ORC, Races::UNDEAD, Races::TROLL, Races::GOBLIN,
                        Races::BLOOD_ELF, Races::PANDAREN_HORDE, Races::NIGHTBORNE,
                        Races::MAGHAR, Races::ZANDALARI, Races::VULPERA};
                break;
            case Classes::PRIEST:
                races = {Races::UNDEAD, Races::TAUREN, Races::TROLL, Races::GOBLIN,
                        Races::BLOOD_ELF, Races::PANDAREN_HORDE, Races::NIGHTBORNE,
                        Races::ZANDALARI, Races::VULPERA};
                break;
            case Classes::DEATH_KNIGHT:
                races = {Races::ORC, Races::UNDEAD, Races::TAUREN, Races::TROLL,
                        Races::GOBLIN, Races::BLOOD_ELF, Races::PANDAREN_HORDE,
                        Races::NIGHTBORNE, Races::HIGHMOUNTAIN, Races::MAGHAR,
                        Races::ZANDALARI, Races::VULPERA};
                break;
            case Classes::SHAMAN:
                races = {Races::ORC, Races::TAUREN, Races::TROLL, Races::GOBLIN,
                        Races::PANDAREN_HORDE, Races::HIGHMOUNTAIN, Races::MAGHAR,
                        Races::ZANDALARI, Races::VULPERA};
                break;
            case Classes::MAGE:
                races = {Races::ORC, Races::UNDEAD, Races::TROLL, Races::GOBLIN,
                        Races::BLOOD_ELF, Races::PANDAREN_HORDE, Races::NIGHTBORNE,
                        Races::MAGHAR, Races::ZANDALARI, Races::VULPERA};
                break;
            case Classes::WARLOCK:
                races = {Races::ORC, Races::UNDEAD, Races::TROLL, Races::GOBLIN,
                        Races::BLOOD_ELF, Races::NIGHTBORNE, Races::VULPERA};
                break;
            case Classes::MONK:
                races = {Races::ORC, Races::UNDEAD, Races::TAUREN, Races::TROLL,
                        Races::BLOOD_ELF, Races::PANDAREN_HORDE, Races::NIGHTBORNE,
                        Races::HIGHMOUNTAIN, Races::MAGHAR, Races::ZANDALARI,
                        Races::VULPERA};
                break;
            case Classes::DRUID:
                races = {Races::TAUREN, Races::TROLL, Races::HIGHMOUNTAIN,
                        Races::ZANDALARI};
                break;
            case Classes::DEMON_HUNTER:
                races = {Races::BLOOD_ELF};
                break;
            case Classes::EVOKER:
                races = {Races::DRACTHYR_HORDE};
                break;
            default:
                TC_LOG_ERROR("playerbot.template",
                    "GetValidRaces: Unknown Horde class {} - no races available!", playerClass);
                break;
        }
    }

    // Return all valid races for this class/faction
    // Player::Create() will validate against playercreateinfo if creation fails
    return races;
}

uint64 BotTemplateRepository::MakeTemplateKey(uint8 playerClass, uint32 specId)
{
    return (static_cast<uint64>(playerClass) << 32) | specId;
}

uint64 BotTemplateRepository::MakeRoleKey(uint8 playerClass, BotRole role)
{
    return (static_cast<uint64>(playerClass) << 8) | static_cast<uint8>(role);
}

} // namespace Playerbot
