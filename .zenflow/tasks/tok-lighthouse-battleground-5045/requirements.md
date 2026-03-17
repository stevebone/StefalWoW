# Product Requirements Document: TOK Lighthouse Battleground

## 1. Overview

### 1.1 Purpose
Make Temple of Kotmogu (TOK) the first 100% correct, fully dynamic battleground in the Playerbot module. TOK will serve as the lighthouse/blueprint for implementing all 13 other battlegrounds.

### 1.2 Current State
The TOK implementation has a well-designed **coordination layer** (~80% complete) but is missing the **execution layer** entirely. The coordination layer includes strategy scripts, position data, role management, and event tracking. The execution layer — where bots actually pick up orbs, attack enemies, move to objectives, and execute strategy — consists only of stub functions that log and return true.

### 1.3 Goal
Deliver a production-ready TOK bot implementation where bots:
- Pick up orbs reliably (>95% success rate)
- Engage in combat with enemies (100% engagement rate)
- Execute dynamic strategy based on game state
- Achieve zero crashes under normal operation

---

## 2. Problem Statement

### 2.1 Critical Gaps in Execution Layer

The three BattlegroundAI.cpp methods for TOK are stubs:

**`PickupOrb()` (line 1579):** Only logs a debug message and returns true. No actual orb discovery, navigation, distance check, or GameObject::Use() call.

**`DefendOrbCarrier()` (line 1591):** Returns true immediately. No target finding, following, or combat engagement.

**`ExecuteKotmoguStrategy()` (line 1569):** Calls the two stubs sequentially with no role-based decision branching.

### 2.2 Missing Execution Behaviors
- No orb GameObject interaction (finding nearby orbs, navigating to them, using them)
- No combat engagement — bots never call `Attack()` against enemy players
- No movement execution — bots don't move to strategic positions
- No escort behavior — bots don't follow or protect orb carriers
- No enemy orb carrier hunting — bots don't pursue enemy players holding orbs
- No center push execution — bots don't move orb carriers to the center zone

### 2.3 Data Accuracy Issues
- Scoring model in `TempleOfKotmoguData.h` uses a simplified 2-tier system (3pts outside, 15pts center) but the actual TrinityCore server uses 3 concentric area triggers with different point values per 5 seconds:
  - **Small area** (innermost/center): 6 pts per 5s (`Plus5VictoryPoints`)
  - **Medium area** (middle ring): 4 pts per 5s (`Plus4VictoryPoints`)
  - **Large area** (outer ring): 2 pts per 5s (`Plus3VictoryPoints`)
- This doesn't affect bot behavior significantly (center is still best), but the data constants should be accurate for the blueprint to be trustworthy.

---

## 3. Requirements

### 3.1 R1: Orb Pickup (Critical)

**Description:** Bots assigned the ORB_CARRIER role must find, navigate to, and pick up available orbs.

**Behavior:**
1. Query nearby game objects for orb entries (212091-212094)
2. If no orb found nearby, navigate to the nearest available orb position (from TempleOfKotmoguData or dynamic discovery)
3. When within interaction range (~5 yards), use the orb GameObject
4. Verify pickup succeeded by checking for orb possession aura (121175-121178)
5. If orb is already held by another player, move to next priority orb

**Constraints:**
- Must respect faction-based orb priority from `GetOrbPriority()` (Alliance prioritizes NE/NW orbs, Horde prioritizes SE/SW)
- Must not attempt pickup of already-held orbs
- Must handle orb respawn timing (orbs respawn ~immediately after dropping in TrinityCore's implementation via `SpawnOrb()`)

**Success Criteria:** >95% pickup success rate when an available orb exists and bot is assigned ORB_CARRIER role.

### 3.2 R2: Combat Engagement (Critical)

**Description:** Bots must engage in combat with enemy players, especially enemy orb carriers.

**Behavior:**
1. When assigned NODE_ATTACKER or FLAG_HUNTER role: find enemy orb carriers and attack them
2. When assigned FLAG_ESCORT or NODE_DEFENDER role: attack enemies threatening friendly orb carriers
3. Combat sequence: `SetSelection(target)` then `Attack(target, true)` for melee, or cast spells for ranged
4. Target validation: target must be alive, hostile, and in range
5. Priority targeting: enemy orb carriers > enemies near friendly carriers > nearest enemy

**Constraints:**
- Must validate target is alive and hostile before attacking
- Must not attack friendly players
- Must integrate with existing bot combat AI (class-specific rotations)

**Success Criteria:** 100% combat engagement rate when enemies are in range and bot is in a combat role.

### 3.3 R3: Center Push Execution (High)

**Description:** Orb carriers and their escorts must move to the center zone for bonus points when strategically appropriate.

**Behavior:**
1. When `ShouldPushToCenter()` returns true, orb carriers navigate along pre-calculated routes from `GetOrbCarrierRoute()`
2. Escorts maintain formation around the carrier using `GetEscortFormation()`
3. Center detection uses `IsInCenterZone()` (within 25 yards of center point)
4. Once in center, carrier stays in center while escorts defend perimeter

**Constraints:**
- Only push when team has 2+ orbs (CENTER_PUSH_ORB_COUNT)
- Don't push in first 30 seconds (INITIAL_HOLD_TIME)
- Only push when team has equal or more orbs than enemy

**Success Criteria:** Orb carriers reach center zone within 15 seconds of push decision when path is unobstructed.

### 3.4 R4: Dynamic Strategy Execution (High)

**Description:** `ExecuteKotmoguStrategy()` must branch behavior based on the bot's assigned role and the current strategic decision from the coordination layer.

**Behavior:**
1. Query BattlegroundCoordinator for bot's current role
2. Query TempleOfKotmoguScript for current strategy phase and decisions
3. Execute role-specific behavior:
   - **ORB_CARRIER:** Pick up orb → move to center (or hold position if defensive) → stay alive
   - **FLAG_ESCORT:** Follow nearest friendly orb carrier → attack enemies near carrier → use healing/defensive abilities on carrier
   - **NODE_ATTACKER:** Find enemy orb carrier → navigate to them → attack → if carrier dies, pick up dropped orb
   - **NODE_DEFENDER:** Position at orb spawn or center → attack enemies approaching → call for help if outnumbered
   - **ROAMER:** Move between objectives → assist where needed → pick up unattended orbs
4. Respond to strategy phase changes (OPENING → MID_GAME → LATE_GAME → DESPERATE)
5. Handle transitions smoothly (don't abandon current action mid-combat)

**Constraints:**
- Must use existing role assignment from BGRoleManager
- Must respect strategy decisions from TempleOfKotmoguScript::AdjustStrategy()
- Must integrate with existing BattlegroundCoordinator

**Success Criteria:** Bots execute role-appropriate behavior and adapt when strategy phase changes.

### 3.5 R5: Edge Case Handling (Medium)

**Description:** Bots must handle edge cases gracefully without crashing.

**Scenarios:**
1. **Bot death while carrying orb:** Orb drops automatically (handled server-side). Bot should re-enter strategy after respawn.
2. **Orb respawn:** When an orb respawns, nearby bots without orbs should attempt pickup.
3. **Multiple bots targeting same orb:** Only one bot can pick up an orb. Others must re-target to different orbs.
4. **No orbs available:** All 4 orbs held. Bots without orbs should defend carriers or hunt enemy carriers.
5. **Team size imbalance:** Strategy should adapt if team has fewer players (more conservative).
6. **Null pointers:** All player/object lookups must be null-checked.

**Success Criteria:** Zero crashes across all edge cases. Bots recover from unexpected states within one update cycle.

### 3.6 R6: Logging & Observability (Medium)

**Description:** Add detailed logging for debugging and validation during manual testing.

**Log Categories:**
- Orb pickup: attempts, successes, failures with reason
- Combat engagement: target selection, attack initiation, target death
- Strategy transitions: phase changes, role reassignments
- Center push: decisions, movement, arrival
- Errors: null pointers, invalid states, unexpected conditions

**Log Level:** `TC_LOG_DEBUG` for routine events, `TC_LOG_INFO` for state transitions, `TC_LOG_ERROR` for error conditions.

**Logger:** `playerbots.bg.tok` for TOK-specific events, `playerbots.bg` for general BG events.

**Success Criteria:** A tester can reconstruct a full game timeline from logs.

---

## 4. Architecture Context

### 4.1 Three-Layer Architecture

```
Server Layer (TrinityCore)
  battleground_temple_of_kotmogu.cpp
  - Handles orb spawning, aura application, scoring
  - Uses OnFlagTaken/OnFlagDropped for orb events
  - Scoring via 3 area triggers (SmallAura/MediumAura/LargeAura)

Coordination Layer (Playerbot - EXISTS)
  TempleOfKotmoguScript.cpp     → Strategy, phases, orb tracking
  BattlegroundCoordinator.cpp   → State management, script loading
  BGStrategyEngine.cpp          → Strategy evaluation
  BGRoleManager.cpp             → Role assignment/rebalancing
  TempleOfKotmoguData.h         → Positions, constants, game objects

Execution Layer (Playerbot - NEEDS IMPLEMENTATION)
  BattlegroundAI.cpp            → ExecuteKotmoguStrategy(), PickupOrb(), DefendOrbCarrier()
  (New behaviors needed)        → HuntEnemyOrbCarrier(), CenterPush(), EscortCarrier()
```

### 4.2 Key Integration Points

- **BattlegroundCoordinator** provides: `GetObjectives()`, `GetBotRole()`, `GetActiveScript()`, `GetFriendlyPlayers()`, `GetEnemyPlayers()`, spatial queries
- **TempleOfKotmoguScript** provides: `IsOrbHeld()`, `GetOrbHolder()`, `ShouldPushToCenter()`, `GetOrbPriority()`, `GetOrbCarrierRoute()`, `GetEscortFormation()`, `IsInCenter()`, `GetCurrentPhase()`, `AdjustStrategy()`
- **BGRoleManager** provides: `GetRole(player)`, `AssignRole()`, `RebalanceRoles()`
- **BGStrategyEngine** provides: `GetCurrentStrategy()`, `ShouldPush()`, `IsWinning()`, `IsLosing()`

### 4.3 Existing Files to Modify

| File | Changes Needed |
|------|---------------|
| `src/modules/Playerbot/PvP/BattlegroundAI.cpp` | Implement `ExecuteKotmoguStrategy()`, `PickupOrb()`, `DefendOrbCarrier()`. Add `HuntEnemyOrbCarrier()`, center push logic. |
| `src/modules/Playerbot/PvP/BattlegroundAI.h` | Add new method declarations for hunting, escort, center push. |

### 4.4 Files That May Need Minor Updates

| File | Potential Changes |
|------|------------------|
| `TempleOfKotmoguData.h` | Correct scoring model to match server's 3-zone system. |
| `TempleOfKotmoguScript.cpp` | May need additional helper methods exposed. |
| `BattlegroundCoordinator.cpp` | May need bridge methods to expose script data to BattlegroundAI. |

---

## 5. Verified Game Data

### 5.1 GameObject Entries (Verified against server script)

| Orb | Entry | Spawn Position |
|-----|-------|---------------|
| Blue | 212091 | (1716.95, 1250.02, 13.33) |
| Purple | 212092 | (1850.22, 1416.82, 13.34) |
| Green | 212093 | (1716.89, 1416.62, 13.21) |
| Orange | 212094 | (1850.17, 1250.12, 13.21) |

Positions match exactly between `TempleOfKotmoguData.h` and `battleground_temple_of_kotmogu.cpp`.

### 5.2 Spell Auras (From server script)

| Spell | ID | Purpose |
|-------|----|---------|
| Orange Orb Aura | 121175 | Orb possession (periodic damage increase) |
| Blue Orb Aura | 121176 | Orb possession |
| Green Orb Aura | 121177 | Orb possession |
| Purple Orb Aura | 121178 | Orb possession |
| Power Orb Scale | 127163 | Periodic scaling visual |
| Power Orb Immunity | 116524 | Periodic immunity check |
| Small Area Aura | 112052 | Center zone (highest points) |
| Medium Area Aura | 112053 | Middle ring |
| Large Area Aura | 112054 | Outer ring |

### 5.3 Scoring System (From server script)

Points are awarded every 5 seconds to orb holders based on which area trigger zone they're in:
- **Small area (center):** `Plus5VictoryPoints` = 6 points per tick
- **Medium area (mid-ring):** `Plus4VictoryPoints` = 4 points per tick
- **Large area (outer ring):** `Plus3VictoryPoints` = 2 points per tick
- **Outside all zones:** 0 points

Victory condition: Controlled by `WorldStates::MaxPoints` (17388 world state). The actual max score value is set by DBC data (typically 1500).

### 5.4 Map Data

- **Map ID:** 998
- **Center coordinates:** (1732.0, 1287.0, 13.0)
- **Center radius:** ~25 yards (approximate based on area trigger size)
- **Team size:** 10v10
- **Max duration:** 25 minutes

---

## 6. Recently Fixed Bugs (Context)

These bugs have already been fixed in recent commits and should NOT be re-introduced:

1. **Infinite recursion in BGStrategyEngine::GetTimeFactor()** (commit bafa449e92): `GetTimeFactor()` called `IsWinning()` which called `GetWinProbability()` which called `GetTimeFactor()`. Fixed by using `GetScoreFactor()` instead.

2. **Continuous bot spawning during active battlegrounds** (commit 65f8d2c3d6): Bot spawner was creating new bots every update tick instead of only when needed.

3. **Warm pool bot invitation tracking and ToK orb discovery** (commit 30e4b2af31): Fixed tracking of bots invited from warm pool and fixed orb position discovery initialization.

---

## 7. Out of Scope

The following items are explicitly NOT part of this task:

- Implementing other battleground strategies (WSG, AB, AV, etc.) — those will use the blueprint
- Modifying the server-side TrinityCore battleground script
- Modifying database schemas or adding new tables
- Implementing automated testing (manual testing with log analysis is the validation method)
- Performance optimization beyond basic good practices
- Modifying class-specific combat AI (bots will use their existing combat rotations)
- Creating Obsidian documentation (the codebase and this PRD are the documentation)

---

## 8. Success Criteria Summary

| Metric | Target | How Measured |
|--------|--------|-------------|
| Orb pickup success rate | >95% | Log analysis: pickup attempts vs successes |
| Combat engagement rate | 100% | Log analysis: enemies in range vs attack calls |
| Crash count | 0 | Server stability during test session |
| Strategy transitions | Observed | Log analysis: phase changes recorded |
| Center push execution | Functional | Log analysis: carriers reach center zone |
| Build compilation | Clean | Zero errors, minimal warnings |

---

## 9. Assumptions

1. The existing coordination layer (TempleOfKotmoguScript, BGRoleManager, etc.) is correct and usable as-is. We build execution on top of it, not replace it.
2. The BattlegroundCoordinator properly initializes and loads the TempleOfKotmoguScript for map 998.
3. Bot combat AI (class rotations, spell casting) already works — we just need to call `Attack()` to initiate combat.
4. The `Player` class has methods like `GetNearestGameObject()`, `CastSpell()`, `Attack()`, `SetSelection()` available for use.
5. Movement can be initiated via the bot's existing movement system (e.g., `MoveTo()` or motion master).
6. The orb pickup mechanism uses TrinityCore's flag/object interaction system (`OnFlagTaken` callback).

---

## 10. Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|-----------|
| Orb pickup requires specific interaction API not yet known | Medium | High | Research TrinityCore's GameObject::Use() for flag-type objects during tech spec |
| Movement system integration unclear | Medium | Medium | Investigate existing bot movement in BattlegroundAI for other BG types (WSG, AB) |
| Coordination layer has undiscovered bugs | Low | High | Test incrementally — orb pickup first, then combat, then strategy |
| Center zone detection doesn't match server's area triggers | Low | Low | Bot behavior just needs to be "move to center" — exact zone boundaries are server-side |
| Role assignment not working for TOK | Medium | Medium | Verify BGRoleManager integration with TempleOfKotmoguScript during implementation |
