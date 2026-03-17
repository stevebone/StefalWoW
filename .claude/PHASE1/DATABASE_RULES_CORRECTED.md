# CORRECTED Database Usage Rules for TrinityCore Playerbot Module

## ✅ CORRECT Database Architecture

### Trinity Core Databases (USE THESE)
```
auth Database (via Trinity APIs)
├── bnet_account               ✅ PRIMARY: Bot BNet accounts via CreateBattlenetAccount()
├── account                    ✅ LINKED: Legacy accounts automatically created
├── account_access             ✅ Bot account permissions
├── account_banned             ✅ Bot account security
└── bnet_account_mapping       ✅ Links BNet accounts to legacy accounts

characters Database (via Trinity APIs)
├── characters                 ✅ Bot characters created here via Player::Create()
├── character_inventory        ✅ Bot inventory handled by Player class
├── character_spell            ✅ Bot spells handled by Player class
├── character_queststatus      ✅ Bot quests handled by Player class
├── guild_member               ✅ Bots can join guilds normally
└── [all character tables]     ✅ All character data uses standard Trinity systems

world Database (READ-ONLY via Trinity APIs)
├── creature_template          ✅ Use existing world data access patterns
├── gameobject_template        ✅ Use existing world data access patterns
├── quest_template             ✅ Use existing world data access patterns
└── [all world tables]         ✅ Use Trinity's world data systems
```

### Playerbot Database (ONLY Bot Metadata)
```
playerbot Database (NEW - Metadata Only)
├── playerbot_ai_state         ✅ AI strategy, behavior settings
├── playerbot_performance      ✅ Performance metrics, statistics
├── playerbot_config           ✅ Runtime configuration overrides
├── playerbot_sessions         ✅ Session tracking and hibernation
└── playerbot_character_meta   ✅ Links to characters.characters via foreign keys
```

## Implementation Examples

### ✅ CORRECT: Bot Account Creation
```cpp
// Use Trinity's AccountMgr - creates BattleNet account + linked legacy account
uint32 bnetAccountId = sAccountMgr->CreateBattlenetAccount(email, password);

// Get the linked legacy account ID (used for character creation)
uint32 legacyAccountId = sAccountMgr->GetAccountIdByBattlenetAccount(bnetAccountId);

// Store ONLY bot-specific metadata in playerbot database
PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_INS_ACCOUNT_META);
stmt->SetData(0, legacyAccountId); // Use legacy account ID for character linking
stmt->SetData(1, bnetAccountId);   // Store BNet account ID for reference
stmt->SetData(2, "default_ai_strategy");
sBotDBPool->ExecuteAsyncNoResult(stmt);
```

### ✅ CORRECT: Bot Character Creation
```cpp
// Use Trinity's Player class - creates in characters.characters
Player* botCharacter = Player::Create(guid, accountId, name, race, classId, gender);
botCharacter->SaveToDB(); // Saves to characters.characters

// Store ONLY bot-specific metadata in playerbot database
PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_INS_CHARACTER_META);
stmt->SetData(0, botCharacter->GetGUID().GetCounter());
stmt->SetData(1, accountId);
stmt->SetData(2, "tank"); // Bot role
stmt->SetData(3, "defensive"); // AI strategy
sBotDBPool->ExecuteAsyncNoResult(stmt);
```

### ✅ CORRECT: Bot Data Access
```cpp
void BotSession::LoadBotData()
{
    // Character data automatically loaded from characters.characters by Trinity
    // Load ONLY bot-specific AI state from playerbot database

    PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_SEL_AI_STATE);
    stmt->SetData(0, GetPlayer()->GetGUID().GetCounter());

    sBotDBPool->ExecuteAsync(stmt, [this](PreparedQueryResult result) {
        if (result) {
            Field* fields = result->Fetch();
            std::string aiStrategy = fields[0].GetString();
            std::string behaviorSettings = fields[1].GetString();

            // Configure bot AI with loaded settings
            ConfigureAI(aiStrategy, behaviorSettings);
        }
    });
}
```

### ✅ CORRECT: World Data Access
```cpp
void BotAI::FindNearbyQuests()
{
    // Use Trinity's existing world data access patterns
    // DO NOT query world database directly

    // Use existing Trinity systems:
    auto questMap = sObjectMgr->GetQuestMap();
    for (auto const& questPair : questMap) {
        Quest const* quest = questPair.second;
        // Process quest using Trinity's Quest class
    }
}
```

## Database Schema Design

### Playerbot Database Schema (ONLY Metadata)
```sql
-- Link to Trinity's auth system (both BNet and legacy accounts)
CREATE TABLE playerbot_account_meta (
    account_id INT UNSIGNED PRIMARY KEY,      -- Legacy account ID (for character creation)
    bnet_account_id INT UNSIGNED NOT NULL,   -- BattleNet account ID (primary account)
    ai_strategy VARCHAR(50) DEFAULT 'default',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    settings JSON,
    FOREIGN KEY (account_id) REFERENCES auth.account(id) ON DELETE CASCADE,
    FOREIGN KEY (bnet_account_id) REFERENCES auth.bnet_account(id) ON DELETE CASCADE
);

-- Link to Trinity's characters.characters
CREATE TABLE playerbot_character_meta (
    character_guid INT UNSIGNED PRIMARY KEY,
    account_id INT UNSIGNED NOT NULL,
    role ENUM('tank', 'healer', 'dps', 'pvp') DEFAULT 'dps',
    ai_strategy VARCHAR(50) DEFAULT 'default',
    behavior_settings JSON,
    performance_stats JSON,
    FOREIGN KEY (character_guid) REFERENCES characters.characters(guid) ON DELETE CASCADE,
    FOREIGN KEY (account_id) REFERENCES auth.account(id) ON DELETE CASCADE
);

-- AI runtime state (can be cleared on restart)
CREATE TABLE playerbot_ai_state (
    character_guid INT UNSIGNED PRIMARY KEY,
    current_strategy VARCHAR(50),
    state_data JSON,
    last_update TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (character_guid) REFERENCES characters.characters(guid) ON DELETE CASCADE
);
```

## Key Benefits of This Approach

### ✅ Advantages
1. **Full Compatibility**: Bots work with ALL existing Trinity systems
2. **GM Commands**: All character/account GM commands work on bots
3. **Guild System**: Bots can join guilds, use guild bank, etc.
4. **Economy**: Bots can use auction house, trade, mail normally
5. **World Events**: Bots participate in events, battlegrounds, dungeons
6. **Minimal Code**: Reuse 99% of Trinity's existing character handling
7. **Easy Debugging**: Use existing Trinity tools and commands

### ❌ What We DON'T Store in Playerbot Database
- Account information (use auth.account)
- Character stats, inventory, spells (use characters.*)
- World data like quests, NPCs, items (use world.*)
- Any data that Trinity already handles

### ✅ What We DO Store in Playerbot Database
- AI strategy and behavior settings
- Bot-specific performance metrics
- Session hibernation state
- Bot role assignments (tank/healer/dps)
- Custom bot configuration overrides

## Implementation Rules

### Database Access Patterns
```cpp
// ✅ CORRECT - Use Trinity's systems
Player* bot = ObjectAccessor::GetPlayer(*GetPlayer(), guid);
bot->GiveXP(experience); // Uses Trinity's character system

// ✅ CORRECT - Use Trinity's account system
std::string accountName = sAccountMgr->GetName(accountId);

// ✅ CORRECT - Bot metadata only
sBotDBPool->ExecuteAsync("SELECT ai_strategy FROM playerbot_character_meta WHERE character_guid = ?");

// ❌ FORBIDDEN - Don't bypass Trinity's systems
CharacterDatabase.Execute("UPDATE characters SET level = ? WHERE guid = ?"); // Use Player class instead
```

This corrected approach ensures bots are treated exactly like player characters while maintaining the enterprise-grade performance optimizations and modular design.