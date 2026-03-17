# Phase 1B: ProfessionManager Per-Bot Instance Refactoring Plan

**Date**: 2025-11-18
**Scope**: Convert ProfessionManager from global singleton to per-bot instance
**Estimated Effort**: 4-6 hours
**Status**: 🔍 PLANNING

---

## Current Architecture Analysis

### ProfessionManager Singleton Structure

**Shared/Global Data** (stays singleton):
- `_recipeDatabase` - Recipe info for all recipes (shared)
- `_professionRecipes` - Recipes per profession (shared)
- `_classRecommendations` - Class -> professions mapping (shared)
- `_professionPairs` - Beneficial profession pairs (shared)
- `_raceBonuses` - Race profession bonuses (shared)

**Per-Bot Data** (needs conversion to instance members):
- `_playerProfiles` - map<playerGuid, Profile> → `_profile`
- `_craftingQueues` - map<playerGuid, Queue> → `_craftingQueue`
- `_playerMetrics` - map<playerGuid, Metrics> → `_metrics`
- `_lastUpdateTimes` - map<playerGuid, uint32> → `_lastUpdateTime`

---

## Refactoring Strategy

### Step 1: Create ProfessionDatabase Singleton

**New File**: `ProfessionDatabase.h/.cpp`

**Purpose**: Centralize shared profession data (recipes, recommendations, bonuses)

**Interface**:
```cpp
class ProfessionDatabase
{
public:
    static ProfessionDatabase* instance();

    void Initialize();

    RecipeInfo const* GetRecipe(uint32 recipeId) const;
    std::vector<uint32> const& GetRecipesForProfession(ProfessionType profession) const;
    std::vector<ProfessionType> const& GetRecommendedProfessions(uint8 classId) const;
    std::vector<ProfessionType> const& GetBeneficialPairs(ProfessionType profession) const;
    uint16 GetRaceProfessionBonus(uint8 raceId, ProfessionType profession) const;

private:
    std::unordered_map<uint32, RecipeInfo> _recipes;
    std::unordered_map<ProfessionType, std::vector<uint32>> _professionRecipes;
    std::unordered_map<uint8, std::vector<ProfessionType>> _classRecommendations;
    std::unordered_map<ProfessionType, std::vector<ProfessionType>> _professionPairs;
    std::unordered_map<uint8, std::unordered_map<ProfessionType, uint16>> _raceBonuses;
};
```

---

### Step 2: Convert ProfessionManager to Per-Bot Instance

**Changes to ProfessionManager**:

**Remove**:
- `static ProfessionManager* instance()` - Singleton pattern
- All map<playerGuid, T> data structures

**Add**:
- `Player* _bot` - Bot player reference (non-owning)
- Per-bot instance members:
  - `ProfessionAutomationProfile _profile`
  - `std::vector<CraftingTask> _craftingQueue`
  - `ProfessionMetrics _metrics`
  - `uint32 _lastUpdateTime`

**Constructor Changes**:
```cpp
// OLD
ProfessionManager(); // Private constructor for singleton

// NEW
explicit ProfessionManager(Player* bot);
~ProfessionManager();
```

**Method Signature Changes**:
```cpp
// OLD
void Update(Player* player, uint32 diff);
bool LearnProfession(Player* player, ProfessionType profession);

// NEW
void Update(uint32 diff);  // Uses _bot internally
bool LearnProfession(ProfessionType profession);  // Uses _bot internally
```

---

### Step 3: Update GameSystemsManager

**GameSystemsManager.h**:
```cpp
// REMOVE this comment:
// Note: ProfessionManager accessed via singleton (not owned) - TODO: convert to per-bot

// CHANGE:
ProfessionManager* GetProfessionManager() const override;  // Returns singleton

// TO:
ProfessionManager* GetProfessionManager() const override { return _professionManager.get(); }

// ADD member:
std::unique_ptr<ProfessionManager> _professionManager;
```

**GameSystemsManager.cpp**:
```cpp
// REMOVE:
// Note: ProfessionManager remains singleton (accessed via facade for now)
// TODO: Convert to per-bot instance like other managers (Phase 1B)

// ADD in Initialize():
_professionManager = std::make_unique<ProfessionManager>(_bot);

// UPDATE in UpdateManagers():
// OLD: GetProfessionManager()->Update(_bot, diff);
// NEW: GetProfessionManager()->Update(diff);

// REMOVE accessor implementation:
// ProfessionManager* GameSystemsManager::GetProfessionManager() const { ... }
// (Now inline in header)
```

---

### Step 4: Update All Callers

**Files to Update** (from grep analysis):
1. ✅ `GameSystemsManager.cpp` - Updated in Step 3
2. `GatheringMaterialsBridge.cpp` - Uses ProfessionManager::instance()
3. `AuctionMaterialsBridge.cpp` - Uses ProfessionManager::instance()
4. `ProfessionAuctionBridge.cpp` - Uses ProfessionManager::instance()
5. `BankingManager.cpp` - Uses ProfessionManager::instance()
6. `FarmingCoordinator.cpp` - Uses ProfessionManager::instance()
7. `ServiceRegistration.h` - DI container registration
8. `PlayerbotModule.cpp` - Global initialization

**Strategy for Non-Facade Callers**:

**Option A**: Keep ProfessionDatabase::instance() for global initialization
**Option B**: Access via facade where possible

For bridges, they should access ProfessionManager through GameSystemsManager facade.

---

### Step 5: Handle Global Initialization

**Challenge**: `PlayerbotModule.cpp` calls `ProfessionManager::instance()->Initialize()`

**Solution**: Move to `ProfessionDatabase::instance()->Initialize()`

```cpp
// PlayerbotModule.cpp
// OLD:
Playerbot::ProfessionManager::instance()->Initialize();

// NEW:
Playerbot::ProfessionDatabase::instance()->Initialize();
```

---

## Implementation Checklist

### Part 1: Create ProfessionDatabase
- [ ] Create ProfessionDatabase.h
- [ ] Create ProfessionDatabase.cpp
- [ ] Move shared data initialization from ProfessionManager
- [ ] Test compilation

### Part 2: Refactor ProfessionManager
- [ ] Update ProfessionManager.h (remove singleton, add per-bot members)
- [ ] Update ProfessionManager.cpp (convert all methods)
- [ ] Remove playerGuid parameters from methods
- [ ] Use _bot for all operations
- [ ] Test compilation

### Part 3: Update GameSystemsManager
- [ ] Add _professionManager unique_ptr to GameSystemsManager.h
- [ ] Update Initialize() to create instance
- [ ] Update Update() to call instance method
- [ ] Make GetProfessionManager() inline
- [ ] Test compilation

### Part 4: Update Callers
- [ ] Update bridge files (3 files)
- [ ] Update BankingManager
- [ ] Update FarmingCoordinator
- [ ] Update ServiceRegistration.h
- [ ] Update PlayerbotModule.cpp
- [ ] Test compilation

### Part 5: Testing & Validation
- [ ] Verify no singleton instance() calls remain
- [ ] Verify per-bot data isolation
- [ ] Test multi-bot scenarios
- [ ] Commit changes

---

## Risk Assessment

**HIGH RISK**:
- Breaking existing profession automation for all bots
- Data loss if per-bot separation is incorrect

**MEDIUM RISK**:
- Performance impact from per-bot allocations
- Thread safety issues if mutex handling is wrong

**LOW RISK**:
- Compilation errors (easy to fix)
- Method signature mismatches (caught by compiler)

---

## Testing Strategy

1. **Compilation Test**: Verify clean build
2. **Single Bot Test**: Spawn one bot, verify profession automation works
3. **Multi-Bot Test**: Spawn 100 bots, verify no data corruption
4. **Performance Test**: Measure memory/CPU before and after

---

**Status**: Ready for implementation
**Next Step**: Create ProfessionDatabase singleton
