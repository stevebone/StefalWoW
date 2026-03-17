# COMPREHENSIVE ANALYSIS: Bot Death Handling and Map::SendObjectUpdates Crash

## Executive Summary

**Root Cause Identified**: TrinityCore's `Object::RemoveFromWorld()` sets `IsInWorld()` to false but **DOES NOT** remove the object from `Map::_updateObjects` queue, causing `Map::SendObjectUpdates()` to crash when it encounters objects that are no longer in world.

**Impact**: Server crashes with `ASSERT(obj->IsInWorld())` failure at `Map.cpp:1940`

**Solution Required**: Either fix TrinityCore to call `Map::RemoveUpdateObject()` during `RemoveFromWorld()`, OR implement a workaround in playerbot module to prevent crashes

---

## 1. Crash Analysis

### Crash Location
```
File: C:\TrinityBots\TrinityCore\src\server\game\Maps\Map.cpp
Line: 1940
Function: Map::SendObjectUpdates()
Error: ASSERT(obj->IsInWorld()) fails
Exception: C0000005 ACCESS_VIOLATION
```

### Crash Context
```cpp
void Map::SendObjectUpdates()
{
    UpdateDataMapType update_players;

    while (!_updateObjects.empty())
    {
        Object* obj = *_updateObjects.begin();  // Line 1939
        ASSERT(obj->IsInWorld());                // Line 1940 - CRASH HERE!
        _updateObjects.erase(_updateObjects.begin());
        obj->BuildUpdate(update_players);
    }
    // ...
}
```

### Crash State (from crash log)
```
Local Object* obj = NULL  // ← DANGLING POINTER!
Exception code: C0000005 ACCESS_VIOLATION
Fault address: 00007FF63A88E401 (Map::SendObjectUpdates+E1)
```

---

## 2. The TOCTOU Race Condition

### Timeline of the Bug

**Thread 1: Map Update Thread (Main Thread)**
```
T1: Bot takes damage in combat
T2: Map::Update() adds bot to _updateObjects queue via Map::AddUpdateObject(bot)
T3: Combat continues, bot health reaches 0
T4: Bot dies, death event triggers
```

**Thread 2: Bot Lifecycle Thread (Worker Thread)**
```
T5: Death event handler calls bot->RemoveFromWorld()
T6: Player::RemoveFromWorld() → Unit::RemoveFromWorld() → WorldObject::RemoveFromWorld() → Object::RemoveFromWorld()
T7: Object::RemoveFromWorld() sets m_inWorld = false
T8: ❌ BUG: Does NOT call Map::RemoveUpdateObject(this)!
T9: Bot object still in Map::_updateObjects queue with IsInWorld() == false
```

**Thread 1: Map Update Thread Returns**
```
T10: Map::SendObjectUpdates() begins processing _updateObjects
T11: obj = *_updateObjects.begin() → gets bot pointer
T12: ASSERT(obj->IsInWorld()) → IsInWorld() returns FALSE
T13: 💥 CRASH! Assertion failure
```

### The Race Window

The TOCTOU (Time-Of-Check-To-Time-Of-Use) window is:
```
CHECK: Bot added to _updateObjects (IsInWorld() == true at this moment)
  ↓
  [RACE WINDOW - bot can be removed from world by another thread]
  ↓
USE: Map::SendObjectUpdates() processes the queue (IsInWorld() == false now)
```

---

## 3. Root Cause: Missing Map::RemoveUpdateObject()

### What SHOULD Happen

When an object is removed from world, TrinityCore should:
1. Set `m_inWorld = false` (currently done)
2. Call `Map::RemoveUpdateObject(this)` to remove from update queue (❌ NOT DONE!)
3. Clear update masks
4. Clean up references

### What ACTUALLY Happens

```cpp
// Object.cpp:124
void Object::RemoveFromWorld()
{
    if (!m_inWorld)
        return;

    m_inWorld = false;  // ← Sets IsInWorld() to false

    // if we remove from world then sending changes not required
    ClearUpdateMask(true);

    m_scriptRef = nullptr;

    // ❌ BUG: Missing call to Map::RemoveUpdateObject(this)!
}
```

### Comparison with AddToWorld()

TrinityCore **DOES** add objects to the update queue when adding to world:

```cpp
// Map.cpp (hypothetical AddToWorld implementation)
void Map::AddToWorld(WorldObject* obj)
{
    // Add to map's object storage
    _objects.insert(obj);

    // Add to update queue
    AddUpdateObject(obj);  // ← This IS called!

    obj->SetMap(this);
}
```

But `Object::RemoveFromWorld()` **DOES NOT** have the corresponding cleanup:

```cpp
void Object::RemoveFromWorld()
{
    m_inWorld = false;
    ClearUpdateMask(true);

    // ❌ Missing: Map::RemoveUpdateObject(this)
}
```

---

## 4. Bot Death Handling Code Paths

### Path 1: Normal Bot Logout (BotWorldEntry.cpp:737-747)

```cpp
// src/modules/Playerbot/Lifecycle/BotWorldEntry.cpp:737
if (_player)
{
    if (_player->IsInWorld())
    {
        _player->RemoveFromWorld();  // ← TRIGGERS THE BUG
    }

    // Clean logout
    _session->LogoutPlayer(false);
    _player = nullptr;
}
```

**Analysis**: This path is relatively safe because it's during controlled shutdown. However, if the bot was in combat and got added to `_updateObjects` just before logout, the bug can still trigger.

### Path 2: Bot Session Cleanup (BotSessionEnhanced.cpp:236-244)

```cpp
// src/modules/Playerbot/Session/BotSessionEnhanced.cpp:236
// Clean up player if created
if (GetPlayer())
{
    if (GetPlayer()->IsInWorld())
    {
        GetPlayer()->RemoveFromWorld();  // ← TRIGGERS THE BUG
    }
    SetPlayer(nullptr);
}
```

**Analysis**: Similar to Path 1, but can happen during error conditions or forced cleanup. Higher risk because it might happen during active gameplay.

### Path 3: Bot Death (via TrinityCore Death System)

**Question**: Does bot death call `RemoveFromWorld()`?

**Answer**: NO! Death does NOT call `RemoveFromWorld()`. Instead:
1. Bot dies → `SetDeathState(DEAD)` is called
2. Bot becomes a corpse → `SetDeathState(CORPSE)`
3. Bot releases spirit → becomes ghost (still in world!)
4. Bot resurrects → becomes alive again

Bots stay in world throughout the entire death-resurrection cycle. `RemoveFromWorld()` is only called when the bot **logs out** or is **forcibly removed**.

**Implication**: The crash is NOT directly caused by normal bot deaths, but by **bot logouts/disconnects while they have pending updates in the queue**.

---

## 5. Existing Workaround: Resurrection Throttling

The `DeathRecoveryManager` already implements resurrection throttling (lines 1016-1043, 1182-1208) as a **partial workaround** for this crash:

```cpp
// DeathRecoveryManager.cpp:1016
// RESURRECTION THROTTLING: Workaround for TrinityCore Map::SendObjectUpdates race condition
// Check if we need to throttle (delay between resurrections)
auto now = std::chrono::steady_clock::now();
auto timeSinceLastRes = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastResurrectionTime).count();

if (timeSinceLastRes < m_resurrectionThrottleMs)
{
    uint32 waitTime = m_resurrectionThrottleMs - static_cast<uint32>(timeSinceLastRes);
    TC_LOG_DEBUG("playerbot.death", "🕒 Bot {} resurrection THROTTLED - waiting {}ms to reduce Map::SendObjectUpdates crashes",
        m_bot->GetName(), waitTime);
    return false; // Will retry in next Update()
}

// Check batch limit (prevent too many simultaneous resurrections)
if (m_currentBatchCount >= m_resurrectionBatchSize)
{
    TC_LOG_DEBUG("playerbot.death", "⏸️  Bot {} resurrection BATCHED - batch limit reached ({}/{})",
        m_bot->GetName(), m_currentBatchCount, m_resurrectionBatchSize);
    return false;
}
```

**Configuration**:
```cpp
m_resurrectionThrottleMs = sPlayerbotConfig->GetUInt("Playerbot.DeathRecovery.ResurrectionThrottle", 100);
m_resurrectionBatchSize = sPlayerbotConfig->GetUInt("Playerbot.DeathRecovery.ResurrectionBatchSize", 10);
```

**Why This Helps**: By spacing out resurrections, it reduces the likelihood of multiple bots triggering updates simultaneously, which reduces the window where bots might be removed from world while in the update queue.

**Why This Is NOT Enough**: This throttling only applies to **resurrections**, not to **logouts/disconnects**, which are the actual triggers for `RemoveFromWorld()`.

---

## 6. Why Normal Players Don't Crash

**Key Insight**: Normal (human) players rarely disconnect during combat, and when they do, it's usually one player at a time. TrinityCore's `RemoveFromWorld()` bug exists for ALL players, but it rarely manifests because:

1. **Low Frequency**: Human players don't mass-disconnect during combat
2. **Logout Timer**: Normal logout has a 20-second sitting timer, giving time for updates to finish
3. **Combat Logout Prevention**: Players in combat can't log out normally
4. **Single Disconnect**: When a player force-quits, it's usually just one player, not 50-500 at once

**With Bots**: The playerbot module introduces:
- **Mass Disconnects**: When server shuts down, 500 bots disconnect simultaneously
- **Rapid Cycling**: Bots can be spawned/despawned rapidly for testing
- **Combat Disconnects**: Bots can be forcibly removed even during combat
- **High Update Frequency**: 500 bots generate way more `AddUpdateObject()` calls than 50 human players

This dramatically increases the probability that `RemoveFromWorld()` is called while objects are in the `_updateObjects` queue.

---

## 7. Solutions

### Solution A: Fix TrinityCore (IDEAL but violates no-core-modifications rule)

**File**: `src/server/game/Entities/Object/Object.cpp`
**Line**: 124 (in `Object::RemoveFromWorld()`)

```cpp
void Object::RemoveFromWorld()
{
    if (!m_inWorld)
        return;

    // ✅ FIX: Remove from map's update queue BEFORE setting m_inWorld = false
    if (Map* map = GetMap())
        map->RemoveUpdateObject(this);

    m_inWorld = false;

    // if we remove from world then sending changes not required
    ClearUpdateMask(true);

    m_scriptRef = nullptr;
}
```

**Pros**:
- Fixes the root cause permanently
- Benefits all code, not just playerbots
- Clean and simple

**Cons**:
- ❌ Violates project rule: "AVOID modify core files"
- Would need to be maintained across TrinityCore updates

---

### Solution B: Playerbot Hook - Override RemoveFromWorld (RECOMMENDED)

Create a custom `PlayerScript` hook that ensures `Map::RemoveUpdateObject()` is called before bots are removed from world.

**File**: `src/modules/Playerbot/scripts/PlayerbotCleanupScript.cpp` (NEW FILE)

```cpp
class PlayerBotCleanupScript : public PlayerScript
{
public:
    PlayerBotCleanupScript() : PlayerScript("PlayerBotCleanupScript") {}

    // Hook that runs BEFORE Player::RemoveFromWorld()
    void OnBeforeRemoveFromWorld(Player* player) override
    {
        // Only for bots
        if (!player->GetSession() || !dynamic_cast<BotSession*>(player->GetSession()))
            return;

        // Ensure bot is removed from map's update queue
        if (Map* map = player->GetMap())
        {
            map->RemoveUpdateObject(player);
            TC_LOG_DEBUG("playerbot.lifecycle",
                "PlayerBotCleanupScript: Removed bot {} from map update queue before RemoveFromWorld()",
                player->GetName());
        }
    }
};

// Register the script
void AddSC_playerbot_cleanup_scripts()
{
    new PlayerBotCleanupScript();
}
```

**Pros**:
- ✅ No TrinityCore modifications
- Only affects bots, not normal players
- Clean separation of concerns

**Cons**:
- ❌ Requires TrinityCore to have `OnBeforeRemoveFromWorld()` hook (may not exist)
- Need to verify hook exists or add it

---

### Solution C: Wrapper Around RemoveFromWorld (SAFEST MODULE-ONLY APPROACH)

Instead of calling `player->RemoveFromWorld()` directly in playerbot code, create a wrapper function that ensures proper cleanup.

**File**: `src/modules/Playerbot/Lifecycle/BotCleanupHelper.h` (NEW FILE)

```cpp
#pragma once
#include "Player.h"
#include "Map.h"

namespace Playerbot
{

class BotCleanupHelper
{
public:
    // Safe wrapper that ensures bot is removed from map's update queue
    static void SafeRemoveFromWorld(Player* bot)
    {
        if (!bot)
            return;

        // CRITICAL FIX: Remove from map's update queue FIRST
        // This prevents Map::SendObjectUpdates() from accessing the bot after IsInWorld() == false
        if (Map* map = bot->GetMap())
        {
            map->RemoveUpdateObject(bot);
            TC_LOG_DEBUG("playerbot.lifecycle",
                "BotCleanupHelper: Removed bot {} from map update queue",
                bot->GetName());
        }

        // Now safe to call RemoveFromWorld()
        bot->RemoveFromWorld();

        TC_LOG_DEBUG("playerbot.lifecycle",
            "BotCleanupHelper: Bot {} safely removed from world",
            bot->GetName());
    }
};

} // namespace Playerbot
```

**Usage** - Replace all `player->RemoveFromWorld()` calls in playerbot code:

```cpp
// BEFORE (BUGGY):
if (_player->IsInWorld())
{
    _player->RemoveFromWorld();
}

// AFTER (SAFE):
if (_player->IsInWorld())
{
    Playerbot::BotCleanupHelper::SafeRemoveFromWorld(_player);
}
```

**Files to Update**:
1. `src/modules/Playerbot/Lifecycle/BotWorldEntry.cpp:742`
2. `src/modules/Playerbot/Session/BotSessionEnhanced.cpp:241`

**Pros**:
- ✅ 100% module-only solution
- ✅ No TrinityCore modifications
- ✅ Clean encapsulation
- ✅ Easy to test and verify

**Cons**:
- Need to audit all `RemoveFromWorld()` calls in playerbot code
- Developers must remember to use the wrapper

---

### Solution D: Defensive Check in Map::SendObjectUpdates (MINIMAL CORE CHANGE)

If we're allowed ONE minimal core change, we could add a defensive null check:

**File**: `src/server/game/Maps/Map.cpp:1933`

```cpp
void Map::SendObjectUpdates()
{
    UpdateDataMapType update_players;

    while (!_updateObjects.empty())
    {
        Object* obj = *_updateObjects.begin();
        _updateObjects.erase(_updateObjects.begin());

        // DEFENSIVE FIX: Skip objects that are no longer in world
        // This can happen if RemoveFromWorld() doesn't call RemoveUpdateObject()
        if (!obj->IsInWorld())
        {
            TC_LOG_WARN("maps", "Map::SendObjectUpdates: Skipping object {} (TypeId: {}) - not in world",
                obj->GetGUID().ToString(), obj->GetTypeId());
            continue;
        }

        obj->BuildUpdate(update_players);
    }
    // ...
}
```

**Pros**:
- ✅ Minimal change (just replace ASSERT with conditional)
- ✅ Fixes the crash for all cases
- ✅ Easy to maintain

**Cons**:
- ❌ Still modifies TrinityCore core file
- Hides the underlying bug rather than fixing root cause

---

## 8. Recommended Solution

**RECOMMENDED**: **Solution C** (Wrapper Around RemoveFromWorld)

**Rationale**:
1. ✅ 100% compliant with "no core modifications" rule
2. ✅ Addresses the specific bot removal use case
3. ✅ Simple to implement and test
4. ✅ Doesn't affect normal player code paths
5. ✅ Can be done immediately without waiting for TrinityCore changes

**Implementation Steps**:
1. Create `BotCleanupHelper.h` with `SafeRemoveFromWorld()` function
2. Update `BotWorldEntry.cpp:742` to use wrapper
3. Update `BotSessionEnhanced.cpp:241` to use wrapper
4. Audit all other `RemoveFromWorld()` calls in playerbot module
5. Add comprehensive logging to track cleanup
6. Test with mass bot spawns/despawns during combat

---

## 9. Testing Plan

### Test Case 1: Mass Bot Logout During Combat
```
1. Spawn 500 bots
2. Put all bots in combat (attack training dummies)
3. Wait until bots are actively fighting (updates queued)
4. Force-disconnect all bots simultaneously (.bot remove all)
5. Verify: No crashes
```

### Test Case 2: Rapid Bot Spawn/Despawn Cycling
```
1. Spawn 100 bots
2. Immediately despawn them (.bot remove all)
3. Repeat 100 times rapidly
4. Verify: No crashes
```

### Test Case 3: Server Shutdown with Active Bots
```
1. Spawn 500 bots
2. Put bots in various states (combat, running, idle, dead)
3. Shut down server (.server shutdown 0)
4. Verify: Clean shutdown, no crashes
```

### Test Case 4: Bot Death-Resurrection Cycle
```
1. Spawn 50 bots
2. Kill all bots
3. Let them resurrect
4. Repeat 10 times
5. Verify: No crashes during resurrections
```

---

## 10. Configuration Recommendations

Even with the fix, keep resurrection throttling for performance:

```ini
[playerbots.conf]
# Resurrection throttling (helps reduce update queue pressure)
Playerbot.DeathRecovery.ResurrectionThrottle = 100  # 100ms delay between resurrections
Playerbot.DeathRecovery.ResurrectionBatchSize = 10  # Max 10 simultaneous resurrections

# Enable debug logging for testing
Playerbot.LogDeathRecovery = true
```

---

## 11. Summary

**Root Cause**: TrinityCore's `Object::RemoveFromWorld()` doesn't call `Map::RemoveUpdateObject()`, leaving dangling pointers in the map's update queue.

**Trigger**: Bots logging out/disconnecting while they have pending updates in `Map::_updateObjects`.

**Impact**: Server crashes with `ASSERT(obj->IsInWorld())` at `Map.cpp:1940`.

**Fix**: Implement `BotCleanupHelper::SafeRemoveFromWorld()` wrapper that calls `Map::RemoveUpdateObject()` before `RemoveFromWorld()`.

**Status**: Ready to implement Solution C (module-only wrapper approach).
