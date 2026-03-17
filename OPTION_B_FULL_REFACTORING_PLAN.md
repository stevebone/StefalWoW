# OPTION B: FULL REFACTORING PLAN - PlayerBot Module Architecture Overhaul

## Executive Summary
This document outlines a comprehensive refactoring plan to address 4 critical bot behavior issues through systematic architectural improvements. The plan involves creating a robust state machine system, implementing behavior priority management, ensuring safe reference handling, and establishing event-driven updates.

**Estimated Total Effort:** 320-480 hours (8-12 weeks with one developer)
**Risk Level:** HIGH - Major architectural changes affecting all bot behavior
**Backward Compatibility:** BREAKING - Requires migration of existing bot configurations

---

## 1. SCOPE DEFINITION

### 1.1 New Files to Create (28 files, ~8,500 lines)

#### State Machine System
- `src/modules/Playerbot/Core/StateMachine/BotStateMachine.h` (300 lines)
- `src/modules/Playerbot/Core/StateMachine/BotStateMachine.cpp` (500 lines)
- `src/modules/Playerbot/Core/StateMachine/BotInitStateMachine.h` (200 lines)
- `src/modules/Playerbot/Core/StateMachine/BotInitStateMachine.cpp` (400 lines)
- `src/modules/Playerbot/Core/StateMachine/StateTransitions.h` (150 lines)
- `src/modules/Playerbot/Core/StateMachine/StateValidators.h` (150 lines)
- `src/modules/Playerbot/Core/StateMachine/StateValidators.cpp` (300 lines)

#### Behavior Priority System
- `src/modules/Playerbot/AI/BehaviorManager/BehaviorManager.h` (250 lines)
- `src/modules/Playerbot/AI/BehaviorManager/BehaviorManager.cpp` (600 lines)
- `src/modules/Playerbot/AI/BehaviorManager/BehaviorPriority.h` (100 lines)
- `src/modules/Playerbot/AI/BehaviorManager/BehaviorContext.h` (150 lines)
- `src/modules/Playerbot/AI/BehaviorManager/BehaviorContext.cpp` (200 lines)
- `src/modules/Playerbot/AI/BehaviorManager/MutualExclusionRules.h` (100 lines)
- `src/modules/Playerbot/AI/BehaviorManager/MutualExclusionRules.cpp` (250 lines)

#### Safe Reference Management
- `src/modules/Playerbot/Core/References/SafeObjectReference.h` (200 lines)
- `src/modules/Playerbot/Core/References/SafeObjectReference.cpp` (350 lines)
- `src/modules/Playerbot/Core/References/ReferenceValidator.h` (150 lines)
- `src/modules/Playerbot/Core/References/ReferenceValidator.cpp` (300 lines)
- `src/modules/Playerbot/Core/References/ReferenceCache.h` (200 lines)
- `src/modules/Playerbot/Core/References/ReferenceCache.cpp` (400 lines)

#### Event System
- `src/modules/Playerbot/Events/BotEventSystem.h` (250 lines)
- `src/modules/Playerbot/Events/BotEventSystem.cpp` (500 lines)
- `src/modules/Playerbot/Events/GroupEventObserver.h` (150 lines)
- `src/modules/Playerbot/Events/GroupEventObserver.cpp` (300 lines)
- `src/modules/Playerbot/Events/CombatEventObserver.h` (150 lines)
- `src/modules/Playerbot/Events/CombatEventObserver.cpp` (300 lines)
- `src/modules/Playerbot/Events/WorldEventObserver.h` (150 lines)
- `src/modules/Playerbot/Events/WorldEventObserver.cpp` (300 lines)

### 1.2 Existing Files to Modify (45+ files, ~3,000 lines changed)

#### Core Bot Files (Major Changes)
- `src/modules/Playerbot/AI/BotAI.h` (~200 lines modified)
- `src/modules/Playerbot/AI/BotAI.cpp` (~500 lines modified)
- `src/modules/Playerbot/Session/BotSession.h` (~100 lines modified)
- `src/modules/Playerbot/Session/BotSession.cpp` (~300 lines modified)
- `src/modules/Playerbot/AI/ClassAI/ClassAI.h` (~150 lines modified)
- `src/modules/Playerbot/AI/ClassAI/ClassAI.cpp` (~400 lines modified)

#### Strategy Files (Moderate Changes)
- `src/modules/Playerbot/AI/Strategy/Strategy.h` (~50 lines modified)
- `src/modules/Playerbot/AI/Strategy/Strategy.cpp` (~100 lines modified)
- `src/modules/Playerbot/AI/Strategy/IdleStrategy.h` (~30 lines modified)
- `src/modules/Playerbot/AI/Strategy/IdleStrategy.cpp` (~50 lines modified)
- `src/modules/Playerbot/Movement/LeaderFollowBehavior.h` (~50 lines modified)
- `src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp` (~150 lines modified)

#### All Class-Specific AI Files (Minor Changes)
- 13 Class AI headers (13 * 20 lines = 260 lines)
- 13 Class AI implementations (13 * 40 lines = 520 lines)
- Multiple specialization files (30+ files, ~30 lines each)

### 1.3 Interfaces and Base Classes

```cpp
// IBehaviorPrioritizable interface
class IBehaviorPrioritizable {
public:
    virtual ~IBehaviorPrioritizable() = default;
    virtual BehaviorPriority GetPriority() const = 0;
    virtual bool CanActivate(const BehaviorContext& context) const = 0;
    virtual bool ConflictsWith(const IBehaviorPrioritizable* other) const = 0;
};

// IStateMachineNode interface
class IStateMachineNode {
public:
    virtual ~IStateMachineNode() = default;
    virtual StateType GetStateType() const = 0;
    virtual bool CanTransitionTo(StateType nextState) const = 0;
    virtual void OnEnter() = 0;
    virtual void OnExit() = 0;
    virtual void Update(uint32 diff) = 0;
};

// ISafeReference interface
template<typename T>
class ISafeReference {
public:
    virtual ~ISafeReference() = default;
    virtual T* Get() const = 0;
    virtual bool IsValid() const = 0;
    virtual void Invalidate() = 0;
    virtual ObjectGuid GetGuid() const = 0;
};

// IEventObserver interface
class IEventObserver {
public:
    virtual ~IEventObserver() = default;
    virtual void OnEvent(const Event& event) = 0;
    virtual EventPriority GetObserverPriority() const = 0;
    virtual bool InterestedIn(EventType type) const = 0;
};
```

---

## 2. COMPONENT BREAKDOWN

### 2.1 Unified Bot Initialization State Machine (2,000 lines)

**Purpose:** Eliminate race conditions during bot initialization and login.

**Classes to Create:**
- `BotStateMachine`: Generic state machine framework
- `BotInitStateMachine`: Specific implementation for bot initialization
- `StateTransitions`: Manages valid state transitions
- `StateValidators`: Validates state preconditions

**Existing Code to Refactor:**
- `BotSession::HandleBotPlayerLogin()` - Complete rewrite
- `BotAI::UpdateAI()` first-update logic - Remove static set, use state machine
- `BotAI::OnGroupJoined()` - Delay until READY state

**Implementation Order:**
1. Create base state machine framework
2. Implement initialization states
3. Integrate with BotSession
4. Update BotAI to use state machine
5. Add comprehensive logging

### 2.2 Behavior Priority System with Mutual Exclusion (1,600 lines)

**Purpose:** Prevent conflicting behaviors from executing simultaneously.

**Classes to Create:**
- `BehaviorManager`: Central behavior coordinator
- `BehaviorPriority`: Priority enumeration and comparisons
- `BehaviorContext`: Runtime context for behavior decisions
- `MutualExclusionRules`: Define which behaviors cannot coexist

**Existing Code to Refactor:**
- All Strategy classes to implement `IBehaviorPrioritizable`
- `BotAI::UpdateStrategies()` - Replace with BehaviorManager
- `LeaderFollowBehavior::GetRelevance()` - Return 0 during combat
- Combat strategies - Ensure mutual exclusion with follow

**Implementation Order:**
1. Create priority system and interfaces
2. Implement BehaviorManager
3. Retrofit existing strategies
4. Add mutual exclusion rules
5. Integration testing

### 2.3 Safe Leader Reference Management (1,250 lines)

**Purpose:** Prevent crashes from dangling pointers.

**Classes to Create:**
- `SafeObjectReference<T>`: Template for safe references
- `ReferenceValidator`: Validates references before use
- `ReferenceCache`: Manages reference lifecycle

**Existing Code to Refactor:**
- All raw `Player*` pointers to leaders/targets
- `ObjectCache` - Replace with SafeObjectReference
- Group member iterations - Use safe references
- Combat target tracking - Use safe references

**Implementation Order:**
1. Implement SafeObjectReference template
2. Create ReferenceValidator
3. Replace ObjectCache implementation
4. Update all pointer usage
5. Add validation everywhere

### 2.4 Event-Driven State Updates (1,700 lines)

**Purpose:** Ensure bots react immediately to state changes.

**Classes to Create:**
- `BotEventSystem`: Central event dispatcher
- `GroupEventObserver`: Observes group changes
- `CombatEventObserver`: Observes combat state
- `WorldEventObserver`: Observes world events

**Existing Code to Refactor:**
- Polling-based state checks - Replace with events
- `BotAI::OnGroupJoined/Left()` - Event-driven
- Combat state transitions - Event-driven
- Leader logout handling - Add proper event

**Implementation Order:**
1. Create event system framework
2. Implement specific observers
3. Register bots with event system
4. Convert polling to events
5. Add event logging

---

## 3. INTEGRATION POINTS

### 3.1 TrinityCore Core Modifications Required

#### Minimal Core Hooks (5 files, ~50 lines total)
```cpp
// src/server/game/Groups/Group.cpp
// Add at appropriate points:
if (PlayerBotHooks::OnGroupMemberAdded)
    PlayerBotHooks::OnGroupMemberAdded(this, member);

if (PlayerBotHooks::OnGroupMemberRemoved)
    PlayerBotHooks::OnGroupMemberRemoved(this, member);

if (PlayerBotHooks::OnGroupDisbanded)
    PlayerBotHooks::OnGroupDisbanded(this);

// src/server/game/Entities/Player/Player.cpp
// In Player::~Player() or logout handling:
if (PlayerBotHooks::OnPlayerLogout)
    PlayerBotHooks::OnPlayerLogout(this);

// src/server/game/Combat/CombatManager.cpp
// Add combat state change hooks:
if (PlayerBotHooks::OnCombatStart)
    PlayerBotHooks::OnCombatStart(owner, target);

if (PlayerBotHooks::OnCombatEnd)
    PlayerBotHooks::OnCombatEnd(owner);
```

### 3.2 Database Schema Changes

```sql
-- New table for bot state persistence
CREATE TABLE `playerbot_states` (
    `guid` INT UNSIGNED NOT NULL PRIMARY KEY,
    `state` VARCHAR(50) NOT NULL,
    `last_transition` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `state_data` JSON,
    INDEX `idx_state` (`state`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- New table for behavior configurations
CREATE TABLE `playerbot_behaviors` (
    `guid` INT UNSIGNED NOT NULL,
    `behavior_type` VARCHAR(50) NOT NULL,
    `priority` INT NOT NULL,
    `enabled` BOOLEAN DEFAULT TRUE,
    `config_data` JSON,
    PRIMARY KEY (`guid`, `behavior_type`),
    INDEX `idx_priority` (`priority`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### 3.3 Breaking Changes

1. **Strategy System**: Complete replacement with BehaviorManager
2. **ObjectCache**: Deprecated in favor of SafeObjectReference
3. **Manual State Checks**: Replaced with event system
4. **Configuration**: New behavior priority configuration required
5. **Database**: Migration required for existing bots

---

## 4. TESTING REQUIREMENTS

### 4.1 Unit Test Scenarios (50+ test cases)

#### State Machine Tests
- State transition validation
- Invalid transition rejection
- Concurrent state updates
- State persistence and recovery
- Race condition handling

#### Behavior Priority Tests
- Priority ordering validation
- Mutual exclusion enforcement
- Context-based activation
- Dynamic priority changes
- Conflict resolution

#### Safe Reference Tests
- Reference validation
- Invalidation on object destruction
- Concurrent access safety
- Cache performance
- Memory leak prevention

#### Event System Tests
- Event dispatch performance
- Observer registration/unregistration
- Event priority handling
- Circular dependency prevention
- Memory management

### 4.2 Integration Test Scenarios

#### Issue 1: Bot Already in Group at Login
```cpp
TEST(BotLogin, ExistingGroupActivation) {
    // 1. Create group with bot
    // 2. Restart server
    // 3. Login player
    // 4. Verify bot follows immediately
    // 5. Verify strategies activated correctly
}
```

#### Issue 2: Ranged DPS Combat
```cpp
TEST(Combat, RangedDPSBehavior) {
    // 1. Create group with ranged bot
    // 2. Initiate combat
    // 3. Verify bot maintains optimal range
    // 4. Verify spell casting
    // 5. Verify no ping-pong movement
}
```

#### Issue 3: Melee Facing
```cpp
TEST(Combat, MeleeFacing) {
    // 1. Create group with melee bot
    // 2. Initiate combat
    // 3. Verify bot faces target
    // 4. Verify damage output
    // 5. Verify position maintenance
}
```

#### Issue 4: Logout Crash Prevention
```cpp
TEST(Logout, SafeCleanup) {
    // 1. Create group with multiple bots
    // 2. Logout group leader
    // 3. Verify no crash
    // 4. Verify bots handle gracefully
    // 5. Verify memory cleanup
}
```

### 4.3 Edge Cases to Cover

1. **Rapid State Changes**: Login/logout/combat in quick succession
2. **Network Latency**: Simulated lag between state changes
3. **Resource Exhaustion**: 100+ bots simultaneously
4. **Concurrent Modifications**: Multiple threads updating same bot
5. **Partial Failures**: Some components fail during initialization
6. **Reference Cycles**: Bots referencing each other
7. **Event Storms**: Massive number of events simultaneously
8. **State Corruption**: Recovery from invalid states

---

## 5. IMPLEMENTATION PHASES

### Phase 1: Foundation (80-100 hours)
**Duration:** 2-3 weeks
**Dependencies:** None

**Deliverables:**
- State machine framework
- Basic behavior interfaces
- Safe reference template
- Event system skeleton

**Tasks:**
1. Create base interfaces and abstract classes
2. Implement state machine core
3. Create safe reference system
4. Build event dispatcher
5. Unit tests for foundations

### Phase 2: Behavior Priority System (60-80 hours)
**Duration:** 1.5-2 weeks
**Dependencies:** Phase 1 complete

**Deliverables:**
- BehaviorManager implementation
- Priority rules engine
- Mutual exclusion system
- Strategy retrofitting

**Tasks:**
1. Implement BehaviorManager
2. Create priority comparison logic
3. Add mutual exclusion rules
4. Retrofit existing strategies
5. Integration tests

### Phase 3: Safe References Integration (80-100 hours)
**Duration:** 2-3 weeks
**Dependencies:** Phase 1 complete

**Deliverables:**
- All pointers converted to safe references
- Reference validation throughout
- Cache implementation
- Memory leak fixes

**Tasks:**
1. Replace ObjectCache
2. Convert all Player* pointers
3. Add validation checks
4. Performance optimization
5. Memory leak testing

### Phase 4: Event System Integration (60-80 hours)
**Duration:** 1.5-2 weeks
**Dependencies:** Phases 1-3 complete

**Deliverables:**
- Full event system
- All observers implemented
- Polling code removed
- Core hooks added

**Tasks:**
1. Implement all observers
2. Add core integration hooks
3. Convert polling to events
4. Event performance tuning
5. Stress testing

### Phase 5: Integration & Testing (40-60 hours)
**Duration:** 1-1.5 weeks
**Dependencies:** Phases 1-4 complete

**Deliverables:**
- All issues verified fixed
- Performance validated
- Documentation complete
- Migration tools

**Tasks:**
1. Full integration testing
2. Performance profiling
3. Bug fixes
4. Documentation
5. Migration scripts

---

## 6. ESTIMATED EFFORT

### Per-Phase Breakdown

| Phase | Min Hours | Max Hours | Critical Path |
|-------|-----------|-----------|---------------|
| Phase 1: Foundation | 80 | 100 | Yes |
| Phase 2: Behavior Priority | 60 | 80 | Yes |
| Phase 3: Safe References | 80 | 100 | Yes |
| Phase 4: Event System | 60 | 80 | Yes |
| Phase 5: Integration | 40 | 60 | Yes |
| **TOTAL** | **320** | **420** | |

### Additional Considerations
- **Code Review:** +40 hours
- **Documentation:** +20 hours
- **Deployment Support:** +20 hours
- **GRAND TOTAL:** 400-500 hours (10-12.5 weeks)

### Resource Requirements
- **Senior C++ Developer:** 1 FTE for 3 months
- **QA Tester:** 0.5 FTE for final month
- **Code Reviewer:** 0.25 FTE throughout

---

## 7. RISK ASSESSMENT

### 7.1 High-Risk Areas

#### Performance Degradation (Risk: HIGH)
- **Impact:** Server performance worse than before
- **Mitigation:** Continuous profiling, rollback capability
- **Monitoring:** CPU/memory metrics at each phase

#### Breaking Existing Functionality (Risk: HIGH)
- **Impact:** Current working features break
- **Mitigation:** Comprehensive test suite, gradual rollout
- **Monitoring:** Regression test suite

#### Integration Complexity (Risk: MEDIUM)
- **Impact:** Unexpected interactions between systems
- **Mitigation:** Phased implementation, extensive testing
- **Monitoring:** Integration test coverage

### 7.2 Rollback Strategy

#### Phase-by-Phase Rollback
1. Each phase creates a Git tag before integration
2. Database migrations are reversible
3. Configuration can disable new systems
4. Old code paths preserved until Phase 5

#### Emergency Rollback Procedure
```bash
# 1. Stop server
systemctl stop trinitycore

# 2. Checkout last stable tag
git checkout stable-before-refactor

# 3. Reverse database migrations
mysql < rollback_migrations.sql

# 4. Restore configuration
cp playerbots.conf.backup playerbots.conf

# 5. Rebuild and restart
cmake . && make -j8
systemctl start trinitycore
```

### 7.3 Backward Compatibility Concerns

#### Breaking Changes
1. **Strategy API**: All strategies must implement new interfaces
2. **Configuration**: New format incompatible with old
3. **Database**: Schema changes require migration
4. **Custom Code**: Any custom bot code will break

#### Compatibility Layer Options
- Provide adapter classes for old strategies (adds complexity)
- Configuration migration tool (one-time conversion)
- Database migration scripts (mandatory)
- API compatibility shims (temporary, removed in Phase 5)

---

## 8. ALTERNATIVE APPROACHES

### 8.1 Incremental Refactoring (Option B-Lite)

**Approach:** Fix issues one at a time without architectural changes

**Pros:**
- Lower risk
- Can be done gradually
- No breaking changes
- 80-120 hours total

**Cons:**
- Doesn't address root causes
- Issues may resurface
- Technical debt remains
- Performance not optimal

**Implementation:**
1. Fix login timing (20 hours)
2. Fix combat relevance (20 hours)
3. Fix facing logic (20 hours)
4. Fix logout crash (20 hours)
5. Testing (20-40 hours)

### 8.2 Minimal Viable Refactor (Option B-Mini)

**Approach:** Only implement critical fixes with minimal architecture changes

**Components:**
- Simple state machine for initialization only
- Basic priority system (no full BehaviorManager)
- Minimal safe references (only for leaders)
- Simple event hooks (no full event system)

**Effort:** 160-200 hours (4-5 weeks)

**Trade-offs:**
- Fixes immediate issues
- Some architectural improvement
- Moderate risk
- Not future-proof

### 8.3 Hybrid Approach (Recommended)

**Approach:** Start with Option B-Mini, evolve to full refactor

**Phase 1:** Critical Fixes (Option A) - 2 weeks
- Fix immediate issues
- Stabilize current system
- Buy time for refactoring

**Phase 2:** Minimal Refactor - 4 weeks
- Simple state machine
- Basic safe references
- Priority system stub

**Phase 3:** Full Refactor - 6 weeks
- Complete architecture
- All systems integrated
- Future-proof design

**Benefits:**
- Immediate relief from issues
- Gradual risk assumption
- Can stop at any phase
- Learning incorporated

---

## 9. DECISION MATRIX

### Comparison of Options

| Criteria | Option A (Quick Fix) | Option B-Mini | Option B-Full | Hybrid |
|----------|---------------------|---------------|---------------|---------|
| **Time to First Fix** | 1 week | 3 weeks | 6 weeks | 1 week |
| **Total Duration** | 2 weeks | 5 weeks | 12 weeks | 12 weeks |
| **Risk Level** | Low | Medium | High | Low→High |
| **Long-term Value** | Low | Medium | High | High |
| **Performance Impact** | Minimal | Good | Excellent | Excellent |
| **Maintainability** | Poor | Good | Excellent | Excellent |
| **Breaking Changes** | None | Few | Many | Gradual |
| **Technical Debt** | Increases | Neutral | Decreases | Decreases |

### Recommendation

**For Production Systems:** Hybrid Approach
- Immediate stability from quick fixes
- Gradual improvement reduces risk
- Can adapt based on results
- Maintains service continuity

**For Development/New Systems:** Option B-Full
- Best long-term architecture
- Highest code quality
- Most maintainable
- Future-proof design

**For Time-Constrained:** Option B-Mini
- Balances improvement with speed
- Fixes core issues
- Moderate risk
- Good foundation for future

---

## 10. NEXT STEPS

### Immediate Actions (This Week)
1. **Stakeholder Decision:** Choose approach based on constraints
2. **Resource Allocation:** Assign developer(s) to project
3. **Environment Setup:** Create development branch and test environment
4. **Quick Fixes:** If Hybrid, implement Option A fixes immediately

### Planning Actions (Next Week)
1. **Detailed Design:** Create detailed technical specifications
2. **Task Breakdown:** Create JIRA/Github issues for all tasks
3. **Test Planning:** Write comprehensive test plans
4. **Review Process:** Establish code review procedures

### Implementation Start
1. **Phase 1 Kickoff:** Begin foundation work
2. **Daily Standups:** Track progress and blockers
3. **Weekly Reviews:** Assess progress and risks
4. **Continuous Integration:** Set up CI/CD pipeline

---

## APPENDIX A: Code Examples

### State Machine Example
```cpp
class BotInitStateMachine : public BotStateMachine {
    enum InitState {
        CREATED,
        LOADING,
        IN_WORLD,
        GROUP_CHECK,
        STRATEGY_INIT,
        READY
    };

    void ProcessState() override {
        switch(currentState) {
            case IN_WORLD:
                if (bot->IsInWorld()) {
                    TransitionTo(GROUP_CHECK);
                }
                break;

            case GROUP_CHECK:
                if (Group* group = bot->GetGroup()) {
                    NotifyGroupJoined(group);
                }
                TransitionTo(STRATEGY_INIT);
                break;

            case STRATEGY_INIT:
                InitializeStrategies();
                TransitionTo(READY);
                break;
        }
    }
};
```

### Behavior Priority Example
```cpp
class BehaviorManager {
    std::multimap<BehaviorPriority, Strategy*> behaviors;

    Strategy* GetActiveBehavior() {
        for (auto it = behaviors.rbegin(); it != behaviors.rend(); ++it) {
            if (it->second->CanActivate(context)) {
                // Check mutual exclusion
                if (!ConflictsWithActive(it->second)) {
                    return it->second;
                }
            }
        }
        return nullptr;
    }
};
```

### Safe Reference Example
```cpp
template<typename T>
class SafeObjectReference {
    ObjectGuid guid;
    mutable T* cached = nullptr;
    mutable uint32 lastCheck = 0;

public:
    T* Get() const {
        if (getMSTime() - lastCheck > 100) {
            cached = ObjectAccessor::GetObjectByGuid<T>(guid);
            lastCheck = getMSTime();
        }
        return cached;
    }

    bool IsValid() const {
        return Get() != nullptr;
    }
};
```

---

## APPENDIX B: Migration Guide

### For Existing Bot Users
1. Backup current configuration
2. Run migration script
3. Test bots in staging
4. Deploy to production
5. Monitor for issues

### For Custom Code
1. Update strategy interfaces
2. Replace raw pointers
3. Convert to event system
4. Test thoroughly
5. Update documentation

---

## END OF REFACTORING PLAN

This plan provides a complete roadmap for addressing the critical bot issues through comprehensive refactoring. The decision between approaches should be based on available resources, risk tolerance, and timeline constraints.