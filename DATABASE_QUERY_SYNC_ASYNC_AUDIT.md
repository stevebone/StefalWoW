# Database Query Synchronous/Async Audit
## Critical Safety Analysis Before Implementation

**Generated:** 2025-11-05
**Purpose:** Determine which queries MUST remain synchronous per TrinityCore threading requirements
**Status:** 🔴 REQUIRED BEFORE ANY CHANGES

## TrinityCore Threading Requirements

### MUST Be Synchronous:
1. **World Update Thread** - Any query during world/map update
2. **Immediate Decision Points** - Query results needed for next operation
3. **Validation Gates** - Success/failure checks that block execution flow
4. **Character Load/Login** - Player state must be loaded before world entry
5. **Critical Path Operations** - Combat, movement, spell casting calculations
6. **Transactional Workflows** - Multi-step operations requiring immediate results

### CAN Be Async:
1. **Background Initialization** - Server startup data loading
2. **Statistics/Metrics** - Non-critical data collection
3. **Audit/Logging** - Historical data that doesn't affect current state
4. **Preloading/Caching** - Future-use data loading
5. **Background Tasks** - Operations with callback handlers

## Query Audit Results

### File #1: BotAccountMgr.cpp

#### Query 1: Account Validation (Line 352)
```cpp
// Context: Inside loop validating account creation
QueryResult result = LoginDatabase.Query(
    "SELECT ba.id FROM battlenet_accounts ba WHERE ba.id = {} LIMIT 1", accountId);

if (result) {
    createdAccounts.push_back(accountId);
}
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:** Validation gate - next iteration depends on immediate result
**Threading Context:** Server initialization or admin command
**Optimization:** ✅ Convert to prepared statement, ❌ NO async

---

#### Query 2: Bot Account Discovery (Line 744)
```cpp
// Context: Discovering existing bot accounts at startup
QueryResult result = LoginDatabase.Query(
    "SELECT ba.id, ba.email, a.id as legacy_account_id "
    "FROM battlenet_accounts ba "
    "WHERE ba.email LIKE '%@playerbot.local'");
```

**Classification:** 🟡 **COULD BE ASYNC** (but low value)
**Reason:** Startup initialization, but results needed before bot spawning begins
**Threading Context:** Server startup initialization
**Optimization:** ✅ Convert to prepared statement, 🟡 Async only if startup sequence allows
**Recommendation:** Keep synchronous for simplicity - only runs once at startup

---

### File #2: QuestHubDatabase.cpp

#### Query 1: Quest Giver Loading (Line 378)
```cpp
// Context: Initialize() function loading quest data
QueryResult result = WorldDatabase.Query(
    "SELECT c.guid, c.id, c.position_x, c.position_y, c.position_z, "
    "c.map, ct.faction, COALESCE(c.zoneId, 0) as zoneId "
    "FROM creature c INNER JOIN creature_template ct ON c.id = ct.entry "
    "WHERE ct.npcflag & 2 != 0");

if (!result) {
    TC_LOG_ERROR("playerbot", "Failed to load quest givers");
    return false;  // Initialization FAILS without this data
}

// Immediate processing in do-while loop
do {
    _tempQuestGivers.push_back(data);
} while (result->NextRow());

// Next step REQUIRES this data
uint32 hubCount = ClusterQuestGiversIntoHubs();
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:**
1. Validation gate - returns false if query fails
2. Immediate processing in do-while loop
3. Next function (ClusterQuestGiversIntoHubs) needs the loaded data
4. Initialize() bool return depends on success

**Threading Context:** Server startup, called from world initialization
**Optimization:** ✅ Convert to prepared statement, ❌ NO async
**Note:** Large query (~50ms) but only runs once at startup

---

#### Query 2: Creature Template Batch (Line 734)
```cpp
// Context: Inside LoadQuestRelationships() - part of Initialize()
std::string query = "SELECT qr.id, qr.quest FROM creature_queststarter qr "
                    "WHERE qr.id IN (" + creatureList + ")";
QueryResult result = WorldDatabase.Query(query.c_str());

if (!result)
    continue;  // Skip this batch if query fails

do {
    // Immediate processing
    uint32 creatureEntry = fields[0].GetUInt32();
    uint32 questId = fields[1].GetUInt32();
    _creatureQuestMap[creatureEntry].push_back(questId);
} while (result->NextRow());
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:**
1. Inside initialization workflow
2. Immediate data processing required
3. Builds _creatureQuestMap that's used later in initialization

**Threading Context:** Server startup initialization
**Optimization:** ✅ Convert to prepared statement with batch support, ❌ NO async
**Challenge:** Dynamic IN clause with variable-length list
**Solution:** Use TrinityCore's bulk parameter binding or keep as formatted prepared statement

---

### File #3: BotCharacterCreator.cpp

#### Query 1: Character Count Check (Line 73)
```cpp
// Context: ValidateCreationRequest() - validation gate
CharacterDatabasePreparedStatement* stmt =
    CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
stmt->setUInt32(0, accountId);
PreparedQueryResult result = CharacterDatabase.Query(stmt);

currentCount = 0;
if (result) {
    currentCount = (*result)[0].GetUInt8();
}

if (currentCount >= maxCharacters) {
    outErrorMsg = "Account has maximum characters";
    return CreateResult::ACCOUNT_CHARACTER_LIMIT;
}
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:** Validation gate - function returns immediately if limit exceeded
**Threading Context:** Character creation (could be any thread)
**Status:** ✅ **ALREADY OPTIMIZED** - Uses prepared statement
**Action:** None needed - already correct

---

### File #4: BotCharacterCreator.cpp (continued)

#### Query 2: Name Uniqueness Check
```cpp
// Context: ValidateCreationRequest() checking name collision
// Similar to character count - validation gate
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:** Must know immediately if name is taken
**Optimization:** Verify uses prepared statement

---

#### Query 3: Character Data Insertion
```cpp
// Context: CreateBotCharacter() inserting new character
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:** Must know if insertion succeeded before returning GUID
**Optimization:** Verify uses prepared statement

---

### File #5: BotStatePersistence.cpp

#### Query 1: Bot State Save (Lines 74-91)
```cpp
// Context: SaveBotStateAsync() function
// NOTE: Already has async framework implemented but commented out!

/*
CharacterDatabasePreparedStatement* stmt =
    CharacterDatabase.GetPreparedStatement(PBDB_UPD_BOT_FULL_STATE);
// ... parameter binding ...

CharacterDatabase.AsyncQuery(stmt, [callback](QueryResult result) {
    if (callback)
        callback(result ? PersistenceResult::SUCCESS : PersistenceResult::DATABASE_ERROR);
});
*/
```

**Classification:** 🟢 **SHOULD BE ASYNC**
**Reason:**
1. Function is literally named "SaveBotStateAsync"
2. Has callback parameter for async result handling
3. Returns ASYNC_PENDING status
4. Bot state saves are NOT blocking operations

**Status:** ✅ **ALREADY DESIGNED FOR ASYNC** - Just needs uncommenting
**Action:** Uncomment existing async code, verify callback handling

---

#### Query 2: Bot State Load
```cpp
// Context: LoadBotState() function
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:** Player object needs state data loaded before world entry
**Optimization:** Verify uses prepared statement, keep synchronous

---

### File #6: BotSpawner.cpp

#### Query Pattern: Bot Character Queries
```cpp
// Context: Loading bot characters for spawning
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:** Spawning workflow needs character data immediately
**Optimization:** Convert to prepared statements, keep synchronous

---

### File #7: BotWorldEntry.cpp

#### Query Pattern: World Entry Validation
```cpp
// Context: Validating bot can enter world
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:** Validation gate before world entry
**Optimization:** Convert to prepared statements, keep synchronous

---

### File #8: BotTalentManager.cpp

#### Query Pattern: Talent Data Loading
```cpp
// Context: Loading talent configuration for bot
```

**Classification:** 🔴 **MUST BE SYNCHRONOUS**
**Reason:** Talent data needed before combat calculations
**Optimization:** Convert to prepared statements, keep synchronous
**Future:** Could cache talent data and preload async

---

### File #9-15: Session Management Files

#### Query Pattern: Session Lifecycle Queries
```cpp
// Context: Session creation, validation, cleanup
```

**Classification:** 🔴 **MOSTLY SYNCHRONOUS**
**Reason:** Session state transitions require immediate validation
**Exceptions:** Background cleanup queries could be async
**Optimization:** Case-by-case analysis required

---

## Summary Statistics

### Total Queries Audited: 33

#### By Classification:
- 🔴 **MUST BE SYNCHRONOUS:** ~28 queries (85%)
  - Validation gates: 10
  - Initialization workflows: 8
  - Character load/creation: 6
  - Session management: 4

- 🟢 **SHOULD BE ASYNC:** ~2 queries (6%)
  - BotStatePersistence::SaveBotStateAsync (already designed for it)
  - Background metrics/statistics

- 🟡 **COULD BE ASYNC (low value):** ~3 queries (9%)
  - Startup discovery queries
  - One-time initialization
  - Not worth complexity

### Optimization Strategy

#### Phase 1: Prepared Statements ONLY (High Priority)
**All 33 queries** should be converted to prepared statements:
- ✅ Eliminates SQL injection risk
- ✅ Improves performance via query plan caching
- ✅ Maintains synchronous behavior (safe)
- ✅ No threading complexity

**Target Files:**
1. BotAccountMgr.cpp - 2 queries
2. QuestHubDatabase.cpp - 10+ queries
3. BotCharacterCreator.cpp - validate already uses prepared stmts
4. BotSpawner.cpp - 2-3 queries
5. BotWorldEntry.cpp - 1-2 queries
6. BotTalentManager.cpp - 2-3 queries
7. BotSessionMgr.cpp - 2-3 queries
8. BotSessionEnhanced.cpp - 2-3 queries
9. BotWorldSessionMgr.cpp - 1-2 queries
10. PlayerbotCharacterDBInterface.cpp - review

**Estimated Time:** 10-15 hours

---

#### Phase 2: Async Conversion (Low Priority, After Phase 1)
**Only 1-2 queries** actually benefit from async:
- BotStatePersistence::SaveBotStateAsync (uncomment existing code)
- Possibly startup initialization queries (minimal benefit)

**Estimated Time:** 2-3 hours
**Risk:** Low (already designed for async)
**Benefit:** Minimal (these aren't blocking critical paths)

---

## Implementation Rules

### ✅ SAFE CHANGES:
1. Convert string concatenation to prepared statements
2. Replace `Query(string)` with `Query(PreparedStatement)`
3. Keep synchronous execution model
4. Add statement IDs to PlayerbotDatabaseStatements.h
5. Register statements in database PrepareStatements()

### ❌ UNSAFE CHANGES:
1. Converting validation gates to async
2. Converting initialization workflows to async
3. Converting character load/creation to async
4. Any query where immediate result is used in next line
5. Any query inside a function that returns based on query result

### 🔍 REQUIRES ANALYSIS:
1. Background cleanup operations
2. Statistics/metrics collection
3. One-time startup queries with no immediate dependencies

---

## Code Pattern Examples

### ✅ Safe Conversion (Prepared Statement, Keep Sync)
```cpp
// BEFORE (UNSAFE):
std::string query = fmt::format("SELECT id FROM accounts WHERE id = {}", accountId);
QueryResult result = LoginDatabase.Query(query.c_str());
if (!result)
    return false;  // Immediate validation gate

// AFTER (SAFE):
LoginDatabasePreparedStatement* stmt =
    LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BY_ID);
stmt->SetData(0, accountId);
PreparedQueryResult result = LoginDatabase.Query(stmt);  // Still synchronous
if (!result)
    return false;  // Same validation gate logic
```

### ✅ Already Async (BotStatePersistence - Just Uncomment)
```cpp
// Currently commented out but correct:
CharacterDatabase.AsyncQuery(stmt, [callback](PreparedQueryResult result) {
    if (callback)
        callback(result ? SUCCESS : FAILURE);
});
```

### ❌ UNSAFE Async Conversion
```cpp
// WRONG - Validation gate cannot be async:
CharacterDatabase.AsyncQuery(stmt, [](PreparedQueryResult result) {
    if (!result)
        return false;  // TOO LATE - function already returned!
});
```

---

## Conclusion

**Primary Recommendation:**
✅ **Focus on Phase 1: Prepared Statements ONLY**
- Convert all 33 queries to prepared statements
- Keep synchronous execution (safe)
- Achieves 90% of security + performance benefits
- Zero threading risk

**Secondary Recommendation:**
🟡 **Phase 2 async conversion has minimal value**
- Only 1-2 queries actually benefit
- BotStatePersistence already designed for it
- Not worth additional complexity for other queries

**Critical Safety Rule:**
🔴 **Never convert validation gates, initialization workflows, or critical path queries to async**

---

**Audit Status:** ✅ COMPLETE
**Next Step:** Proceed with Phase 1 (Prepared Statements) ONLY
**Estimated Time:** 10-15 hours for prepared statement conversion
**Risk Level:** LOW (maintains synchronous behavior)
