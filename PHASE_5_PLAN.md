# Phase 5: Management System Refactoring - Per-Bot Instance Pattern

**Status**: 📋 **PLANNED**
**Date**: 2025-11-18
**Branch**: `claude/playerbot-cleanup-analysis-01CcHSnAWQM7W9zVeUuJMF4h`

---

## Executive Summary

Phase 5 continues the architectural transformation from Phase 4, converting additional singleton manager systems to per-bot instances integrated into GameSystemsManager. This phase targets **management systems** with clear per-bot state that manage bot behaviors and resources.

### Scope

- **Managers to Convert**: 2 (BankingManager, FarmingCoordinator)
- **Estimated Lines**: ~2,000 across 14 files
- **New Managers in GameSystemsManager**: 21 → 23
- **Pattern**: Singleton → Per-Bot Instance (established in Phase 4)

---

## Architecture Analysis

### Current Singleton Landscape

After Phase 4 completion, I identified **90 files** with `static.*instance()` patterns in the PlayerBot module. These fall into three categories:

#### 1. Already Per-Bot (21 managers in GameSystemsManager)
✅ QuestManager, TradeManager, GatheringManager, ProfessionManager, 3 Bridges, AuctionManager, GroupCoordinator, DeathRecoveryManager, UnifiedMovementCoordinator, CombatStateManager, TargetScanner, GroupInvitationHandler, EventDispatcher, ManagerRegistry, 5 Decision Systems

#### 2. Should Stay Global (Infrastructure)
🔒 **BotTalentManager** - Immutable talent loadout cache shared by all bots
🔒 **ConfigManager** - Global configuration system
🔒 **PlayerbotDatabase** - Database connection pool
🔒 **BotLifecycleManager** - Global bot creation/spawning orchestration
🔒 **PlayerbotGroupManager** - Global group management
🔒 **LFGBotManager** - LFG queue management (global)
🔒 **SpatialGridManager** - Spatial indexing (global)
🔒 **BotPriorityManager** - Global bot priority scheduling
🔒 **Event Buses** - Global pub-sub systems (ProfessionEventBus, etc.)

#### 3. Should Be Per-Bot (Phase 5 Candidates)
🎯 **BankingManager** - Personal banking automation (★★★★★ High Priority)
🎯 **FarmingCoordinator** - Profession farming coordination (★★★★★ High Priority)
🎯 **EquipmentManager** - Equipment evaluation and auto-equip (★★★★ Medium Priority)
🎯 **MountManager** - Mount selection and usage (★★★ Lower Priority)
🎯 **BattlePetManager** - Battle pet management (★★★ Lower Priority)
🎯 **UnifiedQuestManager** - Quest management (complex, future phase)
🎯 **UnifiedLootManager** - Loot management (complex, group-based, future phase)

---

## Phase 5 Execution Plan

### Phase 5.1: BankingManager (22nd Manager)

**Priority**: ★★★★★ (Highest)
**Complexity**: Medium
**Estimated Time**: 2-3 hours

#### Why BankingManager First?

1. **Clear Per-Bot State**:
   ```cpp
   std::unordered_map<uint32, BotBankingProfile> _bankingProfiles;
   std::unordered_map<uint32, std::vector<BankingTransaction>> _transactionHistory;
   std::unordered_map<uint32, BankingStatistics> _playerStatistics;
   std::unordered_map<uint32, uint32> _lastBankAccessTimes;
   std::set<uint32> _currentlyBanking;
   ```

2. **All Methods Take Player* Parameter**: 25+ methods all operate on `Player*`
3. **Inherits from BehaviorManager**: Already follows manager lifecycle pattern
4. **Coordinates with Profession Systems**: Integrates with ProfessionManager, GatheringMaterialsBridge, ProfessionAuctionBridge
5. **Thread-Safe via Mutex**: Uses `OrderedRecursiveMutex<TRADE_MANAGER>` for synchronization

#### BankingManager Statistics

- **File**: `src/modules/Playerbot/Banking/BankingManager.h` (494 lines)
- **File**: `src/modules/Playerbot/Banking/BankingManager.cpp` (~800 lines estimated)
- **Public Methods**: 25 methods
- **Key Features**:
  - Gold management (auto-deposit/withdraw)
  - Item management (deposit excess, withdraw for crafting)
  - Banking rules and priorities
  - Transaction history tracking
  - Bank space analysis
  - Banker access and travel

#### Conversion Steps

1. **Header Refactoring** (`BankingManager.h`):
   - Remove `static instance()` method
   - Add `explicit BankingManager(Player* bot);`
   - Add `~BankingManager();` destructor
   - Remove `Player*` parameters from all 25 methods
   - Convert data structures:
     ```cpp
     // BEFORE
     std::unordered_map<uint32, BotBankingProfile> _bankingProfiles;
     std::unordered_map<uint32, BankingStatistics> _playerStatistics;

     // AFTER
     BotBankingProfile _profile;
     BankingStatistics _statistics;
     uint32 _lastBankAccessTime{0};
     bool _currentlyBanking{false};
     ```
   - Keep shared data static (default banking rules, etc.)

2. **Implementation Refactoring** (`BankingManager.cpp`):
   - Implement static member initialization
   - Implement per-bot constructor/destructor
   - Refactor all 25 methods to use `_bot` member
   - Remove mutex locking (per-bot isolation)
   - Update ProfessionManager access via GameSystemsManager facade:
     ```cpp
     ProfessionManager* BankingManager::GetProfessionManager()
     {
         if (!_bot) return nullptr;
         BotSession* session = static_cast<BotSession*>(_bot->GetSession());
         if (!session || !session->GetBotAI()) return nullptr;
         return session->GetBotAI()->GetGameSystems()->GetProfessionManager();
     }
     ```

3. **GameSystemsManager Integration**:
   - Update `GameSystemsManager.h`:
     - Add include: `#include "Banking/BankingManager.h"`
     - Add getter: `BankingManager* GetBankingManager() const { return _bankingManager.get(); }`
     - Add member: `std::unique_ptr<BankingManager> _bankingManager;`
   - Update `GameSystemsManager.cpp`:
     - Constructor: `_bankingManager = std::make_unique<BankingManager>(_bot);`
     - Initialize: Call `_bankingManager->OnInitialize();`
     - Update: Call `_bankingManager->OnUpdate(diff);`
     - Destructor: `_bankingManager.reset();`

4. **Legacy Cleanup**:
   - Remove singleton registration from `PlayerbotModule.cpp`
   - Remove DI registration from `ServiceRegistration.h`
   - Update any direct `BankingManager::instance()` calls

5. **Testing**:
   - Verify bot banking automation works
   - Test gold deposit/withdrawal triggers
   - Test item banking rules
   - Test profession material coordination

---

### Phase 5.2: FarmingCoordinator (23rd Manager)

**Priority**: ★★★★★ (Highest)
**Complexity**: Medium-High
**Estimated Time**: 2-3 hours

#### Why FarmingCoordinator Second?

1. **Clear Per-Bot State**:
   ```cpp
   std::unordered_map<uint32, FarmingSession> _activeSessions;
   std::unordered_map<uint32, FarmingCoordinatorProfile> _profiles;
   std::unordered_map<uint32, uint32> _lastFarmingTimes;
   std::unordered_map<uint32, FarmingStatistics> _playerStatistics;
   ```

2. **All Methods Take Player* Parameter**: 19+ methods all operate on `Player*`
3. **Complex Profession Coordination**: Works with ProfessionManager, GatheringManager, auction systems
4. **Session Management**: Manages farming sessions per bot with state tracking
5. **Zone Selection Logic**: Per-bot zone scoring and travel logic

#### FarmingCoordinator Statistics

- **File**: `src/modules/Playerbot/Professions/FarmingCoordinator.h` (383 lines)
- **File**: `src/modules/Playerbot/Professions/FarmingCoordinator.cpp` (~1,200 lines estimated)
- **Public Methods**: 19 methods
- **Key Features**:
  - Skill gap analysis (character level vs profession skill)
  - Farming session management (start/stop/update)
  - Zone selection and optimization
  - Material stockpile management
  - Travel and positioning
  - Statistics tracking

#### Conversion Steps

1. **Header Refactoring** (`FarmingCoordinator.h`):
   - Remove `static instance()` method
   - Add `explicit FarmingCoordinator(Player* bot);`
   - Add `~FarmingCoordinator();` destructor
   - Remove `Player*` parameters from all 19 methods
   - Convert data structures:
     ```cpp
     // BEFORE
     std::unordered_map<uint32, FarmingSession> _activeSessions;
     std::unordered_map<uint32, FarmingCoordinatorProfile> _profiles;

     // AFTER
     FarmingSession _activeSession;  // Only one session per bot
     FarmingCoordinatorProfile _profile;
     uint32 _lastFarmingTime{0};
     FarmingStatistics _statistics;
     ```
   - Keep shared data static (farming zones, node database, etc.):
     ```cpp
     static std::unordered_map<ProfessionType, std::vector<FarmingZoneInfo>> _farmingZones;
     static bool _farmingZonesInitialized;
     ```

2. **Implementation Refactoring** (`FarmingCoordinator.cpp`):
   - Implement static member initialization (farming zones)
   - Implement per-bot constructor/destructor
   - Refactor all 19 methods to use `_bot` member
   - Remove mutex locking (per-bot isolation)
   - Update ProfessionManager/GatheringManager access via facade

3. **GameSystemsManager Integration**:
   - Update `GameSystemsManager.h`:
     - Add include: `#include "Professions/FarmingCoordinator.h"`
     - Add getter: `FarmingCoordinator* GetFarmingCoordinator() const { return _farmingCoordinator.get(); }`
     - Add member: `std::unique_ptr<FarmingCoordinator> _farmingCoordinator;`
   - Update `GameSystemsManager.cpp`:
     - Constructor: `_farmingCoordinator = std::make_unique<FarmingCoordinator>(_bot);`
     - Initialize: Call `_farmingCoordinator->Initialize(_bot);`
     - Update: Call `_farmingCoordinator->Update(diff);`
     - Destructor: `_farmingCoordinator.reset();`

4. **Legacy Cleanup**:
   - Remove singleton registration from `PlayerbotModule.cpp`
   - Update any direct `FarmingCoordinator::instance()` calls

5. **Testing**:
   - Verify farming session triggers when skill gap detected
   - Test zone selection and travel
   - Test farming session completion
   - Test material stockpile coordination

---

## Established Patterns (From Phase 4)

### 1. Singleton → Per-Bot Transformation Pattern

```cpp
// STEP 1: Remove singleton
- static Bridge* instance();
+ explicit Bridge(Player* bot);
+ ~Bridge();

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

### 2. Facade Access Pattern

```cpp
ProfessionManager* Manager::GetProfessionManager()
{
    if (!_bot) return nullptr;
    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetBotAI()) return nullptr;
    return session->GetBotAI()->GetGameSystems()->GetProfessionManager();
}
```

### 3. GameSystemsManager Integration Pattern

```cpp
// Header
#include "Path/ManagerName.h"
ManagerName* GetManagerName() const { return _managerName.get(); }
std::unique_ptr<ManagerName> _managerName;

// Constructor
_managerName = std::make_unique<ManagerName>(_bot);

// Initialize
if (_managerName)
{
    _managerName->Initialize(_bot);  // or OnInitialize() for BehaviorManager
    TC_LOG_DEBUG("playerbot", "GameSystemsManager: ManagerName initialized");
}

// Update
if (_managerName)
    _managerName->Update(diff);  // or OnUpdate() for BehaviorManager

// Destructor (explicit ordering)
_managerName.reset();
```

---

## Performance Improvements Expected

### Memory Optimization

| Aspect | Before | After | Improvement |
|--------|--------|-------|-------------|
| Map Overhead | ~48 bytes/entry | 0 bytes | 100% reduction |
| Cache Locality | Poor (scattered) | Excellent (contiguous) | 40% estimated |
| Memory per Bot | Variable | Fixed | 30-50% reduction |

### Concurrency Improvements

| Aspect | Before | After | Improvement |
|--------|--------|-------|-------------|
| Mutex Locks | Required | Zero | 100% elimination |
| Lock Contention | High (100+ bots) | None | ∞% improvement |
| Thread Safety | Explicit locking | Implicit isolation | Simplified |

---

## Testing Strategy

### Per-Phase Testing

1. **Compilation**: Ensure all files compile without errors
2. **Linkage**: Verify no undefined references
3. **Runtime**: Test bot creation, initialization, update loop
4. **Integration**: Verify manager interactions work correctly
5. **Performance**: Monitor memory usage and CPU contention

### Integration Testing

1. **Banking Automation**:
   - Bot deposits gold when exceeds threshold
   - Bot withdraws materials for crafting
   - Bank space optimization works

2. **Farming Coordination**:
   - Bot detects skill gap and starts farming session
   - Bot travels to optimal farming zone
   - Bot gathers nodes and gains skill points
   - Session completes when target skill reached

3. **Cross-Manager Coordination**:
   - BankingManager ↔ ProfessionManager (material needs)
   - FarmingCoordinator ↔ GatheringManager (node harvesting)
   - FarmingCoordinator ↔ ProfessionManager (skill tracking)

---

## Migration Guide for Developers

### Old Code (Singleton)

```cpp
BankingManager::instance()->DepositGold(player, 50000);
FarmingCoordinator::instance()->StartFarmingSession(player, ProfessionType::MINING);
```

### New Code (Per-Bot)

```cpp
auto* gameSystems = botAI->GetGameSystems();

gameSystems->GetBankingManager()->DepositGold(50000);
gameSystems->GetFarmingCoordinator()->StartFarmingSession(ProfessionType::MINING);
```

**Key Differences**:
1. No more `::instance()` calls
2. No more `Player* player` parameters
3. Access via `GameSystemsManager` facade
4. Each bot has its own manager instances

---

## Future Phases (Phase 6+)

### Phase 6 Candidates (Medium Priority)

- **EquipmentManager** - Equipment evaluation and auto-equip (per-bot profiles, metrics)
- **MountManager** - Mount selection and usage (per-bot preferences)
- **BattlePetManager** - Battle pet management (per-bot collection)

### Complex Refactoring (Future)

- **UnifiedQuestManager** - Large manager with 100+ methods (requires careful analysis)
- **UnifiedLootManager** - Group-based loot coordination (complex interactions)
- **GuildBankManager** - Guild-level vs per-bot considerations

---

## Success Criteria

Phase 5 is considered complete when:

- ✅ BankingManager converted to per-bot instance (22nd manager)
- ✅ FarmingCoordinator converted to per-bot instance (23rd manager)
- ✅ Both managers integrated into GameSystemsManager
- ✅ All legacy singleton calls removed
- ✅ Code compiles without errors
- ✅ Integration tests pass
- ✅ Documentation updated (PHASE_5_COMPLETE.md)
- ✅ DEVELOPER_GUIDE.md updated with new managers

---

## Risks and Mitigations

### Risk 1: Breaking Existing Functionality
**Mitigation**: Systematic testing after each conversion, maintain backward compatibility during transition

### Risk 2: Performance Regression
**Mitigation**: Profile before/after, measure memory usage and lock contention

### Risk 3: Complex Dependencies
**Mitigation**: Use facade access pattern, document all cross-manager interactions

### Risk 4: Initialization Ordering
**Mitigation**: Follow established GameSystemsManager initialization order, document dependencies

---

## Conclusion

Phase 5 continues the architectural evolution from Phase 4, converting 2 high-priority management systems from singleton to per-bot instances. This phase targets systems with clear per-bot state and strong integration with the profession ecosystem.

**Expected Benefits**:
- Zero lock contention (100% mutex elimination for these 2 managers)
- Better cache locality (direct members vs maps)
- Cleaner API (no Player* parameters)
- Clear ownership (facade pattern)
- Improved scalability (per-bot isolation)

**GameSystemsManager Evolution**: 21 → 23 managers

---

**Document Version**: 1.0
**Last Updated**: 2025-11-18
**Author**: Claude (Anthropic)
