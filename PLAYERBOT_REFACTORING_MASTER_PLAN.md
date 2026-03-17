# PlayerBot Architecture Refactoring - Master Plan

**Project Goal**: Refactor PlayerBot module for enterprise-grade scalability (5000+ bots), eliminating architectural chaos and performance bottlenecks.

**Status**: Phase 1 Complete - Architecture Approved
**Start Date**: 2025-01-06
**Estimated Completion**: 2025-03-10 (9-10 weeks)

---

## Executive Summary

### Problems Identified
1. ❌ **Singleton Automation systems** block 50-100ms per update
2. ❌ **Duplicate quest systems** (QuestAutomation + QuestManager)
3. ❌ **Strategy pattern violation** (IdleStrategy delegates instead of observing)
4. ❌ **Mixed update frequencies** (strategies call heavyweight operations)
5. ❌ **ClassAI only called in groups** (should run in ALL combat)
6. ❌ **No combat movement system** (role-based positioning missing)

### Solution Architecture
- ✅ **Unified Manager Pattern** - All behavior managers inherit from BehaviorManager
- ✅ **Manager/Strategy Separation** - Managers update periodically, strategies observe every frame
- ✅ **Combat Movement Strategy** - Role-based positioning (Tank/Healer/Melee/Ranged)
- ✅ **Universal ClassAI** - Runs in ALL combat situations (solo, group, questing)
- ✅ **Performance Optimized** - <2ms per bot per frame (supports 5000+ bots)

---

## Phase 1: Analysis & Design ✅ COMPLETE

**Duration**: 1 week (2025-01-06 to 2025-01-13)
**Status**: ✅ COMPLETE

### Completed Deliverables
- ✅ Phase 1.1: Bot lifecycle mapping (Login → Spawn → World Entry → AI)
- ✅ Phase 1.2: Strategy pattern inconsistencies documented
- ✅ Phase 1.3: Complete update chain mapping
- ✅ Phase 1.4: Unified architecture design
- ✅ Phase 1.5: Architecture approved by developer

### Key Findings
- **4 architectural problems** identified and documented
- **8 deprecated files** marked for deletion (~2000 lines)
- **10 duplicate/legacy files** requiring investigation
- **Complete update flow** mapped from World::Update() to Strategy::UpdateBehavior()

**Document**: See detailed analysis above in this session

---

## Phase 2: Implementation

### Phase 2.1: Create BehaviorManager Base Class

**Duration**: 1 week (2025-01-13 to 2025-01-20)
**Status**: ⏳ PENDING
**Detailed Plan**: See `PHASE_2_1_BEHAVIOR_MANAGER.md`

**Deliverables**:
- [ ] Create `AI/BehaviorManager.h` base class
- [ ] Implement throttling logic
- [ ] State query interface
- [ ] Unit tests for base class
- [ ] Documentation

**Success Criteria**:
- Base class compiles without errors
- Throttling works correctly (tests prove it)
- All pure virtual methods defined

---

### Phase 2.2: Create CombatMovementStrategy

**Duration**: 1 week (2025-01-20 to 2025-01-27)
**Status**: ⏳ PENDING
**Detailed Plan**: See `PHASE_2_2_COMBAT_MOVEMENT.md`

**Deliverables**:
- [ ] Create `Strategy/CombatMovementStrategy.h/cpp`
- [ ] Role detection (Tank/Healer/Melee DPS/Ranged DPS)
- [ ] Position calculations for each role
- [ ] Mechanic avoidance (fire, poison, etc.)
- [ ] Integration tests with dummy bots
- [ ] Documentation

**Success Criteria**:
- Melee bots stay in melee range during combat
- Ranged bots maintain 20-30 yard distance
- Healers position near group center
- Tanks face boss correctly
- Bots move out of ground effects

---

### Phase 2.3: Fix Combat Activation (Universal ClassAI)

**Duration**: 1 week (2025-01-27 to 2025-02-03)
**Status**: ⏳ PENDING
**Detailed Plan**: See `PHASE_2_3_COMBAT_ACTIVATION.md`

**Deliverables**:
- [ ] Modify BotAI::UpdateAI() to call OnCombatUpdate() in ALL combat
- [ ] Remove group-only restriction from combat logic
- [ ] Test solo quest combat (bot uses spells)
- [ ] Test gathering with mob interruption
- [ ] Test group dungeon combat
- [ ] Documentation

**Success Criteria**:
- Solo bot casts spells when questing
- Bot fights back when attacked while gathering
- Group bots still use combat rotations
- All 13 classes work in all scenarios

---

### Phase 2.4: Refactor Managers (Remove Automation Singletons)

**Duration**: 2 weeks (2025-02-03 to 2025-02-17)
**Status**: ⏳ PENDING
**Detailed Plan**: See `PHASE_2_4_MANAGER_REFACTOR.md`

**Deliverables**:
- [ ] Refactor QuestManager to inherit from BehaviorManager
- [ ] Create TradeManager (replace TradeAutomation)
- [ ] Create GatheringManager (replace GatheringAutomation)
- [ ] Create AuctionManager (replace AuctionAutomation)
- [ ] Update BotAI to instantiate all managers
- [ ] Add UpdateManagers() method to BotAI::UpdateAI()
- [ ] Unit tests for each manager
- [ ] Integration tests
- [ ] Documentation

**Success Criteria**:
- All managers inherit from BehaviorManager
- No singleton pattern used
- Each manager updates on its own schedule
- Performance <1ms amortized per manager
- 100+ bots run without lag

---

### Phase 2.5: Update IdleStrategy (Observer Pattern)

**Duration**: 1 week (2025-02-17 to 2025-02-24)
**Status**: ⏳ PENDING
**Detailed Plan**: See `PHASE_2_5_IDLE_STRATEGY.md`

**Deliverables**:
- [ ] Remove all Automation::instance() calls from IdleStrategy
- [ ] Add manager state queries (IsQuestingActive(), etc.)
- [ ] Lightweight observation pattern (<0.1ms per update)
- [ ] Remove throttling timers (no longer needed)
- [ ] Update strategy activation logic
- [ ] Integration tests
- [ ] Documentation

**Success Criteria**:
- IdleStrategy::UpdateBehavior() takes <0.1ms
- No blocking calls to external systems
- Bots show correct activity in logs
- 5000+ bots supported without lag

---

### Phase 2.6: Integration Testing

**Duration**: 1 week (2025-02-24 to 2025-03-03)
**Status**: ⏳ PENDING
**Detailed Plan**: See `PHASE_2_6_INTEGRATION_TESTING.md`

**Deliverables**:
- [ ] Test all combat scenarios (solo, group, raid)
- [ ] Test movement transitions (follow → combat → follow)
- [ ] Test strategy activation/deactivation
- [ ] Load test with 100, 500, 1000, 5000 bots
- [ ] Performance profiling
- [ ] Bug fixes
- [ ] Documentation

**Success Criteria**:
- All test scenarios pass
- 5000 bots run at <10% server CPU
- No memory leaks detected
- No deadlocks or crashes
- Frame time <2ms per bot average

---

### Phase 2.7: Cleanup & Consolidation

**Duration**: 2-3 weeks (2025-03-03 to 2025-03-24)
**Status**: ⏳ PENDING
**Detailed Plan**: See `PHASE_2_7_CLEANUP.md`

**Deliverables**:
- [ ] Delete 8 deprecated Automation files (~2000 lines)
- [ ] Investigate 10 duplicate/legacy files
- [ ] Consolidate manager hierarchy
- [ ] Move managers to `Managers/` folder
- [ ] Update CMakeLists.txt
- [ ] Remove all dead code
- [ ] Update all includes
- [ ] Documentation cleanup
- [ ] Create DELETED_FILES.md
- [ ] Create MIGRATION_GUIDE.md

**Success Criteria**:
- 3000-5000 lines of code removed
- No duplicate functionality remaining
- All managers in unified folder structure
- Clean compilation without warnings
- All tests still pass

---

### Phase 2.8: Final Documentation

**Duration**: 1 week (2025-03-24 to 2025-03-31)
**Status**: ⏳ PENDING
**Detailed Plan**: See `PHASE_2_8_DOCUMENTATION.md`

**Deliverables**:
- [ ] Complete ARCHITECTURE.md
- [ ] Update README.md
- [ ] API documentation for all managers
- [ ] Code comments audit
- [ ] Developer onboarding guide
- [ ] Performance tuning guide
- [ ] Troubleshooting guide

**Success Criteria**:
- New developers can understand system in <2 hours
- All public APIs documented
- Architecture diagrams up to date
- Migration guide tested by external reviewer

---

## Progress Tracking

### Overall Progress
- **Phase 1**: ✅ 100% (1/1 phases complete)
- **Phase 2**: ⏳ 0% (0/8 phases complete)
- **Overall**: 11% (1/9 phases complete)

### Milestone Dates
- ✅ 2025-01-13: Phase 1 Complete (Architecture Approved)
- ⏳ 2025-02-03: Combat System Complete (Phases 2.1-2.3)
- ⏳ 2025-02-24: Manager Refactor Complete (Phases 2.4-2.5)
- ⏳ 2025-03-10: Integration Testing Complete (Phase 2.6)
- ⏳ 2025-03-31: Full Refactor Complete (Phases 2.7-2.8)

### Risk Register
1. **Performance Regression**: Mitigation = Benchmark each phase
2. **Breaking Existing Functionality**: Mitigation = Incremental testing
3. **Scope Creep**: Mitigation = Strict adherence to plan
4. **Integration Conflicts**: Mitigation = Feature branch, regular rebases

---

## Success Metrics

### Performance Targets
- ✅ **<2ms per bot per frame** (average across all systems)
- ✅ **<10% CPU usage** with 5000 bots on 8-core server
- ✅ **<10MB memory** per bot
- ✅ **Zero deadlocks** in 72-hour stress test
- ✅ **Zero memory leaks** in 72-hour stress test

### Architecture Quality
- ✅ **Single pattern** for all managers (no exceptions)
- ✅ **No duplicate systems** (quest/trade/auction/gathering)
- ✅ **Clear ownership** (BotAI owns managers, strategies observe)
- ✅ **Zero singleton Automation** classes remaining
- ✅ **All ClassAI working** in all combat scenarios

### Code Quality
- ✅ **3000-5000 lines removed** (dead code cleanup)
- ✅ **Zero compiler warnings** in Release build
- ✅ **100% test coverage** for new managers
- ✅ **Documentation complete** for all public APIs

---

## Contact & Approval

**Project Lead**: Developer (User)
**AI Architect**: Claude (Anthropic)
**Approval Status**: ✅ Approved to proceed with Phase 2.1

**Last Updated**: 2025-01-13
**Next Review**: 2025-01-20 (End of Phase 2.1)
