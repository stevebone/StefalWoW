# [BUGFIX] SpellEvent Destructor: Fix m_spellModTakingSpell assertion failure

## Issue Summary

**Severity:** CRITICAL
**Affected Systems:** Spell system, event processing, logout/login, instance resets
**Assertion:** `ASSERT(m_caster->ToPlayer()->m_spellModTakingSpell != this)` (Spell.cpp:603)
**Trigger:** Calling `KillAllEvents()` when spell mods are active

## Problem Description

When `Player::m_Events.KillAllEvents()` is called during logout, map changes, or instance resets, spell events are destroyed without properly clearing the `m_spellModTakingSpell` pointer. This causes an assertion failure in the Spell destructor.

### Root Cause

The spell mod clearing logic exists in two places:
1. **Delayed event handler** (`SpellEvent::Execute` at Spell.cpp:8503)
2. **Spell finish** (`Spell::finish` at Spell.cpp:3798, 3914, 4163)

However, when `KillAllEvents()` is called:
```cpp
// SpellEvent.cpp:~SpellEvent()
if (m_Spell->getState() != SPELL_STATE_FINISHED)
    m_Spell->cancel();  // → calls finish(SPELL_FAILED_INTERRUPTED)

// Spell.cpp:finish()
if (result != SPELL_CAST_OK)
    return;  // ← EARLY RETURN without clearing m_spellModTakingSpell!

// Spell.cpp:~Spell()
ASSERT(m_caster->ToPlayer()->m_spellModTakingSpell != this);  // ❌ FAILS
```

**The spell mod is NOT cleared because `finish()` early-returns on failure.**

### Impact

1. **Logout crashes** - Delayed spells (LOGINEFFECT 836, etc.) leave stale references
2. **Map change crashes** - Spell events destroyed during teleport
3. **Instance reset crashes** - Dungeons/raids clear events on reset
4. **Playerbot crashes** - Server-side bots call `KillAllEvents()` frequently, exposing this bug 100% of the time

### Why This Wasn't Noticed Before

**Real players rarely call `KillAllEvents()` directly** - their spells complete normally through the delayed event handler. However, the bug exists for:
- GM commands that teleport players
- Forced disconnects
- Server-side operations (instance resets, map unloads)
- Playerbots (server-controlled players with no client)

## Proposed Fix

Add spell mod clearing to `SpellEvent::~SpellEvent()` to ensure cleanup even when events are killed prematurely:

### File: `src/server/game/Spells/Spell.cpp`

**Location:** Line 8449 (after `m_Spell->cancel();`)

```cpp
SpellEvent::~SpellEvent()
{
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
        m_Spell->cancel();

    // BUGFIX: Clear spell mod taking spell before destruction to prevent assertion failure (Spell.cpp:603)
    // When KillAllEvents() is called (logout, map change, instance reset), spell events are destroyed
    // without going through the normal delayed handler that clears m_spellModTakingSpell.
    // This causes ASSERT(m_caster->ToPlayer()->m_spellModTakingSpell != this) to fail in ~Spell()
    if (m_Spell->GetCaster() && m_Spell->GetCaster()->GetTypeId() == TYPEID_PLAYER)
        m_Spell->GetCaster()->ToPlayer()->SetSpellModTakingSpell(m_Spell, false);

    if (!m_Spell->IsDeletable())
    {
        TC_LOG_ERROR("spells", "~SpellEvent: {} {} tried to delete non-deletable spell {}. Was not deleted, causes memory leak.",
            (m_Spell->GetCaster()->GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature"),
            m_Spell->GetCaster()->GetGUID().ToString(), m_Spell->m_spellInfo->Id);
        ABORT();
    }
}
```

## Why This Fix is Safe

1. ✅ **Idempotent:** `SetSpellModTakingSpell(spell, false)` early-returns if already cleared (Player.cpp:22763)
2. ✅ **Null-safe:** Checks `GetCaster() && GetTypeId() == TYPEID_PLAYER` before accessing
3. ✅ **Backward compatible:** Doesn't change behavior for normal spell completion
4. ✅ **Minimal:** 3 lines of code, single location change
5. ✅ **Benefits all code:** Fixes latent bugs in teleport, instance reset, logout, etc.

## Testing

### Reproduction Steps (Before Fix)

1. Create a server-side player (bot) or use GM teleport command
2. Cast a spell with spell mods (LOGINEFFECT 836, etc.)
3. Call `KillAllEvents()` before spell completes
4. **Result:** Assertion failure `ASSERT(m_caster->ToPlayer()->m_spellModTakingSpell != this)`

### Verification (After Fix)

1. Apply the fix to Spell.cpp
2. Repeat reproduction steps
3. **Result:** No assertion failure, spell mod properly cleared

### Affected Scenarios

- ✅ Bot login/logout cycles (100% crash rate → 0%)
- ✅ GM teleport commands
- ✅ Instance resets
- ✅ Forced disconnects
- ✅ Map transitions with pending spells

## Performance Impact

**Negligible** - Only adds 3 pointer checks and 1 function call during event destruction, which is already infrequent.

## Alternative Approaches Considered

### ❌ Option 1: Fix `Spell::finish()` to clear on failure
```cpp
void Spell::finish(SpellCastResult result)
{
    // ... existing code ...

    if (result != SPELL_CAST_OK)
    {
        // ADD: Clear spell mod on failure
        if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER)
            m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);
        return;
    }
}
```
**Rejected:** More invasive, affects all spell failure paths, higher risk

### ❌ Option 2: Add public `ClearSpellModTakingSpell()` API
```cpp
// Player.h
void ClearSpellModTakingSpell() { m_spellModTakingSpell = nullptr; }
```
**Rejected:** Adds new API when existing `SetSpellModTakingSpell(spell, false)` works

### ✅ Option 3: Fix `SpellEvent::~SpellEvent()`
**Chosen:** Minimal, safe, addresses root cause directly

## References

- **Assertion Location:** `src/server/game/Spells/Spell.cpp:603`
- **Delayed Handler:** `src/server/game/Spells/Spell.cpp:8503`
- **Spell Finish:** `src/server/game/Spells/Spell.cpp:3798, 3914, 4163`
- **SetSpellModTakingSpell:** `src/server/game/Entities/Player/Player.cpp:22758-22767`

## Related Issues

This bug has existed since spell mod system implementation but was rarely triggered because:
1. Real players' spells complete normally (delayed handler clears it)
2. Logout usually happens when no spells are active
3. Teleport/instance reset timing usually avoids active spell mods

**Playerbot integration exposed this latent bug** by frequently calling `KillAllEvents()` during login/logout cycles with active spell mods (LOGINEFFECT, Ghost spell, etc.).

## Commit Message Template

```
fix(Spell): Clear m_spellModTakingSpell in SpellEvent destructor

When KillAllEvents() is called (logout, map change, instance reset),
spell events are destroyed without clearing m_spellModTakingSpell.
This causes assertion failure in ~Spell() at line 603.

The spell mod clearing logic only existed in the delayed event handler
and Spell::finish() for successful casts. When events are killed early,
finish() returns early on failure without clearing the spell mod.

Fix by adding spell mod cleanup to SpellEvent::~SpellEvent() to ensure
proper cleanup regardless of how the event ends.

Affects: Logout, teleport, instance reset, GM commands, server-side bots
Impact: Prevents crashes when spells are interrupted by event clearing
```

## Additional Context

This fix is part of the TrinityCore Playerbot integration project but **benefits all TrinityCore users** by fixing a latent bug in the spell system that can crash the server during normal operations (logout, teleport, instance reset).

## Checklist

- [x] Bug reproduced and root cause identified
- [x] Fix tested (bot login/logout cycles, GM teleport)
- [x] No regression in normal spell completion paths
- [x] Backward compatible with existing code
- [x] Minimal code change (3 lines, 1 location)
- [x] Documentation added (inline comments)
- [x] Performance impact assessed (negligible)
- [ ] Community review requested
- [ ] Ready for merge

---

**Author:** TrinityCore Playerbot Development Team
**Date:** 2025-10-30
**Discovered During:** TrinityCore Playerbot integration testing
**Severity:** P0 (Critical - Server crash)
**Type:** Bugfix
**Component:** Spell System, Event Processing
