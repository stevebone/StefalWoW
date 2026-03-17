# Claude Code Prompt: Healing & Tank/Aggro System Fix

**Copy this entire prompt into Claude Code to execute the implementation.**

---

## Task

Fix the healing and tank/aggro systems based on the Zenflow analysis. The root cause is that `GroupCombatTrigger` stops firing after bots enter combat, which breaks both healers and tanks.

Read the full implementation plan: `.claude/prompts/HEALING_TANK_FIX_IMPLEMENTATION.md`

---

## Phase 1: Critical Fix - GroupCombatTrigger (MUST DO FIRST)

### Step 1.1: Find and fix GroupCombatTrigger.cpp

```bash
# Find the file
find src/modules/Playerbot -name "GroupCombatTrigger.cpp" -type f
```

Open the file and find the `Check()` function. Look for code like this:

```cpp
if (bot->IsInCombat())
{
    TC_LOG_DEBUG(...);
    return false;
}
```

**DELETE this entire block.** The trigger must continue firing even when bots are in combat.

After deletion, verify the function still:
1. Checks if bot is in a group
2. Checks if group is in combat
3. Returns appropriate boolean

---

## Phase 2: Tank System Fixes

### Step 2.1: Implement IsTauntImmune()

```bash
# Find ThreatAssistant files
find src/modules/Playerbot -name "ThreatAssistant*" -type f
```

Open `ThreatAssistant.cpp` and find the `IsTauntImmune()` function. It's currently a stub:

```cpp
bool ThreatAssistant::IsTauntImmune(Unit* target)
{
    return false;  // STUB
}
```

Replace with full implementation that checks:
1. `target->HasMechanicImmunity(MECHANIC_TAUNT)`
2. Creature flags: `CREATURE_TYPE_FLAG_BOSS_MOB` and `CREATURE_FLAG_EXTRA_TAUNT_IMMUNE`
3. Active immunity auras: `SPELL_AURA_MECHANIC_IMMUNITY` with `MECHANIC_TAUNT`

Add required includes at top of file if missing:
- `CreatureData.h`
- `SpellAuraDefines.h`
- `SpellAuraEffects.h`

### Step 2.2: Fix GetCombatEnemies()

In the same file, find `GetCombatEnemies()` function.

Look for this pattern:
```cpp
for (GroupReference& ref : group->GetMembers())
```

Replace with GroupMemberResolver pattern:
```cpp
for (auto const& slot : group->GetMemberSlots())
{
    Player* member = GroupMemberResolver::ResolveMember(slot.guid);
    if (!member || member->isDead())
        continue;
    // ... rest of logic
}
```

Add include if missing:
```cpp
#include "../../Group/GroupMemberResolver.h"
```

### Step 2.3: Fix GetThreatPercentage() edge case

Find `GetThreatPercentage()` function, look for:
```cpp
if (!victim)
    return 100.0f;
```

Change to:
```cpp
if (!victim)
    return 0.0f;  // No victim = tank doesn't have aggro
```

---

## Phase 3: Healer Verification

### Step 3.1: Check all healer specs use HealingTargetSelector

```bash
# Search for healer implementations
grep -rn "SelectHealingTarget\|HealingTargetSelector" src/modules/Playerbot/AI/ClassAI --include="*.h" --include="*.cpp"
```

Check these files have proper integration:
- `Priests/HolyPriest.h`
- `Priests/DisciplinePriest.h`
- `Druids/RestorationDruid.h`
- `Shamans/RestorationShaman.h`
- `Monks/MistweaverMonk.h`
- `Paladins/HolyPaladin.h` (already verified ✅)

Each should have a `SelectHealingTarget()` method that calls `HealingTargetSelector::SelectTarget()`.

If any are missing or use custom logic, update them to use the service.

---

## Phase 4: Build & Verify

```bash
cd C:\TrinityBots\TrinityCore\build
cmake --build . --config RelWithDebInfo --target worldserver -j 8
```

Fix any compilation errors before proceeding.

---

## Phase 5: Git Commit

```bash
cd C:\TrinityBots\TrinityCore
git add -A
git status
git commit -m "fix(playerbot): Fix healing and tank/aggro systems

ROOT CAUSE: GroupCombatTrigger stopped firing after bots entered combat,
preventing healers from calling HealingTargetSelector and tanks from
monitoring threat.

Changes:
- GroupCombatTrigger: Remove IsInCombat() early return that broke both systems
- ThreatAssistant: Implement IsTauntImmune() to prevent wasted taunts on bosses
- ThreatAssistant: Use GroupMemberResolver in GetCombatEnemies()
- ThreatAssistant: Fix GetThreatPercentage() edge case

Fixes:
- Healers now continuously heal group members during combat
- Tanks now properly manage threat throughout encounters
- Tanks skip taunting immune targets (bosses)

Based on Zenflow analysis: agghro-healing-analysis-2021"
```

---

## Important Notes

1. **Phase 1 is the most critical** - it fixes both healing and tank systems with one small change
2. **Test after each phase** if possible
3. **Don't skip Phase 3** - inconsistent healer implementations will cause issues
4. **Build must succeed** before committing

---

## Expected Outcomes

After implementation:
- ✅ Healers continuously heal injured group members during combat
- ✅ Tanks continuously manage threat during combat
- ✅ Tanks don't waste taunts on immune bosses
- ✅ Tanks see all enemies attacking group members
- ✅ All healer specs use consistent target selection

---

## Troubleshooting

**If GroupCombatTrigger.cpp not found:**
```bash
grep -rn "GroupCombatTrigger" src/modules/Playerbot --include="*.cpp"
```

**If ThreatAssistant.cpp not found:**
```bash
grep -rn "class ThreatAssistant" src/modules/Playerbot --include="*.h"
```

**If build fails with missing includes:**
Check the full include paths in similar files and adjust accordingly.

**If GroupMemberResolver not found:**
```bash
find src/modules/Playerbot -name "*GroupMember*" -type f
```
