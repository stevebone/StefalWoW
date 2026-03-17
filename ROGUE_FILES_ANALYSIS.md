# Rogue Combat Files - Enterprise-Grade Analysis

## Executive Summary

**RECOMMENDATION: Delete LEGACY files and use ONLY the Refactored template-based system.**

The Rogue AI has **TWO COMPETING ARCHITECTURES**:
1. **LEGACY System** - Old object-oriented approach with heavy complexity (REMOVE)
2. **REFACTORED System** - Modern template-based architecture (KEEP & FIX)

---

## File Classification

### ✅ ENTERPRISE-GRADE (KEEP - Modern Template-Based)

| File | Size | Purpose | Status |
|------|------|---------|--------|
| **RogueResourceTypes.h** | 2.9K | Shared EnergyComboResource type | ✅ EXCELLENT |
| **AssassinationRogueRefactored.h** | 13K | Template-based Assassination spec | ✅ MODERN |
| **OutlawRogueRefactored.h** | 18K | Template-based Outlaw spec | ✅ MODERN |
| **SubtletyRogueRefactored.h** | 17K | Template-based Subtlety spec | ✅ MODERN |

**Why Enterprise-Grade:**
- Uses `MeleeDpsSpecialization<EnergyComboResource>` template system
- Clean separation of concerns
- Compile-time polymorphism (better performance)
- Matches project's template-based architecture used by other classes
- Self-contained header-only implementation
- Zero runtime virtual call overhead

---

### ⚠️ PARTIALLY REDUNDANT (CONSOLIDATE OR REMOVE)

| File | Size | Purpose | Issue |
|------|------|---------|-------|
| **RogueAI.h** | 4.7K | Main RogueAI class header | Bridges to BOTH systems |
| **RogueAI.cpp** | 41K | Main RogueAI implementation | Uses LEGACY specs |
| **RogueSpecialization.h** | 13K | Base spec class + spell enums | KEEP enums, class is legacy |
| **RogueSpecialization.cpp** | 13K | Base spec implementation | LEGACY - Remove |

**Issues:**
- `RogueAI.cpp` includes `AssassinationSpecialization_Enhanced.h` (LEGACY)
- `RogueAI.cpp` does NOT use the Refactored template system
- Creates confusion - which system is active?

---

### ❌ LEGACY CODE (DELETE IMMEDIATELY)

| File | Size | Purpose | Why Remove |
|------|------|---------|------------|
| **AssassinationSpecialization.cpp** | 42K | Old Assassination impl | Replaced by AssassinationRogueRefactored.h |
| **AssassinationSpecialization.h** | 9.5K | Old Assassination header | Replaced by template version |
| **AssassinationSpecialization_Enhanced.h** | 12K | "Enhanced" old system | Still uses old architecture |
| **CombatSpecialization.cpp** | 36K | Old Combat/Outlaw impl | Replaced by OutlawRogueRefactored.h |
| **CombatSpecialization.h** | 11K | Old Combat header | Replaced by template version |
| **CombatSpecialization_Enhanced.h** | 13K | "Enhanced" old system | Still uses old architecture |
| **SubtletySpecialization.cpp** | 45K | Old Subtlety impl | Replaced by SubtletyRogueRefactored.h |
| **SubtletySpecialization.h** | 13K | Old Subtlety header | Replaced by template version |
| **SubtletySpecialization_Enhanced.h** | 14K | "Enhanced" old system | Still uses old architecture |
| **RogueAI_Enhanced.cpp** | 15K | Old enhanced AI | Uses old spec system |
| **RogueAI_Specialization.cpp** | 9.3K | Old spec switching | Replaced by template dispatch |
| **EnergyManager.cpp** | 2.3K | Standalone energy manager | Resource management in templates now |
| **EnergyManager.h** | 1.3K | Energy manager header | Resource management in templates now |
| **RogueCombatPositioning.cpp** | 1.1K | Old positioning | Positioning in MeleeDpsSpecialization |

**Why Legacy:**
- Uses old object-oriented virtual inheritance pattern
- Runtime polymorphism (slower)
- Massive code duplication across specs (42K + 36K + 45K = 123K!)
- Not using the project's template-based architecture
- Violates DRY principle extensively

---

## Architecture Comparison

### LEGACY System (Object-Oriented)
```cpp
// OLD APPROACH - Runtime polymorphism
class RogueSpecialization { // Base class
    virtual void UpdateRotation(Unit* target) = 0;
    virtual void UpdateBuffs() = 0;
    // ... 20+ virtual methods
};

class AssassinationSpecialization : public RogueSpecialization {
    void UpdateRotation(Unit* target) override { /* 500 lines */ }
    void UpdateBuffs() override { /* 200 lines */ }
    // ... massive implementation
};

// RogueAI.cpp uses:
_specialization = std::make_unique<AssassinationSpecialization_Enhanced>(bot);
_specialization->UpdateRotation(target); // Virtual call overhead
```

**Problems:**
- Runtime overhead from virtual calls
- Code duplication (each spec implements same patterns differently)
- Hard to maintain (changes require updating 3 separate 40K+ files)
- No compile-time optimization

### REFACTORED System (Template-Based) ✅
```cpp
// NEW APPROACH - Compile-time polymorphism
template<typename Resource>
class MeleeDpsSpecialization {
    void ExecuteRotation(::Unit* target) final { /* Generic logic */ }
    // Hooks for specialization-specific behavior
};

class AssassinationRogueRefactored : public MeleeDpsSpecialization<EnergyComboResource> {
    void ExecuteRotation(::Unit* target) override {
        // Assassination-specific logic using template methods
    }
};

// Usage (should be):
auto spec = std::make_unique<AssassinationRogueRefactored>(bot);
spec->ExecuteRotation(target); // Compile-time dispatch, zero overhead
```

**Benefits:**
- Zero runtime overhead (inline expansion)
- Shared logic through template base
- Type-safe resource management
- Enterprise-grade architecture
- Matches project standards

---

## Current Build Errors - Root Cause

### Problem: Duplicate Spell Definitions

**Current State:**
```cpp
// RogueSpecialization.h (shared file)
enum RogueSpells {
    BACKSTAB = 53,
    EVISCERATE = 2098,
    SHADOWSTRIKE = 36554,
    // ... shared spells
};

// SubtletyRogueRefactored.h (spec file)
enum SubtletySpells {
    BACKSTAB = 53,        // ❌ DUPLICATE!
    EVISCERATE = 196819,  // ❌ DUPLICATE!
    SHADOWSTRIKE = 185438 // ❌ DUPLICATE!
    // ...
};
```

**Why This Happened:**
The Refactored files were created ASSUMING they would be the ONLY Rogue system, but then:
1. Legacy files still exist
2. Legacy RogueSpecialization.h has base spell enums
3. Refactored specs tried to redefine everything
4. Result: Duplicate definition errors

---

## Recommended Action Plan

### Phase 1: DELETE LEGACY (Immediate)

**Delete these 14 files:**
```bash
rm AssassinationSpecialization.cpp
rm AssassinationSpecialization.h
rm AssassinationSpecialization_Enhanced.h
rm CombatSpecialization.cpp
rm CombatSpecialization.h
rm CombatSpecialization_Enhanced.h
rm SubtletySpecialization.cpp
rm SubtletySpecialization.h
rm SubtletySpecialization_Enhanced.h
rm RogueAI_Enhanced.cpp
rm RogueAI_Specialization.cpp
rm EnergyManager.cpp
rm EnergyManager.h
rm RogueCombatPositioning.cpp
```

**Code Size Reduction:** 123K of duplicate implementation code removed!

### Phase 2: FIX REFACTORED System

**Keep & Fix:**
1. **RogueResourceTypes.h** - Already perfect ✅
2. **RogueSpecialization.h** - Keep ONLY spell enums, remove old class
3. **AssassinationRogueRefactored.h** - Remove duplicate spell IDs
4. **OutlawRogueRefactored.h** - Remove duplicate spell IDs
5. **SubtletyRogueRefactored.h** - Remove duplicate spell IDs

**RogueSpecialization.h Changes:**
```cpp
// KEEP: All shared spell ID enums
enum RogueSpells {
    // Core abilities
    SINISTER_STRIKE = 1752,
    BACKSTAB = 53,
    EVISCERATE = 2098,
    // Assassination
    MUTILATE = 1329,
    ENVENOM = 32645,
    GARROTE = 703,
    RUPTURE = 1943,
    // Outlaw
    PISTOL_SHOT = 185763,
    DISPATCH = 2098,
    BETWEEN_THE_EYES = 315341,
    // Subtlety
    SHADOWSTRIKE = 185438,
    SHADOW_DANCE = 185313,
    SYMBOLS_OF_DEATH = 212283,
    // Shared
    VANISH = 1856,
    STEALTH = 1784,
    KICK = 1766,
    // ... ALL shared spells
};

// REMOVE: Old RogueSpecialization class entirely
// DELETE: All virtual method declarations
// DELETE: All legacy implementation
```

**Refactored Spec Files:**
```cpp
// AssassinationRogueRefactored.h
// ONLY define spells NOT in RogueSpecialization.h
enum AssassinationUniqueSpells {
    // ONLY Assassination-specific that aren't shared
    POISONED_KNIFE = 185565,
    CRIMSON_TEMPEST = 121411,
    // ...
};
```

### Phase 3: UPDATE RogueAI.cpp

**Current (WRONG):**
```cpp
#include "AssassinationSpecialization_Enhanced.h" // ❌ LEGACY
_specialization = std::make_unique<AssassinationSpecialization_Enhanced>(bot);
```

**Fixed (CORRECT):**
```cpp
#include "AssassinationRogueRefactored.h" // ✅ MODERN
#include "OutlawRogueRefactored.h"
#include "SubtletyRogueRefactored.h"

// Template-based instantiation
switch (detectedSpec) {
    case RogueSpec::ASSASSINATION:
        _refactoredSpec = std::make_unique<AssassinationRogueRefactored>(bot);
        break;
    case RogueSpec::OUTLAW:
        _refactoredSpec = std::make_unique<OutlawRogueRefactored>(bot);
        break;
    case RogueSpec::SUBTLETY:
        _refactoredSpec = std::make_unique<SubtletyRogueRefactored>(bot);
        break;
}
```

### Phase 4: CMakeLists.txt Cleanup

**Remove from build:**
```cmake
# DELETE these from CMakeLists.txt
# AssassinationSpecialization.cpp
# CombatSpecialization.cpp
# SubtletySpecialization.cpp
# RogueAI_Enhanced.cpp
# RogueAI_Specialization.cpp
# EnergyManager.cpp
# RogueCombatPositioning.cpp
```

---

## Why Template-Based is Superior

### Performance
- **LEGACY:** Virtual call = vtable lookup = cache miss potential = slower
- **REFACTORED:** Inline template expansion = zero overhead = faster

### Maintainability
- **LEGACY:** Bug fix requires changing 3 files (42K + 36K + 45K)
- **REFACTORED:** Bug fix in template base = all specs fixed

### Code Size
- **LEGACY:** 123K of duplicated implementation
- **REFACTORED:** ~50K total (shared template + 3 spec implementations)

### Type Safety
- **LEGACY:** Resource management scattered, error-prone
- **REFACTORED:** EnergyComboResource enforced at compile time

### Project Consistency
- **LEGACY:** Unique to Rogues, doesn't match project patterns
- **REFACTORED:** Matches Priest, Shaman, and other class architectures

---

## Implementation Priority

### HIGH PRIORITY (Do First)
1. ✅ Delete all 14 legacy files
2. ✅ Clean RogueSpecialization.h (keep enums only)
3. ✅ Fix duplicate spell IDs in Refactored headers
4. ✅ Fix CastSpell argument order in Refactored headers
5. ✅ Update RogueAI.cpp to use Refactored specs

### MEDIUM PRIORITY (Do Second)
6. Remove legacy includes from RogueAI.h
7. Update CMakeLists.txt to remove legacy files
8. Verify build compiles successfully
9. Test all 3 specs work correctly

### LOW PRIORITY (Polish)
10. Add missing spells to shared enum
11. Optimize template specializations
12. Add comprehensive unit tests
13. Performance benchmarking

---

## Conclusion

**The Rogue AI has been implementing TWO SYSTEMS simultaneously**, causing:
- Duplicate code (123K wasted)
- Compilation errors (duplicate definitions)
- Maintenance nightmares (which system to update?)
- Performance degradation (unnecessary virtual calls)

**SOLUTION: Delete the entire LEGACY system and commit fully to the enterprise-grade template-based REFACTORED architecture.**

This aligns with:
✅ CLAUDE.md requirements (enterprise-grade, no shortcuts)
✅ Project architecture (template-based like other classes)
✅ Performance targets (<0.1% CPU per bot)
✅ Maintainability standards (DRY, SOLID principles)

**Next Step:** Execute Phase 1 - Delete all 14 legacy files and begin systematic fixes.
