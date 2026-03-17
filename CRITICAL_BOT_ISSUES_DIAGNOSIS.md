# CRITICAL BOT BEHAVIOR ISSUES - ROOT CAUSE ANALYSIS & SOLUTIONS

## Investigation Date: 2025-10-06
## Investigator: Claude AI

---

## ISSUE 1: Bot Already in Group When Player Logs In

### SYMPTOMS
- Bot is already in a group when player logs in (after server restart)
- Bot does NOT follow the leader initially
- Bot DOES attack when group leader attacks
- Bot behavior is not updated on player login

### ROOT CAUSE DIAGNOSIS
**Location:** `BotSession.cpp:946-960` and `BotAI.cpp:116-136`

The problem has TWO root causes:

1. **BotSession Login Handler Missing Group Check:**
   - In `HandleBotPlayerLogin()` at lines 946-960, there IS a check for existing group and calls `OnGroupJoined()`
   - However, this is called BEFORE the bot is fully in the world (before line 932)
   - The bot is not yet `IsInWorld()` when `OnGroupJoined()` is called

2. **BotAI First Update Logic Race Condition:**
   - Lines 116-136 in `BotAI::UpdateAI()` use a static `_initializedBots` set
   - This only runs ONCE per bot GUID when bot `IsInWorld()`
   - But `OnGroupJoined()` was already called BEFORE bot was in world
   - Strategy activation in first update (line 127-128) happens AFTER `OnGroupJoined()`
   - The follow strategy gets activated but its `OnActivate()` is never called

### WHY IT PERSISTS
Previous fixes attempted to call `OnGroupJoined()` at various points but didn't address the timing issue. The bot needs to be fully in the world before group strategies can be properly activated.

### RECOMMENDED FIX
```cpp
// In BotSession::HandleBotPlayerLogin(), MOVE the group check to AFTER line 932:
// Line 932: TC_LOG_INFO("module.playerbot.session", "Bot player {} successfully added to world", pCurrChar->GetName());

// Move lines 946-960 to here (after bot is in world):
if (Player* player = GetPlayer())
{
    if (player->IsInWorld()) // NEW: Verify bot is actually in world
    {
        Group* group = player->GetGroup();
        if (group)
        {
            TC_LOG_INFO("module.playerbot.session", "🔄 Bot {} is already in group at login - activating strategies", player->GetName());
            if (BotAI* ai = GetAI())
            {
                ai->OnGroupJoined(group);
            }
        }
    }
}
```

---

## ISSUE 2: Ranged DPS Combat Not Triggering

### SYMPTOMS
- Leader attacks enemy
- Ranged DPS bot (Mage/Hunter/Warlock) runs TO the target
- Bot hits once (auto-attack)
- Bot runs back to "safe position"
- Bot does NOT cast any spells
- Bot does NOT use class abilities

### ROOT CAUSE DIAGNOSIS
**Locations:**
- `BotAI.cpp:217-223` - Combat delegation
- `ClassAI.cpp:62-144` - OnCombatUpdate implementation
- `ClassAI.cpp:98-129` - Combat movement logic

The problem has THREE root causes:

1. **OnCombatUpdate IS Being Called But Target Is NULL:**
   - Line 75-79 in `ClassAI::OnCombatUpdate()` shows diagnostic logging
   - `_currentCombatTarget` is often NULL even though bot is in combat
   - When target is NULL, only `UpdateBuffs()` is called (line 142)

2. **Movement Logic Fighting With Follow Behavior:**
   - Lines 106-129 implement combat movement to optimal range
   - But `LeaderFollowBehavior` has relevance of 10.0f during combat (line 168)
   - Follow behavior is still trying to follow leader WHILE combat is trying to position for attack
   - This creates the "run to target, run back" ping-pong effect

3. **Target Not Being Set Properly:**
   - `OnCombatStart()` at `BotAI.cpp:445-466` tries to find target
   - Uses `_objectCache.GetTarget()` first which may be NULL
   - Falls back to `GetVictim()` which may also be NULL for ranged bots
   - Ranged bots don't have a "victim" until they actually hit something

### WHY IT PERSISTS
The Phase 2 refactoring separated combat from movement but didn't properly handle target acquisition for ranged classes. The follow behavior still has non-zero relevance during combat, causing movement conflicts.

### RECOMMENDED FIX
```cpp
// In ClassAI::OnCombatUpdate(), fix target acquisition:
void ClassAI::OnCombatUpdate(uint32 diff)
{
    if (!GetBot() || !GetBot()->IsAlive())
        return;

    // FIX: Ensure we have a target before doing anything
    if (!_currentCombatTarget)
    {
        // Try multiple sources to find a target
        _currentCombatTarget = GetBot()->GetVictim();

        if (!_currentCombatTarget && GetBot()->GetGroup())
        {
            // Find what the group leader is attacking
            if (Player* leader = /* get leader from group */)
            {
                _currentCombatTarget = leader->GetVictim();
            }
        }

        if (!_currentCombatTarget)
        {
            // Find nearest enemy
            _currentCombatTarget = GetNearestEnemy(40.0f);
        }
    }

    // Now continue with combat logic...
}

// In LeaderFollowBehavior::GetRelevance(), return 0 during combat:
float LeaderFollowBehavior::GetRelevance(BotAI* ai) const
{
    // ...
    // ZERO relevance during combat - let ClassAI handle ALL combat movement
    if (bot->IsInCombat())
        return 0.0f;  // Changed from 10.0f to 0.0f
    // ...
}
```

---

## ISSUE 3: Melee Bot Facing Wrong Direction

### SYMPTOMS
- Melee bot attacks group leader's target
- Bot FACES the group leader (not the target)
- Bot does NOT attack because not facing target
- Bot positioning is wrong

### ROOT CAUSE DIAGNOSIS
**Location:** `LeaderFollowBehavior.cpp` and combat movement interaction

The problem is caused by:

1. **Follow Behavior Still Active During Combat:**
   - `LeaderFollowBehavior::GetRelevance()` returns 10.0f during combat (line 168)
   - This means follow behavior is still trying to face/follow the leader
   - Combat wants bot to face target, follow wants bot to face leader
   - Follow wins because it updates every frame

2. **SetFacingTo Not Being Called For Combat Target:**
   - Combat movement uses `ChaseTarget()` but doesn't explicitly set facing
   - Follow behavior constantly resets facing to leader

### WHY IT PERSISTS
The Phase 2 refactoring didn't fully disable follow behavior during combat. The 10.0f relevance means it's still partially active.

### RECOMMENDED FIX
```cpp
// In LeaderFollowBehavior::GetRelevance():
if (bot->IsInCombat())
    return 0.0f;  // COMPLETELY disable during combat

// In ClassAI combat movement (line 111):
if (currentDistance > optimalRange + rangeTolerance)
{
    BotMovementUtil::ChaseTarget(GetBot(), _currentCombatTarget, optimalRange);
    GetBot()->SetFacingToObject(_currentCombatTarget); // ADD THIS
}
```

---

## ISSUE 4: Server Crash on Logout While in Group

### SYMPTOMS
- Player is in a group with bots
- Player logs out
- Server crashes

### ROOT CAUSE DIAGNOSIS
**Location:** Group cleanup and bot AI access

The crash is likely caused by:

1. **Dangling Pointer to Leader:**
   - When player logs out, their Player object is destroyed
   - Bots still have pointers to the leader in their AI
   - Next update cycle tries to access destroyed leader object
   - Access violation causes crash

2. **No Logout Notification to Bots:**
   - Bots are not notified when group leader logs out
   - They continue trying to follow/assist a deleted player

### WHY IT PERSISTS
The logout flow doesn't properly notify bots before the player object is destroyed. There's a race condition between player cleanup and bot AI updates.

### RECOMMENDED FIX
```cpp
// Add a pre-logout hook to notify bots:
void NotifyBotsOfLogout(Player* player)
{
    if (Group* group = player->GetGroup())
    {
        for (GroupReference const& itr : group->GetMembers())
        {
            if (Player* member = itr.GetSource())
            {
                if (member != player && member->IsBot())
                {
                    if (BotAI* ai = member->GetBotAI())
                    {
                        // Clear follow target if following this player
                        ai->OnLeaderLogout(player);
                    }
                }
            }
        }
    }
}

// In BotAI, add:
void OnLeaderLogout(Player* leader)
{
    // Clear any cached pointers to this leader
    if (_objectCache.GetGroupLeader() == leader)
    {
        _objectCache.SetGroupLeader(nullptr);
        _objectCache.SetFollowTarget(nullptr);
    }

    // Deactivate follow strategy
    DeactivateStrategy("follow");
}
```

---

## ARCHITECTURAL RECOMMENDATIONS

### 1. Refactor Login/Group Procedures

**Problem:** The current login flow has multiple race conditions where group state is checked at different points with different world states.

**Solution:** Create a unified bot initialization state machine:
```cpp
enum class BotInitState {
    CREATED,
    LOADING,
    IN_WORLD,
    GROUP_CHECK,
    STRATEGIES_INITIALIZED,
    READY
};

class BotInitializer {
    void ProcessInitialization() {
        switch(_state) {
            case IN_WORLD:
                if (CheckExistingGroup()) {
                    _state = GROUP_CHECK;
                }
                break;
            case GROUP_CHECK:
                ActivateGroupStrategies();
                _state = STRATEGIES_INITIALIZED;
                break;
            // etc...
        }
    }
};
```

### 2. Clear Combat/Movement Separation

**Problem:** Follow and combat behaviors conflict during combat.

**Solution:** Implement behavior priority system with mutual exclusion:
```cpp
enum class BehaviorPriority {
    DEAD = 0,
    COMBAT = 100,      // Highest - overrides everything
    FLEEING = 90,
    FOLLOW = 50,       // Mid - only when not in combat
    IDLE = 10          // Lowest
};

class BehaviorManager {
    Strategy* GetActiveBehavior() {
        // Return ONLY the highest priority active behavior
        // Never run multiple movement behaviors simultaneously
    }
};
```

### 3. Proper Leader Reference Management

**Problem:** Bots hold raw pointers to leaders that can become invalid.

**Solution:** Use ObjectGuid and validate on every access:
```cpp
class SafeLeaderReference {
    ObjectGuid _leaderGuid;

    Player* GetLeader() {
        if (_leaderGuid.IsEmpty()) return nullptr;

        // Always re-fetch from ObjectAccessor
        // Never cache Player* pointers
        return ObjectAccessor::FindPlayer(_leaderGuid);
    }
};
```

### 4. Event-Driven State Updates

**Problem:** Bots poll for state changes, missing transitions.

**Solution:** Use observer pattern for group events:
```cpp
class GroupEventObserver {
    virtual void OnMemberJoined(Player* member) = 0;
    virtual void OnMemberLeft(Player* member) = 0;
    virtual void OnLeaderChanged(Player* newLeader) = 0;
    virtual void OnGroupDisbanded() = 0;
};

// Register bots as observers to their group
// Group notifies all observers of changes
```

---

## TESTING RECOMMENDATIONS

1. **Test Server Restart Scenario:**
   - Create group with bots
   - Restart server
   - Login and verify bots follow immediately

2. **Test Combat Transitions:**
   - Have ranged bot in group
   - Attack target
   - Verify bot casts spells, doesn't ping-pong

3. **Test Leader Logout:**
   - Create group with bots
   - Logout group leader
   - Verify no crash, bots handle gracefully

4. **Test Facing During Combat:**
   - Have melee bot in group
   - Attack target
   - Verify bot faces target, not leader

---

## PRIORITY OF FIXES

1. **CRITICAL:** Fix logout crash (Issue 4) - Server stability
2. **HIGH:** Fix ranged combat (Issue 2) - Core functionality broken
3. **HIGH:** Fix melee facing (Issue 3) - Core functionality broken
4. **MEDIUM:** Fix login with group (Issue 1) - Workaround exists (re-invite)

---

## END OF DIAGNOSIS REPORT