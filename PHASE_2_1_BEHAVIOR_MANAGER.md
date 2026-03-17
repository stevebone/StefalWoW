# Phase 2.1: Create BehaviorManager Base Class

**Duration**: 1 week (2025-01-13 to 2025-01-20)
**Status**: ⏳ PENDING
**Owner**: Development Team

---

## Objectives

Create a unified base class for all bot behavior managers, establishing:
1. Common throttling mechanism
2. Standardized lifecycle management
3. Fast state query interface
4. Performance tracking infrastructure

---

## Background

### Current Problem
- Automation singletons (QuestAutomation, TradeAutomation, etc.) cause 50-100ms blocking
- No standardized pattern across managers
- Each manager implements its own throttling differently
- No performance monitoring

### Solution
- Single BehaviorManager base class
- All managers inherit and follow same pattern
- Built-in throttling reduces CPU by 95%+
- Standardized performance tracking

---

## Technical Requirements

### Performance Constraints
- Manager Update(): 50-100ms BUT throttled to run every 1-10 seconds
- Amortized cost: <1ms per frame per manager
- State queries: <0.001ms (atomic reads, no locks)
- Memory: <1KB overhead per manager instance

### Thread Safety
- Manager owned by single BotAI (no sharing between bots)
- No locks needed for manager state (single-threaded access)
- Atomic state flags for fast queries from strategies

### TrinityCore Integration
- Must not depend on TrinityCore internals
- Player* and BotAI* passed as constructor parameters
- Uses TrinityCore timing (getMSTime())

---

## Deliverables

### 1. BehaviorManager.h Header File
Location: `src/modules/Playerbot/AI/BehaviorManager.h`

**Content**: (See cpp-architecture-optimizer agent output)

### 2. BehaviorManager.cpp Implementation
Location: `src/modules/Playerbot/AI/BehaviorManager.cpp`

**Content**: Base implementation of common methods

### 3. Example: QuestManager Refactor
Location: `src/modules/Playerbot/Game/QuestManager.h`

**Content**: Show QuestManager inheriting from BehaviorManager

### 4. Unit Tests
Location: `tests/unit/AI/BehaviorManagerTest.cpp`

**Test Coverage**:
- Throttling works correctly (doesn't call Update() too frequently)
- State queries return correct values
- Performance metrics track correctly
- Lifecycle methods called in correct order

### 5. Documentation
Location: `docs/BEHAVIOR_MANAGER_GUIDE.md`

**Content**:
- How to create a new manager
- Throttling best practices
- Performance guidelines
- Migration guide from Automation singletons

---

## Implementation Steps

### Step 1: Create BehaviorManager Base Class (2 days)
1. Write BehaviorManager.h with full documentation
2. Write BehaviorManager.cpp with base implementation
3. Add to CMakeLists.txt
4. Compile and verify no errors

### Step 2: Write Unit Tests (1 day)
1. Create BehaviorManagerTest.cpp
2. Test throttling mechanism
3. Test state queries
4. Test lifecycle methods
5. All tests must pass

### Step 3: Create Example Manager (2 days)
1. Pick simplest manager (e.g., GatheringManager)
2. Refactor to inherit from BehaviorManager
3. Implement required virtual methods
4. Test with dummy bot
5. Verify performance improvement

### Step 4: Documentation (1 day)
1. Write developer guide
2. Document throttling mechanism
3. Provide migration examples
4. Review and polish

### Step 5: Code Review & Polish (1 day)
1. Internal code review
2. Address feedback
3. Final testing
4. Merge to feature branch

---

## Success Criteria

### Functional
- ✅ BehaviorManager class compiles without errors
- ✅ All unit tests pass
- ✅ Example manager (GatheringManager) works correctly
- ✅ Throttling prevents excessive Update() calls

### Performance
- ✅ State queries take <0.001ms
- ✅ Throttled update amortizes to <1ms per frame
- ✅ No memory leaks detected
- ✅ 100+ bots run without lag

### Code Quality
- ✅ Full documentation in header
- ✅ All public methods documented
- ✅ Follows TrinityCore coding conventions
- ✅ No compiler warnings

---

## Dependencies

### Requires
- Phase 1 complete (architecture approved)

### Blocks
- Phase 2.4 (Manager refactoring requires this base class)
- Phase 2.5 (IdleStrategy needs manager state queries)

---

## Risk Mitigation

### Risk: Performance regression
**Mitigation**: Benchmark before/after with profiler

### Risk: Breaking existing managers
**Mitigation**: Refactor one manager at a time, test incrementally

### Risk: Throttling too aggressive
**Mitigation**: Make update intervals configurable per manager

---

## Agent Tasks

### cpp-architecture-optimizer Agent
**Task**: Design complete BehaviorManager base class
**Status**: ⏳ RUNNING
**Expected Output**: Production-ready BehaviorManager.h/cpp

---

## Next Phase

After completion, proceed to **Phase 2.2: Create CombatMovementStrategy**

---

**Last Updated**: 2025-01-13
**Next Review**: 2025-01-20
