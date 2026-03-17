# FINAL CORRECTED: TrinityCore Bot Account Creation

## ✅ CORRECT TrinityCore Account Architecture

### Modern TrinityCore Account System
```
BattleNet Account (Primary - auth.bnet_account)
├── id: 12345
├── email: bot001@playerbot.local
├── sha_pass_hash: [password hash]
├── joindate: [timestamp]
└── AUTOMATICALLY CREATES →
    Legacy Account (auth.account)
    ├── id: 67890 (different ID)
    ├── username: generated
    ├── expansion: current
    └── USED FOR →
        Characters (characters.characters)
        ├── character 1
        ├── character 2
        └── ... (up to 10 per bot account)
```

## ✅ CORRECT Implementation

### BotAccountMgr Implementation
```cpp
class BotAccountMgr
{
public:
    struct BotAccountInfo {
        uint32 bnetAccountId;     // Primary BattleNet account ID
        uint32 legacyAccountId;   // Legacy account ID (for character creation)
        std::string email;
        std::string aiStrategy;
        std::chrono::system_clock::time_point createdAt;
    };

    BotAccountInfo CreateBotAccount(std::string const& email, std::string const& password)
    {
        // STEP 1: Create BattleNet account (primary account)
        uint32 bnetAccountId = sAccountMgr->CreateBattlenetAccount(email, password);

        if (bnetAccountId == 0) {
            TC_LOG_ERROR("module.playerbot.account",
                "Failed to create BattleNet account for email: {}", email);
            return {};
        }

        // STEP 2: Get the automatically created legacy account ID
        uint32 legacyAccountId = sAccountMgr->GetAccountIdByBattlenetAccount(bnetAccountId);

        if (legacyAccountId == 0) {
            TC_LOG_ERROR("module.playerbot.account",
                "Failed to retrieve legacy account for BNet account: {}", bnetAccountId);
            return {};
        }

        // STEP 3: Store bot metadata linking both account IDs
        PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_INS_ACCOUNT_META);
        stmt->SetData(0, legacyAccountId);  // Legacy account (for characters)
        stmt->SetData(1, bnetAccountId);    // BNet account (primary)
        stmt->SetData(2, "default");        // AI strategy
        sBotDBPool->ExecuteAsyncNoResult(stmt);

        TC_LOG_INFO("module.playerbot.account",
            "Created bot account: BNet ID {}, Legacy ID {}, Email: {}",
            bnetAccountId, legacyAccountId, email);

        return {bnetAccountId, legacyAccountId, email, "default", std::chrono::system_clock::now()};
    }

    bool DeleteBotAccount(uint32 bnetAccountId)
    {
        // Get legacy account ID
        uint32 legacyAccountId = sAccountMgr->GetAccountIdByBattlenetAccount(bnetAccountId);

        // Delete all associated characters first
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARS_BY_ACCOUNT);
        stmt->SetData(0, legacyAccountId);
        CharacterDatabase.Execute(stmt);

        // Delete BattleNet account (also deletes linked legacy account)
        AccountOpResult result = sAccountMgr->DeleteBattlenetAccount(bnetAccountId);

        if (result == AOR_OK) {
            // Remove bot metadata
            stmt = sBotDBPool->GetPreparedStatement(BOT_DEL_ACCOUNT_META);
            stmt->SetData(0, legacyAccountId);
            sBotDBPool->ExecuteAsyncNoResult(stmt);

            return true;
        }

        return false;
    }
};
```

### Character Creation with Correct Account ID
```cpp
class BotCharacterMgr
{
public:
    uint32 CreateBotCharacter(uint32 bnetAccountId, std::string const& name,
                              uint8 race, uint8 classId, uint8 gender)
    {
        // IMPORTANT: Use LEGACY account ID for character creation
        uint32 legacyAccountId = sAccountMgr->GetAccountIdByBattlenetAccount(bnetAccountId);

        if (legacyAccountId == 0) {
            TC_LOG_ERROR("module.playerbot.character",
                "Invalid BNet account ID: {}", bnetAccountId);
            return 0;
        }

        // Check character limit (10 per bot account)
        if (!CanCreateCharacter(legacyAccountId)) {
            TC_LOG_WARN("module.playerbot.character",
                "Character limit reached for legacy account: {}", legacyAccountId);
            return 0;
        }

        // Create character using Trinity's standard system
        uint32 characterGuid = Player::Create(legacyAccountId, name, race, classId, gender);

        if (characterGuid == 0) {
            TC_LOG_ERROR("module.playerbot.character",
                "Failed to create character for legacy account: {}", legacyAccountId);
            return 0;
        }

        // Store bot character metadata
        PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_INS_CHARACTER_META);
        stmt->SetData(0, characterGuid);
        stmt->SetData(1, legacyAccountId);  // Legacy account for character linkage
        stmt->SetData(2, bnetAccountId);    // BNet account for reference
        stmt->SetData(3, DetermineRole(classId)); // tank/healer/dps
        stmt->SetData(4, "default");        // AI strategy
        sBotDBPool->ExecuteAsyncNoResult(stmt);

        TC_LOG_INFO("module.playerbot.character",
            "Created bot character: GUID {}, Name {}, Legacy Account {}, BNet Account {}",
            characterGuid, name, legacyAccountId, bnetAccountId);

        return characterGuid;
    }

private:
    bool CanCreateCharacter(uint32 legacyAccountId) const
    {
        // Check character count using Trinity's character database
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARS_BY_ACCOUNT_ID);
        stmt->SetData(0, legacyAccountId);

        PreparedQueryResult result = CharacterDatabase.Query(stmt);
        uint32 characterCount = result ? static_cast<uint32>(result->GetRowCount()) : 0;

        return characterCount < MAX_CHARACTERS_PER_BOT_ACCOUNT; // 10
    }
};
```

## Database Schema with Correct Foreign Keys

### Playerbot Metadata Tables
```sql
-- Bot account metadata (links to both BNet and legacy accounts)
CREATE TABLE playerbot_account_meta (
    legacy_account_id INT UNSIGNED PRIMARY KEY,     -- For character creation
    bnet_account_id INT UNSIGNED NOT NULL UNIQUE,   -- Primary account
    ai_strategy VARCHAR(50) DEFAULT 'default',
    behavior_settings JSON,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_active TIMESTAMP NULL,

    -- Foreign key constraints
    FOREIGN KEY (legacy_account_id) REFERENCES auth.account(id) ON DELETE CASCADE,
    FOREIGN KEY (bnet_account_id) REFERENCES auth.bnet_account(id) ON DELETE CASCADE,

    -- Indexes for performance
    INDEX idx_bnet_account (bnet_account_id),
    INDEX idx_last_active (last_active)
) ENGINE=InnoDB;

-- Bot character metadata (links to Trinity's character system)
CREATE TABLE playerbot_character_meta (
    character_guid INT UNSIGNED PRIMARY KEY,
    legacy_account_id INT UNSIGNED NOT NULL,
    bnet_account_id INT UNSIGNED NOT NULL,
    role ENUM('tank', 'healer', 'dps', 'pvp') DEFAULT 'dps',
    ai_strategy VARCHAR(50) DEFAULT 'default',
    behavior_settings JSON,
    performance_stats JSON,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_played TIMESTAMP NULL,

    -- Foreign key constraints
    FOREIGN KEY (character_guid) REFERENCES characters.characters(guid) ON DELETE CASCADE,
    FOREIGN KEY (legacy_account_id) REFERENCES auth.account(id) ON DELETE CASCADE,
    FOREIGN KEY (bnet_account_id) REFERENCES auth.bnet_account(id) ON DELETE CASCADE,

    -- Indexes for performance
    INDEX idx_legacy_account (legacy_account_id),
    INDEX idx_bnet_account (bnet_account_id),
    INDEX idx_role (role),
    INDEX idx_last_played (last_played)
) ENGINE=InnoDB;

-- Trigger to enforce character limit
DELIMITER $$
CREATE TRIGGER trg_bot_character_limit
    BEFORE INSERT ON characters.characters
    FOR EACH ROW
BEGIN
    DECLARE char_count INT DEFAULT 0;
    DECLARE is_bot_account INT DEFAULT 0;

    -- Check if this is a bot account
    SELECT COUNT(*) INTO is_bot_account
    FROM playerbot_account_meta
    WHERE legacy_account_id = NEW.account;

    IF is_bot_account > 0 THEN
        -- Count existing characters for this bot account
        SELECT COUNT(*) INTO char_count
        FROM characters.characters
        WHERE account = NEW.account;

        IF char_count >= 10 THEN
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'Bot account character limit (10) exceeded';
        END IF;
    END IF;
END$$
DELIMITER ;
```

## Session Integration

### BotSession Constructor with Correct Account ID
```cpp
BotSession::BotSession(uint32 bnetAccountId)
    : WorldSession(
        sAccountMgr->GetAccountIdByBattlenetAccount(bnetAccountId), // Use legacy account ID
        "",           // Empty username (generated by Trinity)
        bnetAccountId, // BattleNet account ID
        nullptr,      // No socket
        SEC_PLAYER,   // Security level
        EXPANSION_LEVEL_CURRENT,
        0,            // Mute time
        "",           // OS
        Minutes(0),   // Timezone
        0,            // Build
        ClientBuild::Classic,
        LOCALE_enUS,
        0,            // Recruiter
        false)        // Is recruiter
{
    _bnetAccountId = bnetAccountId;
    _legacyAccountId = sAccountMgr->GetAccountIdByBattlenetAccount(bnetAccountId);

    TC_LOG_DEBUG("module.playerbot.session",
        "Created BotSession: BNet ID {}, Legacy ID {}",
        _bnetAccountId, _legacyAccountId);
}
```

## Key Benefits

### ✅ Advantages of This Approach
1. **Fully Compatible** - Works with ALL Trinity account systems
2. **Modern Standards** - Uses current TrinityCore BattleNet account system
3. **Automatic Linking** - Legacy accounts created automatically
4. **Character Limits** - Enforced at database level with triggers
5. **GM Commands** - All account management commands work on bot accounts
6. **Security** - Bot accounts have same security features as player accounts
7. **Audit Trail** - Complete account creation/deletion tracking

### 🔧 Character Creation Flow
```
1. CreateBattlenetAccount(email, password)
   ↓
2. Trinity automatically creates linked legacy account
   ↓
3. Use legacy account ID for Player::Create(...)
   ↓
4. Store bot metadata linking both account IDs
   ↓
5. Characters appear normally in characters.characters
```

This final corrected approach ensures bot accounts are created using TrinityCore's modern BattleNet account system while maintaining full compatibility with all existing Trinity features and the 10-character limit per bot account.