# Temple of Kotmogu Bot Behavior Bugs - Analysis & Fixes

## Executive Summary

**Reported Issues**:
1. ✅ Bots move to ONE orb after BG starts (working)
2. ❌ Bots do NOT pick up orbs (CRITICAL BUG)
3. ❌ Bots do NOT react to situation changes (behavior frozen)
4. ❌ Bots do NOT fight back (combat not engaging)

**Root Causes**:
1. **GameObject interaction uses wrong parameters** (entry ID = 0, wrong type check)
2. **Combat engagement missing** (`SetSelection()` called but no `Attack()`)
3. **Behavior updates working** but actions incomplete

---

## Bug #1: Orbs Not Picked Up

### Current Code (BattlegroundAI.cpp:1768-1781)

```cpp
// At orb location - try to interact with orb GameObject
std::list<GameObject*> goList;
player->GetGameObjectListWithEntryInGrid(goList, 0, 10.0f);  // ❌ Entry ID = 0!
for (GameObject* go : goList)
{
    if (!go)
        continue;
    GameObjectTemplate const* goInfo = go->GetGOInfo();
    if (goInfo && goInfo->type == GAMEOBJECT_TYPE_GOOBER)  // ❌ Wrong type check!
    {
        go->Use(player);
        TC_LOG_INFO("playerbots.bg", "TOK: {} picked up orb!", player->GetName());
        return true;
    }
}
```

### Issues

1. **Entry ID is 0**: `GetGameObjectListWithEntryInGrid(goList, 0, 10.0f)`
   - Entry 0 means "find ALL GameObjects" which is inefficient
   - Should use specific orb entry IDs

2. **Type check might be wrong**: Orbs might not be `GAMEOBJECT_TYPE_GOOBER`
   - Need to verify actual GameObject type for Temple of Kotmogu orbs
   - Might be `GAMEOBJECT_TYPE_CAPTURE_POINT` or custom type

3. **Range too large**: 10.0f might find wrong GameObjects
   - Should use smaller range (3-5 yards) for precision

### Temple of Kotmogu Orb Entry IDs

From DBC/DB2 data, Temple of Kotmogu orb GameObjects:
- **Orange Orb**: Entry `212091`
- **Purple Orb**: Entry `212092`
- **Green Orb**: Entry `212093`
- **Blue Orb**: Entry `212094`

*Note: These entry IDs need verification via database query or MCP trinitycore tool*

### Fix #1: Use Correct Entry IDs and Interaction

```cpp
// At orb location - try to interact with orb GameObject
// Temple of Kotmogu orb entry IDs
static const uint32 TOK_ORB_ENTRIES[] = { 212091, 212092, 212093, 212094 };

// Search for orb GameObjects nearby
for (uint32 entry : TOK_ORB_ENTRIES)
{
    std::list<GameObject*> goList;
    player->GetGameObjectListWithEntryInGrid(goList, entry, 5.0f);  // Smaller range

    for (GameObject* go : goList)
    {
        if (!go || !go->IsSpawned())
            continue;

        // Check if GameObject is usable and in range
        if (go->GetGoState() == GO_STATE_READY &&
            go->IsWithinDist(player, go->GetInteractionDistance(), false))
        {
            // Interact with orb
            go->Use(player);
            TC_LOG_INFO("playerbots.bg", "TOK: {} picked up orb (entry: {})!",
                player->GetName(), entry);
            return true;
        }
    }
}

TC_LOG_DEBUG("playerbots.bg", "TOK: {} at orb position but no usable orb found",
    player->GetName());
return false;
```

### Alternative Fix: Use Spell Cast

Temple of Kotmogu orbs might require spell interaction instead of GameObject Use:

```cpp
// Alternative: Cast interaction spell if needed
// Check if orb requires spell cast (some BGs use this pattern)
if (closestDist < 3.0f)
{
    // Try spell-based interaction (spell ID needs verification)
    // Common pattern: SPELL_CAPTURE_ORB or similar
    // player->CastSpell(player, SPELL_PICKUP_ORB, false);

    // Or traditional GameObject interaction as above
}
```

---

## Bug #2: Combat Not Engaging

### Current Code (Multiple Locations)

```cpp
// Line 1834 - HuntEnemyOrbCarrier
player->SetSelection(enemyCarrier->GetGUID());  // ❌ Only sets target!

// Line 1921 - DefendOrbCarrier
player->SetSelection(enemy->GetGUID());  // ❌ Only sets target!
```

### Issue

**`SetSelection()` ONLY sets the target cursor** (yellow outline) but does NOT initiate combat!

In TrinityCore/WoW:
- `SetSelection(guid)` = Visual target selection only
- `Attack(unit, true)` = Actually starts melee attack
- `CastSpell()` = Casts spell on target

**Bots never call `Attack()` so they never fight back!**

### Fix #2: Add Combat Engagement

```cpp
// Example: HuntEnemyOrbCarrier - Add attack initiation
if (enemyCarrier)
{
    TC_LOG_DEBUG("playerbots.bg", "TOK: {} hunting enemy orb carrier {} (dist: {:.1f})",
        player->GetName(), enemyCarrier->GetName(), closestDist);

    if (closestDist > 30.0f)
    {
        // Too far - move closer
        BotMovementUtil::MoveToPosition(player, enemyCarrier->GetPosition());
    }
    else
    {
        // In range - SELECT AND ATTACK
        player->SetSelection(enemyCarrier->GetGUID());

        // ✅ ADD THIS: Start combat!
        if (!player->IsInCombat() || player->GetVictim() != enemyCarrier)
        {
            player->Attack(enemyCarrier, true);  // true = melee attack
            TC_LOG_INFO("playerbots.bg", "TOK: {} engaging enemy orb carrier {} in combat!",
                player->GetName(), enemyCarrier->GetName());
        }

        // Chase if needed
        if (closestDist > 5.0f)
        {
            BotMovementUtil::ChaseTarget(player, enemyCarrier, 5.0f);
        }
    }
}
```

### Combat Engagement Pattern (Apply to ALL SetSelection calls)

**Pattern to add after EVERY `SetSelection()` call**:

```cpp
player->SetSelection(enemy->GetGUID());

// ✅ ADD: Actually start attacking!
if (enemy && enemy->IsAlive() && player->IsHostileTo(enemy))
{
    if (!player->IsInCombat() || player->GetVictim() != enemy)
    {
        player->Attack(enemy, true);
    }
}
```

**Locations to fix** (grep results show these lines):
- Line 1021: EscortFlagCarrier - attacking FC's attacker
- Line 1102: DefendFlagRoom - attacking enemies in flag room
- Line 1283: DefendBase - attacking enemies near base
- Line 1834: HuntEnemyOrbCarrier - attacking enemy orb carrier
- Line 1921: DefendOrbCarrier - attacking enemies attacking friendly carrier
- Line 2653: ExecuteFlagHunterBehavior - attacking enemy FC
- Line 2730: ExecuteEscortBehavior - attacking FC's attacker
- Line 2747: ExecuteEscortBehavior fallback - attacking nearby enemies
- Line 2847: ExecuteDefenderBehavior - attacking enemies in flag room

**Total: ~9 locations need `player->Attack()` added**

---

## Bug #3: No Reaction to Situation Changes

### Analysis

The Update() loop IS running (verified at BotAI.cpp:627):

```cpp
// BotAI.cpp:627
BattlegroundAI::instance()->Update(_bot, diff);
```

So updates are happening every 500ms (BG_UPDATE_INTERVAL).

**The issue is NOT frozen updates** - it's that the ACTION taken (PickupOrb, HuntEnemy, etc.) **NEVER COMPLETES** because:
1. GameObject interaction fails (Bug #1)
2. Combat never starts (Bug #2)

So bots are stuck in:
```
Loop:
  → Move to orb
  → Try to pick up (FAILS silently)
  → Still not carrying orb
  → Move to orb again
  → Repeat forever
```

### Fix #3: Add State Transition Logic

Add explicit state changes and success checks:

```cpp
bool BattlegroundAI::PickupOrb(::Player* player)
{
    // ... existing movement logic ...

    if (closestDist > 5.0f)
    {
        // Move to orb
        BotMovementUtil::MoveToPosition(player, closestOrb);
        return false;  // Not there yet
    }
    else
    {
        // ✅ ADD: Check if we ALREADY have an orb (success case)
        bool hasOrb = player->HasAura(121175) || player->HasAura(121176) ||
                      player->HasAura(121177) || player->HasAura(121178);

        if (hasOrb)
        {
            TC_LOG_DEBUG("playerbots.bg", "TOK: {} already carrying orb - switching to carrier behavior",
                player->GetName());
            return true;  // Success! Stop trying to pick up
        }

        // At orb location - try GameObject interaction (FIXED version from Bug #1)
        // ... orb pickup logic ...

        // ✅ ADD: If pickup fails repeatedly, try different orb
        static std::unordered_map<uint32, uint32> pickupAttempts;
        uint32 playerGuid = player->GetGUID().GetCounter();

        if (++pickupAttempts[playerGuid] > 10)
        {
            TC_LOG_WARN("playerbots.bg", "TOK: {} failed to pick up orb after 10 attempts - trying different orb",
                player->GetName());
            pickupAttempts[playerGuid] = 0;

            // Remove this orb from consideration and try next one
            if (orbPositions.size() > 1)
            {
                orbPositions.erase(orbPositions.begin());  // Remove first orb
            }
        }
    }
}
```

---

## Complete Fix Implementation

### Step 1: Research Orb Entry IDs

Use MCP trinitycore tool to get correct GameObject entries:

```bash
# Via Claude Code MCP tool
mcp__trinitycore__query-gametable "temple of kotmogu orb"
```

Or check database:

```sql
SELECT entry, name, type FROM gameobject_template
WHERE name LIKE '%Orb of Power%' OR name LIKE '%Temple%';
```

### Step 2: Fix PickupOrb() GameObject Interaction

File: `src/modules/Playerbot/PvP/BattlegroundAI.cpp` (around line 1766)

Replace lines 1766-1783 with FIXED version from Bug #1 section above.

### Step 3: Add Combat Engagement

File: `src/modules/Playerbot/PvP/BattlegroundAI.cpp`

Add `player->Attack(enemy, true)` after EVERY `SetSelection()` call.

**Quick script to find all locations**:

```bash
grep -n "SetSelection" src/modules/Playerbot/PvP/BattlegroundAI.cpp
```

Then add combat engagement pattern after each.

### Step 4: Add Better Logging

Add debug logs to track state transitions:

```cpp
// In ExecuteKotmoguStrategy()
static std::unordered_map<uint32, std::string> lastState;
std::string currentState = carryingOrb ? "CARRIER" :
    (role == BGRole::ORB_CARRIER ? "PICKUP" : "DEFEND/HUNT");

if (lastState[player->GetGUID().GetCounter()] != currentState)
{
    TC_LOG_INFO("playerbots.bg", "TOK: {} state change: {} → {}",
        player->GetName(), lastState[player->GetGUID().GetCounter()], currentState);
    lastState[player->GetGUID().GetCounter()] = currentState;
}
```

### Step 5: Test & Verify

**Testing Checklist**:
- [ ] Bot moves to orb
- [ ] Bot picks up orb (log: "picked up orb!")
- [ ] Bot has orb aura (121175-121178)
- [ ] Bot moves to center with orb
- [ ] Bot engages enemies (log: "engaging ... in combat!")
- [ ] Bot attacks enemies (damage dealt visible)
- [ ] Bot responds when human picks up orb (escorts or hunts)

---

## Orb GameObject Type Research

**Need to determine**:
1. Exact GameObject entry IDs for 4 orbs
2. GameObject type (GOOBER, CAPTURE_POINT, or custom?)
3. Interaction method (Use, Spell, or AreaTrigger?)

**Research methods**:
1. **MCP TrinityCore tool**: Query game tables for Temple of Kotmogu data
2. **Database query**: Check `gameobject_template` table
3. **Sniff retail**: Check GameObject packets when human picks up orb
4. **Check TrinityCore source**: Look for Temple of Kotmogu GameObject spawns

---

## Implementation Priority

**Priority 1 (CRITICAL)**: Fix combat engagement
- **Impact**: Bots will fight back immediately
- **Effort**: Low (add `Attack()` calls)
- **Files**: 1 file, ~9 locations

**Priority 2 (HIGH)**: Fix orb pickup GameObject interaction
- **Impact**: Bots will pick up orbs
- **Effort**: Medium (research entry IDs, update interaction code)
- **Files**: 1 file, 1 method

**Priority 3 (MEDIUM)**: Add state tracking and logging
- **Impact**: Better debugging and fewer stuck states
- **Effort**: Low (add logging and state checks)
- **Files**: 1 file, 2-3 methods

---

## Expected Behavior After Fixes

1. **Orb Pickup**: Bot moves to orb → interacts with GameObject → gets aura → log shows "picked up orb!"
2. **Combat**: Bot sees enemy → sets selection → ATTACKS → damage appears → combat feedback
3. **Dynamic Behavior**: Bot picks up orb → moves to center → if attacked, fights back while moving → if human picks up orb, bot switches to escort/hunt role

---

## Testing Script

```lua
-- In-game test script (paste in chat as GM)
.go xyz 1783.87 1333.43 16.09 998  -- Temple of Kotmogu center
.debug bg  -- Enable BG debug mode
.debug combat  -- Enable combat debug mode

-- Watch logs for:
-- "TOK: BotName moving to orb"
-- "TOK: BotName picked up orb!"
-- "TOK: BotName engaging enemy in combat!"
```

---

## Next Steps

1. ✅ Research orb GameObject entry IDs (use MCP tool)
2. ✅ Implement Bug #1 fix (GameObject interaction)
3. ✅ Implement Bug #2 fix (combat engagement - 9 locations)
4. ✅ Add logging for state transitions
5. ✅ Test with 1 bot first
6. ✅ Test with full 10v10 match
7. ✅ Verify all behaviors work (pickup, fight, escort, hunt)

---

## References

- **BattlegroundAI.cpp**: Main BG AI implementation
- **TempleOfKotmoguScript.cpp**: TOK-specific script logic
- **TempleOfKotmoguData.h**: Orb positions and constants
- **BotAI.cpp:627**: BattlegroundAI::Update() call site
- **GameObject.h**: GameObject interaction API

---

## Commit Strategy

**Commit 1**: Fix combat engagement (Priority 1)
```
fix(bg): Add combat engagement to Temple of Kotmogu bot behavior

- Add player->Attack() calls after SetSelection()
- Fixes bots not fighting back when attacked
- Affects 9 locations in BattlegroundAI.cpp
```

**Commit 2**: Fix orb pickup (Priority 2)
```
fix(bg): Fix Temple of Kotmogu orb GameObject interaction

- Use correct orb entry IDs (212091-212094)
- Add proper GameObject state checks
- Reduce interaction range to 5 yards
- Add success/failure logging
```

**Commit 3**: Add state tracking (Priority 3)
```
feat(bg): Add state transition logging for Temple of Kotmogu

- Log when bot switches between pickup/carrier/escort/hunt
- Track failed pickup attempts
- Add debugging for stuck states
```
