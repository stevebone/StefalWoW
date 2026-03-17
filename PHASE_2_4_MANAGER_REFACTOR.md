# Phase 2.4: Refactor Managers - Remove Automation Singletons

**Duration**: 2 weeks (2025-02-03 to 2025-02-17)
**Status**: ⏳ PENDING
**Owner**: Development Team

---

## Objectives

Replace all singleton Automation classes with per-bot Manager instances:
1. Delete QuestAutomation, TradeAutomation, AuctionAutomation, GatheringAutomation
2. Create/refactor managers to inherit from BehaviorManager base class
3. Integrate managers into BotAI lifecycle
4. Eliminate 50-100ms blocking updates
5. Achieve <1ms amortized cost per manager

---

## Background

### Current Problem: Singleton Automation Classes

**QuestAutomation.h** (324 lines):
```cpp
class QuestAutomation
{
public:
    static QuestAutomation* instance();  // ❌ Singleton pattern
    void UpdateBotAutomation(Player* bot, uint32 diff);  // 50-100ms blocking

private:
    QuestAutomation() = default;
    static QuestAutomation* _instance;
};
```

**Problems**:
1. **Performance**: Each Update() call takes 50-100ms
2. **Scalability**: Singleton shared state doesn't scale to 5000 bots
3. **Duplication**: QuestAutomation duplicates QuestManager functionality
4. **Blocking**: IdleStrategy calls these every frame, causing lag
5. **Architecture**: Violates per-bot instance pattern

**Current Files to Delete** (~2000 lines):
- `Quest/QuestAutomation.h` (324 lines)
- `Quest/QuestAutomation.cpp` (estimated 500 lines)
- `Social/TradeAutomation.h` (estimated 200 lines)
- `Social/TradeAutomation.cpp` (estimated 400 lines)
- `Social/AuctionAutomation.h` (estimated 200 lines)
- `Social/AuctionAutomation.cpp` (estimated 400 lines)
- `Professions/GatheringAutomation.h` (estimated 150 lines)
- `Professions/GatheringAutomation.cpp` (estimated 300 lines)

### Solution: Per-Bot Manager Instances

**New Pattern** (all inherit from BehaviorManager):
```cpp
// Base class from Phase 2.1
class BehaviorManager
{
public:
    BehaviorManager(Player* bot, BotAI* ai, uint32 updateInterval);
    virtual ~BehaviorManager() = default;

    // Throttled update (called by BotAI::UpdateManagers)
    void Update(uint32 diff);

    // Fast state queries (atomic, no locks)
    bool IsActive() const { return _active.load(); }

protected:
    virtual void OnUpdate(uint32 diff) = 0;  // Subclass implements

    Player* _bot;
    BotAI* _ai;
    std::atomic<bool> _active{false};
    uint32 _updateInterval;
    uint32 _lastUpdate = 0;
};

// Example manager
class QuestManager : public BehaviorManager
{
public:
    QuestManager(Player* bot, BotAI* ai)
        : BehaviorManager(bot, ai, 2000)  // Update every 2 seconds
    {}

    bool IsQuestingActive() const { return _hasActiveQuests.load(); }

protected:
    void OnUpdate(uint32 diff) override
    {
        // Do heavyweight quest logic here (runs every 2 seconds)
        _hasActiveQuests.store(bot->GetQuestCount() > 0);
    }

private:
    std::atomic<bool> _hasActiveQuests{false};
};
```

**Benefits**:
- ✅ Per-bot instance (no shared state)
- ✅ Throttled updates (2s interval = 0.05ms amortized)
- ✅ Fast state queries (atomic, <0.001ms)
- ✅ Unified pattern (all managers work the same way)
- ✅ Scalable to 5000+ bots

---

## Technical Requirements

### Manager Specifications

**QuestManager**:
- Update interval: 2000ms (2 seconds)
- Responsibilities: Quest pickup, turn-in, objective tracking
- State queries: IsQuestingActive(), HasActiveQuest(questId)
- Already exists at `Game/QuestManager.h` - needs refactoring

**TradeManager** (NEW):
- Update interval: 5000ms (5 seconds)
- Responsibilities: Item trading, vendor interactions, repair
- State queries: IsTradingActive(), NeedsRepair()
- Replaces: TradeAutomation

**AuctionManager**:
- Update interval: 10000ms (10 seconds)
- Responsibilities: Auction house interactions, buying/selling
- State queries: HasActiveAuctions(), IsAtAuctionHouse()
- Already exists at `Economy/AuctionManager.h` - needs refactoring

**GatheringManager** (NEW):
- Update interval: 1000ms (1 second)
- Responsibilities: Herb/ore detection, gathering execution
- State queries: IsGathering(), HasNearbyResources()
- Replaces: GatheringAutomation

### Performance Requirements
- Manager::Update() amortized cost: <0.2ms per manager
- Manager state query: <0.001ms (atomic read)
- Total manager overhead: <0.8ms per bot per frame (4 managers × 0.2ms)
- Memory overhead: <4KB per bot (all managers combined)

### Integration with BotAI

**New UpdateManagers() Method**:
```cpp
void BotAI::UpdateManagers(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // Each manager throttles itself internally
    if (_questManager)
        _questManager->Update(diff);

    if (_tradeManager)
        _tradeManager->Update(diff);

    if (_auctionManager)
        _auctionManager->Update(diff);

    if (_gatheringManager)
        _gatheringManager->Update(diff);
}
```

---

## Deliverables

### 1. Refactor QuestManager
Location: `src/modules/Playerbot/Game/QuestManager.h`

**Changes Required**:
```cpp
// BEFORE: Custom implementation
class QuestManager
{
public:
    QuestManager(Player* bot, BotAI* ai);
    void Update(uint32 diff);  // No throttling
    // ... methods ...
};

// AFTER: Inherits from BehaviorManager
class QuestManager : public BehaviorManager
{
public:
    QuestManager(Player* bot, BotAI* ai);

    // Fast state queries
    bool IsQuestingActive() const { return _hasActiveQuests.load(); }
    bool HasActiveQuest(uint32 questId) const;
    uint32 GetActiveQuestCount() const { return _activeQuestCount.load(); }

protected:
    void OnUpdate(uint32 diff) override;

private:
    std::atomic<bool> _hasActiveQuests{false};
    std::atomic<uint32> _activeQuestCount{0};
    // ... existing quest tracking data ...
};
```

### 2. Create TradeManager
Location: `src/modules/Playerbot/Social/TradeManager.h` (NEW)

**Full Implementation**:
```cpp
#pragma once

#include "AI/BehaviorManager.h"

namespace Playerbot
{

class TradeManager : public BehaviorManager
{
public:
    TradeManager(Player* bot, BotAI* ai);
    ~TradeManager() override = default;

    // State queries
    bool IsTradingActive() const { return _isTrading.load(); }
    bool NeedsRepair() const { return _needsRepair.load(); }
    bool NeedsSupplies() const { return _needsSupplies.load(); }

protected:
    void OnUpdate(uint32 diff) override;

private:
    // Trade logic
    void CheckRepairStatus();
    void CheckSupplyStatus();
    void FindNearbyVendors();
    void ExecuteTrade();

    // State
    std::atomic<bool> _isTrading{false};
    std::atomic<bool> _needsRepair{false};
    std::atomic<bool> _needsSupplies{false};
    ObjectGuid _currentVendor;
};

} // namespace Playerbot
```

### 3. Refactor AuctionManager
Location: `src/modules/Playerbot/Economy/AuctionManager.h`

**Changes Required**: Similar to QuestManager refactor
- Inherit from BehaviorManager
- Add atomic state flags
- Implement OnUpdate() with 10-second throttling
- Add fast state query methods

### 4. Create GatheringManager
Location: `src/modules/Playerbot/Professions/GatheringManager.h` (NEW)

**Full Implementation**:
```cpp
#pragma once

#include "AI/BehaviorManager.h"

namespace Playerbot
{

class GatheringManager : public BehaviorManager
{
public:
    GatheringManager(Player* bot, BotAI* ai);
    ~GatheringManager() override = default;

    // State queries
    bool IsGathering() const { return _isGathering.load(); }
    bool HasNearbyResources() const { return _hasNearbyResources.load(); }

protected:
    void OnUpdate(uint32 diff) override;

private:
    // Gathering logic
    void ScanForResources();
    void SelectBestResource();
    void MoveToResource();
    void GatherResource();

    // State
    std::atomic<bool> _isGathering{false};
    std::atomic<bool> _hasNearbyResources{false};
    ObjectGuid _currentResource;
    std::vector<ObjectGuid> _nearbyHerbs;
    std::vector<ObjectGuid> _nearbyOres;
};

} // namespace Playerbot
```

### 5. Integrate Managers into BotAI
Location: `src/modules/Playerbot/AI/BotAI.h`

**Add Manager Members**:
```cpp
class BotAI
{
public:
    // ... existing methods ...

    // Manager access
    QuestManager* GetQuestManager() const { return _questManager.get(); }
    TradeManager* GetTradeManager() const { return _tradeManager.get(); }
    AuctionManager* GetAuctionManager() const { return _auctionManager.get(); }
    GatheringManager* GetGatheringManager() const { return _gatheringManager.get(); }

private:
    // NEW: Manager update method
    void UpdateManagers(uint32 diff);

    // Manager instances
    std::unique_ptr<QuestManager> _questManager;
    std::unique_ptr<TradeManager> _tradeManager;
    std::unique_ptr<AuctionManager> _auctionManager;
    std::unique_ptr<GatheringManager> _gatheringManager;
};
```

**Modify BotAI::UpdateAI()**:
```cpp
void BotAI::UpdateAI(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // PHASE 1: Manager updates (heavyweight, throttled) ✅ NEW
    UpdateManagers(diff);

    // PHASE 2: Strategy updates (lightweight, every frame)
    UpdateStrategies(diff);

    // PHASE 3: Movement execution
    UpdateMovement(diff);

    // PHASE 4: Combat specialization
    if (IsInCombat())
        OnCombatUpdate(diff);
}
```

### 6. Delete Automation Files

**Remove from filesystem**:
```bash
cd src/modules/Playerbot
rm Quest/QuestAutomation.h
rm Quest/QuestAutomation.cpp
rm Social/TradeAutomation.h
rm Social/TradeAutomation.cpp
rm Social/AuctionAutomation.h
rm Social/AuctionAutomation.cpp
rm Professions/GatheringAutomation.h
rm Professions/GatheringAutomation.cpp
```

**Remove from CMakeLists.txt**:
```cmake
# Remove these lines:
src/modules/Playerbot/Quest/QuestAutomation.cpp
src/modules/Playerbot/Social/TradeAutomation.cpp
src/modules/Playerbot/Social/AuctionAutomation.cpp
src/modules/Playerbot/Professions/GatheringAutomation.cpp
```

**Find and remove all includes**:
```bash
grep -r "QuestAutomation.h" --include="*.cpp" --include="*.h" src/modules/Playerbot/
grep -r "TradeAutomation.h" --include="*.cpp" --include="*.h" src/modules/Playerbot/
grep -r "AuctionAutomation.h" --include="*.cpp" --include="*.h" src/modules/Playerbot/
grep -r "GatheringAutomation.h" --include="*.cpp" --include="*.h" src/modules/Playerbot/
```

**Files to update**:
- `AI/Strategy/IdleStrategy.cpp` (remove all Automation includes)
- Any other files that reference Automation classes

### 7. Unit Tests
Location: `tests/unit/Managers/`

**Test Files**:
- `QuestManagerTest.cpp` - Throttling, state queries
- `TradeManagerTest.cpp` - Repair detection, vendor finding
- `AuctionManagerTest.cpp` - Auction tracking, bid logic
- `GatheringManagerTest.cpp` - Resource detection, gathering flow

### 8. Integration Tests
Location: `tests/integration/ManagerIntegrationTest.cpp`

**Test Scenarios**:
- All 4 managers update correctly with 100 bots
- Manager state queries are fast (<0.001ms)
- Managers don't interfere with each other
- Performance is <0.8ms total per bot
- No memory leaks after 10000 updates

### 9. Documentation
Location: `docs/MANAGER_SYSTEM_GUIDE.md`

**Content**:
- Manager pattern explanation
- How to create a new manager
- Throttling mechanism details
- State query best practices
- Migration guide from Automation classes
- Performance tuning guide

---

## Implementation Steps

### Week 1: Create New Managers (Days 1-7)

**Day 1-2: Refactor QuestManager**
1. Add BehaviorManager inheritance
2. Implement OnUpdate() with throttling
3. Add atomic state flags
4. Test with 100 bots

**Day 3-4: Create TradeManager**
1. Write TradeManager.h header
2. Implement TradeManager.cpp
3. Add repair detection logic
4. Add vendor finding logic
5. Test basic functionality

**Day 5-6: Create GatheringManager**
1. Write GatheringManager.h header
2. Implement GatheringManager.cpp
3. Add resource scanning logic
4. Add gathering execution logic
5. Test herb/ore gathering

**Day 7: Refactor AuctionManager**
1. Add BehaviorManager inheritance
2. Implement OnUpdate() with throttling
3. Add atomic state flags
4. Test auction tracking

### Week 2: Integration and Cleanup (Days 8-14)

**Day 8-9: Integrate into BotAI**
1. Add manager members to BotAI.h
2. Implement UpdateManagers() method
3. Initialize managers in BotAI constructor
4. Modify UpdateAI() to call UpdateManagers()
5. Compile and test basic integration

**Day 10: Delete Automation Files**
1. Find all Automation includes
2. Remove includes from all files
3. Delete Automation source files
4. Update CMakeLists.txt
5. Verify compilation

**Day 11: Write Tests**
1. Unit tests for all 4 managers
2. Integration test for manager system
3. Performance benchmark tests
4. All tests must pass

**Day 12-13: Performance Testing**
1. Benchmark with 100 bots
2. Benchmark with 500 bots
3. Benchmark with 1000 bots
4. Profile manager updates
5. Optimize if needed

**Day 14: Documentation**
1. Write manager system guide
2. Create migration guide
3. Update architecture documentation
4. Final code review

---

## Success Criteria

### Functional Requirements
- ✅ All 4 managers inherit from BehaviorManager
- ✅ No Automation singleton classes remain
- ✅ QuestManager refactored successfully
- ✅ TradeManager created and working
- ✅ GatheringManager created and working
- ✅ AuctionManager refactored successfully
- ✅ All managers integrated into BotAI
- ✅ UpdateManagers() called every frame

### Performance Requirements
- ✅ QuestManager amortized cost <0.2ms
- ✅ TradeManager amortized cost <0.2ms
- ✅ AuctionManager amortized cost <0.2ms
- ✅ GatheringManager amortized cost <0.2ms
- ✅ Total manager overhead <0.8ms per bot
- ✅ State queries <0.001ms (atomic)
- ✅ 100 bots run without lag
- ✅ 500 bots run with <5% CPU increase

### Code Quality
- ✅ All managers follow same pattern
- ✅ Full documentation for all managers
- ✅ Comprehensive test coverage
- ✅ No compiler warnings
- ✅ No memory leaks
- ✅ Clean separation of concerns

### Cleanup
- ✅ 8 Automation files deleted (~2000 lines)
- ✅ All Automation includes removed
- ✅ CMakeLists.txt updated
- ✅ No dead code remaining
- ✅ Clean compilation

---

## Dependencies

### Requires
- Phase 2.1 complete (BehaviorManager base class)
- Phase 2.2 complete (CombatMovementStrategy - for context)
- Phase 2.3 complete (Universal ClassAI - for context)

### Blocks
- Phase 2.5 (IdleStrategy needs manager state queries)
- Phase 2.6 (Integration testing needs all managers working)
- Phase 2.7 (Cleanup needs managers refactored first)

---

## Risk Mitigation

### Risk: Breaking existing functionality
**Mitigation**:
- Refactor one manager at a time
- Test after each refactor
- Keep parallel implementations temporarily if needed

### Risk: Performance regression
**Mitigation**:
- Benchmark before/after each manager
- Profile with 500+ bots
- Adjust throttling intervals if needed

### Risk: Missing Automation functionality
**Mitigation**:
- Compare Automation vs Manager methods line-by-line
- Port all important logic
- Create migration checklist

### Risk: Difficult debugging
**Mitigation**:
- Add comprehensive logging during development
- Use performance monitoring
- Create debug commands for manager state

---

## Migration Checklist

### For Each Manager:
- [ ] List all methods from Automation class
- [ ] Implement equivalent methods in Manager class
- [ ] Add throttling to heavy operations
- [ ] Add atomic state flags for queries
- [ ] Write unit tests
- [ ] Test with 100 bots
- [ ] Document new API
- [ ] Remove Automation class
- [ ] Update all references

### QuestManager Migration:
- [ ] QuestAutomation::UpdateBotAutomation → QuestManager::OnUpdate
- [ ] Quest pickup logic ported
- [ ] Quest turn-in logic ported
- [ ] Objective tracking ported
- [ ] State queries added (IsQuestingActive, etc.)

### TradeManager Migration:
- [ ] TradeAutomation::UpdatePlayerAutomation → TradeManager::OnUpdate
- [ ] Repair logic ported
- [ ] Vendor finding ported
- [ ] Item selling logic ported
- [ ] State queries added (NeedsRepair, etc.)

### GatheringManager Migration:
- [ ] GatheringAutomation::Update → GatheringManager::OnUpdate
- [ ] Resource scanning ported
- [ ] Gathering execution ported
- [ ] Profession checks ported
- [ ] State queries added (IsGathering, etc.)

### AuctionManager Migration:
- [ ] AuctionAutomation::UpdatePlayerAutomation → AuctionManager::OnUpdate
- [ ] Auction tracking ported
- [ ] Bid logic ported
- [ ] Sale logic ported
- [ ] State queries added (HasActiveAuctions, etc.)

---

## Next Phase

After completion, proceed to **Phase 2.5: Update IdleStrategy (Observer Pattern)**

---

**Last Updated**: 2025-01-13
**Next Review**: 2025-02-17
