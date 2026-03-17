# Automated World Population System - Troubleshooting FAQ

## Overview

This guide addresses common issues, error messages, and troubleshooting procedures for the Automated World Population System in TrinityCore Playerbot module.

---

## Table of Contents

1. [Startup Issues](#startup-issues)
2. [Configuration Problems](#configuration-problems)
3. [Database Issues](#database-issues)
4. [Bot Creation Failures](#bot-creation-failures)
5. [Performance Issues](#performance-issues)
6. [Distribution Imbalance](#distribution-imbalance)
7. [Gear Generation Issues](#gear-generation-issues)
8. [Talent Application Failures](#talent-application-failures)
9. [Zone Placement Issues](#zone-placement-issues)
10. [ThreadPool Integration Issues](#threadpool-integration-issues)
11. [Debugging Techniques](#debugging-techniques)
12. [Error Code Reference](#error-code-reference)

---

## Startup Issues

### Q1: System fails to initialize with "BotLevelDistribution not ready"

**Symptoms:**
```
ERROR: BotLevelDistribution failed to initialize
WARN: Automated World Population System disabled
```

**Causes:**
- Configuration file not loaded
- Invalid bracket percentages (don't sum to 100%)
- System explicitly disabled in config

**Solutions:**

1. **Verify Configuration File:**
```bash
# Check if playerbots.conf exists
ls -l etc/playerbots.conf

# Verify it's being loaded
grep "Playerbot.LevelManager.Enabled" etc/playerbots.conf
```

2. **Check Bracket Percentages:**
```conf
# Each faction must sum to 100% (±0.1%)
Playerbot.LevelDistribution.Alliance.Bracket.L1 = 3.0
# ... remaining 16 brackets must sum to 100.0%
```

**Verification Command:**
```bash
# Sum Alliance bracket percentages
grep "Playerbot.LevelDistribution.Alliance" etc/playerbots.conf | \
  awk -F'=' '{sum += $2} END {print "Total: " sum "%"}'
```

3. **Enable System:**
```conf
Playerbot.LevelManager.Enabled = 1
```

**Expected Output After Fix:**
```
BotLevelDistribution: Loaded 17 Alliance brackets (100.0% total)
BotLevelDistribution: Loaded 17 Horde brackets (100.0% total)
✅ BotLevelDistribution initialized successfully
```

---

### Q2: "GearFactory failed to initialize - no items loaded"

**Symptoms:**
```
ERROR: BotGearFactory: No items found for class 1 spec 0
WARN: GearFactory cache is empty
```

**Causes:**
- Item tables not populated in database
- Database connection failure
- Item cache disabled

**Solutions:**

1. **Verify Database Population:**
```sql
-- Check if items exist
SELECT COUNT(*) as total_items FROM item_template WHERE Quality >= 2;

-- Check class-appropriate items
SELECT class, subclass, COUNT(*)
FROM item_template
WHERE InventoryType > 0 AND Quality >= 2
GROUP BY class, subclass;
```

**Expected:** Should return thousands of items for various classes/subclasses.

2. **Check Database Connection:**
```sql
-- Test connection
SHOW TABLES LIKE 'item_template';

-- Verify permissions
SHOW GRANTS FOR 'playerbot'@'localhost';
```

3. **Enable Caching:**
```conf
Playerbot.GearFactory.EnableCache = 1
```

4. **Rebuild Cache:**
```
.reload config
# Restart worldserver to rebuild caches
```

**Expected Output After Fix:**
```
BotGearFactory: Building cache for 13 classes...
BotGearFactory: Cached 45,283 items for all classes/specs
✅ BotGearFactory initialized successfully
```

---

### Q3: "TalentManager: No loadouts available"

**Symptoms:**
```
WARN: BotTalentManager: No talent loadouts loaded
ERROR: Cannot apply talents - loadout table empty
```

**Causes:**
- Database table `playerbot_talent_loadouts` not created
- Table exists but has no data
- Database migration not applied

**Solutions:**

1. **Create Table:**
```sql
-- Run the SQL migration
SOURCE src/modules/Playerbot/sql/base/playerbot_talent_loadouts.sql;

-- Verify table exists
SHOW TABLES LIKE 'playerbot_talent_loadouts';
```

2. **Populate Initial Data:**
```sql
-- Insert basic loadouts for all classes/specs
INSERT INTO playerbot_talent_loadouts
(class_id, spec_id, min_level, max_level, talent_string, description)
VALUES
-- Warrior Protection
(1, 0, 10, 80, '12345,67890,11223', 'Protection Warrior - Tank'),
-- Add remaining 38 specs (13 classes × 3 specs each)
...;
```

3. **Verify Data:**
```sql
-- Check loadout count
SELECT class_id, spec_id, COUNT(*) as loadouts
FROM playerbot_talent_loadouts
GROUP BY class_id, spec_id;
```

**Expected:** At least 1 loadout per class/spec combination (39 minimum).

**Expected Output After Fix:**
```
BotTalentManager: Loaded 127 talent loadouts for 13 classes
✅ BotTalentManager initialized successfully
```

---

## Configuration Problems

### Q4: Percentages don't sum to 100%

**Symptoms:**
```
ERROR: Alliance bracket percentages sum to 98.5%, expected 100.0%
FATAL: BotLevelDistribution initialization failed
```

**Cause:** Configuration typo or missing bracket.

**Solution:**

```bash
# Calculate current sum
grep "Playerbot.LevelDistribution.Alliance.Bracket" etc/playerbots.conf | \
  awk -F'=' '{sum += $2} END {print sum}'

# Find missing/incorrect values
grep "Playerbot.LevelDistribution.Alliance.Bracket" etc/playerbots.conf | \
  awk -F'=' '{if($2 < 1.0 || $2 > 20.0) print "SUSPICIOUS: " $0}'
```

**Fix Template:**
```conf
# Must have exactly 17 brackets summing to 100.0%
Playerbot.LevelDistribution.Alliance.Bracket.L1 = 3.0
Playerbot.LevelDistribution.Alliance.Bracket.L5 = 5.0
Playerbot.LevelDistribution.Alliance.Bracket.L10 = 6.0
# ... continue for all 17 brackets
```

**Validation Script:**
```bash
#!/bin/bash
SUM=$(grep "Alliance.Bracket" playerbots.conf | awk -F'=' '{s+=$2} END {print s}')
if (( $(echo "$SUM >= 99.9 && $SUM <= 100.1" | bc -l) )); then
    echo "✅ Valid sum: $SUM%"
else
    echo "❌ Invalid sum: $SUM% (must be 100.0%)"
fi
```

---

### Q5: Bots spawn at wrong levels

**Symptoms:**
- All bots spawning at level 80
- No low-level bots being created
- Distribution heavily skewed

**Causes:**
- Bracket percentages heavily favor endgame (L80 = 50%)
- Tolerance too loose (>15%)
- Selection algorithm not working

**Solutions:**

1. **Review Distribution:**
```conf
# Ensure balanced distribution
Playerbot.LevelDistribution.Alliance.Bracket.L80 = 8.0  # Not 50.0!
```

2. **Check Tolerance:**
```conf
Playerbot.LevelManager.DistributionTolerance = 15  # ±15% is recommended
```

3. **Verify In-Game:**
```sql
-- Check current distribution
SELECT
    FLOOR(level / 5) * 5 as bracket,
    COUNT(*) as count,
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM characters WHERE account > 1), 2) as percent
FROM characters
WHERE account > 1
GROUP BY FLOOR(level / 5)
ORDER BY bracket;
```

**Expected:** Each bracket within ±15% of target percentage.

---

## Database Issues

### Q6: "Duplicate entry" error during loadout insertion

**Symptoms:**
```sql
ERROR 1062: Duplicate entry '1-0-10-80' for key 'unique_loadout'
```

**Cause:** Trying to insert overlapping level ranges for same class/spec.

**Solution:**

```sql
-- Find duplicates
SELECT class_id, spec_id, min_level, max_level, COUNT(*)
FROM playerbot_talent_loadouts
GROUP BY class_id, spec_id, min_level, max_level
HAVING COUNT(*) > 1;

-- Remove duplicates (keep lowest ID)
DELETE t1 FROM playerbot_talent_loadouts t1
INNER JOIN playerbot_talent_loadouts t2
WHERE t1.id > t2.id
  AND t1.class_id = t2.class_id
  AND t1.spec_id = t2.spec_id
  AND t1.min_level = t2.min_level
  AND t1.max_level = t2.max_level;
```

**Prevention:** Use non-overlapping level ranges:
```sql
-- Good: Non-overlapping
(10, 39), (40, 69), (70, 80)

-- Bad: Overlapping
(10, 50), (40, 80)  -- 40-50 overlaps!
```

---

### Q7: Database connection timeout during cache building

**Symptoms:**
```
ERROR: Lost connection to MySQL server during query
WARN: GearFactory cache incomplete (23,891 / 45,283 items)
```

**Causes:**
- Query timeout too short
- Large item table (>100k rows)
- Network latency

**Solutions:**

1. **Increase MySQL Timeouts:**
```sql
-- Temporary (session)
SET SESSION wait_timeout = 600;
SET SESSION interactive_timeout = 600;

-- Permanent (my.cnf / my.ini)
[mysqld]
wait_timeout = 600
interactive_timeout = 600
net_read_timeout = 120
net_write_timeout = 120
```

2. **Optimize Item Query:**
```cpp
// Current query (potentially slow)
SELECT * FROM item_template WHERE Quality >= 2;

// Optimized query (with index hint)
SELECT * FROM item_template USE INDEX (idx_quality) WHERE Quality >= 2;
```

3. **Add Database Index:**
```sql
-- Speed up item cache queries
CREATE INDEX idx_quality_class ON item_template (Quality, class, subclass);
```

**Verification:**
```bash
# Test query performance
time mysql -u playerbot -p playerbot_world -e \
  "SELECT COUNT(*) FROM item_template WHERE Quality >= 2;"
```

**Expected:** Query completes in <5 seconds.

---

## Bot Creation Failures

### Q8: "Failed to apply bot - Player not found"

**Symptoms:**
```
ERROR: ApplyBot_MainThread: Player GUID 0x... not found
WARN: Bot creation task failed after 3 retries
```

**Causes:**
- Bot logged out before main thread processing
- Player object destroyed prematurely
- GUID mismatch

**Solutions:**

1. **Verify Bot Exists:**
```sql
-- Check character exists in database
SELECT guid, name, account, online
FROM characters
WHERE guid = 12345;  -- Replace with actual GUID
```

2. **Check Processing Delay:**
```conf
# Reduce queue delay to process faster
Playerbot.LevelManager.MaxBotsPerUpdate = 20  # Process more per tick
```

3. **Verify Bot Session:**
```cpp
// In calling code
Player* bot = CreateNewBot(...);
if (!bot || !bot->IsInWorld())
{
    LOG_ERROR("Failed to create bot session");
    return;
}

// Submit task
sBotLevelManager->CreateBotAsync(bot);

// Keep bot session alive (don't logout immediately!)
```

**Prevention:** Ensure bots remain online during processing:
```cpp
// Good: Bot stays online
CreateBotAsync(bot);  // Returns taskId
// Bot remains in world, task processes asynchronously

// Bad: Bot logs out immediately
CreateBotAsync(bot);
bot->GetSession()->LogoutPlayer();  // DON'T DO THIS!
```

---

### Q9: Bots stuck in creation queue

**Symptoms:**
```
INFO: Queue size: 1,247 tasks (growing)
WARN: Tasks older than 5 minutes detected
```

**Causes:**
- Main thread not calling `ProcessBotCreationQueue()`
- MaxBotsPerUpdate set too low
- World Update hanging

**Solutions:**

1. **Verify Queue Processing:**
```cpp
// In World::Update() or appropriate hook
void World::Update(uint32 diff)
{
    // ... existing updates ...

    // Process bot creation queue (MUST BE CALLED EVERY TICK)
    if (sBotLevelManager->IsEnabled())
        sBotLevelManager->ProcessBotCreationQueue(20);
}
```

2. **Check Processing Rate:**
```conf
# Increase bots processed per update
Playerbot.LevelManager.MaxBotsPerUpdate = 50  # Up from 10
```

3. **Monitor Queue:**
```cpp
// Console command for debugging
.playerbot queue status

// Output:
// Queue Size: 1,247 tasks
// Processed This Tick: 20
// Oldest Task Age: 127 seconds
// Estimated Drain Time: 63 seconds
```

**Expected Behavior:**
- Queue size decreases over time
- All tasks processed within 2 minutes
- No tasks older than 5 minutes

---

## Performance Issues

### Q10: High CPU usage after system activation

**Symptoms:**
```
CPU: 85% worldserver
TPS: 12 (target 50)
Lag spikes every 2-3 seconds
```

**Causes:**
- Too many bots processing simultaneously
- Worker threads contending for resources
- Cache not initialized (database queries per bot)

**Solutions:**

1. **Reduce Batch Size:**
```conf
# Process fewer bots per tick
Playerbot.LevelManager.MaxBotsPerUpdate = 5  # Down from 10

# Reduce ThreadPool workers
Playerbot.LevelManager.ThreadPoolWorkers = 2  # Down from 4
```

2. **Verify Caches Loaded:**
```
.playerbot gear status
# Should show: "Cache loaded: 45,283 items"

.playerbot talents status
# Should show: "Loadouts loaded: 127"
```

If caches not loaded, every bot creation hits database!

3. **Profile Performance:**
```bash
# Linux: Use perf
perf record -g -p $(pidof worldserver) sleep 30
perf report

# Look for hotspots:
# - BotGearFactory::SelectItem (should be <1%)
# - Database queries (should be 0% if cached)
```

4. **Optimize Throttling:**
```conf
# Gradual rollout
Day 1: Playerbot.LevelManager.MaxBotsPerUpdate = 5
Day 2: Playerbot.LevelManager.MaxBotsPerUpdate = 10
Day 3: Playerbot.LevelManager.MaxBotsPerUpdate = 20
```

**Expected After Fix:**
- CPU: <10% worldserver
- TPS: 48-50 stable
- No lag spikes

---

### Q11: Memory usage growing unbounded

**Symptoms:**
```
Memory: 12 GB worldserver (started at 2 GB)
Growing by 50 MB/minute
OOM killer triggered after 6 hours
```

**Causes:**
- Tasks not being cleaned up
- Gear sets not released
- Memory leak in task queue

**Solutions:**

1. **Check Task Cleanup:**
```cpp
// Verify tasks are released after processing
uint64 taskId = CreateBotAsync(bot);

// After processing, task should be destroyed
// Check with:
.playerbot stats

// Output should show:
// Tasks Submitted: 5,000
// Tasks Completed: 5,000
// Tasks In Queue: 0  // <-- Should be 0 after processing!
```

2. **Monitor Memory Growth:**
```bash
# Track worldserver memory
watch -n 5 'ps aux | grep worldserver | awk "{print \$6}"'
```

3. **Check for Leaks:**
```bash
# Run with valgrind (debug build)
valgrind --leak-check=full --track-origins=yes ./worldserver

# Look for:
# - "definitely lost" (critical leaks)
# - "possibly lost" (investigate)
```

4. **Verify Shared Pointer Cleanup:**
```cpp
// Task should auto-delete when last reference goes away
std::shared_ptr<BotCreationTask> task = DequeueTask();
// Process task
// ... task destructor called automatically when out of scope
```

**Expected Memory Profile:**
- Startup: ~2 GB (caches loaded)
- After 1,000 bots: ~2.05 GB (+50 MB)
- After 5,000 bots: ~2.25 GB (+250 MB)
- Stable: No growth after all bots created

---

## Distribution Imbalance

### Q12: All bots spawning at level 80

**Symptoms:**
```sql
SELECT level, COUNT(*) FROM characters WHERE account > 1 GROUP BY level;
-- Result: 4,873 bots at level 80, 127 at other levels
```

**Causes:**
- L80 bracket percentage set too high (e.g., 50%)
- Weighted selection favoring L80 excessively
- Bug in bracket selection algorithm

**Solutions:**

1. **Verify Configuration:**
```conf
# L80 should be ~8% (not 50%!)
Playerbot.LevelDistribution.Alliance.Bracket.L80 = 8.0
Playerbot.LevelDistribution.Horde.Bracket.L80 = 8.0
```

2. **Check Selection Weights:**
```cpp
// Debug log the selection
.playerbot distribution debug 1

// Output should show:
// Bracket L1-4: Priority=2.1 (underpopulated)
// Bracket L80: Priority=0.01 (overpopulated)
```

3. **Force Rebalance:**
```cpp
// Delete excess L80 bots or manually reassign levels
UPDATE characters
SET level = FLOOR(1 + RAND() * 79)
WHERE account > 1 AND level = 80
LIMIT 4000;  -- Reduce from 4,873 to 873
```

4. **Reset Statistics:**
```cpp
.playerbot distribution reset

// Recalculates all bracket counts from database
```

**Expected Distribution After Fix:**
```sql
SELECT
    FLOOR(level / 5) * 5 as bracket,
    COUNT(*) as count,
    ROUND(COUNT(*) * 100.0 / 5000, 2) as percent,
    ROUND(target_percent, 2) as target
FROM characters c
JOIN bracket_config b ON FLOOR(c.level / 5) = b.bracket_id
WHERE c.account > 1
GROUP BY bracket
ORDER BY bracket;

-- Each bracket within ±15% of target
```

---

## Gear Generation Issues

### Q13: Bots spawning with no gear

**Symptoms:**
- Bots created but inventory empty
- No items equipped
- Console shows "Gear set is empty"

**Causes:**
- GearFactory cache not initialized
- Item database empty or corrupt
- Gear application failed silently

**Solutions:**

1. **Verify Cache:**
```
.playerbot gear status

Expected:
✅ GearFactory initialized
✅ Cache loaded: 45,283 items
✅ All classes/specs covered
```

2. **Test Gear Generation:**
```
.playerbot gear test 1 0 20 0

Expected output:
Generated gear set for class 1 (Warrior), spec 0 (Protection), level 20:
- Head: [Item: 12345] Green Helmet (ilvl 22)
- Chest: [Item: 12346] Green Chestplate (ilvl 23)
... (14 slots total)
```

3. **Check Item Eligibility:**
```sql
-- Verify items exist for a specific class/spec/level
SELECT
    InventoryType,
    COUNT(*) as items,
    MIN(ItemLevel) as min_ilvl,
    MAX(ItemLevel) as max_ilvl
FROM item_template
WHERE class = 4  -- Armor
  AND subclass IN (1, 2, 3, 4)  -- Cloth, Leather, Mail, Plate
  AND Quality >= 2  -- Uncommon+
  AND RequiredLevel <= 20
  AND RequiredLevel >= 10
GROUP BY InventoryType;
```

**Expected:** At least 5-10 items per slot.

4. **Enable Debug Logging:**
```conf
Playerbot.GearFactory.LogStats = 1
```

**Logs Should Show:**
```
DEBUG: Generating gear for Warrior/Protection/L20
DEBUG: Slot HEAD: Found 17 candidates, selected [12345] (ilvl 22, score 87.3)
DEBUG: Slot CHEST: Found 23 candidates, selected [12346] (ilvl 23, score 91.2)
...
DEBUG: Gear set complete: 14/14 slots filled
```

---

### Q14: Bots get inappropriate gear (cloth on warriors, etc.)

**Symptoms:**
- Warrior wearing cloth chest
- Mage wearing plate boots
- Wrong stat priorities (Intellect on Rogue)

**Causes:**
- Armor type filter not working
- Stat scoring incorrect
- Class restrictions not enforced

**Solutions:**

1. **Verify Armor Type Logic:**
```cpp
// Should filter by allowed armor types
ArmorTypes BotGearFactory::GetAllowedArmorTypes(uint8 cls)
{
    switch (cls)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
            return {ARMOR_PLATE, ARMOR_MAIL};  // Can use plate AND mail (for leveling)

        case CLASS_HUNTER:
        case CLASS_SHAMAN:
            return {ARMOR_MAIL, ARMOR_LEATHER};

        case CLASS_ROGUE:
        case CLASS_DRUID:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return {ARMOR_LEATHER};

        case CLASS_MAGE:
        case CLASS_PRIEST:
        case CLASS_WARLOCK:
            return {ARMOR_CLOTH};

        default:
            return {ARMOR_CLOTH};  // Fallback
    }
}
```

2. **Check Stat Scoring:**
```cpp
// Debug specific item scoring
.playerbot gear score 12345 1 0

Expected output:
Item [12345] score for Warrior/Protection:
- Stamina: +50 (weight 1.5) = 75.0 points
- Strength: +30 (weight 1.2) = 36.0 points
- Armor: +120 (weight 1.0) = 120.0 points
- Intellect: +10 (weight 0.0) = 0.0 points  // <-- Ignored!
Total Score: 231.0
```

3. **Update Stat Weights:**
```cpp
// In BotGearFactory::GetStatWeights()
std::map<ItemModType, float> weights;

if (role == ROLE_TANK)
{
    weights[ITEM_MOD_STAMINA] = 1.5f;
    weights[ITEM_MOD_ARMOR] = 1.0f;
    weights[ITEM_MOD_INTELLECT] = 0.0f;  // NO value for tanks!
}
```

**Expected After Fix:**
- Warriors: Plate/Mail only, Str/Sta focus
- Rogues: Leather only, Agi/Sta focus
- Mages: Cloth only, Int/Sta focus

---

## Talent Application Failures

### Q15: "Failed to apply talents - loadout not found"

**Symptoms:**
```
ERROR: No talent loadout found for class 5 (Priest), spec 2 (Shadow), level 45
WARN: Bot created without talents
```

**Causes:**
- Missing loadout in database for that class/spec/level range
- Level falls outside all defined ranges
- Spec ID mismatch

**Solutions:**

1. **Check Loadout Coverage:**
```sql
-- Find gaps in loadout coverage
SELECT
    c.class_id,
    c.spec_id,
    l.level,
    CASE
        WHEN tl.id IS NULL THEN 'MISSING'
        ELSE 'OK'
    END as status
FROM
    (SELECT DISTINCT class_id FROM playerbot_talent_loadouts) c
    CROSS JOIN (SELECT DISTINCT spec_id FROM playerbot_talent_loadouts WHERE class_id = c.class_id) s
    CROSS JOIN (SELECT 10 as level UNION SELECT 20 UNION SELECT 30 UNION SELECT 40 UNION SELECT 50 UNION SELECT 60 UNION SELECT 70 UNION SELECT 80) l
    LEFT JOIN playerbot_talent_loadouts tl
        ON tl.class_id = c.class_id
        AND tl.spec_id = s.spec_id
        AND l.level BETWEEN tl.min_level AND tl.max_level
WHERE tl.id IS NULL;
```

**Expected:** Zero missing entries. If any found, add loadouts:

```sql
-- Fill missing Priest/Shadow/L40-49 range
INSERT INTO playerbot_talent_loadouts
(class_id, spec_id, min_level, max_level, talent_string, description)
VALUES
(5, 2, 40, 49, '12345,23456,34567', 'Shadow Priest L40-49');
```

2. **Verify Spec IDs:**
```cpp
// WoW 12.0 Spec IDs (0-indexed)
// Priest: 0=Discipline, 1=Holy, 2=Shadow
// Ensure database uses same indexing!

SELECT class_id, spec_id, COUNT(*)
FROM playerbot_talent_loadouts
WHERE class_id = 5  -- Priest
GROUP BY spec_id;

-- Expected:
-- spec_id 0: 3-5 loadouts (Discipline)
-- spec_id 1: 3-5 loadouts (Holy)
-- spec_id 2: 3-5 loadouts (Shadow)
```

3. **Add Fallback Loadout:**
```sql
-- Catch-all loadout for any level
INSERT INTO playerbot_talent_loadouts
(class_id, spec_id, min_level, max_level, talent_string, description)
VALUES
(5, 2, 10, 80, '12345,23456,34567,45678', 'Shadow Priest - Universal Fallback');
```

**Expected After Fix:**
- Every class/spec has at least one loadout covering levels 10-80
- No "loadout not found" errors in logs

---

## Zone Placement Issues

### Q16: Bots teleported to incorrect zones (Horde in Stormwind, etc.)

**Symptoms:**
```
ERROR: Teleport failed - faction mismatch
WARN: Bot placed in enemy capital city
```

**Causes:**
- Zone faction filter not working
- TEAM_NEUTRAL zones used incorrectly
- Fallback zone has wrong faction

**Solutions:**

1. **Verify Zone Faction:**
```cpp
// In BotWorldPositioner::BuildDefaultZones()

// Alliance-only zones
_zones.push_back({
    12,              // zoneId: Elwynn Forest
    0,               // mapId: Eastern Kingdoms
    -8949.95f, -132.493f, 83.5312f, 0.0f,  // coordinates
    1, 10,           // minLevel, maxLevel
    TEAM_ALLIANCE,   // faction: Alliance ONLY
    "Elwynn Forest",
    true             // isStarterZone
});

// Horde-only zones
_zones.push_back({
    14,              // zoneId: Durotar
    1,               // mapId: Kalimdor
    -602.608f, -4262.17f, 38.9529f, 0.0f,
    1, 10,
    TEAM_HORDE,      // faction: Horde ONLY
    "Durotar",
    true
});

// Neutral zones (both factions)
_zones.push_back({
    3456,            // zoneId: Tanaris
    1,               // mapId: Kalimdor
    -7180.0f, -3780.0f, 8.87f, 0.0f,
    40, 50,
    TEAM_NEUTRAL,    // faction: NEUTRAL (both allowed)
    "Tanaris",
    false
});
```

2. **Check Zone Selection Logic:**
```cpp
// Should filter by faction
ZonePlacement const* BotWorldPositioner::SelectZone(uint32 level, TeamId faction)
{
    auto const& zones = GetZonesForLevel(level);

    std::vector<ZonePlacement const*> validZones;
    for (auto const& zone : zones)
    {
        // MUST check faction compatibility!
        if (zone.faction == TEAM_NEUTRAL || zone.faction == faction)
            validZones.push_back(&zone);
    }

    // Random selection from valid zones
    return validZones[rand() % validZones.size()];
}
```

3. **Test Zone Placement:**
```
.playerbot zone test 20 0  # Alliance, level 20

Expected:
Selected zone: [Westfall] (Alliance, L10-20)
Coordinates: -9449.06, 64.84, 56.36
Map: 0 (Eastern Kingdoms)

.playerbot zone test 20 1  # Horde, level 20

Expected:
Selected zone: [Silverpine Forest] (Horde, L10-20)
Coordinates: 505.12, 1504.63, 124.99
Map: 0 (Eastern Kingdoms)
```

4. **Fix Capital City Fallback:**
```cpp
// Ensure faction-appropriate capitals
Position BotWorldPositioner::GetCapitalCity(TeamId faction)
{
    if (faction == TEAM_ALLIANCE)
        return Position(-8833.38f, 628.62f, 94.00f, 1.06f);  // Stormwind
    else
        return Position(1676.21f, -4315.29f, 61.53f, 3.14f); // Orgrimmar
}
```

**Expected After Fix:**
- Alliance bots: Only Alliance/Neutral zones
- Horde bots: Only Horde/Neutral zones
- No cross-faction teleportation errors

---

## ThreadPool Integration Issues

### Q17: "ThreadPool not available" error

**Symptoms:**
```
ERROR: Cannot submit bot creation task - ThreadPool is null
FATAL: Automated World Population System disabled
```

**Causes:**
- TrinityCore version doesn't have ThreadPool
- ThreadPool disabled in config
- System initialized before World

**Solutions:**

1. **Check TrinityCore Version:**
```bash
# ThreadPool introduced in TrinityCore commit abc123 (2023-05-15)
git log --grep="ThreadPool" --oneline

# Ensure you're on a version with ThreadPool support
git log --oneline | head -20
```

2. **Verify ThreadPool Enabled:**
```conf
# In worldserver.conf
ThreadPool.Enabled = 1
ThreadPool.NumThreads = 4  # 2-8 recommended
```

3. **Check Initialization Order:**
```cpp
// WRONG: BotLevelManager initialized before World
sBotLevelManager->Initialize();  // ThreadPool not ready yet!
sWorld->Initialize();

// CORRECT: BotLevelManager initialized after World
sWorld->Initialize();
sBotLevelManager->Initialize();  // ThreadPool now available
```

4. **Fallback to Synchronous:**
```cpp
// Detect ThreadPool availability
if (!sWorld->GetThreadPool())
{
    LOG_WARN("ThreadPool not available, using synchronous mode");
    _useSynchronousMode = true;
}

// In CreateBotAsync()
if (_useSynchronousMode)
{
    // Process immediately on calling thread
    PrepareBot_WorkerThread(task);
    ApplyBot_MainThread(task.get());
}
else
{
    // Submit to ThreadPool (preferred)
    sWorld->GetThreadPool()->PostWork([this, task]() {
        PrepareBot_WorkerThread(task);
    });
}
```

**Expected After Fix:**
- ThreadPool available and accepting tasks
- Asynchronous processing working
- No synchronous fallback needed

---

## Debugging Techniques

### Debug Console Commands

```
# System Status
.playerbot status                  # Overall system health
.playerbot stats                   # Detailed statistics

# Subsystem Status
.playerbot distribution status     # Level distribution
.playerbot gear status             # Gear factory cache
.playerbot talents status          # Talent manager loadouts
.playerbot zones status            # World positioner zones

# Testing
.playerbot distribution test 0     # Test Alliance bracket selection
.playerbot gear test 1 0 20 0      # Test Warrior/Protection/L20 gear
.playerbot talents test 5 2 45     # Test Priest/Shadow/L45 talents
.playerbot zone test 60 1          # Test Horde L60 zone selection

# Queue Management
.playerbot queue status            # Current queue size
.playerbot queue drain             # Force process all tasks
.playerbot queue clear             # Clear pending tasks (emergency)

# Debug Logging
.playerbot debug 1                 # Enable verbose logging
.playerbot debug 0                 # Disable verbose logging
```

### SQL Debugging Queries

```sql
-- Bot distribution by level
SELECT
    FLOOR(level / 5) * 5 as bracket,
    COUNT(*) as count,
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM characters WHERE account > 1), 2) as percent
FROM characters
WHERE account > 1
GROUP BY bracket
ORDER BY bracket;

-- Bot distribution by faction
SELECT
    CASE WHEN race IN (1,3,4,7,11,22,25,29) THEN 'Alliance' ELSE 'Horde' END as faction,
    COUNT(*) as count
FROM characters
WHERE account > 1
GROUP BY faction;

-- Bots with missing gear (empty inventory)
SELECT c.guid, c.name, c.level, c.class
FROM characters c
LEFT JOIN character_inventory ci ON ci.guid = c.guid
WHERE c.account > 1
  AND ci.guid IS NULL
LIMIT 100;

-- Bots with incomplete talents
SELECT c.guid, c.name, c.level, c.class, COUNT(ct.spell) as talent_count
FROM characters c
LEFT JOIN character_talent ct ON ct.guid = c.guid
WHERE c.account > 1
  AND c.level >= 10
GROUP BY c.guid
HAVING talent_count < 5
LIMIT 100;

-- Zone distribution
SELECT
    FLOOR(position_x / 1000) * 1000 as x_sector,
    FLOOR(position_y / 1000) * 1000 as y_sector,
    map,
    COUNT(*) as bots
FROM characters
WHERE account > 1
GROUP BY x_sector, y_sector, map
ORDER BY bots DESC
LIMIT 50;
```

### Performance Profiling

**Linux:**
```bash
# CPU profiling
perf record -g -p $(pidof worldserver) sleep 30
perf report

# Memory profiling
valgrind --tool=massif --time-unit=B ./worldserver
ms_print massif.out.*

# System call tracing
strace -c -p $(pidof worldserver)
```

**Windows:**
```powershell
# Visual Studio Profiler
devenv /DebugExe worldserver.exe

# Performance Monitor
perfmon /res
# Add counters: Process\Private Bytes, Process\% Processor Time

# Process Explorer
procexp64.exe
# Monitor worldserver.exe threads, handles, memory
```

---

## Error Code Reference

### BotLevelDistribution Errors

- `LEVEL_DIST_001`: Configuration file not found
- `LEVEL_DIST_002`: Invalid bracket percentages (don't sum to 100%)
- `LEVEL_DIST_003`: Bracket selection failed (all brackets overpopulated)
- `LEVEL_DIST_004`: Invalid faction ID (must be 0=Alliance, 1=Horde)

### BotGearFactory Errors

- `GEAR_FACTORY_001`: Cache initialization failed (no items loaded)
- `GEAR_FACTORY_002`: No items found for class/spec/level combination
- `GEAR_FACTORY_003`: Item quality distribution error (percentages don't sum to 100%)
- `GEAR_FACTORY_004`: Invalid item entry (not found in item_template)
- `GEAR_FACTORY_005`: Gear set incomplete (<6 items equipped)

### BotTalentManager Errors

- `TALENT_MGR_001`: Database table `playerbot_talent_loadouts` not found
- `TALENT_MGR_002`: No loadouts loaded (empty table)
- `TALENT_MGR_003`: Loadout not found for class/spec/level
- `TALENT_MGR_004`: Invalid talent entry ID
- `TALENT_MGR_005`: Dual spec setup failed (level < 10)
- `TALENT_MGR_006`: Hero talent application failed (level < 71)

### BotWorldPositioner Errors

- `WORLD_POS_001`: No zones defined (empty zone list)
- `WORLD_POS_002`: Zone not found for level/faction combination
- `WORLD_POS_003`: Teleportation failed (invalid coordinates)
- `WORLD_POS_004`: Faction mismatch (bot faction != zone faction)
- `WORLD_POS_005`: Invalid map ID

### BotLevelManager Errors

- `LEVEL_MGR_001`: ThreadPool not available (null pointer)
- `LEVEL_MGR_002`: Task submission failed (queue full)
- `LEVEL_MGR_003`: Bot not found (invalid GUID)
- `LEVEL_MGR_004`: Task timeout (>5 minutes in queue)
- `LEVEL_MGR_005`: Subsystem not initialized (dependency missing)

---

## Contact & Support

**For Additional Help:**

- **GitHub Issues**: Report bugs and performance issues
- **Development Team**: Contact for critical production issues
- **Documentation**: See API_REFERENCE.md and DEPLOYMENT_GUIDE.md
- **Community Forums**: Share troubleshooting experiences

---

**Last Updated:** 2025-01-18
**Version:** 1.0.0
**Compatible With:** TrinityCore WoW 12.0 (The War Within)
