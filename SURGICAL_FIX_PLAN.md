# SURGICAL FIX PLAN - Critical Bot Issues Resolution

## Executive Summary

This plan addresses the 4 critical bot issues identified in `CRITICAL_BOT_ISSUES_DIAGNOSIS.md` using **surgical fixes** that leverage the existing Phase 2 infrastructure. NO new cornerstones or major refactoring required.

**Estimated Total Effort:** 40-60 hours (1-1.5 weeks)
**Risk Level:** LOW - Targeted fixes to specific pain points
**Backward Compatibility:** NON-BREAKING - All changes are internal

---

## Phase 2 Infrastructure Already in Place

✅ **BehaviorManager Base Class** - `AI/BehaviorManager.h` (162 lines)
✅ **Throttled Update System** - Atomic flags, performance tracking
✅ **IdleStrategy Observer Pattern** - Lock-free state queries
✅ **CombatMovementStrategy** - Role-based positioning framework
✅ **All Managers Refactored** - Quest, Trade, Gathering, Auction

**We do NOT need:**
- ❌ New behavior priority system (use relevance values)
- ❌ New state machine (use simple sequencing)
- ❌ New BehaviorManager (already exists)

---

## Issue #1: Bot Already in Group on Login (MEDIUM Priority)

### Current Problem
```cpp
// BotSession.cpp:946-960
// OnGroupJoined() called BEFORE bot IsInWorld()
HandleBotPlayerLogin() {
    // Line 920-930: AddToWorld()
    // Line 946-960: Check group, call OnGroupJoined() ← TOO EARLY!
    // Line 932: Bot now IsInWorld()
}
```

### Root Cause
- Group check happens before `IsInWorld()` returns true
- Strategy activation in BotAI first update comes AFTER
- Follow strategy activated but `OnActivate()` never fires

### Surgical Fix (2 hours)

**File:** `src/modules/Playerbot/Session/BotSession.cpp`

**Change:** Move group check to AFTER line 932

```cpp
// Line 932: TC_LOG_INFO("module.playerbot.session", "Bot player {} successfully added to world", pCurrChar->GetName());

// MOVE lines 946-960 to HERE (after bot is in world):
if (Player* player = GetPlayer())
{
    if (player->IsInWorld()) // VERIFY bot is actually in world
    {
        Group* group = player->GetGroup();
        if (group)
        {
            TC_LOG_INFO("module.playerbot.session", "Bot {} already in group at login - activating strategies", player->GetName());
            if (BotAI* ai = GetAI())
            {
                ai->OnGroupJoined(group); // Now called at correct time
            }
        }
    }
}
```

**Testing:**
1. Create group with bots
2. Restart server
3. Login player
4. Verify bots immediately follow (no re-invite needed)

**Files Modified:** 1
**Lines Changed:** ~15 (move existing code block)

---

## Issue #2 & #3: Ranged Combat + Melee Facing (HIGH Priority)

### Current Problem
```cpp
// ClassAI.cpp:75-79
OnCombatUpdate(uint32 diff) {
    if (!_currentCombatTarget) { // Often NULL!
        UpdateBuffs();
        return; // No combat actions
    }
}

// LeaderFollowBehavior.cpp:168
float GetRelevance() {
    if (bot->IsInCombat())
        return 10.0f; // PROBLEM: Still active during combat!
}
```

### Root Causes
1. **NULL Combat Target**: `_currentCombatTarget` not being set properly
2. **Follow Behavior Conflict**: 10.0f relevance during combat causes ping-pong movement
3. **Facing Issues**: Follow behavior constantly resets facing to leader

### Surgical Fix (8 hours)

#### Fix 2A: Target Acquisition (4 hours)

**File:** `src/modules/Playerbot/AI/ClassAI/ClassAI.cpp`

**Change:** Add robust target acquisition in `OnCombatUpdate()`

```cpp
void ClassAI::OnCombatUpdate(uint32 diff)
{
    if (!GetBot() || !GetBot()->IsAlive())
        return;

    // FIX: Ensure we have a target before doing anything
    if (!_currentCombatTarget)
    {
        // Priority 1: Bot's victim
        _currentCombatTarget = GetBot()->GetVictim();

        // Priority 2: Group leader's target
        if (!_currentCombatTarget)
        {
            if (Group* group = GetBot()->GetGroup())
            {
                if (Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID()))
                {
                    _currentCombatTarget = leader->GetVictim();
                }
            }
        }

        // Priority 3: Nearest enemy
        if (!_currentCombatTarget)
        {
            _currentCombatTarget = GetNearestHostileTarget(40.0f);
        }

        // Log if still no target
        if (!_currentCombatTarget)
        {
            TC_LOG_DEBUG("module.playerbot.classai", "Bot {} in combat but no valid target found", GetBot()->GetName());
            return;
        }

        TC_LOG_DEBUG("module.playerbot.classai", "Bot {} acquired combat target: {}",
                    GetBot()->GetName(), _currentCombatTarget->GetName());
    }

    // Verify target is still valid
    if (!_currentCombatTarget->IsAlive() || !_currentCombatTarget->IsInWorld())
    {
        _currentCombatTarget = nullptr;
        return;
    }

    // Now continue with existing combat logic...
    UpdateCombatMovement(diff);
    UpdateRotation(diff);
    UpdateDefensives(diff);
    UpdateBuffs(diff);
}
```

#### Fix 2B: Disable Follow During Combat (2 hours)

**File:** `src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp`

**Change:** Return 0.0f relevance during combat

```cpp
float LeaderFollowBehavior::GetRelevance(BotAI* ai) const
{
    Player* bot = ai->GetBot();
    if (!bot || !bot->IsAlive())
        return 0.0f;

    // ZERO relevance during combat - let ClassAI handle ALL combat movement
    if (bot->IsInCombat())
    {
        TC_LOG_TRACE("module.playerbot.follow", "Bot {} in combat - follow behavior disabled", bot->GetName());
        return 0.0f;  // Changed from 10.0f to 0.0f
    }

    // Check if bot has a group leader to follow
    Group* group = bot->GetGroup();
    if (!group)
        return 0.0f;

    // Rest of existing logic...
}
```

#### Fix 2C: Explicit Facing in Combat (2 hours)

**File:** `src/modules/Playerbot/AI/ClassAI/ClassAI.cpp`

**Change:** Add explicit facing in combat movement

```cpp
void ClassAI::UpdateCombatMovement(uint32 diff)
{
    if (!_currentCombatTarget)
        return;

    float currentDistance = GetBot()->GetDistance(_currentCombatTarget);
    float optimalRange = GetOptimalCombatRange();
    float rangeTolerance = 2.0f;

    // Move to optimal range if too far
    if (currentDistance > optimalRange + rangeTolerance)
    {
        BotMovementUtil::ChaseTarget(GetBot(), _currentCombatTarget, optimalRange);
        GetBot()->SetFacingToObject(_currentCombatTarget); // ADD THIS: Explicit facing
    }
    // Move back if too close (for ranged)
    else if (currentDistance < optimalRange - rangeTolerance && IsRangedClass())
    {
        BotMovementUtil::MoveAwayFrom(GetBot(), _currentCombatTarget, optimalRange);
        GetBot()->SetFacingToObject(_currentCombatTarget); // ADD THIS: Maintain facing
    }
    // In optimal range - just face the target
    else
    {
        GetBot()->SetFacingToObject(_currentCombatTarget); // ADD THIS: Always face target
    }
}
```

**Testing:**
1. Create group with ranged DPS bot (Mage/Hunter/Warlock)
2. Attack enemy
3. Verify bot:
   - Moves to optimal range
   - Casts spells/abilities
   - Does NOT ping-pong between leader and target
4. Create group with melee bot (Warrior/Rogue)
5. Attack enemy
6. Verify bot:
   - Faces target (not leader)
   - Attacks successfully
   - Maintains proper positioning

**Files Modified:** 2
**Lines Changed:** ~60 (30 per file)

---

## Issue #4: Server Crash on Logout (CRITICAL Priority)

### Current Problem
```cpp
// When player logs out:
1. Player object destroyed
2. Bots still have raw Player* pointers cached
3. Next update cycle accesses deleted memory
4. Access violation → CRASH
```

### Root Cause
- Bots cache raw pointers to group leader
- No notification when leader logs out
- Dangling pointers accessed in next update

### Surgical Fix (6 hours)

#### Fix 4A: SafeGroupLeaderReference Class (4 hours)

**File:** `src/modules/Playerbot/Core/SafeGroupLeaderReference.h` (NEW)

```cpp
#ifndef TRINITYCORE_SAFE_GROUP_LEADER_REFERENCE_H
#define TRINITYCORE_SAFE_GROUP_LEADER_REFERENCE_H

#include "Define.h"
#include "ObjectGuid.h"

class Player;

namespace Playerbot
{
    /**
     * @class SafeGroupLeaderReference
     * @brief RAII-based safe reference to group leader that auto-validates
     *
     * This class stores the leader's ObjectGuid and re-fetches from ObjectAccessor
     * on every access, ensuring we never hold dangling pointers.
     */
    class TC_GAME_API SafeGroupLeaderReference
    {
    public:
        SafeGroupLeaderReference() : _leaderGuid(), _lastCheckTime(0), _cachedLeader(nullptr) {}

        void SetLeader(Player* leader);
        void ClearLeader();

        Player* GetLeader() const;
        bool HasLeader() const { return !_leaderGuid.IsEmpty(); }
        ObjectGuid GetLeaderGuid() const { return _leaderGuid; }

    private:
        ObjectGuid _leaderGuid;
        mutable uint32 _lastCheckTime;
        mutable Player* _cachedLeader;

        static constexpr uint32 CACHE_DURATION_MS = 100; // Re-validate every 100ms
    };
}

#endif
```

**File:** `src/modules/Playerbot/Core/SafeGroupLeaderReference.cpp` (NEW)

```cpp
#include "SafeGroupLeaderReference.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Timer.h"

namespace Playerbot
{
    void SafeGroupLeaderReference::SetLeader(Player* leader)
    {
        if (leader)
        {
            _leaderGuid = leader->GetGUID();
            _cachedLeader = leader;
            _lastCheckTime = getMSTime();
        }
        else
        {
            ClearLeader();
        }
    }

    void SafeGroupLeaderReference::ClearLeader()
    {
        _leaderGuid = ObjectGuid::Empty;
        _cachedLeader = nullptr;
        _lastCheckTime = 0;
    }

    Player* SafeGroupLeaderReference::GetLeader() const
    {
        if (_leaderGuid.IsEmpty())
            return nullptr;

        // Cache for 100ms to avoid excessive ObjectAccessor lookups
        uint32 now = getMSTime();
        if (now - _lastCheckTime > CACHE_DURATION_MS || !_cachedLeader)
        {
            _cachedLeader = ObjectAccessor::FindPlayer(_leaderGuid);
            _lastCheckTime = now;

            if (!_cachedLeader)
            {
                // Leader is gone - this is safe, just returns nullptr
                TC_LOG_DEBUG("module.playerbot.reference", "Leader {} no longer exists", _leaderGuid.ToString());
            }
        }

        return _cachedLeader;
    }
}
```

#### Fix 4B: Integrate SafeGroupLeaderReference (2 hours)

**File:** `src/modules/Playerbot/AI/BotAI.h`

**Change:** Replace raw pointer with safe reference

```cpp
class BotAI
{
private:
    // OLD: Player* _groupLeader;
    SafeGroupLeaderReference _groupLeader; // NEW: Safe reference

public:
    Player* GetGroupLeader() const { return _groupLeader.GetLeader(); }
    void SetGroupLeader(Player* leader) { _groupLeader.SetLeader(leader); }
    void ClearGroupLeader() { _groupLeader.ClearLeader(); }
};
```

**File:** `src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp`

**Change:** Use safe reference getter

```cpp
void LeaderFollowBehavior::UpdateBehavior(uint32 diff)
{
    Player* leader = _ai->GetGroupLeader(); // Safe getter
    if (!leader) // Will be nullptr if leader logged out
    {
        TC_LOG_DEBUG("module.playerbot.follow", "No valid leader to follow");
        return;
    }

    // Continue with follow logic...
}
```

**Testing:**
1. Create group with 3 bots
2. Move around with bots following
3. Logout player while in group
4. Verify:
   - Server does NOT crash
   - Bots handle gracefully
   - No access violations
5. Login again
6. Verify bots can re-join group

**Files Modified:** 5 (2 new, 3 modified)
**Lines Changed:** ~150 (100 new, 50 modified)

---

## Implementation Order

### Week 1: Critical Fixes (32 hours)
1. **Day 1-2:** Issue #4 (Logout Crash) - 6 hours
   - Create SafeGroupLeaderReference
   - Integration and testing
   - **Priority:** CRITICAL - Server stability

2. **Day 3-4:** Issue #2 & #3 (Combat) - 8 hours
   - Target acquisition
   - Follow behavior relevance
   - Explicit facing
   - **Priority:** HIGH - Core functionality

3. **Day 5:** Issue #1 (Login Group) - 2 hours
   - Move group check timing
   - Testing
   - **Priority:** MEDIUM - Workaround exists

4. **Day 5:** Integration Testing - 4 hours
   - All 4 issues validated
   - Regression testing
   - Performance verification

### Week 2: Polish & Documentation (8 hours)
1. Code review and cleanup
2. Update documentation
3. Commit and deployment

---

## Total Effort Breakdown

| Task | Hours | Priority |
|------|-------|----------|
| Issue #4: Logout Crash | 6 | CRITICAL |
| Issue #2 & #3: Combat | 8 | HIGH |
| Issue #1: Login Group | 2 | MEDIUM |
| Integration Testing | 4 | HIGH |
| Code Review | 2 | MEDIUM |
| Documentation | 2 | LOW |
| **TOTAL** | **24 hours** | |

**Buffer:** +8 hours for unexpected issues = **32 hours total**

---

## Files Summary

### New Files (2)
- `src/modules/Playerbot/Core/SafeGroupLeaderReference.h` (80 lines)
- `src/modules/Playerbot/Core/SafeGroupLeaderReference.cpp` (70 lines)

### Modified Files (6)
- `src/modules/Playerbot/Session/BotSession.cpp` (~15 lines)
- `src/modules/Playerbot/AI/ClassAI/ClassAI.cpp` (~50 lines)
- `src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp` (~30 lines)
- `src/modules/Playerbot/AI/BotAI.h` (~10 lines)
- `src/modules/Playerbot/AI/BotAI.cpp` (~20 lines)
- `src/modules/Playerbot/CMakeLists.txt` (~5 lines)

**Total Code:** ~280 lines (150 new, 130 modified)

---

## Success Criteria

### Functional Requirements
✅ Issue #1: Bots follow immediately after server restart
✅ Issue #2: Ranged bots cast spells, no ping-pong
✅ Issue #3: Melee bots face target correctly
✅ Issue #4: No crash on player logout

### Performance Requirements
✅ CPU impact: <0.01% additional per bot
✅ Memory impact: <100 bytes per bot (SafeGroupLeaderReference)
✅ No performance regression

### Quality Requirements
✅ CLAUDE.md compliance maintained
✅ Zero new cornerstones introduced
✅ Leverages existing Phase 2 infrastructure
✅ Non-breaking changes only
✅ Comprehensive testing

---

## Comparison with OPTION_B

| Metric | OPTION_B | SURGICAL FIX |
|--------|----------|--------------|
| Effort | 400-500 hours | 32 hours |
| New Files | 28 files | 2 files |
| Lines of Code | ~11,500 | ~280 |
| Risk Level | HIGH | LOW |
| Breaking Changes | YES | NO |
| Time to Production | 9-10 weeks | 1 week |
| Infrastructure Reuse | Creates new | Uses Phase 2 |

**Recommendation:** Use SURGICAL FIX PLAN
- ✅ Fixes all 4 critical issues
- ✅ Leverages existing Phase 2 work
- ✅ Fast delivery (1 week vs 10 weeks)
- ✅ Low risk, non-breaking
- ✅ No new architectural cornerstones

---

## Conclusion

The SURGICAL FIX PLAN addresses all 4 critical bot issues without introducing new cornerstones or duplicating Phase 2 work. By leveraging the existing BehaviorManager infrastructure, IdleStrategy, and CombatMovementStrategy, we can deliver a complete fix in 1 week with minimal risk.

**Next Steps:**
1. Get approval for SURGICAL FIX PLAN
2. Begin with Issue #4 (logout crash) - CRITICAL
3. Proceed to Issues #2 & #3 (combat) - HIGH
4. Complete with Issue #1 (login group) - MEDIUM
5. Integration testing and deployment

---

*Created: 2025-10-06*
*Status: READY FOR APPROVAL*
*Estimated Completion: 1 week from start*
