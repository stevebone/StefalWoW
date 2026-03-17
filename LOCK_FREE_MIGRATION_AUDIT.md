# LOCK-FREE MIGRATION AUDIT REPORT
**Generated**: October 28, 2025
**Objective**: Complete the partially-finished lock-free refactoring to enable Option 5 safely

---

## EXECUTIVE SUMMARY

**Total Issues Found**: 175 unsafe code locations
- **135 ObjectAccessor calls** from worker threads (race condition with Option 5)
- **40 Cell::Visit calls** with "DEADLOCK FIX" comments (unmigrated code)

**Affected Files**: 32 files across AI subsystems
- Class AI: 13 files (all 13 classes)
- Combat: 10 files
- Actions: 3 files
- Strategies: 3 files
- Event Handlers: 3 files

**Migration Status**:
- ✅ **Fully Migrated**: 2 files (QuestCompletion_LockFree.cpp, GatheringManager_LockFree.cpp)
- ⚠️ **Partially Migrated**: 20 files (spatial grid + unsafe fallbacks)
- ❌ **Unmigrated**: 12 files (still using Cell visitors with TODO comments)

---

## CATEGORY 1: UNMIGRATED CODE (40 locations)

### Pattern: "DEADLOCK FIX" Comments Without Implementation

**Signature**: Comment says "Use lock-free spatial grid" but code still uses `Cell::VisitGridObjects`

### Files (12 total):

#### 1. **CombatSpecializationBase.cpp** (2 locations)
- Line 424: `GetNearbyEnemies()` - Still uses Cell visitor
- Line 474: `GetNearbyAllies()` - Still uses Cell visitor

#### 2. **Class AI Files** (13 classes × 1-3 locations each = 26 locations)
- **DeathKnightAI.cpp** (line 1561)
- **DemonHunterAI.cpp** (lines 1084, 1137)
- **EvokerAI.cpp** (line 949)
- **HunterAI.cpp** (lines 1142, 1365, 1423)
- **MonkAI.cpp** (lines 958, 1025, 1088)
- **PaladinAI.cpp** (lines 916, 975)
- **RogueAI.cpp** (line 700)
- **ShamanAI.cpp** (lines 754, 1319, 1822, 1951, 2051)
- **WarlockAI.cpp** (lines 956, 1019)
- **WarriorAI.cpp** (line 581)

#### 3. **Combat Systems** (9 locations)
- **InterruptAwareness.cpp** (line 710)
- **KitingManager.cpp** (line 74)
- **LineOfSightManager.cpp** (lines 316, 358, 575, 623)
- **ObstacleAvoidanceManager.cpp** (lines 345, 874, 1017)
- **TargetScanner.cpp** (line 278)

#### 4. **Other** (3 locations)
- **Action.cpp** (line 207)
- **DispelCoordinator.cpp** (line 954)
- **LootStrategy.cpp** (line 244)

### Migration Pattern for Unmigrated Code:

**BEFORE**:
```cpp
std::vector<::Unit*> CombatSpecializationBase::GetNearbyEnemies(float range) const
{
    std::vector<::Unit*> enemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck checker(_bot, range);
    Trinity::UnitListSearcher searcher(_bot, enemies, checker);
    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Cell::VisitGridObjects(_bot, searcher, range);  // ❌ NOT FIXED!
    return enemies;
}
```

**AFTER**:
```cpp
std::vector<ObjectGuid> CombatSpecializationBase::GetNearbyEnemyGuids(float range) const
{
    std::vector<ObjectGuid> enemyGuids;

    auto grid = sSpatialGridManager.GetGrid(_bot->GetMap());
    if (!grid) return enemyGuids;

    auto creatures = grid->QueryNearbyCreatures(_bot->GetPosition(), range);
    for (auto const& snapshot : creatures) {
        if (snapshot.isAlive && snapshot.isHostile)
            enemyGuids.push_back(snapshot.guid);
    }
    return enemyGuids;  // ✅ GUIDs only, no pointers!
}
```

---

## CATEGORY 2: PARTIALLY MIGRATED CODE (135 locations)

### Pattern: Spatial Grid First, Then ObjectAccessor Fallback

**Signature**: Code queries spatial grid BUT then calls `ObjectAccessor::GetUnit/GetCreature()` to get pointers

### High-Priority Files (50+ ObjectAccessor calls):

#### 1. **ShamanAI.cpp** (19 calls)
Lines: 781, 787, 1114, 1147, 1346, 1352, 1469, 1620, 1811, 1849, 1855, 1876, 1978, 1984, 2001, 2078, 2084, 2673

#### 2. **MonkAI.cpp** (6 calls)
Lines: 985, 991, 1052, 1058, 1115, 1121

#### 3. **PaladinAI.cpp** (4 calls)
Lines: 943, 949, 1002, 1008

#### 4. **HunterAI.cpp** (5 calls)
Lines: 1168, 1391, 1449, 1551, 1552

### Medium-Priority Files (10-20 calls):

#### 5. **Combat Systems** (~40 calls total)
- **BotThreatManager.cpp** (9 calls)
- **TargetSelector.cpp** (8 calls)
- **DispelCoordinator.cpp** (3 calls)
- **AoEDecisionManager.cpp** (4 calls)
- **DefensiveBehaviorManager.cpp** (3 calls)
- **InterruptRotationManager.cpp** (6 calls)
- **CombatStateAnalyzer.cpp** (1 call)
- **ObstacleAvoidanceManager.cpp** (2 calls)
- **PositionManager.cpp** (1 call)

#### 6. **Event Handlers** (6 calls)
- **BotAI_EventHandlers.cpp** (lines 183, 209, 233, 248, 271, 530)

### Migration Pattern for Fallback Code:

**BEFORE**:
```cpp
// Spatial grid first (good)
auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
if (!snapshot || !snapshot->IsAlive())
    continue;

// UNSAFE FALLBACK (worker thread!)
Unit* target = ObjectAccessor::GetUnit(*_bot, guid);  // ❌ RACE CONDITION!
if (target)
{
    bot->CastSpell(target, spellId);  // ❌ Direct manipulation!
    AnalyzeThreat(target);
}
```

**AFTER**:
```cpp
// Snapshot validation only
auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
if (!snapshot || !snapshot->isAlive)
    continue;

// Queue action with GUID (no pointer!)
sBotActionMgr->QueueAction(
    BotAction::CastSpell(botGuid, spellId, snapshot->guid, timestamp));

// Analyze using snapshot data (no pointer!)
AnalyzeThreatFromSnapshot(*snapshot);
```

---

## CATEGORY 3: FULLY MIGRATED (Reference Implementations)

### ✅ Good Examples to Copy:

#### 1. **QuestCompletion_LockFree.cpp** (461 lines)
Perfect reference for:
- Snapshot-only validation
- Action queue usage
- NO ObjectAccessor calls
- NO pointer dereferencing

#### 2. **GatheringManager_LockFree.cpp**
Additional example showing profession integration

#### 3. **TargetScanner.cpp** (ScanForHostileTargets method)
- Lines 332-356: Returns GUIDs only
- Comment at line 348: "NO ObjectAccessor::GetUnit() call → THREAD-SAFE!"
- **This is the CORRECT pattern**

---

## PRIORITY MIGRATION ORDER

### Phase 1: Remove Fallbacks (Highest Risk)
**Priority 1 - Combat-Critical** (50 calls):
1. BotThreatManager.cpp (9 calls) - Threat calculation in combat
2. TargetSelector.cpp (8 calls) - Target selection
3. InterruptRotationManager.cpp (6 calls) - Interrupt coordination
4. AoEDecisionManager.cpp (4 calls) - AoE targeting

**Priority 2 - Class Abilities** (35 calls):
1. ShamanAI.cpp (19 calls)
2. MonkAI.cpp (6 calls)
3. PaladinAI.cpp (4 calls)
4. HunterAI.cpp (5 calls)

**Priority 3 - Defensive/Support** (20 calls):
1. DefensiveBehaviorManager.cpp (3 calls)
2. DispelCoordinator.cpp (3 calls)
3. CombatStateAnalyzer.cpp (1 call)

**Priority 4 - Other** (30 calls):
Event handlers, movement, positioning, etc.

### Phase 2: Complete Unmigrated (Medium Risk)
**All 12 files with "DEADLOCK FIX" comments**

Order by dependency:
1. CombatSpecializationBase.cpp - Base class for all classes
2. All 13 class AI files - Depend on base class
3. Combat systems - Support systems

---

## TESTING CHECKPOINTS

After each phase:
1. **Compile check**: Zero errors
2. **Grep check**: `grep -rn "ObjectAccessor::Get" [modified files]` → Verify removals
3. **Integration test**: 100 bots, 15 minutes, quest actions
4. **Crash monitoring**: Check for Spell.cpp:603 crashes

Final validation:
1. **Complete audit**: Zero unsafe ObjectAccessor calls from worker threads
2. **Load test**: 5000 bots, 1 hour
3. **ThreadSanitizer**: Zero race condition warnings

---

## SUCCESS CRITERIA

- [ ] Zero ObjectAccessor calls from worker threads (grep verification)
- [ ] Zero Cell::Visit calls (all replaced with spatial grids)
- [ ] All decision-making uses snapshots only
- [ ] All TrinityCore API calls via BotActionQueue
- [ ] No crashes after 24 hours (5000 bots)
- [ ] Option 5 fire-and-forget stable

---

**NEXT ACTION**: Begin Phase 1 - Remove fallbacks from Priority 1 combat-critical files
