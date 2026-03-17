# BotResurrectionScript - DEPRECATED AND REMOVED

**Date**: 2025-01-05
**Reason**: Critical bug causing bots to skip graveyard teleport sequence
**Status**: Files renamed to .DEPRECATED, marked for deletion

## Problem Statement

`BotResurrectionScript::OnPlayerRepop()` was auto-resurrecting bots **BEFORE** TrinityCore's `RepopAtGraveyard()` teleport could execute, causing bots to revive instantly at their death location instead of following the proper WoW death sequence.

## Root Cause Analysis

### The Bug Sequence
```
1. Bot dies
2. DeathRecoveryManager sends CMSG_REPOP_REQUEST
3. TrinityCore processes packet → calls BuildPlayerRepop()
4. BuildPlayerRepop() fires OnPlayerRepop hook
5. ❌ BotResurrectionScript::OnPlayerRepop() auto-resurrects bot (WRONG!)
6. ❌ RepopAtGraveyard() skipped because bot is already alive
```

### The Correct Sequence Should Be
```
1. Bot dies
2. DeathRecoveryManager sends CMSG_REPOP_REQUEST
3. TrinityCore processes packet → calls BuildPlayerRepop()
4. BuildPlayerRepop() fires OnPlayerRepop hook (observer only, no resurrection)
5. ✅ RepopAtGraveyard() teleports bot to nearest graveyard
6. ✅ Bot runs from graveyard to corpse
7. ✅ DeathRecoveryManager resurrects bot when at corpse (state 6)
```

## Why VALIDATION 5A Failed

The script had a "corpse freshness" check (line 152-171) intended to prevent resurrection before graveyard teleport:

```cpp
// If corpse was created within last 10 seconds, bot hasn't had time to teleport yet
time_t corpseAge = currentTime - ghostTime;
if (corpseAge < 10)
{
    failureReason = "Corpse too fresh (need 10+ seconds for graveyard teleport)";
    return false;
}
```

**The Problem**: This check measures **corpse AGE**, not **whether teleport has occurred**. OnPlayerRepop fires **BEFORE** RepopAtGraveyard() is called, so even a 10-second delay doesn't help - the bot is still at the death location when the validation runs.

## Solution

**DISABLE BotResurrectionScript entirely** and let `DeathRecoveryManager` handle resurrection after the corpse run completes. The DeathRecoveryManager already has the correct state machine:

- State 5: `RUNNING_TO_CORPSE` - Bot runs from graveyard to corpse
- State 6: `AT_CORPSE` - Bot reaches corpse → **RESURRECT HERE** ✅
- State 10: `RESURRECTING` - Resurrection in progress

## Files Deprecated

- `BotResurrectionScript.h` → `BotResurrectionScript.h.DEPRECATED`
- `BotResurrectionScript.cpp` → `BotResurrectionScript.cpp.DEPRECATED`

## Code Changes

**PlayerbotEventScripts.cpp**:
- Commented out `#include "Scripting/BotResurrectionScript.h"`
- Commented out `new Playerbot::BotResurrectionScript()`
- Added comprehensive explanation of why script was disabled

## Evidence from TrinityCore Source

**Player.cpp:4815** - RepopAtGraveyard() implementation:
```cpp
TeleportTo({ .Location = closestGrave->Loc, ...},
           shouldResurrect ? TELE_REVIVE_AT_TELEPORT : TELE_TO_NONE);
```

This shows that TrinityCore **DOES** perform the graveyard teleport when `RepopAtGraveyard()` is called. The issue was that `BotResurrectionScript` was intercepting and resurrecting bots before this function could execute.

## Testing Notes

After this fix, bots should:
1. ✅ Die and release spirit
2. ✅ Get teleported to nearest graveyard
3. ✅ Run as ghosts from graveyard to their corpse
4. ✅ Resurrect when reaching the corpse location

## Related Files

- `DeathRecoveryManager.cpp` - Proper death recovery state machine (still active)
- `PlayerbotEventScripts.cpp` - Script registration (BotResurrectionScript disabled)
- `Player.cpp:4784` - TrinityCore's RepopAtGraveyard() implementation

## Recommendation

**DELETE** these deprecated files after confirming the fix works correctly in-game.

---

**Fix implemented by**: Claude Code
**Issue reported by**: User in-game testing
**Fix date**: 2025-01-05
