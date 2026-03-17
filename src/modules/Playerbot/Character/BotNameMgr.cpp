/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotNameMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "CharacterCache.h"
#include "Database/PlayerbotDatabase.h"
#include "PreparedStatement.h"
#include "Random.h"
#include "Config/PlayerbotConfig.h"
#include <random>
#include <algorithm>
#include <sstream>

BotNameMgr* BotNameMgr::instance()
{
    static BotNameMgr instance;
    return &instance;
}

bool BotNameMgr::Initialize()
{
    // Load configuration from PlayerbotConfig
    _useRandomNames = sPlayerbotConfig->GetBool("Playerbot.Names.UseRandomNames", true);
    _minLength = sPlayerbotConfig->GetInt("Playerbot.Names.MinLength", 4);
    _maxLength = sPlayerbotConfig->GetInt("Playerbot.Names.MaxLength", 12);
    _useRaceTheme = sPlayerbotConfig->GetBool("Playerbot.Names.UseRaceTheme", true);

    TC_LOG_DEBUG("module.playerbot.names",
        "BotNameMgr: Config loaded - UseRandom=%d, MinLen=%u, MaxLen=%u, UseRaceTheme=%d",
        _useRandomNames, _minLength, _maxLength, _useRaceTheme);

    // Load names from playerbot database first (primary source)
    LoadNamesFromDatabase();
    LoadUsedNames();

    // CRITICAL: Cross-reference with actual characters table to prevent duplicate key errors
    // Names may exist in characters table from previous sessions that weren't properly tracked
    SyncWithCharactersTable();

    TC_LOG_INFO("module.playerbot.names",
        "Loaded {} names ({} male, {} female), {} in use",
        _names.size(),
        _availableMaleNames.size() + (_guidToNameId.size() / 2), // Approximate
        _availableFemaleNames.size() + (_guidToNameId.size() / 2),
        _guidToNameId.size());

    return true;
}

void BotNameMgr::Shutdown()
{
    std::lock_guard lock(_mutex);
    _names.clear();
    _availableMaleNames.clear();
    _availableFemaleNames.clear();
    _nameToId.clear();
    _guidToNameId.clear();
    _nameIdToGuid.clear();
}

std::string BotNameMgr::AllocateName(uint8 gender, uint32 characterGuid)
{
    std::lock_guard lock(_mutex);
    
    // Check if character already has a name
    auto it = _guidToNameId.find(characterGuid);
    if (it != _guidToNameId.end())
    {
        auto nameIt = _names.find(it->second);
        if (nameIt != _names.end())
        {
            TC_LOG_WARN("module.playerbot.names", 
                "Character {} already has name '{}'", 
                characterGuid, nameIt->second.name);
            return nameIt->second.name;
        }
    }
    
    // Get available names for gender
    std::unordered_set<uint32>* availableNames = nullptr;
    if (gender == 0) // Male
        availableNames = &_availableMaleNames;
    else if (gender == 1) // Female
        availableNames = &_availableFemaleNames;
    else
    {
        TC_LOG_ERROR("module.playerbot.names", 
            "Invalid gender {} for name allocation", gender);
        return "";
    }
    
    if (availableNames->empty())
    {
        TC_LOG_ERROR("module.playerbot.names", 
            "No available names for gender {}", gender);
        return "";
    }
    
    // Pick a random available name using TrinityCore's thread-safe random generator
    size_t randomIndex = urand(0, availableNames->size() - 1);

    auto iter = availableNames->begin();
    std::advance(iter, randomIndex);
    uint32 selectedNameId = *iter;
    
    // Get the name entry
    auto nameIt = _names.find(selectedNameId);
    if (nameIt == _names.end())
    {
        TC_LOG_ERROR("module.playerbot.names", 
            "Name ID {} not found in names map", selectedNameId);
        return "";
    }
    
    NameEntry& entry = nameIt->second;
    std::string allocatedName = entry.name;
    
    // Mark as used
    entry.used = true;
    entry.usedByGuid = characterGuid;
    
    // Update available names set
    availableNames->erase(selectedNameId);
    
    // Update mappings
    _guidToNameId[characterGuid] = selectedNameId;
    _nameIdToGuid[selectedNameId] = characterGuid;

    // Persist to database - INSERT INTO playerbots_names_used
    std::ostringstream ss;
    ss << "INSERT INTO playerbots_names_used (name_id, character_guid) VALUES ("
       << selectedNameId << ", " << characterGuid << ") "
       << "ON DUPLICATE KEY UPDATE character_guid = VALUES(character_guid)";

    if (!sPlayerbotDatabase->Execute(ss.str()))
    {
        TC_LOG_ERROR("module.playerbot.names",
            "Failed to persist name allocation to database: name_id={}, character_guid={}",
            selectedNameId, characterGuid);
    }

    TC_LOG_INFO("module.playerbot.names",
        "Allocated name '{}' (ID: {}) to character {}",
        allocatedName, selectedNameId, characterGuid);

    return allocatedName;
}

void BotNameMgr::ReleaseName(uint32 characterGuid)
{
    std::lock_guard lock(_mutex);
    
    auto it = _guidToNameId.find(characterGuid);
    if (it == _guidToNameId.end())
    {
        TC_LOG_DEBUG("module.playerbot.names", 
            "Character {} has no allocated name", characterGuid);
        return;
    }
    
    uint32 nameId = it->second;
    
    // Find the name entry
    auto nameIt = _names.find(nameId);
    if (nameIt != _names.end())
    {
        NameEntry& entry = nameIt->second;
        std::string releasedName = entry.name;
        
        // Mark as available
        entry.used = false;
        entry.usedByGuid = 0;
        
        // Add back to available names
    if (entry.gender == 0)
            _availableMaleNames.insert(nameId);
        else if (entry.gender == 1)
            _availableFemaleNames.insert(nameId);
        
        TC_LOG_INFO("module.playerbot.names", 
            "Released name '{}' (ID: {}) from character {}", 
            releasedName, nameId, characterGuid);
    }
    
    // Remove from mappings
    _guidToNameId.erase(characterGuid);
    _nameIdToGuid.erase(nameId);

    // Persist to database - DELETE FROM playerbots_names_used
    std::ostringstream ss;
    ss << "DELETE FROM playerbots_names_used WHERE name_id = " << nameId;

    if (!sPlayerbotDatabase->Execute(ss.str()))
    {
        TC_LOG_ERROR("module.playerbot.names",
            "Failed to remove name allocation from database: name_id={}", nameId);
    }
}

void BotNameMgr::ReleaseName(std::string const& name)
{
    std::lock_guard lock(_mutex);
    
    auto it = _nameToId.find(name);
    if (it == _nameToId.end())
    {
        TC_LOG_WARN("module.playerbot.names", 
            "Name '{}' not found in name pool", name);
        return;
    }
    
    uint32 nameId = it->second;
    auto guidIt = _nameIdToGuid.find(nameId);
    if (guidIt != _nameIdToGuid.end())
    {
        ReleaseName(guidIt->second);
    }
}

bool BotNameMgr::IsNameAvailable(std::string const& name) const
{
    std::lock_guard lock(_mutex);

    auto it = _nameToId.find(name);
    if (it == _nameToId.end())
        return false; // Name not in pool

    auto nameIt = _names.find(it->second);
    if (nameIt == _names.end())
        return false;

    return !nameIt->second.used;
}

bool BotNameMgr::IsNameInUseAnywhere(std::string const& name) const
{
    // Check 1: Is name in use in our internal pool?
    {
        std::lock_guard lock(_mutex);
        auto it = _nameToId.find(name);
        if (it != _nameToId.end())
        {
            auto nameIt = _names.find(it->second);
            if (nameIt != _names.end() && nameIt->second.used)
                return true;
        }
    }

    // Check 2: Is name in use in the characters table?
    if (sCharacterCache->GetCharacterCacheByName(name))
        return true;

    return false;
}

std::string BotNameMgr::GenerateUniqueName(uint8 gender, uint32 maxRetries) const
{
    // Fantasy name components
    static const std::vector<std::string> malePrefixes = {
        "Thar", "Grim", "Kael", "Vor", "Zan", "Drak", "Thor", "Gor", "Bael", "Mor",
        "Kar", "Vex", "Jor", "Ren", "Lok", "Ash", "Zul", "Kor", "Mal", "Skar"
    };
    static const std::vector<std::string> femalePrefixes = {
        "Aela", "Luna", "Sera", "Lyra", "Nova", "Mira", "Zara", "Kira", "Vela", "Nyla",
        "Aria", "Eris", "Thea", "Iris", "Vera", "Cora", "Syla", "Nera", "Faye", "Myra"
    };
    static const std::vector<std::string> suffixes = {
        "ion", "ius", "an", "or", "us", "ax", "en", "ar", "on", "is",
        "oth", "ak", "ir", "ul", "os", "ek", "im", "as", "ur", "ok"
    };
    static const std::vector<std::string> uniqueSuffixes = {
        "a", "o", "i", "e", "u", "y", "", "", "", ""
    };

    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<std::string> const& prefixes = (gender == 0) ? malePrefixes : femalePrefixes;
    std::uniform_int_distribution<> prefixDist(0, prefixes.size() - 1);
    std::uniform_int_distribution<> suffixDist(0, suffixes.size() - 1);
    std::uniform_int_distribution<> uniqueDist(0, uniqueSuffixes.size() - 1);

    for (uint32 attempt = 0; attempt < maxRetries; ++attempt)
    {
        std::string name;
        name.reserve(12);

        // Build name: Prefix + Suffix + UniqueSuffix
        name = prefixes[prefixDist(gen)];
        name += suffixes[suffixDist(gen)];

        // Only add unique suffix if name is short enough
        if (name.length() < 10)
            name += uniqueSuffixes[uniqueDist(gen)];

        // Ensure name isn't too long
        if (name.length() > 12)
            name = name.substr(0, 12);

        // Capitalize properly
        if (!name.empty())
        {
            name[0] = std::toupper(name[0]);
            for (size_t i = 1; i < name.length(); ++i)
                name[i] = std::tolower(name[i]);
        }

        // Check if this name is available
        if (!IsNameInUseAnywhere(name))
        {
            TC_LOG_DEBUG("module.playerbot.names",
                "Generated unique name '{}' after {} attempts", name, attempt + 1);
            return name;
        }
    }

    TC_LOG_ERROR("module.playerbot.names",
        "Failed to generate unique name after {} attempts", maxRetries);
    return "";
}

std::string BotNameMgr::GetCharacterName(uint32 characterGuid) const
{
    std::lock_guard lock(_mutex);
    
    auto it = _guidToNameId.find(characterGuid);
    if (it == _guidToNameId.end())
        return "";
    
    auto nameIt = _names.find(it->second);
    if (nameIt == _names.end())
        return "";
    
    return nameIt->second.name;
}

uint32 BotNameMgr::GetAvailableNameCount(uint8 gender) const
{
    std::lock_guard lock(_mutex);
    
    if (gender == 0)
        return _availableMaleNames.size();
    else if (gender == 1)
        return _availableFemaleNames.size();
    else
        return _availableMaleNames.size() + _availableFemaleNames.size();
}

uint32 BotNameMgr::GetTotalNameCount() const
{
    std::lock_guard lock(_mutex);
    return _names.size();
}

uint32 BotNameMgr::GetUsedNameCount() const
{
    std::lock_guard lock(_mutex);
    return _guidToNameId.size();
}

void BotNameMgr::ReloadNames()
{
    TC_LOG_INFO("module.playerbot.names", "Reloading name pool from database");

    std::lock_guard lock(_mutex);

    // Clear ALL current data including mappings
    _names.clear();
    _availableMaleNames.clear();
    _availableFemaleNames.clear();
    _nameToId.clear();
    _guidToNameId.clear();
    _nameIdToGuid.clear();

    // Reload from database
    LoadNamesFromDatabase();
    LoadUsedNames();

    // Sync with actual characters to catch any orphaned names
    SyncWithCharactersTable();
}

void BotNameMgr::LoadNamesFromDatabase()
{
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT name_id, name, gender FROM playerbots_names");

    if (!result)
    {
        TC_LOG_ERROR("module.playerbot.names",
            "Failed to load names from playerbots_names table - table may not exist or be empty! "
            "Bot character creation will fail without names.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        NameEntry entry;
        entry.nameId = fields[0].GetUInt32();
        entry.name = fields[1].GetString();
        entry.gender = fields[2].GetUInt8();
        entry.used = false;
        entry.usedByGuid = 0;

        _names[entry.nameId] = entry;
        _nameToId[entry.name] = entry.nameId;

        // Add to available names (will be adjusted when loading used names)
        if (entry.gender == 0)
            _availableMaleNames.insert(entry.nameId);
        else if (entry.gender == 1)
            _availableFemaleNames.insert(entry.nameId);

        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("module.playerbot.names",
        "Loaded {} names from database ({} male, {} female)",
        count, _availableMaleNames.size(), _availableFemaleNames.size());
}

void BotNameMgr::LoadUsedNames()
{
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT name_id, character_guid FROM playerbots_names_used");

    if (!result)
    {
        TC_LOG_DEBUG("module.playerbot.names", "No used names found");
        return;
    }
    
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 nameId = fields[0].GetUInt32();
        uint32 characterGuid = fields[1].GetUInt32();
        
        auto it = _names.find(nameId);
        if (it == _names.end())
        {
            TC_LOG_WARN("module.playerbot.names", 
                "Used name ID {} not found in name pool", nameId);
            continue;
        }
        
        NameEntry& entry = it->second;
        entry.used = true;
        entry.usedByGuid = characterGuid;
        
        // Remove from available names
    if (entry.gender == 0)
            _availableMaleNames.erase(nameId);
        else if (entry.gender == 1)
            _availableFemaleNames.erase(nameId);
        
        // Update mappings
        _guidToNameId[characterGuid] = nameId;
        _nameIdToGuid[nameId] = characterGuid;
        
        ++count;
        
    } while (result->NextRow());
    
    TC_LOG_INFO("module.playerbot.names",
        "Loaded {} used names", count);
}

void BotNameMgr::SyncWithCharactersTable()
{
    // Query ALL character names from the characters table
    // This catches names that exist but weren't tracked in playerbots_names_used
    // (e.g., from crashed sessions, manual testing, or orphaned records)
    QueryResult result = CharacterDatabase.Query(
        "SELECT guid, name FROM characters");

    if (!result)
    {
        TC_LOG_DEBUG("module.playerbot.names", "No characters found in database");
        return;
    }

    uint32 syncCount = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 charGuid = fields[0].GetUInt32();
        std::string charName = fields[1].GetString();

        // Check if this name is in our name pool
        auto nameIt = _nameToId.find(charName);
        if (nameIt == _nameToId.end())
            continue;  // Not one of our managed names

        uint32 nameId = nameIt->second;

        // Check if this name is already marked as used
        auto entryIt = _names.find(nameId);
        if (entryIt == _names.end())
            continue;

        NameEntry& entry = entryIt->second;

        // If already marked as used by the same character, skip
        if (entry.used && entry.usedByGuid == charGuid)
            continue;

        // If already marked as used by a DIFFERENT character, we have a conflict
        // The actual character in the database takes precedence
        if (entry.used && entry.usedByGuid != charGuid)
        {
            TC_LOG_WARN("module.playerbot.names",
                "Name '{}' (ID: {}) was tracked to guid {} but actually belongs to guid {} - fixing",
                charName, nameId, entry.usedByGuid, charGuid);

            // Remove old mapping
            _guidToNameId.erase(entry.usedByGuid);
        }

        // Mark as used by the actual character
        entry.used = true;
        entry.usedByGuid = charGuid;

        // Remove from available names
        if (entry.gender == 0)
            _availableMaleNames.erase(nameId);
        else if (entry.gender == 1)
            _availableFemaleNames.erase(nameId);

        // Update mappings
        _guidToNameId[charGuid] = nameId;
        _nameIdToGuid[nameId] = charGuid;

        ++syncCount;

        TC_LOG_DEBUG("module.playerbot.names",
            "Synced name '{}' (ID: {}) to existing character {}",
            charName, nameId, charGuid);

    } while (result->NextRow());

    if (syncCount > 0)
    {
        TC_LOG_INFO("module.playerbot.names",
            "Synced {} names with existing characters table (available: {} male, {} female)",
            syncCount, _availableMaleNames.size(), _availableFemaleNames.size());
    }
}
