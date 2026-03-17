# Task 1.7: State Machine Integration with BotAI & BotSession

## Summary
This task integrates the BotInitStateMachine into BotAI and BotSession to fix Issue #1 where bots already in groups at login wouldn't follow their leader.

## Root Cause Analysis
The bug occurred because `OnGroupJoined()` was called BEFORE the bot was fully in the world (`IsInWorld()` returned false), causing strategy activation to fail silently. The state machine enforces proper initialization ordering.

## Files Modified

### 1. BotAI.h - Add State Machine and Safe References
**Location:** `c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/BotAI.h`

**Key Changes:**
```cpp
// Add includes at top:
#include "Core/StateMachine/BotInitStateMachine.h"
#include "Core/References/SafeObjectReference.h"

// Add to private members:
std::unique_ptr<StateMachine::BotInitStateMachine> m_initStateMachine;
References::SafePlayerReference m_groupLeader;

// Add public methods:
bool IsFullyInitialized() const {
    return m_initStateMachine && m_initStateMachine->IsReady();
}

float GetInitProgress() const {
    return m_initStateMachine ? m_initStateMachine->GetProgress() : 0.0f;
}

void SetGroupLeader(Player* leader) {
    m_groupLeader.Set(leader);
}

Player* GetGroupLeader() const {
    return m_groupLeader.Get();
}

void ActivateBaseStrategies();
```

### 2. BotAI.cpp - Integrate State Machine into Update Loop
**Location:** `c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/BotAI.cpp`

**Constructor Changes:**
```cpp
BotAI::BotAI(Player* bot)
    : _bot(bot)
    , m_initStateMachine(nullptr)  // Will be created in first UpdateAI
    , m_groupLeader()              // Safe reference, initially empty
{
    // ... existing initialization ...

    // DON'T check for group here - let state machine handle it
    // REMOVED: if (_bot->GetGroup()) { OnGroupJoined(_bot->GetGroup()); }
}
```

**UpdateAI Changes:**
```cpp
void BotAI::UpdateAI(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // PHASE 1: Initialize state machine on first update
    if (!m_initStateMachine && _bot->IsInWorld())
    {
        m_initStateMachine = std::make_unique<StateMachine::BotInitStateMachine>(_bot);
        m_initStateMachine->Start();

        TC_LOG_INFO("module.playerbot",
            "BotInitStateMachine created and started for bot {}",
            _bot->GetName());
    }

    // PHASE 2: Update state machine
    if (m_initStateMachine && !m_initStateMachine->IsReady())
    {
        m_initStateMachine->Update(diff);

        // Don't process AI logic until initialization complete
        if (!m_initStateMachine->IsReady())
        {
            return; // Skip rest of update
        }

        // State machine just became ready!
        TC_LOG_INFO("module.playerbot",
            "Bot {} initialization complete - now ready for AI updates",
            _bot->GetName());

        // Check if bot was in group at login
        if (m_initStateMachine->WasInGroupAtLogin())
        {
            if (Group* group = _bot->GetGroup())
            {
                OnGroupJoined(group);
            }
        }
        else
        {
            ActivateStrategy("idle");
        }
    }

    // PHASE 3: Normal AI updates (existing code continues here)
    // ... rest of UpdateAI ...
}
```

**New Method:**
```cpp
void BotAI::ActivateBaseStrategies()
{
    // Called by BotInitStateMachine when ready
    ActivateStrategy("idle");

    TC_LOG_DEBUG("module.playerbot",
        "Base strategies activated for bot {}",
        _bot ? _bot->GetName() : "NULL");
}
```

**Modified OnGroupJoined:**
```cpp
void BotAI::OnGroupJoined(Group* group)
{
    if (!group || !_bot) return;

    // Set group leader using safe reference
    ObjectGuid leaderGuid = group->GetLeaderGUID();
    if (Player* leader = ObjectAccessor::FindPlayer(leaderGuid))
    {
        SetGroupLeader(leader);

        TC_LOG_INFO("module.playerbot",
            "Bot {} joined group, leader set to {}",
            _bot->GetName(), leader->GetName());
    }

    // Activate follow strategy
    ActivateStrategy("follow");

    // ... rest of existing OnGroupJoined logic ...
}
```

### 3. BotSession.cpp - Remove Problematic Group Check
**Location:** `c:/TrinityBots/TrinityCore/src/modules/Playerbot/Session/BotSession.cpp`
**Lines:** 944-960

**REMOVE THIS ENTIRE BLOCK:**
```cpp
// DELETE THESE LINES (944-960):
if (Player* player = GetPlayer())
{
    Group* group = player->GetGroup();
    TC_LOG_ERROR("module.playerbot.session", "Bot {} login group check: player={}, group={}",
                player->GetName(), (void*)player, (void*)group);

    if (group)
    {
        TC_LOG_INFO("module.playerbot.session", "Bot {} is already in group at login - activating strategies", player->GetName());
        if (BotAI* ai = GetAI())
        {
            TC_LOG_ERROR("module.playerbot.session", "About to call OnGroupJoined with group={}", (void*)group);
            ai->OnGroupJoined(group);  // TOO EARLY!
        }
    }
}
```

**REPLACE WITH:**
```cpp
// NEW CODE - State machine handles this properly now:
// The BotInitStateMachine will check for group membership
// at the correct time (in CHECKING_GROUP state, AFTER IsInWorld())
// No explicit call needed here - state machine handles it automatically

TC_LOG_INFO("module.playerbot.session",
    "Bot {} login complete - state machine will handle initialization",
    GetPlayer()->GetName());
```

### 4. LeaderFollowBehavior.cpp - Use Safe References
**Location:** `c:/TrinityBots/TrinityCore/src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp`

**Modified UpdateBehavior:**
```cpp
void LeaderFollowBehavior::UpdateBehavior(uint32 diff, BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();

    // NEW: Use safe reference from BotAI
    Player* leader = ai->GetGroupLeader();

    if (!leader)
    {
        // Leader logged out or no longer exists - safe!
        TC_LOG_DEBUG("module.playerbot.follow",
            "Bot {} has no valid leader to follow",
            bot->GetName());

        ClearFollowTarget();
        return;
    }

    // Continue with existing follow logic using 'leader'
    SetFollowTarget(leader);

    // ... rest of existing UpdateBehavior code ...
}
```

## Integration Flow

```
Player Login
    ↓
BotSession::HandleBotPlayerLogin()
    ↓
AddToWorld()
    ↓
Bot is now IsInWorld() == true
    ↓
[No explicit group check here anymore]
    ↓
BotAI::UpdateAI() first call
    ↓
Create BotInitStateMachine
    ↓
Start() → CREATED → LOADING_CHARACTER
    ↓
UpdateAI() subsequent calls
    ↓
State machine progresses:
    LOADING_CHARACTER → IN_WORLD → CHECKING_GROUP
    ↓
In CHECKING_GROUP state:
    - Bot is guaranteed IsInWorld()
    - Safe to call GetGroup()
    - If in group, cache leader info
    ↓
ACTIVATING_STRATEGIES state:
    - Call OnGroupJoined() if in group
    - Follow strategy activates
    - OnActivate() runs successfully
    ↓
READY state:
    - Bot fully initialized
    - Normal AI updates begin
```

## Testing Checklist

1. **Solo Bot Login**
   - [ ] Bot logs in without group
   - [ ] State machine initializes properly
   - [ ] Idle strategy activates
   - [ ] No crashes or errors

2. **Grouped Bot Login**
   - [ ] Bot logs in already in group
   - [ ] State machine detects group
   - [ ] Follow strategy activates
   - [ ] Bot follows leader correctly

3. **Leader Logout**
   - [ ] Leader logs out while bot is following
   - [ ] Safe reference returns nullptr
   - [ ] Bot stops following gracefully
   - [ ] No crash on leader deletion

4. **Performance**
   - [ ] State machine overhead < 1ms
   - [ ] Safe reference cache hit rate > 90%
   - [ ] No memory leaks
   - [ ] No thread deadlocks

## Expected Log Output

### Successful Grouped Bot Login:
```
[INFO] BotInitStateMachine created and started for bot TestBot
[DEBUG] State transition: CREATED → LOADING_CHARACTER
[DEBUG] State transition: LOADING_CHARACTER → IN_WORLD
[DEBUG] State transition: IN_WORLD → CHECKING_GROUP
[INFO] Bot TestBot is in group with leader TestLeader
[DEBUG] State transition: CHECKING_GROUP → ACTIVATING_STRATEGIES
[INFO] Bot TestBot joined group, leader set to TestLeader
[INFO] Bot TestBot activated group strategies - follow and group_combat
[DEBUG] State transition: ACTIVATING_STRATEGIES → READY
[INFO] Bot TestBot initialization complete - now ready for AI updates
```

## Files Affected

1. `src/modules/Playerbot/AI/BotAI.h` - Added state machine member and methods
2. `src/modules/Playerbot/AI/BotAI.cpp` - Integrated state machine into UpdateAI
3. `src/modules/Playerbot/Session/BotSession.cpp` - Removed early group check
4. `src/modules/Playerbot/Movement/LeaderFollowBehavior.cpp` - Use safe references
5. `src/modules/Playerbot/Core/Events/BotEventTypes.h` - Already exists (skeleton)
6. `src/modules/Playerbot/CMakeLists.txt` - Already includes all required files

## Build Instructions

1. The CMakeLists.txt already includes:
   - `Core/StateMachine/BotInitStateMachine.cpp/h`
   - `Core/References/SafeObjectReference.cpp/h`
   - `Core/Events/BotEventTypes.h`

2. Include directories are already set:
   - `${CMAKE_CURRENT_SOURCE_DIR}/Core`
   - `${CMAKE_CURRENT_SOURCE_DIR}/Core/References`
   - `${CMAKE_CURRENT_SOURCE_DIR}/Core/StateMachine`

3. Rebuild:
   ```bash
   cd build
   cmake --build . --target playerbot
   ```

## Conclusion

This integration fixes Issue #1 by ensuring bot initialization happens in the correct order using a state machine. The bot is guaranteed to be fully in the world before any group operations occur. Additionally, safe references prevent crashes when group leaders log out (Issue #4).

The solution is non-intrusive, maintaining backward compatibility while adding robust initialization sequencing. The state machine pattern can be extended for other initialization requirements in the future.