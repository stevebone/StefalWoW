# PlayerBot Packet Sniffer - Current Status

## Date: 2025-10-11

## Critical Discovery

During WoW 11.2 compilation, discovered that the packet reading approach is fundamentally incompatible:

### Issue: ServerPacket classes don't have Read() methods

In WoW 11.2, packet classes are split into:
- **ServerPacket**: For packets sent FROM server TO client (only has Write() method)
- **ClientPacket**: For packets sent FROM client TO server (only has Read() method)

Our packet sniffer intercepts **outgoing server packets** (WorldSession::SendPacket), which means we're dealing with ServerPacket classes that DON'T have Read() methods.

### Example:
```cpp
class ReadyCheckStarted final : public ServerPacket
{
public:
    WorldPacket const* Write() override;  // ✓ Has Write()
    // ❌ NO Read() method!

    int8 PartyIndex = 0;
    ObjectGuid PartyGUID;
    ObjectGuid InitiatorGUID;
    WorldPackets::Duration<Milliseconds> Duration;
};
```

### Solutions Considered:

#### Option 1: Raw Packet Parsing (Current Approach - BLOCKED)
Parse WorldPacket raw bytes manually
- ❌ **Problem**: Extremely fragile, breaks with every patch
- ❌ **Problem**: Requires deep knowledge of binary packet structure
- ❌ **Problem**: Not maintainable long-term

#### Option 2: Hook Before Packet Serialization (BEST SOLUTION)
Instead of hooking `WorldSession::SendPacket(WorldPacket*)`, hook at the point where typed packets are created:
```cpp
// Instead of this:
WorldSession::SendPacket(WorldPacket const* packet)  // Too late - already serialized

// Hook here:
WorldSession::SendPacket(ReadyCheckStarted const& packet)  // Still has typed data!
```

**Advantages:**
- ✅ Access to strongly-typed packet objects
- ✅ No parsing needed - direct field access
- ✅ Maintainable - uses TrinityCore's own types
- ✅ Type-safe - compile-time checking

**Implementation:**
Create templated SendPacket wrapper that captures typed packets before serialization

#### Option 3: Event Bus Only (FALLBACK)
Don't use packet sniffer at all - subscribe to TrinityCore's existing event systems
- ✅ **Advantage**: Uses official APIs
- ❌ **Problem**: Many events don't exist (ready check, target icons, etc.)
- ❌ **Problem**: Still need packet sniffer for events without APIs

#### Option 4: Opcode-Only Detection (TEMPORARY SOLUTION)
Just detect **which** packets are sent, don't parse contents
- ✅ Can compile and test infrastructure
- ✅ Useful for some use cases (cooldown detection, loot window opened, etc.)
- ❌ Can't extract detailed data (who initiated ready check, which icon, etc.)

### Current Status:

**Blocked on architectural decision**

Cannot proceed with current implementation until we decide:
1. Implement Option 2 (hook before serialization) - **RECOMMENDED**
2. Use Option 4 (opcode-only) as temporary solution
3. Abandon packet sniffer entirely (Option 3)

### Compilation Errors Summary:

1. ✅ **FIXED**: Player::IsPlayerBot() → PlayerBotHooks::IsPlayerBot()
2. ✅ **FIXED**: packet.GetOpcode() type cast to OpcodeServer
3. ✅ **FIXED**: AuraPackets.h → SpellPackets.h
4. ❌ **BLOCKED**: WorldPackets::*::Read(WorldPacket&) doesn't exist for ServerPackets
5. ❌ **PENDING**: ~35+ invalid opcodes for WoW 11.2

### Recommendation:

**Implement Option 2** - Hook before packet serialization

This requires modifying WorldSession to provide typed packet hooks:
```cpp
// In WorldSession.h
template<typename PacketType>
void SendPacketTyped(PacketType const& packet)
{
#ifdef BUILD_PLAYERBOT
    if (_player && PlayerBotHooks::IsPlayerBot(_player))
        PlayerbotPacketSniffer::OnTypedPacket(this, packet);
#endif

    SendPacket(packet.Write());
}
```

This is a CLAUDE.md-compliant minimal core modification that enables the entire packet sniffer system.

### Next Steps (Awaiting Decision):

1. Get approval to implement Option 2 (typed packet hooks)
2. OR: Implement Option 4 (opcode-only) as proof of concept
3. OR: Abandon packet sniffer approach entirely

---

**Blocked Status**: Cannot continue implementation without architectural decision
**Recommendation**: Option 2 (typed packet hooks) - most maintainable solution
**Files Modified**: 8 files created, 3 critical fixes applied, ~35 opcodes still need verification
