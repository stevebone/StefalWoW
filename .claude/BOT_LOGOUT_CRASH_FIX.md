# Bot Logout Crash Fix - Map Iterator Race Condition

## Issue Summary

**Crash Location**: `Map.cpp:686` - ACCESS_VIOLATION when dereferencing map reference iterator
**Root Cause**: Bot exception handler calling `RemoveFromWorld()` directly from worker thread during `Map::Update()` iteration
**Impact**: Server crash when bot login fails and cleanup runs concurrently with map updates
**Fix Date**: 2025-10-30

## Technical Analysis

### Crash Mechanism

1. **Map Update Loop** (Map.cpp:684-692):
```cpp
for (m_mapRefIter = m_mapRefManager.begin(); m_mapRefIter != m_mapRefManager.end(); ++m_mapRefIter)
{
    Player* player = m_mapRefIter->GetSource();  // CRASH: returns nullptr

    if (!player || !player->IsInWorld())
        continue;

    player->Update(t_diff);
}
```

2. **Race Condition**:
   - `Map::Update()` runs in worker thread via `MapUpdater::WorkerThread`
   - Bot login exception handler calls `GetPlayer()->RemoveFromWorld()` from worker thread
   - `RemoveFromWorld()` unlinks `MapReference` from `m_mapRefManager`
   - Map iterator `m_mapRefIter` becomes invalid mid-iteration
   - Next `m_mapRefIter->GetSource()` call returns nullptr or invalid pointer
   - ACCESS_VIOLATION crash occurs

### TrinityCore's Correct Logout Flow

**Real Player Logout** (WorldSession.cpp:587-718):

```cpp
void WorldSession::LogoutPlayer(bool save)
{
    // Sets flag for deferred processing
    m_playerLogout = true;
    m_playerSave = save;

    // Cleanup happens in next Update() cycle on main thread:
    // Line 716-718
    if (Map* _map = _player->FindMap())
        _map->RemovePlayerFromMap(_player, true);  // Proper removal

    SetPlayer(nullptr);
}
```

**Key Principles**:
1. `LogoutPlayer()` sets `m_playerLogout` flag immediately
2. Actual map removal deferred to next `Update()` cycle
3. `RemovePlayerFromMap()` called from main thread, not during map iteration
4. No direct `RemoveFromWorld()` calls from worker threads

### Playerbot Bug

**Original Buggy Code** (BotSessionEnhanced.cpp:236-244):

```cpp
// Exception handler during bot login
catch (std::exception const& e)
{
    _loginState = LoginState::LOGIN_FAILED;

    // BUG: Direct RemoveFromWorld() call
    if (GetPlayer())
    {
        if (GetPlayer()->IsInWorld())
        {
            GetPlayer()->RemoveFromWorld();  // Called from worker thread!
        }
        SetPlayer(nullptr);
    }

    return false;
}
```

**Problem**: Exception can occur during login which may run in worker thread context, causing direct `RemoveFromWorld()` call during active `Map::Update()` iteration.

## Solution

**Fixed Code** (BotSessionEnhanced.cpp:236-244):

```cpp
// Exception handler during bot login
catch (std::exception const& e)
{
    TC_LOG_ERROR("module.playerbot.session",
                "Exception during bot login: {}",
                e.what());

    _loginState = LoginState::LOGIN_FAILED;

    // PLAYERBOT FIX: Use LogoutPlayer() to match TrinityCore's logout flow
    // Issue: Direct RemoveFromWorld() call from worker thread causes Map iterator crash (Map.cpp:686)
    // Solution: Call LogoutPlayer() which sets m_playerLogout flag, then RemovePlayerFromMap()
    // is called during next Update() cycle on main thread (WorldSession.cpp:716)
    // This matches how real players logout and prevents race conditions
    if (GetPlayer())
    {
        LogoutPlayer(false);  // false = don't save (login failed)
    }

    return false;
}
```

**Benefits**:
1. Matches TrinityCore's real player logout flow exactly
2. Defers map removal to main thread during next `Update()` cycle
3. Prevents race condition with map iterator
4. No core file modifications required (module-only fix)

## Files Modified

- **src/modules/Playerbot/Session/BotSessionEnhanced.cpp** (lines 236-244)
  - Exception handler cleanup logic
  - Replaced direct `RemoveFromWorld()` with `LogoutPlayer(false)`

## Testing Validation

**Before Fix**:
- Server crash when bot login fails
- ACCESS_VIOLATION at Map.cpp:686
- Map iterator corruption during concurrent logout

**After Fix** (Expected):
- Graceful bot login failure handling
- No map iterator corruption
- Proper deferred cleanup on main thread
- Server stability maintained

## Related Systems

**Other Bot Cleanup Paths** (Already Correct):
- `DeathRecoveryManager::ExecuteReleaseSpirit()` - Uses packets, not direct calls
- `BotSession::UpdateEnhanced()` - Uses `LogoutPlayer()` correctly (line 289)
- `BotWorldEntry::EnterWorldSync()` - Cleanup handled by world entry manager

**Why This Path Was Vulnerable**:
- Exception handler is edge case during login failure
- Can trigger from worker thread context
- Was only code path using direct `RemoveFromWorld()`

## Prevention Guidelines

**For Future Bot Development**:

1. **NEVER** call `RemoveFromWorld()` directly from bot code
2. **ALWAYS** use `LogoutPlayer()` for bot cleanup
3. **ALWAYS** defer map modifications to main thread
4. **FOLLOW** TrinityCore's real player patterns exactly
5. **AVOID** direct map/world manipulation from worker threads

## Commit Information

**Branch**: playerbot-dev
**Commit**: [To be filled after commit]
**Files Changed**: 1
**Lines Changed**: +7 / -5 (net +2)
**Quality**: Module-only fix, no core modifications
**Backwards Compatibility**: Fully maintained
