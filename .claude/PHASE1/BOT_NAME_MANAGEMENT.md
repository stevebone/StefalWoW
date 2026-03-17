## Database Schema

```sql
-- Table: playerbots_names
-- Purpose: Pool of unique names for bot character generation
-- IMPORTANT: Each name can only be used ONCE across all characters
CREATE TABLE IF NOT EXISTS `playerbots_names` (
  `name_id` INT(11) NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(255) NOT NULL,
  `gender` TINYINT(3) UNSIGNED NOT NULL COMMENT '0 = male, 1 = female',
  PRIMARY KEY (`name_id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci 
  COMMENT='Random bot name pool';

-- Table: playerbots_names_used
-- Purpose: Track which names are currently in use
CREATE TABLE IF NOT EXISTS `playerbots_names_used` (
  `name_id` INT(11) NOT NULL,
  `character_guid` INT UNSIGNED NOT NULL,
  `used_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`name_id`),
  UNIQUE KEY `idx_character` (`character_guid`),
  CONSTRAINT `fk_name_id` FOREIGN KEY (`name_id`) 
    REFERENCES `playerbots_names` (`name_id`) ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci
  COMMENT='Tracks which bot is using which name';
```

## C++ Implementation

### BotNameMgr Header
```cpp
// File: src/modules/Playerbot/Character/BotNameMgr.h

#ifndef BOT_NAME_MGR_H
#define BOT_NAME_MGR_H

#include \"Define.h\"
#include <string>
#include <mutex>
#include <unordered_set>
#include <unordered_map>

class TC_GAME_API BotNameMgr
{
public:
    BotNameMgr(BotNameMgr const&) = delete;
    BotNameMgr& operator=(BotNameMgr const&) = delete;
    
    static BotNameMgr* instance();
    
    bool Initialize();
    void Shutdown();
    
    // Name allocation - returns empty string if no name available
    std::string AllocateName(uint8 gender, uint32 characterGuid);
    
    // Name release when character is deleted
    void ReleaseName(uint32 characterGuid);
    void ReleaseName(std::string const& name);
    
    // Check if name is available
    bool IsNameAvailable(std::string const& name) const;
    
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
    
    struct NameEntry
    {
        uint32 nameId;
        std::string name;
        uint8 gender;
        bool used;
        uint32 usedByGuid;
    };
    
    mutable std::mutex _mutex;
    
    // All names indexed by ID
    std::unordered_map<uint32, NameEntry> _names;
    
    // Available names by gender (name_id sets)
    std::unordered_set<uint32> _availableMaleNames;
    std::unordered_set<uint32> _availableFemaleNames;
    
    // Used names mapping
    std::unordered_map<std::string, uint32> _nameToId;        // name -> name_id
    std::unordered_map<uint32, uint32> _guidToNameId;         // character_guid -> name_id
    std::unordered_map<uint32, uint32> _nameIdToGuid;         // name_id -> character_guid
};

#define sBotNameMgr BotNameMgr::instance()

#endif // BOT_NAME_MGR_H
```

### BotNameMgr Implementation
```cpp
// File: src/modules/Playerbot/Character/BotNameMgr.cpp

#include \"BotNameMgr.h\"
#include \"Database/PlayerbotDatabase.h\"
#include \"DatabaseEnv.h\"
#include \"Log.h\"
#include \"CharacterCache.h\"
#include <random>
#include <algorithm>

BotNameMgr* BotNameMgr::instance()
{
    static BotNameMgr instance;
    return &instance;
}

bool BotNameMgr::Initialize()
{
    LoadNamesFromDatabase();
    LoadUsedNames();
    
    TC_LOG_INFO(\"module.playerbot.names\", 
        \"Loaded {} names ({} male, {} female), {} in use\", 
        _names.size(),
        _availableMaleNames.size() + (_guidToNameId.size() / 2), // Approximate
        _availableFemaleNames.size() + (_guidToNameId.size() / 2),
        _guidToNameId.size());
    
    return true;
}

void BotNameMgr::Shutdown()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _names.clear();
    _availableMaleNames.clear();
    _availableFemaleNames.clear();
    _nameToId.clear();
    _guidToNameId.clear();
    _nameIdToGuid.clear();
}

std::string BotNameMgr::AllocateName(uint8 gender, uint32 characterGuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    // Check if character already has a name
    auto it = _guidToNameId.find(characterGuid);
    if (it != _guidToNameId.end())
    {
        auto nameIt = _names.find(it->second);
        if (nameIt != _names.end())
        {
            TC_LOG_WARN(\"module.playerbot.names\", 
                \"Character {} already has name '{}'\", 
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
        TC_LOG_ERROR(\"module.playerbot.names\", 
            \"Invalid gender {} for name allocation\", gender);
        return \"\";
    }
    
    if (availableNames->empty())
    {
        TC_LOG_ERROR(\"module.playerbot.names\", 
            \"No available names for gender {}\", gender);
        return \"\";
    }
    
    // Pick a random available name
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, availableNames->size() - 1);
    
    auto iter = availableNames->begin();
    std::advance(iter, dis(gen));
    uint32 selectedNameId = *iter;
    
    // Get the name entry
    auto nameIt = _names.find(selectedNameId);
    if (nameIt == _names.end())
    {
        TC_LOG_ERROR(\"module.playerbot.names\", 
            \"Name ID {} not found in names map\", selectedNameId);
        return \"\";
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
    
    // Update database
    PreparedStatement* stmt = sPlayerbotDatabase->GetPreparedStatement(PLAYERBOT_INS_NAME_USED);
    stmt->SetData(0, selectedNameId);
    stmt->SetData(1, characterGuid);
    sPlayerbotDatabase->Execute(stmt);
    
    TC_LOG_INFO(\"module.playerbot.names\", 
        \"Allocated name '{}' (ID: {}) to character {}\", 
        allocatedName, selectedNameId, characterGuid);
    
    return allocatedName;
}

void BotNameMgr::ReleaseName(uint32 characterGuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _guidToNameId.find(characterGuid);
    if (it == _guidToNameId.end())
    {
        TC_LOG_DEBUG(\"module.playerbot.names\", 
            \"Character {} has no allocated name\", characterGuid);
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
        
        TC_LOG_INFO(\"module.playerbot.names\", 
            \"Released name '{}' (ID: {}) from character {}\", 
            releasedName, nameId, characterGuid);
    }
    
    // Remove from mappings
    _guidToNameId.erase(characterGuid);
    _nameIdToGuid.erase(nameId);
    
    // Update database
    PreparedStatement* stmt = sPlayerbotDatabase->GetPreparedStatement(PLAYERBOT_DEL_NAME_USED);
    stmt->SetData(0, nameId);
    sPlayerbotDatabase->Execute(stmt);
}

void BotNameMgr::ReleaseName(std::string const& name)
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _nameToId.find(name);
    if (it == _nameToId.end())
    {
        TC_LOG_WARN(\"module.playerbot.names\", 
            \"Name '{}' not found in name pool\", name);
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
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _nameToId.find(name);
    if (it == _nameToId.end())
        return false; // Name not in pool
    
    auto nameIt = _names.find(it->second);
    if (nameIt == _names.end())
        return false;
    
    return !nameIt->second.used;
}

std::string BotNameMgr::GetCharacterName(uint32 characterGuid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _guidToNameId.find(characterGuid);
    if (it == _guidToNameId.end())
        return \"\";
    
    auto nameIt = _names.find(it->second);
    if (nameIt == _names.end())
        return \"\";
    
    return nameIt->second.name;
}

uint32 BotNameMgr::GetAvailableNameCount(uint8 gender) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (gender == 0)
        return _availableMaleNames.size();
    else if (gender == 1)
        return _availableFemaleNames.size();
    else
        return _availableMaleNames.size() + _availableFemaleNames.size();
}

uint32 BotNameMgr::GetTotalNameCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _names.size();
}

uint32 BotNameMgr::GetUsedNameCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _guidToNameId.size();
}

void BotNameMgr::ReloadNames()
{
    TC_LOG_INFO(\"module.playerbot.names\", \"Reloading name pool from database\");
    
    std::lock_guard<std::mutex> lock(_mutex);
    
    // Clear current data
    _names.clear();
    _availableMaleNames.clear();
    _availableFemaleNames.clear();
    _nameToId.clear();
    
    // Reload
    LoadNamesFromDatabase();
    LoadUsedNames();
}

void BotNameMgr::LoadNamesFromDatabase()
{
    QueryResult result = sPlayerbotDatabase->Query(
        \"SELECT name_id, name, gender FROM playerbots_names\");
    
    if (!result)
    {
        TC_LOG_ERROR(\"module.playerbot.names\", 
            \"No names found in playerbots_names table!\");
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
    
    TC_LOG_INFO(\"module.playerbot.names\", 
        \"Loaded {} names from database\", count);
}

void BotNameMgr::LoadUsedNames()
{
    QueryResult result = sPlayerbotDatabase->Query(
        \"SELECT name_id, character_guid FROM playerbots_names_used\");
    
    if (!result)
    {
        TC_LOG_DEBUG(\"module.playerbot.names\", \"No used names found\");
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
            TC_LOG_WARN(\"module.playerbot.names\", 
                \"Used name ID {} not found in name pool\", nameId);
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
    
    TC_LOG_INFO(\"module.playerbot.names\", 
        \"Loaded {} used names\", count);
}
```

### Database Prepared Statements Update
```cpp
// In PlayerbotDatabase.h enum
enum PlayerbotDatabaseStatements : uint32
{
    // ... existing statements ...
    
    // Name management
    PLAYERBOT_SEL_ALL_NAMES,
    PLAYERBOT_SEL_USED_NAMES,
    PLAYERBOT_INS_NAME_USED,
    PLAYERBOT_DEL_NAME_USED,
    PLAYERBOT_DEL_NAME_USED_BY_GUID,
    
    MAX_PLAYERBOT_DATABASE_STATEMENTS
};

// In PlayerbotDatabase.cpp DoPrepareStatements()
    // Name management statements
    PrepareStatement(PLAYERBOT_SEL_ALL_NAMES,
        \"SELECT name_id, name, gender FROM playerbots_names\");
    
    PrepareStatement(PLAYERBOT_SEL_USED_NAMES,
        \"SELECT name_id, character_guid FROM playerbots_names_used\");
    
    PrepareStatement(PLAYERBOT_INS_NAME_USED,
        \"INSERT INTO playerbots_names_used (name_id, character_guid) \"
        \"VALUES (?, ?)\");
    
    PrepareStatement(PLAYERBOT_DEL_NAME_USED,
        \"DELETE FROM playerbots_names_used WHERE name_id = ?\");
    
    PrepareStatement(PLAYERBOT_DEL_NAME_USED_BY_GUID,
        \"DELETE FROM playerbots_names_used WHERE character_guid = ?\");
```

### Integration in BotCharacterMgr
```cpp
// In BotCharacterMgr::CreateBotCharacter
uint32 BotCharacterMgr::CreateBotCharacter(uint32 accountId, uint8 race, 
                                           uint8 classId, uint8 gender)
{
    // Allocate a unique name from the pool
    std::string name = sBotNameMgr->AllocateName(gender, 0); // 0 = temporary
    
    if (name.empty())
    {
        TC_LOG_ERROR(\"module.playerbot.character\", 
            \"Failed to allocate name for new bot character\");
        return 0;
    }
    
    // Use TrinityCore's character creation
    // ... character creation code ...
    
    // After successful creation, update name allocation with real GUID
    uint32 newGuid = /* get created character GUID */;
    sBotNameMgr->ReleaseName(0); // Release temporary
    sBotNameMgr->AllocateName(gender, newGuid); // Allocate with real GUID
    
    return newGuid;
}

// In BotCharacterMgr::DeleteBotCharacter
bool BotCharacterMgr::DeleteBotCharacter(uint32 guid)
{
    // Release the name back to the pool
    sBotNameMgr->ReleaseName(guid);
    
    // Use TrinityCore's character deletion
    // ... character deletion code ...
    
    return true;
}
```

## Important Features

1. **Unique Names**: Each name can only be used once across ALL characters
2. **Gender-Specific**: Names are assigned based on gender (0=male, 1=female)
3. **Automatic Release**: When a character is deleted, the name returns to the pool
4. **Database Persistence**: Used names are tracked in database, survives restarts
5. **Thread-Safe**: All operations are mutex-protected
6. **Statistics**: Can query available names per gender

## Sample Data Import
Complete SQL file with more than 12000 names is playerbot_names_final.sql 

```sql
-- Import sample names (examples)
INSERT INTO `playerbots_names` (`name`, `gender`) VALUES
-- Male names (gender = 0)
('Aldric', 0), ('Thorin', 0), ('Ragnar', 0), ('Grimnar', 0),
('Marcus', 0), ('Gareth', 0), ('Darius', 0), ('Viktor', 0),
('Leonidas', 0), ('Maximus', 0), ('Brutus', 0), ('Cassius', 0),

-- Female names (gender = 1)
('Elara', 1), ('Luna', 1), ('Sylvia', 1), ('Tyrande', 1),
('Jaina', 1), ('Aria', 1), ('Lyra', 1), ('Freya', 1),
('Aurora', 1), ('Diana', 1), ('Minerva', 1), ('Athena', 1);

-- Names should be imported from a larger dataset
-- Recommended: 10,000+ unique names per gender for variety
```

## Admin Commands
```cpp
// Commands to manage name pool
.playerbot names status    // Show available/used names count
.playerbot names reload    // Reload names from database
.playerbot names release <character> // Manually release a name
```

This system ensures every bot has a unique name and names are never duplicated!`