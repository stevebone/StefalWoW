# REFACTORING CONTINUATION PLAN - Post Phase 1

## Executive Summary

**Phase 1 (State Machine Foundation) is COMPLETE**. This document outlines the continuation plan that harmonizes with the EXISTING Phase 2 infrastructure (BehaviorManager, IdleStrategy, CombatMovementStrategy) that was completed previously.

**Critical Understanding:**
- ✅ **OLD Phase 2** (BehaviorManager infrastructure) = ALREADY DONE
- ✅ **NEW Phase 1** (State Machine) = JUST COMPLETED
- 🔄 **Next**: Continue with behavior priority and combat fixes

---

## ARCHITECTURE HARMONIZATION

### What We Now Have (Phase 1 + Old Phase 2)

```
┌─────────────────────────────────────────────────────────────┐
│                    COMPLETED LAYERS                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  NEW Phase 1: State Machine Layer (JUST COMPLETED)   │  │
│  │  ────────────────────────────────────────────────────│  │
│  │  • BotStateMachine (lifecycle management)            │  │
│  │  • BotInitStateMachine (fixes Issue #1)              │  │
│  │  • SafeObjectReference (fixes Issue #4)              │  │
│  │  • Event system skeleton                              │  │
│  └──────────────────────────────────────────────────────┘  │
│                           ↓ uses                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  OLD Phase 2: Infrastructure Layer (ALREADY DONE)    │  │
│  │  ────────────────────────────────────────────────────│  │
│  │  • BehaviorManager (throttled updates, atomics)      │  │
│  │  • IdleStrategy (observer pattern)                   │  │
│  │  • CombatMovementStrategy (role positioning)         │  │
│  │  • QuestManager, TradeManager, etc. (refactored)     │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                              │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    REMAINING WORK                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Phase 2: Behavior Priority System (70 hours)               │
│  ────────────────────────────────────────────────────────── │
│  • EXTEND existing BehaviorManager with priority logic      │
│  • Fix combat target acquisition (Issue #2)                 │
│  • Fix LeaderFollowBehavior relevance during combat (#2/#3) │
│  • Implement mutual exclusion rules                         │
│                                                              │
│  Phase 3: Safe References - PARTIALLY DONE (20 hours)       │
│  ────────────────────────────────────────────────────────── │
│  • SafeObjectReference already created in Phase 1 ✅        │
│  • Just need: Apply to all remaining raw pointers           │
│  • ReferenceValidator utilities                             │
│  • Integration with ObjectCache                             │
│                                                              │
│  Phase 4: Event System Full Implementation (70 hours)       │
│  ────────────────────────────────────────────────────────── │
│  • Expand BotEventTypes.h skeleton from Phase 1             │
│  • BotEventSystem dispatcher                                │
│  • Group/Combat/World event observers                       │
│  • Convert polling to event-driven                          │
│                                                              │
│  Phase 5: Integration & Testing (50 hours)                  │
│  ────────────────────────────────────────────────────────── │
│  • Validate all 4 issues fixed                              │
│  • Performance testing                                      │
│  • Production deployment                                    │
│                                                              │
└─────────────────────────────────────────────────────────────┘

TOTAL REMAINING: ~210 hours (reduced from 365 because Phase 1 done + Phase 3 partially done)
```

---

## PHASE 2: BEHAVIOR PRIORITY SYSTEM (70 hours)

### Objective
Fix Issues #2 & #3 by implementing behavior priority and combat coordination.

### What EXISTS (Don't Recreate)
- ✅ BehaviorManager base class (AI/BehaviorManager.h)
- ✅ Throttling system with atomic flags
- ✅ Template method pattern for updates
- ✅ IdleStrategy observer pattern
- ✅ CombatMovementStrategy framework

### What's MISSING (Must Create)
- ❌ Priority-based behavior selection
- ❌ Mutual exclusion rules between behaviors
- ❌ Combat target acquisition robustness
- ❌ Follow behavior combat relevance fix

### Components to Build

#### 2.1 BehaviorPriorityManager.h/cpp (NEW - 400 lines)
```cpp
// EXTENDS the existing BehaviorManager, doesn't replace it
namespace Playerbot {

enum class BehaviorPriority : uint8_t {
    DEAD = 0,
    CRITICAL_ERROR = 10,
    COMBAT = 100,        // Highest operational priority
    FLEEING = 90,
    FOLLOW = 50,         // Mid - only when not in combat
    GATHERING = 40,
    TRADING = 30,
    IDLE = 10            // Lowest
};

class BehaviorPriorityManager {
public:
    // Select the highest priority ACTIVE behavior
    Strategy* SelectActiveBehavior(std::vector<Strategy*>& strategies);

    // Check if two behaviors can run simultaneously
    bool CanCoexist(Strategy* a, Strategy* b);

    // Register mutual exclusion rules
    void AddExclusionRule(BehaviorPriority a, BehaviorPriority b);

private:
    std::map<BehaviorPriority, std::vector<BehaviorPriority>> m_exclusionRules;
};

} // namespace
```

#### 2.2 Update LeaderFollowBehavior.cpp (MODIFY - 50 lines)
```cpp
// FIX FOR ISSUES #2 & #3
float LeaderFollowBehavior::GetRelevance(BotAI* ai) const {
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsAlive())
        return 0.0f;

    // CRITICAL FIX: ZERO relevance during combat
    // This prevents follow from interfering with combat positioning
    if (bot->IsInCombat()) {
        TC_LOG_TRACE("module.playerbot.follow",
            "Bot {} in combat - follow behavior disabled (FIX FOR ISSUE #2 & #3)",
            bot->GetName());
        return 0.0f;  // Changed from 10.0f to 0.0f
    }

    // Normal follow logic when not in combat
    // ... existing code ...
}
```

#### 2.3 Update ClassAI.cpp - Target Acquisition (MODIFY - 80 lines)
```cpp
// FIX FOR ISSUE #2: Robust target acquisition
void ClassAI::OnCombatUpdate(uint32 diff) {
    if (!GetBot() || !GetBot()->IsAlive())
        return;

    // CRITICAL FIX: Ensure we have a target before doing anything
    if (!_currentCombatTarget || !_currentCombatTarget->IsAlive()) {
        // Priority 1: Bot's current victim
        _currentCombatTarget = GetBot()->GetVictim();

        // Priority 2: Group leader's target
        if (!_currentCombatTarget) {
            if (Group* group = GetBot()->GetGroup()) {
                Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
                if (leader && leader->IsInCombat()) {
                    _currentCombatTarget = leader->GetVictim();
                }
            }
        }

        // Priority 3: Nearest enemy
        if (!_currentCombatTarget) {
            _currentCombatTarget = GetNearestHostileTarget(40.0f);
        }

        // If still no target, return and wait
        if (!_currentCombatTarget) {
            TC_LOG_DEBUG("module.playerbot.classai",
                "Bot {} in combat but no valid target found - waiting",
                GetBot()->GetName());
            return;
        }

        TC_LOG_DEBUG("module.playerbot.classai",
            "Bot {} acquired combat target: {} (FIX FOR ISSUE #2)",
            GetBot()->GetName(), _currentCombatTarget->GetName());
    }

    // Verify target is still valid
    if (!_currentCombatTarget->IsAlive() || !_currentCombatTarget->IsInWorld()) {
        _currentCombatTarget = nullptr;
        return;
    }

    // Now continue with combat logic (existing code)
    UpdateCombatMovement(diff);
    UpdateRotation(diff);
    UpdateDefensives(diff);
    UpdateBuffs(diff);
}

// FIX FOR ISSUE #3: Explicit facing
void ClassAI::UpdateCombatMovement(uint32 diff) {
    if (!_currentCombatTarget)
        return;

    float currentDistance = GetBot()->GetDistance(_currentCombatTarget);
    float optimalRange = GetOptimalCombatRange();
    float rangeTolerance = 2.0f;

    if (currentDistance > optimalRange + rangeTolerance) {
        BotMovementUtil::ChaseTarget(GetBot(), _currentCombatTarget, optimalRange);
        GetBot()->SetFacingToObject(_currentCombatTarget); // FIX: Explicit facing
    } else if (currentDistance < optimalRange - rangeTolerance && IsRangedClass()) {
        BotMovementUtil::MoveAwayFrom(GetBot(), _currentCombatTarget, optimalRange);
        GetBot()->SetFacingToObject(_currentCombatTarget); // FIX: Maintain facing
    } else {
        // In optimal range - just face the target
        GetBot()->SetFacingToObject(_currentCombatTarget); // FIX: Always face target
    }
}
```

#### 2.4 Integration with BotAI.cpp (MODIFY - 100 lines)
```cpp
void BotAI::UpdateAI(uint32 diff) {
    // Phase 1 already added: State machine initialization
    if (!m_initStateMachine || !m_initStateMachine->IsReady()) {
        return; // Skip until initialized
    }

    // NEW: Behavior priority selection
    if (!m_behaviorPriorityManager) {
        m_behaviorPriorityManager = std::make_unique<BehaviorPriorityManager>();
        ConfigurePriorityRules();
    }

    // Collect all active strategies
    std::vector<Strategy*> activeStrategies;
    for (auto& strategy : m_strategies) {
        if (strategy->IsActive()) {
            activeStrategies.push_back(strategy.get());
        }
    }

    // Select highest priority behavior that can run
    Strategy* activeBehavior = m_behaviorPriorityManager->SelectActiveBehavior(activeStrategies);

    if (activeBehavior) {
        activeBehavior->Update(diff);
    }

    // Managers always update (they have their own throttling)
    UpdateManagers(diff);
}

void BotAI::ConfigurePriorityRules() {
    // Combat and Follow are mutually exclusive
    m_behaviorPriorityManager->AddExclusionRule(
        BehaviorPriority::COMBAT,
        BehaviorPriority::FOLLOW
    );

    // Fleeing overrides everything except critical errors
    m_behaviorPriorityManager->AddExclusionRule(
        BehaviorPriority::FLEEING,
        BehaviorPriority::COMBAT
    );

    // ... more rules as needed
}
```

### Task Breakdown

| Task | Description | Hours | Agent |
|------|-------------|-------|-------|
| 2.1 | Create BehaviorPriorityManager.h/cpp | 12 | cpp-architecture-optimizer |
| 2.2 | Update LeaderFollowBehavior.cpp | 4 | general-purpose |
| 2.3 | Update ClassAI.cpp target acquisition | 8 | wow-mechanics-expert |
| 2.4 | Update ClassAI.cpp combat movement | 6 | wow-mechanics-expert |
| 2.5 | Integrate with BotAI.cpp | 8 | cpp-architecture-optimizer |
| 2.6 | Add mutual exclusion rules | 6 | general-purpose |
| 2.7 | Create unit tests | 12 | test-automation-engineer |
| 2.8 | Integration testing | 8 | test-automation-engineer |
| 2.9 | Performance validation | 4 | general-purpose |
| 2.10 | Documentation | 2 | general-purpose |

**Total: 70 hours**

---

## PHASE 3: SAFE REFERENCES COMPLETION (20 hours)

### Status: 70% Complete from Phase 1

Phase 1 already created:
- ✅ SafeObjectReference<T> template (fixes Issue #4)
- ✅ Thread-safe caching
- ✅ ObjectGuid validation
- ✅ Unit tests

### Remaining Work (20 hours)

#### 3.1 Apply to All Remaining Raw Pointers (12 hours)
```cpp
// Replace ALL occurrences of:
Player* m_target;           // UNSAFE
Player* m_groupLeader;      // UNSAFE (already fixed in Phase 1 for BotAI)
Creature* m_followTarget;   // UNSAFE

// With:
SafePlayerReference m_target;
SafePlayerReference m_groupLeader;
SafeCreatureReference m_followTarget;
```

**Files to update** (~15 files):
- All ClassAI files (13 classes)
- All specialization files
- Movement behaviors
- Combat strategies

#### 3.2 ReferenceValidator Utilities (4 hours)
```cpp
class ReferenceValidator {
public:
    // Batch validation for multiple references
    static bool ValidateAll(std::vector<SafeObjectReference<Player>*> refs);

    // Validate reference with logging
    static Player* ValidateAndLog(SafePlayerReference& ref, const char* context);

    // Clean up invalid references
    static void CleanupInvalidReferences(BotAI* ai);
};
```

#### 3.3 Integration & Testing (4 hours)
- Update all reference usage sites
- Add validation tests
- Performance regression tests

---

## PHASE 4: EVENT SYSTEM FULL IMPLEMENTATION (70 hours)

### Status: Skeleton exists from Phase 1

Phase 1 created:
- ✅ BotEventTypes.h skeleton
- ✅ BotEvent structure
- ✅ IEventObserver interface
- ✅ Forward declarations

### Remaining Work (70 hours)

#### 4.1 BotEventSystem.h/cpp (20 hours)
```cpp
class BotEventSystem {
public:
    // Register observer for specific event types
    void RegisterObserver(IEventObserver* observer, std::vector<EventType> interests);

    // Dispatch event to all interested observers
    void DispatchEvent(const BotEvent& event);

    // Priority-based event queue
    void QueueEvent(const BotEvent& event, EventPriority priority);

    // Process queued events
    void ProcessEvents();

private:
    std::map<EventType, std::vector<IEventObserver*>> m_observers;
    std::priority_queue<BotEvent> m_eventQueue;
};
```

#### 4.2 Group Event Observer (15 hours)
```cpp
class GroupEventObserver : public IEventObserver {
public:
    void OnEvent(const BotEvent& event) override;

private:
    void HandleGroupJoin(const BotEvent& event);
    void HandleGroupLeave(const BotEvent& event);
    void HandleLeaderChange(const BotEvent& event);
    void HandleLeaderLogout(const BotEvent& event); // FIX FOR ISSUE #4
};
```

#### 4.3 Combat Event Observer (15 hours)
#### 4.4 World Event Observer (10 hours)
#### 4.5 Integration & Testing (10 hours)

---

## PHASE 5: FINAL INTEGRATION & TESTING (50 hours)

### 5.1 Issue Validation (15 hours)
- ✅ Issue #1: Bot in group at login (validate fix works)
- ✅ Issue #2: Ranged combat (validate target acquisition + relevance fix)
- ✅ Issue #3: Melee facing (validate facing fix)
- ✅ Issue #4: Logout crash (validate SafeObjectReference works)

### 5.2 Performance Testing (15 hours)
- 5000 bot stress test
- Memory leak testing
- Thread safety validation
- CPU profiling

### 5.3 Production Deployment (10 hours)
- Build verification
- Database migration scripts
- Configuration guide
- Deployment documentation

### 5.4 Final Documentation (10 hours)
- Architecture overview
- Developer guide
- API documentation
- Migration guide

---

## CRITICAL SUCCESS FACTORS

### 1. Harmonization Checklist
- ✅ Don't recreate BehaviorManager (already exists)
- ✅ Don't recreate IdleStrategy (already exists)
- ✅ Don't recreate CombatMovementStrategy (already exists)
- ✅ EXTEND existing components, don't replace
- ✅ Use Phase 1's state machine for initialization
- ✅ Use Phase 1's SafeObjectReference for safety

### 2. Integration Points
- BehaviorPriorityManager EXTENDS BehaviorManager
- ClassAI updates USE CombatMovementStrategy
- Event system NOTIFIES existing strategies
- State machine COORDINATES with existing managers

### 3. No Breaking Changes
- Existing Phase 2 code continues to work
- Phase 1 components integrate seamlessly
- All new features are additive
- Backward compatibility maintained

---

## ESTIMATED COMPLETION

| Phase | Hours | Status |
|-------|-------|--------|
| Phase 1 (State Machine) | 85 | ✅ COMPLETE |
| Phase 2 (Behavior Priority) | 70 | 🔄 NEXT |
| Phase 3 (Safe Refs Complete) | 20 | ⏳ After Phase 2 |
| Phase 4 (Event System) | 70 | ⏳ After Phase 3 |
| Phase 5 (Integration) | 50 | ⏳ After Phase 4 |

**Total Remaining**: 210 hours (~5.3 weeks @ 40hrs/week)

---

## NEXT IMMEDIATE STEPS

1. **Start Phase 2.1**: Create BehaviorPriorityManager.h/cpp
2. **Update LeaderFollowBehavior**: Fix relevance to 0.0f during combat
3. **Update ClassAI**: Fix target acquisition and facing
4. **Integrate with BotAI**: Priority-based strategy selection
5. **Test**: Validate Issues #2 & #3 are fixed

**Ready to begin Phase 2 implementation.**

---

*Continuation Plan Created: 2025-10-06*
*Current Status: Phase 1 Complete, Phase 2 Ready*
*Total Project: 295 hours (85 done + 210 remaining)*
