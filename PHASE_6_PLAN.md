# Phase 6: Equipment, Mount, and Battle Pet Management - Per-Bot Instance Pattern

**Status**: 📋 **PLANNED**
**Date**: 2025-11-18
**Branch**: `claude/playerbot-cleanup-analysis-01CcHSnAWQM7W9zVeUuJMF4h`

---

## Executive Summary

Phase 6 continues the architectural transformation from Phases 4 and 5, converting 3 additional manager systems to per-bot instances integrated into GameSystemsManager. This phase targets **equipment, mount, and battle pet management** systems with clear per-bot state.

### Scope

- **Managers to Convert**: 3 (EquipmentManager, MountManager, BattlePetManager)
- **Estimated Lines**: ~3,500 across 18 files
- **New Managers in GameSystemsManager**: 23 → 26
- **Pattern**: Singleton → Per-Bot Instance (established in Phases 4 & 5)

---

## Architecture Analysis

### Current State (Post Phase 5.2)

**GameSystemsManager**: 23 managers
- Phase 4: 3 bridges (GatheringMaterialsBridge, AuctionMaterialsBridge, ProfessionAuctionBridge)
- Phase 5.1: BankingManager
- Phase 5.2: FarmingCoordinator

### Phase 6 Candidates Analysis

#### 1. EquipmentManager ⭐⭐⭐⭐ (HIGH PRIORITY - 24th Manager)

**Current Implementation**:
```cpp
class EquipmentManager {
    static EquipmentManager* instance();

    // Per-player data structures
    std::unordered_map<uint32, EquipmentProfile> _equipmentProfiles;
    std::unordered_map<uint32, EquipmentMetrics> _equipmentMetrics;
    std::unordered_map<uint32, std::vector<EquipmentRecommendation>> _recommendations;
    std::mutex _mutex;
};
```

**Why High Priority?**:
1. **Clear Per-Bot State**: Equipment preferences, gear scoring, auto-equip rules
2. **Frequent Updates**: Auto-equip checks every 10 seconds (high contention)
3. **Integration Points**: Uses stat weights from class/spec systems
4. **Performance Impact**: Mutex contention with 100+ bots checking gear constantly

**Estimated Complexity**: Medium
**Estimated Time**: 3-4 hours
**File Count**: ~6-8 files

---

#### 2. MountManager ⭐⭐⭐ (MEDIUM PRIORITY - 25th Manager)

**Current Implementation**:
```cpp
class MountManager {
    static MountManager* instance();

    // Per-player data structures
    std::unordered_map<uint32, MountPreferences> _mountPreferences;
    std::unordered_map<uint32, std::set<uint32>> _collectedMounts;
    std::unordered_map<uint32, MountUsageStats> _usageStats;
    std::mutex _mutex;
};
```

**Why Medium Priority?**:
1. **Clear Per-Bot State**: Mount preferences, collection tracking
2. **Moderate Updates**: Called on zone changes, combat state changes
3. **Integration Points**: Zone detection, speed buffs, flying vs ground logic
4. **Lower Contention**: Less frequent than equipment checks

**Estimated Complexity**: Low-Medium
**Estimated Time**: 2-3 hours
**File Count**: ~4-6 files

---

#### 3. BattlePetManager ⭐⭐⭐ (MEDIUM PRIORITY - 26th Manager)

**Current Implementation**:
```cpp
class BattlePetManager {
    static BattlePetManager* instance();

    // Per-player data structures
    std::unordered_map<uint32, BattlePetCollection> _petCollections;
    std::unordered_map<uint32, BattlePetTeam> _activeTeams;
    std::unordered_map<uint32, BattlePetStats> _battleStats;
    std::mutex _mutex;
};
```

**Why Medium Priority?**:
1. **Clear Per-Bot State**: Pet collection, team composition, battle stats
2. **Infrequent Updates**: Only active during pet battles
3. **Optional System**: Not all bots use battle pets
4. **Lower Impact**: Minimal performance impact on main gameplay

**Estimated Complexity**: Low-Medium
**Estimated Time**: 2-3 hours
**File Count**: ~4-6 files

---

## Phase 6 Execution Plan

### Phase 6.1: EquipmentManager (24th Manager)

**Priority**: ★★★★ (Highest)

#### EquipmentManager Statistics

**Files**:
- `src/modules/Playerbot/Equipment/EquipmentManager.h` (~600 lines estimated)
- `src/modules/Playerbot/Equipment/EquipmentManager.cpp` (~1,200 lines estimated)

**Public Methods**: ~30 methods (all take Player* parameter)

**Key Features**:
- Gear scoring and evaluation (stat weights per class/spec)
- Auto-equip best gear (triggered periodically)
- Equipment recommendations
- Item comparison and upgrades
- Durability monitoring
- Equipment set bonuses

#### Conversion Steps

1. **Header Refactoring** (`EquipmentManager.h`):
   - Remove `static instance()` method
   - Add `explicit EquipmentManager(Player* bot);`
   - Add `~EquipmentManager();` destructor
   - Remove `Player*` parameters from all ~30 methods
   - Convert data structures:
     ```cpp
     // BEFORE
     std::unordered_map<uint32, EquipmentProfile> _equipmentProfiles;
     std::unordered_map<uint32, EquipmentMetrics> _equipmentMetrics;

     // AFTER
     EquipmentProfile _profile;
     EquipmentMetrics _metrics;
     ```
   - Keep shared data static:
     ```cpp
     static std::unordered_map<uint8, StatWeights> _classStatWeights;  // Class defaults
     static std::unordered_map<uint8, StatWeights> _specStatWeights;   // Spec defaults
     static bool _statWeightsInitialized;
     ```

2. **Implementation Refactoring** (`EquipmentManager.cpp`):
   - Implement static member initialization
   - Implement per-bot constructor/destructor
   - Refactor all methods to use `_bot` member
   - Remove mutex locking (100% elimination)
   - Load stat weights once (shared across all bots)

3. **GameSystemsManager Integration**:
   - Update `GameSystemsManager.h`:
     - Add `#include "Equipment/EquipmentManager.h"`
     - Add `EquipmentManager* GetEquipmentManager() const`
     - Add `std::unique_ptr<EquipmentManager> _equipmentManager;`
   - Update `GameSystemsManager.cpp`:
     - Constructor: `_equipmentManager = std::make_unique<EquipmentManager>(_bot);`
     - Initialize: `_equipmentManager->Initialize();`
     - Update: `_equipmentManager->Update(diff);`
     - Destructor: `_equipmentManager.reset();`

4. **Legacy Cleanup**:
   - Remove singleton calls from `GameSystemsManager.cpp` (current auto-equip logic)
   - Update any direct `EquipmentManager::instance()` calls

---

### Phase 6.2: MountManager (25th Manager)

**Priority**: ★★★ (Medium)

#### MountManager Statistics

**Files**:
- `src/modules/Playerbot/Mounts/MountManager.h` (~400 lines estimated)
- `src/modules/Playerbot/Mounts/MountManager.cpp` (~800 lines estimated)

**Public Methods**: ~20 methods

**Key Features**:
- Mount selection based on zone (flying vs ground)
- Mount speed optimization
- Combat dismount handling
- Mount collection tracking
- Preference management

#### Conversion Steps

Follow same pattern as EquipmentManager:
1. Remove singleton, add per-bot constructor
2. Convert maps to direct members
3. Remove Player* parameters
4. Add static shared data (mount database, speed tables)
5. Integrate with GameSystemsManager

---

### Phase 6.3: BattlePetManager (26th Manager)

**Priority**: ★★★ (Medium)

#### BattlePetManager Statistics

**Files**:
- `src/modules/Playerbot/BattlePets/BattlePetManager.h` (~500 lines estimated)
- `src/modules/Playerbot/BattlePets/BattlePetManager.cpp` (~1,000 lines estimated)

**Public Methods**: ~25 methods

**Key Features**:
- Pet collection management
- Battle team composition
- Pet leveling automation
- Battle AI (move selection)
- Pet capture logic

#### Conversion Steps

Follow same pattern:
1. Remove singleton, add per-bot constructor
2. Convert maps to direct members
3. Remove Player* parameters
4. Add static shared data (pet database, ability data)
5. Integrate with GameSystemsManager

---

## Established Patterns (From Phases 4 & 5)

### 1. Singleton → Per-Bot Transformation Pattern

```cpp
// STEP 1: Remove singleton
- static Manager* instance();
+ explicit Manager(Player* bot);
+ ~Manager();

// STEP 2: Convert data structures
- std::unordered_map<playerGuid, T> _playerData;
+ T _data;  // Direct member

// STEP 3: Update method signatures
- void Method(Player* player, ...);
+ void Method(...);  // Use _bot instead

// STEP 4: Add static shared data
+ static SharedData _sharedData;
+ static bool _sharedDataInitialized;

// STEP 5: Remove mutex (per-bot isolation)
- std::mutex _mutex;
- std::lock_guard<std::mutex> lock(_mutex);
```

### 2. GameSystemsManager Integration Pattern

```cpp
// Header (.h)
#include "Path/ManagerName.h"
ManagerName* GetManagerName() const { return _managerName.get(); }
std::unique_ptr<ManagerName> _managerName;

// Constructor (.cpp)
_managerName = std::make_unique<ManagerName>(_bot);

// Initialize
if (_managerName)
{
    _managerName->Initialize();
    TC_LOG_INFO("...", "ManagerName initialized");
}

// Update
if (_managerName)
    _managerName->Update(diff);

// Destructor (explicit ordering)
if (_managerName)
{
    TC_LOG_DEBUG("...", "Destroying ManagerName");
    _managerName.reset();
}
```

---

## Performance Improvements Expected

### Per Manager

| Manager | Mutex Locks | Map Lookups | Memory Reduction | Cache Improvement |
|---------|-------------|-------------|------------------|-------------------|
| EquipmentManager | Required → Zero | Per check → Direct | 30-50% | 40% estimated |
| MountManager | Required → Zero | Per use → Direct | 30-50% | 40% estimated |
| BattlePetManager | Required → Zero | Per battle → Direct | 30-50% | 40% estimated |

### Cumulative Impact

- **Mutex Elimination**: 3 more managers with zero lock contention
- **Total Managers**: 23 → 26 (13% growth)
- **Cache Locality**: Better memory layout for bot data
- **Scalability**: Linear scaling to 1000+ bots

---

## Testing Strategy

### Per-Manager Testing

1. **Compilation**: Ensure all files compile without errors
2. **Linkage**: Verify no undefined references
3. **Runtime**: Test bot creation, initialization, update loop
4. **Integration**: Verify manager interactions work correctly

### Integration Testing

1. **EquipmentManager**:
   - Bot auto-equips best gear on drops
   - Stat weights applied correctly per class/spec
   - Equipment recommendations generated
   - Durability warnings triggered

2. **MountManager**:
   - Bot selects flying mount in flying zones
   - Bot selects ground mount in non-flying zones
   - Combat dismount handled correctly
   - Speed buffs detected

3. **BattlePetManager**:
   - Pet collection tracked per bot
   - Battle teams composed correctly
   - Pet battles automated
   - Pet capture logic works

---

## Migration Guide for Developers

### Old Code (Singleton)

```cpp
// EquipmentManager
EquipmentManager::instance()->AutoEquipBestGear(player);
float score = EquipmentManager::instance()->CalculateItemScore(player, item);

// MountManager
MountManager::instance()->SummonAppropriateMount(player);

// BattlePetManager
BattlePetManager::instance()->StartBattle(player, opponent);
```

### New Code (Per-Bot)

```cpp
auto* gameSystems = botAI->GetGameSystems();

// EquipmentManager
gameSystems->GetEquipmentManager()->AutoEquipBestGear();
float score = gameSystems->GetEquipmentManager()->CalculateItemScore(item);

// MountManager
gameSystems->GetMountManager()->SummonAppropriateMount();

// BattlePetManager
gameSystems->GetBattlePetManager()->StartBattle(opponent);
```

**Key Differences**:
1. No more `::instance()` calls
2. No more `Player* player` parameters
3. Access via `GameSystemsManager` facade
4. Each bot has its own manager instances

---

## Phase 6 Success Criteria

Phase 6 is considered complete when:

- ✅ EquipmentManager converted to per-bot instance (24th manager)
- ✅ MountManager converted to per-bot instance (25th manager)
- ✅ BattlePetManager converted to per-bot instance (26th manager)
- ✅ All managers integrated into GameSystemsManager
- ✅ All legacy singleton calls removed
- ✅ Code compiles without errors
- ✅ Integration tests pass
- ✅ Documentation updated (PHASE_6_COMPLETE.md)
- ✅ DEVELOPER_GUIDE.md updated with new managers

---

## Risks and Mitigations

### Risk 1: EquipmentManager Complexity
**Impact**: Complex stat weight calculations, many item types
**Mitigation**: Thorough testing of scoring formulas, validate against known good items

### Risk 2: MountManager Zone Detection
**Impact**: Flying/ground logic depends on zone flags
**Mitigation**: Test in multiple zone types (flying, no-flying, battlegrounds, arenas)

### Risk 3: BattlePetManager Optional Usage
**Impact**: Not all bots use battle pets
**Mitigation**: Ensure graceful handling of bots without pets, null checks

### Risk 4: Performance with Auto-Equip
**Impact**: EquipmentManager called frequently (every 10 seconds per bot)
**Mitigation**: Profile performance, ensure direct member access is fast

---

## Conclusion

Phase 6 converts 3 management systems from singleton to per-bot instances, bringing GameSystemsManager to **26 managers**. These conversions eliminate mutex contention for equipment, mount, and pet systems, improving scalability and cache locality.

**Expected Benefits**:
- Zero lock contention (100% mutex elimination for 3 managers)
- Better cache locality (direct members vs maps)
- Cleaner API (no Player* parameters)
- Clear ownership (facade pattern)
- Improved scalability (per-bot isolation)

**GameSystemsManager Evolution**: 23 → 26 managers

---

**Document Version**: 1.0
**Last Updated**: 2025-11-18
**Author**: Claude (Anthropic)
