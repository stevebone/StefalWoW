# Playerbot Cleanup - Quick Reference Guide

**Last Updated:** 2025-11-17

---

## 📊 At a Glance

| Category | Count | Status |
|----------|-------|--------|
| Total Files | 922 | ⚠️ High |
| Files with Commented Code | 413 (45%) | 🔴 Critical |
| Duplicate "Refactored" Files | 40+ | 🔴 Critical |
| Manager Classes | 105+ | ⚠️ High |
| DI Interfaces | 89 | ⚠️ High |
| Movement Systems | 7 | 🔴 Needs consolidation |
| Loot Management Files | 9 | ⚠️ Scattered |
| Test Files | 119 | ✅ Good coverage |

---

## 🎯 Top 5 Priority Issues

### 1. 🔴 Remove 40+ Refactored Legacy Files
**Impact:** HIGH | **Effort:** LOW | **Risk:** LOW
- **Lines to remove:** ~8,000
- **Timeline:** 1 week
- **Files:** All `*Refactored.h` class specializations
- **Action:** Choose canonical version, migrate references, delete duplicates

### 2. 🔴 Duplicate GroupCoordinator
**Impact:** HIGH | **Effort:** MEDIUM | **Risk:** MEDIUM
- **Files:** 2 implementations with overlapping logic
- **Timeline:** 1.5 weeks
- **Action:** Merge into single unified coordinator with subsystems

### 3. ⚠️ Movement System Scatter
**Impact:** HIGH | **Effort:** HIGH | **Risk:** HIGH
- **Systems:** 7 separate movement implementations
- **Timeline:** 4 weeks
- **Action:** Consolidate into 3-layer unified architecture

### 4. ⚠️ BotAI God Class
**Impact:** MEDIUM | **Effort:** MEDIUM | **Risk:** MEDIUM
- **Dependencies:** 73 files depend on it
- **Lines:** 906 header lines, 2,155 total
- **Timeline:** 1 week
- **Action:** Extract game systems into manager facade

### 5. ⚠️ Vendor/NPC Interaction Scatter
**Impact:** MEDIUM | **Effort:** LOW | **Risk:** LOW
- **Files:** 6 separate implementations
- **Timeline:** 1 week
- **Action:** Consolidate around InteractionManager

---

## 📁 File Locations

### Duplicate Coordinators
```
src/modules/Playerbot/AI/Coordination/GroupCoordinator.h      [REMOVE]
src/modules/Playerbot/Advanced/GroupCoordinator.h             [KEEP & MERGE]
```

### Legacy Refactored Files (40+)
```
src/modules/Playerbot/AI/ClassAI/ClassAI_Refactored.h         [REMOVE]
src/modules/Playerbot/AI/ClassAI/ThreadSafeClassAI.h          [REMOVE]
src/modules/Playerbot/AI/ClassAI/DeathKnights/*Refactored.h   [REMOVE ALL]
src/modules/Playerbot/AI/ClassAI/DemonHunters/*Refactored.h   [REMOVE ALL]
src/modules/Playerbot/AI/ClassAI/Druids/*Refactored.h         [REMOVE ALL]
... (all class specializations)
```

### Vendor Interaction Files
```
Game/VendorPurchaseManager                                    [KEEP]
Interaction/Core/InteractionManager                           [KEEP - MAIN]
Interaction/VendorInteractionManager                          [MERGE]
Social/VendorInteraction.h                                    [REMOVE - DUPLICATE]
```

### Movement System Files
```
Movement/Arbiter/MovementArbiter                              [KEEP - ENHANCE]
Movement/UnifiedMovementCoordinator                           [REMOVE - MERGE]
AI/Strategy/CombatMovementStrategy                            [MERGE LOGIC]
Movement/GroupFormationManager                                [MERGE LOGIC]
Movement/LeaderFollowBehavior                                 [MERGE LOGIC]
```

---

## 📋 Cleanup Phases Summary

### Phase 1: Foundation (Weeks 1-4)
**Risk:** LOW | **Impact:** HIGH
```
✓ Create IManager base interface
✓ Unify GroupCoordinator
✓ Consolidate vendor/NPC interaction
✓ Remove 40+ refactored files
```
**Result:** -8,300 lines, -42 files

### Phase 2: Movement (Weeks 5-8)
**Risk:** HIGH | **Impact:** HIGH
```
✓ Design 3-layer movement architecture
✓ Implement MovementArbiter + Planner + Strategies
✓ Migrate all movement requests
✓ Remove old systems
```
**Result:** -1,000 lines, -5 files

### Phase 3: Events (Weeks 9-11)
**Risk:** MEDIUM | **Impact:** MEDIUM
```
✓ Create EventBus<T> template
✓ Migrate 58 event buses
✓ Unify subscription patterns
```
**Result:** -400 lines, duplication eliminated

### Phase 4: Cleanup (Weeks 12-14)
**Risk:** LOW | **Impact:** MEDIUM
```
✓ Extract IGameSystemsManager
✓ Reduce BotAI complexity
✓ Remove dead code
✓ Clean up documentation
```
**Result:** -600 lines, -90 doc files

---

## 🎯 Key Metrics

### Before Cleanup
- **Files:** 922
- **Lines:** ~400,000
- **Managers:** 105+
- **Commented Code:** 45%
- **Circular Deps:** HIGH
- **Build Time:** Baseline

### After Cleanup (Target)
- **Files:** 870 (-5.6%)
- **Lines:** ~390,000 (-2.7%)
- **Managers:** 95 (-9.5%)
- **Commented Code:** <25% (-44%)
- **Circular Deps:** LOW (-70%)
- **Build Time:** -8%

---

## 🔍 Problem Areas Map

### Critical Duplication
```
ClassAI System:
  ├─ ClassAI.h (386 lines)                    [KEEP]
  ├─ ClassAI_Refactored.h (267 lines)         [REMOVE]
  └─ ThreadSafeClassAI.h                      [REMOVE]

GroupCoordinator:
  ├─ AI/Coordination/GroupCoordinator         [REMOVE]
  └─ Advanced/GroupCoordinator                [KEEP & ENHANCE]

Per-Class Specializations:
  ├─ BloodDeathKnight.h                       [KEEP]
  ├─ BloodDeathKnightRefactored.h             [REMOVE]
  └─ ... (38 more pairs)                      [RESOLVE ALL]
```

### Scattered Systems
```
Movement (7 systems):
  ├─ MovementArbiter                          [CORE]
  ├─ UnifiedMovementCoordinator               [MERGE]
  ├─ CombatMovementStrategy                   [MERGE]
  ├─ MovementIntegration                      [MERGE]
  ├─ GroupFormationManager                    [MERGE]
  ├─ LeaderFollowBehavior                     [MERGE]
  └─ BotMovementUtil                          [KEEP AS UTIL]

Loot (9 files):
  ├─ UnifiedLootManager                       [CORE]
  ├─ LootStrategy                             [ROUTE THROUGH CORE]
  ├─ LootDistribution                         [SUBSYSTEM]
  ├─ LootCoordination                         [SUBSYSTEM]
  ├─ LootAnalysis                             [SUBSYSTEM]
  └─ ...                                      [CONSOLIDATE]
```

---

## 🚨 High Risk Areas

### Movement System Refactor
**Risk:** HIGH
- Complex interdependencies
- Performance-critical
- Affects all bot activities

**Mitigation:**
- Keep old system until new proven
- Incremental migration
- Comprehensive testing
- Performance profiling

### BotAI Decoupling
**Risk:** MEDIUM
- 73 files depend on BotAI
- Central to all systems
- Complex to refactor safely

**Mitigation:**
- Use adapter pattern
- Incremental changes
- Extensive integration testing
- Rollback plan ready

---

## ✅ Testing Checklist

### Unit Tests
- [ ] Manager lifecycle (Initialize, Update, Shutdown)
- [ ] Event publication/subscription
- [ ] Movement request prioritization
- [ ] Group role assignment
- [ ] Spell casting logic

### Integration Tests
- [ ] BotAI ↔ Manager interactions
- [ ] Event flow across components
- [ ] Movement request → execution
- [ ] Group coordination
- [ ] Combat behavior integration

### System Tests
- [ ] Solo questing
- [ ] Group dungeons
- [ ] Raid encounters
- [ ] PvP scenarios
- [ ] Auction house operations
- [ ] Crafting/gathering

### Performance Tests
- [ ] Memory per bot
- [ ] CPU with 100/500/1000 bots
- [ ] Movement performance
- [ ] Event throughput
- [ ] Database query performance

---

## 📞 Quick Commands

### Find Refactored Files
```bash
find src/modules/Playerbot -name "*Refactored.h"
```

### Count Files with TODOs
```bash
grep -r "TODO\|FIXME" src/modules/Playerbot --include="*.cpp" | wc -l
```

### Check BotAI Dependencies
```bash
grep -r "BotAI.h" src/modules/Playerbot --include="*.h" | wc -l
```

### Count Manager Classes
```bash
find src/modules/Playerbot -name "*Manager.h" | wc -l
```

### Find Commented Code
```bash
grep -r "^[[:space:]]*//.*" src/modules/Playerbot --include="*.cpp" | wc -l
```

---

## 📖 Documentation References

- **Full Plan:** `CLEANUP_PLAN_2025.md`
- **Executive Summary:** `CLEANUP_EXECUTIVE_SUMMARY.md`
- **Architecture:** `ARCHITECTURE.md`
- **Contributing:** `CONTRIBUTING.md`

---

## 👥 Team Contacts

| Role | Responsibility |
|------|----------------|
| **Technical Lead** | Architecture approval, technical decisions |
| **Project Manager** | Timeline, resources, coordination |
| **QA Lead** | Testing strategy, test coverage |
| **DevOps Lead** | CI/CD pipeline, build optimization |

---

## 🎯 Success Criteria

### Code Quality
✓ Files reduced from 922 to ~870
✓ Commented code <25%
✓ All refactored files removed
✓ Circular dependencies reduced 70%

### Performance
✓ Build time -8%
✓ No behavioral regressions
✓ Performance tests pass

### Maintainability
✓ Unified manager interface
✓ Single GroupCoordinator
✓ 3-layer movement system
✓ Generic EventBus<T>

---

## 📅 Timeline

```
Weeks 1-4:   Foundation & Quick Wins
Weeks 5-8:   Movement System
Weeks 9-11:  Event System
Weeks 12-14: Final Cleanup

Total: 14-16 weeks with 2-3 developers
```

---

**Status:** Planning Phase
**Version:** 1.0
**Last Updated:** 2025-11-17
