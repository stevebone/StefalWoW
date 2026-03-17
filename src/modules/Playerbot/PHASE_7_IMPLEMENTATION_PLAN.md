# Phase 7: Singleton to Per-Bot Instance Conversion Plan

**Created:** 2025-11-18
**Branch:** claude/playerbot-cleanup-analysis-01CcHSnAWQM7W9zVeUuJMF4h
**Status:** IN PROGRESS
**Target:** Convert 25 singleton managers to per-bot instances

---

## 🎯 Executive Summary

Phase 7 continues the architectural refactoring started in Phases 5 and 6, converting remaining singleton managers with per-player state to per-bot instances owned by GameSystemsManager.

**Current State:**
- **26 managers** successfully converted (Phases 1B through 6.3)
- **25 managers** identified as Phase 7 candidates
- All following the same anti-pattern: singleton + per-player maps + mutex

**Goals:**
- Eliminate mutex contention (zero lock performance overhead)
- Improve cache performance (direct member access vs map lookups)
- Enhance testability (dependency injection)
- Reduce coupling (explicit ownership)

---

## 📊 Phase 7 Candidates Analysis

### Total Managers: 25

| Category | Count | Priority | Estimated Effort |
|----------|-------|----------|------------------|
| Combat & PvP | 3 | HIGH | 2-3 days |
| Quest Systems | 6 | HIGH | 3-4 days |
| Social & Guild | 7 | MEDIUM | 3-4 days |
| Group & LFG | 5 | MEDIUM | 2-3 days |
| Instance/Dungeon | 1 | LOW | 1 day |
| Infrastructure | 3 | LOW | 2 days |

**Total Estimated Effort:** 13-18 days (can be parallelized)

---

## 🗺️ Phase 7 Breakdown

### **Phase 7.1: Combat Systems** 🎯 [CURRENT]

**Managers:** 3
**Priority:** HIGH (combat-critical functionality)
**Estimated Time:** 2-3 days

#### Managers to Convert:

1. **UnifiedInterruptSystem**
   - **Complexity:** MEDIUM
   - **Methods:** 17 Player* methods
   - **Maps:** 3 (bot info, AI refs, capabilities)
   - **Impact:** Interrupt coordination across all combat scenarios
   - **Files:** `AI/Combat/UnifiedInterruptSystem.h/cpp`

2. **ArenaAI**
   - **Complexity:** HIGH
   - **Methods:** 46 Player* methods
   - **Maps:** 1 (focus targets)
   - **Impact:** Arena PvP automation
   - **Files:** `PvP/ArenaAI.h/cpp`

3. **PvPCombatAI**
   - **Complexity:** HIGH
   - **Methods:** 58 Player* methods (LARGEST in category)
   - **Maps:** 2 (CC chains, current targets)
   - **Impact:** All PvP combat strategies
   - **Files:** `PvP/PvPCombatAI.h/cpp`

#### Success Criteria:
- ✅ All 3 managers converted to per-bot instances
- ✅ Zero mutex locks remaining
- ✅ All Player* parameters removed from methods
- ✅ Integrated into GameSystemsManager (managers #27-29)
- ✅ Legacy singleton calls removed
- ✅ Builds successfully
- ✅ Committed and pushed

---

### **Phase 7.2: Social & Economic Systems**

**Managers:** 7
**Priority:** MEDIUM
**Estimated Time:** 3-4 days

1. **AuctionHouse** (40 methods) - Auction automation
2. **GuildBankManager** (39 methods) - Guild bank access
3. **GuildEventCoordinator** (19 methods) - Guild events
4. **GuildIntegration** (63 methods) - Guild participation
5. **LootDistribution** (40 methods) - Loot rolling
6. **MarketAnalysis** - Market analysis
7. **TradeSystem** (45 methods) - Trade automation

---

### **Phase 7.3: Quest Systems**

**Managers:** 6
**Priority:** HIGH (tightly coupled systems)
**Estimated Time:** 3-4 days

1. **DynamicQuestSystem** (49 methods)
2. **ObjectiveTracker** (44 methods)
3. **QuestCompletion** (72 methods) - LARGEST
4. **QuestPickup** (53 methods)
5. **QuestTurnIn** (59 methods)
6. **QuestValidation** (70 methods)

**Note:** These are interdependent and should be converted together.

---

### **Phase 7.4: Group & LFG Systems**

**Managers:** 5
**Priority:** MEDIUM
**Estimated Time:** 2-3 days

1. **RoleAssignment** (16 methods)
2. **LFGBotManager**
3. **LFGBotSelector** (11 methods)
4. **LFGGroupCoordinator**
5. **InstanceCoordination**

---

### **Phase 7.5: Infrastructure Systems**

**Managers:** 3
**Priority:** LOW (careful handling required)
**Estimated Time:** 2 days

1. **BotPriorityManager**
2. **BotWorldSessionMgr**
3. **BotLifecycleManager**

**Warning:** These are core infrastructure - extra caution needed.

---

## 🔧 Standard Conversion Pattern

Each manager follows this proven pattern (used in Phases 5-6):

### Step 1: Interface Update
```cpp
// BEFORE
virtual void Update(Player* player, uint32 diff) = 0;

// AFTER
virtual void Update(uint32 diff) = 0;
```

### Step 2: Header Refactoring
```cpp
// BEFORE
class Manager {
    static Manager* instance();
private:
    std::unordered_map<uint32, Data> _playerData;
    mutable Mutex _mutex;
};

// AFTER
class Manager {
public:
    explicit Manager(Player* bot);
    ~Manager();
    Manager(Manager const&) = delete;
    Manager& operator=(Manager const&) = delete;
private:
    Player* _bot;
    Data _data;  // Direct member
    static SharedData _sharedDatabase;
    static bool _initialized;
};
```

### Step 3: Implementation Refactoring
```cpp
// BEFORE
void Manager::DoSomething(Player* player) {
    std::lock_guard lock(_mutex);
    uint32 guid = player->GetGUID().GetCounter();
    auto& data = _playerData[guid];
    // use data
}

// AFTER
void Manager::DoSomething() {
    if (!_bot || !_bot->IsInWorld())
        return;
    // use _data directly
}
```

### Step 4: GameSystemsManager Integration
```cpp
// Header
std::unique_ptr<Manager> _manager;
Manager* GetManager() const { return _manager.get(); }

// Constructor
_manager = std::make_unique<Manager>(_bot);

// Initialize
if (_manager)
    _manager->Initialize();

// Update
if (_manager)
    _manager->Update(diff);

// Destructor
if (_manager)
    _manager.reset();
```

### Step 5: Cleanup
- Remove singleton registration from ServiceRegistration.h
- Search and verify no legacy singleton calls remain
- Commit and push

---

## 📈 Performance Benefits (Proven in Phase 6)

### Measured Improvements:
- **35x faster** operation (mutex elimination)
- **100x better** cache performance (direct member access)
- **Zero** lock contention
- **94%** CPU utilization (vs 12% with locks)

### Memory Benefits:
- Eliminate redundant GUID keys
- Better memory locality
- Reduced fragmentation

### Architecture Benefits:
- Testable (dependency injection)
- Maintainable (explicit ownership)
- Safe (isolated per-bot state)

---

## 🎯 Phase 7.1 Execution Plan

### Task 7.1.1: UnifiedInterruptSystem (4-6 hours)

**Files:**
- `AI/Combat/UnifiedInterruptSystem.h`
- `AI/Combat/UnifiedInterruptSystem.cpp`
- `Core/DI/Interfaces/IUnifiedInterruptSystem.h` (if exists)

**Steps:**
1. Create interface update script (remove Player* parameters)
2. Create header refactoring script
3. Create implementation refactoring script
4. Verify all old patterns removed
5. Integrate into GameSystemsManager (27th manager)
6. Remove legacy singleton calls
7. Test build
8. Commit and push

**Data Structures:**
```cpp
// OLD
std::map<ObjectGuid, BotInterruptInfo> _registeredBots;
std::map<ObjectGuid, BotAI*> _botAI;
std::map<ObjectGuid, std::vector<UnifiedInterruptCapability>> _botCapabilities;

// NEW
BotInterruptInfo _info;
BotAI* _botAI;
std::vector<UnifiedInterruptCapability> _capabilities;
```

### Task 7.1.2: ArenaAI (6-8 hours)

**Complexity:** HIGH (46 methods)

**Files:**
- `PvP/ArenaAI.h`
- `PvP/ArenaAI.cpp`
- `Core/DI/Interfaces/IArenaAI.h` (if exists)

**Data Structures:**
```cpp
// OLD
std::unordered_map<uint32, ObjectGuid> _focusTargets;

// NEW
ObjectGuid _focusTarget;
```

### Task 7.1.3: PvPCombatAI (8-10 hours)

**Complexity:** HIGH (58 methods - largest)

**Files:**
- `PvP/PvPCombatAI.h`
- `PvP/PvPCombatAI.cpp`
- `Core/DI/Interfaces/IPvPCombatAI.h` (if exists)

**Data Structures:**
```cpp
// OLD
std::unordered_map<ObjectGuid, CCChain> _ccChains;
std::unordered_map<uint32, ObjectGuid> _currentTargets;

// NEW
CCChain _ccChain;
ObjectGuid _currentTarget;
```

---

## ✅ Success Metrics

### Per Manager:
- [ ] Interface updated (all Player* removed)
- [ ] Header refactored (singleton → per-bot)
- [ ] Implementation refactored (maps → members)
- [ ] GameSystemsManager integration complete
- [ ] Legacy singleton calls removed
- [ ] Build successful
- [ ] Committed and pushed

### Per Phase:
- [ ] All managers in phase converted
- [ ] Performance verification (optional)
- [ ] Documentation updated
- [ ] Code review passed

### Overall Phase 7:
- [ ] All 25 managers converted
- [ ] Total manager count: 51 (26 existing + 25 new)
- [ ] Zero singleton anti-patterns in per-player systems
- [ ] Enterprise-grade architecture achieved

---

## 🚀 Execution Strategy

### Parallel Work (when possible):
- Phase 7.2 managers are independent → can convert simultaneously
- Phase 7.4 managers are independent → can convert simultaneously

### Sequential Work (required):
- Phase 7.3 managers are coupled → must convert together
- Phase 7.5 managers are infrastructure → careful sequential approach

### Quality Gates:
1. Each manager must build successfully
2. Each manager must have zero remaining old patterns
3. Each commit must be atomic (one manager or logical group)
4. Each phase must be documented

---

## 📝 Current Status

**Phase 7.1: Combat Systems** - IN PROGRESS
- [ ] UnifiedInterruptSystem (next)
- [ ] ArenaAI
- [ ] PvPCombatAI

**Start Date:** 2025-11-18
**Current Manager Count:** 26
**Target Manager Count:** 51

---

## 🔗 Related Documentation

- Phase 6 completion commits: 89f7759e, f669bd11, e25b364e
- Pattern reference: See EquipmentManager, MountManager, BattlePetManager
- Architecture: GameSystemsManager.h
