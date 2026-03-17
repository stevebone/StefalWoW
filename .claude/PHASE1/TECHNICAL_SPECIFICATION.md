# Phase 1 Technical Specification: Bot Account & Character Management

## System Architecture

### Module Structure
```
Playerbot Module
├── Account Management Layer
│   ├── Bot Account Creation/Deletion
│   ├── Character Limit Enforcement (10 max)
│   └── Account Statistics & Monitoring
├── Character Management Layer
│   ├── Character Creation/Deletion
│   ├── Name Allocation System
│   └── Race/Class Distribution
├── Database Layer
│   ├── Prepared Statements
│   ├── Connection Pooling
│   └── Transaction Management
└── Configuration Layer
    ├── Runtime Settings
    ├── Performance Tuning
    └── Security Controls
```

## Core Components

### 1. BotAccountMgr (Singleton)

**Purpose**: Centralized management of bot accounts with strict character limit enforcement.

**Key Responsibilities**:
- Account CRUD operations
- Character limit validation (10 max)
- Startup integrity checks
- Performance caching
- Thread safety

**Interface**:
```cpp
class BotAccountMgr
{
public:
    static BotAccountMgr* instance();
    
    // Core Operations
    bool Initialize();
    void Shutdown();
    
    // Account Management
    uint32 CreateBotAccount(const std::string& username);
    bool DeleteBotAccount(uint32 accountId);
    bool IsBotAccount(uint32 accountId) const;
    
    // Character Limit Management
    bool CanCreateCharacter(uint32 accountId) const;
    uint32 GetCharacterCount(uint32 accountId) const;
    bool ValidateCharacterLimitsOnStartup();
    void AutoFixCharacterLimitViolations();
    
    // Hooks
    void OnCharacterCreate(uint32 accountId, uint32 characterGuid);
    void OnCharacterDelete(uint32 accountId, uint32 characterGuid);
    
    // Statistics
    uint32 GetTotalBotAccounts() const;
    uint32 GetActiveBotAccounts() const;
    
private:
    static constexpr uint32 MAX_CHARACTERS_PER_BOT_ACCOUNT = 10;
    
    // Thread-safe data structures
    std::unordered_map<uint32, BotAccountInfo> _botAccounts;
    std::unordered_map<uint32, uint32> _characterCounts;
    mutable std::mutex _accountMutex;
    mutable std::mutex _characterCountMutex;
};
```

### 2. BotNameMgr (Singleton)

**Purpose**: Unique name allocation and management for bot characters.

**Key Features**:
- 12,000+ pre-generated names
- Gender-based allocation
- Automatic release on deletion
- Database persistence
- Thread safety

**Interface**:
```cpp
class BotNameMgr
{
public:
    static BotNameMgr* instance();
    
    // Core Operations
    bool Initialize();
    void Shutdown();
    
    // Name Management
    std::string AllocateName(uint8 gender, uint32 characterGuid);
    void ReleaseName(uint32 characterGuid);
    void ReleaseName(const std::string& name);
    bool IsNameAvailable(const std::string& name) const;
    
    // Statistics
    uint32 GetAvailableNameCount(uint8 gender) const;
    uint32 GetTotalNameCount() const;
    uint32 GetUsedNameCount() const;
    
private:
    // Thread-safe name pool
    std::unordered_map<uint32, NameEntry> _names;
    std::unordered_set<uint32> _availableMaleNames;
    std::unordered_set<uint32> _availableFemaleNames;
    std::unordered_map<uint32, uint32> _guidToNameId;
    mutable std::mutex _mutex;
};
```

### 3. PlayerbotDatabase

**Purpose**: Database abstraction layer with prepared statements and connection pooling.

**Features**:
- 33 prepared statements
- Async/Sync operations
- Connection pooling
- Transaction support
- Error handling

**Key Statements**:
```cpp
enum PlayerbotDatabaseStatements : uint32
{
    // Account Management (8)
    PLAYERBOT_SEL_BOT_ACCOUNTS,
    PLAYERBOT_INS_BOT_ACCOUNT,
    PLAYERBOT_UPD_BOT_ACCOUNT_CHARACTER_COUNT,
    PLAYERBOT_SEL_ACCOUNTS_EXCEEDING_LIMIT,
    
    // Name Management (8)
    PLAYERBOT_SEL_ALL_NAMES,
    PLAYERBOT_INS_NAME_USED,
    PLAYERBOT_DEL_NAME_USED,
    
    // Character Management (6)
    PLAYERBOT_INS_BOT_CHARACTER,
    PLAYERBOT_DEL_BOT_CHARACTER,
    
    // Statistics (3)
    PLAYERBOT_INS_PERFORMANCE_METRIC,
    PLAYERBOT_SEL_PERFORMANCE_METRICS,
    
    MAX_PLAYERBOT_DATABASE_STATEMENTS
};
```

## Database Schema

### Core Tables

#### playerbot_accounts
```sql
CREATE TABLE `playerbot_accounts` (
  `account_id` INT UNSIGNED NOT NULL PRIMARY KEY,
  `is_bot` TINYINT(1) NOT NULL DEFAULT 1,
  `is_active` TINYINT(1) NOT NULL DEFAULT 1,
  `character_count` INT UNSIGNED NOT NULL DEFAULT 0,
  `last_login` TIMESTAMP NULL DEFAULT NULL,
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT `chk_character_limit` CHECK (`character_count` <= 10)
);
```

#### playerbots_names
```sql
CREATE TABLE `playerbots_names` (
  `name_id` INT(11) NOT NULL AUTO_INCREMENT PRIMARY KEY,
  `name` VARCHAR(255) NOT NULL UNIQUE,
  `gender` TINYINT(3) UNSIGNED NOT NULL,
  KEY `idx_gender` (`gender`)
);
```

#### playerbots_names_used
```sql
CREATE TABLE `playerbots_names_used` (
  `name_id` INT(11) NOT NULL PRIMARY KEY,
  `character_guid` INT UNSIGNED NOT NULL UNIQUE,
  `used_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (`name_id`) REFERENCES `playerbots_names` (`name_id`)
);
```

### Triggers for Automation

```sql
-- Auto-update character count on insert
CREATE TRIGGER `trg_bot_character_insert` 
AFTER INSERT ON `characters`
FOR EACH ROW
BEGIN
    UPDATE playerbot_accounts 
    SET character_count = character_count + 1 
    WHERE account_id = NEW.account AND is_bot = 1;
END;

-- Auto-update character count on delete
CREATE TRIGGER `trg_bot_character_delete` 
AFTER DELETE ON `characters`
FOR EACH ROW
BEGIN
    UPDATE playerbot_accounts 
    SET character_count = GREATEST(0, character_count - 1)
    WHERE account_id = OLD.account AND is_bot = 1;
    
    DELETE FROM playerbots_names_used WHERE character_guid = OLD.guid;
END;
```

### Monitoring Views

```sql
-- Account statistics
CREATE VIEW v_bot_account_stats AS
SELECT 
    COUNT(*) AS total_accounts,
    SUM(character_count) AS total_characters,
    AVG(character_count) AS avg_characters_per_account,
    MAX(character_count) AS max_characters,
    SUM(CASE WHEN character_count > 10 THEN 1 ELSE 0 END) AS accounts_over_limit
FROM playerbot_accounts WHERE is_bot = 1;

-- Name usage statistics
CREATE VIEW v_name_usage_stats AS
SELECT 
    (SELECT COUNT(*) FROM playerbots_names) AS total_names,
    (SELECT COUNT(*) FROM playerbots_names_used) AS used_names,
    (SELECT COUNT(*) FROM playerbots_names pn 
     LEFT JOIN playerbots_names_used pnu ON pn.name_id = pnu.name_id 
     WHERE pnu.name_id IS NULL) AS available_names;
```

## Configuration System

### Critical Settings

```conf
# Character Limit Enforcement
Playerbot.MaxCharactersPerAccount = 10     # Hard limit (cannot exceed)
Playerbot.StrictCharacterLimit = 1         # 1 = Abort startup on violation
Playerbot.AutoFixCharacterLimit = 0        # 1 = Auto-disable excess characters

# Performance Tuning
Playerbot.Performance.CacheTime = 60       # Cache duration in seconds
Playerbot.Performance.MaxBotsPerUpdate = 10 # Batch processing size

# Security
Playerbot.Security.PreventBotLogin = 1     # Prevent direct bot account login
Playerbot.Security.MaxGoldPerBot = 1000    # Economic balance control
```

## Thread Safety Design

### Mutex Strategy
1. **Granular Locking**: Separate mutexes for accounts and names
2. **Read-Write Optimization**: Const methods use shared locks
3. **RAII Pattern**: Lock guards ensure automatic unlock
4. **Deadlock Prevention**: Consistent lock ordering

### Example Implementation
```cpp
bool BotAccountMgr::CanCreateCharacter(uint32 accountId) const
{
    // Read-only operation uses const method
    std::lock_guard<std::mutex> lock(_characterCountMutex);
    
    auto it = _characterCounts.find(accountId);
    if (it != _characterCounts.end())
    {
        return it->second < MAX_CHARACTERS_PER_BOT_ACCOUNT;
    }
    
    // Cache miss - query database
    return GetCharacterCountFromDB(accountId) < MAX_CHARACTERS_PER_BOT_ACCOUNT;
}
```

## Performance Optimizations

### 1. Caching Strategy
- Character counts cached in memory
- 60-second cache invalidation
- Write-through cache updates
- LRU eviction for large datasets

### 2. Database Optimization
- Prepared statements reduce parsing
- Connection pooling minimizes overhead
- Batch operations for bulk updates
- Indexed columns for fast lookups

### 3. Startup Optimization
- Lazy loading of non-critical data
- Parallel validation threads
- Early termination on critical errors
- Progress logging for transparency

## Error Handling

### Error Categories

1. **Critical Errors** (Server Stop)
   - Database connection failure
   - Character limit violations (strict mode)
   - Configuration file missing

2. **Recoverable Errors** (Auto-Fix)
   - Excess characters (lenient mode)
   - Name pool exhaustion
   - Cache inconsistency

3. **Warnings** (Log Only)
   - Performance degradation
   - Non-critical validation failures
   - Deprecated configuration options

### Error Response Examples
```cpp
// Critical error - stop server
if (!ValidateCharacterLimitsOnStartup())
{
    TC_LOG_FATAL("module.playerbot", 
        "Critical: Bot accounts exceed character limit!");
    return false; // Abort startup
}

// Recoverable error - auto-fix
if (characterCount > MAX_CHARACTERS_PER_BOT_ACCOUNT)
{
    TC_LOG_WARN("module.playerbot", 
        "Warning: Account {} has {} characters, fixing...",
        accountId, characterCount);
    DisableExcessCharacters(accountId, characterCount - MAX_CHARACTERS_PER_BOT_ACCOUNT);
}

// Warning - log only
if (availableNames < 100)
{
    TC_LOG_INFO("module.playerbot", 
        "Info: Name pool running low ({} remaining)", availableNames);
}
```

## Integration Points

### 1. World Server Hooks
```cpp
// Server startup
World::StartPlayerbotsModule()
{
    if (!PlayerbotModule::Initialize())
        return ERROR_EXIT_CODE;
}

// Character creation
CharacterHandler::HandleCharCreateOpcode()
{
    if (sBotAccountMgr->IsBotAccount(accountId))
    {
        if (!sBotAccountMgr->CanCreateCharacter(accountId))
            return CHAR_CREATE_SERVER_LIMIT;
    }
}
```

### 2. GM Commands
```cpp
// .botaccount create <name>
// .botaccount delete <id>
// .botaccount charlimit check
// .botaccount charlimit fix
```

### 3. Event System
- `OnCharacterCreate` - Update counts
- `OnCharacterDelete` - Release names
- `OnServerStartup` - Validate limits
- `OnConfigReload` - Refresh settings

## Testing Strategy

### Unit Tests
1. Character limit enforcement
2. Name allocation/release
3. Thread safety under load
4. Database transaction rollback
5. Configuration parsing

### Integration Tests
1. Server startup validation
2. Character creation flow
3. Trigger functionality
4. View accuracy
5. Performance benchmarks

### Load Tests
1. 1000+ bot accounts
2. 10,000+ characters
3. Concurrent operations
4. Memory leak detection
5. Database connection limits

## Deployment Checklist

- [ ] Database schema created
- [ ] Configuration file deployed
- [ ] Name pool populated
- [ ] Triggers installed
- [ ] Views created
- [ ] Stored procedures ready
- [ ] CMake flags enabled
- [ ] Dependencies linked
- [ ] Logs configured
- [ ] Monitoring enabled

## Success Metrics

1. **Functional**
   - 100% character limit compliance
   - Zero duplicate names
   - Successful startup validation

2. **Performance**
   - <1ms character creation check
   - <100MB memory usage
   - <5s startup validation

3. **Reliability**
   - Zero crashes in 24h
   - Automatic error recovery
   - Complete audit trail

## Phase 1 Completion Criteria

✅ All bot accounts limited to 10 characters
✅ Startup validation implemented
✅ Name allocation system operational
✅ Database schema deployed
✅ Configuration system functional
✅ Thread safety verified
✅ Performance targets met
✅ Documentation complete

## Next Phase Preview

**Phase 2**: Bot AI and Behavior System
- Movement strategies
- Combat AI
- Quest handling
- Group coordination
- Economic participation
