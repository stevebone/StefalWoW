# Playerbot Module Cleanup & Streamlining Plan (2025)

**Version:** 1.0
**Date:** 2025-11-17
**Status:** Planning Phase
**Estimated Duration:** 14-16 weeks (3.5-4 months)
**Team Size:** 2-3 developers

---

## Executive Summary

After extensive implementation across 5+ development phases, the playerbot module has accumulated significant architectural debt. This cleanup plan addresses:

- **922 source files** with 45% containing commented legacy code
- **40+ duplicate "Refactored" class files** (~8,000 duplicate lines)
- **105+ manager classes** without unified interface
- **Multiple duplicate systems** (2 GroupCoordinators, 7 movement systems, 9 loot managers)
- **God class (BotAI)** with 73 direct dependencies

**Expected Results:**
- Reduce codebase by ~10,850 lines (2.7%)
- Remove 50+ duplicate/legacy files
- Reduce circular dependencies by 70%
- Decrease maintenance burden by 40%
- Improve build time by 8%

---

## Table of Contents

1. [Critical Issues Identified](#critical-issues-identified)
2. [Cleanup Phases](#cleanup-phases)
3. [Priority Consolidation Targets](#priority-consolidation-targets)
4. [Risk Assessment](#risk-assessment)
5. [Testing Strategy](#testing-strategy)
6. [Rollback Plan](#rollback-plan)
7. [Success Metrics](#success-metrics)

---

## Critical Issues Identified

### Issue 1: BotAI God Class (CRITICAL)
**Location:** `src/modules/Playerbot/AI/BotAI.h` (906 lines)

**Problem:**
- 73 files directly depend on BotAI
- Contains 15+ manager instances as direct members
- Handles 16+ separate responsibilities
- Makes testing, modification, and understanding extremely difficult

**Impact:** HIGH - Central bottleneck for all changes

### Issue 2: Duplicate GroupCoordinator Implementations (HIGH)
**Locations:**
- `src/modules/Playerbot/AI/Coordination/GroupCoordinator.h` (266 lines)
- `src/modules/Playerbot/Advanced/GroupCoordinator.h` (338 lines)

**Problem:**
- Two separate implementations with overlapping responsibilities
- BotAI holds BOTH coordinators simultaneously
- Duplicate role detection, group state tracking
- Conflicting target assignments

**Impact:** HIGH - Causes coordination bugs and confusion

### Issue 3: 40+ Refactored Legacy Files (HIGH)
**Pattern:** `*Refactored.h` files alongside originals

**Examples:**
- `ClassAI_Refactored.h` vs `ClassAI.h`
- `BloodDeathKnightRefactored.h` vs `BloodDeathKnight.h`
- (38 more similar pairs)

**Problem:**
- ~8,000 lines of duplicate specialization code
- Maintenance burden (must update both versions)
- Unclear which version is canonical

**Impact:** HIGH - Massive code duplication

### Issue 4: Vendor/NPC Interaction Scatter (MEDIUM)
**Locations:**
- `Game/VendorPurchaseManager`
- `Game/NPCInteractionManager`
- `Interaction/VendorInteractionManager`
- `Interaction/Core/InteractionManager`
- `Interaction/Vendors/VendorInteraction.h`
- `Social/VendorInteraction.h` (DUPLICATE)

**Problem:** 6 files handling vendor interaction with unclear hierarchy

**Impact:** MEDIUM - Causes confusion and potential bugs

### Issue 5: 7 Separate Movement Systems (MEDIUM)
**Systems:**
1. `Movement/Arbiter/MovementArbiter`
2. `Movement/UnifiedMovementCoordinator`
3. `AI/Strategy/CombatMovementStrategy`
4. `AI/Combat/MovementIntegration`
5. `Movement/GroupFormationManager`
6. `Movement/LeaderFollowBehavior`
7. `BotMovementUtil`

**Problem:** No clear interaction patterns, duplicate logic

**Impact:** MEDIUM - Movement bugs difficult to debug

### Issue 6: 9 Loot Management Files (MEDIUM)
**Locations:**
- `AI/Strategy/LootStrategy.cpp` (2,485 lines)
- `Social/UnifiedLootManager`
- `Social/LootDistribution`
- `Social/LootCoordination`
- `Social/LootAnalysis`
- `Loot/LootEventBus`
- `Network/ParseLootPacket_Typed.cpp`
- (+ 2 DI interfaces)

**Problem:** Loot logic scattered without clear ownership

**Impact:** MEDIUM - Loot distribution bugs

### Issue 7: No Unified Manager Interface (LOW)
**Problem:**
- 105+ manager classes with inconsistent lifecycle methods
- Some have Initialize(), some don't
- Some have Reset(), some don't
- Makes ManagerRegistry difficult to implement properly

**Impact:** MEDIUM - Reduces reusability and testability

---

## Cleanup Phases

### PHASE 1: Foundation & Quick Wins (4 weeks)

**Goal:** Establish architectural foundation and remove obvious duplicates

#### Week 1: Establish Manager Base Class
**Effort:** 40 hours | **Risk:** LOW

**Tasks:**
1. Create `src/modules/Playerbot/Core/IManager.h` interface
   ```cpp
   class IManager {
   public:
       virtual void Initialize() = 0;
       virtual void Update(uint32 diff) = 0;
       virtual void Reset() = 0;
       virtual void Shutdown() = 0;
       virtual ~IManager() = default;
   };
   ```

2. Update `QuestManager` to inherit from `IManager`
3. Update `TradeManager` to inherit from `IManager`
4. Update remaining 103 managers incrementally

**Success Criteria:**
- [ ] All managers inherit from IManager
- [ ] ManagerRegistry can treat all managers uniformly
- [ ] Compilation succeeds
- [ ] Existing tests pass

**Files Modified:** 105+ manager files

---

#### Week 1.5: Unify GroupCoordinator (Part 1)
**Effort:** 30 hours | **Risk:** MEDIUM

**Tasks:**
1. Analysis phase:
   - Document all BotAI usages of both coordinators
   - Map responsibilities of each coordinator
   - Design unified interface

2. Create new unified coordinator:
   ```
   Advanced/GroupCoordinator (MAIN)
     ├─ TacticalCoordinator (interrupts, dispels, cooldowns)
     ├─ SocialCoordinator (quests, loot, invites)
     └─ RaidCoordinator (boss mechanics)
   ```

**Success Criteria:**
- [ ] Unified coordinator design documented
- [ ] New class structure created
- [ ] Compilation succeeds

**Files Created:**
- `Advanced/GroupCoordinator.h` (unified)
- `Advanced/TacticalCoordinator.h`

---

#### Week 2: Unify GroupCoordinator (Part 2)
**Effort:** 30 hours | **Risk:** MEDIUM

**Tasks:**
1. Migrate all references from `Coordination::GroupCoordinator` to new unified version
2. Update BotAI to use single coordinator:
   ```cpp
   // Before:
   std::unique_ptr<GroupCoordinator> _groupCoordinator;
   std::unique_ptr<Coordination::GroupCoordinator> _tacticalCoordinator;

   // After:
   std::unique_ptr<Advanced::GroupCoordinator> _groupCoordinator;
   ```

3. Remove old `Coordination::GroupCoordinator` files

**Success Criteria:**
- [ ] BotAI uses only one GroupCoordinator
- [ ] All 73 dependent files updated
- [ ] Group coordination tests pass
- [ ] No regression in group behavior

**Files Removed:**
- `AI/Coordination/GroupCoordinator.h`
- `AI/Coordination/GroupCoordinator.cpp`

**Lines Saved:** ~100 lines

---

#### Week 3: Consolidate Vendor/NPC Interaction
**Effort:** 40 hours | **Risk:** LOW

**Tasks:**
1. Choose `Interaction/Core/InteractionManager` as central hub
2. Merge vendor logic from 6 locations into unified hierarchy:
   ```
   InteractionManager (CORE)
     ├─ VendorInteractionManager
     ├─ NPCInteractionManager
     ├─ FlightMasterManager
     └─ QuestGiverManager
   ```

3. Remove duplicate `Social/VendorInteraction.h`
4. Update all vendor interaction calls to use InteractionManager

**Success Criteria:**
- [ ] Single vendor interaction entry point
- [ ] All vendor purchases route through InteractionManager
- [ ] Vendor interaction tests pass
- [ ] NPC gossip dialogs work correctly

**Files Removed:**
- `Social/VendorInteraction.h` (duplicate)
- `Interaction/Vendors/VendorInteraction.h` (merged)

**Lines Saved:** ~200 lines

---

#### Week 4: Remove Refactored Legacy Files
**Effort:** 20 hours | **Risk:** LOW

**Tasks:**
1. Audit which version is actually used for each class specialization:
   ```bash
   # For each *Refactored.h file:
   git grep -l "BloodDeathKnightRefactored"  # Check usage
   git grep -l "BloodDeathKnight[^R]"        # Check original usage
   ```

2. Choose canonical version (usually non-Refactored)
3. Migrate any unique logic from Refactored version to canonical
4. Remove all 40+ Refactored files:
   - `ClassAI_Refactored.h/.cpp`
   - `*Refactored.h` for all class specializations

**Success Criteria:**
- [ ] All class specializations have single implementation
- [ ] No references to Refactored versions remain
- [ ] All ClassAI tests pass
- [ ] Combat rotations work correctly

**Files Removed:** 40+ files

**Lines Saved:** ~8,000 lines

---

### PHASE 2: Movement System Consolidation (4 weeks)

**Goal:** Unify 7 movement systems into coherent 3-layer architecture

#### Week 5: Design Unified Movement Architecture
**Effort:** 40 hours | **Risk:** MEDIUM

**Tasks:**
1. Document current movement request flows
2. Design 3-layer architecture:
   ```
   Layer 1: MovementArbiter (priority-based request queue)
   Layer 2: MovementPlanner (pathfinding + positioning logic)
   Layer 3: MovementStrategies (high-level behavior)
   ```

3. Create movement request type hierarchy
4. Define interfaces for each layer

**Success Criteria:**
- [ ] Movement architecture documented
- [ ] Interface design reviewed
- [ ] Migration plan created

**Deliverables:**
- Architecture document
- Interface definitions
- Migration checklist

---

#### Week 6-7: Implement Unified Movement System
**Effort:** 80 hours | **Risk:** HIGH

**Tasks:**
1. Implement MovementArbiter:
   - Priority-based request queue
   - Conflict resolution
   - Request lifecycle management

2. Implement MovementPlanner:
   - Pathfinding integration
   - Combat positioning logic (from CombatMovementStrategy)
   - Formation logic (from GroupFormationManager)
   - Follow behavior (from LeaderFollowBehavior)

3. Create MovementStrategies:
   - FollowStrategy
   - CombatStrategy
   - FormationStrategy
   - PathfindingStrategy

4. Integrate with BotAI

**Success Criteria:**
- [ ] New movement system compiles
- [ ] Basic movement tests pass
- [ ] BotAI can submit movement requests

**Files Created:**
- `Movement/Core/MovementArbiter.h/.cpp`
- `Movement/Core/MovementPlanner.h/.cpp`
- `Movement/Strategies/FollowStrategy.h/.cpp`
- `Movement/Strategies/CombatStrategy.h/.cpp`
- `Movement/Strategies/FormationStrategy.h/.cpp`

---

#### Week 8: Movement System Migration & Testing
**Effort:** 40 hours | **Risk:** HIGH

**Tasks:**
1. Migrate all movement requests to new system
2. Remove old movement implementations:
   - `UnifiedMovementCoordinator`
   - `CombatMovementStrategy` (logic moved to MovementPlanner)
   - Duplicate formation/follow logic

3. Comprehensive testing:
   - Solo movement
   - Group formation
   - Combat positioning
   - Follow behavior
   - Pathfinding with obstacles

**Success Criteria:**
- [ ] All movement tests pass
- [ ] No regression in bot movement behavior
- [ ] Performance metrics within acceptable range
- [ ] Old movement files removed

**Files Removed:**
- `Movement/UnifiedMovementCoordinator.h/.cpp`
- `AI/Strategy/CombatMovementStrategy.cpp` (logic merged)
- Duplicate portions of GroupFormationManager

**Lines Saved:** ~1,000 lines

---

### PHASE 3: Event System Consolidation (3 weeks)

**Goal:** Unify 58 event bus implementations using generic template

#### Week 9: Implement Generic EventBus Template
**Effort:** 40 hours | **Risk:** MEDIUM

**Tasks:**
1. Create `Core/Events/EventBus.h`:
   ```cpp
   template<typename EventType>
   class EventBus {
   public:
       using Subscriber = std::function<void(const EventType&)>;

       void Subscribe(const std::string& subscriberId, Subscriber handler);
       void Unsubscribe(const std::string& subscriberId);
       void Publish(const EventType& event);

   private:
       std::unordered_map<std::string, Subscriber> _subscribers;
       std::mutex _mutex;
   };
   ```

2. Define type aliases for existing event buses:
   ```cpp
   using GroupEventBus = EventBus<GroupEvent>;
   using CombatEventBus = EventBus<CombatEvent>;
   using LootEventBus = EventBus<LootEvent>;
   // ... etc
   ```

3. Create event type definitions

**Success Criteria:**
- [ ] Generic EventBus compiles
- [ ] Type aliases created
- [ ] Basic subscription/publication works

**Files Created:**
- `Core/Events/EventBus.h`
- `Core/Events/EventTypes.h`

---

#### Week 10: Migrate Event Buses
**Effort:** 40 hours | **Risk:** MEDIUM

**Tasks:**
1. Migrate each event bus to use EventBus<T>:
   - GroupEventBus
   - CombatEventBus
   - LootEventBus
   - AuraEventBus
   - CooldownEventBus
   - QuestEventBus
   - (52 more)

2. Update subscribers to new pattern
3. Test event publication and handling

**Success Criteria:**
- [ ] All event buses use EventBus<T>
- [ ] Event handling tests pass
- [ ] No regression in event-driven behavior

**Files Modified:** 58 event bus files

**Lines Saved:** ~400 lines (duplication eliminated)

---

#### Week 11: Event System Testing & Documentation
**Effort:** 40 hours | **Risk:** LOW

**Tasks:**
1. Comprehensive event testing:
   - Event ordering
   - Subscriber notification
   - Concurrent event handling
   - Performance profiling

2. Update documentation:
   - Event flow diagrams
   - Subscription patterns
   - Best practices

3. Remove old phase-specific event documentation

**Success Criteria:**
- [ ] All event tests pass
- [ ] Performance within acceptable limits
- [ ] Documentation updated

**Deliverables:**
- Event system documentation
- Performance benchmarks
- Migration guide

---

### PHASE 4: BotAI Decoupling & Final Cleanup (3 weeks)

**Goal:** Reduce BotAI complexity and remove dead code

#### Week 12: Extract Game Systems Manager
**Effort:** 40 hours | **Risk:** MEDIUM

**Tasks:**
1. Create `IGameSystemsManager` interface:
   ```cpp
   class IGameSystemsManager {
   public:
       virtual QuestManager* GetQuestManager() = 0;
       virtual TradeManager* GetTradeManager() = 0;
       virtual GatheringManager* GetGatheringManager() = 0;
       virtual AuctionManager* GetAuctionManager() = 0;
       // ... etc
   };
   ```

2. Implement `GameSystemsManager`:
   ```cpp
   class GameSystemsManager : public IGameSystemsManager {
       // Owns all game system managers
   };
   ```

3. Update BotAI to hold single GameSystemsManager:
   ```cpp
   // Before: 15+ manager members
   std::unique_ptr<QuestManager> _questManager;
   std::unique_ptr<TradeManager> _tradeManager;
   // ... etc

   // After: 1 manager
   std::unique_ptr<IGameSystemsManager> _gameSystems;
   ```

**Success Criteria:**
- [ ] BotAI complexity reduced
- [ ] Manager access still works
- [ ] All tests pass

**Files Created:**
- `Core/Managers/IGameSystemsManager.h`
- `Core/Managers/GameSystemsManager.h/.cpp`

**Lines Removed from BotAI:** ~100 lines

---

#### Week 13: Remove Dead Code
**Effort:** 20 hours | **Risk:** LOW

**Tasks:**
1. Remove confirmed dead code files:
   - `ThreadSafeClassAI.h` (merge features into ClassAI)
   - `Interaction/Core/InteractionManager_COMPLETE_FIX.cpp` (backup file)
   - `CombatMovementStrategy_Integration.cpp` (documentation)

2. Remove commented code from 413 files:
   - Run automated cleanup for obvious dead code
   - Manual review for commented sections >20 lines

3. Remove old TODOs that are completed or obsolete

**Success Criteria:**
- [ ] Dead code files removed
- [ ] Commented code reduced from 45% to <25%
- [ ] Compilation succeeds
- [ ] No functionality lost

**Files Removed:** 3-5 dead code files

**Lines Saved:** ~500 lines

---

#### Week 14: Documentation Cleanup
**Effort:** 20 hours | **Risk:** LOW

**Tasks:**
1. Remove old phase documentation (94 files):
   - Keep only: Latest architecture overview
   - Remove: Phase-specific tracking files

2. Update main documentation:
   - `README.md` - Current architecture
   - `ARCHITECTURE.md` - Updated diagrams
   - `CONTRIBUTING.md` - Updated guidelines

3. Create migration guides:
   - Old → New GroupCoordinator
   - Old → New Movement System
   - Old → New Event System

**Success Criteria:**
- [ ] Old phase docs removed
- [ ] Main documentation updated
- [ ] Migration guides created

**Deliverables:**
- Updated documentation
- Migration guides
- Architecture diagrams

---

## Priority Consolidation Targets

### Priority 1: GroupCoordinator (MUST DO)
- **Effort:** 1.5 weeks
- **Impact:** HIGH
- **Risk:** MEDIUM
- **Lines Saved:** ~100
- **Rationale:** Duplicate implementations causing coordination bugs

### Priority 2: Refactored Files (MUST DO)
- **Effort:** 0.5 weeks
- **Impact:** HIGH
- **Risk:** LOW
- **Lines Saved:** ~8,000
- **Rationale:** Massive code duplication, easy to remove

### Priority 3: Movement System (SHOULD DO)
- **Effort:** 4 weeks
- **Impact:** HIGH
- **Risk:** HIGH
- **Lines Saved:** ~1,000
- **Rationale:** Current scatter causes movement bugs

### Priority 4: Event System (SHOULD DO)
- **Effort:** 3 weeks
- **Impact:** MEDIUM
- **Risk:** MEDIUM
- **Lines Saved:** ~400
- **Rationale:** Reduces duplication, improves maintainability

### Priority 5: Vendor/NPC Interaction (COULD DO)
- **Effort:** 1 week
- **Impact:** MEDIUM
- **Risk:** LOW
- **Lines Saved:** ~200
- **Rationale:** Reduces confusion, not critical

### Priority 6: Loot Management (COULD DO)
- **Effort:** 2 weeks
- **Impact:** MEDIUM
- **Risk:** MEDIUM
- **Lines Saved:** ~500
- **Rationale:** Already has UnifiedLootManager, needs routing

### Priority 7: BotAI Decoupling (NICE TO HAVE)
- **Effort:** 1 week
- **Impact:** MEDIUM
- **Risk:** MEDIUM
- **Lines Saved:** ~100
- **Rationale:** Improves architecture, not urgent

---

## Risk Assessment

### High Risk Items

#### 1. Movement System Refactor
**Risk Level:** HIGH
**Probability:** 40%
**Impact:** HIGH

**Risks:**
- Movement bugs difficult to detect
- Performance regressions possible
- Complex interdependencies with combat, groups, pathfinding

**Mitigation:**
- Comprehensive test suite before starting
- Incremental migration with feature flags
- Performance profiling at each step
- Keep old system until new system proven

#### 2. BotAI Decoupling
**Risk Level:** MEDIUM
**Probability:** 30%
**Impact:** MEDIUM

**Risks:**
- BotAI is referenced by 73 files
- Changes may break many systems
- Difficult to test in isolation

**Mitigation:**
- Make changes incrementally
- Use adapter pattern for compatibility
- Extensive integration testing
- Rollback plan ready

### Medium Risk Items

#### 3. GroupCoordinator Unification
**Risk Level:** MEDIUM
**Probability:** 25%
**Impact:** MEDIUM

**Risks:**
- Different coordinators may have subtle behavioral differences
- Group coordination is complex
- Raid mechanics may break

**Mitigation:**
- Document both implementations thoroughly
- Create compatibility layer initially
- Extensive group/raid testing
- Beta testing period

#### 4. Event System Migration
**Risk Level:** MEDIUM
**Probability:** 20%
**Impact:** LOW

**Risks:**
- Event ordering may change
- Some edge cases may not be covered
- Performance differences

**Mitigation:**
- Test event ordering thoroughly
- Performance benchmarks
- Gradual migration (one event type at a time)

### Low Risk Items

#### 5. Remove Refactored Files
**Risk Level:** LOW
**Probability:** 10%
**Impact:** LOW

**Risks:**
- May remove code that's still needed
- Some references may be missed

**Mitigation:**
- Comprehensive grep for all references
- Compilation will catch missing references
- High test coverage

#### 6. Documentation Cleanup
**Risk Level:** LOW
**Probability:** 5%
**Impact:** LOW

**Risks:**
- May remove useful historical context

**Mitigation:**
- Keep docs in git history
- Review before deletion
- Archive important docs

---

## Testing Strategy

### Test Levels

#### 1. Unit Tests
**Coverage:** Individual components

**Test Cases:**
- Manager lifecycle (Initialize, Update, Shutdown)
- Event publication and subscription
- Movement request prioritization
- Group role assignment
- Spell casting logic

**Tools:**
- Google Test framework
- Mock objects for dependencies

#### 2. Integration Tests
**Coverage:** Component interactions

**Test Cases:**
- BotAI → Manager interactions
- Event flow across multiple components
- Movement request → pathfinding → execution
- Group coordination across multiple bots
- Combat behavior integration

**Tools:**
- Integration test framework (existing)
- Test bots with controlled scenarios

#### 3. System Tests
**Coverage:** Full bot behavior

**Test Cases:**
- Solo questing
- Group dungeon runs
- Raid encounters
- PvP scenarios
- Auction house operations
- Crafting and gathering

**Tools:**
- Automated test scenarios
- Live server testing

#### 4. Performance Tests
**Coverage:** Resource usage and scalability

**Test Cases:**
- Memory usage per bot
- CPU usage with 100, 500, 1000 bots
- Movement performance
- Event handling throughput
- Database query performance

**Tools:**
- Profiling tools
- Performance benchmarks

### Test Execution Plan

**Before Each Phase:**
1. Run full test suite
2. Record baseline metrics
3. Create phase-specific tests

**During Each Phase:**
1. Run affected tests continuously
2. Monitor performance
3. Fix regressions immediately

**After Each Phase:**
1. Run full test suite
2. Compare metrics with baseline
3. Document any changes
4. Get approval before proceeding

### Test Coverage Goals

| Component | Current | Target | Priority |
|-----------|---------|--------|----------|
| Core AI | 75% | 85% | HIGH |
| Movement | 60% | 80% | HIGH |
| Combat | 70% | 80% | HIGH |
| Groups | 65% | 80% | HIGH |
| Events | 55% | 75% | MEDIUM |
| Managers | 50% | 70% | MEDIUM |
| Utilities | 40% | 60% | LOW |

---

## Rollback Plan

### Rollback Strategy

**Git Strategy:**
- Create feature branch for each phase
- Tag before each major change
- Keep main branch stable

**Rollback Triggers:**
- Critical bugs in production
- Performance degradation >10%
- Test failure rate >5%
- More than 2 days to fix

### Rollback Procedures

#### Phase 1 Rollback
**Trigger:** Manager base class causes issues

**Steps:**
1. Revert IManager interface
2. Restore original manager implementations
3. Rebuild and test
4. Estimated time: 2 hours

#### Phase 2 Rollback
**Trigger:** Movement system regression

**Steps:**
1. Re-enable old movement systems
2. Disable new MovementArbiter
3. Restore old movement calls
4. Rebuild and test
5. Estimated time: 4 hours

#### Phase 3 Rollback
**Trigger:** Event system issues

**Steps:**
1. Restore old event buses
2. Revert EventBus<T> changes
3. Update subscribers
4. Rebuild and test
5. Estimated time: 3 hours

#### Phase 4 Rollback
**Trigger:** BotAI issues after decoupling

**Steps:**
1. Restore direct manager members in BotAI
2. Remove GameSystemsManager
3. Update all references
4. Rebuild and test
5. Estimated time: 3 hours

### Rollback Testing
- Run full test suite after rollback
- Verify all functionality restored
- Document rollback reason
- Plan alternative approach

---

## Success Metrics

### Code Quality Metrics

| Metric | Before | Target | Measurement |
|--------|--------|--------|-------------|
| Source Files | 922 | 870 | `find . -name "*.cpp" -o -name "*.h" \| wc -l` |
| Lines of Code | ~400,000 | ~390,000 | `cloc src/modules/Playerbot` |
| Manager Classes | 105 | 95 | Manual count |
| Average Class Size | ~250 | ~230 | `cloc --by-file` |
| Commented Code % | 45% | 25% | Custom script |
| Circular Dependencies | HIGH | LOW | Dependency analysis tool |
| DI Interfaces | 89 | 60 | Manual count |
| Duplicate Files | 40 | 0 | Manual tracking |

### Performance Metrics

| Metric | Baseline | Target | Measurement |
|--------|----------|--------|-------------|
| Build Time | 100% | <95% | CI build time |
| Memory per Bot | X MB | <X MB | Memory profiler |
| CPU per Bot | X% | <X% | Performance profiler |
| Movement Latency | X ms | <X ms | Performance tests |
| Event Throughput | X/sec | ≥X/sec | Performance tests |

### Maintainability Metrics

| Metric | Before | Target | Measurement |
|--------|--------|--------|-------------|
| Files with TODOs | 77 (19%) | <50 (12%) | `grep -r "TODO\|FIXME"` |
| Average PR Review Time | X hours | -20% | GitHub metrics |
| New Developer Onboarding | X days | -30% | Survey |
| Bug Fix Time | X hours | -25% | Issue tracker |

### Test Coverage Metrics

| Component | Before | Target | Measurement |
|-----------|--------|--------|-------------|
| Overall Coverage | ~60% | 75% | Coverage tool |
| Core AI | 75% | 85% | Coverage tool |
| Movement | 60% | 80% | Coverage tool |
| Combat | 70% | 80% | Coverage tool |
| Integration Tests | 50 | 75 | Test count |

---

## Timeline & Resources

### Phase Timeline

```
Week 1-4:   Phase 1 (Foundation & Quick Wins)
Week 5-8:   Phase 2 (Movement System)
Week 9-11:  Phase 3 (Event System)
Week 12-14: Phase 4 (Final Cleanup)
```

### Resource Allocation

**Option 1: 3 Developers (Parallel)**
- Developer A: Phase 1 → Phase 4
- Developer B: Phase 2 (Movement)
- Developer C: Phase 3 (Events)
- **Duration:** 8 weeks

**Option 2: 2 Developers (Sequential)**
- Developer A: Phase 1, 3
- Developer B: Phase 1, 2, 4
- **Duration:** 14 weeks

**Option 3: 1 Developer (Sequential)**
- Single developer: All phases
- **Duration:** 16 weeks

### Recommended Approach

**Hybrid: 2 Developers, Partial Parallel**
- Both: Phase 1 (2 weeks, shared work)
- Dev A: Phase 2 (4 weeks, movement)
- Dev B: Phase 3 (3 weeks, events) → Phase 4 (3 weeks)
- **Total Duration:** 10-12 weeks

---

## Approval & Sign-off

### Phase 1 Approval
- [ ] Technical Lead Review
- [ ] Architecture Approval
- [ ] Test Plan Approved
- [ ] Risk Assessment Accepted

### Phase 2 Approval
- [ ] Phase 1 Complete
- [ ] Phase 1 Metrics Met
- [ ] Phase 2 Design Reviewed
- [ ] Movement Expert Consulted

### Phase 3 Approval
- [ ] Phase 2 Complete
- [ ] Phase 2 Metrics Met
- [ ] Event System Design Reviewed
- [ ] Performance Benchmarks Met

### Phase 4 Approval
- [ ] Phase 3 Complete
- [ ] Phase 3 Metrics Met
- [ ] BotAI Refactor Reviewed
- [ ] Documentation Complete

### Final Sign-off
- [ ] All Phases Complete
- [ ] All Tests Passing
- [ ] Performance Metrics Met
- [ ] Documentation Updated
- [ ] Code Review Complete
- [ ] Production Ready

---

## Appendix A: File Inventory

### Files to Remove (Phase 1)
```
src/modules/Playerbot/AI/ClassAI/ClassAI_Refactored.h
src/modules/Playerbot/AI/ClassAI/ClassAI_Refactored.cpp
src/modules/Playerbot/AI/ClassAI/ThreadSafeClassAI.h
src/modules/Playerbot/AI/ClassAI/DeathKnights/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/DemonHunters/*Refactored.h (2 files)
src/modules/Playerbot/AI/ClassAI/Druids/*Refactored.h (4 files)
src/modules/Playerbot/AI/ClassAI/Evokers/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/Hunters/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/Mages/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/Monks/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/Paladins/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/Priests/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/Rogues/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/Shamans/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/Warlocks/*Refactored.h (3 files)
src/modules/Playerbot/AI/ClassAI/Warriors/*Refactored.h (3 files)
src/modules/Playerbot/AI/Coordination/GroupCoordinator.h
src/modules/Playerbot/AI/Coordination/GroupCoordinator.cpp
src/modules/Playerbot/Social/VendorInteraction.h
src/modules/Playerbot/Interaction/Vendors/VendorInteraction.h
```

### Files to Remove (Phase 2)
```
src/modules/Playerbot/Movement/UnifiedMovementCoordinator.h
src/modules/Playerbot/Movement/UnifiedMovementCoordinator.cpp
src/modules/Playerbot/AI/Strategy/CombatMovementStrategy.cpp (merge logic first)
```

### Files to Remove (Phase 4)
```
src/modules/Playerbot/Interaction/Core/InteractionManager_COMPLETE_FIX.cpp
src/modules/Playerbot/AI/Combat/CombatMovementStrategy_Integration.cpp
docs/playerbot/PHASE*.md (90+ files, keep only latest)
```

### Files to Create

**Phase 1:**
```
src/modules/Playerbot/Core/IManager.h
src/modules/Playerbot/Advanced/TacticalCoordinator.h
src/modules/Playerbot/Advanced/TacticalCoordinator.cpp
```

**Phase 2:**
```
src/modules/Playerbot/Movement/Core/MovementArbiter.h
src/modules/Playerbot/Movement/Core/MovementArbiter.cpp
src/modules/Playerbot/Movement/Core/MovementPlanner.h
src/modules/Playerbot/Movement/Core/MovementPlanner.cpp
src/modules/Playerbot/Movement/Strategies/FollowStrategy.h
src/modules/Playerbot/Movement/Strategies/FollowStrategy.cpp
src/modules/Playerbot/Movement/Strategies/CombatStrategy.h
src/modules/Playerbot/Movement/Strategies/CombatStrategy.cpp
src/modules/Playerbot/Movement/Strategies/FormationStrategy.h
src/modules/Playerbot/Movement/Strategies/FormationStrategy.cpp
```

**Phase 3:**
```
src/modules/Playerbot/Core/Events/EventBus.h
src/modules/Playerbot/Core/Events/EventTypes.h
```

**Phase 4:**
```
src/modules/Playerbot/Core/Managers/IGameSystemsManager.h
src/modules/Playerbot/Core/Managers/GameSystemsManager.h
src/modules/Playerbot/Core/Managers/GameSystemsManager.cpp
```

---

## Appendix B: Migration Checklists

### GroupCoordinator Migration Checklist

**Preparation:**
- [ ] Document all BotAI usages of both coordinators
- [ ] Map all method calls to each coordinator
- [ ] Identify unique responsibilities
- [ ] Design unified interface

**Implementation:**
- [ ] Create Advanced/GroupCoordinator (unified)
- [ ] Create TacticalCoordinator subsystem
- [ ] Implement all methods from both old coordinators
- [ ] Add compatibility layer if needed

**Migration:**
- [ ] Update BotAI to use single coordinator
- [ ] Migrate 73 dependent files
- [ ] Update all direct references
- [ ] Update test files

**Testing:**
- [ ] Unit tests for new coordinator
- [ ] Integration tests for group behavior
- [ ] Raid coordination tests
- [ ] PvP group tests

**Cleanup:**
- [ ] Remove old Coordination::GroupCoordinator files
- [ ] Remove compatibility layer (if added)
- [ ] Update documentation

### Movement System Migration Checklist

**Preparation:**
- [ ] Document all movement request types
- [ ] Map current movement call sites
- [ ] Design MovementArbiter interface
- [ ] Design MovementPlanner interface
- [ ] Design strategy hierarchy

**Implementation:**
- [ ] Implement MovementArbiter
- [ ] Implement MovementPlanner
- [ ] Implement movement strategies
- [ ] Add request priority system
- [ ] Add conflict resolution

**Migration:**
- [ ] Convert solo movement calls
- [ ] Convert group movement calls
- [ ] Convert combat movement calls
- [ ] Convert follow behavior calls
- [ ] Convert formation calls

**Testing:**
- [ ] Unit tests for each component
- [ ] Integration tests for movement flow
- [ ] Performance tests
- [ ] Edge case testing

**Cleanup:**
- [ ] Remove old movement systems
- [ ] Update documentation
- [ ] Remove dead code

---

## Appendix C: Code Examples

### Example 1: IManager Interface

```cpp
// File: src/modules/Playerbot/Core/IManager.h
#ifndef IMANAGER_H
#define IMANAGER_H

#include <cstdint>

namespace Playerbot {

/**
 * @brief Base interface for all manager classes in the playerbot system
 *
 * Provides a unified lifecycle contract that all managers must implement.
 * This enables:
 * - Consistent initialization and cleanup
 * - Uniform manager registry integration
 * - Easier testing via mocking
 * - Better resource management
 */
class IManager {
public:
    virtual ~IManager() = default;

    /**
     * @brief Initialize the manager
     *
     * Called once after construction to set up resources, register
     * event handlers, and prepare for operation.
     *
     * @throws std::exception if initialization fails
     */
    virtual void Initialize() = 0;

    /**
     * @brief Update the manager
     *
     * Called every game tick to process manager logic.
     *
     * @param diff Time since last update in milliseconds
     */
    virtual void Update(uint32 diff) = 0;

    /**
     * @brief Reset the manager to initial state
     *
     * Called when the bot is reset or respawns. Should clear
     * state but keep resources allocated.
     */
    virtual void Reset() = 0;

    /**
     * @brief Shutdown the manager
     *
     * Called before destruction to release resources, unregister
     * handlers, and clean up.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Get manager name for debugging
     */
    virtual const char* GetName() const = 0;
};

} // namespace Playerbot

#endif // IMANAGER_H
```

### Example 2: Unified GroupCoordinator

```cpp
// File: src/modules/Playerbot/Advanced/GroupCoordinator.h
#ifndef GROUPCOORDINATOR_H
#define GROUPCOORDINATOR_H

#include "Core/IManager.h"
#include "TacticalCoordinator.h"
#include <memory>

namespace Playerbot {

class BotAI;

/**
 * @brief Unified group coordination system
 *
 * Combines tactical coordination (interrupts, dispels, cooldowns) with
 * social coordination (quests, loot, invites) and raid mechanics.
 *
 * Replaces:
 * - AI/Coordination/GroupCoordinator (tactical)
 * - Advanced/GroupCoordinator (comprehensive)
 */
class GroupCoordinator : public IManager {
public:
    enum class GroupRole {
        TANK,
        HEALER,
        MELEE_DPS,
        RANGED_DPS,
        SUPPORT,
        UNKNOWN
    };

    explicit GroupCoordinator(BotAI* botAI);
    ~GroupCoordinator() override;

    // IManager interface
    void Initialize() override;
    void Update(uint32 diff) override;
    void Reset() override;
    void Shutdown() override;
    const char* GetName() const override { return "GroupCoordinator"; }

    // Role management
    GroupRole DetermineRole() const;
    bool AssignRaidRole(Player* target, GroupRole role);

    // Tactical coordination (from old Coordination::GroupCoordinator)
    ObjectGuid AssignInterrupt(ObjectGuid targetGuid);
    ObjectGuid AssignDispel(ObjectGuid targetGuid);
    void CoordinateCooldowns();

    // Social coordination
    void HandleLootDistribution();
    void HandleQuestSharing();
    void HandleGroupInvites();

    // Raid coordination
    void ExecuteBossStrategy(Creature* boss);
    void HandleRaidMechanics();

private:
    BotAI* _botAI;
    std::unique_ptr<TacticalCoordinator> _tacticalCoordinator;
    // ... other members
};

} // namespace Playerbot

#endif // GROUPCOORDINATOR_H
```

### Example 3: EventBus Template

```cpp
// File: src/modules/Playerbot/Core/Events/EventBus.h
#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <functional>
#include <unordered_map>
#include <mutex>
#include <string>

namespace Playerbot::Events {

/**
 * @brief Generic event bus implementation
 *
 * Provides thread-safe publish/subscribe pattern for events.
 *
 * Usage:
 *   using GroupEventBus = EventBus<GroupEvent>;
 *
 *   GroupEventBus bus;
 *   bus.Subscribe("bot1", [](const GroupEvent& evt) {
 *       // Handle event
 *   });
 *   bus.Publish(GroupEvent{...});
 */
template<typename EventType>
class EventBus {
public:
    using Subscriber = std::function<void(const EventType&)>;

    /**
     * @brief Subscribe to events
     *
     * @param subscriberId Unique identifier for this subscriber
     * @param handler Callback to invoke when event is published
     */
    void Subscribe(const std::string& subscriberId, Subscriber handler) {
        std::lock_guard<std::mutex> lock(_mutex);
        _subscribers[subscriberId] = std::move(handler);
    }

    /**
     * @brief Unsubscribe from events
     *
     * @param subscriberId Subscriber to remove
     */
    void Unsubscribe(const std::string& subscriberId) {
        std::lock_guard<std::mutex> lock(_mutex);
        _subscribers.erase(subscriberId);
    }

    /**
     * @brief Publish event to all subscribers
     *
     * @param event Event to publish
     */
    void Publish(const EventType& event) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& [id, handler] : _subscribers) {
            try {
                handler(event);
            } catch (...) {
                // Log error but continue notifying other subscribers
            }
        }
    }

    /**
     * @brief Get subscriber count
     */
    size_t GetSubscriberCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _subscribers.size();
    }

private:
    std::unordered_map<std::string, Subscriber> _subscribers;
    mutable std::mutex _mutex;
};

// Common event bus type aliases
struct GroupEvent { /* ... */ };
struct CombatEvent { /* ... */ };
struct LootEvent { /* ... */ };

using GroupEventBus = EventBus<GroupEvent>;
using CombatEventBus = EventBus<CombatEvent>;
using LootEventBus = EventBus<LootEvent>;

} // namespace Playerbot::Events

#endif // EVENTBUS_H
```

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-11-17 | AI Analysis | Initial cleanup plan created |

---

## Approval Signatures

**Technical Lead:** ________________________ Date: ____________

**Architecture Lead:** ________________________ Date: ____________

**Project Manager:** ________________________ Date: ____________

---

**End of Cleanup Plan**
