# Healing & Tank/Aggro Fix Implementation Plan

**Priority**: P0 (Critical)  
**Estimated Duration**: 8-12 hours  
**Based On**: Zenflow Analysis (2026-01-27)

---

## Executive Summary

One critical bug in `GroupCombatTrigger` breaks BOTH healing and tank systems. This plan fixes all identified issues in priority order.

---

## Phase 1: Critical Fix - GroupCombatTrigger (30 min)

### Task 1.1: Fix GroupCombatTrigger::Check()

**File**: `src/modules/Playerbot/AI/Combat/GroupCombatTrigger.cpp`

**Problem**: Trigger stops firing after bot enters combat state.

**Current Code (lines ~48-53)**:
```cpp
if (bot->IsInCombat())
{
    TC_LOG_DEBUG("module.playerbot.combat", 
                 "GroupCombatTrigger::Check - {} already in combat, skipping",
                 bot->GetName());
    return false;
}
```

**Action**: DELETE these lines entirely.

**Verification**:
1. Search for `IsInCombat()` in the Check() function
2. Remove the early return block
3. Ensure the function continues to check group combat state

**Expected Result**:
- Trigger continues firing throughout combat
- Healers continuously call HealingTargetSelector
- Tanks continuously monitor threat

---

## Phase 2: Tank System Fixes (3-4 hours)

### Task 2.1: Implement IsTauntImmune()

**File**: `src/modules/Playerbot/AI/Services/ThreatAssistant.cpp`

**Problem**: Function is a stub that always returns `false`.

**Current Code (lines ~223-238)**:
```cpp
bool ThreatAssistant::IsTauntImmune(Unit* target)
{
    // STUB: Always returns false
    return false;
}
```

**Replace With**:
```cpp
bool ThreatAssistant::IsTauntImmune(Unit* target)
{
    if (!target)
        return true;
    
    // 1. Check taunt mechanic immunity
    if (target->HasMechanicImmunity(MECHANIC_TAUNT))
    {
        TC_LOG_DEBUG("playerbot", "ThreatAssistant::IsTauntImmune - {} immune via MECHANIC_TAUNT", 
                     target->GetName());
        return true;
    }
    
    // 2. Check creature flags
    if (Creature* creature = target->ToCreature())
    {
        CreatureTemplate const* tmpl = creature->GetCreatureTemplate();
        if (tmpl)
        {
            // Boss flag - most bosses are taunt immune
            if (tmpl->type_flags & CREATURE_TYPE_FLAG_BOSS_MOB)
            {
                TC_LOG_DEBUG("playerbot", "ThreatAssistant::IsTauntImmune - {} is BOSS_MOB", 
                             target->GetName());
                return true;
            }
            
            // Explicit taunt immune flag
            if (tmpl->flags_extra & CREATURE_FLAG_EXTRA_TAUNT_IMMUNE)
            {
                TC_LOG_DEBUG("playerbot", "ThreatAssistant::IsTauntImmune - {} has TAUNT_IMMUNE flag", 
                             target->GetName());
                return true;
            }
        }
    }
    
    // 3. Check active immunity auras
    if (target->HasAuraType(SPELL_AURA_MECHANIC_IMMUNITY))
    {
        Unit::AuraEffectList const& immunities = 
            target->GetAuraEffectsByType(SPELL_AURA_MECHANIC_IMMUNITY);
        
        for (AuraEffect const* aurEff : immunities)
        {
            if (aurEff->GetMiscValue() == MECHANIC_TAUNT)
            {
                TC_LOG_DEBUG("playerbot", "ThreatAssistant::IsTauntImmune - {} has immunity aura", 
                             target->GetName());
                return true;
            }
        }
    }
    
    return false;
}
```

**Required Includes** (add if missing):
```cpp
#include "CreatureData.h"
#include "SpellAuraDefines.h"
#include "SpellAuraEffects.h"
```

---

### Task 2.2: Fix GetCombatEnemies() to use GroupMemberResolver

**File**: `src/modules/Playerbot/AI/Services/ThreatAssistant.cpp`

**Problem**: Uses `GetMembers()` directly instead of `GroupMemberResolver`.

**Find the function** `GetCombatEnemies()` (around line 268-289).

**Current Pattern**:
```cpp
for (GroupReference& ref : group->GetMembers())
{
    Player* member = ref.GetSource();
    // ...
}
```

**Replace With**:
```cpp
// Use GroupMemberResolver pattern (same as HealingTargetSelector)
for (auto const& slot : group->GetMemberSlots())
{
    Player* member = GroupMemberResolver::ResolveMember(slot.guid);
    
    if (!member || member->isDead())
        continue;
    
    // ... rest of threat aggregation logic
}
```

**Required Includes** (add if missing):
```cpp
#include "../../Group/GroupMemberResolver.h"
```

---

### Task 2.3: Clarify GetThreatPercentage() Edge Case

**File**: `src/modules/Playerbot/AI/Services/ThreatAssistant.cpp`

**Find** (around line 195-196):
```cpp
Unit* victim = target->GetVictim();
if (!victim)
    return 100.0f;  // Misleading
```

**Change to**:
```cpp
Unit* victim = target->GetVictim();
if (!victim)
    return 0.0f;  // No victim = tank doesn't have aggro
```

---

## Phase 3: Healer System Verification (1-2 hours)

### Task 3.1: Verify All Healer Specs Use HealingTargetSelector

**Files to Check**:
1. `src/modules/Playerbot/AI/ClassAI/Priests/HolyPriest.h`
2. `src/modules/Playerbot/AI/ClassAI/Priests/DisciplinePriest.h`
3. `src/modules/Playerbot/AI/ClassAI/Druids/RestorationDruid.h`
4. `src/modules/Playerbot/AI/ClassAI/Shamans/RestorationShaman.h`
5. `src/modules/Playerbot/AI/ClassAI/Monks/MistweaverMonk.h`
6. `src/modules/Playerbot/AI/ClassAI/Paladins/HolyPaladin.h` (already verified ✅)

**Search Pattern**:
```bash
grep -rn "SelectHealingTarget\|HealingTargetSelector" --include="*.h" --include="*.cpp"
```

**Expected Pattern in Each File**:
```cpp
Player* SelectHealingTarget(Group* group)
{
    if (!group)
        return this->GetBot();
    
    return HealingTargetSelector::SelectTarget(
        this->GetBot(),
        40.0f,   // Range
        100.0f   // Min health percent
    );
}
```

**If Missing**: Add the integration following HolyPaladin pattern.

---

## Phase 4: Testing & Verification (2-3 hours)

### Task 4.1: Build Verification

```bash
cd C:\TrinityBots\TrinityCore\build
cmake --build . --config RelWithDebInfo --target worldserver -j 8
```

**Expected**: No compilation errors.

---

### Task 4.2: Functional Tests

**Test 1: Healer Healing (After Phase 1)**
```
Setup: Tank + Healer + DPS bots
Action: Pull mob, damage DPS to 50%
Expected: Healer heals DPS continuously
```

**Test 2: Tank Taunt Skip (After Phase 2)**
```
Setup: Tank bot vs Raid Boss (taunt immune)
Action: Observe tank behavior
Expected: Tank does NOT use Taunt ability
```

**Test 3: Tank Taunt Normal (After Phase 2)**
```
Setup: Tank + DPS bots vs normal mob
Action: DPS pulls aggro
Expected: Tank taunts mob off DPS
```

**Test 4: Complete Group Combat (After All Phases)**
```
Setup: Tank + Healer + 2 DPS bots
Action: Enter dungeon, pull trash pack
Expected: 
  - Tank holds aggro
  - Healer heals injured members
  - All bots respond to threats
```

---

## Phase 5: Git Commit (15 min)

### Commit Message Template

```
fix(playerbot): Fix healing and tank/aggro systems

ROOT CAUSE: GroupCombatTrigger stopped firing after bots entered combat,
preventing healers from calling HealingTargetSelector and tanks from
monitoring threat.

Changes:
- GroupCombatTrigger: Remove IsInCombat() early return that broke both systems
- ThreatAssistant: Implement IsTauntImmune() to prevent wasted taunts on bosses
- ThreatAssistant: Use GroupMemberResolver in GetCombatEnemies()
- ThreatAssistant: Fix GetThreatPercentage() edge case
- Verified all healer specs use HealingTargetSelector service

Fixes:
- Healers now continuously heal group members during combat
- Tanks now properly manage threat throughout encounters
- Tanks skip taunting immune targets (bosses)
- Complete enemy detection for tank threat management

Based on Zenflow analysis task: agghro-healing-analysis-2021
```

---

## File Summary

| File | Changes |
|------|---------|
| `GroupCombatTrigger.cpp` | Delete IsInCombat() early return |
| `ThreatAssistant.cpp` | Implement IsTauntImmune(), fix GetCombatEnemies(), fix GetThreatPercentage() |
| `ThreatAssistant.h` | Add includes if needed |
| Various Healer*.h | Verify/add HealingTargetSelector integration |

---

## Risk Assessment

| Change | Risk | Mitigation |
|--------|------|------------|
| GroupCombatTrigger fix | Low | Small change, well-understood |
| IsTauntImmune() | Low | Pure addition, no existing logic changed |
| GetCombatEnemies() fix | Low | Uses proven pattern from HealingTargetSelector |
| GetThreatPercentage() | None | One-line clarity change |
| Healer verification | None | Verification + refactoring only |

---

## Success Criteria

- [ ] Build completes without errors
- [ ] Healers heal injured group members during combat
- [ ] Tanks manage threat throughout combat
- [ ] Tanks skip taunting immune bosses
- [ ] All healer specs use HealingTargetSelector
- [ ] Git commit created with proper message
