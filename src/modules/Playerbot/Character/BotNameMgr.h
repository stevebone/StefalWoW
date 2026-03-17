/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef BOT_NAME_MGR_H
#define BOT_NAME_MGR_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include <string>
#include <mutex>
#include <unordered_set>
#include <unordered_map>

/**
 * @brief Bot Name Manager
 *
 * Manages allocation and tracking of bot character names.
 */
class TC_GAME_API BotNameMgr final
{
public:
    BotNameMgr(BotNameMgr const&) = delete;
    BotNameMgr& operator=(BotNameMgr const&) = delete;

    static BotNameMgr* instance();

    // Core lifecycle
    bool Initialize();
    void Shutdown();

    // Name allocation - returns empty string if no name available
    std::string AllocateName(uint8 gender, uint32 characterGuid);

    // Name release when character is deleted
    void ReleaseName(uint32 characterGuid);
    void ReleaseName(std::string const& name);

    // Check if name is available in the name pool
    bool IsNameAvailable(std::string const& name) const;

    /**
     * @brief Check if a name is in use anywhere (pool + characters table)
     * @param name The name to check
     * @return true if name is already in use
     *
     * Checks both:
     * - The internal name pool (playerbots_names_used)
     * - The characters cache (existing characters in database)
     */
    bool IsNameInUseAnywhere(std::string const& name) const;

    /**
     * @brief Generate a unique fantasy name that isn't in use
     * @param gender 0=male, 1=female
     * @param maxRetries Maximum attempts to generate unique name (default 100)
     * @return Unique name, or empty string if all retries exhausted
     *
     * Generates random fantasy-style names and verifies they're not in use
     * in the characters table before returning.
     */
    std::string GenerateUniqueName(uint8 gender, uint32 maxRetries = 100) const;

    // Get name for existing character
    std::string GetCharacterName(uint32 characterGuid) const;

    // Statistics
    uint32 GetAvailableNameCount(uint8 gender) const;
    uint32 GetTotalNameCount() const;
    uint32 GetUsedNameCount() const;

    // Reload names from database
    void ReloadNames();
    
private:
    BotNameMgr() = default;
    ~BotNameMgr() = default;

    void LoadNamesFromDatabase();
    void LoadUsedNames();
    void SyncWithCharactersTable();  // Cross-reference names with existing characters
    
    struct NameEntry
    {
        uint32 nameId;
        std::string name;
        uint8 gender;
        bool used;
        uint32 usedByGuid;
    };
    
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _mutex;
    
    // All names indexed by ID
    std::unordered_map<uint32, NameEntry> _names;
    
    // Available names by gender (name_id sets)
    std::unordered_set<uint32> _availableMaleNames;
    std::unordered_set<uint32> _availableFemaleNames;
    
    // Used names mapping
    std::unordered_map<std::string, uint32> _nameToId;        // name -> name_id
    std::unordered_map<uint32, uint32> _guidToNameId;         // character_guid -> name_id
    std::unordered_map<uint32, uint32> _nameIdToGuid;         // name_id -> character_guid

    // Configuration (loaded from playerbots.conf)
    bool _useRandomNames{true};       // Playerbot.Names.UseRandomNames
    uint32 _minLength{4};             // Playerbot.Names.MinLength
    uint32 _maxLength{12};            // Playerbot.Names.MaxLength
    bool _useRaceTheme{true};         // Playerbot.Names.UseRaceTheme
};

#define sBotNameMgr BotNameMgr::instance()

#endif // BOT_NAME_MGR_H
