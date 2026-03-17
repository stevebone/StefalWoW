# Phase 1B/3/4 Execution Plan - Enterprise Grade Completion

**Date**: 2025-11-18
**Status**: Foundation Complete, Executing Remaining Work
**Estimated Total Remaining**: 8-12 hours

---

## Phase 1B Status: Foundation Complete ✅

**Completed**:
- ✅ ProfessionDatabase.h created (220 lines)
- ✅ ProfessionDatabase.cpp created (630 lines)
- ✅ Shared data extraction complete
- ✅ Committed and pushed (5974c52f)

**Remaining Phase 1B Work**: 4-6 hours

---

## Part 2: Refactor ProfessionManager (NEXT - 3-4 hours)

### Step 2.1: Refactor ProfessionManager.h

**Changes Required**:

```cpp
// REMOVE:
static ProfessionManager* instance();

// CHANGE Constructor:
// FROM:
ProfessionManager();  // Private

// TO:
explicit ProfessionManager(Player* bot);  // Public
~ProfessionManager();

// ADD Member:
Player* _bot;  // Non-owning pointer to bot

// CHANGE Per-Bot Data (map<playerGuid, T> → T):
// FROM:
std::unordered_map<uint32, ProfessionAutomationProfile> _playerProfiles;
std::unordered_map<uint32, std::vector<CraftingTask>> _craftingQueues;
std::unordered_map<uint32, ProfessionMetrics> _playerMetrics;
std::unordered_map<uint32, uint32> _lastUpdateTimes;

// TO:
ProfessionAutomationProfile _profile;
std::vector<CraftingTask> _craftingQueue;
ProfessionMetrics _metrics;
uint32 _lastUpdateTime{0};

// REMOVE Shared Data (now in ProfessionDatabase):
// _recipeDatabase
// _professionRecipes
// _classRecommendations
// _professionPairs
// _raceBonuses

// CHANGE Method Signatures (remove Player* parameter):
// FROM:
void Update(::Player* player, uint32 diff) override;
bool LearnProfession(::Player* player, ProfessionType profession) override;
// ... ~30 more methods

// TO:
void Update(uint32 diff) override;
bool LearnProfession(ProfessionType profession) override;
// ... ~30 more methods
```

**Affected Methods** (~35 total):
1. Update
2. LearnProfession
3. HasProfession
4. GetProfessionSkill
5. GetMaxProfessionSkill
6. GetPlayerProfessions
7. UnlearnProfession
8. AutoLearnProfessionsForClass
9. LearnRecipe
10. KnowsRecipe
11. GetRecipe
12. GetRecipesForProfession
13. GetOptimalLevelingRecipe
14. GetSkillUpChance
15. GetMissingMaterials
16. CraftRecipe
17. QueueCraftingTask
18. GetCraftingQueue
19. ClearCraftingQueue
20. ProcessCraftingQueue
21. SetAutomationEnabled
22. IsAutomationEnabled
23. SetTargetSkillLevel
24. GetTargetSkillLevel
25. CanAutoLearn
26. CanAutoLevel
27. ShouldCraft
28. ShouldGather
29. GetCurrentActivity
30. SetActivity
31. GetMetrics
32. ResetMetrics
33. OnRecipeLearned (event handler)
34. OnSkillUp (event handler)
35. On... (more event handlers)

**Files**:
- `ProfessionManager.h` (~400 lines, modify ~150 lines)

---

### Step 2.2: Refactor ProfessionManager.cpp

**Changes Required**:

1. **Include ProfessionDatabase**:
```cpp
#include "ProfessionDatabase.h"
```

2. **Remove Singleton**:
```cpp
// REMOVE:
ProfessionManager* ProfessionManager::instance()
{
    static ProfessionManager instance;
    return &instance;
}
```

3. **Update Constructor**:
```cpp
// FROM:
ProfessionManager::ProfessionManager()
{
}

// TO:
ProfessionManager::ProfessionManager(Player* bot)
    : _bot(bot)
    , _lastUpdateTime(0)
{
    TC_LOG_DEBUG("playerbot", "ProfessionManager: Creating instance for bot '{}'",
        _bot ? _bot->GetName() : "Unknown");
}

ProfessionManager::~ProfessionManager()
{
    TC_LOG_DEBUG("playerbot", "ProfessionManager: Destroying instance for bot '{}'",
        _bot ? _bot->GetName() : "Unknown");
}
```

4. **Remove Initialize() - Now ProfessionDatabase::Initialize()**:
```cpp
// REMOVE ENTIRELY (moved to ProfessionDatabase):
void ProfessionManager::Initialize()
{
    // ~800 lines of initialization code
}

void ProfessionManager::LoadRecipeDatabase() { ... }
void ProfessionManager::LoadProfessionRecommendations() { ... }
void ProfessionManager::InitializeClassProfessions() { ... }
// ... all 13 class initializers
void ProfessionManager::InitializeProfessionPairs() { ... }
void ProfessionManager::InitializeRaceBonuses() { ... }
```

5. **Update All Methods** (~400 lines to modify):

**Pattern for Each Method**:
```cpp
// OLD:
void ProfessionManager::Update(::Player* player, uint32 diff)
{
    if (!player)
        return;

    uint32 playerGuid = player->GetGUID().GetCounter();

    std::lock_guard lock(_mutex);

    _lastUpdateTimes[playerGuid] += diff;
    // ... use _playerProfiles[playerGuid], _craftingQueues[playerGuid], etc.
}

// NEW:
void ProfessionManager::Update(uint32 diff)
{
    if (!_bot)
        return;

    std::lock_guard lock(_mutex);

    _lastUpdateTime += diff;
    // ... use _profile, _craftingQueue, _metrics directly
}
```

**Apply This Pattern To** (~35 methods):
- Every method signature: remove `::Player* player` parameter
- Every method body: replace `player` with `_bot`
- Every `playerGuid` usage: remove (use direct members)
- Every `_playerProfiles[playerGuid]` → `_profile`
- Every `_craftingQueues[playerGuid]` → `_craftingQueue`
- Every `_playerMetrics[playerGuid]` → `_metrics`
- Every `_lastUpdateTimes[playerGuid]` → `_lastUpdateTime`

6. **Update Database Queries**:
```cpp
// OLD:
RecipeInfo const* recipe = GetRecipe(recipeId);
auto recommended = GetRecommendedProfessions(classId);
auto pairs = GetBeneficialPairs(profession);
uint16 bonus = GetRaceProfessionBonus(raceId, profession);

// NEW:
RecipeInfo const* recipe = ProfessionDatabase::instance()->GetRecipe(recipeId);
auto recommended = ProfessionDatabase::instance()->GetRecommendedProfessions(classId);
auto pairs = ProfessionDatabase::instance()->GetBeneficialPairs(profession);
uint16 bonus = ProfessionDatabase::instance()->GetRaceProfessionBonus(raceId, profession);
```

**Files**:
- `ProfessionManager.cpp` (~1221 lines, remove ~800 initialization, modify ~400)

**Estimated Time**: 2-3 hours

---

## Part 3: Update GameSystemsManager (30 minutes)

### Step 3.1: GameSystemsManager.h

```cpp
// CHANGE:
// FROM:
ProfessionManager* GetProfessionManager() const override;  // Returns singleton (TODO: convert to per-bot)

// TO:
ProfessionManager* GetProfessionManager() const override { return _professionManager.get(); }

// ADD Member:
std::unique_ptr<ProfessionManager> _professionManager;
```

### Step 3.2: GameSystemsManager.cpp

```cpp
// ADD in Initialize():
_professionManager = std::make_unique<ProfessionManager>(_bot);

// UPDATE in UpdateManagers():
// FROM:
GetProfessionManager()->Update(_bot, diff);

// TO:
GetProfessionManager()->Update(diff);

// REMOVE accessor implementation:
// DELETE:
ProfessionManager* GameSystemsManager::GetProfessionManager() const
{
    return ProfessionManager::instance();
}
```

### Step 3.3: IGameSystemsManager.h

No changes needed - interface already correct.

**Files**:
- `GameSystemsManager.h`
- `GameSystemsManager.cpp`

---

## Part 4: Update Callers (1-2 hours)

### Bridges (3 files)

**Files**:
1. `GatheringMaterialsBridge.cpp`
2. `AuctionMaterialsBridge.cpp`
3. `ProfessionAuctionBridge.cpp`

**Pattern**:
```cpp
// OLD:
ProfessionManager::instance()->GetRecipe(recipeId);
ProfessionManager::instance()->KnowsRecipe(player, recipeId);

// NEW - Access via ProfessionDatabase for shared data:
ProfessionDatabase::instance()->GetRecipe(recipeId);

// NEW - Access via bot's facade for per-bot operations:
// Need to get BotAI* from player, then access facade
BotSession* session = static_cast<BotSession*>(player->GetSession());
if (session && session->GetBotAI())
{
    ProfessionManager* profMgr = session->GetBotAI()->GetGameSystems()->GetProfessionManager();
    if (profMgr)
        profMgr->KnowsRecipe(recipeId);  // Note: no player parameter
}
```

### Other Files (2 files)

1. **BankingManager.cpp**
2. **FarmingCoordinator.cpp**

Same pattern as bridges.

### Service Registration (1 file)

**File**: `ServiceRegistration.h`

```cpp
// REMOVE:
container.RegisterInstance<IProfessionManager>(
    std::shared_ptr<IProfessionManager>(
        Playerbot::ProfessionManager::instance(),
        [](IProfessionManager*) {}  // No-op deleter (singleton)
    )
);

// NOTE: May need to change DI strategy since ProfessionManager is now per-bot
// Option 1: Remove from DI container (access via GameSystemsManager instead)
// Option 2: Change to factory pattern
```

### Global Initialization (1 file)

**File**: `PlayerbotModule.cpp`

```cpp
// CHANGE:
// FROM:
Playerbot::ProfessionManager::instance()->Initialize();

// TO:
Playerbot::ProfessionDatabase::instance()->Initialize();
```

**Files** (~7 total):
- 3 bridge files
- 2 manager files
- 1 DI registration file
- 1 module initialization file

---

## Part 5: Build System (15 minutes)

### CMakeLists.txt

**Add ProfessionDatabase to build**:

```cmake
# Find the profession sources section
${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionDatabase.cpp  # ADD THIS
${CMAKE_CURRENT_SOURCE_DIR}/Professions/ProfessionEventBus.cpp
# ... etc
```

**Files**:
- `src/modules/Playerbot/CMakeLists.txt`

---

## Part 6: Testing & Validation (1 hour)

### Test Scenarios

1. **Compilation Test**:
   ```bash
   cmake --build . --target playerbot
   ```

2. **Single Bot Test**:
   - Spawn one bot
   - Verify auto-learn professions works
   - Verify crafting automation works
   - Check logs for errors

3. **Multi-Bot Test**:
   - Spawn 10 bots
   - Verify each bot has independent profession data
   - Check for data corruption
   - Verify no shared state issues

4. **Performance Test**:
   - Spawn 100 bots
   - Measure memory usage (should be ~same as before)
   - Check CPU usage
   - Verify no memory leaks

### Validation Checklist

- [ ] Clean compilation (zero errors, zero warnings)
- [ ] No singleton instance() calls to ProfessionManager remain
- [ ] All bots have independent profession data
- [ ] ProfessionDatabase initialized once
- [ ] Per-bot ProfessionManager instances created/destroyed correctly
- [ ] No data corruption in multi-bot scenarios
- [ ] Memory usage acceptable
- [ ] No performance regression

---

## Phase 3: Bridge Analysis (4-6 hours)

### Objective
Determine if the 3 profession bridges provide architectural value or should be deprecated.

### Analysis Tasks

1. **Code Review** (2 hours):
   - Review all bridge implementations
   - Identify unique value vs simple delegation
   - Check for coupling issues
   - Assess complexity vs benefit

2. **Dependency Analysis** (1 hour):
   - Map all callers of each bridge
   - Identify alternative access patterns
   - Check for circular dependencies

3. **Decision Matrix** (1 hour):
   - Create architectural decision document
   - List pros/cons of keeping vs deprecating
   - Make final recommendation

4. **Documentation** (30 min):
   - Document findings in PHASE_3_BRIDGE_ANALYSIS.md
   - Commit analysis

### Possible Outcomes

**Option A**: Keep Bridges
- Bridges provide meaningful abstraction
- Continue to Phase 4 with integration

**Option B**: Deprecate Bridges
- Bridges are unnecessary indirection
- Remove and use managers directly

---

## Phase 4: Execute Decision (2-3 hours)

### If Option A (Keep Bridges)

1. **Integrate into Facade** (2 hours):
   - Add bridge instances to GameSystemsManager
   - Update bridge constructors to accept bot
   - Remove singleton pattern
   - Update all callers

2. **Test & Commit** (30 min)

### If Option B (Deprecate Bridges)

1. **Migration Plan** (1 hour):
   - Identify all bridge usage
   - Create replacement code
   - Document migration

2. **Execute Deprecation** (1 hour):
   - Update all callers to use managers directly
   - Mark bridges as deprecated
   - Add TODO comments for removal

3. **Test & Commit** (30 min)

---

## Timeline Summary

| Phase | Task | Estimated | Cumulative |
|-------|------|-----------|------------|
| 1B.2 | Refactor ProfessionManager | 3-4 hours | 3-4 hours |
| 1B.3 | Update GameSystemsManager | 30 min | 3.5-4.5 hours |
| 1B.4 | Update Callers | 1-2 hours | 4.5-6.5 hours |
| 1B.5 | Build System | 15 min | 4.75-6.75 hours |
| 1B.6 | Testing | 1 hour | 5.75-7.75 hours |
| **Phase 1B Total** | | **5.75-7.75 hours** | |
| 3 | Bridge Analysis | 4-6 hours | 9.75-13.75 hours |
| 4 | Execute Decision | 2-3 hours | 11.75-16.75 hours |
| **Grand Total** | | **11.75-16.75 hours** | |

---

## Risk Mitigation

**HIGH RISK**:
- Breaking profession automation for all bots
- ✅ Mitigation: Commit frequently, test incrementally

**MEDIUM RISK**:
- Performance regression with per-bot instances
- ✅ Mitigation: Profile before/after

**LOW RISK**:
- Compilation errors
- ✅ Mitigation: Compiler catches immediately

---

## Commit Strategy

1. ✅ **Commit 1**: ProfessionDatabase foundation (DONE - 5974c52f)
2. **Commit 2**: ProfessionManager refactored
3. **Commit 3**: GameSystemsManager updated
4. **Commit 4**: All callers updated + build system
5. **Commit 5**: Testing validation + Phase 1B complete
6. **Commit 6**: Phase 3 analysis document
7. **Commit 7**: Phase 4 implementation
8. **Commit 8**: Final documentation update

---

**Status**: Ready to execute Part 2 (ProfessionManager refactoring)
**Next Step**: Refactor ProfessionManager.h
