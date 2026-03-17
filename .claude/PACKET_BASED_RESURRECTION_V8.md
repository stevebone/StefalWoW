# Packet-Based Resurrection Implementation (v8)

**Date**: 2025-10-29
**Status**: ✅ Complete - Ready for Testing

## Problem Statement

All previous resurrection approaches (v1-v7) failed with `Spell::~Spell` assertion at Spell.cpp:603:

```
ASSERT(m_caster->ToPlayer()->m_spellModTakingSpell != this);
```

**Root Cause**: Attempting to clean up spell events on worker thread before scheduling main-thread resurrection callback. Even with:
- `m_spellModTakingSpell = nullptr`
- `KillAllEvents(true)`
- `m_Events.Update(0)`
- `RemoveAllAuras()`

...spell events were still being destroyed during continued `Player::Update()` execution on worker thread, causing assertion failures.

## Solution: Packet-Based Resurrection

**Key Insight**: Real players resurrect via packet system, not direct function calls.

### How Real Players Resurrect

1. Client sends `CMSG_RECLAIM_CORPSE` packet
2. Network thread receives packet
3. Packet queued via `WorldSession::QueuePacket()`
4. Main thread processes packet in `WorldSession::Update()`
5. `WorldSession::HandleReclaimCorpse()` executes on main thread (PROCESS_THREADUNSAFE)
6. Handler calls `ResurrectPlayer()` on main thread
7. Complete thread safety - no worker thread involvement!

### Bot Implementation

**Instead of:**
```cpp
// FAILED v1-v7 approach
Worker thread → Clear events → AddFarSpellCallback → Main thread ResurrectPlayer
```

**Now:**
```cpp
// v8 packet-based approach
Worker thread → QueuePacket(CMSG_RECLAIM_CORPSE) →
Main thread HandleReclaimCorpse → ResurrectPlayer
```

## Implementation Details

### PATH 1: HandleAtCorpse - GM Force Resurrect

```cpp
// Create CMSG_RECLAIM_CORPSE packet
WorldPacket* reclaimPacket = new WorldPacket(CMSG_RECLAIM_CORPSE, 16);
*reclaimPacket << corpse->GetGUID();

// Queue for main thread processing
m_bot->GetSession()->QueuePacket(reclaimPacket);
```

**Opcode**: `CMSG_RECLAIM_CORPSE (0x300073)`
**Handler**: `WorldSession::HandleReclaimCorpse` (MiscHandler.cpp:417-446)
**Thread**: Main thread (PROCESS_THREADUNSAFE)

### PATH 2: ExecuteGraveyardResurrection - Spirit Healer

```cpp
// Create CMSG_REPOP_REQUEST packet
WorldPacket* repopPacket = new WorldPacket(CMSG_REPOP_REQUEST, 1);
*repopPacket << uint8(0); // CheckInstance = false

// Queue for main thread processing
m_bot->GetSession()->QueuePacket(repopPacket);
```

**Opcode**: `CMSG_REPOP_REQUEST (0x3000C4)`
**Handler**: `WorldSession::HandleRepopRequest`
**Thread**: Main thread (PROCESS_THREADUNSAFE)

### PATH 3: ForceResurrection - Manual Force Resurrect

```cpp
Corpse* corpse = m_bot->GetCorpse();
if (corpse)
{
    // Use CMSG_RECLAIM_CORPSE if corpse exists
    WorldPacket* reclaimPacket = new WorldPacket(CMSG_RECLAIM_CORPSE, 16);
    *reclaimPacket << corpse->GetGUID();
    m_bot->GetSession()->QueuePacket(reclaimPacket);
}
else
{
    // Fallback to CMSG_REPOP_REQUEST if no corpse
    WorldPacket* repopPacket = new WorldPacket(CMSG_REPOP_REQUEST, 1);
    *repopPacket << uint8(0);
    m_bot->GetSession()->QueuePacket(repopPacket);
}
```

### PATH 4: InteractWithCorpse - Corpse Reclaim (Main Path)

```cpp
// Create CMSG_RECLAIM_CORPSE packet
WorldPacket* reclaimPacket = new WorldPacket(CMSG_RECLAIM_CORPSE, 16);
*reclaimPacket << corpse->GetGUID();

// Queue for main thread processing
m_bot->GetSession()->QueuePacket(reclaimPacket);
```

## Packet Processing Flow

### CMSG_RECLAIM_CORPSE Processing

1. **Worker Thread** (DeathRecoveryManager::Update):
   ```cpp
   m_bot->GetSession()->QueuePacket(reclaimPacket);
   ```

2. **Packet Queueing** (WorldSession.cpp:337-340):
   ```cpp
   void WorldSession::QueuePacket(WorldPacket* new_packet)
   {
       _recvQueue.add(new_packet);
   }
   ```

3. **Main Thread Processing** (WorldSession::Update):
   - Dequeues packet from `_recvQueue`
   - Dispatches to handler based on opcode

4. **Handler Execution** (MiscHandler.cpp:417-446):
   ```cpp
   void WorldSession::HandleReclaimCorpse(WorldPackets::Misc::ReclaimCorpse& packet)
   {
       // Validation checks (alive, arena, ghost flag, delay, distance)

       // Resurrect on main thread
       _player->ResurrectPlayer(_player->InBattleground() ? 1.0f : 0.5f);
       _player->SpawnCorpseBones();
   }
   ```

5. **ResurrectPlayer** (Player.cpp:4362-4442):
   - Removes ghost auras (8326, 20584)
   - Sets death state to ALIVE
   - Restores health/mana
   - Updates visibility
   - Casts item enchant spells (`CastAllObtainSpells`)
   - Applies resurrection sickness if needed
   - All on main thread - completely safe!

## Benefits

### ✅ Thread Safety

- **No worker thread cleanup**: No `m_spellModTakingSpell`, `KillAllEvents`, `RemoveAllAuras` on worker thread
- **No callbacks**: No `Map::AddFarSpellCallback` scheduling complexity
- **Pure main thread**: Entire resurrection process executes on main thread

### ✅ TrinityCore Integration

- **Proven code path**: Uses exact same handlers as real players
- **Validation built-in**: TrinityCore handlers perform all validation (alive, arena, ghost, delay, distance)
- **Maintained code**: Uses core TrinityCore systems, not custom bot logic

### ✅ Simplicity

- **Fewer lines**: Eliminated ~40 lines of worker thread cleanup per path
- **Less complexity**: No spell event lifecycle management
- **Clear flow**: Worker thread → Queue packet → Main thread handles everything

## Testing Checklist

- [ ] Test PATH 1: GM force resurrect at corpse
- [ ] Test PATH 2: Spirit healer resurrection at graveyard
- [ ] Test PATH 3: Manual force resurrection (with/without corpse)
- [ ] Test PATH 4: Normal corpse reclaim (main path)
- [ ] Test with 100+ concurrent bot deaths
- [ ] Test during combat with active spell events
- [ ] Monitor for `Spell::~Spell` assertions
- [ ] Verify resurrection sickness application
- [ ] Verify ghost aura removal
- [ ] Check health/mana restoration

## Technical References

### TrinityCore Packet System

- **Opcodes**: `src/server/game/Server/Protocol/Opcodes.h`
- **Packet Structures**: `src/server/game/Server/Packets/MiscPackets.h`
- **Handlers**: `src/server/game/Handlers/MiscHandler.cpp`
- **Packet Processing**: `src/server/game/Server/WorldSession.cpp`

### Resurrection Mechanics

- **ResurrectPlayer**: `src/server/game/Entities/Player/Player.cpp:4362-4442`
- **HandleReclaimCorpse**: `src/server/game/Handlers/MiscHandler.cpp:417-446`
- **HandleRepopRequest**: `src/server/game/Handlers/MiscHandler.cpp:448-478`

### Opcode Definitions

```cpp
CMSG_RECLAIM_CORPSE = 0x300073  // Corpse resurrection
CMSG_REPOP_REQUEST  = 0x3000C4  // Spirit healer/graveyard resurrection
```

### Packet Handler Registration

```cpp
// Opcodes.cpp:828
DEFINE_HANDLER(CMSG_RECLAIM_CORPSE, STATUS_LOGGEDIN, PROCESS_THREADUNSAFE, &WorldSession::HandleReclaimCorpse);

// PROCESS_THREADUNSAFE = Executes on main thread (world thread)
```

## Code Changes

### Files Modified

- `src/modules/Playerbot/Lifecycle/DeathRecoveryManager.cpp`

### Lines Changed

- **PATH 1** (HandleAtCorpse): ~25 lines removed, ~10 added
- **PATH 2** (ExecuteGraveyardResurrection): ~40 lines removed, ~9 added
- **PATH 3** (ForceResurrection): ~25 lines removed, ~18 added
- **PATH 4** (InteractWithCorpse): ~35 lines removed, ~12 added

**Total**: ~125 lines removed, ~49 added = **Net -76 lines**

### Removed Code

- All `m_bot->m_spellModTakingSpell = nullptr` assignments
- All `m_bot->m_Events.KillAllEvents()` calls
- All `m_bot->m_Events.Update(0)` force updates
- All `m_bot->RemoveAllAuras()` calls
- All `m_bot->GetMap()->AddFarSpellCallback` lambda scheduling
- All worker thread spell event cleanup logic

### Added Code

- `WorldPacket* reclaimPacket = new WorldPacket(CMSG_RECLAIM_CORPSE, 16)`
- `*reclaimPacket << corpse->GetGUID()`
- `m_bot->GetSession()->QueuePacket(reclaimPacket)`
- `WorldPacket* repopPacket = new WorldPacket(CMSG_REPOP_REQUEST, 1)`
- `*repopPacket << uint8(0)`
- `m_bot->GetSession()->QueuePacket(repopPacket)`

## Conclusion

The packet-based resurrection approach (v8) represents a fundamental architectural shift from callback-based to packet-based resurrection. By using TrinityCore's packet system, we completely eliminate worker thread synchronization issues and use the exact same code path as real players.

This solution is:
- **Simpler**: Less code, less complexity
- **Safer**: No worker thread cleanup, all on main thread
- **More maintainable**: Uses core TrinityCore systems
- **More reliable**: Proven code path used by millions of real players

**Expected Result**: Zero `Spell::~Spell` assertions, stable bot resurrection at scale.
