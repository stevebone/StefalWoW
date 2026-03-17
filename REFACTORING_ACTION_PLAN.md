# Playerbot Refactoring - Executive Action Plan

**Date**: 2025-10-08
**Objective**: Eliminate movement conflicts, group desyncs, and strategy overlaps
**Expected Outcome**: 51% code reduction, zero functionality loss, improved performance

---

## QUICK WINS (Can implement immediately)

### 1. Enforce BehaviorPriorityManager (1 day)
**File**: `src/modules/Playerbot/AI/BotAI.cpp`

**Current Problem**:
```cpp
// BotAI::UpdateStrategies() - runs ALL active strategies
for (auto& strategy : _strategies)
{
    if (strategy->IsActive(this))
        strategy->UpdateBehavior(this, diff);  // Multiple strategies run!
}
```

**Solution**:
```cpp
// Run ONLY the highest priority strategy
void BotAI::UpdateStrategies(uint32 diff)
{
    std::vector<Strategy*> allStrategies = GetAllStrategies();

    // Let priority manager select ONE strategy
    Strategy* activeStrategy = _priorityManager->SelectActiveBehavior(allStrategies);

    if (activeStrategy)
        activeStrategy->UpdateBehavior(this, diff);
}
```

**Impact**: Immediately fixes Issue #2 & #3 (movement conflicts)

---

### 2. Fix Strategy Priorities (30 minutes)
**Files**:
- `src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp:123`
- `src/modules/Playerbot/AI/Strategy/CombatMovementStrategy.cpp:47`

**Changes**:
```cpp
// LeaderFollowBehavior.cpp:123
LeaderFollowBehavior::LeaderFollowBehavior()
    : Strategy("follow")
{
    _priority = 50;  // Changed from 200
}

// CombatMovementStrategy.cpp:47
CombatMovementStrategy::CombatMovementStrategy()
    : Strategy("CombatMovement")
{
    SetPriority(100);  // Changed from 80
    _exclusive = true;  // Add this line
}
```

**Impact**: Ensures combat has exclusive control over movement

---

### 3. Add Group Events (2 hours)
**File**: `src/modules/Playerbot/Core/Events/BotEventTypes.h`

**Add event types**:
```cpp
enum class EventType
{
    // ... existing events ...

    // Group events
    GROUP_JOINED,
    GROUP_LEFT,
    GROUP_LEADER_CHANGED,
    GROUP_DISBANDED,
};
```

**File**: `src/modules/Playerbot/Group/GroupInvitationHandler.cpp:500`

**Dispatch event on successful join**:
```cpp
void GroupInvitationHandler::AcceptInvitation(ObjectGuid inviterGuid)
{
    // ... existing accept logic ...

    if (acceptSuccess)
    {
        // NEW: Dispatch event
        BotEvent evt(EventType::GROUP_JOINED, _bot->GetGUID(), inviterGuid);
        evt.group = _bot->GetGroup();
        _eventDispatcher->Dispatch(std::move(evt));
    }
}
```

**File**: `src/modules/Playerbot/AI/BotAI.cpp:54`

**Subscribe to events in constructor**:
```cpp
BotAI::BotAI(Player* bot) : _bot(bot)
{
    // ... existing initialization ...

    // Subscribe to group events
    _eventDispatcher->Subscribe(EventType::GROUP_JOINED, this);
    _eventDispatcher->Subscribe(EventType::GROUP_LEFT, this);
}
```

**Impact**: Eliminates 1s polling lag, instant strategy activation

---

## MEDIUM-TERM IMPROVEMENTS (Week 1-2)

### 4. Standardize Movement Execution (3 days)
**Goal**: ALL movement flows through BotMovementUtil

**Files to modify**:
- `src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp:1234`
- `src/modules/Playerbot/AI/Strategy/CombatMovementStrategy.cpp:425`
- `src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp:132`

**Pattern to apply**:
```cpp
// BEFORE (direct MotionMaster call):
bot->GetMotionMaster()->Clear();
bot->GetMotionMaster()->MovePoint(1, position);

// AFTER (use BotMovementUtil):
BotMovementUtil::MoveToPosition(bot, position, 1);
```

**Impact**: Eliminates stuttering/blinking, deduplicates movement commands

---

### 5. Merge GroupCombatStrategy into CombatMovementStrategy (2 days)
**Files**:
- Move logic from `src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp` to `CombatMovementStrategy.cpp`
- Delete `GroupCombatStrategy.cpp` and `GroupCombatStrategy.h`

**New CombatMovementStrategy structure**:
```cpp
void CombatMovementStrategy::UpdateBehavior(BotAI* ai, uint32 diff)
{
    // 1. Check if group in combat but bot isn't (assist mode)
    if (!bot->IsInCombat() && IsGroupInCombat(ai))
    {
        Unit* groupTarget = GetGroupMemberTarget(ai);
        if (groupTarget)
        {
            bot->SetTarget(groupTarget->GetGUID());
            bot->Attack(groupTarget, true);
        }
    }

    // 2. Handle combat positioning (existing logic)
    if (bot->IsInCombat())
    {
        // ... existing combat positioning ...
    }
}
```

**Impact**: 272 lines removed, clearer responsibility

---

### 6. Add Throttling to LeaderFollowBehavior (1 hour)
**File**: `src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp:297`

**Change**:
```cpp
void LeaderFollowBehavior::UpdateBehavior(BotAI* ai, uint32 diff)
{
    // Add throttling
    static uint32 updateTimer = 0;
    updateTimer += diff;
    if (updateTimer < 100)  // 10 Hz update rate
        return;
    updateTimer = 0;

    // Now call existing logic
    UpdateFollowBehavior(ai, diff);
}
```

**Impact**: Reduces CPU usage from ~0.3% to ~0.05% per bot

---

## LONG-TERM CLEANUP (Week 3-4)

### 7. Remove ClassAI Movement Logic (5 days)
**Goal**: ClassAI only provides optimal range/angle, never executes movement

**Files to modify** (all in `src/modules/Playerbot/AI/ClassAI/`):
- `Hunters/HunterAI.cpp`
- `Mages/MageAI.cpp`
- `Warriors/WarriorAI.cpp`
- `Priests/PriestAI.cpp`
- (etc. for all 13 classes)

**Pattern**:
```cpp
// REMOVE all movement execution:
void HunterAI::UpdateAI(uint32 diff)
{
    // DELETE THIS:
    if (distance > 25.0f)
        bot->GetMotionMaster()->MoveChase(target, 25.0f);
}

// KEEP only range calculation:
float HunterAI::GetOptimalRange(Unit* target) override
{
    return 25.0f;  // This is all ClassAI should provide
}
```

**Impact**: ~200 lines removed, consistent behavior across all classes

---

### 8. Remove BotAI Periodic Group Check (15 minutes)
**File**: `src/modules/Playerbot/AI/BotAI.cpp:216-242`

**Delete this workaround**:
```cpp
// DELETE LINES 216-242:
static uint32 groupCheckTimer = 0;
groupCheckTimer += diff;
if (groupCheckTimer >= 1000)
{
    // This entire block - delete it
}
```

**Prerequisite**: Must complete step #3 (Add Group Events) first

**Impact**: Cleaner code, instant response instead of 1s lag

---

## CODE REDUCTION TRACKER

| Component | Before | After | Reduction |
|-----------|--------|-------|-----------|
| LeaderFollowBehavior | 1,497 | 800 | 697 lines (47%) |
| CombatMovementStrategy | 734 | 400 | 334 lines (45%) |
| GroupCombatStrategy | 272 | 0 | 272 lines (100%) |
| BotAI periodic checks | 27 | 0 | 27 lines (100%) |
| ClassAI movement code | ~200 | 0 | 200 lines (100%) |
| **TOTAL** | **~2,730** | **~1,200** | **~1,530 lines (56%)** |

---

## TESTING CHECKLIST

After each change, verify:

### Movement System Tests
- [ ] Bot follows player smoothly (no stuttering/blinking)
- [ ] Bot stops movement when entering combat
- [ ] Bot resumes following after combat ends
- [ ] Melee classes stay at 5yd, ranged at 25yd
- [ ] Bots avoid fire/poison ground effects
- [ ] Pathfinding still works correctly

### Group System Tests
- [ ] Bot accepts group invite instantly
- [ ] Follow strategy activates within 50ms of group join
- [ ] Bot leaves group cleanly (no errors in log)
- [ ] Idle strategy activates when group disbanded
- [ ] Multiple bots can join same group

### Strategy Priority Tests
- [ ] Only ONE strategy executes at a time
- [ ] Combat strategy has exclusive control during combat
- [ ] Follow strategy never runs during combat
- [ ] Priority transitions are instant (<50ms)

### Performance Tests
- [ ] CPU per bot: <0.1% (down from ~0.3%)
- [ ] Memory per bot: <10MB (unchanged)
- [ ] Strategy selection: <0.01ms per frame
- [ ] Movement deduplication: 90%+ command reduction

---

## ROLLBACK PLAN

If any change causes issues:

### Git Workflow
```bash
# Before making changes, create a backup branch
git checkout -b refactor-backup-YYYYMMDD

# If something breaks, revert
git checkout playerbot-dev
git reset --hard refactor-backup-YYYYMMDD
```

### Per-Feature Rollback
Each feature is independent and can be rolled back separately:

1. **BehaviorPriorityManager enforcement** → Revert `BotAI.cpp:UpdateStrategies()`
2. **Strategy priorities** → Revert constructor changes
3. **Group events** → Comment out event dispatch, restore periodic check
4. **Movement standardization** → Revert to direct MotionMaster calls
5. **GroupCombatStrategy merge** → Restore deleted files from git
6. **Throttling** → Remove throttling code
7. **ClassAI cleanup** → Revert ClassAI movement removal
8. **Periodic check removal** → Restore deleted code

---

## DEPENDENCIES

### Internal (Playerbot Module)
- ✅ EventDispatcher exists and works
- ✅ BehaviorPriorityManager exists
- ✅ BotMovementUtil exists
- ✅ Strategy base class supports priority

### External (TrinityCore)
- ✅ MotionMaster APIs stable
- ✅ Group system APIs stable
- ✅ No TrinityCore changes required

### Configuration
- ⚠️ May need to adjust `playerbots.conf` for throttling intervals
- ⚠️ Recommend adding `Playerbot.MovementUpdateInterval = 100` (ms)

---

## SUCCESS METRICS

### Before Refactoring
- Movement conflicts: ~30% of combat encounters
- Strategy overlap: 3+ strategies running simultaneously
- CPU per bot: ~0.3%
- Code duplication: ~40%
- Group join lag: ~1 second

### After Refactoring
- Movement conflicts: 0%
- Strategy overlap: 1 strategy at a time (enforced)
- CPU per bot: <0.1%
- Code duplication: <5%
- Group join lag: <50ms

---

## PHASED ROLLOUT

### Week 1: Foundation
- Day 1: Enforce BehaviorPriorityManager
- Day 2: Fix strategy priorities
- Day 3: Add group events
- Day 4-5: Testing and validation

### Week 2: Movement
- Day 1-3: Standardize movement to BotMovementUtil
- Day 4: Merge GroupCombatStrategy
- Day 5: Add throttling

### Week 3: Cleanup
- Day 1-3: Remove ClassAI movement
- Day 4: Remove periodic checks
- Day 5: Final testing

### Week 4: Polish
- Day 1-2: Performance optimization
- Day 3-4: Documentation
- Day 5: Release candidate

---

## APPROVAL CHECKLIST

Before implementing:
- [ ] Review PLAYERBOT_ARCHITECTURE_DEEP_ANALYSIS.md
- [ ] Understand the 3 core issues (movement conflicts, group desync, strategy overlap)
- [ ] Agree on the solution approach
- [ ] Commit to NO shortcuts (full implementation)
- [ ] Allocate 4 weeks for complete refactoring
- [ ] Set up testing environment

---

## QUESTIONS TO ANSWER

1. **Should we implement all changes at once or phase them in?**
   - Recommendation: Phased (Week 1 → Week 2 → Week 3)

2. **What's the rollback strategy if something breaks?**
   - Recommendation: Git branch per phase, easy rollback

3. **Do we need to notify users of behavior changes?**
   - Recommendation: Yes - movement will be smoother but might look different

4. **Should we keep dead code commented out or delete it?**
   - Recommendation: Delete (git history preserves it)

5. **What happens to existing saved bot data?**
   - Recommendation: No impact - all changes are code-only

---

## NEXT STEPS

1. **Review this document** and the detailed analysis
2. **Approve the approach** (or request changes)
3. **Choose rollout strategy** (all-at-once vs phased)
4. **Create git branch** for refactoring work
5. **Implement Quick Wins** (Day 1-2)
6. **Validate** with testing checklist
7. **Proceed to Medium-Term** improvements

---

**READY TO PROCEED?** Start with Quick Win #1 (Enforce BehaviorPriorityManager) - it's the highest impact, lowest risk change.
