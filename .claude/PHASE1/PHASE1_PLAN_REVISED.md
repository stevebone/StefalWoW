# PHASE 1: Foundation Infrastructure - Detailed Action Plan (REVISED)

## IMPORTANT: Using TrinityCore's Account and Character Systems
**Bot accounts are created in `auth.account` using TrinityCore's AccountMgr**  
**Bot characters are created in `characters.characters` using TrinityCore's Player class**  
**Only bot-specific metadata is stored in the playerbot database**

## Phase Overview
**Duration:** 6 Weeks (Week 1-6)
**Goal:** Establish the core foundation for the Playerbot module with zero impact on TrinityCore functionality

## Success Criteria
- [ ] TrinityCore compiles and runs identically with BUILD_PLAYERBOT=OFF
- [ ] Module loads when BUILD_PLAYERBOT=ON and Playerbot.Enable=1
- [ ] Bot accounts created using AccountMgr in auth database
- [ ] Bot characters created using Player class in characters database
- [ ] Configuration system works with hot-reload
- [ ] Database connection established to separate playerbot database (metadata only)
- [ ] Virtual session management operational
- [ ] All unit tests pass
- [ ] Zero memory leaks detected
- [ ] Performance impact <1% when disabled

---

## WEEK 1-2: Build System & Configuration

### Day 1: Project Setup
**Time: 4 hours**

#### Task 1.1: Create Directory Structure
```bash
# Execute from C:\TrinityBots\TrinityCore
mkdir src\modules
mkdir src\modules\Playerbot
mkdir src\modules\Playerbot\Config
mkdir src\modules\Playerbot\Database
mkdir src\modules\Playerbot\Session
mkdir src\modules\Playerbot\Account
mkdir src\modules\Playerbot\Character
mkdir src\modules\Playerbot\AI
mkdir src\modules\Playerbot\Tests
mkdir src\modules\Playerbot\conf
mkdir src\modules\Playerbot\sql
```

#### Task 1.2: Create Base CMakeLists.txt
**File:** `src/modules/Playerbot/CMakeLists.txt`
```cmake
# Copyright 2024 TrinityCore
# Playerbot Module CMake Configuration

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES)

add_library(playerbot STATIC
  ${PRIVATE_SOURCES})

target_include_directories(playerbot
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/server/game
    ${CMAKE_SOURCE_DIR}/src/server/game/Accounts
    ${CMAKE_SOURCE_DIR}/src/server/game/Entities/Player
    ${CMAKE_SOURCE_DIR}/src/server/game/World
    ${CMAKE_SOURCE_DIR}/src/server/database/Database
    ${CMAKE_SOURCE_DIR}/src/common/Configuration)

target_link_libraries(playerbot
  PUBLIC
    game
    common
    database)

# Install configuration file
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/conf/playerbots.conf.dist"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/etc")

# Add to worldserver only if enabled
if(BUILD_PLAYERBOT)
  target_compile_definitions(worldserver PRIVATE -DPLAYERBOT_ENABLED)
endif()
```

#### Task 1.3: Update Root CMakeLists.txt
**File:** `CMakeLists.txt` (root)
**Action:** Add after line ~200 (after other options)
```cmake
option(BUILD_PLAYERBOT "Build Playerbot module" OFF)

if(BUILD_PLAYERBOT)
  message(STATUS "Playerbot module ENABLED")
  add_subdirectory(src/modules/Playerbot)
else()
  message(STATUS "Playerbot module DISABLED")
endif()
```

### Day 2: Module Entry Point
**Time: 6 hours**

#### Task 2.1: Create PlayerbotModule Header
**File:** `src/modules/Playerbot/PlayerbotModule.h`
```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_MODULE_H
#define PLAYERBOT_MODULE_H

#include "Define.h"
#include <atomic>
#include <memory>

class PlayerbotConfig;
class BotAccountMgr;
class BotCharacterMgr;

class TC_GAME_API PlayerbotModule
{
public:
    PlayerbotModule(PlayerbotModule const&) = delete;
    PlayerbotModule& operator=(PlayerbotModule const&) = delete;
    
    static PlayerbotModule* instance();
    
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled.load(); }
    
    // Runtime control
    void Enable();
    void Disable();
    
    // Module version for compatibility checking
    static constexpr uint32 GetModuleVersion() { return 1; }
    
    // Managers
    BotAccountMgr* GetAccountMgr() const { return _accountMgr.get(); }
    BotCharacterMgr* GetCharacterMgr() const { return _characterMgr.get(); }
    
private:
    PlayerbotModule() = default;
    ~PlayerbotModule() = default;
    
    void RegisterHooks();
    void UnregisterHooks();
    bool ValidateEnvironment();
    
    std::atomic<bool> _enabled{false};
    std::atomic<bool> _initialized{false};
    
    std::unique_ptr<BotAccountMgr> _accountMgr;
    std::unique_ptr<BotCharacterMgr> _characterMgr;
};

#define sPlayerbotModule PlayerbotModule::instance()

#endif // PLAYERBOT_MODULE_H
```

#### Task 2.2: Create BotAccountMgr (Using TrinityCore AccountMgr)
**File:** `src/modules/Playerbot/Account/BotAccountMgr.h`
```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_ACCOUNT_MGR_H
#define BOT_ACCOUNT_MGR_H

#include "Define.h"
#include <string>
#include <vector>
#include <mutex>
#include <unordered_set>

class TC_GAME_API BotAccountMgr
{
public:
    BotAccountMgr();
    ~BotAccountMgr();
    
    bool Initialize();
    void Shutdown();
    
    // Account management using TrinityCore's AccountMgr
    uint32 CreateBotAccount(std::string const& username);
    bool DeleteBotAccount(uint32 accountId);
    bool IsBotAccount(uint32 accountId) const;
    
    // Batch operations
    std::vector<uint32> CreateBotAccounts(uint32 count);
    void MarkAccountAsBot(uint32 accountId);
    
    // Statistics
    uint32 GetBotAccountCount() const;
    std::vector<uint32> GetAllBotAccounts() const;
    
    // Account naming
    std::string GenerateBotAccountName(uint32 index);
    
private:
    void LoadBotAccounts();
    void SaveBotAccountInfo(uint32 accountId);
    
    mutable std::mutex _mutex;
    std::unordered_set<uint32> _botAccounts;
    std::string _accountPrefix;
    uint32 _nextAccountIndex;
};

#endif // BOT_ACCOUNT_MGR_H
```

**File:** `src/modules/Playerbot/Account/BotAccountMgr.cpp`
```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotAccountMgr.h"
#include "AccountMgr.h"
#include "Config/PlayerbotConfig.h"
#include "Database/PlayerbotDatabase.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include <fmt/format.h>

BotAccountMgr::BotAccountMgr() : 
    _accountPrefix("Bot"),
    _nextAccountIndex(1)
{
}

BotAccountMgr::~BotAccountMgr()
{
}

bool BotAccountMgr::Initialize()
{
    _accountPrefix = sPlayerbotConfig->GetString("Playerbot.BotAccountPrefix", "Bot");
    
    LoadBotAccounts();
    
    TC_LOG_INFO("module.playerbot.account", 
        "Bot Account Manager initialized with {} existing bot accounts", 
        _botAccounts.size());
    
    // Create initial accounts if configured
    if (sPlayerbotConfig->GetBool("Playerbot.CreateAccountsOnStartup", true))
    {
        uint32 minBots = sPlayerbotConfig->GetUInt("Playerbot.MinBots", 20);
        if (_botAccounts.size() < minBots)
        {
            uint32 toCreate = minBots - _botAccounts.size();
            TC_LOG_INFO("module.playerbot.account", 
                "Creating {} bot accounts to reach minimum of {}", 
                toCreate, minBots);
            
            CreateBotAccounts(toCreate);
        }
    }
    
    return true;
}

void BotAccountMgr::Shutdown()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _botAccounts.clear();
}

uint32 BotAccountMgr::CreateBotAccount(std::string const& username)
{
    // Use TrinityCore's AccountMgr to create account in auth.account
    std::string password = "botpass"; // Simple password for bots
    std::string email = fmt::format("{}@bot.local", username);
    
    AccountOpResult result = sAccountMgr->CreateAccount(username, password, email);
    
    if (result != AOR_OK)
    {
        TC_LOG_ERROR("module.playerbot.account", 
            "Failed to create bot account '{}': {}", username, uint32(result));
        return 0;
    }
    
    // Get the created account ID
    uint32 accountId = sAccountMgr->GetId(username);
    if (accountId == 0)
    {
        TC_LOG_ERROR("module.playerbot.account", 
            "Created account '{}' but couldn't retrieve ID", username);
        return 0;
    }
    
    // Mark as bot in our tracking
    MarkAccountAsBot(accountId);
    
    TC_LOG_INFO("module.playerbot.account", 
        "Created bot account '{}' with ID {}", username, accountId);
    
    return accountId;
}

bool BotAccountMgr::DeleteBotAccount(uint32 accountId)
{
    if (!IsBotAccount(accountId))
    {
        TC_LOG_WARN("module.playerbot.account", 
            "Attempted to delete non-bot account {}", accountId);
        return false;
    }
    
    // Use TrinityCore's AccountMgr to delete from auth.account
    AccountOpResult result = sAccountMgr->DeleteAccount(accountId);
    
    if (result != AOR_OK)
    {
        TC_LOG_ERROR("module.playerbot.account", 
            "Failed to delete bot account {}: {}", accountId, uint32(result));
        return false;
    }
    
    // Remove from our tracking
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _botAccounts.erase(accountId);
    }
    
    // Remove from playerbot database
    PreparedStatement* stmt = sPlayerbotDatabase->GetPreparedStatement(PLAYERBOT_DEL_ACCOUNT_INFO);
    stmt->SetData(0, accountId);
    sPlayerbotDatabase->Execute(stmt);
    
    return true;
}

bool BotAccountMgr::IsBotAccount(uint32 accountId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _botAccounts.find(accountId) != _botAccounts.end();
}

std::vector<uint32> BotAccountMgr::CreateBotAccounts(uint32 count)
{
    std::vector<uint32> createdAccounts;
    createdAccounts.reserve(count);
    
    uint32 batchSize = sPlayerbotConfig->GetUInt("Playerbot.AccountsPerBatch", 50);
    
    for (uint32 i = 0; i < count; ++i)
    {
        std::string username = GenerateBotAccountName(_nextAccountIndex++);
        uint32 accountId = CreateBotAccount(username);
        
        if (accountId != 0)
        {
            createdAccounts.push_back(accountId);
        }
        
        // Batch processing to avoid blocking
        if ((i + 1) % batchSize == 0)
        {
            TC_LOG_INFO("module.playerbot.account", 
                "Created {}/{} bot accounts", i + 1, count);
        }
    }
    
    TC_LOG_INFO("module.playerbot.account", 
        "Successfully created {} bot accounts", createdAccounts.size());
    
    return createdAccounts;
}

void BotAccountMgr::MarkAccountAsBot(uint32 accountId)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _botAccounts.insert(accountId);
    }
    
    SaveBotAccountInfo(accountId);
}

uint32 BotAccountMgr::GetBotAccountCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return static_cast<uint32>(_botAccounts.size());
}

std::vector<uint32> BotAccountMgr::GetAllBotAccounts() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return std::vector<uint32>(_botAccounts.begin(), _botAccounts.end());
}

std::string BotAccountMgr::GenerateBotAccountName(uint32 index)
{
    return fmt::format("{}{:04d}", _accountPrefix, index);
}

void BotAccountMgr::LoadBotAccounts()
{
    // Load bot account IDs from playerbot database
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT account_id FROM playerbot_account_info WHERE is_bot = 1");
    
    if (!result)
    {
        TC_LOG_INFO("module.playerbot.account", "No existing bot accounts found");
        return;
    }
    
    std::lock_guard<std::mutex> lock(_mutex);
    do
    {
        Field* fields = result->Fetch();
        uint32 accountId = fields[0].GetUInt32();
        
        // Verify account still exists in auth database
        if (sAccountMgr->GetName(accountId).empty())
        {
            TC_LOG_WARN("module.playerbot.account", 
                "Bot account {} no longer exists in auth database", accountId);
            continue;
        }
        
        _botAccounts.insert(accountId);
        
    } while (result->NextRow());
    
    TC_LOG_INFO("module.playerbot.account", 
        "Loaded {} bot accounts", _botAccounts.size());
}

void BotAccountMgr::SaveBotAccountInfo(uint32 accountId)
{
    // Save bot account info to playerbot database
    PreparedStatement* stmt = sPlayerbotDatabase->GetPreparedStatement(PLAYERBOT_INS_ACCOUNT_INFO);
    stmt->SetData(0, accountId);
    stmt->SetData(1, 1); // is_bot = true
    stmt->SetData(2, "random"); // bot_type
    sPlayerbotDatabase->Execute(stmt);
}
```

#### Task 2.3: Create BotCharacterMgr (Using TrinityCore Player)
**File:** `src/modules/Playerbot/Character/BotCharacterMgr.h`
```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_CHARACTER_MGR_H
#define BOT_CHARACTER_MGR_H

#include "Define.h"
#include "SharedDefines.h"
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

struct BotCharacterInfo
{
    uint32 guid;
    uint32 accountId;
    std::string name;
    uint8 race;
    uint8 classId;
    uint8 level;
    std::string role; // tank, healer, dps
};

class TC_GAME_API BotCharacterMgr
{
public:
    BotCharacterMgr();
    ~BotCharacterMgr();
    
    bool Initialize();
    void Shutdown();
    
    // Character management using TrinityCore's character creation
    uint32 CreateBotCharacter(uint32 accountId, std::string const& name, 
                              uint8 race, uint8 classId, uint8 gender);
    bool DeleteBotCharacter(uint32 guid);
    bool IsBotCharacter(uint32 guid) const;
    
    // Character generation
    std::string GenerateRandomName(uint8 race, uint8 gender);
    void GenerateRandomAppearance(uint8 race, uint8 classId, uint8 gender,
                                  uint8& skin, uint8& face, uint8& hairStyle,
                                  uint8& hairColor, uint8& facialHair);
    
    // Bot character info
    BotCharacterInfo GetBotCharacterInfo(uint32 guid) const;
    std::vector<uint32> GetAccountCharacters(uint32 accountId) const;
    
    // Role management
    void SetCharacterRole(uint32 guid, std::string const& role);
    std::string DetermineRole(uint8 classId, uint32 specId = 0);
    
private:
    void LoadBotCharacters();
    void SaveBotCharacterInfo(uint32 guid, uint32 accountId, std::string const& role);
    
    mutable std::mutex _mutex;
    std::unordered_map<uint32, BotCharacterInfo> _botCharacters;
    std::unordered_map<uint32, std::vector<uint32>> _accountCharacters;
};

#endif // BOT_CHARACTER_MGR_H
```

### Day 3-4: Configuration System  
**Time: 8 hours**

#### Task 3.3: Create Configuration Template (UPDATED)
**File:** `src/modules/Playerbot/conf/playerbots.conf.dist`
```ini
###################################################################################################
# PLAYERBOT MODULE CONFIGURATION
#
# This file controls ALL playerbot functionality.
# The main worldserver.conf file remains unchanged.
#
# IMPORTANT: Bot accounts are created in the standard auth.account table
#            Bot characters are created in the standard characters.characters table
#            Only bot-specific metadata is stored in the playerbot database
#
# To enable playerbots:
# 1. Compile with -DBUILD_PLAYERBOT=1
# 2. Copy playerbots.conf.dist to playerbots.conf
# 3. Set Playerbot.Enable = 1 below
###################################################################################################

###################################################################################################
# CORE SETTINGS
###################################################################################################

# Playerbot.Enable
#     Description: Master switch for playerbot functionality
#     Important:   When disabled, the module has ZERO performance impact
#     Default:     0 (Disabled)
#     Values:      0 - Completely disabled (no memory/CPU usage)
#                  1 - Enabled
Playerbot.Enable = 0

# Playerbot.ConfigVersion
#     Description: Config file version for compatibility checking
#     Important:   Do not modify manually
#     Default:     1
Playerbot.ConfigVersion = 1

###################################################################################################
# DATABASE SETTINGS
# Note: This database only stores bot metadata. Accounts/characters use standard Trinity DBs
###################################################################################################

# Playerbot.Database.Host
#     Description: MySQL host for playerbot metadata database
#     Default:     "127.0.0.1"
Playerbot.Database.Host = "127.0.0.1"

# Playerbot.Database.Port
#     Description: MySQL port
#     Default:     3306
Playerbot.Database.Port = 3306

# Playerbot.Database.Username
#     Description: MySQL username for playerbot database
#     Default:     "trinity"
Playerbot.Database.Username = "trinity"

# Playerbot.Database.Password
#     Description: MySQL password for playerbot database
#     Default:     "trinity"
Playerbot.Database.Password = "trinity"

# Playerbot.Database.Name
#     Description: Database name for playerbot metadata tables
#     Important:   This is a SEPARATE database from auth/characters/world
#                  It only stores bot-specific information
#     Default:     "playerbot"
Playerbot.Database.Name = "playerbot"

# Playerbot.Database.ConnectionPool.Size
#     Description: Number of database connections in pool
#     Default:     10
#     Range:       5-50
Playerbot.Database.ConnectionPool.Size = 10

###################################################################################################
# BOT ACCOUNT SETTINGS
# Bot accounts are created in auth.account using TrinityCore's AccountMgr
###################################################################################################

# Playerbot.MaxBots
#     Description: Maximum number of concurrent bots
#     Important:   Higher values require more memory and CPU
#     Default:     100
#     Range:       1-500
Playerbot.MaxBots = 100

# Playerbot.MinBots
#     Description: Minimum number of bot accounts to maintain
#     Default:     20
#     Range:       0-100
Playerbot.MinBots = 20

# Playerbot.BotAccountPrefix
#     Description: Prefix for bot account names
#     Example:     "Bot" results in accounts like Bot0001, Bot0002, etc.
#     Important:   These accounts are created in auth.account table
#     Default:     "Bot"
Playerbot.BotAccountPrefix = "Bot"

# Playerbot.CreateAccountsOnStartup
#     Description: Auto-create bot accounts in auth database if they don't exist
#     Default:     1 (Enabled)
Playerbot.CreateAccountsOnStartup = 1

# Playerbot.AccountsPerBatch
#     Description: Number of accounts to create per batch (to avoid blocking)
#     Default:     50
#     Range:       10-100
Playerbot.AccountsPerBatch = 50

###################################################################################################
# BOT CHARACTER SETTINGS
# Bot characters are created in characters.characters using TrinityCore's character creation
###################################################################################################

# Playerbot.CharactersPerAccount
#     Description: Number of characters to create per bot account
#     Default:     1
#     Range:       1-10
Playerbot.CharactersPerAccount = 1

# Playerbot.RandomLevel.Min
#     Description: Minimum level for randomly created bot characters
#     Default:     1
Playerbot.RandomLevel.Min = 1

# Playerbot.RandomLevel.Max
#     Description: Maximum level for randomly created bot characters
#     Default:     60
Playerbot.RandomLevel.Max = 60

###################################################################################################
# PERFORMANCE SETTINGS
###################################################################################################

# Playerbot.Threading.WorkerThreads
#     Description: Number of worker threads for bot AI processing
#     Default:     0 (Auto-detect based on CPU cores)
#     Range:       0-32
Playerbot.Threading.WorkerThreads = 0

# Playerbot.AI.UpdateInterval
#     Description: Bot AI update interval in milliseconds
#     Important:   Lower values = more responsive bots but higher CPU usage
#     Default:     100
#     Range:       50-1000
Playerbot.AI.UpdateInterval = 100

# Playerbot.AI.MaxDecisionTime
#     Description: Maximum time for AI decision in milliseconds
#     Important:   If exceeded, bot will use fallback behavior
#     Default:     50
#     Range:       10-200
Playerbot.AI.MaxDecisionTime = 50

# Playerbot.Memory.MaxPerBot
#     Description: Maximum memory per bot in MB
#     Default:     10
#     Range:       5-50
Playerbot.Memory.MaxPerBot = 10

###################################################################################################
# More settings will be added as the module develops
###################################################################################################
```

### Day 6-7: Database Schema Design (UPDATED)
**Time: 8 hours**

#### Task 6.1: Create Initial Schema (REVISED - Metadata Only)
**File:** `src/modules/Playerbot/sql/playerbot_001_schema.sql`
```sql
-- Playerbot Module Database Schema v1
-- This creates a separate database for bot-specific metadata only
-- Bot accounts are stored in auth.account
-- Bot characters are stored in characters.characters

-- Create database if not exists
CREATE DATABASE IF NOT EXISTS `playerbot` 
  DEFAULT CHARACTER SET utf8mb4 
  COLLATE utf8mb4_unicode_ci;

USE `playerbot`;

-- Table: playerbot_account_info
-- Purpose: Track additional bot-specific account information
-- Links to auth.account table
CREATE TABLE IF NOT EXISTS `playerbot_account_info` (
  `account_id` INT UNSIGNED NOT NULL,
  `is_bot` TINYINT(1) DEFAULT 1,
  `bot_type` ENUM('random', 'guild', 'arena', 'dungeon', 'quest') DEFAULT 'random',
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `last_selected_char` INT UNSIGNED DEFAULT 0,
  `owner_account` INT UNSIGNED DEFAULT 0 COMMENT 'Human player who can control this bot',
  `settings` JSON DEFAULT NULL COMMENT 'Bot-specific settings',
  PRIMARY KEY (`account_id`),
  KEY `idx_bot_type` (`bot_type`),
  KEY `idx_owner` (`owner_account`),
  KEY `idx_is_bot` (`is_bot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci 
  COMMENT='Bot account metadata - links to auth.account';

-- Table: playerbot_character_info
-- Purpose: Track additional bot-specific character information
-- Links to characters.characters table
CREATE TABLE IF NOT EXISTS `playerbot_character_info` (
  `guid` INT UNSIGNED NOT NULL,
  `account_id` INT UNSIGNED NOT NULL,
  `is_bot` TINYINT(1) DEFAULT 1,
  `role` ENUM('tank', 'healer', 'dps', 'pvp') DEFAULT 'dps',
  `ai_strategy` VARCHAR(50) DEFAULT 'default',
  `follow_target` INT UNSIGNED DEFAULT 0 COMMENT 'Character GUID to follow',
  `group_role` ENUM('leader', 'member', 'none') DEFAULT 'none',
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `last_played` TIMESTAMP NULL DEFAULT NULL,
  `settings` JSON DEFAULT NULL COMMENT 'Character-specific AI settings',
  PRIMARY KEY (`guid`),
  KEY `idx_account` (`account_id`),
  KEY `idx_role` (`role`),
  KEY `idx_is_bot` (`is_bot`),
  KEY `idx_follow` (`follow_target`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci 
  COMMENT='Bot character metadata - links to characters.characters';

-- Table: playerbot_ai_state
-- Purpose: Store AI decision state and memory
CREATE TABLE IF NOT EXISTS `playerbot_ai_state` (
  `guid` INT UNSIGNED NOT NULL,
  `strategy` VARCHAR(50) NOT NULL DEFAULT 'default',
  `sub_strategy` VARCHAR(50) DEFAULT NULL,
  `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `state_data` JSON DEFAULT NULL COMMENT 'Current AI state',
  `memory_data` JSON DEFAULT NULL COMMENT 'AI memory/learning data',
  `combat_data` JSON DEFAULT NULL COMMENT 'Combat statistics',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci 
  COMMENT='Runtime AI state - cleared on restart';

-- Table: playerbot_sessions
-- Purpose: Track active bot sessions
CREATE TABLE IF NOT EXISTS `playerbot_sessions` (
  `session_id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `guid` INT UNSIGNED NOT NULL,
  `account_id` INT UNSIGNED NOT NULL,
  `login_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `logout_time` TIMESTAMP NULL DEFAULT NULL,
  `session_data` JSON DEFAULT NULL,
  PRIMARY KEY (`session_id`),
  KEY `idx_guid` (`guid`),
  KEY `idx_account` (`account_id`),
  KEY `idx_active` (`logout_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci 
  COMMENT='Bot session tracking';

-- Table: playerbot_config
-- Purpose: Store runtime configuration overrides
CREATE TABLE IF NOT EXISTS `playerbot_config` (
  `key` VARCHAR(100) NOT NULL,
  `value` TEXT NOT NULL,
  `description` TEXT,
  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci 
  COMMENT='Runtime config overrides';

-- Table: playerbot_statistics
-- Purpose: Performance and usage statistics
CREATE TABLE IF NOT EXISTS `playerbot_statistics` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `timestamp` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `metric_name` VARCHAR(100) NOT NULL,
  `metric_value` DOUBLE NOT NULL,
  `metric_data` JSON DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_timestamp` (`timestamp`),
  KEY `idx_metric` (`metric_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci 
  COMMENT='Performance metrics';

-- Table: playerbot_name_pool
-- Purpose: Pool of names for bot character generation
CREATE TABLE IF NOT EXISTS `playerbot_name_pool` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(50) NOT NULL,
  `race` TINYINT UNSIGNED DEFAULT 0 COMMENT '0 = any race',
  `gender` TINYINT UNSIGNED DEFAULT 2 COMMENT '0 = male, 1 = female, 2 = any',
  `used` TINYINT(1) DEFAULT 0,
  `used_by` INT UNSIGNED DEFAULT NULL COMMENT 'Character GUID using this name',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_name` (`name`),
  KEY `idx_available` (`used`, `race`, `gender`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci 
  COMMENT='Available names for bot generation';

-- Initial configuration values
INSERT INTO `playerbot_config` (`key`, `value`, `description`) VALUES
  ('schema_version', '1', 'Database schema version'),
  ('last_migration', NOW(), 'Last migration timestamp');

-- Sample names for bot generation
INSERT INTO `playerbot_name_pool` (`name`, `race`, `gender`) VALUES
  ('Aldric', 1, 0), ('Thorin', 3, 0), ('Elara', 4, 1),
  ('Grimnar', 6, 0), ('Luna', 4, 1), ('Baine', 6, 0),
  ('Sylvia', 1, 1), ('Ragnar', 2, 0), ('Tyrande', 4, 1),
  ('Garrosh', 2, 0), ('Jaina', 1, 1), ('Thrall', 2, 0);
```

#### Task 8.2: Update Database Prepared Statements
**File:** `src/modules/Playerbot/Database/PlayerbotDatabase.h` (Modified enum)
```cpp
enum PlayerbotDatabaseStatements : uint32
{
    // Account Info (metadata only - accounts are in auth.account)
    PLAYERBOT_SEL_ACCOUNT_INFO,
    PLAYERBOT_INS_ACCOUNT_INFO,
    PLAYERBOT_UPD_ACCOUNT_INFO,
    PLAYERBOT_DEL_ACCOUNT_INFO,
    
    // Character Info (metadata only - characters are in characters.characters)  
    PLAYERBOT_SEL_CHARACTER_INFO,
    PLAYERBOT_SEL_CHARACTERS_BY_ACCOUNT,
    PLAYERBOT_INS_CHARACTER_INFO,
    PLAYERBOT_UPD_CHARACTER_INFO,
    PLAYERBOT_DEL_CHARACTER_INFO,
    
    // AI State
    PLAYERBOT_SEL_AI_STATE,
    PLAYERBOT_INS_AI_STATE,
    PLAYERBOT_UPD_AI_STATE,
    PLAYERBOT_DEL_AI_STATE,
    
    // Sessions
    PLAYERBOT_INS_SESSION,
    PLAYERBOT_UPD_SESSION_LOGOUT,
    PLAYERBOT_SEL_ACTIVE_SESSIONS,
    
    // Config
    PLAYERBOT_SEL_CONFIG,
    PLAYERBOT_UPD_CONFIG,
    
    // Statistics
    PLAYERBOT_INS_STATISTIC,
    PLAYERBOT_SEL_STATISTICS,
    
    // Name Pool
    PLAYERBOT_SEL_RANDOM_NAME,
    PLAYERBOT_UPD_NAME_USED,
    
    MAX_PLAYERBOT_DATABASE_STATEMENTS
};
```

**File:** `src/modules/Playerbot/Database/PlayerbotDatabase.cpp` (Modified PrepareStatements)
```cpp
void PlayerbotDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_PLAYERBOT_DATABASE_STATEMENTS);
    
    // Account Info statements (metadata only)
    PrepareStatement(PLAYERBOT_SEL_ACCOUNT_INFO, 
        "SELECT account_id, is_bot, bot_type, created_at, last_selected_char, "
        "owner_account, settings FROM playerbot_account_info WHERE account_id = ?");
    
    PrepareStatement(PLAYERBOT_INS_ACCOUNT_INFO,
        "INSERT INTO playerbot_account_info (account_id, is_bot, bot_type) "
        "VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE is_bot = VALUES(is_bot)");
    
    PrepareStatement(PLAYERBOT_UPD_ACCOUNT_INFO,
        "UPDATE playerbot_account_info SET last_selected_char = ?, settings = ? "
        "WHERE account_id = ?");
    
    PrepareStatement(PLAYERBOT_DEL_ACCOUNT_INFO,
        "DELETE FROM playerbot_account_info WHERE account_id = ?");
    
    // Character Info statements (metadata only)
    PrepareStatement(PLAYERBOT_SEL_CHARACTER_INFO,
        "SELECT guid, account_id, is_bot, role, ai_strategy, follow_target, "
        "group_role, settings FROM playerbot_character_info WHERE guid = ?");
    
    PrepareStatement(PLAYERBOT_SEL_CHARACTERS_BY_ACCOUNT,
        "SELECT guid, role, ai_strategy FROM playerbot_character_info "
        "WHERE account_id = ? AND is_bot = 1");
    
    PrepareStatement(PLAYERBOT_INS_CHARACTER_INFO,
        "INSERT INTO playerbot_character_info "
        "(guid, account_id, is_bot, role, ai_strategy) "
        "VALUES (?, ?, ?, ?, ?)");
    
    PrepareStatement(PLAYERBOT_UPD_CHARACTER_INFO,
        "UPDATE playerbot_character_info "
        "SET role = ?, ai_strategy = ?, follow_target = ?, last_played = NOW() "
        "WHERE guid = ?");
    
    PrepareStatement(PLAYERBOT_DEL_CHARACTER_INFO,
        "DELETE FROM playerbot_character_info WHERE guid = ?");
    
    // AI State statements
    PrepareStatement(PLAYERBOT_SEL_AI_STATE,
        "SELECT strategy, sub_strategy, state_data, memory_data, combat_data "
        "FROM playerbot_ai_state WHERE guid = ?");
    
    PrepareStatement(PLAYERBOT_INS_AI_STATE,
        "INSERT INTO playerbot_ai_state (guid, strategy) VALUES (?, ?) "
        "ON DUPLICATE KEY UPDATE strategy = VALUES(strategy)");
    
    PrepareStatement(PLAYERBOT_UPD_AI_STATE,
        "UPDATE playerbot_ai_state "
        "SET strategy = ?, sub_strategy = ?, state_data = ?, memory_data = ? "
        "WHERE guid = ?");
    
    PrepareStatement(PLAYERBOT_DEL_AI_STATE,
        "DELETE FROM playerbot_ai_state WHERE guid = ?");
    
    // Session statements
    PrepareStatement(PLAYERBOT_INS_SESSION,
        "INSERT INTO playerbot_sessions (guid, account_id) VALUES (?, ?)");
    
    PrepareStatement(PLAYERBOT_UPD_SESSION_LOGOUT,
        "UPDATE playerbot_sessions SET logout_time = NOW() "
        "WHERE guid = ? AND logout_time IS NULL");
    
    PrepareStatement(PLAYERBOT_SEL_ACTIVE_SESSIONS,
        "SELECT session_id, guid, account_id, login_time "
        "FROM playerbot_sessions WHERE logout_time IS NULL");
    
    // Config statements
    PrepareStatement(PLAYERBOT_SEL_CONFIG,
        "SELECT value FROM playerbot_config WHERE `key` = ?");
    
    PrepareStatement(PLAYERBOT_UPD_CONFIG,
        "INSERT INTO playerbot_config (`key`, value) VALUES (?, ?) "
        "ON DUPLICATE KEY UPDATE value = VALUES(value)");
    
    // Statistics statements
    PrepareStatement(PLAYERBOT_INS_STATISTIC,
        "INSERT INTO playerbot_statistics (metric_name, metric_value, metric_data) "
        "VALUES (?, ?, ?)");
    
    PrepareStatement(PLAYERBOT_SEL_STATISTICS,
        "SELECT timestamp, metric_value, metric_data "
        "FROM playerbot_statistics "
        "WHERE metric_name = ? AND timestamp >= ? AND timestamp <= ? "
        "ORDER BY timestamp DESC LIMIT ?");
    
    // Name Pool statements
    PrepareStatement(PLAYERBOT_SEL_RANDOM_NAME,
        "SELECT id, name FROM playerbot_name_pool "
        "WHERE used = 0 AND (race = ? OR race = 0) "
        "AND (gender = ? OR gender = 2) "
        "ORDER BY RAND() LIMIT 1");
    
    PrepareStatement(PLAYERBOT_UPD_NAME_USED,
        "UPDATE playerbot_name_pool SET used = 1, used_by = ? WHERE id = ?");
}
```

---

## Summary of Changes

### Key Architectural Changes:
1. **Bot Accounts** → Created in `auth.account` using `AccountMgr::CreateAccount()`
2. **Bot Characters** → Created in `characters.characters` using standard character creation
3. **Playerbot Database** → Only stores metadata and bot-specific information
4. **No Duplication** → Leverages all existing TrinityCore systems

### Benefits:
- ✅ Bot accounts/characters are fully compatible with all TrinityCore systems
- ✅ Can use existing GM commands on bots
- ✅ Bot characters can join guilds, use mail, auction house normally
- ✅ No custom character/account handling needed
- ✅ Reduced code complexity
- ✅ Better maintainability

### Implementation Notes:
- `BotAccountMgr` wraps `AccountMgr` for bot-specific operations
- `BotCharacterMgr` uses standard character creation APIs
- Playerbot database only tracks which accounts/characters are bots
- All game data remains in standard Trinity databases

This approach ensures maximum compatibility and minimal core modifications!