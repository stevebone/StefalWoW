# WoW 12.0.0 Migration Analysis for Playerbot Module

**Upstream Commit:** `b0a596908d5c1b5b09f90e97b17a7fc785e5366f`
**Commit Message:** "Core: Updated to 12.0.0"
**Impact:** 92 files changed, +8,452/-5,638 lines
**Analysis Date:** 2026-01-29

---

## Executive Summary

TrinityCore has updated to WoW 12.0.0 (The War Within expansion continuation). This is a **MAJOR** protocol update affecting:
- All network opcodes (complete renumbering)
- Stats system (Spirit stat re-added)
- Packet structures (new fields, restructured data)
- UpdateFields (expanded masks, new fields)
- Spell system (new attributes, modifiers, auras)
- Quest system (new reward types)
- Housing system (new map types)

**Estimated Playerbot Impact:** HIGH - Requires significant updates across multiple systems.

---

## Critical Changes Requiring Immediate Playerbot Updates

### 1. Stats System Overhaul

**Change:** Spirit stat (STAT_SPIRIT = 4) has been re-added to the game.

```cpp
// OLD (11.x)
enum Stats : uint16 {
    STAT_STRENGTH  = 0,
    STAT_AGILITY   = 1,
    STAT_STAMINA   = 2,
    STAT_INTELLECT = 3,
};
#define MAX_STATS 4

// NEW (12.0)
enum Stats : uint16 {
    STAT_STRENGTH  = 0,
    STAT_AGILITY   = 1,
    STAT_STAMINA   = 2,
    STAT_INTELLECT = 3,
    STAT_SPIRIT    = 4,  // NEW
};
#define MAX_STATS 5
```

**Playerbot Files Affected:**
- `src/modules/Playerbot/Combat/Stats/` - All stat calculation code
- `src/modules/Playerbot/AI/` - Stat priority evaluation
- `src/modules/Playerbot/Gear/` - Item stat weighting
- Any code using `Stats` enum or `MAX_STATS`

**Required Changes:**
- Update all stat arrays from size 4 to size 5
- Add Spirit handling to stat evaluation logic
- Update gear scoring to consider Spirit for healers

---

### 2. UnitMods System Update

**Change:** New `UNIT_MOD_STAT_SPIRIT` added.

```cpp
// NEW entries in UnitMods enum
UNIT_MOD_STAT_SPIRIT,  // After UNIT_MOD_STAT_INTELLECT

// Range change
UNIT_MOD_STAT_END = UNIT_MOD_STAT_SPIRIT + 1,  // Was UNIT_MOD_STAT_INTELLECT + 1
```

**Playerbot Files Affected:**
- Any code iterating `UNIT_MOD_STAT_START` to `UNIT_MOD_STAT_END`

---

### 3. Difficulty Enum Type Change

**Change:** `Difficulty` changed from `uint8` to `int16`.

```cpp
// OLD
enum Difficulty : uint8;

// NEW
enum Difficulty : int16;
```

**Playerbot Files Affected:**
- `src/modules/Playerbot/Dungeon/` - All dungeon difficulty handling
- `src/modules/Playerbot/Quest/` - Quest difficulty checks
- Any function signatures using `Difficulty` parameters

---

### 4. Spell Packet Structure Changes

**SpellCastRequest.Misc Array Expansion:**
```cpp
// OLD
int32 Misc[2] = { };

// NEW
std::array<int32, 3> Misc = { };
```

**New SpellFailure Fields:**
```cpp
// All spell failure packets now have:
ObjectGuid FailedBy;  // Unit that caused the spell to fail
```

**SpellCraftingReagent Restructured:**
```cpp
// OLD
struct SpellCraftingReagent {
    int32 ItemID = 0;
    int32 DataSlotIndex = 0;
    int32 Quantity = 0;
    Optional<uint8> Source;
};

// NEW
struct SpellCraftingReagent {
    int32 Slot = 0;
    int32 Quantity = 0;
    Crafting::CraftingReagentBase Reagent;
    Optional<uint8> Source;
};
```

**Playerbot Files Affected:**
- `src/modules/Playerbot/Combat/Spells/` - All spell casting code
- `src/modules/Playerbot/Crafting/` - If any crafting integration exists
- Any code building `SpellCastRequest` packets

**Required Changes:**
- Update `CastItemUseSpell` calls: `int32* misc` → `std::array<int32, 3> const& misc`
- Handle `FailedBy` in spell failure handlers

---

### 5. Party/Group Packet Changes

**Critical Change - RequestPartyMemberStats:**
```cpp
// OLD
ObjectGuid TargetGUID;

// NEW
Array<ObjectGuid, 40> Targets;  // Now supports multiple targets!
```

**Difficulty Settings Type Changes:**
```cpp
// OLD
uint32 DungeonDifficultyID = 0u;
uint32 RaidDifficultyID = 0u;
uint32 LegacyRaidDifficultyID = 0u;

// NEW
int16 DungeonDifficultyID = 0u;
int16 RaidDifficultyID = 0u;
int16 LegacyRaidDifficultyID = 0u;
```

**Playerbot Files Affected:**
- `src/modules/Playerbot/Group/` - All group management code
- `src/modules/Playerbot/Session/` - Session handling

---

### 6. UpdateFields Changes

**UnitData Mask Expansion:**
```cpp
// OLD
struct UnitData : HasChangesMask<224>

// NEW
struct UnitData : HasChangesMask<228>
```

**Stats Arrays Expanded:**
```cpp
// OLD (size 4)
UpdateFieldArray<int32, 4, 185, 186> Stats;
UpdateFieldArray<int32, 4, 185, 190> StatPosBuff;
UpdateFieldArray<int32, 4, 185, 194> StatNegBuff;
UpdateFieldArray<int32, 4, 185, 198> StatSupportBuff;

// NEW (size 5)
UpdateFieldArray<int32, 5, 185, 186> Stats;
UpdateFieldArray<int32, 5, 185, 191> StatPosBuff;
UpdateFieldArray<int32, 5, 185, 196> StatNegBuff;
UpdateFieldArray<int32, 5, 185, 201> StatSupportBuff;
```

**VisibleItem Expansion:**
```cpp
// OLD
struct VisibleItem : HasChangesMask<6>
    UpdateField<int32, 0, 1> ItemID;
    // ...

// NEW
struct VisibleItem : HasChangesMask<8>
    UpdateField<bool, 0, 1> Field_10;
    UpdateField<bool, 0, 2> Field_11;
    UpdateField<int32, 0, 3> ItemID;  // Index shifted!
    // ...
```

**Field Renames:**
```cpp
// OLD → NEW
Field_31C → NameplateDistanceMod
Field_320 → AutoAttackRangeMod
```

**Playerbot Files Affected:**
- Any code reading/writing UpdateFields directly
- Stat caching systems
- Visual item inspection

---

### 7. MovementInfo Changes

**New Field:**
```cpp
float gravityModifier = 1.0f;  // NEW
```

**MovementForce Renames:**
```cpp
// OLD → NEW
Unknown1110_1 → DurationMs
Unused1110 → EndTimestamp
```

**Playerbot Files Affected:**
- `src/modules/Playerbot/Movement/` - All movement code
- Flight path handling
- Jump/fall calculations

---

### 8. ObjectGuid Factory Changes

**CreatePlayer Signature Change:**
```cpp
// OLD
static ObjectGuid CreatePlayer(uint32 realmId, uint64 dbId);

// NEW
static ObjectGuid CreatePlayer(uint32 realmId, uint8 subType, uint32 arg1, uint64 dbId);
```

**Default Template Still Works:**
```cpp
// This still works (passes 0, 0 for new params):
ObjectGuid::Create<HighGuid::Player>(dbId);
```

**Playerbot Files Affected:**
- Any code calling `ObjectGuidFactory::CreatePlayer` directly
- Bot GUID generation (if using factory directly)

---

### 9. New Spell System Additions

**New SpellAttr16 Enum:**
```cpp
enum SpellAttr16 : uint32 {
    SPELL_ATTR16_UNK0 = 0x00000001,
    // ... 32 new flags
};
```

**New SpellMod:**
```cpp
// NEW
MaxTargets = 40  // Was previously MAX_SPELLMOD = 40, now 41
```

**New SpellPvpModifier Enum:**
```cpp
enum class SpellPvpModifier : uint8 {
    HealingAndDamage = 0,
    PeriodicHealingAndDamage = 1,
    BonusCoefficient = 2,
    Points = 4,
    PointsIndex0 = 5,
    // ...
};
```

**New Aura Types (646-655):**
```cpp
SPELL_AURA_ADD_FLAT_PVP_MODIFIER = 646,
SPELL_AURA_ADD_PCT_PVP_MODIFIER = 647,
SPELL_AURA_ADD_FLAT_PVP_MODIFIER_BY_SPELL_LABEL = 648,
SPELL_AURA_ADD_PCT_PVP_MODIFIER_BY_SPELL_LABEL = 649,
// ... more
SPELL_AURA_REMOVE_TRANSMOG_OUTFIT_UPDATE_COST = 655,
```

**New Spell Effects:**
```cpp
SPELL_EFFECT_CREATE_AREATRIGGER_2 = 353,
SPELL_EFFECT_SET_NEIGHBORHOOD_INITIATIVE = 354,
```

**Playerbot Files Affected:**
- `src/modules/Playerbot/Combat/Spells/SpellAnalyzer.cpp` - If analyzing spell attributes
- PvP combat calculations
- Aura handling

---

### 10. Quest System Changes

**Field Rename:**
```cpp
// In QuestObjective
int32 SecondaryAmount = 0;  // OLD
int32 ConditionalAmount = 0;  // NEW
```

**New Reward Field:**
```cpp
int32 _rewardFavor = 0;  // NEW - Housing favor system
int32 GetRewardFavor() const { return _rewardFavor; }
```

**Playerbot Files Affected:**
- `src/modules/Playerbot/Quest/` - Quest completion logic
- Quest evaluation/prioritization

---

### 11. New Map Types (Housing System)

```cpp
enum MapTypes {
    // ... existing ...
    MAP_WOWLABS = 6,           // NEW - Plunderstorm/Labs
    MAP_HOUSE_INTERIOR = 7,    // NEW - Player housing interior
    MAP_HOUSE_NEIGHBORHOOD = 8 // NEW - Housing neighborhood
};
```

**Playerbot Files Affected:**
- `src/modules/Playerbot/Navigation/` - Map type checks
- Instance detection logic
- Zone handling

---

### 12. Opcode Renumbering (CRITICAL)

**ALL opcodes have new values!** This is a complete protocol update.

Example changes (client opcodes):
```cpp
// Opcodes are now hex values like:
CMSG_ATTACK_SWING = 0x3A0124,
CMSG_ATTACK_STOP = 0x3A0125,
CMSG_CAST_SPELL = 0x3A00XX,  // All changed
```

**Playerbot Impact:**
- If playerbot builds ANY packets directly using opcode values, they MUST be updated
- All packet handlers relying on specific opcode numbers need review
- Any hardcoded opcode values are now INVALID

---

## Migration Priority Matrix

| Priority | System | Effort | Risk |
|----------|--------|--------|------|
| P0 (Critical) | Stats System (MAX_STATS) | Medium | Build Break |
| P0 (Critical) | Difficulty enum type | Low | Build Break |
| P0 (Critical) | SpellCastRequest.Misc | Low | Build Break |
| P1 (High) | UpdateFields masks | Medium | Runtime Crash |
| P1 (High) | Party packet changes | Medium | Runtime Crash |
| P1 (High) | ObjectGuid::CreatePlayer | Low | Runtime Crash |
| P2 (Medium) | MovementInfo changes | Low | Feature Break |
| P2 (Medium) | Quest field renames | Low | Feature Break |
| P3 (Low) | New spell attributes | Low | Missing Features |
| P3 (Low) | New map types | Low | Missing Features |

---

## Recommended Migration Steps

### Phase 1: Build Fixes (Immediate)
1. Update all `Stats` enum usages (MAX_STATS = 5)
2. Update `Difficulty` type from uint8 to int16
3. Update `SpellCastRequest.Misc` to `std::array<int32, 3>`
4. Update `CastItemUseSpell` signature

### Phase 2: UpdateField Compatibility
1. Review all UpdateField access code
2. Update stat array indices
3. Handle new VisibleItem field offsets

### Phase 3: Packet Structure Updates
1. Update party/group packet handling
2. Add FailedBy handling to spell failure
3. Review movement packet structures

### Phase 4: Feature Updates
1. Add Spirit stat support to gear evaluation
2. Handle new map types
3. Add new aura type handlers if needed

---

## Files to Search in Playerbot

Run these searches to find affected code:

```bash
# Stats system
grep -r "MAX_STATS\|STAT_INTELLECT\|Stats\[" src/modules/Playerbot/

# Difficulty type
grep -r "Difficulty\s" src/modules/Playerbot/

# Spell misc
grep -r "Misc\[0\]\|Misc\[1\]" src/modules/Playerbot/

# UpdateFields
grep -r "UnitData\|UpdateField" src/modules/Playerbot/

# Party packets
grep -r "PartyMemberStats\|RequestPartyMember" src/modules/Playerbot/

# ObjectGuid creation
grep -r "CreatePlayer\|ObjectGuidFactory" src/modules/Playerbot/
```

---

## Testing Requirements

After migration:
1. Verify bot stat display is correct (including Spirit if applicable)
2. Test spell casting in all scenarios
3. Test group formation and updates
4. Test dungeon/raid difficulty handling
5. Test movement in all zones including new map types
6. Verify no packet errors in server logs

---

---

## Specific Playerbot Files Requiring Updates

### Confirmed Files Needing Changes:

#### 1. SpellPacketBuilder.cpp (CRITICAL)
**Location:** `src/modules/Playerbot/Packets/SpellPacketBuilder.cpp`
**Lines:** 1046-1047, 1176-1177, 1292-1293

**Current Code:**
```cpp
buffer << int32(request.Misc[0]);
buffer << int32(request.Misc[1]);
// ...
castRequest.Misc[0] = 0;
castRequest.Misc[1] = 0;
```

**Required Change:**
```cpp
buffer << int32(request.Misc[0]);
buffer << int32(request.Misc[1]);
buffer << int32(request.Misc[2]);  // NEW - add third element
// ...
castRequest.Misc[0] = 0;
castRequest.Misc[1] = 0;
castRequest.Misc[2] = 0;  // NEW
```

#### 2. GroupCoordinator.h
**Location:** `src/modules/Playerbot/Advanced/GroupCoordinator.h:26`

**Current Code:**
```cpp
enum Difficulty : uint8;
```

**Required Change:**
```cpp
enum Difficulty : int16;
```

#### 3. CombatContextDetector (Review)
**Location:** `src/modules/Playerbot/AI/Common/CombatContextDetector.cpp:187`
**Function:** `GetInstanceDifficulty()`
- Review return type and handling of difficulty values
- May need int16 compatibility

---

## Notes

- This is WoW 12.0.0 (The War Within Season 2 or later)
- Housing system is now fully integrated
- Spirit stat return suggests class design changes
- All client compatibility requires 12.0.0 client
