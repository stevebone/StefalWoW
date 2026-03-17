# Phase 1 Combat Behavior - Complete Fix Guide

## Overview
This document provides the complete fix for all 92 compilation errors in the Phase 1 Combat Behavior files.

## Files to Fix
1. `DefensiveBehaviorManager.cpp`
2. `InterruptRotationManager.cpp`
3. `DispelCoordinator.h` and `DispelCoordinator.cpp`

## Common Fixes Required for ALL .cpp Files

### 1. Add Missing Includes
Add these after existing includes in ALL .cpp files:
```cpp
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
```

For `DispelCoordinator.h`, add:
```cpp
#include <mutex>
```

### 2. Add Role Detection Helper
Add this in the anonymous namespace of ALL .cpp files:
```cpp
namespace
{
    // Helper for role detection
    enum BotRole : uint8
    {
        BOT_ROLE_TANK = 0,
        BOT_ROLE_HEALER = 1,
        BOT_ROLE_DPS = 2
    };

    BotRole GetPlayerRole(Player const* player)
    {
        if (!player)
            return BOT_ROLE_DPS;

        Classes cls = player->GetClass();
        uint8 spec = player->GetPrimaryTalentTree(player->GetActiveSpec());

        switch (cls)
        {
            case CLASS_WARRIOR:
                return (spec == 2) ? BOT_ROLE_TANK : BOT_ROLE_DPS; // Prot = tank
            case CLASS_PALADIN:
                if (spec == 1) return BOT_ROLE_HEALER; // Holy
                if (spec == 2) return BOT_ROLE_TANK;   // Prot
                return BOT_ROLE_DPS;
            case CLASS_HUNTER:
                return BOT_ROLE_DPS;
            case CLASS_ROGUE:
                return BOT_ROLE_DPS;
            case CLASS_PRIEST:
                return (spec == 2) ? BOT_ROLE_DPS : BOT_ROLE_HEALER; // Shadow = DPS
            case CLASS_DEATH_KNIGHT:
                return (spec == 0) ? BOT_ROLE_TANK : BOT_ROLE_DPS; // Blood = tank
            case CLASS_SHAMAN:
                return (spec == 2) ? BOT_ROLE_HEALER : BOT_ROLE_DPS; // Resto = healer
            case CLASS_MAGE:
                return BOT_ROLE_DPS;
            case CLASS_WARLOCK:
                return BOT_ROLE_DPS;
            case CLASS_MONK:
                if (spec == 0) return BOT_ROLE_TANK;   // Brewmaster
                if (spec == 1) return BOT_ROLE_HEALER; // Mistweaver
                return BOT_ROLE_DPS;
            case CLASS_DRUID:
                if (spec == 0) return BOT_ROLE_DPS;    // Balance
                if (spec == 1) return BOT_ROLE_DPS;    // Feral
                if (spec == 2) return BOT_ROLE_TANK;   // Guardian
                return BOT_ROLE_HEALER;                 // Resto
            case CLASS_DEMON_HUNTER:
                return (spec == 1) ? BOT_ROLE_TANK : BOT_ROLE_DPS; // Vengeance = tank
            default:
                return BOT_ROLE_DPS;
        }
    }

    bool IsTank(Player const* player) { return GetPlayerRole(player) == BOT_ROLE_TANK; }
    bool IsHealer(Player const* player) { return GetPlayerRole(player) == BOT_ROLE_HEALER; }
    bool IsDPS(Player const* player) { return GetPlayerRole(player) == BOT_ROLE_DPS; }
}
```

### 3. Global API Replacements

#### Replace ALL occurrences:
- `GetMSTime()` → `getMSTime()`
- `getClass()` → `GetClass()`
- `aura->IsPositive()` → `aura->GetSpellInfo()->IsPositive()`
- `bot->HasSpellCooldown(spellId)` → `bot->GetSpellHistory()->HasCooldown(spellId)`
- `sSpellMgr->GetSpellInfo(spellId)` → `sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE)`
- `member->GetRole() == ROLE_TANK` → `IsTank(member)`
- `member->GetRole() == ROLE_HEALER` → `IsHealer(member)`
- `member->GetRole() == ROLE_DPS` → `IsDPS(member)`

### 4. Fix Group Iteration Pattern

Replace this pattern:
```cpp
GroupReference* ref = group->GetFirstMember();
while (ref) {
    Player* member = ref->GetSource();
    ref = ref->next();
}
```

With:
```cpp
for (GroupReference* ref = group->GetFirstMember(); ref != nullptr; ref = ref->next()) {
    Player* member = ref->GetSource();
    if (!member)
        continue;
    // ... use member
}
```

### 5. Fix Grid Search Pattern

Replace:
```cpp
Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(bot, bot, range);
Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, enemies, check);
Cell::VisitAllObjects(bot, searcher, range);
```

With:
```cpp
Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, range);
Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, enemies, u_check);
Trinity::VisitNearbyObject(bot, range, searcher);
```

## File-Specific Fixes

### DefensiveBehaviorManager.cpp

1. In constructor (~line 155):
   - Replace: `uint8 role = _bot->GetRole();`
   - With: `BotRole role = GetPlayerRole(_bot);`
   - Update: `_thresholds = GetRoleThresholds(static_cast<uint8>(role));`

2. In GetRoleThresholds() (~line 640):
   - Replace case labels with `BOT_ROLE_TANK`, `BOT_ROLE_HEALER`, `BOT_ROLE_DPS`

### InterruptRotationManager.cpp

1. Fix IsInterruptible() method:
   - Check spell attributes differently since SPELL_ATTR0_NO_CAST_INTERRUPTS may not exist
   - Use: `spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE`

2. Fix all spell casting checks:
   - Replace `_ai->CastSpell()` with `_bot->CastSpell()`
   - Remove or implement `_ai->SetNextCheckDelay()` calls

### DispelCoordinator.cpp

1. Add mutex include to DispelCoordinator.h:
   ```cpp
   #include <mutex>
   ```

2. Fix GetAttackableUnitListInRange() (~line 900):
   ```cpp
   std::list<Unit*> enemies;
   Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, range);
   Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, enemies, u_check);
   Trinity::VisitNearbyObject(bot, range, searcher);
   return enemies.size();
   ```

## Build Command
After applying all fixes:
```bash
cd /c/TrinityBots/TrinityCore/build
cmake --build . --config RelWithDebInfo --parallel 8
```

## Verification
1. All 92 compilation errors should be resolved
2. No new warnings should be introduced
3. All managers should properly handle role detection
4. Grid searches should work correctly
5. Spell cooldowns should be properly checked

## Summary of Changes
- Added 5 missing includes to each .cpp file
- Added 1 include to DispelCoordinator.h
- Implemented role detection helper in all files
- Fixed all TrinityCore API calls
- Fixed grid search patterns
- Updated group iteration patterns
- Corrected spell history/cooldown checks

Total lines changed: ~200 across 4 files