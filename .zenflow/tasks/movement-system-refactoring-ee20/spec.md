# Technical Specification: Enterprise-Grade Movement & Pathfinding System Redesign

## Document Information

| Field | Value |
|-------|-------|
| **Version** | 1.0 |
| **Date** | 2026-01-27 |
| **Status** | Draft |
| **PRD Reference** | `requirements.md` v1.0 |

---

## 1. Technical Context

### 1.1 Technology Stack

**Core Technologies:**
- **Language**: C++17 (with selective C++20 features where available)
- **Engine**: TrinityCore 3.3.5a/Master
- **Navigation**: Recast/Detour NavMesh (v1.5+)
- **Build System**: CMake 3.16+
- **Compiler Requirements**: 
  - GCC 10+ / Clang 10+ (Linux/macOS)
  - MSVC 2019+ (Windows)

**Dependencies:**
- G3D Math Library (Vector3, Quaternion)
- Detour NavMesh Query API
- TrinityCore Core APIs:
  - `Unit` / `WorldObject` / `Player`
  - `Map` / `MapManager`
  - `MotionMaster` / `MovementGenerator`
  - `MoveSpline` / `MoveSplineInit`
  - VMAP (collision geometry)

**Existing Infrastructure:**
- Navigation Meshes (*.mmap, *.mmtile files)
- VMAP collision data (*.vmap)
- Liquid data (*.wdt, *.wdb)
- MotionMaster architecture
- MovementGenerator pattern

### 1.2 Current Architecture Assessment

**Strengths:**
- ✅ Robust MotionMaster orchestration system
- ✅ Well-structured MovementGenerator pattern (Strategy pattern)
- ✅ Comprehensive spline movement system (smooth interpolation)
- ✅ Existing PathGenerator with Detour integration
- ✅ Multi-slot movement system (default/active/controlled)
- ✅ Movement flag infrastructure

**Weaknesses (Root Causes):**
- ❌ PathGenerator validation insufficient (doesn't check collision/LOS)
- ❌ No ground height validation before movement execution
- ❌ Swimming state not properly managed
- ❌ Movement flags not synchronized with environmental state
- ❌ No stuck detection/recovery mechanism
- ❌ PathGenerator doesn't validate water transitions
- ❌ No centralized position validation system
- ❌ Limited error handling and fallback strategies

**Critical Gap:**
> The current system generates paths but **does NOT validate** them before execution. There's no safety layer between path generation and movement execution.

---

## 2. Implementation Approach

### 2.1 Architecture Philosophy

**Core Principle:** **Validation-First Movement**

Every movement request must pass through a **validation pipeline** before execution:

```
Movement Request
    ↓
Position Validation (bounds, ground, collision)
    ↓
Path Generation (PathGenerator + MMap)
    ↓
Path Validation (collision, environment transitions)
    ↓
Movement Execution (MotionMaster + Spline)
    ↓
Continuous Monitoring (stuck detection, state sync)
```

**Design Patterns:**
- **Strategy Pattern**: Existing MovementGenerator system (preserve)
- **State Machine Pattern**: Movement state management (new)
- **Decorator Pattern**: Wrap PathGenerator with validation (new)
- **Observer Pattern**: Movement events for stuck detection (new)
- **Singleton Pattern**: Global movement configuration/cache (new)
- **Template Method Pattern**: Base validation workflow (new)

### 2.2 Integration Strategy

**Non-Invasive Enhancement Approach:**

Instead of rewriting TrinityCore's movement system, we will:

1. **Wrap existing systems** with validation layers
2. **Extend (not replace)** MovementGenerator classes
3. **Add new components** alongside existing ones
4. **Preserve backward compatibility** with existing scripts/AI

**Integration Points:**

| TrinityCore Component | Integration Method |
|----------------------|-------------------|
| `PathGenerator` | Wrap with `ValidatedPathGenerator` |
| `MotionMaster` | Inject validation hooks in `Add()` / `MoveX()` methods |
| `MovementGenerator` | Extend with `BotMovementGenerator` base class |
| `Unit::Update()` | Add stuck detection update tick |
| `PlayerAI` | Extend for bot-specific movement logic |

### 2.3 Backward Compatibility

**Compatibility Requirements:**
- ✅ Existing NPCs/creatures continue working unchanged
- ✅ Existing MovementGenerators remain functional
- ✅ Scripts using MotionMaster API work without modification
- ✅ No changes to database schemas
- ✅ No changes to network protocol
- ✅ MMap files compatible without regeneration

**Compatibility Strategy:**
- Use **feature flags** to enable enhanced validation per-unit
- Default behavior: **legacy mode** (current behavior)
- Bot units: **validated mode** (new behavior)
- Config option: `BotMovement.EnableValidation = 1`

---

## 3. Source Code Structure

### 3.1 New Directory Structure

```
src/server/game/Movement/
├── BotMovement/                    # New bot movement subsystem
│   ├── Core/
│   │   ├── BotMovementManager.h/cpp      # Singleton manager
│   │   ├── BotMovementController.h/cpp   # Per-bot controller
│   │   ├── BotMovementConfig.h/cpp       # Configuration
│   │   └── BotMovementDefines.h          # Enums, constants
│   │
│   ├── Validation/
│   │   ├── PositionValidator.h/cpp       # Position/bounds validation
│   │   ├── GroundValidator.h/cpp         # Ground height validation
│   │   ├── CollisionValidator.h/cpp      # LOS/collision checks
│   │   ├── LiquidValidator.h/cpp         # Water/liquid detection
│   │   └── ValidationResult.h            # Result structures
│   │
│   ├── Pathfinding/
│   │   ├── ValidatedPathGenerator.h/cpp  # PathGenerator wrapper
│   │   ├── PathValidationPipeline.h/cpp  # Multi-stage validation
│   │   ├── PathCache.h/cpp               # LRU path caching
│   │   └── PathSmoother.h/cpp            # Path optimization
│   │
│   ├── StateMachine/
│   │   ├── MovementStateMachine.h/cpp    # State machine core
│   │   ├── MovementState.h               # State interface
│   │   ├── GroundMovementState.h/cpp     # Ground movement
│   │   ├── SwimmingMovementState.h/cpp   # Swimming
│   │   ├── FlyingMovementState.h/cpp     # Flying (future)
│   │   ├── FallingMovementState.h/cpp    # Falling
│   │   ├── StuckState.h/cpp              # Stuck handling
│   │   └── IdleState.h/cpp               # Idle
│   │
│   ├── Detection/
│   │   ├── StuckDetector.h/cpp           # Stuck detection
│   │   ├── EnvironmentDetector.h/cpp     # Env state detection
│   │   └── RecoveryStrategies.h/cpp      # Recovery logic
│   │
│   └── Generators/
│       ├── BotMovementGeneratorBase.h/cpp  # Base for bot generators
│       ├── BotPointMovementGenerator.h/cpp # Point movement (validated)
│       ├── BotChaseMovementGenerator.h/cpp # Chase (validated)
│       └── BotFollowMovementGenerator.h/cpp # Follow (validated)
│
├── [Existing TrinityCore files unchanged]
    MotionMaster.h/cpp
    PathGenerator.h/cpp
    MovementGenerators/...
    Spline/...
```

### 3.2 File Size Estimates

| Component | Files | Est. Lines | Complexity |
|-----------|-------|-----------|------------|
| Core | 4 | ~1,500 | Medium |
| Validation | 5 | ~1,800 | High |
| Pathfinding | 4 | ~1,200 | High |
| State Machine | 8 | ~2,000 | Medium |
| Detection | 3 | ~1,000 | Medium |
| Generators | 4 | ~1,500 | Medium |
| **Total** | **28** | **~9,000** | **-** |

---

## 4. Core Component Design

### 4.1 BotMovementManager (Singleton)

**Responsibility:** Global management of bot movement system.

**Key Features:**
- Singleton accessor: `BotMovementManager::instance()`
- Global configuration management
- Path cache management (shared across all bots)
- Performance metrics collection
- Bot controller registry

**Interface:**
```cpp
class BotMovementManager
{
public:
    static BotMovementManager* instance();
    
    // Controller lifecycle
    BotMovementController* GetControllerForUnit(Unit* unit);
    void RegisterController(Unit* unit, BotMovementController* controller);
    void UnregisterController(Unit* unit);
    
    // Configuration
    BotMovementConfig const& GetConfig() const { return _config; }
    void ReloadConfig();
    
    // Path caching
    PathCache* GetPathCache() { return &_globalCache; }
    
    // Metrics
    MovementMetrics GetGlobalMetrics() const;
    void ResetMetrics();
    
private:
    BotMovementManager();
    ~BotMovementManager();
    
    BotMovementConfig _config;
    PathCache _globalCache;
    std::unordered_map<ObjectGuid, BotMovementController*> _controllers;
    MovementMetrics _metrics;
};
```

### 4.2 BotMovementController (Per-Bot)

**Responsibility:** Per-bot movement orchestration and state management.

**Key Features:**
- Owns MovementStateMachine for the bot
- Owns StuckDetector
- Manages ValidatedPathGenerator
- Bridges MotionMaster and validation system
- Tracks movement history

**Interface:**
```cpp
class BotMovementController
{
public:
    explicit BotMovementController(Unit* owner);
    ~BotMovementController();
    
    // Main update loop (called from Unit::Update)
    void Update(uint32 diff);
    
    // Validated movement API
    bool MoveToPosition(Position const& dest, bool forceDest = false);
    bool MoveFollow(Unit* target, float distance, float angle);
    bool MoveChase(Unit* target, float distance);
    
    // State queries
    MovementStateType GetCurrentState() const;
    bool IsStuck() const;
    bool IsMoving() const;
    
    // Path queries
    PathType GetLastPathType() const;
    Movement::PointsArray const& GetCurrentPath() const;
    
    // Recovery
    void TriggerStuckRecovery();
    void ClearStuckState();
    
private:
    Unit* _owner;
    std::unique_ptr<MovementStateMachine> _stateMachine;
    std::unique_ptr<StuckDetector> _stuckDetector;
    std::unique_ptr<ValidatedPathGenerator> _pathGenerator;
    
    // Movement history for stuck detection
    std::deque<PositionSnapshot> _positionHistory;
    TimePoint _lastMovementTime;
    
    // Internal helpers
    void UpdateStateMachine(uint32 diff);
    void UpdateStuckDetection(uint32 diff);
    void SyncMovementFlags();
};
```

### 4.3 Validation Pipeline

**Architecture:**

```
Position Request
    ↓
[1] PositionValidator::ValidateBounds()
    ↓ (pass)
[2] GroundValidator::ValidateGroundHeight()
    ↓ (pass)
[3] CollisionValidator::ValidateLineOfSight()
    ↓ (pass)
[4] LiquidValidator::CheckLiquidTransition()
    ↓ (pass)
ValidatedPathGenerator::CalculatePath()
    ↓
[5] PathValidationPipeline::ValidatePath()
    ↓ (pass)
MotionMaster::MovePoint() / MoveSpline()
```

**ValidationResult Structure:**

```cpp
struct ValidationResult
{
    bool Valid;
    ValidationFailureReason Reason;
    std::string Message;
    Optional<Position> SuggestedAlternative;
    
    static ValidationResult Success() 
    { 
        return { true, ValidationFailureReason::None, "", {} }; 
    }
    
    static ValidationResult Failure(ValidationFailureReason reason, 
                                     std::string message,
                                     Optional<Position> alternative = {})
    {
        return { false, reason, std::move(message), alternative };
    }
    
    explicit operator bool() const { return Valid; }
};

enum class ValidationFailureReason : uint8
{
    None = 0,
    OutOfBounds,
    InvalidHeight,
    NoGroundFound,
    CollisionDetected,
    UnsafeTerrain,
    LiquidBlocking,
    PathGenerationFailed,
    PathTooLong,
    DestinationUnreachable
};
```

### 4.4 ValidatedPathGenerator (Decorator)

**Responsibility:** Wraps PathGenerator with validation.

**Interface:**
```cpp
class ValidatedPathGenerator
{
public:
    explicit ValidatedPathGenerator(WorldObject const* owner);
    
    // Enhanced path calculation with validation
    ValidationResult CalculateValidatedPath(
        Position const& start,
        Position const& dest,
        bool forceDest = false);
    
    // Get results
    PathGenerator const& GetPathGenerator() const { return _pathGenerator; }
    Movement::PointsArray const& GetValidatedPath() const { return _validatedPath; }
    PathType GetPathType() const { return _pathGenerator.GetPathType(); }
    
    // Configuration
    void SetValidationLevel(ValidationLevel level);
    void EnablePathSmoothing(bool enable);
    
private:
    PathGenerator _pathGenerator;
    Movement::PointsArray _validatedPath;
    ValidationLevel _validationLevel;
    
    // Validation stages
    ValidationResult ValidateDestination(Position const& dest);
    ValidationResult ValidatePathSegments(Movement::PointsArray const& path);
    ValidationResult ValidateEnvironmentTransitions(Movement::PointsArray const& path);
};

enum class ValidationLevel : uint8
{
    None = 0,        // Legacy mode - no validation
    Basic = 1,       // Bounds + ground only
    Standard = 2,    // Basic + collision
    Strict = 3       // Standard + environment + all checks
};
```

---

## 5. Movement State Machine

### 5.1 State Diagram

```
                    ┌─────────────┐
                    │    IDLE     │
                    └──────┬──────┘
                           │
                    Movement Request
                           │
                           ▼
           ┌───────────────────────────────┐
           │   Environment Detection       │
           │  (Ground? Water? Air?)        │
           └───────────┬───────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
 ┌──────────┐   ┌──────────┐   ┌──────────┐
 │  GROUND  │   │ SWIMMING │   │  FLYING  │
 └─────┬────┘   └─────┬────┘   └─────┬────┘
       │              │              │
       │   Edge       │   Drowning   │
       │   ───▶   ┌──────────┐      │
       │          │ FALLING  │      │
       └──────▶   └────┬─────┘  ◀───┘
                       │
                   Landing
                       │
                       ▼
                 ┌──────────┐
         Stuck   │  GROUND  │
         ───▶    └──────────┘
                       │
              No Progress 3s
                       │
                       ▼
                 ┌──────────┐
                 │  STUCK   │◀──── Any State
                 └────┬─────┘
                      │
              Recovery Success
                      │
                      ▼
              Previous State or IDLE
```

### 5.2 State Implementations

**Base State Interface:**

```cpp
class MovementState
{
public:
    virtual ~MovementState() = default;
    
    virtual void OnEnter(MovementStateMachine* sm, MovementState* prevState) = 0;
    virtual void OnExit(MovementStateMachine* sm, MovementState* nextState) = 0;
    virtual void Update(MovementStateMachine* sm, uint32 diff) = 0;
    
    virtual MovementStateType GetType() const = 0;
    virtual uint32 GetRequiredMovementFlags() const = 0;
    
protected:
    void TransitionTo(MovementStateMachine* sm, MovementStateType newState);
};

enum class MovementStateType : uint8
{
    Idle = 0,
    Ground = 1,
    Swimming = 2,
    Flying = 3,
    Falling = 4,
    Stuck = 5
};
```

**GroundMovementState:**

```cpp
class GroundMovementState : public MovementState
{
public:
    void OnEnter(MovementStateMachine* sm, MovementState* prevState) override;
    void OnExit(MovementStateMachine* sm, MovementState* nextState) override;
    void Update(MovementStateMachine* sm, uint32 diff) override;
    
    MovementStateType GetType() const override { return MovementStateType::Ground; }
    uint32 GetRequiredMovementFlags() const override 
    { 
        return MOVEMENTFLAG_FORWARD; // No swimming, flying, falling flags
    }
    
private:
    void CheckForEdge(MovementStateMachine* sm);
    void CheckForWater(MovementStateMachine* sm);
};
```

**SwimmingMovementState:**

```cpp
class SwimmingMovementState : public MovementState
{
public:
    void OnEnter(MovementStateMachine* sm, MovementState* prevState) override
    {
        // Set swimming flag
        sm->GetOwner()->SetMovementFlag(MOVEMENTFLAG_SWIMMING, true);
        
        // Trigger swim animation
        sm->GetOwner()->SetAnimationTier(AnimTier::Swim);
        
        _timeUnderwater = 0;
        _needsAir = sm->GetOwner()->GetMaxBreath() > 0;
    }
    
    void OnExit(MovementStateMachine* sm, MovementState* nextState) override
    {
        sm->GetOwner()->SetMovementFlag(MOVEMENTFLAG_SWIMMING, false);
        sm->GetOwner()->SetAnimationTier(AnimTier::Ground);
    }
    
    void Update(MovementStateMachine* sm, uint32 diff) override
    {
        // Check if still in water
        if (!sm->GetOwner()->IsInWater())
        {
            TransitionTo(sm, MovementStateType::Ground);
            return;
        }
        
        // Handle breath mechanic
        if (_needsAir)
        {
            _timeUnderwater += diff;
            if (_timeUnderwater > SURFACE_FOR_AIR_THRESHOLD)
                SurfaceForAir(sm);
        }
        
        // Continue path following with swimming movement
    }
    
    MovementStateType GetType() const override { return MovementStateType::Swimming; }
    uint32 GetRequiredMovementFlags() const override 
    { 
        return MOVEMENTFLAG_SWIMMING; 
    }
    
private:
    uint32 _timeUnderwater;
    bool _needsAir;
    
    void SurfaceForAir(MovementStateMachine* sm);
};
```

### 5.3 MovementStateMachine

**Interface:**

```cpp
class MovementStateMachine
{
public:
    explicit MovementStateMachine(Unit* owner);
    
    void Update(uint32 diff);
    void TransitionTo(MovementStateType newState);
    
    // State queries
    MovementStateType GetCurrentStateType() const;
    MovementState* GetCurrentState() const { return _currentState.get(); }
    
    // Environment queries (used by states)
    bool IsOnGround() const;
    bool IsInWater() const;
    bool IsFlying() const;
    bool IsFalling() const;
    
    Unit* GetOwner() const { return _owner; }
    
private:
    Unit* _owner;
    std::unique_ptr<MovementState> _currentState;
    MovementStateType _currentStateType;
    
    // State instances (reused, not recreated)
    std::array<std::unique_ptr<MovementState>, 6> _states;
    
    void InitializeStates();
    MovementState* GetStateInstance(MovementStateType type);
};
```

---

## 6. Stuck Detection & Recovery

### 6.1 StuckDetector

**Detection Criteria:**

1. **Position-Based:** No position change for N seconds (configurable)
2. **Progress-Based:** Not advancing along path waypoints
3. **Collision-Based:** Repeated collision failures
4. **Path-Based:** Path calculation repeatedly fails

**Interface:**

```cpp
class StuckDetector
{
public:
    explicit StuckDetector(Unit* owner);
    
    void Update(uint32 diff);
    void RecordPosition(Position const& pos);
    void RecordPathFailure();
    void RecordCollision();
    void Reset();
    
    bool IsStuck() const { return _isStuck; }
    StuckType GetStuckType() const { return _stuckType; }
    Milliseconds GetStuckDuration() const;
    
private:
    Unit* _owner;
    bool _isStuck;
    StuckType _stuckType;
    TimePoint _stuckStartTime;
    
    // Detection tracking
    std::deque<PositionSnapshot> _positionHistory;
    uint32 _consecutivePathFailures;
    uint32 _consecutiveCollisions;
    TimePoint _lastMovementTime;
    
    // Configuration (from BotMovementConfig)
    Milliseconds _stuckPositionThreshold;    // e.g., 3000ms
    float _stuckDistanceThreshold;            // e.g., 2.0f yards
    uint32 _stuckPathFailureThreshold;        // e.g., 3 failures
    
    bool CheckPositionStuck();
    bool CheckPathFailureStuck();
    bool CheckCollisionStuck();
};

enum class StuckType : uint8
{
    None = 0,
    PositionStuck,      // Not moving
    GeometryStuck,      // Stuck in collision
    PathFailure,        // Can't find path
    UnreachableTarget   // Target unreachable
};

struct PositionSnapshot
{
    Position Pos;
    TimePoint Timestamp;
    uint32 WaypointIndex; // Which waypoint we're heading to
};
```

### 6.2 Recovery Strategies

**Escalation Levels:**

```cpp
class RecoveryStrategies
{
public:
    static RecoveryResult TryRecoverFromStuck(
        Unit* owner, 
        StuckType stuckType,
        uint32 attemptCount);
    
private:
    // Level 1: Retry current path
    static RecoveryResult Level1_RecalculatePath(Unit* owner);
    
    // Level 2: Back up and retry
    static RecoveryResult Level2_BackupAndRetry(Unit* owner);
    
    // Level 3: Move to random nearby position
    static RecoveryResult Level3_RandomNearbyPosition(Unit* owner);
    
    // Level 4: Teleport to last known-good position
    static RecoveryResult Level4_TeleportToSafePosition(Unit* owner);
    
    // Level 5: Evade/reset
    static RecoveryResult Level5_EvadeAndReset(Unit* owner);
};

struct RecoveryResult
{
    bool Success;
    RecoveryLevel LevelUsed;
    std::string Message;
    
    static RecoveryResult Succeeded(RecoveryLevel level, std::string msg)
    {
        return { true, level, std::move(msg) };
    }
    
    static RecoveryResult Failed(RecoveryLevel level, std::string msg)
    {
        return { false, level, std::move(msg) };
    }
};

enum class RecoveryLevel : uint8
{
    None = 0,
    RecalculatePath = 1,
    BackupAndRetry = 2,
    RandomNearby = 3,
    TeleportSafe = 4,
    EvadeReset = 5
};
```

**Recovery Logic Flow:**

```
Stuck Detected
    │
    ├─ Attempt Count = 0? ──▶ Level 1: Recalculate Path
    │                           ├─ Success ──▶ Clear Stuck, Resume
    │                           └─ Failure ──▶ Increment, Continue
    │
    ├─ Attempt Count = 1? ──▶ Level 2: Backup 5 yards, Retry
    │                           ├─ Success ──▶ Clear Stuck, Resume
    │                           └─ Failure ──▶ Increment, Continue
    │
    ├─ Attempt Count = 2? ──▶ Level 3: Random Nearby (10y radius)
    │                           ├─ Success ──▶ Clear Stuck, Resume
    │                           └─ Failure ──▶ Increment, Continue
    │
    ├─ Attempt Count = 3? ──▶ Level 4: Teleport to Last Safe (10s ago)
    │                           ├─ Success ──▶ Clear Stuck, Resume
    │                           └─ Failure ──▶ Increment, Continue
    │
    └─ Attempt Count = 4+ ──▶ Level 5: Evade/Reset to Home
                                └─ Always Succeeds (hard reset)
```

---

## 7. Integration with TrinityCore

### 7.1 MotionMaster Integration

**Approach:** Inject validation into existing `MovePoint()` / `MoveChase()` methods via hooks.

**Implementation:**

```cpp
// In MotionMaster.cpp - Enhanced MovePoint for bots

void MotionMaster::MovePoint(uint32 id, Position const& pos, bool generatePath, ...)
{
    // Check if this unit is a bot with enhanced movement
    if (BotMovementController* controller = BotMovementManager::instance()
            ->GetControllerForUnit(_owner))
    {
        // Use validated movement
        if (!controller->MoveToPosition(pos, forceDest))
        {
            // Validation failed - log and fallback
            TC_LOG_WARN("movement.bot", "Bot {} validation failed for MovePoint to {}",
                        _owner->GetName(), pos.ToString());
            
            // Fallback to safe position or don't move
            return;
        }
        
        // Controller handles validated movement internally
        return;
    }
    
    // Legacy path - existing logic unchanged
    // ... [existing MovePoint implementation]
}
```

### 7.2 Unit::Update Integration

**Approach:** Call BotMovementController::Update() from Unit::Update().

**Implementation:**

```cpp
// In Unit.cpp - Unit::Update()

void Unit::Update(uint32 diff)
{
    // ... [existing update logic]
    
    // Update movement state machine and stuck detection for bots
    if (BotMovementController* controller = BotMovementManager::instance()
            ->GetControllerForUnit(this))
    {
        controller->Update(diff);
    }
    
    // ... [continue existing update logic]
}
```

### 7.3 PlayerAI Extension

**Approach:** Create BotPlayerAI that extends PlayerAI with enhanced movement.

**Implementation:**

```cpp
// New file: src/server/game/AI/PlayerAI/BotPlayerAI.h

class BotPlayerAI : public PlayerAI
{
public:
    explicit BotPlayerAI(Player* player);
    ~BotPlayerAI() override;
    
    void UpdateAI(uint32 diff) override;
    void OnCharmed(bool isNew) override;
    
    // Enhanced movement commands
    bool MoveToPosition(Position const& pos);
    bool FollowTarget(Unit* target, float distance);
    bool ChaseTarget(Unit* target);
    
    // Status queries
    bool IsMoving() const;
    bool IsStuck() const;
    
private:
    std::unique_ptr<BotMovementController> _movementController;
};
```

---

## 8. Configuration & Tuning

### 8.1 Configuration File

**File:** `src/server/worldserver/worldserver.conf.dist`

**New Section:**

```ini
###############################################################################
# BOT MOVEMENT SYSTEM
#
# BotMovement.Enable
#     Enable enhanced bot movement system with validation and stuck recovery.
#     Default: 1 (enabled)

BotMovement.Enable = 1

# BotMovement.ValidationLevel
#     0 = None (legacy mode)
#     1 = Basic (bounds + ground only)
#     2 = Standard (basic + collision)
#     3 = Strict (all checks)
#     Default: 2

BotMovement.ValidationLevel = 2

# BotMovement.StuckDetection.PositionThreshold
#     Time in milliseconds without position change to consider stuck.
#     Default: 3000 (3 seconds)

BotMovement.StuckDetection.PositionThreshold = 3000

# BotMovement.StuckDetection.DistanceThreshold
#     Distance in yards below which bot is considered not moving.
#     Default: 2.0

BotMovement.StuckDetection.DistanceThreshold = 2.0

# BotMovement.Recovery.MaxAttempts
#     Maximum recovery attempts before evade/reset.
#     Default: 5

BotMovement.Recovery.MaxAttempts = 5

# BotMovement.PathCache.Size
#     Number of paths to cache (LRU eviction).
#     Default: 1000

BotMovement.PathCache.Size = 1000

# BotMovement.PathCache.TTL
#     Path cache time-to-live in seconds.
#     Default: 60

BotMovement.PathCache.TTL = 60

# BotMovement.Debug.LogLevel
#     0 = Disabled
#     1 = Errors only
#     2 = Warnings + Errors
#     3 = Info + Warnings + Errors
#     4 = Debug (verbose)
#     Default: 2

BotMovement.Debug.LogLevel = 2
```

### 8.2 BotMovementConfig Class

```cpp
class BotMovementConfig
{
public:
    void Load();
    void Reload();
    
    // Getters for config values
    bool IsEnabled() const { return _enabled; }
    ValidationLevel GetValidationLevel() const { return _validationLevel; }
    Milliseconds GetStuckPositionThreshold() const { return _stuckPosThreshold; }
    float GetStuckDistanceThreshold() const { return _stuckDistThreshold; }
    uint32 GetMaxRecoveryAttempts() const { return _maxRecoveryAttempts; }
    uint32 GetPathCacheSize() const { return _pathCacheSize; }
    Seconds GetPathCacheTTL() const { return _pathCacheTTL; }
    uint32 GetDebugLogLevel() const { return _debugLogLevel; }
    
private:
    bool _enabled;
    ValidationLevel _validationLevel;
    Milliseconds _stuckPosThreshold;
    float _stuckDistThreshold;
    uint32 _maxRecoveryAttempts;
    uint32 _pathCacheSize;
    Seconds _pathCacheTTL;
    uint32 _debugLogLevel;
};
```

---

## 9. Data Model / API / Interface Changes

### 9.1 Database Changes

**None required.** All configuration is in worldserver.conf.

### 9.2 Network Protocol Changes

**None required.** Existing movement packets (SMSG_MONSTER_MOVE, SMSG_SPLINE_MOVE_*) remain unchanged.

### 9.3 Public API Additions

**New Public APIs for Bot Control:**

```cpp
// Get/create bot movement controller
BotMovementController* GetBotController(Unit* unit);

// Enable bot movement for a unit
void EnableBotMovement(Unit* unit);

// Disable bot movement for a unit
void DisableBotMovement(Unit* unit);

// Query APIs
bool IsBotStuck(Unit* unit);
MovementStateType GetBotMovementState(Unit* unit);
PathType GetBotLastPathType(Unit* unit);

// Recovery APIs
void TriggerBotStuckRecovery(Unit* unit);
void ClearBotStuckState(Unit* unit);

// Metrics APIs
BotMovementMetrics GetBotMovementMetrics(Unit* unit);
GlobalBotMovementMetrics GetGlobalBotMovementMetrics();
```

---

## 10. Delivery Phases

### Phase 1: Foundation (Week 1-2)

**Goal:** Core infrastructure and validation framework.

**Deliverables:**
- ✅ BotMovementManager (singleton)
- ✅ BotMovementController (per-bot)
- ✅ BotMovementConfig (configuration loading)
- ✅ ValidationResult structures
- ✅ PositionValidator (bounds checking)
- ✅ GroundValidator (height validation)
- ✅ Unit tests for validators

**Verification:**
```bash
# Unit tests pass
./worldserver --run-tests BotMovement.*

# Validators correctly reject invalid positions
# Manual test: teleport bot to invalid position, verify rejection
```

### Phase 2: Pathfinding Enhancement (Week 2-3)

**Goal:** Validated path generation and caching.

**Deliverables:**
- ✅ ValidatedPathGenerator (PathGenerator wrapper)
- ✅ CollisionValidator (LOS checks)
- ✅ LiquidValidator (water detection)
- ✅ PathValidationPipeline
- ✅ PathCache (LRU caching)
- ✅ Integration tests for pathfinding

**Verification:**
```bash
# Pathfinding tests pass
./worldserver --run-tests BotMovement.Pathfinding.*

# Bot correctly rejects paths through walls
# Bot correctly handles water transitions
# Path cache hit rate > 30% in test scenario
```

### Phase 3: State Machine (Week 3-4)

**Goal:** Movement state management and environmental transitions.

**Deliverables:**
- ✅ MovementStateMachine core
- ✅ MovementState base class
- ✅ GroundMovementState
- ✅ SwimmingMovementState
- ✅ FallingMovementState
- ✅ IdleState
- ✅ State transition tests

**Verification:**
```bash
# State machine tests pass
./worldserver --run-tests BotMovement.StateMachine.*

# Bot correctly transitions Ground → Water → Ground
# Swimming flag set/unset correctly
# Falling detected on edges
```

### Phase 4: Stuck Detection & Recovery (Week 4-5)

**Goal:** Reliable stuck detection and automatic recovery.

**Deliverables:**
- ✅ StuckDetector implementation
- ✅ RecoveryStrategies implementation
- ✅ StuckState (state machine state)
- ✅ Recovery escalation logic
- ✅ Stuck detection integration tests

**Verification:**
```bash
# Stuck detection tests pass
./worldserver --run-tests BotMovement.StuckDetection.*

# Bot detects stuck within 3 seconds
# Bot recovers automatically (95%+ success rate)
# Recovery attempts escalate correctly
# Evade/reset works as last resort
```

### Phase 5: Movement Generators (Week 5-6)

**Goal:** Bot-specific MovementGenerator implementations.

**Deliverables:**
- ✅ BotMovementGeneratorBase
- ✅ BotPointMovementGenerator
- ✅ BotChaseMovementGenerator
- ✅ BotFollowMovementGenerator
- ✅ Integration with MotionMaster

**Verification:**
```bash
# Movement generator tests pass
./worldserver --run-tests BotMovement.Generators.*

# Bot follows player correctly (no wall clipping, no water hopping)
# Bot chases target correctly (proper pathing)
# Bot moves to point correctly (ground validation, collision avoidance)
```

### Phase 6: Integration & Testing (Week 6-7)

**Goal:** Full system integration and stress testing.

**Deliverables:**
- ✅ MotionMaster integration hooks
- ✅ Unit::Update integration
- ✅ BotPlayerAI implementation
- ✅ Configuration loading
- ✅ Comprehensive integration tests
- ✅ Performance benchmarks
- ✅ 1000-bot stress test

**Verification:**
```bash
# All tests pass
./worldserver --run-tests BotMovement.*

# Performance benchmarks meet targets:
# - p95 path calc < 5ms
# - 1000 bots < 5% CPU per 100 bots
# - Memory < 10MB per 100 bots

# Stress test: 1000 bots moving simultaneously
# - All bots move successfully
# - < 0.1% stuck incidents
# - Server maintains 50+ FPS
```

### Phase 7: Polish & Documentation (Week 7-8)

**Goal:** Production-ready system with documentation.

**Deliverables:**
- ✅ Code documentation (Doxygen comments)
- ✅ Configuration documentation
- ✅ Troubleshooting guide
- ✅ Performance tuning guide
- ✅ Migration guide (for existing bot systems)
- ✅ Example scripts/usage

**Verification:**
- Documentation reviewed and complete
- All critical issues resolved
- Code reviewed by team
- Performance validated on production-like environment

---

## 11. Verification Approach

### 11.1 Unit Tests

**Framework:** Catch2 (existing TrinityCore test framework)

**Test Structure:**
```
tests/game/Movement/BotMovement/
├── ValidatorTests.cpp           # PositionValidator, GroundValidator, etc.
├── PathGeneratorTests.cpp       # ValidatedPathGenerator
├── StateMachineTests.cpp        # State transitions
├── StuckDetectionTests.cpp      # StuckDetector
├── RecoveryTests.cpp            # RecoveryStrategies
└── IntegrationTests.cpp         # Full system integration
```

**Coverage Target:** 80%+ line coverage for new code.

**Running Tests:**
```bash
# Run all bot movement tests
./worldserver --run-tests "BotMovement.*"

# Run specific test suite
./worldserver --run-tests "BotMovement.Validators.*"

# Generate coverage report
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
```

### 11.2 Integration Tests

**Test Scenarios:**

| Test ID | Scenario | Expected Result |
|---------|----------|----------------|
| INT-001 | Bot walks into wall | Stops at wall, recalculates path |
| INT-002 | Bot walks to cliff edge | Stops at edge, doesn't fall |
| INT-003 | Bot walks into water | Smooth transition to swimming |
| INT-004 | Bot stuck in corner | Detects stuck, recovers within 10s |
| INT-005 | Bot follows player through complex path | No wall clipping, natural path |
| INT-006 | Bot target unreachable | Detects unreachable, stops gracefully |
| INT-007 | 100 bots move simultaneously | All move correctly, no stuck |
| INT-008 | Bot on bridge (multi-level terrain) | Correct Z-coordinate, stays on bridge |

**Test Maps:**
- Elwynn Forest (basic terrain)
- Stormwind City (complex geometry, bridges)
- Thousand Needles (cliffs, water)
- Deeprun Tram (underground, complex)

### 11.3 Performance Tests

**Benchmark Targets:**

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Path calculation (p95) | < 5ms | Average over 10,000 paths |
| Path calculation (p99) | < 20ms | Average over 10,000 paths |
| Stuck detection overhead | < 100μs/update | Microbenchmark |
| Validation overhead | < 1ms/destination | Microbenchmark |
| CPU usage (100 bots) | < 5% | 10-minute sustained load |
| Memory (100 bots) | < 10MB | Heap profiling |
| Cache hit rate | > 30% | Instrumentation |

**Stress Test:**
```
Test: 1000 Concurrent Bots
Duration: 30 minutes
Actions: Random movement, follow, chase
Success Criteria:
  - All bots functional
  - Server FPS > 50
  - < 0.1% stuck incidents
  - < 1% validation failures
  - No crashes or deadlocks
```

### 11.4 Lint & Typecheck

**Tools:**
- clang-tidy (C++ static analysis)
- cppcheck (additional static analysis)
- Address Sanitizer (runtime error detection)
- Thread Sanitizer (race condition detection)

**Commands:**
```bash
# Run static analysis
cmake -DCMAKE_CXX_CLANG_TIDY="clang-tidy" ..
make

# Run with AddressSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
make
./worldserver

# Run with ThreadSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON ..
make
./worldserver
```

---

## 12. Risk Assessment & Mitigation

### 12.1 Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| **NavMesh Inaccuracy** | High | High | Validation layer catches issues; fallback to direct LOS path if NavMesh unreliable |
| **Performance Regression** | Medium | High | Extensive benchmarking; async path calculation option; caching |
| **Integration Breaks Existing AI** | Medium | Medium | Backward compatibility mode; feature flags; comprehensive testing |
| **Stuck Detection False Positives** | Medium | Medium | Tunable thresholds; multiple detection criteria; config per-bot-type |
| **Race Conditions (multi-threading)** | Low | High | Avoid shared mutable state; use thread-safe collections; TSan validation |
| **Memory Leaks** | Low | Medium | Smart pointers; RAII; ASan validation |

### 12.2 Schedule Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| **Scope Creep** | High | High | Strict MVP definition; defer non-critical features to Phase 2+ |
| **Underestimated Complexity** | Medium | High | Add 20% buffer to estimates; prioritize ruthlessly |
| **Blocking Dependencies** | Low | Medium | Identify dependencies early; design for loose coupling |
| **Testing Takes Longer** | Medium | Medium | Start testing early; automate as much as possible |

### 12.3 Operational Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| **Difficult to Debug** | Medium | Medium | Comprehensive logging; visual debugging tools; metrics |
| **Configuration Too Complex** | Medium | Low | Sensible defaults; documentation; validation |
| **Hard to Tune** | Medium | Medium | Runtime tuning; A/B testing; metrics dashboard |

---

## 13. Open Questions & Decisions Needed

### 13.1 Critical Decisions

**Q1: Async Path Calculation?**
- **Question:** Should path calculation be async (off main thread)?
- **Pros:** Better performance for 1000+ bots
- **Cons:** Added complexity, race conditions, callback overhead
- **Recommendation:** Start synchronous; add async if benchmarks show need
- **Decision:** TBD after Phase 6 performance testing

**Q2: Flying Movement Scope?**
- **Question:** Include flying bots in MVP?
- **Recommendation:** Defer to Phase 2 (not in MVP)
- **Decision:** Deferred (MVP: ground + swimming only)

**Q3: Dynamic Obstacle Avoidance?**
- **Question:** Should bots avoid other bots/players dynamically?
- **Recommendation:** Not in MVP (YAGNI); add if needed
- **Decision:** Deferred

### 13.2 Design Decisions

**Q4: Path Smoothing Algorithm?**
- **Options:** Spline smoothing vs. corner cutting vs. both
- **Recommendation:** Both (corner cutting + spline)
- **Decision:** TBD during Phase 2 implementation

**Q5: Stuck Detection Tuning?**
- **Question:** Default thresholds (3s, 2.0y) sufficient?
- **Recommendation:** Start with defaults, tune based on testing
- **Decision:** Use defaults, provide config options

**Q6: Recovery Teleport Safety?**
- **Question:** Is teleporting bots acceptable for recovery?
- **Recommendation:** Yes, as last resort (Level 4)
- **Decision:** Approved

---

## 14. Success Criteria Summary

### 14.1 Functional Requirements

- ✅ **P0-1:** Bots respect walls and collision geometry (99.9% success)
- ✅ **P0-2:** Bots don't fall off cliffs/into void (99.99% success)
- ✅ **P0-3:** Bots swim naturally in water (99%+ correct)
- ✅ **P0-4:** Bots don't hop randomly (90%+ reduction)
- ✅ **P0-5:** Bots detect and recover from stuck (95%+ recovery)
- ✅ **P1-6:** Bots take natural paths (90%+ path quality)

### 14.2 Non-Functional Requirements

- ✅ **Performance:** p95 path calc < 5ms, p99 < 20ms
- ✅ **Scalability:** 1000+ concurrent bots supported
- ✅ **Reliability:** 99.9% movement success rate
- ✅ **Maintainability:** 80%+ test coverage, clear architecture
- ✅ **Compatibility:** Existing NPCs/scripts work unchanged

### 14.3 Acceptance Criteria

**System is production-ready when:**
1. All P0 issues resolved (wall walking, void falls, swimming, stuck)
2. All unit tests pass (80%+ coverage)
3. All integration tests pass
4. Performance benchmarks met (1000 bots, < 5ms p95)
5. Stress test successful (30 minutes, 1000 bots, no failures)
6. Documentation complete (code docs, config docs, troubleshooting)
7. Code reviewed and approved
8. Backward compatibility verified (existing AI works)

---

## 15. Dependencies & External Factors

### 15.1 Internal Dependencies

| Dependency | Status | Impact if Delayed |
|------------|--------|------------------|
| TrinityCore master branch | Stable | None (track upstream) |
| MMap files | Existing | None (use existing) |
| Recast/Detour library | Integrated | None (already integrated) |
| Unit test framework (Catch2) | Existing | None |

### 15.2 External Dependencies

| Dependency | Version | Impact if Unavailable |
|------------|---------|---------------------|
| C++17 compiler | GCC 10+ / Clang 10+ / MSVC 2019+ | Cannot compile (required) |
| CMake | 3.16+ | Cannot build (required) |
| MMap data files | Any version | Pathfinding disabled (fallback to LOS) |

### 15.3 Assumptions

1. ✅ NavMesh files are reasonably accurate (validated by testing)
2. ✅ TrinityCore core is stable (track master branch)
3. ✅ Server hardware can handle 1000+ units (validate with stress test)
4. ✅ Test environment available (dev server with MMap data)
5. ✅ Bot decision-making (AI) is separate from movement system

---

## 16. Glossary

| Term | Definition |
|------|------------|
| **Bot** | AI-controlled player character |
| **MMap** | Movement Map - navigation mesh file format |
| **NavMesh** | Navigation Mesh - 3D polygon mesh for pathfinding |
| **Detour** | Navigation library for pathfinding queries |
| **Recast** | NavMesh generation library |
| **VMAP** | Visual Map - collision geometry for LOS |
| **LOS** | Line of Sight - visibility/collision check |
| **Spline** | Smooth curve for movement interpolation |
| **PolyRef** | Navigation mesh polygon reference ID |
| **Ground Height** | Z-coordinate of terrain at (X, Y) |
| **Stuck** | Bot unable to make progress for N seconds |
| **Validation** | Checking if position/path is safe/valid |
| **Recovery** | Automatic actions to resolve stuck state |

---

## Document Approval

| Role | Name | Status | Date |
|------|------|--------|------|
| **Author** | AI Assistant | Draft | 2026-01-27 |
| **Tech Lead** | TBD | Pending | - |
| **Architect** | TBD | Pending | - |

**Next Steps:**
1. Review this technical specification
2. Validate design decisions (esp. Q1-Q6 in Section 13)
3. Approve for implementation
4. Proceed to Planning phase (break into tasks)

---

**End of Technical Specification**
