# ObjectAccessor → SpatialGrid Migration Plan
## Comprehensive Replacement Strategy for 100+ Map/ObjectAccessor Calls

**Created**: 2025-10-21
**Scope**: Entire PlayerBot module (src/modules/Playerbot/)
**Goal**: Eliminate ALL Map/ObjectAccessor calls from worker threads to prevent deadlocks

---

## 📊 AUDIT SUMMARY

### Total Findings
- **100+ ObjectAccessor calls** across PlayerBot module
- **1 commented Map::FindMap call** (already disabled)
- **0 active direct Map access** (excellent!)

### Call Distribution by Type
| Method | Count | Risk Level |
|--------|-------|------------|
| `ObjectAccessor::GetUnit()` | ~70 | 🔴 HIGH (worker thread) |
| `ObjectAccessor::GetCreature()` | ~25 | 🔴 HIGH (worker thread) |
| `ObjectAccessor::GetPlayer()` | ~3 | 🟡 MEDIUM (mixed context) |
| `ObjectAccessor::GetGameObject()` | ~3 | 🟡 MEDIUM (quest/loot) |
| `ObjectAccessor::GetDynamicObject()` | ~8 | 🟡 MEDIUM (dungeon scripts) |

---

## 🎯 PRIORITY CLASSIFICATION

### Priority 1: CRITICAL - Worker Thread Context (DEADLOCK SOURCES)

These files are called from BotAI::Update() worker thread loop:

| File | Calls | Pattern | Replacement Required |
|------|-------|---------|----------------------|
| `AI/Combat/BotThreatManager.cpp` | 8 | GetUnit from threat GUID | Query creature snapshots by GUID |
| `AI/Combat/TargetSelector.cpp` | 4 | GetUnit from nearby GUIDs | Query creature snapshots in range |
| `AI/CombatBehaviors/InterruptRotationManager.cpp` | 5 | GetUnit for cast validation | Query creature snapshots by GUID |
| `AI/CombatBehaviors/AoEDecisionManager.cpp` | 3 | GetUnit for AoE target validation | Query creature snapshots in range |
| `AI/CombatBehaviors/DefensiveBehaviorManager.cpp` | 3 | GetUnit for defensive target | Query player/creature snapshots |
| `AI/CombatBehaviors/DispelCoordinator.cpp` | 3 | GetUnit for dispel target | Query creature snapshots by GUID |
| `AI/Combat/PositionManager.cpp` | 1 | GetUnit for positioning | Query creature snapshots in range |
| `AI/Combat/ObstacleAvoidanceManager.cpp` | 2 | GetUnit + GetDynamicObject | Query creature + dynamicObject snapshots |

**Total Priority 1**: ~29 calls

### Priority 2: HIGH - Main Thread with Worker Thread Risk

These may be called from either thread context:

| File | Calls | Pattern | Replacement Required |
|------|-------|---------|----------------------|
| `Advanced/AdvancedBehaviorManager.cpp` | 7 | GetCreature for behavior logic | Query creature snapshots by GUID |
| `Advanced/GroupCoordinator.cpp` | 2 | GetUnit for group target | Query player snapshots by GUID |
| `AI/EnhancedBotAI.cpp` | 3 | GetPlayer for follow target | Query player snapshots by GUID |

**Total Priority 2**: ~12 calls

### Priority 3: MEDIUM - Main Thread Context (Lower Risk)

These are typically called from main thread (NPC interaction, quest, loot):

| File | Calls | Pattern | Replacement Required |
|------|-------|---------|----------------------|
| `Game/NPCInteractionManager.cpp` | 14 | GetCreature for NPC interaction | Query creature snapshots, validate on main thread |
| `Game/QuestManager.cpp` | 1 | GetCreature for quest NPC | Query creature snapshots |
| `Game/InventoryManager.cpp` | 3 | GetCreature/GameObject for vendors | Query snapshots, validate on main thread |
| `AI/Strategy/QuestStrategy.cpp` | 3 | GetGameObject/Unit for quest | Query snapshots |
| `AI/Strategy/LootStrategy.cpp` | 2 | GetCreature/GameObject for loot | Query snapshots |
| `Lifecycle/DeathRecoveryManager.cpp` | 6 | GetCreature for spirit healer | Query creature snapshots |
| `Interaction/Core/InteractionManager.cpp` | 2 | GetCreature for interaction | Query creature snapshots |

**Total Priority 3**: ~31 calls

### Priority 4: LOW - Dungeon Scripts (Instance-Specific)

| File | Calls | Pattern | Replacement Required |
|------|-------|---------|----------------------|
| `Dungeon/Scripts/Vanilla/*.cpp` | 8 | GetDynamicObject for mechanic avoidance | Query dynamicObject snapshots |
| `Dungeon/DungeonBehavior.cpp` | 4 | GetCreature for boss mechanics | Query creature snapshots |
| `Dungeon/EncounterStrategy.cpp` | 1 | GetDynamicObject for encounter | Query dynamicObject snapshots |

**Total Priority 4**: ~13 calls

### Priority 5: DOCUMENTATION/TESTING (No Code Changes)

| File | Calls | Type | Action |
|------|-------|------|--------|
| `Events/EVENT_SYSTEM_DOCUMENTATION.md` | 6 | Documentation examples | Update examples to use spatial grid |
| `PHASE4_EVENT_INTEGRATION_PLAN.md` | 2 | Planning docs | Update migration plan |
| `AI/Combat/ThreatIntegrationExample.h` | 1 | Example code | Update to spatial grid pattern |
| `Network/TYPED_PACKET_HOOKS_MIGRATION.md` | 1 | Migration guide | Update to spatial grid pattern |

**Total Priority 5**: ~10 calls (documentation only)

### Priority 6: CLASS-SPECIFIC AI (Medium Risk)

| File | Calls | Pattern | Replacement Required |
|------|-------|---------|----------------------|
| `AI/ClassAI/Warriors/WarriorSpecialization.cpp` | 3 | GetCreature for warrior mechanics | Query creature snapshots |
| `AI/ClassAI/Warriors/ProtectionSpecialization.cpp` | 4 | GetCreature for tank targeting | Query creature snapshots |
| `AI/ClassAI/Warriors/FurySpecialization.cpp` | 1 | GetCreature for DPS targeting | Query creature snapshots |
| `AI/ClassAI/Warriors/WarriorAI.cpp` | 1 | GetCreature for AI logic | Query creature snapshots |

**Total Priority 6**: ~9 calls

---

## 🔧 REPLACEMENT PATTERNS

### Pattern 1: GetUnit() for GUID Validation (Most Common - ~70 uses)

**OLD CODE** (DEADLOCK RISK):
```cpp
Unit* target = ObjectAccessor::GetUnit(*_bot, guid);
if (!target || !target->IsAlive())
    continue;

// Use target->GetHealth(), target->GetDistance(), etc.
```

**NEW CODE** (THREAD-SAFE):
```cpp
// Step 1: Query spatial grid for creature snapshot
auto spatialGrid = sSpatialGridManager.GetGrid(_bot->GetMapId());
if (!spatialGrid)
    continue;

auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(_bot->GetPosition(), 100.0f);

// Step 2: Find snapshot matching GUID
const DoubleBufferedSpatialGrid::CreatureSnapshot* snapshot = nullptr;
for (auto const& snap : creatureSnapshots)
{
    if (snap.guid == guid)
    {
        snapshot = &snap;
        break;
    }
}

if (!snapshot || !snapshot->IsAlive())
    continue;

// Step 3: Use snapshot data (NO Map access)
uint64 health = snapshot->health;
float distance = _bot->GetDistance(snapshot->position);
// All data available in snapshot!
```

### Pattern 2: GetCreature() for NPC Interaction (Main Thread OK)

**OLD CODE**:
```cpp
Creature* creature = ObjectAccessor::GetCreature(*m_bot, guid);
if (!creature)
    return nullptr;

return creature;
```

**NEW CODE** (HYBRID - Snapshot for validation, ObjectAccessor only on main thread):
```cpp
// Step 1: Validate using spatial grid snapshot (thread-safe)
auto spatialGrid = sSpatialGridManager.GetGrid(m_bot->GetMapId());
if (!spatialGrid)
    return nullptr;

auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(m_bot->GetPosition(), 100.0f);

const DoubleBufferedSpatialGrid::CreatureSnapshot* snapshot = nullptr;
for (auto const& snap : creatureSnapshots)
{
    if (snap.guid == guid)
    {
        snapshot = &snap;
        break;
    }
}

if (!snapshot || !snapshot->IsValid())
    return nullptr;

// Step 2: If on main thread and need actual Creature*, use ObjectAccessor
// ONLY do this if you're certain you're on main thread (e.g., NPC interaction manager)
if (sScriptMgr->IsMainThread())
{
    return ObjectAccessor::GetCreature(*m_bot, guid);
}

// Step 3: Otherwise, use GUID for SetTarget() or other GUID-based operations
return nullptr; // Can't return Creature* from worker thread
```

### Pattern 3: GetPlayer() for Group/Follow Logic

**OLD CODE**:
```cpp
if (Player* leader = ObjectAccessor::GetPlayer(*GetBot(), _followTarget))
{
    float distance = GetBot()->GetDistance(leader);
    // ...
}
```

**NEW CODE** (THREAD-SAFE):
```cpp
auto spatialGrid = sSpatialGridManager.GetGrid(GetBot()->GetMapId());
if (!spatialGrid)
    return;

auto playerSnapshots = spatialGrid->QueryNearbyPlayers(GetBot()->GetPosition(), 100.0f);

const DoubleBufferedSpatialGrid::PlayerSnapshot* leaderSnapshot = nullptr;
for (auto const& snap : playerSnapshots)
{
    if (snap.guid == _followTarget)
    {
        leaderSnapshot = &snap;
        break;
    }
}

if (!leaderSnapshot || !leaderSnapshot->isAlive)
    return;

float distance = GetBot()->GetDistance(leaderSnapshot->position);
// All data available in snapshot!
```

### Pattern 4: GetGameObject() for Quest/Loot

**OLD CODE**:
```cpp
GameObject* object = ObjectAccessor::GetGameObject(*bot, objectGuid);
if (!object || !object->isSpawned())
    return false;
```

**NEW CODE** (THREAD-SAFE):
```cpp
auto spatialGrid = sSpatialGridManager.GetGrid(bot->GetMapId());
if (!spatialGrid)
    return false;

auto gameObjectSnapshots = spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), 100.0f);

const DoubleBufferedSpatialGrid::GameObjectSnapshot* snapshot = nullptr;
for (auto const& snap : gameObjectSnapshots)
{
    if (snap.guid == objectGuid)
    {
        snapshot = &snap;
        break;
    }
}

if (!snapshot || !snapshot->isSpawned)
    return false;

// Use snapshot data
```

### Pattern 5: GetDynamicObject() for Danger Avoidance

**OLD CODE**:
```cpp
DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*player, guid);
if (!dynObj)
    continue;

if (dynObj->GetRadius() > 0.0f)
{
    // Avoid dangerous AoE
}
```

**NEW CODE** (THREAD-SAFE):
```cpp
auto spatialGrid = sSpatialGridManager.GetGrid(player->GetMapId());
if (!spatialGrid)
    continue;

auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(player->GetPosition(), 100.0f);

const DoubleBufferedSpatialGrid::DynamicObjectSnapshot* snapshot = nullptr;
for (auto const& snap : dynamicObjectSnapshots)
{
    if (snap.guid == guid)
    {
        snapshot = &snap;
        break;
    }
}

if (!snapshot)
    continue;

if (snapshot->radius > 0.0f && snapshot->isDangerous)
{
    // Avoid dangerous AoE using snapshot data
}
```

---

## 🏗️ IMPLEMENTATION STRATEGY

### Phase 5A: Create Helper Utility Class

Create `src/modules/Playerbot/Spatial/SpatialGridQueryHelpers.h/cpp`:

```cpp
namespace Playerbot
{
    class SpatialGridQueryHelpers
    {
    public:
        /**
         * @brief Find creature snapshot by GUID (thread-safe)
         * @return Pointer to snapshot if found, nullptr otherwise
         */
        static DoubleBufferedSpatialGrid::CreatureSnapshot const*
        FindCreatureByGuid(Player* bot, ObjectGuid guid);

        /**
         * @brief Find player snapshot by GUID (thread-safe)
         */
        static DoubleBufferedSpatialGrid::PlayerSnapshot const*
        FindPlayerByGuid(Player* bot, ObjectGuid guid);

        /**
         * @brief Find GameObject snapshot by GUID (thread-safe)
         */
        static DoubleBufferedSpatialGrid::GameObjectSnapshot const*
        FindGameObjectByGuid(Player* bot, ObjectGuid guid);

        /**
         * @brief Find DynamicObject snapshot by GUID (thread-safe)
         */
        static DoubleBufferedSpatialGrid::DynamicObjectSnapshot const*
        FindDynamicObjectByGuid(Player* bot, ObjectGuid guid);

        /**
         * @brief Validate if creature exists and matches criteria (thread-safe)
         */
        static bool ValidateCreature(Player* bot, ObjectGuid guid,
            bool requireAlive = true, bool requireHostile = false);

        /**
         * @brief Get distance to entity by GUID (thread-safe)
         */
        static float GetDistanceToEntity(Player* bot, ObjectGuid guid);
    };
}
```

### Phase 5B: Replace Priority 1 Files (CRITICAL - Worker Thread)

1. **BotThreatManager.cpp** (8 calls)
   - Replace all `ObjectAccessor::GetUnit()` in threat analysis loop
   - Use `FindCreatureByGuid()` helper
   - Validate using snapshot data only

2. **TargetSelector.cpp** (4 calls)
   - Replace GetUnit() in GetNearbyEnemies()
   - Use spatial grid QueryNearbyCreatures() directly
   - Filter using snapshot data

3. **InterruptRotationManager.cpp** (5 calls)
   - Replace GetUnit() for cast validation
   - Use snapshot spell/cast data

4. **AoEDecisionManager.cpp** (3 calls)
   - Replace GetUnit() for AoE target validation
   - Use snapshot position/health data

5. **DefensiveBehaviorManager.cpp** (3 calls)
   - Replace GetUnit() for defensive target validation
   - Use snapshot health/position data

6. **DispelCoordinator.cpp** (3 calls)
   - Replace GetUnit() for dispel target validation
   - Use snapshot aura/debuff data (if available)

7. **PositionManager.cpp** (1 call)
   - Replace GetUnit() for positioning logic
   - Use snapshot position data

8. **ObstacleAvoidanceManager.cpp** (2 calls)
   - Replace GetUnit() and GetDynamicObject()
   - Use creature + dynamicObject snapshots

### Phase 5C: Replace Priority 2 Files (HIGH - Mixed Context)

9. **AdvancedBehaviorManager.cpp** (7 calls)
10. **GroupCoordinator.cpp** (2 calls)
11. **EnhancedBotAI.cpp** (3 calls)

### Phase 5D: Replace Priority 3 Files (MEDIUM - Main Thread)

12-18. NPCInteractionManager, QuestManager, InventoryManager, QuestStrategy, LootStrategy, DeathRecoveryManager, InteractionManager

### Phase 5E: Replace Priority 4 Files (LOW - Dungeon Scripts)

19-21. Dungeon scripts and encounter strategies

### Phase 5F: Replace Priority 6 Files (Class-Specific AI)

22-25. Warrior specializations and other class AIs

### Phase 5G: Update Documentation (Priority 5)

26. Update all documentation to reflect spatial grid patterns

---

## ✅ SUCCESS CRITERIA

### Build Validation
- ✅ Zero compilation errors
- ✅ Zero warnings related to ObjectAccessor usage
- ✅ All tests pass

### Runtime Validation
- ✅ Zero deadlocks with 100 concurrent bots
- ✅ Zero deadlocks with 1000 concurrent bots
- ✅ Zero deadlocks with 5000 concurrent bots
- ✅ No performance degradation (CPU/memory)

### Code Quality
- ✅ All ObjectAccessor calls replaced (except main thread-only validated cases)
- ✅ No Map access from worker threads
- ✅ All spatial grid queries use lock-free snapshot pattern
- ✅ Documentation updated

---

## 📈 ESTIMATED EFFORT

| Phase | Files | Calls | Estimated Time |
|-------|-------|-------|----------------|
| 5A: Helper Utilities | 1 | - | 2 hours |
| 5B: Priority 1 (Worker Thread) | 8 | 29 | 8 hours |
| 5C: Priority 2 (Mixed Context) | 3 | 12 | 3 hours |
| 5D: Priority 3 (Main Thread) | 7 | 31 | 6 hours |
| 5E: Priority 4 (Dungeon Scripts) | 3 | 13 | 3 hours |
| 5F: Priority 6 (Class AI) | 4 | 9 | 2 hours |
| 5G: Documentation Updates | 4 | 10 | 1 hour |
| **TOTAL** | **30 files** | **104 calls** | **25 hours** |

---

## 🎯 NEXT STEPS

1. ✅ **Review and approve this plan** (User approval required)
2. ⏭️ **Implement Phase 5A** - Create SpatialGridQueryHelpers utility class
3. ⏭️ **Implement Phase 5B** - Replace Priority 1 files (CRITICAL)
4. ⏭️ **Build and test after each file** - Ensure no regressions
5. ⏭️ **Continue with Priorities 2-6** - Systematic replacement
6. ⏭️ **Final validation** - 5000 bot deadlock test
7. ⏭️ **Commit and document** - Comprehensive commit message

---

**Ready to proceed with Phase 5A (Helper Utilities)?**
