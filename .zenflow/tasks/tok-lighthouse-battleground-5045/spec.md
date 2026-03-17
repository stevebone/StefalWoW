# Technical Specification: TOK Lighthouse Battleground

## 1. Technical Context

### 1.1 Language & Build

- **Language**: C++20
- **Build System**: CMake
- **Build Command**: `cmake --build . --config RelWithDebInfo --target worldserver -- /maxcpucount:4`
- **Module Flag**: `BUILD_PLAYERBOT=1`
- **Compiler**: MSVC 2022 (Windows)

### 1.2 Key Dependencies

- TrinityCore server framework (provides `Player`, `GameObject`, `Battleground`, `MotionMaster` APIs)
- Playerbot module coordination layer (provides `BattlegroundCoordinator`, `TempleOfKotmoguScript`, `BGRoleManager`, `BGStrategyEngine`)
- `BotMovementUtil` (provides movement primitives)
- `BGSpatialQueryCache` (provides O(cells) spatial lookups instead of O(n))

### 1.3 Architecture Overview

```
Server Layer (TrinityCore - READ ONLY, no modifications)
  battleground_temple_of_kotmogu.cpp
  - Orb type: GAMEOBJECT_TYPE_FLAGSTAND
  - Pickup via: go->Use(player) -> OnFlagTaken callback
  - Drop via: aura removal -> OnFlagDropped callback
  - Scoring: 3 area triggers (2/4/6 pts per 5s)
  - Orbs respawn instantly at base on drop

Coordination Layer (Playerbot - EXISTS, minor additions possible)
  TempleOfKotmoguScript.cpp   -> Orb tracking, strategy, phases, routes
  TempleOfKotmoguData.h       -> Positions, constants, entries, auras
  BattlegroundCoordinator.cpp -> State, spatial cache, roles, script bridge
  BGRoleManager.cpp           -> Role assignment (ORB_CARRIER, FLAG_ESCORT, etc.)
  BGStrategyEngine.cpp        -> Strategy evaluation (AGGRESSIVE, DEFENSIVE, etc.)

Execution Layer (Playerbot - PRIMARY IMPLEMENTATION TARGET)
  BattlegroundAI.cpp          -> ExecuteKotmoguStrategy(), PickupOrb(), DefendOrbCarrier()
  BattlegroundAI.h            -> New method declarations
```

---

## 2. Implementation Approach

### 2.1 Core Principle

The coordination layer already handles strategy, role assignment, orb tracking, route planning, and phase management. The execution layer needs to **read decisions from the coordination layer and execute them** using TrinityCore APIs (movement, combat, object interaction).

### 2.2 Pattern: Follow Existing WSG/AB Templates

The codebase already has fully working BG implementations. Every new function follows established patterns:

1. **Object interaction** -> follows `PickupFlag()` (BattlegroundAI.cpp:650-719) and `CaptureBase()` (lines 1151-1196)
2. **Escort behavior** -> follows `EscortFlagCarrier()` (lines 899-1004)
3. **Defense behavior** -> follows `DefendFlagRoom()` (lines 1006-1101)
4. **Role-based dispatch** -> follows `ExecuteWSGStrategy()` (lines 585-648)
5. **Target finding** -> follows `FindEnemyFlagCarrier()` (lines 847-897)
6. **Spatial queries** -> uses coordinator's O(cells) cache consistently

### 2.3 Files to Modify

| File | Changes |
|------|---------|
| `src/modules/Playerbot/PvP/BattlegroundAI.cpp` | Replace 3 stub functions, add 3 new functions |
| `src/modules/Playerbot/PvP/BattlegroundAI.h` | Add 3 new method declarations |

### 2.4 Files NOT Modified

| File | Reason |
|------|--------|
| `TempleOfKotmoguScript.cpp` | Coordination layer already complete; no new APIs needed |
| `TempleOfKotmoguData.h` | Data constants already accurate (scoring model is simplified but functionally correct for bot decision-making; center = best, which is what matters) |
| `BattlegroundCoordinator.cpp` | Bridge methods already exist |
| `BGRoleManager.cpp` | Role definitions already handle TOK (ORB_CARRIER, FLAG_ESCORT, FLAG_HUNTER) |
| Server-side scripts | Out of scope; read-only reference |

---

## 3. Source Code Structure Changes

### 3.1 BattlegroundAI.h - New Method Declarations

Add 3 new public methods after the existing TOK declarations (after line 301):

```cpp
// Temple of Kotmogu
void ExecuteKotmoguStrategy(::Player* player);    // EXISTS - to be reimplemented
bool PickupOrb(::Player* player);                  // EXISTS - to be reimplemented
bool DefendOrbCarrier(::Player* player);           // EXISTS - to be reimplemented
bool HuntEnemyOrbCarrier(::Player* player);        // NEW
bool EscortOrbCarrier(::Player* player);           // NEW
bool ExecuteOrbCarrierMovement(::Player* player);  // NEW
```

### 3.2 BattlegroundAI.cpp - Function Implementations

#### 3.2.1 `ExecuteKotmoguStrategy()` (Replace stub at line 1569)

**Purpose**: Main dispatch function. Reads bot's role from coordinator and delegates to role-specific behavior.

**Logic**:
1. Get `BattlegroundCoordinator*` via `sBGCoordinatorMgr->GetCoordinatorForPlayer(player)`
2. If no coordinator, fall back to simple behavior (PickupOrb or patrol)
3. Get `IBGScript*` via `coordinator->GetScript()`, cast to `TempleOfKotmoguScript*`
4. Get bot's role via `coordinator->GetBotRole(player->GetGUID())`
5. Dispatch by role:
   - `ORB_CARRIER`: If not holding orb -> `PickupOrb()`. If holding orb -> `ExecuteOrbCarrierMovement()`
   - `FLAG_ESCORT`: `EscortOrbCarrier()`
   - `FLAG_HUNTER` / `NODE_ATTACKER`: `HuntEnemyOrbCarrier()`
   - `NODE_DEFENDER`: `DefendOrbCarrier()`
   - `ROAMER`: If unattended orb available -> `PickupOrb()`. Otherwise -> `HuntEnemyOrbCarrier()`
   - `UNASSIGNED` / default: `PickupOrb()` (grab an orb if possible)

**Coordination layer calls**:
- `coordinator->GetBotRole(guid)` -> current role
- `tokScript->IsPlayerHoldingOrb(guid)` -> check if bot already has orb
- `tokScript->GetCurrentPhase()` -> available through `AdjustStrategy()` results
- `coordinator->GetStrategyEngine()->GetCurrentStrategy()` -> current strategy

**Key design decision**: The function checks if the bot is already holding an orb before deciding behavior. An ORB_CARRIER that already has an orb executes carrier movement (center push or hold); one without an orb executes pickup.

#### 3.2.2 `PickupOrb()` (Replace stub at line 1579)

**Purpose**: Find an available orb, navigate to it, and use the GameObject to pick it up.

**Logic**:
1. Check if player already holds an orb (check auras 121175-121178). If yes, return true (already done).
2. Get coordinator and TOK script. If available, get prioritized orb list from `tokScript->GetOrbPriority(faction)`.
3. For each orb in priority order:
   a. Check if orb is already held: `tokScript->IsOrbHeld(orbId)`. Skip if held.
   b. Get orb position: `tokScript->GetDynamicOrbPosition(orbId)` (falls back to hardcoded positions).
   c. Calculate distance to orb position.
4. Select the nearest unheld orb from the priority list.
5. If no orb available (all 4 held), return false.
6. If distance > `OBJECTIVE_RANGE` (10.0f): call `BotMovementUtil::MoveToPosition(player, orbPos)` and return true (in progress).
7. If within range: search nearby GameObjects using `player->GetGameObjectListWithEntryInGrid(goList, orbEntry, OBJECTIVE_RANGE)`.
8. For each found GO: verify `goInfo->type == GAMEOBJECT_TYPE_FLAGSTAND`, verify `go->IsWithinDistInMap(player, OBJECTIVE_RANGE)`, then call `go->Use(player)` and return true.
9. If no GO found in range despite being at position: log warning and return false (orb may have been picked up between checks).

**Orb entry to check**: Use constants from `TempleOfKotmoguData.h`:
- `TokData::GameObjects::BLUE_ORB` (212091)
- `TokData::GameObjects::PURPLE_ORB` (212092)
- `TokData::GameObjects::GREEN_ORB` (212093)
- `TokData::GameObjects::ORANGE_ORB` (212094)

**Orb auras to check** (for "already holding" detection):
- `TokData::Spells::ORANGE_ORB_AURA` (121175)
- `TokData::Spells::BLUE_ORB_AURA` (121176)
- `TokData::Spells::GREEN_ORB_AURA` (121177)
- `TokData::Spells::PURPLE_ORB_AURA` (121178)

**Conflict resolution**: When multiple bots target the same orb, the first to call `go->Use(player)` wins. Others will find `IsOrbHeld()` returns true on next tick and re-target.

**Template**: Follows `PickupFlag()` (line 650) and `CaptureBase()` (line 1151) patterns exactly.

#### 3.2.3 `DefendOrbCarrier()` (Replace stub at line 1591)

**Purpose**: Position near a friendly orb carrier and attack enemies that threaten them.

**Logic**:
1. Get coordinator and TOK script.
2. Find a friendly orb carrier to defend:
   a. Using coordinator: iterate `tokScript->GetOrbHolder(orbId)` for all 4 orbs. Filter for same-faction holders.
   b. If multiple friendly carriers, choose the nearest one.
   c. If none found, fall back to patrolling center area.
3. If friendly carrier found:
   a. Check distance to carrier. If > 30.0f (DEFENSE_RADIUS): `BotMovementUtil::MoveToPosition()` toward carrier.
   b. If within 30.0f: check for nearby enemies using `coordinator->QueryNearbyEnemies(carrierPos, 30.0f)`.
   c. If enemies found near carrier: `player->SetSelection(enemy->GetGUID())` then `player->Attack(enemy, true)` for the nearest/most dangerous.
   d. If no enemies: maintain escort distance (8.0f behind carrier using angle-based positioning, same as `EscortFlagCarrier`).
4. If no carrier found: patrol around center or nearest orb spawn using `BotMovementUtil::MoveRandomAround()`.

**Template**: Follows `DefendFlagRoom()` (line 1006) and `EscortFlagCarrier()` (line 899) patterns.

#### 3.2.4 `HuntEnemyOrbCarrier()` (NEW)

**Purpose**: Find and attack enemy players who are carrying orbs.

**Logic**:
1. Get coordinator and TOK script.
2. Find enemy orb carriers:
   a. Iterate all 4 orbs via `tokScript->GetOrbHolder(orbId)`.
   b. For each held orb, get the holder player via `ObjectAccessor::FindPlayer(guid)`.
   c. Filter for enemy faction (not same team as bot).
   d. Select nearest enemy carrier.
3. If enemy carrier found:
   a. Check distance. If > combat range (30.0f): `BotMovementUtil::ChaseTarget(player, enemyCarrier, 5.0f)`.
   b. If within combat range: `player->SetSelection(enemyCarrier->GetGUID())` then `player->Attack(enemyCarrier, true)`.
4. If no enemy carrier found:
   a. Use spatial cache: `coordinator->GetNearestEnemy(playerPos, 40.0f)`.
   b. If enemy found: attack them.
   c. If no enemies: move to a strategic position (center or nearest contested orb spawn).

**Template**: Follows `FindEnemyFlagCarrier()` (line 847) + `ExecuteFlagHunterBehavior()` patterns.

**Priority targeting**: Enemy carriers in center zone (earning 6pts/tick) are higher priority than those outside.

#### 3.2.5 `EscortOrbCarrier()` (NEW)

**Purpose**: Follow and protect a friendly orb carrier as they move to center.

**Logic**:
1. Get coordinator and TOK script.
2. Find nearest friendly orb carrier (same logic as DefendOrbCarrier step 2).
3. If carrier found:
   a. Calculate escort position using trigonometric positioning (behind carrier at 8.0f distance).
   b. Use `BotMovementUtil::MoveToPosition()` to escort position.
   c. If enemies approach within 15.0f of carrier: engage them with `Attack()`.
   d. If carrier is in combat: assist by attacking carrier's attacker.
4. If no carrier found: fall back to `DefendOrbCarrier()` behavior.

**Template**: Follows `EscortFlagCarrier()` (line 899) pattern exactly.

**Escort formation**: Uses `tokScript->GetEscortFormation(carrierX, carrierY, carrierZ)` which returns 8 positions (4 melee-range, 4 ranged) around the carrier.

#### 3.2.6 `ExecuteOrbCarrierMovement()` (NEW)

**Purpose**: Once a bot holds an orb, move toward center zone for maximum scoring.

**Logic**:
1. Get coordinator and TOK script.
2. Check if center push is appropriate: `tokScript->ShouldPushToCenter(faction)`.
   - Requires 2+ friendly orbs held and >= 30s elapsed.
3. If should push to center:
   a. Get route from `tokScript->GetOrbCarrierRoute(orbId)` (4-5 waypoints to center).
   b. Navigate along waypoints using `BotMovementUtil::MoveToPosition()`.
   c. Once in center (within `CENTER_RADIUS` = 25.0f of center), hold position.
   d. If in center and enemies approach: consider retreating if outnumbered (check `coordinator->CountEnemiesInRadius(centerPos, 30.0f)` vs allies).
4. If should NOT push to center:
   a. Hold near orb spawn (defensive positioning).
   b. Use `tokScript->GetOrbDefensePositions(orbId)` for defensive spots.
   c. Stay alive — carrier dying loses the orb.
5. Survival priority: If health < 30% and outnumbered, move away from enemies.

**Center position**: `TokData::CENTER_X` (1732.0f), `TokData::CENTER_Y` (1287.0f), `TokData::CENTER_Z` (13.0f).

---

## 4. Data Model / Interface Changes

### 4.1 No Database Changes

No schema changes, no new tables, no migrations required.

### 4.2 No New Configuration

No config file changes needed. All constants come from `TempleOfKotmoguData.h`.

### 4.3 Header Changes (BattlegroundAI.h)

Three new method declarations added to the public section of the `BattlegroundAI` class:

```cpp
bool HuntEnemyOrbCarrier(::Player* player);
bool EscortOrbCarrier(::Player* player);
bool ExecuteOrbCarrierMovement(::Player* player);
```

No new member variables. No new structs. No new enums.

### 4.4 API Contracts with Coordination Layer

The execution layer calls these coordination APIs (all already exist):

| API Call | Returns | Used By |
|----------|---------|---------|
| `sBGCoordinatorMgr->GetCoordinatorForPlayer(player)` | `BattlegroundCoordinator*` or `nullptr` | All functions |
| `coordinator->GetScript()` | `IBGScript*` | All functions |
| `coordinator->GetBotRole(guid)` | `BGRole` enum | `ExecuteKotmoguStrategy` |
| `coordinator->QueryNearbyEnemies(pos, radius)` | `vector<BGPlayerSnapshot*>` | Combat functions |
| `coordinator->GetNearestEnemy(pos, radius, &dist)` | `BGPlayerSnapshot*` | Combat functions |
| `coordinator->CountEnemiesInRadius(pos, radius)` | `uint32` | `ExecuteOrbCarrierMovement` |
| `coordinator->CountAlliesInRadius(pos, radius)` | `uint32` | `ExecuteOrbCarrierMovement` |
| `tokScript->IsOrbHeld(orbId)` | `bool` | `PickupOrb` |
| `tokScript->GetOrbHolder(orbId)` | `ObjectGuid` | `DefendOrbCarrier`, `HuntEnemyOrbCarrier`, `EscortOrbCarrier` |
| `tokScript->IsPlayerHoldingOrb(guid)` | `bool` | `ExecuteKotmoguStrategy` |
| `tokScript->GetOrbPriority(faction)` | `vector<uint32>` | `PickupOrb` |
| `tokScript->GetDynamicOrbPosition(orbId)` | `Position` | `PickupOrb` |
| `tokScript->ShouldPushToCenter(faction)` | `bool` | `ExecuteOrbCarrierMovement` |
| `tokScript->GetOrbCarrierRoute(orbId)` | `vector<Position>` | `ExecuteOrbCarrierMovement` |
| `tokScript->GetEscortFormation(x, y, z)` | `vector<Position>` | `EscortOrbCarrier` |
| `tokScript->GetOrbDefensePositions(orbId)` | `vector<Position>` | `ExecuteOrbCarrierMovement` |
| `tokScript->IsInCenter(x, y)` | `bool` | `ExecuteOrbCarrierMovement`, `HuntEnemyOrbCarrier` |

---

## 5. Delivery Phases

### Phase 1: Orb Pickup (Foundation)

**Scope**: Implement `PickupOrb()` and the basic `ExecuteKotmoguStrategy()` dispatch (ORB_CARRIER role only).

**What to implement**:
- Replace `PickupOrb()` stub with full implementation
- Replace `ExecuteKotmoguStrategy()` stub with role-based dispatch (initially just ORB_CARRIER -> PickupOrb)
- Add "already holding orb" detection via aura checks
- Add orb priority selection via coordination layer
- Add distance check + movement to orb
- Add `GetGameObjectListWithEntryInGrid()` + `go->Use(player)` for actual pickup
- Add logging for pickup attempts, successes, failures

**Verification**:
- Build compiles clean
- Log output shows orb pickup attempts with correct entry IDs
- Bot navigates to orb position and attempts `Use()`

**Estimated scope**: ~120 lines of implementation code

### Phase 2: Combat Engagement

**Scope**: Implement `HuntEnemyOrbCarrier()` and `DefendOrbCarrier()`.

**What to implement**:
- `HuntEnemyOrbCarrier()`: Find enemy carriers via coordination layer, chase with `ChaseTarget()`, engage with `SetSelection()` + `Attack()`
- `DefendOrbCarrier()`: Find friendly carrier, position nearby, attack threats
- Wire both into `ExecuteKotmoguStrategy()` dispatch for FLAG_HUNTER and NODE_DEFENDER roles
- Add `BattlegroundAI.h` declarations for `HuntEnemyOrbCarrier()`
- Add logging for combat engagement decisions

**Verification**:
- Build compiles clean
- Bots with FLAG_HUNTER role chase and attack enemy carriers
- Bots with NODE_DEFENDER role position near friendly carriers

**Estimated scope**: ~150 lines of implementation code

### Phase 3: Escort & Center Push

**Scope**: Implement `EscortOrbCarrier()` and `ExecuteOrbCarrierMovement()`.

**What to implement**:
- `EscortOrbCarrier()`: Follow carrier using escort formation, assist in combat
- `ExecuteOrbCarrierMovement()`: Center push logic when conditions met, defensive hold otherwise
- Add `BattlegroundAI.h` declarations for both new methods
- Wire into `ExecuteKotmoguStrategy()` for FLAG_ESCORT and ORB_CARRIER (holding orb) roles
- Complete the ROAMER role dispatch

**Verification**:
- Build compiles clean
- Orb carriers move to center when conditions met (2+ orbs, 30s+ elapsed)
- Escorts follow carrier in formation
- Strategy adaptation visible in logs

**Estimated scope**: ~180 lines of implementation code

### Phase 4: Polish & Edge Cases

**Scope**: Edge case handling, logging completeness, code quality.

**What to implement**:
- Handle bot death with orb (no special code needed — server handles drop; bot re-enters strategy after respawn)
- Handle all-orbs-held scenario (bots without orbs defend or hunt)
- Handle no coordinator fallback (basic orb pickup + patrol)
- Add strategy/phase transition logging
- Null pointer safety audit on all new code
- Remove any remaining TODO comments
- Verify consistent code style with rest of file

**Verification**:
- Build compiles clean with zero warnings from modified files
- All code paths have null checks
- No crashes in edge case scenarios
- Complete log coverage for debugging

**Estimated scope**: ~50 lines of additional code + modifications

---

## 6. Verification Approach

### 6.1 Build Verification

```bash
cmake --build . --config RelWithDebInfo --target worldserver -- /maxcpucount:4
```

- Zero compilation errors
- Zero warnings from modified files (BattlegroundAI.cpp, BattlegroundAI.h)

### 6.2 Static Analysis (Manual Review)

For each function:
- All pointer dereferences preceded by null check
- No uninitialized variables
- No memory leaks (no raw `new` — use stack objects and existing framework)
- All container iterations use const references where applicable
- `ObjectAccessor::FindPlayer()` results null-checked
- `coordinator->GetScript()` result null-checked and type-verified

### 6.3 Runtime Testing (Manual)

Configure logging:
```
Logger.playerbots.bg.tok=6,Console Server File
```

**Test Scenarios**:

| # | Scenario | Expected Behavior | Metric |
|---|----------|-------------------|--------|
| 1 | Bot near available orb | Bot navigates to orb, calls Use(), gains aura | Orb pickup >95% |
| 2 | All orbs held | Bot defends carrier or hunts enemies | No crashes |
| 3 | Enemy holds orb | Hunter bot chases and attacks enemy | 100% engagement |
| 4 | Friendly holds orb | Escort bot follows carrier | Maintains 8yd distance |
| 5 | 2+ friendly orbs | Carriers push to center | Center reached <15s |
| 6 | Bot dies with orb | Orb drops, bot re-enters strategy | No crash, orb respawns |
| 7 | Score differential | Strategy shifts (AGGRESSIVE/DEFENSIVE) | Logged transitions |
| 8 | Full 10v10 game | Game runs to completion | Zero crashes |

### 6.4 Log Validation

Logs should show:
- `[TOK] Player {guid} role={role} strategy={strategy}` — on each update
- `[TOK] PickupOrb: targeting orb {id} ({name}) at ({x},{y},{z}), dist={d}` — on approach
- `[TOK] PickupOrb: Used orb {id} ({name}) successfully` — on pickup
- `[TOK] PickupOrb: No available orbs (all held)` — when all orbs taken
- `[TOK] HuntEnemy: Found enemy carrier {guid} holding orb {id}, dist={d}` — on target acquisition
- `[TOK] HuntEnemy: Engaging enemy carrier {guid}` — on attack start
- `[TOK] DefendCarrier: Defending carrier {guid} at ({x},{y},{z})` — on escort
- `[TOK] CenterPush: Moving carrier to center via route ({waypoints})` — on push decision
- `[TOK] CenterPush: Carrier in center zone, holding position` — on arrival

---

## 7. Risk Mitigation

### 7.1 Orb Pickup May Fail Silently

**Risk**: `go->Use(player)` may not trigger pickup for bots (server-side validation might reject bot interactions).

**Mitigation**: After calling `Use()`, verify on the next tick that the bot has the corresponding orb aura. If not, log failure reason and retry. The server-side code uses `GAMEOBJECT_TYPE_FLAGSTAND` which is the same type WSG flags use — and WSG flag pickup already works for bots via the same `Use()` API.

### 7.2 Coordination Layer Returns Stale Data

**Risk**: `IsOrbHeld()` may return stale data if the coordination layer hasn't processed the `ORB_PICKED_UP` event yet.

**Mitigation**: Always check the actual aura on the target player as a secondary validation, not just the coordination layer's tracking.

### 7.3 Multiple Bots Race for Same Orb

**Risk**: Multiple bots converge on same orb, wasting time.

**Mitigation**: Accept this as normal behavior. The first bot to `Use()` gets the orb. Others will re-evaluate on next tick (500ms interval) and target a different orb. The orb priority system (faction-based) already distributes bots across different orbs naturally.

### 7.4 Movement Interruption During Combat

**Risk**: `MoveToPosition()` could override combat movement.

**Mitigation**: `BotMovementUtil::MoveToPosition()` already never interrupts `CHASE_MOTION_TYPE` (combat movement). This is built into the movement utility. Additionally, check `player->IsInCombat()` before issuing non-combat movement commands.

### 7.5 TempleOfKotmoguScript Cast Failure

**Risk**: `coordinator->GetScript()` returns non-TOK script or null.

**Mitigation**: Always verify the script's `GetBGType() == BGType::TEMPLE_OF_KOTMOGU` before casting. Fall back to basic behavior (grab nearest orb, attack nearest enemy) if script unavailable.

---

## 8. Constants Reference

All constants from `TempleOfKotmoguData.h` used by the execution layer:

```
Map ID:           998
Center:           (1732.0, 1287.0, 13.0)
Center Radius:    25.0 yards
Orb Count:        4
Max Score:         1500
Team Size:        10

Orb Entries:      212091 (Blue), 212092 (Purple), 212093 (Green), 212094 (Orange)
Orb Auras:        121176 (Blue), 121178 (Purple), 121177 (Green), 121175 (Orange)

Objective Range:  10.0 yards (interaction distance)
Defense Radius:   30.0 yards (patrol/defend perimeter)
Escort Distance:  8.0 yards (behind carrier)
Chase Distance:   5.0 yards (melee attack range)
Spatial Query:    40.0 yards (default enemy/ally search radius)

Center Push:      Requires 2+ friendly orbs + 30s elapsed
Update Interval:  500ms (BG_UPDATE_INTERVAL)
```

---

## 9. Code Style & Conventions

All new code follows existing patterns in BattlegroundAI.cpp:

- Section headers: `// ============================================================================`
- Null checks: `if (!player) return false;`
- Coordinator access: `BattlegroundCoordinator* coordinator = sBGCoordinatorMgr->GetCoordinatorForPlayer(player);`
- Spatial cache preference: Always try coordinator cache before grid search fallback
- Logging: `TC_LOG_DEBUG("playerbots.bg", "BattlegroundAI: ...")` for routine events
- `::Player*` with explicit global scope prefix
- `ObjectGuid` by value, `Position` by const reference
- No raw `new`/`delete`; use stack objects and framework-managed pointers
