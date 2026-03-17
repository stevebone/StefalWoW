# DEADLOCK FIX PHASE 2 - ClassAI UpdateRotation Pattern

## ROOT CAUSE ANALYSIS

**Problem**: 86 files in `src/modules/Playerbot` call `ObjectAccessor::GetUnit()`, `ObjectAccessor::GetCreature()`, or `ObjectAccessor::GetPlayer()` from **WORKER THREADS** during `UpdateRotation()` execution.

**Why This Causes Deadlock**:
- `UpdateRotation()` is called from worker thread pool by BotAI
- `ObjectAccessor::GetUnit(*bot, guid)` calls `Map::GetCreature(guid)`
- `Map::_objectsStore` has **NO MUTEX** (single-threaded by design)
- Multiple worker threads accessing `Map::_objectsStore` simultaneously → **DEADLOCK**
- Futures 3-14 never complete with 5000+ bots

## INCORRECT PATTERN (DEADLOCK)

```cpp
void UnholySpecialization::UpdateRotation(Unit* target)
{
    Player* bot = GetBot();

    // ❌ DEADLOCK: Calling ObjectAccessor from worker thread!
    ::Unit* target = ObjectAccessor::GetUnit(*bot, bot->GetTarget());

    if (!target)
        return;

    // Use target...
    float targetHealthPct = target->GetHealthPct();

    // Cast spells using ObjectAccessor calls...
}
```

**Why This is Wrong**:
1. `UpdateRotation()` is called from worker thread
2. `ObjectAccessor::GetUnit()` accesses `Map::_objectsStore` (NOT thread-safe)
3. Multiple bots doing this simultaneously → deadlock

## CORRECT PATTERN (THREAD-SAFE)

```cpp
void UnholySpecialization::UpdateRotation(Unit* target)
{
    Player* bot = GetBot();

    // ✅ CORRECT: Use the target parameter passed from main thread!
    if (!target || !target->IsInWorld())
        return;

    // Use target directly - NO ObjectAccessor calls!
    float targetHealthPct = target->GetHealthPct();

    // Queue actions using BotActionManager (thread-safe)
    sBotActionMgr->QueueAction(BotAction::CastSpell(
        bot->GetGUID(),
        DEATH_STRIKE,
        target->GetGUID(),
        getMSTime()
    ));
}
```

**Why This is Correct**:
1. Use `target` parameter (already resolved on main thread)
2. NO `ObjectAccessor` calls from worker thread
3. Queue actions using `sBotActionMgr` (thread-safe queue)
4. Main thread executes actions with Map access

## FILES REQUIRING FIX (86 total)

### High Priority (Class AI Specializations - Run on Worker Threads)
1. `src/modules/Playerbot/AI/ClassAI/DeathKnights/UnholySpecialization.cpp` (4 locations)
2. `src/modules/Playerbot/AI/ClassAI/DeathKnights/FrostSpecialization.cpp` (1 location)
3. `src/modules/Playerbot/AI/ClassAI/DeathKnights/DeathKnightAI.cpp` (1 location)
4. `src/modules/Playerbot/AI/ClassAI/Warriors/*.cpp`
5. `src/modules/Playerbot/AI/ClassAI/Hunters/*.cpp`
6. `src/modules/Playerbot/AI/ClassAI/Rogues/*.cpp`
7. `src/modules/Playerbot/AI/ClassAI/Mages/*.cpp`
8. `src/modules/Playerbot/AI/ClassAI/Warlocks/*.cpp`
9. `src/modules/Playerbot/AI/ClassAI/Shamans/*.cpp`
10. `src/modules/Playerbot/AI/ClassAI/Paladins/*.cpp`
11. `src/modules/Playerbot/AI/ClassAI/Priests/*.cpp`
12. `src/modules/Playerbot/AI/ClassAI/Druids/*.cpp`
13. `src/modules/Playerbot/AI/ClassAI/Monks/*.cpp`
14. `src/modules/Playerbot/AI/ClassAI/DemonHunters/*.cpp`
15. `src/modules/Playerbot/AI/ClassAI/Evokers/*.cpp`

### Medium Priority (Combat Managers - May Run on Worker Threads)
16. `src/modules/Playerbot/AI/Combat/CombatStateAnalyzer.cpp`
17. `src/modules/Playerbot/AI/Combat/TargetSelector.cpp`
18. `src/modules/Playerbot/AI/Combat/PositionManager.cpp`
19. `src/modules/Playerbot/AI/Combat/InterruptManager.cpp`
20. `src/modules/Playerbot/AI/Strategy/CombatMovementStrategy.cpp`

### Low Priority (Main Thread Only - Safe but Should Still Use Actions)
21. `src/modules/Playerbot/Threading/BotActionProcessor.cpp` (✅ Already uses actions)
22. `src/modules/Playerbot/AI/BotAI.cpp` (✅ Main thread, but good to use actions)
23. Quest/Interaction managers (main thread operations)

## IMPLEMENTATION STRATEGY

### Phase 2A: Remove ObjectAccessor from UpdateRotation
**For each ClassAI file:**
1. Find all `ObjectAccessor::GetUnit(*bot, bot->GetTarget())` calls
2. Replace with use of `target` parameter
3. Add validation: `if (!target || !target->IsInWorld()) return;`

### Phase 2B: Convert Direct Spell Casts to Action Queue
**For each spell cast:**
```cpp
// ❌ BEFORE (Direct cast from worker thread - unsafe with future changes):
bot->CastSpell(target, DEATH_STRIKE, false);

// ✅ AFTER (Queue action for main thread execution):
sBotActionMgr->QueueAction(BotAction::CastSpell(
    bot->GetGUID(),
    DEATH_STRIKE,
    target->GetGUID(),
    getMSTime()
));
```

### Phase 2C: Convert Direct Attacks to Action Queue
```cpp
// ❌ BEFORE:
bot->Attack(target, true);

// ✅ AFTER:
sBotActionMgr->QueueAction(BotAction::AttackTarget(
    bot->GetGUID(),
    target->GetGUID(),
    getMSTime()
));
```

## AUTOMATION SCRIPT

Create Python script to automate the fix across all 86 files:

```python
import re
import os

def fix_update_rotation_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Pattern 1: Remove ObjectAccessor::GetUnit(*bot, bot->GetTarget())
    pattern1 = r'::Unit\*\s+target\s*=\s*ObjectAccessor::GetUnit\(\*bot,\s*bot->GetTarget\(\)\);'
    replacement1 = '// DEADLOCK FIX: Use target parameter instead of ObjectAccessor'

    content = re.sub(pattern1, replacement1, content)

    # Pattern 2: Add target validation if missing
    if 'if (!target' not in content:
        # Add after function start
        pass

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

# Run on all ClassAI files
base_dir = 'src/modules/Playerbot/AI/ClassAI'
for root, dirs, files in os.walk(base_dir):
    for file in files:
        if file.endswith('.cpp'):
            filepath = os.path.join(root, file)
            fix_update_rotation_file(filepath)
```

## EXPECTED RESULTS AFTER FIX

✅ **All ObjectAccessor calls removed from worker threads**
✅ **UpdateRotation methods use passed target parameter**
✅ **All spell casts queued via BotActionManager**
✅ **Main thread processes queued actions with safe Map access**
✅ **Futures 3-14 complete with 5000+ bots**
✅ **No more 60-second hangs or deadlocks**

## TESTING CHECKLIST

- [ ] Build succeeds with zero errors
- [ ] Spawn 100 bots - all futures complete
- [ ] Spawn 1000 bots - all futures complete
- [ ] Spawn 5000 bots - **futures 3-14 complete** (THIS IS THE KEY TEST)
- [ ] No server hangs or 60-second pauses
- [ ] Bots execute rotations correctly
- [ ] Combat system works normally

## REFERENCES

- Action Queue Implementation: `src/modules/Playerbot/Threading/BotActionManager.h`
- Action Definitions: `src/modules/Playerbot/Threading/BotAction.h`
- Action Executors: `src/modules/Playerbot/Threading/BotActionProcessor.cpp`
- Integration Point: `src/server/game/World/World.cpp:2348-2356`
