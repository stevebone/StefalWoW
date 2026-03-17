# Product Requirements Document: Enterprise-Grade Movement & Pathfinding System Redesign

## Executive Summary

This document outlines the requirements for redesigning TrinityCore's movement and pathfinding system to support an enterprise-grade AI bot system capable of controlling 1000+ concurrent player-controlled characters with human-like movement behavior. The current system exhibits critical defects in collision detection, ground validation, swimming mechanics, and stuck recovery that make it unsuitable for large-scale bot operations.

## Project Context

### Current System Overview

**Technology Stack:**
- **Engine**: TrinityCore (World of Warcraft server emulator)
- **Language**: C++ (C++17/20)
- **Navigation**: Recast/Detour Navigation Mesh (MMAPs)
- **Architecture**: MotionMaster + MovementGenerator pattern

**Existing Movement Architecture:**
```
MotionMaster (per Unit)
│
├── MovementGenerator (abstract base)
│   ├── IdleMovementGenerator
│   ├── ChaseMovementGenerator  
│   ├── FollowMovementGenerator
│   ├── PointMovementGenerator
│   ├── FleeingMovementGenerator
│   ├── RandomMovementGenerator
│   ├── WaypointMovementGenerator
│   └── FormationMovementGenerator
│
├── PathGenerator (uses Detour NavMesh)
├── MoveSpline (smooth movement interpolation)
└── AbstractFollower (shared follow/chase logic)
```

**Key Existing Files:**
- `src/server/game/Movement/MotionMaster.{h,cpp}` - Movement orchestration
- `src/server/game/Movement/PathGenerator.{h,cpp}` - Navigation mesh pathfinding
- `src/server/game/Movement/MovementGenerators/*` - Movement behavior implementations
- `src/server/game/AI/PlayerAI/PlayerAI.{h,cpp}` - Player AI control (charmed players)
- `src/server/game/AI/CoreAI/PetAI.{h,cpp}` - Pet AI control (reference implementation)

### Target System

The redesigned system must support:
- **Scale**: 1000+ concurrent AI-controlled player characters (bots)
- **Use Case**: Player bots with human-like movement behavior
- **Quality**: Enterprise-grade reliability, maintainability, and performance
- **Integration**: Seamless integration with TrinityCore's existing infrastructure

## Critical Problems

### 1. **P0: Bots Walk Through Walls**
**Severity**: Critical - Game Breaking
**Description**: AI-controlled units ignore solid obstacles and walk through walls, buildings, and terrain geometry.

**Root Causes (to be validated during investigation):**
- PathGenerator may not properly validate collision with static geometry
- Movement execution may skip line-of-sight (LOS) checks
- MMap polygons may not accurately represent walkable areas
- Movement validation may be disabled or insufficient

**User Impact**: 
- Bots appear obviously non-human
- Can access restricted areas
- Breaks immersion and game integrity

### 2. **P0: Bots Walk Into Void/Off Cliffs**
**Severity**: Critical - Game Breaking
**Description**: AI units walk off edges, into empty space, or fall to their death without detecting dangerous terrain.

**Root Causes (to be validated):**
- No ground height validation before movement
- PathGenerator doesn't check for valid Z-coordinate
- Missing edge detection in pathfinding
- No "fall damage zone" awareness

**User Impact**:
- Bots die unexpectedly
- Disrupts automation workflows
- Appears obviously non-human

### 3. **P0: Bots Don't Swim - They Hop**
**Severity**: Critical - Behavior Breaking
**Description**: When encountering water, bots "hop" across the surface instead of swimming naturally.

**Root Causes (to be validated):**
- Swimming movement state not properly set
- Movement flags (`MOVEMENTFLAG_SWIMMING`) not applied
- PathGenerator treats water as solid ground
- No liquid detection in movement execution
- Missing swim animation triggers

**User Impact**:
- Obviously non-human behavior
- May get stuck in water
- Breaks immersion

### 4. **P0: Bots Hop Through Air**
**Severity**: High - Behavior Breaking  
**Description**: Bots randomly jump or "hop" while moving on flat ground without reason.

**Root Causes (to be validated):**
- Gravity not properly applied during movement
- Z-coordinate interpolation issues in spline movement
- Attempting to "jump over" minor obstacles
- Incorrect height adjustment during pathfinding
- Missing fall state detection

**User Impact**:
- Unnatural movement patterns
- Easily detectable as bots
- May trigger anti-cheat systems

### 5. **P0: Bots Get Stuck**
**Severity**: High - Operational Breaking
**Description**: Bots frequently get stuck in geometry, corners, or obstacles with no recovery mechanism.

**Root Causes (to be validated):**
- No stuck detection system
- No timeout-based recovery
- PathGenerator doesn't detect path failure
- No alternative path calculation on stuck
- Missing collision avoidance

**User Impact**:
- Requires manual intervention
- Breaks automation
- Reduces system reliability

### 6. **P1: Bots Take Weird Paths**
**Severity**: Medium - Quality Issue
**Description**: Bots take inefficient, unnatural, or nonsensical routes to destinations.

**Root Causes (to be validated):**
- PathGenerator cost heuristics not optimized
- No path smoothing or corner cutting
- Inefficient waypoint selection
- Missing direct line-of-sight shortcuts
- Poor handling of dynamic obstacles

**User Impact**:
- Unnatural movement patterns
- Increased travel time
- Detectable as non-human

## Business Goals

### Primary Objectives

1. **Eliminate Critical Movement Bugs**: Fix all P0 issues to achieve 99.9% reliable movement
2. **Enable Large-Scale Bot Operations**: Support 1000+ concurrent bots without performance degradation
3. **Human-Like Movement**: Bots should be indistinguishable from human players in movement patterns
4. **Zero Stuck States**: Implement robust detection and recovery to eliminate manual intervention
5. **Maintainable Codebase**: Create clean, documented, testable code following enterprise standards

### Success Metrics

| Metric | Current (Baseline) | Target |
|--------|-------------------|--------|
| Wall collision violations | Unknown (High) | < 0.1% of movements |
| Void falls per 1000 movements | Unknown (High) | < 0.01% |
| Stuck incidents per bot/hour | Unknown (High) | < 0.1 |
| Swimming behavior correctness | ~0% | 99%+ |
| Path optimality (vs. optimal) | Unknown | 90%+ |
| Recovery success rate | ~0% | 95%+ |
| Avg. path calculation time | Unknown | < 5ms (p95) |
| System supports concurrent bots | Unknown | 1000+ |

### Non-Goals (Out of Scope)

- Modifying core TrinityCore systems (Map, World, DB schemas)
- Rewriting Recast/Detour navigation mesh generation
- Implementing new movement types beyond existing (no flying mounts, etc.)
- Anti-detection/anti-cheat evasion features
- Cross-server or networked bot coordination
- Machine learning or advanced AI decision-making

## User Stories & Requirements

### Epic 1: Collision Detection & Validation

**US-1.1**: As a bot system operator, I need bots to respect solid geometry so they don't walk through walls.
- **Acceptance Criteria:**
  - PathGenerator validates all waypoints against collision geometry
  - Line-of-sight checks performed before movement execution
  - Movement blocked if collision detected
  - Logged warnings when collision would occur
  - Alternative path calculated on collision

**US-1.2**: As a bot system operator, I need bots to detect cliffs/void so they don't fall to their death.
- **Acceptance Criteria:**
  - Ground height validated for all path waypoints
  - Edge detection prevents movement into void
  - Z-coordinate validated against map geometry
  - Falling state properly detected and handled
  - Bots avoid "unsafe" terrain (lava, deep water without swim capability)

### Epic 2: Environmental Movement (Water, Air, Ground)

**US-2.1**: As a bot system operator, I need bots to swim naturally when in water.
- **Acceptance Criteria:**
  - Liquid detection triggers swimming state
  - `MOVEMENTFLAG_SWIMMING` properly set/unset
  - Swimming animation plays correctly
  - Vertical movement (up/down) works in water
  - Transition from ground → water → ground is smooth
  - Bots surface for air if needed (breath mechanic)

**US-2.2**: As a bot system operator, I need bots to move naturally on ground without random jumping.
- **Acceptance Criteria:**
  - Gravity applied during all movement
  - Z-coordinate smoothly follows terrain contours
  - No "hopping" on flat surfaces
  - Jumps only occur when intentionally commanded
  - Falling state properly detected (edges, knockback)
  - Fall damage correctly applied

**US-2.3**: As a bot system operator, I need bots with flying capability to fly correctly (if applicable).
- **Acceptance Criteria:**
  - Flying units respect flight path height
  - No collision with ground during flight
  - Takeoff/landing transitions smooth
  - Flight path avoids obstacles
  - *(Note: Lower priority - validate if needed)*

### Epic 3: Stuck Detection & Recovery

**US-3.1**: As a bot system operator, I need bots to detect when they're stuck.
- **Acceptance Criteria:**
  - Position tracking detects no movement for N seconds
  - Failed path attempts increment stuck counter
  - Collision detection failures trigger stuck state
  - Different stuck types identified (geometry, pathing, combat)
  - Stuck logged with diagnostics (position, last path, time)

**US-3.2**: As a bot system operator, I need bots to automatically recover from stuck states.
- **Acceptance Criteria:**
  - Incremental recovery strategies: 
    1. Recalculate path
    2. Back up and retry
    3. Move to random nearby valid position
    4. Teleport to last known-good position
    5. Evade/reset if all else fails
  - Recovery attempts logged
  - Recovery success/failure tracked
  - Maximum retry limits prevent infinite loops
  - Critical failures escalated (alert operator)

### Epic 4: Path Quality & Optimization

**US-4.1**: As a bot system operator, I need bots to take natural, optimal paths.
- **Acceptance Criteria:**
  - Paths follow natural player routes
  - Direct line-of-sight shortcuts used when safe
  - Path smoothing reduces waypoint count
  - Corner cutting where appropriate
  - Avoidance of obviously "weird" routes
  - Cost function prioritizes natural paths

**US-4.2**: As a bot system operator, I need path calculation to be performant at scale.
- **Acceptance Criteria:**
  - p95 path calculation < 5ms
  - p99 path calculation < 20ms
  - Caching for frequently-used paths
  - Async path calculation doesn't block game loop
  - Performance scales to 1000+ concurrent bots
  - Path calculation timeouts prevent hangs

### Epic 5: Movement State Machine

**US-5.1**: As a developer, I need a clear state machine for movement states.
- **Acceptance Criteria:**
  - States: Idle, Ground, Swimming, Flying, Falling, Stuck, Evading
  - Valid state transitions defined
  - State entry/exit hooks
  - State persisted across updates
  - State changes logged for debugging
  - Invalid state transitions prevented

**US-5.2**: As a developer, I need movement states to correctly handle environmental changes.
- **Acceptance Criteria:**
  - Ground → Water transition triggers Swimming
  - Water → Ground transition triggers Ground
  - Ground lost → Falling
  - Falling → Ground on landing
  - Stuck detection works in all states
  - Environmental changes force state re-evaluation

### Epic 6: Integration & Compatibility

**US-6.1**: As a developer, I need the new system to integrate with existing TrinityCore infrastructure.
- **Acceptance Criteria:**
  - Uses existing MotionMaster interface
  - Compatible with existing MovementGenerator API
  - Works with current PathGenerator/MMap system
  - No breaking changes to public APIs
  - Backward compatible with existing scripts
  - Existing PetAI/CreatureAI continues to work

**US-6.2**: As a bot operator, I need a clean API for bot movement control.
- **Acceptance Criteria:**
  - Simple API: MoveTo(position), Follow(target), etc.
  - Status query API: IsStuck(), GetCurrentPath(), etc.
  - Configuration API: SetStuckThreshold(), EnableSwimming(), etc.
  - Callback/event system for movement completion
  - Logging and diagnostics API

### Epic 7: Testing & Quality

**US-7.1**: As a developer, I need comprehensive testing for movement.
- **Acceptance Criteria:**
  - Unit tests for PathGenerator validation
  - Integration tests for movement scenarios
  - Performance benchmarks for path calculation
  - Stress tests with 1000+ bots
  - Test maps with known problematic geometry
  - Automated regression tests

**US-7.2**: As a developer, I need debugging tools for movement issues.
- **Acceptance Criteria:**
  - Visual path debugging (in-game or external tool)
  - Detailed movement logs (configurable verbosity)
  - Metrics collection (stuck rate, path quality, perf)
  - Diagnostic commands (`.bot path`, `.bot stuck`, etc.)
  - Replay system for reproducing issues

## Functional Requirements

### FR-1: Position Validation System

**FR-1.1**: Validate Destination Position
- Check coordinates are within map bounds
- Verify Z-coordinate is valid (not void/sky)
- Confirm position is walkable (NavMesh polygon exists)
- Check for hazards (lava, fatigue zones)
- Return validation result + reason for failure

**FR-1.2**: Validate Path Waypoints
- Every waypoint validated (not just start/end)
- Check line-of-sight between consecutive waypoints
- Verify no collision with static geometry
- Confirm terrain is traversable
- Check for environmental hazards

**FR-1.3**: Ground Height Detection
- Query map for ground Z at (X, Y)
- Handle cases with multiple Z levels (bridges, caves)
- Detect when no ground exists (void, air)
- Return liquid height if present
- Cache results for performance

### FR-2: Pathfinding System

**FR-2.1**: Path Generation
- Use existing PathGenerator with enhanced validation
- Calculate path using NavMesh (MMAPs)
- Apply cost heuristics for natural paths
- Smooth path to reduce waypoints
- Limit path length to prevent excessive calculation

**FR-2.2**: Path Validation
- Validate entire path before execution
- Check collision for all segments
- Verify environmental transitions (land ↔ water)
- Confirm destination reachability
- Return path quality metrics

**FR-2.3**: Path Caching
- Cache frequent paths (by hash of start/end)
- Invalidate cache on map changes
- LRU eviction policy
- Per-bot and global caches
- Cache hit metrics

**FR-2.4**: Fallback Strategies
- Direct line if no NavMesh available
- Partial path if complete path fails
- Alternative destinations if primary unreachable
- Safe position fallback on complete failure

### FR-3: Movement Execution

**FR-3.1**: Movement State Management
- State machine: Idle ↔ Ground ↔ Swimming ↔ Flying ↔ Falling ↔ Stuck
- State-specific movement handlers
- State transition validation
- State persistence across updates
- State change callbacks

**FR-3.2**: Movement Flags
- Properly set/unset movement flags:
  - `MOVEMENTFLAG_WALKING`
  - `MOVEMENTFLAG_SWIMMING`
  - `MOVEMENTFLAG_FLYING`
  - `MOVEMENTFLAG_FALLING`
- Synchronize flags with movement state
- Network packet updates

**FR-3.3**: Spline Movement
- Use MoveSpline for smooth interpolation
- Proper velocity calculation
- Acceleration/deceleration curves
- Orientation updates (facing direction)
- Animation synchronization

### FR-4: Environmental Awareness

**FR-4.1**: Liquid Detection
- Detect liquid type (water, lava, slime)
- Get liquid level (Z height)
- Check if unit can survive in liquid
- Detect breath/fatigue mechanics
- Trigger swimming state on entry

**FR-4.2**: Collision Detection
- Ray-casting for line-of-sight
- AABB collision checks
- Static geometry collision (VMAP)
- Dynamic object avoidance
- Collision response (stop, recalculate path)

**FR-4.3**: Terrain Analysis
- Slope detection (climbable vs. too steep)
- Edge detection (cliffs, drops)
- Surface type (ground, water, air)
- Traversability check (can unit move here?)
- Safe vs. hazardous terrain

### FR-5: Stuck Detection & Recovery

**FR-5.1**: Stuck Detection
- Position tracking (no movement for N seconds)
- Path progress tracking (not advancing waypoints)
- Collision failure counter
- Repeated path calculation failures
- Manual stuck reporting API

**FR-5.2**: Recovery Strategies
- **Level 1**: Recalculate current path
- **Level 2**: Move backwards 5 yards, retry
- **Level 3**: Move to random nearby valid position
- **Level 4**: Teleport to last known-good position (5+ seconds ago)
- **Level 5**: Evade/reset to home position
- **Escalation**: Alert operator if all strategies fail

**FR-5.3**: Recovery Tracking
- Log all stuck events with context
- Track recovery strategy success rates
- Blacklist problem positions/areas
- Metrics: stuck rate, recovery rate, time to recover

### FR-6: Performance & Scalability

**FR-6.1**: Asynchronous Path Calculation
- Path calculation off main thread (optional)
- Callback on completion
- Timeout for long calculations
- Priority queue for path requests

**FR-6.2**: Resource Management
- Path cache memory limits
- Max concurrent path calculations
- Rate limiting per bot
- Object pooling (PathGenerator, etc.)

**FR-6.3**: Performance Monitoring
- Metrics: path calc time, stuck rate, cache hit rate
- Alerts on degraded performance
- Configurable performance budgets
- Graceful degradation under load

### FR-7: Configuration & Tuning

**FR-7.1**: Global Configuration
- Stuck detection thresholds (time, distance)
- Path calculation timeouts
- Cache sizes and policies
- Performance budgets
- Debug/logging levels

**FR-7.2**: Per-Bot Configuration
- Movement speed modifiers
- Stuck recovery enabled/disabled
- Path quality vs. performance tradeoff
- Specific movement capabilities (swim, fly)

### FR-7.3**: Runtime Tuning
- Hot-reload configuration without restart
- A/B testing different parameters
- Per-map configuration overrides

## Non-Functional Requirements

### NFR-1: Performance
- **Path Calculation**: p95 < 5ms, p99 < 20ms
- **Stuck Detection**: < 100μs per update
- **Validation**: < 1ms per destination check
- **Memory**: < 10MB per 100 bots (excluding NavMesh)
- **CPU**: < 5% CPU per 100 bots on modern server

### NFR-2: Scalability
- Support 1000+ concurrent bots per server
- Linear performance scaling up to target load
- No global locks in hot paths
- Efficient resource sharing (cache, NavMesh)

### NFR-3: Reliability
- 99.9% movement success rate (reaches destination)
- 95%+ automatic recovery from stuck
- No crashes or deadlocks
- Graceful handling of invalid input
- Circuit breakers for cascading failures

### NFR-4: Maintainability
- Clean architecture (SOLID principles)
- Comprehensive inline documentation
- Unit test coverage > 80%
- Integration test coverage for critical paths
- Logging and diagnostics built-in

### NFR-5: Compatibility
- Compatible with TrinityCore master branch
- No breaking changes to public APIs
- Backward compatible with existing AI scripts
- Supports existing MMap format

### NFR-6: Debuggability
- Detailed logging (configurable verbosity)
- Visual debugging tools
- Metric collection and reporting
- Reproducible test cases
- Clear error messages

## Technical Constraints

### Must Use
- **C++17/20**: TrinityCore's current standard
- **Recast/Detour**: Existing NavMesh library
- **TrinityCore APIs**: Map, Unit, MotionMaster, etc.
- **Existing MMap files**: No regeneration required

### Cannot Modify
- Core TrinityCore classes (Map, World, Object)
- Database schemas
- Network protocol
- Client-side assets

### Integration Points
- `MotionMaster`: Primary interface for movement
- `PathGenerator`: Pathfinding implementation
- `MovementGenerator`: Movement behavior implementations
- `UnitAI` / `PlayerAI`: AI control integration
- MMap system: Navigation mesh queries

## Open Questions & Clarifications Needed

### Critical Clarifications

**Q1: Target Bot System**
- **Question**: Is this for TrinityCore's existing PlayerAI (charmed players) or a NEW custom bot system?
- **Impact**: Determines scope - enhance PlayerAI vs. build new system
- **Decision Needed**: Before detailed design

**Q2: Bot Control Mechanism**
- **Question**: How are bots controlled? (Script AI, external commands, learning system?)
- **Impact**: Affects API design and integration points
- **Options**: 
  - Extend PlayerAI class
  - New BotAI class separate from PlayerAI
  - External bot management system
- **Decision Needed**: Before architecture design

**Q3: Performance Priority**
- **Question**: Trade-off preference: Path quality vs. calculation speed?
- **Impact**: Determines optimization strategy
- **Assumption**: Prioritize quality (human-like), aim for < 5ms p95
- **Validation Needed**: Confirm acceptable latency

**Q4: Flying Movement**
- **Question**: Do bots need flying capability (flying mounts)?
- **Impact**: Significant additional complexity
- **Assumption**: Out of scope initially (YAGNI)
- **Validation Needed**: Confirm scope

### Minor Clarifications

**Q5: Recovery Escalation**
- **Question**: What happens when all recovery strategies fail?
- **Options**: Log error, teleport home, despawn, alert operator
- **Assumption**: Teleport to home/safe position + alert
- **Decision Needed**: During detailed design

**Q6: Path Debugging**
- **Question**: Preference for visual debugging? In-game, external tool, or logs only?
- **Assumption**: Logs + in-game visualization (draw path)
- **Validation Needed**: Confirm tooling requirements

**Q7: Multi-Level Terrain**
- **Question**: How to handle bridges, caves, multi-level areas?
- **Impact**: Affects Z-coordinate validation logic
- **Assumption**: NavMesh handles it, validate with existing system
- **Investigation Needed**: Test edge cases

**Q8: Dynamic Obstacles**
- **Question**: Should bots avoid other bots/players dynamically?
- **Impact**: Adds collision avoidance complexity
- **Assumption**: Not initially required (bots can overlap)
- **Validation Needed**: Confirm requirements

## Assumptions

1. **NavMesh Quality**: Existing MMap files are reasonably accurate
2. **TrinityCore Stability**: Core server is stable and well-tested
3. **Performance Baseline**: Current TrinityCore can handle 1000+ units (NPCs)
4. **AI Control**: Bot decision-making (what to do) is separate from movement (how to move)
5. **Client Support**: WoW client properly renders movement packets
6. **Testing Environment**: Development server available for testing
7. **Map Coverage**: MMap files exist for target test maps

## Dependencies

### Internal Dependencies
- TrinityCore master branch (latest)
- Existing MMap files
- Recast/Detour library
- G3D math library (Vector3, etc.)

### External Dependencies
- C++17/20 compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- (Optional) Visual debugging tools

### Data Dependencies
- Navigation mesh files (*.mmap, *.mmtile)
- Map geometry (*.map, *.vmap)
- Liquid data (*.wdt)

## Success Criteria

### Minimum Viable Product (MVP)

**Phase 1 Deliverables:**
1. Complete system audit and root cause analysis
2. Technical architecture design
3. Implementation plan with estimates

**MVP Must Fix:**
- P0-1: Collision detection (no wall walking)
- P0-2: Ground validation (no void walking)
- P0-3: Swimming mechanics (proper swim state)
- P0-5: Basic stuck detection + recovery

**MVP Nice-to-Have:**
- P0-4: Air hopping reduction
- P1-6: Path optimization

### Phase 2: Full Production System

- All P0 issues resolved
- All MVP + nice-to-have features
- Performance targets met (1000+ bots)
- Comprehensive testing
- Documentation complete

### Acceptance Testing

**Test Scenario 1: Wall Collision**
- Bot commanded to move through wall
- **Expected**: Bot stops at wall, recalculates path around

**Test Scenario 2: Cliff Detection**
- Bot commanded to location beyond cliff edge
- **Expected**: Bot stops at edge, doesn't fall

**Test Scenario 3: Swimming**
- Bot walks into water (lake, ocean)
- **Expected**: Smooth transition to swimming, proper animation, surfaces if needed

**Test Scenario 4: Stuck Recovery**
- Bot wedged in geometry (corner, crevice)
- **Expected**: Stuck detected within 3 seconds, recovery initiated, successful within 10 seconds

**Test Scenario 5: Path Quality**
- Bot commanded to move 100 yards across city
- **Expected**: Natural path following roads/walkways, similar to human player

**Test Scenario 6: Scale Test**
- 1000 bots commanded to move simultaneously
- **Expected**: All bots move, server maintains 50+ FPS, < 20ms path calc p99

## Risks & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| NavMesh inaccuracies | High | High | Validation layer catches issues, fallback to direct path |
| Performance regression | Medium | High | Benchmarking, profiling, async path calc |
| Integration breaks existing AI | Medium | Medium | Backward compatibility layer, comprehensive testing |
| Stuck detection false positives | Medium | Medium | Tunable thresholds, multiple detection criteria |
| Scope creep | High | Medium | Clear MVP definition, phase-gated development |
| Recast/Detour limitations | Low | High | Early prototyping, identify workarounds |

## Timeline Estimate (High-Level)

### Phase 1: Analysis & Design (40 hours)
- ✅ Requirements (included in this PRD)
- System Audit (8h)
- Root Cause Analysis (8h)
- TrinityCore Reference Study (6h)
- Architecture Design (10h)
- Implementation Plan (8h)

### Phase 2: Implementation (120-160 hours)
- Core validation system (20h)
- Movement state machine (20h)
- Stuck detection & recovery (25h)
- Swimming/environment handling (20h)
- Path optimization (15h)
- Integration & refactoring (20h)
- Testing & debugging (40-60h)

### Phase 3: Testing & Deployment (40 hours)
- Unit tests (15h)
- Integration tests (10h)
- Performance testing (10h)
- Documentation (5h)

**Total Estimate**: 200-240 hours (5-6 weeks full-time, 10-12 weeks part-time)

## Appendices

### Appendix A: Movement System File Inventory

**Core Movement:**
- `src/server/game/Movement/MotionMaster.{h,cpp}` - Movement orchestration
- `src/server/game/Movement/PathGenerator.{h,cpp}` - Pathfinding with NavMesh
- `src/server/game/Movement/MovementGenerator.{h,cpp}` - Base movement generator
- `src/server/game/Movement/MovementDefines.{h,cpp}` - Enums, types, flags
- `src/server/game/Movement/AbstractFollower.{h,cpp}` - Shared follow/chase logic

**Movement Generators:**
- `ChaseMovementGenerator` - Combat pursuit
- `FollowMovementGenerator` - Following targets (pets, players)
- `PointMovementGenerator` - Move to specific coordinates
- `FleeingMovementGenerator` - Running away
- `RandomMovementGenerator` - Random wandering
- `WaypointMovementGenerator` - Patrol paths
- `IdleMovementGenerator` - Stationary
- `HomeMovementGenerator` - Return to spawn
- `FormationMovementGenerator` - Group formations
- `SplineChainMovementGenerator` - Complex scripted paths

**Spline System:**
- `src/server/game/Movement/Spline/MoveSpline.{h,cpp}` - Smooth movement interpolation
- `src/server/game/Movement/Spline/MoveSplineInit.{h,cpp}` - Spline initialization
- `src/server/game/Movement/Spline/Spline.{h,cpp}` - Mathematical spline curves

**AI Integration:**
- `src/server/game/AI/PlayerAI/PlayerAI.{h,cpp}` - Player AI control (1308 lines)
- `src/server/game/AI/CoreAI/PetAI.{h,cpp}` - Pet AI (reference for following)
- `src/server/game/AI/CoreAI/UnitAI.{h,cpp}` - Base unit AI

**Packets:**
- `src/server/game/Server/Packets/MovementPackets.{h,cpp}` - Network protocol

**Unit/Entity:**
- `src/server/game/Entities/Unit/Unit.{h,cpp}` - Unit class (movement state, flags)
- `src/server/game/Entities/Object/MovementInfo.h` - Movement data structures

### Appendix B: Movement Flags Reference

```cpp
// Key movement flags (from Trinity/client)
MOVEMENTFLAG_FORWARD            = 0x00000001
MOVEMENTFLAG_BACKWARD           = 0x00000002
MOVEMENTFLAG_STRAFE_LEFT        = 0x00000004
MOVEMENTFLAG_STRAFE_RIGHT       = 0x00000008
MOVEMENTFLAG_LEFT               = 0x00000010
MOVEMENTFLAG_RIGHT              = 0x00000020
MOVEMENTFLAG_PITCH_UP           = 0x00000040
MOVEMENTFLAG_PITCH_DOWN         = 0x00000080
MOVEMENTFLAG_WALKING            = 0x00000100
MOVEMENTFLAG_ONTRANSPORT        = 0x00000200
MOVEMENTFLAG_DISABLE_GRAVITY    = 0x00000400
MOVEMENTFLAG_ROOT               = 0x00000800
MOVEMENTFLAG_FALLING            = 0x00001000
MOVEMENTFLAG_FALLING_FAR        = 0x00002000
MOVEMENTFLAG_PENDING_STOP       = 0x00004000
MOVEMENTFLAG_PENDING_STRAFE_STOP= 0x00008000
MOVEMENTFLAG_PENDING_FORWARD    = 0x00010000
MOVEMENTFLAG_PENDING_BACKWARD   = 0x00020000
MOVEMENTFLAG_PENDING_STRAFE_LEFT= 0x00040000
MOVEMENTFLAG_PENDING_STRAFE_RIGHT=0x00080000
MOVEMENTFLAG_PENDING_ROOT       = 0x00100000
MOVEMENTFLAG_SWIMMING           = 0x00200000
MOVEMENTFLAG_ASCENDING          = 0x00400000
MOVEMENTFLAG_DESCENDING         = 0x00800000
MOVEMENTFLAG_CAN_FLY            = 0x01000000
MOVEMENTFLAG_FLYING             = 0x02000000
```

### Appendix C: PathType Flags Reference

```cpp
enum PathType
{
    PATHFIND_BLANK             = 0x00,   // path not built yet
    PATHFIND_NORMAL            = 0x01,   // normal path
    PATHFIND_SHORTCUT          = 0x02,   // travel through obstacles, terrain, air, etc
    PATHFIND_INCOMPLETE        = 0x04,   // partial path - getting closer to target
    PATHFIND_NOPATH            = 0x08,   // no valid path at all
    PATHFIND_NOT_USING_PATH    = 0x10,   // flying/swimming or map without mmaps
    PATHFIND_SHORT             = 0x20,   // path at limited length
    PATHFIND_FARFROMPOLY_START = 0x40,   // start position far from mmap polygon
    PATHFIND_FARFROMPOLY_END   = 0x80,   // end position far from mmap polygon
};
```

### Appendix D: Glossary

- **MMap**: Movement Map, navigation mesh file format used by TrinityCore
- **NavMesh**: Navigation Mesh, 3D polygon mesh representing walkable surfaces
- **Detour**: Navigation library for pathfinding on NavMesh
- **Recast**: NavMesh generation library (companion to Detour)
- **Spline**: Smooth curve used for movement interpolation
- **MotionMaster**: TrinityCore class managing unit movement
- **MovementGenerator**: Abstract class defining movement behaviors
- **PathGenerator**: Class calculating paths using NavMesh
- **VMAP**: Visual Map, collision geometry for line-of-sight
- **LOS**: Line of Sight, visibility/collision check
- **Polygon (Poly)**: Single walkable surface in NavMesh
- **PolyRef**: Reference ID to NavMesh polygon
- **Z-coordinate**: Height/elevation in 3D space
- **Bot**: AI-controlled player character
- **Unit**: Base class for all moving entities (players, NPCs, pets)
- **WorldObject**: Base class for all positioned entities

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-27 | AI Assistant | Initial PRD creation |

## Approval

This PRD requires approval/clarification on:
1. Bot system type (PlayerAI vs. custom)
2. Bot control mechanism
3. Flying movement scope
4. Performance vs. quality trade-offs

**Next Steps:**
1. Review and validate assumptions
2. Clarify open questions
3. Proceed to Technical Specification phase
