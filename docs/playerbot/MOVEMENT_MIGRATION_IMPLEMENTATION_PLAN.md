# Phase 2 Movement System Migration - Implementation Plan

**Date:** 2025-11-18
**Project:** Complete UnifiedMovementCoordinator Migration
**Duration:** 3 weeks (120 hours)
**Status:** STARTING - Week 1

---

## Executive Summary

Complete the incomplete Phase 2 migration by integrating UnifiedMovementCoordinator into BotAI and migrating all 204+ MovementArbiter call sites to use the unified system.

**Approach:** Gradual migration with compatibility layer
- Week 1: Integration & Compatibility
- Week 2: Migration (primary systems)
- Week 3: Migration (remaining) & Cleanup

---

## Week 1: Integration & Compatibility Layer (40 hours)

### Goal
Add UnifiedMovementCoordinator to BotAI while maintaining backward compatibility with existing MovementArbiter usage.

### Tasks

#### Task 1.1: Add UnifiedMovementCoordinator to BotAI (8 hours)

**Files to Modify:**
- `src/modules/Playerbot/AI/BotAI.h`
- `src/modules/Playerbot/AI/BotAI.cpp`

**Changes:**

**BotAI.h:**
```cpp
// Add forward declaration (after line 49)
class MovementArbiter;
class UnifiedMovementCoordinator;  // NEW

// Add getter method (after line 321)
UnifiedMovementCoordinator* GetUnifiedMovementCoordinator() {
    return _unifiedMovementCoordinator.get();
}
UnifiedMovementCoordinator const* GetUnifiedMovementCoordinator() const {
    return _unifiedMovementCoordinator.get();
}

// Add member (after line 775)
std::unique_ptr<MovementArbiter> _movementArbiter;  // KEEP for compatibility
std::unique_ptr<UnifiedMovementCoordinator> _unifiedMovementCoordinator;  // NEW primary system
```

**BotAI.cpp:**
```cpp
// Add include (after line 49)
#include "Movement/Arbiter/MovementArbiter.h"
#include "Movement/UnifiedMovementCoordinator.h"  // NEW

// Update initialization (around line 110)
// OLD:
// _movementArbiter = std::make_unique<MovementArbiter>(_bot);

// NEW:
_unifiedMovementCoordinator = std::make_unique<UnifiedMovementCoordinator>(_bot);
_movementArbiter = std::make_unique<MovementArbiter>(_bot);  // Keep for compatibility temporarily
```

**Testing:**
- [ ] Compilation succeeds
- [ ] Bot initialization works
- [ ] Both systems accessible via getters
- [ ] No memory leaks (valgrind check)

---

#### Task 1.2: Create Migration Documentation (4 hours)

**File to Create:**
- `docs/playerbot/MOVEMENT_MIGRATION_GUIDE.md`

**Content:**
- How to migrate from MovementArbiter to UnifiedMovementCoordinator
- API comparison table
- Code examples (before/after)
- Benefits of migration
- Deprecation timeline

---

#### Task 1.3: Add Deprecation Warnings (4 hours)

**Strategy:**
Add compile-time warnings when MovementArbiter is used directly.

**BotAI.h:**
```cpp
// Mark as deprecated
[[deprecated("Use GetUnifiedMovementCoordinator() instead. MovementArbiter will be removed after all call sites migrated.")]]
MovementArbiter* GetMovementArbiter() { return _movementArbiter.get(); }

[[deprecated("Use GetUnifiedMovementCoordinator() instead. MovementArbiter will be removed after all call sites migrated.")]]
MovementArbiter const* GetMovementArbiter() const { return _movementArbiter.get(); }
```

**MovementArbiter.h:**
```cpp
// Add deprecation notice in class documentation
/**
 * @deprecated This class is deprecated. Use UnifiedMovementCoordinator instead.
 * MovementArbiter is now a subsystem within UnifiedMovementCoordinator.
 * Direct usage will be removed in a future update.
 */
class TC_GAME_API MovementArbiter
```

**Testing:**
- [ ] Deprecation warnings appear when compiling
- [ ] Warnings are clear and actionable
- [ ] No hard errors (still compiles)

---

#### Task 1.4: Migrate BotAI Internal Usage (8 hours)

**Goal:** Update BotAI's own movement calls to use UnifiedMovementCoordinator

**Files:**
- `src/modules/Playerbot/AI/BotAI.cpp`

**Changes:**
Search for all `_movementArbiter->` calls in BotAI.cpp and replace with `_unifiedMovementCoordinator->`.

**Example:**
```cpp
// OLD:
_movementArbiter->RequestMovement(request);

// NEW:
_unifiedMovementCoordinator->RequestMovement(request);
```

**Testing:**
- [ ] All BotAI movement requests work
- [ ] No regression in bot behavior
- [ ] Performance same or better

---

#### Task 1.5: Update 5 High-Traffic Files (16 hours)

**Target Files** (based on grep analysis):
1. `Advanced/AdvancedBehaviorManager.cpp` (7 refs)
2. `Dungeon/EncounterStrategy.cpp` (8 refs)
3. `AI/Combat/RoleBasedCombatPositioning.cpp` (8 refs)
4. `Dungeon/DungeonBehavior.cpp` (6 refs)
5. `AI/Combat/CombatAIIntegrator.cpp` (9 refs)

**Process per file:**
1. Read file, identify all MovementArbiter usage
2. Replace with UnifiedMovementCoordinator
3. Test compilation
4. Test functionality (if possible)
5. Commit changes

**Testing:**
- [ ] Each file compiles
- [ ] No runtime errors
- [ ] Bot combat behavior unchanged
- [ ] Dungeon behavior unchanged

---

### Week 1 Deliverables

- [ ] UnifiedMovementCoordinator integrated into BotAI
- [ ] Both systems coexist (compatibility mode)
- [ ] Migration guide created
- [ ] Deprecation warnings added
- [ ] BotAI migrated
- [ ] 5 high-traffic files migrated
- [ ] All tests pass
- [ ] Documentation updated

**Estimated Lines Changed:** ~200 lines
**Files Modified:** ~10 files

---

## Week 2: Primary System Migration (40 hours)

### Goal
Migrate the remaining high and medium-traffic files to UnifiedMovementCoordinator.

### Task 2.1: Migrate Remaining Combat Files (16 hours)

**Target Files:**
- `AI/Combat/ObstacleAvoidanceManager.cpp` (5 refs)
- `AI/Combat/MechanicAwareness.cpp` (3 refs)
- `AI/Combat/KitingManager.cpp` (2 refs)
- `AI/Combat/InterruptManager.cpp` (3 refs)
- `AI/Combat/FormationManager.cpp` (2 refs)
- `AI/Combat/UnifiedInterruptSystem.cpp` (3 refs)

**Process:** Same as Week 1 Task 1.5

---

### Task 2.2: Migrate Strategy Files (12 hours)

**Target Files:**
- `AI/Strategy/QuestStrategy.cpp` (2 refs)
- `AI/Strategy/LootStrategy.cpp` (3 refs)
- `AI/Strategy/CombatMovementStrategy.cpp` (2 refs)

**Note:** CombatMovementStrategy will need special attention as it's one of the systems being consolidated.

---

### Task 2.3: Migrate Class AI Files (8 hours)

**Target Files:**
- `AI/ClassAI/CombatSpecializationBase.cpp` (2 refs)
- `AI/ClassAI/Priests/PriestAI.cpp` (2 refs)
- `AI/ClassAI/Hunters/HunterAI.cpp` (2 refs)

---

### Task 2.4: Testing & Bug Fixes (4 hours)

- [ ] Full bot combat testing
- [ ] Movement behavior verification
- [ ] Performance profiling
- [ ] Bug fixes for any issues found

---

### Week 2 Deliverables

- [ ] All combat-related files migrated
- [ ] Strategy files migrated
- [ ] Class AI files migrated
- [ ] Comprehensive testing complete
- [ ] Bugs fixed
- [ ] Performance verified

**Estimated Lines Changed:** ~150 lines
**Files Modified:** ~15 files

---

## Week 3: Completion & Cleanup (40 hours)

### Goal
Complete migration, remove old system, consolidate duplicate systems.

### Task 3.1: Migrate Remaining Files (12 hours)

**Target Files:**
- `Lifecycle/DeathRecoveryManager.cpp` (5 refs)
- `Movement/LeaderFollowBehavior.cpp` (3 refs)
- `Tests/*` (test files)
- Any remaining stragglers

---

### Task 3.2: Consolidate Duplicate Systems (16 hours)

**CombatMovementStrategy → UnifiedMovementCoordinator.PositionModule**
- Extract combat positioning logic from CombatMovementStrategy
- Move to UnifiedMovementCoordinator's PositionManager
- Remove CombatMovementStrategy files
- Update all references

**GroupFormationManager → UnifiedMovementCoordinator.FormationModule**
- Extract formation logic from GroupFormationManager
- Move to UnifiedMovementCoordinator's FormationManager
- Remove GroupFormationManager files (if all logic moved)
- Update all references

**MovementIntegration → UnifiedMovementCoordinator.ArbiterModule**
- Merge priority systems
- Consolidate MovementCommand with MovementRequest
- Remove MovementIntegration files
- Update all references

---

### Task 3.3: Remove Old MovementArbiter (4 hours)

**Once all call sites migrated:**

**BotAI.h:**
```cpp
// REMOVE:
// class MovementArbiter;  // OLD system - REMOVED
// [[deprecated(...)]] MovementArbiter* GetMovementArbiter();
// std::unique_ptr<MovementArbiter> _movementArbiter;  // OLD - REMOVED
```

**BotAI.cpp:**
```cpp
// REMOVE:
// #include "Movement/Arbiter/MovementArbiter.h"  // OLD - REMOVED
// _movementArbiter = std::make_unique<MovementArbiter>(_bot);  // OLD - REMOVED
```

**Verification:**
```bash
# Should find 0 references (except in UnifiedMovementCoordinator internals)
grep -r "GetMovementArbiter()" src/modules/Playerbot/ --include="*.cpp" --include="*.h"
```

---

### Task 3.4: Update LeaderFollowBehavior Integration (6 hours)

**Complexity:** LeaderFollowBehavior is the largest system (1,941 lines)

**Options:**
1. **Keep as standalone** but use UnifiedMovementCoordinator for movement requests
2. **Integrate into UnifiedMovementCoordinator** as a new FollowModule
3. **Refactor to use Formation + Position modules**

**Recommended:** Option 1 (simplest, least risky)
- Update LeaderFollowBehavior to use UnifiedMovementCoordinator for movement
- Keep state machine logic intact
- Reduces risk while gaining benefits

---

### Task 3.5: Final Testing & Documentation (2 hours)

- [ ] Full regression testing
- [ ] Performance benchmarks
- [ ] Memory leak checks
- [ ] Update CLEANUP_PROGRESS.md
- [ ] Update architecture diagrams
- [ ] Create migration report

---

### Week 3 Deliverables

- [ ] All files migrated to UnifiedMovementCoordinator
- [ ] Old MovementArbiter removed
- [ ] Duplicate systems consolidated
- [ ] LeaderFollowBehavior integrated
- [ ] Comprehensive testing complete
- [ ] Documentation updated
- [ ] Migration report created

**Estimated Lines Removed:** ~5,253 lines (as per analysis)
**Estimated Lines Changed:** ~300-400 lines
**Files Removed:** 6 files (CombatMovementStrategy, MovementIntegration, GroupFormationManager duplicates)

---

## Success Criteria

### Code Quality
- [ ] Single movement system (UnifiedMovementCoordinator)
- [ ] <3 movement-related files (down from 14)
- [ ] 69% reduction in movement code
- [ ] No MovementArbiter references (except deprecation comments)

### Functionality
- [ ] All bot movement behaviors work correctly
- [ ] No regression in combat movement
- [ ] Formation system works
- [ ] Follow behavior works
- [ ] Pathfinding works

### Performance
- [ ] Movement request latency <= baseline
- [ ] Memory usage <= baseline
- [ ] CPU usage <= baseline
- [ ] No new memory leaks

### Documentation
- [ ] Migration guide complete
- [ ] API documentation updated
- [ ] Architecture diagrams updated
- [ ] CLEANUP_PROGRESS.md updated

---

## Risk Mitigation

### High-Risk Areas
1. **LeaderFollowBehavior** (1,941 lines) - Most complex system
   - Mitigation: Keep as standalone, only change movement requests

2. **Combat movement** - Performance critical
   - Mitigation: Extensive testing, performance profiling

3. **Pathfinding** - Affects all movement
   - Mitigation: Gradual migration, parallel testing

### Rollback Plan
If critical issues found:
1. Revert to previous commit
2. Fix issues in separate branch
3. Re-test before merging
4. Keep both systems until proven stable

---

## Testing Strategy

### Unit Tests
- [ ] UnifiedMovementCoordinator initialization
- [ ] Movement request submission
- [ ] Priority arbitration
- [ ] Pathfinding calculation
- [ ] Formation positioning

### Integration Tests
- [ ] Bot movement in combat
- [ ] Group formation movement
- [ ] Following leader
- [ ] Dungeon navigation
- [ ] Boss mechanic avoidance

### Performance Tests
- [ ] 100 bots concurrent movement (< 1% CPU per bot)
- [ ] Movement request latency (< 0.01ms)
- [ ] Memory usage (< 500 bytes per bot)

---

## Timeline

| Week | Focus | Hours | Deliverables |
|------|-------|-------|--------------|
| Week 1 | Integration & Compatibility | 40 | UMC in BotAI, 5 files migrated, docs |
| Week 2 | Primary Migration | 40 | Combat/Strategy/ClassAI files migrated |
| Week 3 | Completion & Cleanup | 40 | All migrated, old system removed |
| **Total** | | **120** | Complete migration |

---

## Sign-off

**Plan Created By:** Claude Code AI Assistant
**Date:** 2025-11-18
**Status:** READY FOR IMPLEMENTATION
**Recommendation:** APPROVE and begin Week 1

---

**END OF IMPLEMENTATION PLAN**
