# Full SDD workflow

## Configuration
- **Artifacts Path**: {@artifacts_path} → `.zenflow/tasks/{task_id}`

---

## Workflow Steps

### [x] Step: Requirements
<!-- chat-id: 18aaa213-e6ea-438d-8e84-6f9f5982afb2 -->

Create a Product Requirements Document (PRD) based on the feature description.

1. Review existing codebase to understand current architecture and patterns
2. Analyze the feature definition and identify unclear aspects
3. Ask the user for clarifications on aspects that significantly impact scope or user experience
4. Make reasonable decisions for minor details based on context and conventions
5. If user can't clarify, make a decision, state the assumption, and continue

Save the PRD to `{@artifacts_path}/requirements.md`.

### [x] Step: Technical Specification
<!-- chat-id: 14d8a422-e14a-4d5d-8944-1a9f83dc4bf9 -->

Create a technical specification based on the PRD in `{@artifacts_path}/requirements.md`.

1. Review existing codebase architecture and identify reusable components
2. Define the implementation approach

Save to `{@artifacts_path}/spec.md` with:
- Technical context (language, dependencies)
- Implementation approach referencing existing code patterns
- Source code structure changes
- Data model / API / interface changes
- Delivery phases (incremental, testable milestones)
- Verification approach using project lint/test commands

### [x] Step: Planning
<!-- chat-id: b55eafe3-e8ad-4c05-838f-4d9065d92cf1 -->

Create a detailed implementation plan based on `{@artifacts_path}/spec.md`.

1. Break down the work into concrete tasks
2. Each task should reference relevant contracts and include verification steps
3. Replace the Implementation step below with the planned tasks

Rule of thumb for step size: each step should represent a coherent unit of work (e.g., implement a component, add an API endpoint, write tests for a module). Avoid steps that are too granular (single function) or too broad (entire feature).

If the feature is trivial and doesn't warrant full specification, update this workflow to remove unnecessary steps and explain the reasoning to the user.

Save to `{@artifacts_path}/plan.md`.

---

## Implementation Steps

### [x] Step: Phase 1.1 - Core Infrastructure Setup
<!-- chat-id: ee9929b5-d716-4410-be45-b50692220fe8 -->

**Goal:** Create base directory structure and define core interfaces

**Tasks:**
- [ ] Create directory structure under `src/server/game/Movement/BotMovement/` with subdirectories: Core/, Validation/, Pathfinding/, StateMachine/, Detection/, Generators/
- [ ] Create `BotMovementDefines.h` with core enums (MovementStateType, ValidationFailureReason, StuckType, RecoveryLevel, ValidationLevel)
- [ ] Create `ValidationResult.h` structure definition
- [ ] Add CMake configuration to include new BotMovement directory in build system
- [ ] Verify compilation of empty structure

**Verification:**
```bash
cmake --build . --target worldserver
# Should compile successfully with new files
```

**References:** spec.md Section 3 (Source Code Structure), Section 4.3 (Validation Pipeline)

---

### [x] Step: Phase 1.2 - Configuration System
<!-- chat-id: e539c8bf-8f77-4bc8-a4e4-233431326908 -->

**Goal:** Implement configuration loading and management

**Tasks:**
- [ ] Create `BotMovementConfig.h/cpp` with configuration class
- [ ] Add configuration section to `worldserver.conf.dist` (BotMovement.* keys)
- [ ] Implement `BotMovementConfig::Load()` to read config values using TrinityCore's ConfigMgr
- [ ] Implement `BotMovementConfig::Reload()` for runtime config changes
- [ ] Add default values for all config options
- [ ] Write unit tests for config loading

**Verification:**
```bash
# Run unit tests
./worldserver --run-tests "BotMovement.Config.*"
# Verify config section appears in worldserver.conf
grep "BotMovement" worldserver.conf.dist
```

**References:** spec.md Section 8 (Configuration & Tuning)

---

### [x] Step: Phase 1.3 - BotMovementManager Singleton
<!-- chat-id: 57b43634-fcce-4a88-8d39-3990942e8671 -->

**Goal:** Implement global movement system manager

**Tasks:**
- [x] Create `BotMovementManager.h/cpp` with singleton pattern
- [x] Implement controller registry (map of ObjectGuid -> BotMovementController*)
- [x] Implement `GetControllerForUnit()`, `RegisterController()`, `UnregisterController()`
- [x] Add global PathCache instance
- [x] Add global MovementMetrics tracking
- [x] Implement `ReloadConfig()` method
- [ ] Write unit tests for manager lifecycle

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Manager.*"
# Verify singleton creation and controller registration
```

**References:** spec.md Section 4.1 (BotMovementManager)

---

### [x] Step: Phase 1.4 - PositionValidator Implementation
<!-- chat-id: 57b43634-fcce-4a88-8d39-3990942e8671 -->

**Goal:** Implement position bounds and validity checking

**Tasks:**
- [x] Create `PositionValidator.h/cpp`
- [x] Implement `ValidateBounds()` - check coordinates within map bounds
- [x] Implement `ValidateMapId()` - verify map ID is valid
- [x] Implement `ValidatePosition()` - master validation method
- [x] Add error messages for each failure type
- [ ] Write comprehensive unit tests (valid positions, out-of-bounds, invalid map IDs)

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Validators.Position.*"
# Tests should verify rejection of invalid positions
```

**References:** spec.md Section 4.3 (Validation Pipeline), requirements.md FR-1.1

---

### [x] Step: Phase 1.5 - GroundValidator Implementation
<!-- chat-id: 57b43634-fcce-4a88-8d39-3990942e8671 -->

**Goal:** Implement ground height validation to prevent void falls

**Tasks:**
- [x] Create `GroundValidator.h/cpp`
- [x] Implement `GetGroundHeight()` using Map::GetHeight()
- [x] Implement `ValidateGroundHeight()` - check if Z-coordinate is valid
- [x] Handle cases: no ground (void), water, bridges/multi-level
- [x] Implement ground height caching for performance
- [x] Add detection for "unsafe terrain" (lava, fatigue zones)
- [ ] Write unit tests with various terrain scenarios

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Validators.Ground.*"
# Test edge detection, void detection, bridge handling
```

**References:** spec.md Section 4.3, requirements.md P0-2 (void falls)

---

### [x] Step: Phase 1.6 - BotMovementController Base Implementation
<!-- chat-id: 538877ff-c7fc-40b1-b43b-67df9b4c959a -->

**Goal:** Create per-bot movement controller (basic version without full state machine)

**Tasks:**
- [x] Create `BotMovementController.h/cpp`
- [x] Implement constructor/destructor with Unit* owner
- [x] Add basic position history tracking (std::deque<PositionSnapshot>)
- [x] Implement `Update(uint32 diff)` stub (to be filled in later phases)
- [x] Add helper methods: `GetOwner()`, `GetLastPosition()`, `RecordPosition()`
- [x] Implement controller registration with BotMovementManager on creation
- [ ] Write tests for controller lifecycle and position tracking

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Controller.*"
# Verify controller creation, registration, position tracking
```

**References:** spec.md Section 4.2 (BotMovementController)

---

### [ ] Step: Phase 1.7 - Phase 1 Integration Testing
<!-- chat-id: 2befcd37-ef67-4c0b-b94a-30155532914b -->

**Goal:** Validate Phase 1 deliverables work together

**Tasks:**
- [ ] Write integration test: create manager, register controller
- [ ] Write integration test: validate valid position (should pass)
- [ ] Write integration test: validate invalid position (should fail with correct reason)
- [ ] Write integration test: validate ground height at known coordinates
- [ ] Run all Phase 1 tests
- [ ] Fix any integration issues

**Verification:**
```bash
./worldserver --run-tests "BotMovement.*"
# All Phase 1 tests should pass
# Run with AddressSanitizer to detect memory issues
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
make && ./worldserver --run-tests "BotMovement.*"
```

**References:** spec.md Section 10 (Phase 1 deliverables)

---

### [ ] Step: Phase 2.1 - CollisionValidator Implementation

**Goal:** Implement line-of-sight and collision detection

**Tasks:**
- [ ] Create `CollisionValidator.h/cpp`
- [ ] Implement `ValidateLineOfSight()` using VMAP collision checks
- [ ] Implement `CheckCollisionAlongPath()` for multi-point path validation
- [ ] Add collision tolerance thresholds
- [ ] Handle edge cases: underwater LOS, multi-level terrain
- [ ] Write tests with known collision scenarios

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Validators.Collision.*"
# Test wall detection, LOS blocking, path segment collision
```

**References:** spec.md Section 4.3, requirements.md P0-1 (wall walking)

---

### [ ] Step: Phase 2.2 - LiquidValidator Implementation

**Goal:** Implement water/liquid detection for swimming transitions

**Tasks:**
- [ ] Create `LiquidValidator.h/cpp`
- [ ] Implement `IsInLiquid()` using Map liquid queries
- [ ] Implement `GetLiquidHeight()` at position
- [ ] Implement `CheckLiquidTransition()` - detect ground→water or water→ground transitions
- [ ] Add liquid type detection (water, lava, slime)
- [ ] Add breath requirement checks
- [ ] Write tests for water detection and transitions

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Validators.Liquid.*"
# Test water detection, transition detection, liquid types
```

**References:** spec.md Section 4.3, requirements.md P0-3 (swimming)

---

### [ ] Step: Phase 2.3 - ValidatedPathGenerator Core

**Goal:** Wrap PathGenerator with validation decorator

**Tasks:**
- [ ] Create `ValidatedPathGenerator.h/cpp`
- [ ] Wrap existing PathGenerator instance
- [ ] Implement `CalculateValidatedPath()` method
- [ ] Add validation pipeline: destination validation → path generation → path segment validation
- [ ] Implement `ValidateDestination()` using PositionValidator and GroundValidator
- [ ] Implement `ValidatePathSegments()` using CollisionValidator
- [ ] Implement `ValidateEnvironmentTransitions()` using LiquidValidator
- [ ] Add validation level support (None, Basic, Standard, Strict)

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Pathfinding.Validated.*"
# Test path validation with various scenarios
```

**References:** spec.md Section 4.4 (ValidatedPathGenerator)

---

### [ ] Step: Phase 2.4 - PathValidationPipeline

**Goal:** Multi-stage path validation system

**Tasks:**
- [ ] Create `PathValidationPipeline.h/cpp`
- [ ] Implement pipeline pattern for chaining validators
- [ ] Add `AddValidator()` to register validators in pipeline
- [ ] Implement `ValidatePath()` - run all validators sequentially
- [ ] Add short-circuit on first failure
- [ ] Add option to collect all errors (strict mode)
- [ ] Write tests for pipeline execution

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Pathfinding.Pipeline.*"
# Verify validators execute in order, short-circuit works
```

**References:** spec.md Section 4.3 (Validation Pipeline)

---

### [ ] Step: Phase 2.5 - PathCache Implementation

**Goal:** LRU caching for frequently-used paths

**Tasks:**
- [ ] Create `PathCache.h/cpp`
- [ ] Implement LRU cache with configurable size
- [ ] Implement path hashing (start + end position hash)
- [ ] Implement `TryGetCachedPath()` and `CachePath()`
- [ ] Add TTL (time-to-live) for cache entries
- [ ] Implement cache invalidation on map changes
- [ ] Add cache hit/miss metrics
- [ ] Write tests for cache behavior

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Pathfinding.Cache.*"
# Test cache hits, misses, eviction, TTL expiration
```

**References:** spec.md Section 10 (Phase 2), requirements.md FR-2.3

---

### [ ] Step: Phase 2.6 - PathSmoother Implementation

**Goal:** Path optimization to reduce waypoint count

**Tasks:**
- [ ] Create `PathSmoother.h/cpp`
- [ ] Implement corner cutting algorithm (remove unnecessary intermediate waypoints)
- [ ] Implement spline smoothing for natural curves
- [ ] Add LOS checks to ensure smoothed path is valid
- [ ] Add configurable aggressiveness level
- [ ] Write tests comparing original vs. smoothed paths

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Pathfinding.Smoother.*"
# Verify smoothed paths have fewer waypoints but remain valid
```

**References:** spec.md Section 2.3, requirements.md P1-6 (path quality)

---

### [ ] Step: Phase 2.7 - Phase 2 Integration Testing

**Goal:** Validate pathfinding system works end-to-end

**Tasks:**
- [ ] Integration test: calculate validated path through simple terrain (should succeed)
- [ ] Integration test: calculate path through wall (should fail with collision error)
- [ ] Integration test: calculate path to void (should fail with ground validation error)
- [ ] Integration test: calculate path crossing water (should detect transition)
- [ ] Integration test: verify path caching (second request should hit cache)
- [ ] Integration test: verify path smoothing reduces waypoints
- [ ] Run all Phase 2 tests
- [ ] Fix integration issues

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Pathfinding.*"
# All pathfinding tests pass
# Cache hit rate > 30% in repeated path scenario
```

**References:** spec.md Section 10 (Phase 2 verification)

---

### [ ] Step: Phase 3.1 - MovementState Base Class

**Goal:** Define state interface and base implementation

**Tasks:**
- [ ] Create `MovementState.h/cpp` with abstract base class
- [ ] Define interface: `OnEnter()`, `OnExit()`, `Update()`, `GetType()`, `GetRequiredMovementFlags()`
- [ ] Add helper: `TransitionTo()` to trigger state changes
- [ ] Create MovementStateType enum
- [ ] Write tests for base state behavior

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StateMachine.Base.*"
# Verify state interface works correctly
```

**References:** spec.md Section 5.2 (State Implementations)

---

### [ ] Step: Phase 3.2 - MovementStateMachine Core

**Goal:** Implement state machine orchestration

**Tasks:**
- [ ] Create `MovementStateMachine.h/cpp`
- [ ] Implement state storage and current state tracking
- [ ] Implement `TransitionTo()` with state change validation
- [ ] Call `OnExit()` on old state, `OnEnter()` on new state
- [ ] Implement `Update()` to delegate to current state
- [ ] Add environment query helpers: `IsOnGround()`, `IsInWater()`, `IsFlying()`, `IsFalling()`
- [ ] Implement state instance management (reuse states, don't recreate)
- [ ] Write tests for state transitions

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StateMachine.Core.*"
# Test state transitions, update delegation, environment queries
```

**References:** spec.md Section 5.3 (MovementStateMachine)

---

### [ ] Step: Phase 3.3 - IdleState Implementation

**Goal:** Implement idle/stopped movement state

**Tasks:**
- [ ] Create `IdleState.h/cpp`
- [ ] Implement `OnEnter()` - stop movement, clear movement flags
- [ ] Implement `OnExit()` - prepare for movement
- [ ] Implement `Update()` - remain idle, wait for movement command
- [ ] Set `GetRequiredMovementFlags()` to no flags
- [ ] Write tests for idle state behavior

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StateMachine.Idle.*"
```

**References:** spec.md Section 5.1 (State Diagram)

---

### [ ] Step: Phase 3.4 - GroundMovementState Implementation

**Goal:** Implement ground movement state

**Tasks:**
- [ ] Create `GroundMovementState.h/cpp`
- [ ] Implement `OnEnter()` - set ground movement flags
- [ ] Implement `Update()` - check for edge/water, continue path following
- [ ] Implement `CheckForEdge()` - detect cliff edges, transition to Falling if needed
- [ ] Implement `CheckForWater()` - detect water entry, transition to Swimming if needed
- [ ] Set `GetRequiredMovementFlags()` to MOVEMENTFLAG_FORWARD
- [ ] Write tests for ground movement and transitions

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StateMachine.Ground.*"
# Test edge detection, water detection, state transitions
```

**References:** spec.md Section 5.2, requirements.md P0-2 (void falls)

---

### [ ] Step: Phase 3.5 - SwimmingMovementState Implementation

**Goal:** Implement swimming state for water movement

**Tasks:**
- [ ] Create `SwimmingMovementState.h/cpp`
- [ ] Implement `OnEnter()` - set MOVEMENTFLAG_SWIMMING, trigger swim animation
- [ ] Implement `OnExit()` - clear swimming flag, restore ground animation
- [ ] Implement `Update()` - check if still in water, handle breath mechanic
- [ ] Implement `SurfaceForAir()` - move bot upward when breath needed
- [ ] Add underwater time tracking
- [ ] Set `GetRequiredMovementFlags()` to MOVEMENTFLAG_SWIMMING
- [ ] Write tests for swimming state and breath mechanics

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StateMachine.Swimming.*"
# Test water entry/exit, swimming flag, breath mechanics
```

**References:** spec.md Section 5.2, requirements.md P0-3 (swimming)

---

### [ ] Step: Phase 3.6 - FallingMovementState Implementation

**Goal:** Implement falling state for gravity and landing

**Tasks:**
- [ ] Create `FallingMovementState.h/cpp`
- [ ] Implement `OnEnter()` - detect fall initiation, record start height
- [ ] Implement `Update()` - apply gravity, check for ground/water collision
- [ ] Implement landing detection - transition to Ground when landing
- [ ] Calculate and apply fall damage if applicable
- [ ] Handle landing in water (transition to Swimming instead of Ground)
- [ ] Set `GetRequiredMovementFlags()` to MOVEMENTFLAG_FALLING
- [ ] Write tests for falling and landing scenarios

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StateMachine.Falling.*"
# Test edge falling, landing detection, fall damage, water landing
```

**References:** spec.md Section 5.1, requirements.md P0-4 (hopping)

---

### [ ] Step: Phase 3.7 - State Machine Integration with Controller

**Goal:** Integrate state machine into BotMovementController

**Tasks:**
- [ ] Update `BotMovementController` to own MovementStateMachine instance
- [ ] Implement `UpdateStateMachine()` called from controller's `Update()`
- [ ] Implement `SyncMovementFlags()` to synchronize flags with state
- [ ] Add state query methods: `GetCurrentState()`, `GetCurrentStateType()`
- [ ] Trigger state transitions based on environment changes
- [ ] Write integration tests

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Controller.*"
# Test controller with state machine updates
```

**References:** spec.md Section 4.2 (BotMovementController)

---

### [ ] Step: Phase 3.8 - Phase 3 Integration Testing

**Goal:** Validate state machine works in realistic scenarios

**Tasks:**
- [ ] Integration test: bot transitions Idle → Ground on movement command
- [ ] Integration test: bot transitions Ground → Swimming when entering water
- [ ] Integration test: bot transitions Swimming → Ground when exiting water
- [ ] Integration test: bot transitions Ground → Falling when walking off edge
- [ ] Integration test: bot transitions Falling → Ground on landing
- [ ] Integration test: movement flags synchronized correctly with states
- [ ] Run all Phase 3 tests
- [ ] Fix integration issues

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StateMachine.*"
# All state machine tests pass
# Manual test: bot swims correctly in water, walks correctly on ground
```

**References:** spec.md Section 10 (Phase 3 verification)

---

### [ ] Step: Phase 4.1 - StuckDetector Core Implementation

**Goal:** Implement stuck detection logic

**Tasks:**
- [ ] Create `StuckDetector.h/cpp`
- [ ] Implement position-based stuck detection (no movement for N seconds)
- [ ] Implement progress-based stuck detection (not advancing along waypoints)
- [ ] Implement collision-based stuck detection (repeated collision failures)
- [ ] Implement path-based stuck detection (path generation repeatedly fails)
- [ ] Add `RecordPosition()`, `RecordPathFailure()`, `RecordCollision()` tracking methods
- [ ] Implement `IsStuck()` and `GetStuckType()` query methods
- [ ] Add configurable thresholds from BotMovementConfig
- [ ] Write tests for each stuck detection type

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StuckDetection.Detector.*"
# Test position stuck, collision stuck, path failure stuck
```

**References:** spec.md Section 6.1 (StuckDetector)

---

### [ ] Step: Phase 4.2 - RecoveryStrategies Implementation

**Goal:** Implement escalating recovery strategies

**Tasks:**
- [ ] Create `RecoveryStrategies.h/cpp`
- [ ] Implement `Level1_RecalculatePath()` - retry path calculation
- [ ] Implement `Level2_BackupAndRetry()` - move bot backward 5 yards, retry
- [ ] Implement `Level3_RandomNearbyPosition()` - move to random valid nearby position
- [ ] Implement `Level4_TeleportToSafePosition()` - teleport to last known-good position
- [ ] Implement `Level5_EvadeAndReset()` - evade combat, reset to home position
- [ ] Implement `TryRecoverFromStuck()` with escalation logic
- [ ] Return RecoveryResult with success/failure and level used
- [ ] Write tests for each recovery level

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StuckDetection.Recovery.*"
# Test each recovery level, escalation logic
```

**References:** spec.md Section 6.2 (Recovery Strategies)

---

### [ ] Step: Phase 4.3 - StuckState Implementation

**Goal:** Create stuck state for state machine

**Tasks:**
- [ ] Create `StuckState.h/cpp`
- [ ] Implement `OnEnter()` - trigger stuck detection, start recovery
- [ ] Implement `Update()` - attempt recovery, track recovery attempts
- [ ] Implement recovery escalation (attempt higher levels on failure)
- [ ] Transition back to previous state on successful recovery
- [ ] Transition to Idle on complete recovery failure (evade)
- [ ] Add logging for stuck incidents and recovery attempts
- [ ] Write tests for stuck state behavior

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StateMachine.Stuck.*"
# Test stuck state entry, recovery attempts, escalation
```

**References:** spec.md Section 5.1 (State Diagram)

---

### [ ] Step: Phase 4.4 - Stuck Detection Integration with Controller

**Goal:** Integrate stuck detection into movement controller

**Tasks:**
- [ ] Update `BotMovementController` to own StuckDetector instance
- [ ] Implement `UpdateStuckDetection()` called from controller's `Update()`
- [ ] Call `RecordPosition()` every update tick
- [ ] Trigger stuck state transition when stuck detected
- [ ] Implement `TriggerStuckRecovery()` public method
- [ ] Implement `ClearStuckState()` public method
- [ ] Add stuck tracking to position history
- [ ] Write integration tests

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Controller.*"
# Test controller with stuck detection integration
```

**References:** spec.md Section 4.2, Section 6.1

---

### [ ] Step: Phase 4.5 - Phase 4 Integration Testing

**Goal:** Validate stuck detection and recovery work end-to-end

**Tasks:**
- [ ] Integration test: bot detects position stuck within 3 seconds
- [ ] Integration test: bot recovers from stuck using Level 1 (recalculate)
- [ ] Integration test: bot escalates to Level 2 if Level 1 fails
- [ ] Integration test: bot eventually evades if all recovery attempts fail
- [ ] Integration test: stuck state transitions work correctly
- [ ] Manual test: place bot in corner, verify stuck detection and recovery
- [ ] Run all Phase 4 tests
- [ ] Fix integration issues

**Verification:**
```bash
./worldserver --run-tests "BotMovement.StuckDetection.*"
# All stuck detection tests pass
# Stuck recovery success rate > 95% in tests
```

**References:** spec.md Section 10 (Phase 4 verification)

---

### [ ] Step: Phase 5.1 - BotMovementGeneratorBase Implementation

**Goal:** Create base class for bot movement generators

**Tasks:**
- [ ] Create `BotMovementGeneratorBase.h/cpp` inheriting from MovementGeneratorMedium
- [ ] Add validation hooks in movement execution
- [ ] Add reference to BotMovementController
- [ ] Implement common bot movement behavior
- [ ] Add movement flag synchronization
- [ ] Override `Initialize()`, `Finalize()`, `Reset()`
- [ ] Write tests for base generator

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Generators.Base.*"
```

**References:** spec.md Section 10 (Phase 5)

---

### [ ] Step: Phase 5.2 - BotPointMovementGenerator Implementation

**Goal:** Implement validated point-to-point movement

**Tasks:**
- [ ] Create `BotPointMovementGenerator.h/cpp` inheriting from BotMovementGeneratorBase
- [ ] Override `Initialize()` to validate destination using ValidatedPathGenerator
- [ ] Implement validated path calculation before movement
- [ ] Reject movement if validation fails
- [ ] Use MotionMaster to execute movement along validated path
- [ ] Implement arrival detection and finalization
- [ ] Write tests for point movement with various destinations

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Generators.Point.*"
# Test movement to valid destination, rejection of invalid destination
```

**References:** spec.md Section 10 (Phase 5)

---

### [ ] Step: Phase 5.3 - BotChaseMovementGenerator Implementation

**Goal:** Implement validated chase movement

**Tasks:**
- [ ] Create `BotChaseMovementGenerator.h/cpp` inheriting from BotMovementGeneratorBase
- [ ] Implement continuous path recalculation as target moves
- [ ] Add validation for each path recalculation
- [ ] Implement proper distance maintenance
- [ ] Handle target loss (unreachable, out of range)
- [ ] Add combat movement support
- [ ] Write tests for chase scenarios

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Generators.Chase.*"
# Test chasing moving target, target loss handling
```

**References:** spec.md Section 10 (Phase 5)

---

### [ ] Step: Phase 5.4 - BotFollowMovementGenerator Implementation

**Goal:** Implement validated follow movement

**Tasks:**
- [ ] Create `BotFollowMovementGenerator.h/cpp` inheriting from BotMovementGeneratorBase
- [ ] Implement follow with distance and angle maintenance
- [ ] Add formation support (optional parameter)
- [ ] Implement smooth following (don't recalculate on every target micro-movement)
- [ ] Add "catch up" behavior if too far behind
- [ ] Handle target loss gracefully
- [ ] Write tests for follow scenarios

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Generators.Follow.*"
# Test following target at distance, formation behavior
```

**References:** spec.md Section 10 (Phase 5)

---

### [ ] Step: Phase 5.5 - MotionMaster Integration Hooks

**Goal:** Inject validation into MotionMaster methods

**Tasks:**
- [ ] Modify `MotionMaster::MovePoint()` to check for BotMovementController and use validated path
- [ ] Modify `MotionMaster::MoveChase()` to use BotChaseMovementGenerator for bots
- [ ] Modify `MotionMaster::MoveFollow()` to use BotFollowMovementGenerator for bots
- [ ] Add feature flag check (only apply to units with bot controller)
- [ ] Preserve legacy behavior for non-bot units
- [ ] Add fallback handling if validation fails
- [ ] Write tests for MotionMaster integration

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Integration.MotionMaster.*"
# Test MotionMaster calls use validated paths for bots
```

**References:** spec.md Section 7.1 (MotionMaster Integration)

---

### [ ] Step: Phase 5.6 - Phase 5 Integration Testing

**Goal:** Validate movement generators work with full system

**Tasks:**
- [ ] Integration test: bot moves to point using BotPointMovementGenerator (respects walls, ground)
- [ ] Integration test: bot chases target using BotChaseMovementGenerator
- [ ] Integration test: bot follows target using BotFollowMovementGenerator
- [ ] Integration test: MotionMaster integration works correctly for bots
- [ ] Integration test: non-bot units still use legacy movement (backward compatibility)
- [ ] Run all Phase 5 tests
- [ ] Fix integration issues

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Generators.*"
# All movement generator tests pass
# Manual test: bot follows player without wall clipping
```

**References:** spec.md Section 10 (Phase 5 verification)

---

### [ ] Step: Phase 6.1 - Unit::Update Integration

**Goal:** Integrate controller updates into game loop

**Tasks:**
- [ ] Modify `Unit::Update()` to call BotMovementController::Update() for units with controllers
- [ ] Add null check for controller
- [ ] Ensure update is called at appropriate frequency (every tick)
- [ ] Add performance instrumentation (measure update time)
- [ ] Write tests for update integration

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Integration.Unit.*"
# Test controller updates are called correctly
```

**References:** spec.md Section 7.2 (Unit::Update Integration)

---

### [ ] Step: Phase 6.2 - BotPlayerAI Implementation

**Goal:** Create bot-specific PlayerAI extension

**Tasks:**
- [ ] Create `BotPlayerAI.h/cpp` inheriting from PlayerAI
- [ ] Add BotMovementController ownership
- [ ] Implement enhanced movement commands: `MoveToPosition()`, `FollowTarget()`, `ChaseTarget()`
- [ ] Override `UpdateAI()` to integrate movement updates
- [ ] Add status query methods: `IsMoving()`, `IsStuck()`
- [ ] Implement controller lifecycle (creation, destruction)
- [ ] Write tests for BotPlayerAI

**Verification:**
```bash
./worldserver --run-tests "BotMovement.AI.*"
# Test BotPlayerAI creation, movement commands, status queries
```

**References:** spec.md Section 7.3 (PlayerAI Extension)

---

### [ ] Step: Phase 6.3 - Public API Implementation

**Goal:** Create public API for bot movement control

**Tasks:**
- [ ] Implement `GetBotController(Unit* unit)` global function
- [ ] Implement `EnableBotMovement(Unit* unit)` to create controller
- [ ] Implement `DisableBotMovement(Unit* unit)` to remove controller
- [ ] Implement query APIs: `IsBotStuck()`, `GetBotMovementState()`, `GetBotLastPathType()`
- [ ] Implement recovery APIs: `TriggerBotStuckRecovery()`, `ClearBotStuckState()`
- [ ] Implement metrics APIs: `GetBotMovementMetrics()`, `GetGlobalBotMovementMetrics()`
- [ ] Add API documentation comments
- [ ] Write tests for public API

**Verification:**
```bash
./worldserver --run-tests "BotMovement.API.*"
# Test all public API functions
```

**References:** spec.md Section 9.3 (Public API Additions)

---

### [ ] Step: Phase 6.4 - Configuration File Integration

**Goal:** Integrate configuration into worldserver

**Tasks:**
- [ ] Add BotMovement configuration section to `worldserver.conf.dist`
- [ ] Document all configuration options with comments
- [ ] Load configuration during server startup (WorldConfig initialization)
- [ ] Implement runtime reload support (`.reload config` command)
- [ ] Test configuration loading with various values
- [ ] Verify default values work correctly

**Verification:**
```bash
# Start server, verify config section loaded
grep "BotMovement" worldserver.log
# Test config reload
.reload config
```

**References:** spec.md Section 8.1 (Configuration File)

---

### [ ] Step: Phase 6.5 - Comprehensive Unit Tests

**Goal:** Ensure all components have complete unit test coverage

**Tasks:**
- [ ] Review test coverage for all components
- [ ] Add missing unit tests for edge cases
- [ ] Ensure 80%+ line coverage for new code
- [ ] Add tests for error handling paths
- [ ] Add tests for boundary conditions
- [ ] Run coverage analysis

**Verification:**
```bash
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
# Verify 80%+ coverage in coverage report
```

**References:** spec.md Section 11.1 (Unit Tests)

---

### [ ] Step: Phase 6.6 - Integration Test Suite

**Goal:** Create comprehensive integration test suite

**Tasks:**
- [ ] Implement INT-001: Bot walks into wall (stops correctly)
- [ ] Implement INT-002: Bot walks to cliff edge (doesn't fall)
- [ ] Implement INT-003: Bot walks into water (swims correctly)
- [ ] Implement INT-004: Bot stuck in corner (recovers within 10s)
- [ ] Implement INT-005: Bot follows player through complex path
- [ ] Implement INT-006: Bot target unreachable (handles gracefully)
- [ ] Implement INT-007: 100 bots move simultaneously
- [ ] Implement INT-008: Bot on bridge (correct Z-coordinate)
- [ ] Run all integration tests on test maps

**Verification:**
```bash
./worldserver --run-tests "BotMovement.Integration.*"
# All integration tests pass
```

**References:** spec.md Section 11.2 (Integration Tests)

---

### [ ] Step: Phase 6.7 - Performance Benchmarks

**Goal:** Validate performance meets targets

**Tasks:**
- [ ] Implement path calculation benchmark (10,000 paths)
- [ ] Implement stuck detection overhead microbenchmark
- [ ] Implement validation overhead microbenchmark
- [ ] Implement 100-bot CPU usage test (10-minute sustained load)
- [ ] Implement 100-bot memory usage test (heap profiling)
- [ ] Implement cache hit rate measurement
- [ ] Run all benchmarks and collect metrics
- [ ] Compare against targets (p95 < 5ms, etc.)
- [ ] Optimize if targets not met

**Verification:**
```bash
./worldserver --run-benchmarks "BotMovement.*"
# Verify:
# - Path calculation p95 < 5ms
# - CPU usage per 100 bots < 5%
# - Memory per 100 bots < 10MB
# - Cache hit rate > 30%
```

**References:** spec.md Section 11.3 (Performance Tests)

---

### [ ] Step: Phase 6.8 - 1000-Bot Stress Test

**Goal:** Validate system works at scale

**Tasks:**
- [ ] Create test scenario: 1000 concurrent bots
- [ ] Implement random movement, follow, and chase behaviors
- [ ] Run test for 30 minutes
- [ ] Monitor server FPS, CPU usage, memory usage
- [ ] Track stuck incidents
- [ ] Track validation failures
- [ ] Monitor for crashes or deadlocks
- [ ] Analyze results against success criteria

**Verification:**
```bash
# Run 1000-bot stress test
./stress_test_1000_bots.sh
# Success criteria:
# - All bots functional
# - Server FPS > 50
# - Stuck incidents < 0.1%
# - Validation failures < 1%
# - No crashes or deadlocks
```

**References:** spec.md Section 11.3 (Stress Test)

---

### [ ] Step: Phase 6.9 - Static Analysis and Sanitizers

**Goal:** Validate code quality with static analysis tools

**Tasks:**
- [ ] Run clang-tidy on all new code
- [ ] Fix all critical warnings
- [ ] Run cppcheck on all new code
- [ ] Fix reported issues
- [ ] Build and test with AddressSanitizer
- [ ] Fix any memory errors detected
- [ ] Build and test with ThreadSanitizer
- [ ] Fix any race conditions detected

**Verification:**
```bash
# clang-tidy
cmake -DCMAKE_CXX_CLANG_TIDY="clang-tidy" ..
make 2>&1 | tee clang-tidy.log
# AddressSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
make && ./worldserver --run-tests "BotMovement.*"
# ThreadSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON ..
make && ./worldserver --run-tests "BotMovement.*"
```

**References:** spec.md Section 11.4 (Lint & Typecheck)

---

### [ ] Step: Phase 6.10 - Phase 6 Final Validation

**Goal:** Ensure all Phase 6 success criteria met

**Tasks:**
- [ ] Verify all unit tests pass
- [ ] Verify all integration tests pass
- [ ] Verify all performance benchmarks meet targets
- [ ] Verify 1000-bot stress test passes
- [ ] Verify no static analysis or sanitizer errors
- [ ] Document any known issues or limitations
- [ ] Create issue list for follow-up work

**Verification:**
```bash
# Run full test suite
./worldserver --run-tests "BotMovement.*"
./worldserver --run-benchmarks "BotMovement.*"
# All tests pass, all benchmarks meet targets
```

**References:** spec.md Section 10 (Phase 6 verification)

---

### [ ] Step: Phase 7.1 - Code Documentation

**Goal:** Add comprehensive Doxygen documentation

**Tasks:**
- [ ] Add Doxygen comments to all public classes
- [ ] Add Doxygen comments to all public methods
- [ ] Add class-level documentation explaining purpose and usage
- [ ] Add parameter documentation for all method parameters
- [ ] Add return value documentation
- [ ] Add usage examples in comments where helpful
- [ ] Generate Doxygen documentation and review

**Verification:**
```bash
doxygen Doxyfile
# Open docs/html/index.html and verify documentation
```

**References:** spec.md Section 10 (Phase 7)

---

### [ ] Step: Phase 7.2 - Configuration Documentation

**Goal:** Create comprehensive configuration guide

**Tasks:**
- [ ] Document all configuration options in worldserver.conf.dist
- [ ] Create configuration tuning guide document
- [ ] Document performance implications of each setting
- [ ] Provide recommended values for different scenarios (small/medium/large bot count)
- [ ] Document troubleshooting for common config issues

**Verification:**
- Configuration guide reviewed and complete
- All options documented with examples

**References:** spec.md Section 8 (Configuration)

---

### [ ] Step: Phase 7.3 - Troubleshooting Guide

**Goal:** Create guide for diagnosing and fixing movement issues

**Tasks:**
- [ ] Document common issues: bots stuck, bots taking weird paths, performance issues
- [ ] Create diagnostic procedures for each issue type
- [ ] Document how to read movement logs
- [ ] Document how to use debug commands
- [ ] Create troubleshooting flowchart
- [ ] Add FAQ section

**Verification:**
- Troubleshooting guide reviewed and complete

**References:** spec.md Section 10 (Phase 7)

---

### [ ] Step: Phase 7.4 - Performance Tuning Guide

**Goal:** Create guide for optimizing movement system performance

**Tasks:**
- [ ] Document performance tuning parameters
- [ ] Provide guidance for different bot counts (100, 500, 1000+)
- [ ] Document performance monitoring tools
- [ ] Document optimization techniques (cache tuning, validation level adjustment)
- [ ] Document performance profiling procedures
- [ ] Add performance benchmarking guide

**Verification:**
- Performance tuning guide reviewed and complete

**References:** spec.md Section 11.3 (Performance Tests)

---

### [ ] Step: Phase 7.5 - Migration Guide

**Goal:** Create guide for migrating from existing bot systems

**Tasks:**
- [ ] Document differences between old and new movement system
- [ ] Document API changes
- [ ] Create step-by-step migration procedure
- [ ] Document backward compatibility features
- [ ] Document how to enable/disable new system
- [ ] Provide migration examples for common scenarios

**Verification:**
- Migration guide reviewed and complete

**References:** spec.md Section 2.3 (Backward Compatibility)

---

### [ ] Step: Phase 7.6 - Example Scripts and Usage

**Goal:** Provide working examples for bot movement

**Tasks:**
- [ ] Create example script: simple bot movement
- [ ] Create example script: bot following player
- [ ] Create example script: bot patrol route
- [ ] Create example script: formation movement
- [ ] Create example script: stuck recovery testing
- [ ] Document how to use public API in scripts

**Verification:**
- All example scripts run successfully
- Examples cover common use cases

**References:** spec.md Section 9.3 (Public API)

---

### [ ] Step: Phase 7.7 - Final Code Review

**Goal:** Review all code for quality and best practices

**Tasks:**
- [ ] Review all new code for adherence to TrinityCore coding standards
- [ ] Review for potential bugs or edge cases
- [ ] Review error handling and logging
- [ ] Review performance considerations
- [ ] Review thread safety (if applicable)
- [ ] Request peer review from team
- [ ] Address review feedback

**Verification:**
- Code review completed
- All critical feedback addressed

**References:** spec.md Section 14 (Success Criteria)

---

### [ ] Step: Phase 7.8 - Production Readiness Validation

**Goal:** Final validation before production release

**Tasks:**
- [ ] Verify all P0 issues resolved (wall walking, void falls, swimming, hopping, stuck)
- [ ] Verify all unit tests pass (80%+ coverage)
- [ ] Verify all integration tests pass
- [ ] Verify performance benchmarks met
- [ ] Verify stress test passed (30 minutes, 1000 bots)
- [ ] Verify documentation complete
- [ ] Verify backward compatibility validated
- [ ] Create final release checklist
- [ ] Get approval for production deployment

**Verification:**
- All success criteria met (spec.md Section 14)
- System approved for production

**References:** spec.md Section 14 (Success Criteria Summary)

---

## Notes

**Testing Strategy:**
- Unit tests run after each component implementation
- Integration tests run after each phase completion
- Performance benchmarks run in Phase 6
- Stress testing run in Phase 6

**Code Quality:**
- clang-tidy and cppcheck for static analysis
- AddressSanitizer for memory error detection
- ThreadSanitizer for race condition detection
- 80%+ line coverage target

**Performance Targets:**
- p95 path calculation < 5ms
- p99 path calculation < 20ms
- CPU usage < 5% per 100 bots
- Memory < 10MB per 100 bots
- 1000+ concurrent bots supported

**Success Metrics:**
- Wall collision violations < 0.1%
- Void falls < 0.01% per 1000 movements
- Swimming correctness > 99%
- Stuck incidents < 0.1 per bot/hour
- Recovery success rate > 95%
