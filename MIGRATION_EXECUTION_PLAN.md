# LOCK-FREE MIGRATION EXECUTION PLAN
**Status**: IN PROGRESS - Overnight One-Shot
**Approach**: Systematic file-by-file migration, compile at end

---

## EXECUTION STRATEGY

**Phase 2**: Remove all 135 ObjectAccessor fallbacks
**Phase 3**: Complete all 40 unmigrated Cell::Visit calls
**Total**: 175 locations across 32 files

---

## COMMON MIGRATION PATTERNS

### Pattern 1: Remove ObjectAccessor Fallback (Most Common - 135 instances)

**BEFORE**:
```cpp
auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
if (!snapshot || !snapshot->IsAlive())
    continue;

Unit* target = ObjectAccessor::GetUnit(*_bot, guid);  // ❌ REMOVE
if (!target)
    continue;

DoSomethingWithPointer(target);  // ❌ Change to use snapshot
```

**AFTER**:
```cpp
auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
if (!snapshot || !snapshot->isAlive)
    continue;

// Use snapshot data directly - NO pointer needed!
DoSomethingWithSnapshot(*snapshot);  // ✅ Or queue action with GUID
```

### Pattern 2: Replace Cell::Visit (40 instances)

**BEFORE**:
```cpp
std::vector<::Unit*> GetNearbyEnemies(float range) const
{
    std::vector<::Unit*> enemies;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck checker(_bot, range);
    Trinity::UnitListSearcher searcher(_bot, enemies, checker);
    // DEADLOCK FIX: Use lock-free spatial grid ❌ NOT DONE!
    Cell::VisitGridObjects(_bot, searcher, range);
    return enemies;
}
```

**AFTER**:
```cpp
std::vector<ObjectGuid> GetNearbyEnemyGuids(float range) const
{
    std::vector<ObjectGuid> enemyGuids;

    auto grid = sSpatialGridManager.GetGrid(_bot->GetMap());
    if (!grid) return enemyGuids;

    auto creatures = grid->QueryNearbyCreatures(_bot->GetPosition(), range);
    for (auto const& snapshot : creatures) {
        if (snapshot.isAlive && snapshot.isHostile)
            enemyGuids.push_back(snapshot.guid);
    }
    return enemyGuids;  // ✅ GUIDs only
}
```

### Pattern 3: Convert Direct API Calls to Actions

**BEFORE**:
```cpp
Unit* target = ObjectAccessor::GetUnit(*_bot, guid);
if (target && target->IsAlive()) {
    _bot->CastSpell(target, spellId, false);  // ❌ Direct call
}
```

**AFTER**:
```cpp
auto snapshot = spatialGrid->QueryCreatureByGuid(guid);
if (snapshot && snapshot->isAlive) {
    sBotActionMgr->QueueAction(
        BotAction::CastSpell(_bot->GetGUID(), spellId, guid, getMSTime()));
}
```

---

## FILE-BY-FILE EXECUTION ORDER

### PRIORITY 1: Combat-Critical (4 files, 27 calls)

#### 1. BotThreatManager.cpp (9 ObjectAccessor calls)
**Lines to fix**: 273, 393, 399, 453, 544, 569, 825
**Changes**:
- Remove all `ObjectAccessor::GetUnit` fallbacks after snapshot validation
- Change ThreatTarget struct to store ObjectGuid instead of Unit*
- Update AnalyzeTargetThreat to work with snapshots
- Convert threat-based actions to action queue

**Estimated LOC**: ~50 lines changed

#### 2. TargetSelector.cpp (8 ObjectAccessor calls)
**Lines to fix**: 479, 517, 831, 862
**Changes**:
- Remove ObjectAccessor fallbacks in GetNearbyEnemies/Allies
- Change return types to std::vector<ObjectGuid>
- Update all callers

**Estimated LOC**: ~40 lines changed

#### 3. InterruptRotationManager.cpp (6 ObjectAccessor calls)
**Lines to fix**: 360, 690, 692, 875
**Changes**:
- Remove fallbacks in interrupt target validation
- Use snapshot data for interrupt decisions
- Queue interrupt actions instead of direct casts

**Estimated LOC**: ~30 lines changed

#### 4. AoEDecisionManager.cpp (4 ObjectAccessor calls)
**Lines to fix**: 238, 559, 669
**Changes**:
- Remove fallbacks in AoE target counting
- Use snapshot collections for target validation

**Estimated LOC**: ~25 lines changed

### PRIORITY 2: Class AI (35 calls across 4 files)

#### 5. ShamanAI.cpp (19 ObjectAccessor calls)
**Unmigrated Cell calls**: Lines 754, 1319, 1822, 1951, 2051
**Fallback calls**: Lines 781, 787, 1114, 1147, 1346, 1352, 1469, 1620, 1811, 1849, 1855, 1876, 1978, 1984, 2001, 2078, 2084, 2673

**Changes**:
- Replace 5 Cell::Visit calls with spatial grid queries
- Remove 14 ObjectAccessor fallbacks
- Convert spell casts to action queue

**Estimated LOC**: ~100 lines changed

#### 6-9. MonkAI.cpp, PaladinAI.cpp, HunterAI.cpp (16 calls total)
**Similar patterns to Shaman**

**Estimated LOC**: ~80 lines changed combined

### PRIORITY 3: Combat Support (20 calls across 8 files)

#### 10-17. Various combat behavior files
**DefensiveBehaviorManager, DispelCoordinator, CombatStateAnalyzer, etc.**

**Estimated LOC**: ~100 lines changed combined

### PRIORITY 4: Complete Unmigrated (remaining Cell::Visit calls)

#### 18-32. Complete all remaining files with "DEADLOCK FIX" comments

**Files**:
- CombatSpecializationBase.cpp
- All remaining class AI files
- Combat utility files
- Strategy files

**Estimated LOC**: ~150 lines changed combined

---

## TOTAL ESTIMATED CHANGES

**Files Modified**: 32
**Lines Changed**: ~575 lines
**New Lines Added**: ~200 lines (snapshot handling)
**Lines Removed**: ~375 lines (ObjectAccessor calls, Cell visitors)

---

## VERIFICATION STEPS (After All Changes)

1. **Compile**:
   ```bash
   cd build && cmake --build . --config RelWithDebInfo --target worldserver -j8
   ```

2. **Grep Verification**:
   ```bash
   # Should return 0 (except BotActionProcessor which is safe)
   grep -rn "ObjectAccessor::Get" src/modules/Playerbot/AI/ --include="*.cpp" \
     | grep -v "BotActionProcessor" | grep -v "// SAFE:" | wc -l

   # Should return 0
   grep -rn "Cell::Visit" src/modules/Playerbot/AI/ --include="*.cpp" | wc -l
   ```

3. **Fix Compilation Errors**:
   - Address any ThreatTarget struct signature changes
   - Fix method return type mismatches
   - Add missing includes for BotActionManager

---

## STARTING EXECUTION NOW

**Current Time**: Beginning overnight migration run
**Expected Completion**: All 32 files migrated, ready for compilation
