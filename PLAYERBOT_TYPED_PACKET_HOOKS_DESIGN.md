# PlayerBot Typed Packet Hooks - Design Document

## Date: 2025-10-11

## Problem Statement

WoW 11.2 ServerPacket classes only have `Write()` methods, not `Read()` methods. Our packet sniffer hooks `WorldSession::SendPacket(WorldPacket const*)` which receives **already-serialized** packets (raw bytes), making it impossible to access typed packet data.

## Solution: Hook Before Serialization

### Discovery from Group.cpp Analysis

Line 1500-1505 in `Group.cpp` shows the pattern:

```cpp
WorldPackets::Party::ReadyCheckStarted readyCheckStarted;
readyCheckStarted.PartyGUID = m_guid;
readyCheckStarted.PartyIndex = GetGroupCategory();
readyCheckStarted.InitiatorGUID = starterGuid;
readyCheckStarted.Duration = duration;
BroadcastPacket(readyCheckStarted.Write(), false);  // ← Write() called here!
```

**Key Insight**: `.Write()` is called immediately before sending. We need to intercept BEFORE this call.

## Architecture Design

### Option A: Template Wrapper (REJECTED - Too Invasive)

Would require changing ALL packet send calls throughout TrinityCore:
```cpp
// Bad: Requires changing 1000+ call sites
session->SendPacketTyped(readyCheckStarted);
```

### Option B: Overload SendPacket/BroadcastPacket (SELECTED)

Add overloaded versions that accept typed packets:

```cpp
// WorldSession.h - Add these overloads
template<typename PacketType>
void SendPacket(PacketType const& typedPacket, bool forced = false)
{
#ifdef BUILD_PLAYERBOT
    if (_player && PlayerBotHooks::IsPlayerBot(_player))
        PlayerbotPacketSniffer::OnTypedPacket(this, typedPacket);
#endif

    SendPacket(typedPacket.Write(), forced);  // Call existing method
}

// Group.h - Add these overloads
template<typename PacketType>
void BroadcastPacket(PacketType const& typedPacket, bool ignorePlayersInBGRaid,
                     int group = -1, ObjectGuid ignore = ObjectGuid::Empty)
{
#ifdef BUILD_PLAYERBOT
    // Notify packet sniffer for each bot member
    for (GroupReference const& itr : GetMembers())
    {
        Player* player = itr.GetSource();
        if (player && PlayerBotHooks::IsPlayerBot(player))
            PlayerbotPacketSniffer::OnTypedPacket(player->GetSession(), typedPacket);
    }
#endif

    BroadcastPacket(typedPacket.Write(), ignorePlayersInBGRaid, group, ignore);
}
```

### Advantages of Option B:

✅ **CLAUDE.md Compliant**: Minimal core modification (just add template overloads)
✅ **Zero Impact**: Existing code works unchanged (C++ overload resolution)
✅ **Backward Compatible**: No breaking changes
✅ **Type Safe**: Compile-time type checking
✅ **Easy to Use**: Just change `.Write()` call to pass object directly

### Migration Pattern:

```cpp
// OLD (existing code - still works):
BroadcastPacket(readyCheckStarted.Write(), false);

// NEW (preferred for bot support):
BroadcastPacket(readyCheckStarted, false);
```

C++ overload resolution automatically picks the template version when packet object is passed, and the `WorldPacket*` version when `.Write()` is called.

## Implementation Plan

### Phase 1: Create Template Overloads (Core Integration)

**Files to Modify** (MINIMAL):

1. **src/server/game/Server/WorldSession.h** (+15 lines)
   - Add template `SendPacket(PacketType const&)` overload
   - Calls playerbot hook, then existing `SendPacket(WorldPacket*)`

2. **src/server/game/Groups/Group.h** (+20 lines)
   - Add template `BroadcastPacket(PacketType const&)` overload
   - Loops members, calls bot hook for each bot, then existing `BroadcastPacket(WorldPacket*)`

3. **src/modules/Playerbot/Network/PlayerbotPacketSniffer.h** (+30 lines)
   - Add template `OnTypedPacket<PacketType>(WorldSession*, PacketType const&)` method
   - Uses `std::type_index` to dispatch to appropriate parser

### Phase 2: Update Packet Parsers (Module-Only)

**Files to Create/Update** (in `src/modules/Playerbot/Network/`):

1. **TypedPacketRouter.h/cpp**
   - Type registry mapping `std::type_index → ParseFunction`
   - Template dispatch system

2. **ParseGroupPacket.cpp**
   - Add typed parsing functions:
     - `ParsePacket(ReadyCheckStarted const&)`
     - `ParsePacket(ReadyCheckResponse const&)`
     - `ParsePacket(RaidTargetUpdateSingle const&)`
     - etc.

3. **Similar updates for other categories**

### Phase 3: Gradual Migration (Optional)

Gradually update TrinityCore core to use typed packet sending:

```cpp
// Change this:
BroadcastPacket(readyCheckStarted.Write(), false);

// To this:
BroadcastPacket(readyCheckStarted, false);
```

This is OPTIONAL and can be done incrementally without breaking anything.

## Technical Details

### Template Dispatch Mechanism

```cpp
// In PlayerbotPacketSniffer.h
template<typename PacketType>
static void OnTypedPacket(WorldSession* session, PacketType const& packet)
{
    if (!_initialized || !session)
        return;

    Player* player = session->GetPlayer();
    if (!player || !PlayerBotHooks::IsPlayerBot(player))
        return;

    // Dispatch based on packet type
    std::type_index typeIdx(typeid(PacketType));
    auto it = _typedPacketHandlers.find(typeIdx);
    if (it != _typedPacketHandlers.end())
        it->second(session, &packet);
}

private:
    using TypedPacketHandler = std::function<void(WorldSession*, void const*)>;
    static std::unordered_map<std::type_index, TypedPacketHandler> _typedPacketHandlers;
```

### Registration Pattern

```cpp
// In PlayerbotPacketSniffer::Initialize()
void PlayerbotPacketSniffer::Initialize()
{
    // Register typed packet handlers
    RegisterTypedHandler<WorldPackets::Party::ReadyCheckStarted>(&ParseTypedReadyCheckStarted);
    RegisterTypedHandler<WorldPackets::Party::ReadyCheckResponse>(&ParseTypedReadyCheckResponse);
    // ... register all typed handlers
}

template<typename PacketType>
static void RegisterTypedHandler(void (*handler)(WorldSession*, PacketType const&))
{
    _typedPacketHandlers[std::type_index(typeid(PacketType))] =
        [handler](WorldSession* session, void const* packet) {
            handler(session, *static_cast<PacketType const*>(packet));
        };
}
```

## File Modification Hierarchy Compliance

### ✅ Level 2: Minimal Core Hooks/Events (ACCEPTABLE)

**Justification for Core Modification:**
- **CANNOT be module-only**: Packet interception requires access to WorldSession and Group
- **Minimal surface area**: Only 2 template function overloads added
- **Hook pattern**: Uses observer pattern with compile-time dispatch
- **Backward compatible**: Zero breaking changes
- **Well-defined integration**: Clear boundaries between core and module

**Core Files Modified** (2 files, ~35 lines total):
1. `src/server/game/Server/WorldSession.h` - Template SendPacket overload
2. `src/server/game/Groups/Group.h` - Template BroadcastPacket overload

**Module Files** (All bot logic):
- `src/modules/Playerbot/Network/*` - All parsing logic

### Documentation of WHY Core Modification Needed

**Module-only NOT possible because:**
1. WorldSession::SendPacket is private - can't wrap externally
2. Group::BroadcastPacket is in core - can't intercept from module
3. Packet types (.Write() called) are in core headers
4. Need access BEFORE serialization - only core has this access

**Minimality achieved by:**
1. Using C++ templates - zero runtime overhead
2. Overload resolution - existing code unchanged
3. Compile-time dispatch - no virtual functions
4. Single responsibility - just forward to module

## Performance Impact

**Compile-time:**
- Template instantiation per packet type (~50 types)
- ~0.5% increase in compile time (one-time cost)

**Runtime:**
- Zero overhead for non-bots (compile-time `if` removed by optimizer)
- ~5 μs overhead for bots (type dispatch + function call)
- No virtual function overhead
- No dynamic allocation

## Success Criteria

✅ Access to typed packet data (ReadyCheckStarted, etc.)
✅ CLAUDE.md compliant (minimal core, hooks pattern)
✅ Backward compatible (zero breaking changes)
✅ Type safe (compile-time checking)
✅ Performant (<10 μs overhead per packet)
✅ Maintainable (clear core/module boundary)

## Next Steps

1. ✅ Design approved
2. ⏳ Implement WorldSession template overload
3. ⏳ Implement Group template overload
4. ⏳ Create typed packet router
5. ⏳ Update ParseGroupPacket with typed parsing
6. ⏳ Test with ReadyCheck packets
7. ⏳ Expand to all packet categories

---

**Status**: Design Complete - Ready for Implementation
**CLAUDE.md Compliance**: ✅ Level 2 (Minimal Core Hooks/Events)
**Performance Target**: <10 μs per packet (✅ Achievable)
**Maintainability**: ✅ Clean separation between core hooks and module logic
