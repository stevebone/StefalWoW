# WoW 12.0 Migration Analysis - Playerbot Module

**Date**: 2026-02-03
**TrinityCore Commit**: b0a596908d5c1b5b09f90e97b17a7fc785e5366f
**Migration Status**: COMPLETED (Phase 1-3)

## Migration Completed - Summary

| Phase | Status | Commits |
|-------|--------|---------|
| Difficulty Type (uint8→int16) | ✅ DONE | d641facf2f |
| Stats/Spirit (MAX_STATS=5) | ✅ DONE | d641facf2f |
| File Renames (WoW112→WoW120) | ✅ DONE | a588cc2639 |
| Namespace Updates | ✅ DONE | a588cc2639 |
| Comment Updates (11.2→12.0) | ✅ DONE | a588cc2639 |
| Item Squish | ✅ VERIFIED | N/A - Handled by TrinityCore |
| SpellAttr16 | ⏳ OPTIONAL | Future enhancement |
| Map Type Checks | ⏳ OPTIONAL | Future enhancement |

## Executive Summary

The TrinityCore project has been updated to WoW 12.0 (The War Within Season 2+). This document analyzes all changes affecting the Playerbot module and provides a systematic migration plan.

---

## Critical API Changes in 12.0

### 1. Stats System Changes

**BREAKING CHANGE**: `STAT_SPIRIT` re-added, `MAX_STATS` increased from 4 to 5

```cpp
// OLD (11.x)
enum Stats : uint16
{
    STAT_STRENGTH   = 0,
    STAT_AGILITY    = 1,
    STAT_STAMINA    = 2,
    STAT_INTELLECT  = 3,
};
#define MAX_STATS 4

// NEW (12.0)
enum Stats : uint16
{
    STAT_STRENGTH   = 0,
    STAT_AGILITY    = 1,
    STAT_STAMINA    = 2,
    STAT_INTELLECT  = 3,
    STAT_SPIRIT     = 4,  // RE-ADDED
};
#define MAX_STATS 5
```

**Playerbot Impact**:
- Stats array iterations must loop to `MAX_STATS` (5 now)
- Healer/caster calculations may use Spirit for mana regen
- Already partially implemented in:
  - `LFGRoleDetector.cpp:265` - Spirit used in healer score
  - `DoubleBufferedSpatialGrid.cpp:704` - Spirit snapshot
  - `RoleAssignment.cpp:670` - Spirit in stat calculation

### 2. Difficulty Type Change

**BREAKING CHANGE**: `Difficulty` enum type changed from `uint8` to `int16`

```cpp
// OLD (11.x)
enum Difficulty : uint8

// NEW (12.0)
enum Difficulty : int16
```

**Playerbot Impact**:
- `InstanceDifficulty` enum needs update to `int16`
- `RaidDifficulty` enum needs update to `int16`
- `DungeonDifficulty` enum needs update to `int16`
- `GroupEvent::DifficultyChanged()` parameter type change
- All casts to `uint8` for difficulty need review

**Files Affected**:
- `src/modules/Playerbot/Dungeon/InstanceFarmingManager.h:61`
- `src/modules/Playerbot/AI/Coordination/Raid/RaidState.h:81`
- `src/modules/Playerbot/AI/Coordination/Dungeon/DungeonState.h:42`
- `src/modules/Playerbot/Group/GroupEvents.h:140`
- `src/modules/Playerbot/Group/GroupEvents.cpp:196`
- `src/modules/Playerbot/Group/PlayerbotGroupScript.cpp:400-402`

### 3. Spell System Changes

**NEW**: `SpellAttr16` attribute set added

```cpp
// NEW (12.0)
enum SpellAttr16 : uint32
{
    SPELL_ATTR16_UNK0 = 0x00000001,
    // ... 32 flags
    SPELL_ATTR16_UNK31 = 0x80000000
};

// SpellInfo now has:
uint32 AttributesEx16 = 0;
bool HasAttribute(SpellAttr16 attribute) const;
```

**NEW**: `SpellModOp::MaxTargets = 40`, `MAX_SPELLMOD` increased to 41

**NEW**: `SpellPvpModifier` enum for PvP spell modifications

**Playerbot Impact**:
- Spell validation code may need updates
- PvP spell handling for BG bots should use new modifiers
- Combat calculations should check AttributesEx16

### 4. Item Level System Changes

**CRITICAL**: Item Squish Era system added for cross-expansion item level normalization

```cpp
// BonusData changes
struct BonusData {
    uint32 ItemSquishEraID;  // NEW
    bool IgnoreSquish;       // NEW
    // ...
};

// ItemTemplate changes
uint32 GetItemSquishEraId() const;

// NEW bonus type
ITEM_BONUS_ITEM_BONUS_LIST = 50
```

**Playerbot Impact**:
- GearScore calculations must account for squish eras
- Item level comparisons need era-aware logic
- Bot equipment decisions need squish awareness
- Template repository item levels need validation

**Files Requiring Updates** (70 files use item level):
- `Equipment/EquipmentManager.cpp` - Core gear management
- `Equipment/BotGearFactory.cpp` - Gear creation
- `Lifecycle/Instance/BotTemplateRepository.cpp` - Template item levels
- `LFG/LFGBotSelector.cpp` - LFG matching by item level
- `Group/RoleAssignment.cpp` - Role-based gear checks

### 5. UpdateFields Changes

**CRITICAL**: Stats array size in UnitData changed from 4 to 5

```cpp
// OLD (11.x)
UpdateFieldArray<int32, 4, 185, 186> Stats;
UpdateFieldArray<int32, 4, 185, 190> StatPosBuff;
UpdateFieldArray<int32, 4, 185, 194> StatNegBuff;
UpdateFieldArray<int32, 4, 185, 198> StatSupportBuff;

// NEW (12.0)
UpdateFieldArray<int32, 5, 185, 186> Stats;
UpdateFieldArray<int32, 5, 185, 191> StatPosBuff;
UpdateFieldArray<int32, 5, 185, 196> StatNegBuff;
UpdateFieldArray<int32, 5, 185, 201> StatSupportBuff;
```

**NEW**: `VisibleItem` has 2 new bool fields

**NEW**: `LeaverInfo` structure changes:
- `LeaverStatus` → `Flags`
- `IsLeaver` bool added

### 6. Map Type Changes

**NEW**: 3 new map types added

```cpp
// NEW (12.0)
MAP_WOWLABS           = 6,  // Plunderstorm/WoW Labs
MAP_HOUSE_INTERIOR    = 7,  // Player housing interior
MAP_HOUSE_NEIGHBORHOOD = 8  // Player housing neighborhood
```

**Playerbot Impact**:
- Bot teleportation/map checks need updates
- Housing maps should be excluded from bot spawning
- IsValidBotMap() checks need new map types

### 7. New Criteria Types

**CriteriaType::Count** increased from 279 to 283 (4 new criteria added)

### 8. New GameRules

```cpp
// NEW (12.0)
EjJourneysDisabled = 156,
PvPInitialRatingOverride = 190,
```

### 9. New GlobalCurves for Housing

```cpp
// NEW (12.0)
HouseLevelFavorForLevel = 37,
HouseInteriorDecorBudget = 38,
HouseExteriorDecorBudget = 39,
HouseRoomPlacementBudget = 40,
HouseFixtureBudget = 41,
TransmogCost = 43,
MaxHouseSizeForLevel = 46,
```

### 10. New Spell Effects

```cpp
// NEW (12.0)
SPELL_EFFECT_CREATE_AREATRIGGER_2 = 353,
SPELL_EFFECT_SET_NEIGHBORHOOD_INITIATIVE = 354,
```

### 11. New SpellCastResult

```cpp
// NEW (12.0)
SPELL_FAILED_TRANSMOG_OUTFIT_ALREADY_KNOWN = 323,
SPELL_FAILED_UNKNOWN = 324,  // Shifted from 323
```

---

## Documentation Updates Required

### Files Referencing "11.x" That Need Version Updates

1. **AUTOMATED_WORLD_POPULATION_DESIGN.md** - Multiple 11.2 references
2. **WoW112CharacterCreation.h** - Entire file named for 11.2
3. **docs/TROUBLESHOOTING_FAQ.md** - Version references
4. **docs/DEPLOYMENT_GUIDE.md** - Version references
5. **docs/CONFIGURATION_REFERENCE.md** - Version references
6. **docs/ARCHITECTURE_OVERVIEW.md** - Version references
7. **Chat/BotChatCommandHandler.cpp:240** - Comment reference
8. **Banking/BankingManager.cpp:842,847** - Comment references
9. **Achievements/AchievementManager.cpp:99** - Comment reference
10. **CMakeLists.txt:787** - Comment reference
11. **Character/ZoneLevelHelper.h:27** - Comment reference

---

## Migration Tasks

### Phase 1: Critical Breaking Changes (IMMEDIATE)

| Task | Priority | Files | Complexity |
|------|----------|-------|------------|
| Update Difficulty enum type to int16 | HIGH | 6 files | LOW |
| Update Stats array handling (MAX_STATS=5) | HIGH | ~10 files | MEDIUM |
| Validate item level squish handling | HIGH | 70 files | HIGH |

### Phase 2: Feature Parity Updates

| Task | Priority | Files | Complexity |
|------|----------|-------|------------|
| Add SpellAttr16 support in spell checks | MEDIUM | 5-10 files | LOW |
| Update map type validations | MEDIUM | 3-5 files | LOW |
| Add new SpellModOp::MaxTargets handling | LOW | 2-3 files | LOW |

### Phase 3: Documentation Updates

| Task | Priority | Files | Complexity |
|------|----------|-------|------------|
| Rename WoW112*.h → WoW120*.h | HIGH | 2 files | LOW |
| Update all "11.x" references to "12.0" | MEDIUM | 15+ files | LOW |
| Update version numbers in configs | LOW | 3 files | LOW |

---

## Code Audit Checklist

### Stats System
- [x] All `MAX_STATS` iterations include index 4 (Spirit)
- [x] Healer/caster stat calculations use Spirit
- [x] Stat snapshot code captures Spirit
- [x] No hardcoded "4" for stats array size
- [x] WoW120CharacterCreation.h updated with Spirit values

### Difficulty System
- [x] All custom Difficulty enums use `int16`
- [x] All difficulty casts use `static_cast<int16>`
- [x] GroupEvent difficulty parameter updated
- [x] No truncation warnings

### Item Level System
- [x] GearScore accounts for ItemSquishEraID (handled by TrinityCore Item::GetItemLevel())
- [x] Item comparisons are era-aware (TrinityCore handles transparently)
- [x] Bot equipment manager handles squish (uses Item API correctly)
- [x] Template item levels validated for 12.0

### Spell System
- [ ] AttributesEx16 checks where appropriate (OPTIONAL - future enhancement)
- [ ] SpellPvpModifier usage for PvP bots (OPTIONAL - future enhancement)
- [x] New spell effects handled gracefully (no breaking changes)

### Map System
- [ ] Housing maps excluded from bot spawning (OPTIONAL - future enhancement)
- [ ] WowLabs maps handled appropriately (OPTIONAL - future enhancement)
- [x] Map type checks include new types (no breaking changes)

### File/Namespace Updates
- [x] WoW112CharacterCreation.h → WoW120CharacterCreation.h
- [x] ResourceTypes_WoW112.h → ResourceTypes_WoW120.h
- [x] SpellValidation_WoW112.h → SpellValidation_WoW120.h
- [x] SpellValidation_WoW112_Part2.h → SpellValidation_WoW120_Part2.h
- [x] CombatSpecializationTemplate_WoW112.h → WoW120 variant
- [x] WoW112Spells namespace → WoW120Spells namespace
- [x] All include paths updated
- [x] All "11.2" comments updated to "12.0"

---

## Testing Requirements

### Unit Tests
- [ ] Stats iteration tests with Spirit
- [ ] Item level squish calculation tests
- [ ] Difficulty type conversion tests
- [ ] Map type validation tests

### Integration Tests
- [ ] Bot spawning on all map types
- [ ] Item equipping with squish-era items
- [ ] Group difficulty changes
- [ ] PvP combat with new modifiers

### Regression Tests
- [ ] Existing bot behavior unchanged
- [ ] Performance metrics maintained
- [ ] No new crashes or errors

---

## Implementation Order

1. **Difficulty Type Migration** (LOW RISK)
   - Update enums to int16
   - Update function signatures
   - Update casts
   - Compile and test

2. **Stats System Update** (MEDIUM RISK)
   - Audit all MAX_STATS usage
   - Update iterations
   - Verify Spirit handling
   - Test stat calculations

3. **Item Level Squish** (HIGH RISK)
   - Research squish curve behavior
   - Update GearScore calculations
   - Update item comparisons
   - Extensive testing required

4. **Documentation Updates** (NO RISK)
   - Rename files
   - Update version references
   - Update comments

---

## Rollback Plan

If migration causes issues:
1. Revert Playerbot module changes
2. Core TrinityCore 12.0 is compatible with 11.x Playerbot data
3. Document incompatibilities for future fix

---

## References

- TrinityCore 12.0 Commit: `b0a596908d5c1b5b09f90e97b17a7fc785e5366f`
- Files Changed: 92 files, +8452/-5638 lines
- Key Files:
  - `src/server/game/DataStores/DBCEnums.h`
  - `src/server/game/Miscellaneous/SharedDefines.h`
  - `src/server/game/Entities/Item/Item.cpp`
  - `src/server/game/Entities/Object/Updates/UpdateFields.h`
