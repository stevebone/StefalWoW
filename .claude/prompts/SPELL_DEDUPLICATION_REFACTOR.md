# Spell Constant Deduplication - Complete Refactoring

## Executive Summary

The codebase has **333 duplicate spell definitions** across ClassAI files. A centralized spell registry already exists (`SpellValidation_WoW112.h`) but is not consistently used. This refactoring removes local spell enums and uses the central registry.

**Expected Impact:**
- Remove ~2,000 lines of duplicate code
- Single source of truth for spell IDs
- Easier maintenance when spell IDs change

---

## Phase 1: Understand the Current State

### Central Spell Registry (KEEP THIS)
```
src/modules/Playerbot/AI/ClassAI/SpellValidation_WoW112.h
src/modules/Playerbot/AI/ClassAI/SpellValidation_WoW112_Part2.h
```

Structure:
```cpp
namespace Playerbot::WoW112Spells {
    namespace DeathKnight {
        constexpr uint32 DEATH_STRIKE = 49998;
        namespace Blood {
            constexpr uint32 MARROWREND = 195182;
        }
    }
    // ... other classes
}
```

### Files with Local Duplicates (REFACTOR THESE)

**Death Knights:**
- `AI/ClassAI/DeathKnights/BloodDeathKnight.h` - enum BloodDeathKnightSpells
- `AI/ClassAI/DeathKnights/FrostDeathKnight.h` - enum FrostDeathKnightSpells  
- `AI/ClassAI/DeathKnights/UnholyDeathKnight.h` - enum UnholyDeathKnightSpells

**Demon Hunters:**
- `AI/ClassAI/DemonHunters/HavocDemonHunter.h`
- `AI/ClassAI/DemonHunters/VengeanceDemonHunter.h`

**Druids:**
- `AI/ClassAI/Druids/BalanceDruid.h`
- `AI/ClassAI/Druids/FeralDruid.h`
- `AI/ClassAI/Druids/GuardianDruid.h`
- `AI/ClassAI/Druids/RestorationDruid.h`

**Evokers:**
- `AI/ClassAI/Evokers/AugmentationEvoker.h`
- `AI/ClassAI/Evokers/DevastationEvoker.h`
- `AI/ClassAI/Evokers/PreservationEvoker.h`

**Hunters:**
- `AI/ClassAI/Hunters/BeastMasteryHunter.h`
- `AI/ClassAI/Hunters/MarksmanshipHunter.h`
- `AI/ClassAI/Hunters/SurvivalHunter.h`

**Mages:**
- `AI/ClassAI/Mages/ArcaneMage.h`
- `AI/ClassAI/Mages/FireMage.h`
- `AI/ClassAI/Mages/FrostMage.h`

**Monks:**
- `AI/ClassAI/Monks/BrewmasterMonk.h`
- `AI/ClassAI/Monks/MistweaverMonk.h`
- `AI/ClassAI/Monks/WindwalkerMonk.h`

**Paladins:**
- `AI/ClassAI/Paladins/HolyPaladin.h`
- `AI/ClassAI/Paladins/ProtectionPaladin.h`
- `AI/ClassAI/Paladins/RetributionPaladin.h`

**Priests:**
- `AI/ClassAI/Priests/DisciplinePriest.h`
- `AI/ClassAI/Priests/HolyPriest.h`
- `AI/ClassAI/Priests/ShadowPriest.h`

**Rogues:**
- `AI/ClassAI/Rogues/AssassinationRogue.h`
- `AI/ClassAI/Rogues/OutlawRogue.h`
- `AI/ClassAI/Rogues/SubtletyRogue.h`

**Shamans:**
- `AI/ClassAI/Shamans/ElementalShaman.h`
- `AI/ClassAI/Shamans/EnhancementShaman.h`
- `AI/ClassAI/Shamans/RestorationShaman.h`

**Warlocks:**
- `AI/ClassAI/Warlocks/AfflictionWarlock.h`
- `AI/ClassAI/Warlocks/DemonologyWarlock.h`
- `AI/ClassAI/Warlocks/DestructionWarlock.h`

**Warriors:**
- `AI/ClassAI/Warriors/ArmsWarrior.h`
- `AI/ClassAI/Warriors/FuryWarrior.h`
- `AI/ClassAI/Warriors/ProtectionWarrior.h`

---

## Phase 2: Refactoring Pattern

### For Each Class File:

#### Step 1: Add Include (if not present)
```cpp
#include "../SpellValidation_WoW112.h"
```

#### Step 2: Add Using Declarations
```cpp
// At namespace level, after includes
using namespace Playerbot::WoW112Spells::DeathKnight;
using namespace Playerbot::WoW112Spells::DeathKnight::Blood;
```

#### Step 3: Remove Local Enum
Delete the entire local spell enum block:
```cpp
// DELETE THIS ENTIRE BLOCK
enum BloodDeathKnightSpells
{
    HEART_STRIKE = 206930,
    // ... all entries
};
```

#### Step 4: Handle Naming Differences
If local names differ from central names, create aliases:
```cpp
// Only if names differ
constexpr uint32 LOCAL_NAME = WoW112Spells::DeathKnight::Blood::CENTRAL_NAME;
```

#### Step 5: Compile and Fix
Build the project and fix any compilation errors:
- Missing spells in central registry → Add to SpellValidation_WoW112.h
- Name conflicts → Use explicit namespace qualification
- Type mismatches → Ensure uint32 consistency

---

## Phase 3: Execution Order

Process classes in this order (simplest first):

### Batch 1: Warriors (cleanest implementation, lowest risk)
```bash
# Start with Warriors - they have the cleanest code
1. ArmsWarrior.h
2. FuryWarrior.h
3. ProtectionWarrior.h
# Compile and test after each
```

### Batch 2: Mages (simple, well-structured)
```bash
4. ArcaneMage.h
5. FireMage.h
6. FrostMage.h
```

### Batch 3: Paladins (good template adoption)
```bash
7. HolyPaladin.h
8. ProtectionPaladin.h
9. RetributionPaladin.h
```

### Batch 4: Death Knights (well-documented)
```bash
10. BloodDeathKnight.h
11. FrostDeathKnight.h
12. UnholyDeathKnight.h
```

### Batch 5: Monks
```bash
13. BrewmasterMonk.h
14. MistweaverMonk.h
15. WindwalkerMonk.h
```

### Batch 6: Demon Hunters
```bash
16. HavocDemonHunter.h
17. VengeanceDemonHunter.h
```

### Batch 7: Hunters
```bash
18. BeastMasteryHunter.h
19. MarksmanshipHunter.h
20. SurvivalHunter.h
```

### Batch 8: Warlocks
```bash
21. AfflictionWarlock.h
22. DemonologyWarlock.h
23. DestructionWarlock.h
```

### Batch 9: Druids
```bash
24. BalanceDruid.h
25. FeralDruid.h
26. GuardianDruid.h
27. RestorationDruid.h
```

### Batch 10: Evokers
```bash
28. AugmentationEvoker.h
29. DevastationEvoker.h
30. PreservationEvoker.h
```

### Batch 11: Shamans (no utilities, may need more attention)
```bash
31. ElementalShaman.h
32. EnhancementShaman.h
33. RestorationShaman.h
```

### Batch 12: Rogues (highest complexity, save for last)
```bash
34. AssassinationRogue.h
35. OutlawRogue.h
36. SubtletyRogue.h
```

### Batch 13: Priests (legacy, most complex)
```bash
37. DisciplinePriest.h
38. HolyPriest.h
39. ShadowPriest.h
```

---

## Phase 4: Handling Missing Spells

When you encounter spells in local enums that don't exist in SpellValidation_WoW112.h:

### Option A: Add to Central Registry (Preferred)
```cpp
// In SpellValidation_WoW112.h, find the right namespace and add:
namespace Warrior::Arms
{
    constexpr uint32 NEW_SPELL_NAME = 123456;
}
```

### Option B: Keep Local Definition (Last Resort)
```cpp
// Only for truly spec-specific spells not used elsewhere
namespace LocalSpells
{
    constexpr uint32 VERY_SPECIFIC_SPELL = 123456;
}
```

---

## Phase 5: Verification

### After Each File:
```bash
# Compile
cmake --build build --target Playerbot

# If tests exist:
ctest -R ClassAI
```

### After Each Batch:
```bash
# Full rebuild
cmake --build build --clean-first

# Run the server briefly to check for runtime errors
```

### Final Verification:
```bash
# Search for remaining local spell enums
grep -r "enum.*Spells" src/modules/Playerbot/AI/ClassAI/

# Should only find SpellValidation files, not individual class files
```

---

## Constraints

1. **DO NOT** change any spell ID values
2. **DO NOT** change any game logic
3. **DO NOT** rename spells in the central registry (other code may depend on names)
4. **DO** compile after each file change
5. **DO** commit after each successful batch
6. **DO** add missing spells to central registry rather than keeping local copies

---

## Git Workflow

```bash
# Create feature branch
git checkout -b refactor/spell-deduplication

# After each batch:
git add -A
git commit -m "refactor(ClassAI): deduplicate spells for [ClassName]"

# After all complete:
git push origin refactor/spell-deduplication
```

---

## Example: Complete BloodDeathKnight.h Refactoring

### Before:
```cpp
#pragma once

#include "../Common/StatusEffectTracker.h"
// ... other includes

namespace Playerbot
{

enum BloodDeathKnightSpells
{
    HEART_STRIKE             = 206930,
    BLOOD_BOIL               = 50842,
    MARROWREND               = 195182,
    // ... 30+ more entries
};

class BloodDeathKnight : public TankSpecialization<RuneRunicPowerResource>
{
    // Uses HEART_STRIKE, BLOOD_BOIL, etc.
};

}
```

### After:
```cpp
#pragma once

#include "../Common/StatusEffectTracker.h"
#include "../SpellValidation_WoW112.h"  // ADD THIS
// ... other includes

namespace Playerbot
{

// USE CENTRAL REGISTRY
using namespace WoW112Spells::DeathKnight;
using namespace WoW112Spells::DeathKnight::Blood;

// ENUM REMOVED - was here, now deleted

class BloodDeathKnight : public TankSpecialization<RuneRunicPowerResource>
{
    // Still uses HEART_STRIKE, BLOOD_BOIL, etc. - now from central registry
};

}
```

---

## Estimated Time

- Per file: 5-10 minutes (with compile)
- Per batch: 30-45 minutes
- Total: 4-6 hours

---

## Success Criteria

1. ✅ All 39 spec files use SpellValidation_WoW112.h
2. ✅ No local spell enums remain in spec files
3. ✅ Project compiles without errors
4. ✅ All tests pass (if available)
5. ✅ No functional changes to bot behavior
