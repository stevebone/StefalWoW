# Claude Code Prompt: Movement System - Phase 2-6 Implementation

**Copy this prompt into Claude Code to continue the Enterprise Movement System implementation.**

---

## Context

Zenflow has completed Phase 1 of the Enterprise Movement System redesign. The core infrastructure is in place:

**Completed (Phase 1):**
- `src/server/game/Movement/BotMovement/Core/` - Manager, Controller, Config, Defines
- `src/server/game/Movement/BotMovement/Validation/` - PositionValidator, GroundValidator
- `src/server/game/Movement/BotMovement/Pathfinding/PathCache.h` - Header only

**Your Task:** Implement Phase 2-6 following the technical specification.

---

## Read First

```bash
# Read the technical specification
cat .claude/prompts/MOVEMENT_TECHNICAL_SPEC.md

# Read the status summary
cat .claude/analysis/MOVEMENT_ZENFLOW_STATUS.md

# Examine existing implementation
ls -la src/server/game/Movement/BotMovement/
cat src/server/game/Movement/BotMovement/Core/BotMovementDefines.h
cat src/server/game/Movement/BotMovement/Validation/GroundValidator.h
```

---

## Phase 2: Pathfinding Enhancement (Critical for Wall Walking)

### 2.1 CollisionValidator

Create `src/server/game/Movement/BotMovement/Validation/CollisionValidator.h/cpp`:

```cpp
class CollisionValidator
{
public:
    // Check line of sight between two points
    static bool ValidateLineOfSight(Unit const* unit, Position const& start, Position const& end);
    
    // Check collision along entire path
    static ValidationResult CheckCollisionAlongPath(Unit const* unit, Movement::PointsArray const& path);
    
    // Check if position is inside geometry (wall)
    static bool IsInsideGeometry(uint32 mapId, Position const& pos);
};
```

Use TrinityCore's VMAP system:
- `VMAP::VMapFactory::createOrGetVMapManager()`
- `vmap->isInLineOfSight()`

### 2.2 LiquidValidator (Critical for Swimming)

Create `src/server/game/Movement/BotMovement/Validation/LiquidValidator.h/cpp`:

```cpp
struct LiquidInfo
{
    bool isInLiquid;
    float liquidHeight;
    float depth;           // How deep underwater
    LiquidType type;       // Water, Lava, Slime
    bool requiresBreath;
};

class LiquidValidator
{
public:
    static LiquidInfo GetLiquidInfo(Unit const* unit);
    static LiquidInfo GetLiquidInfoAt(uint32 mapId, Position const& pos);
    static bool IsSwimmingRequired(Unit const* unit);
    static bool CheckLiquidTransition(Position const& from, Position const& to, uint32 mapId);
};
```

Use TrinityCore's liquid system:
- `Map::GetLiquidStatus()`
- `ZLiquidStatus` enum

### 2.3 ValidatedPathGenerator

Create `src/server/game/Movement/BotMovement/Pathfinding/ValidatedPathGenerator.h/cpp`:

Wrap TrinityCore's PathGenerator with validation:

```cpp
class ValidatedPathGenerator
{
public:
    explicit ValidatedPathGenerator(WorldObject const* owner);
    
    ValidationResult CalculateValidatedPath(Position const& dest);
    
private:
    PathGenerator _pathGenerator;
    
    ValidationResult ValidateDestination(Position const& dest);
    ValidationResult ValidatePathSegments();
    ValidationResult ValidateEnvironmentTransitions();
};
```

### 2.4 PathCache Implementation

Implement `src/server/game/Movement/BotMovement/Pathfinding/PathCache.cpp`:
- LRU eviction
- TTL expiration
- Cache key = hash(mapId, startPos, endPos)

---

## Phase 3: State Machine (Critical for Swimming/Falling)

### 3.1 State Interface

Create `src/server/game/Movement/BotMovement/StateMachine/MovementState.h`:

```cpp
class MovementState
{
public:
    virtual ~MovementState() = default;
    virtual void OnEnter(MovementStateMachine* sm) = 0;
    virtual void OnExit(MovementStateMachine* sm) = 0;
    virtual void Update(MovementStateMachine* sm, uint32 diff) = 0;
    virtual MovementStateType GetType() const = 0;
    virtual uint32 GetRequiredMovementFlags() const = 0;
};
```

### 3.2 MovementStateMachine

Create `src/server/game/Movement/BotMovement/StateMachine/MovementStateMachine.h/cpp`:

```cpp
class MovementStateMachine
{
public:
    explicit MovementStateMachine(BotMovementController* controller);
    
    void Update(uint32 diff);
    void TransitionTo(MovementStateType newState);
    
    MovementStateType GetCurrentStateType() const;
    Unit* GetOwner() const;
    
    // Environment queries
    bool IsOnGround() const;
    bool IsInWater() const;
    bool IsFlying() const;
    bool IsFalling() const;
    
private:
    BotMovementController* _controller;
    std::unique_ptr<MovementState> _currentState;
    std::unordered_map<MovementStateType, std::unique_ptr<MovementState>> _states;
};
```

### 3.3 State Implementations

Create in `src/server/game/Movement/BotMovement/StateMachine/`:

**IdleState.h/cpp:**
- No movement flags
- Wait for movement command

**GroundMovementState.h/cpp:**
- Set MOVEMENTFLAG_FORWARD
- Check for edges (cliff detection!)
- Check for water entry → transition to Swimming
- Check for falling → transition to Falling

**SwimmingMovementState.h/cpp (CRITICAL!):**
- Set MOVEMENTFLAG_SWIMMING
- Track underwater time
- Surface for air when needed
- Check for water exit → transition to Ground

**FallingMovementState.h/cpp (CRITICAL!):**
- Set MOVEMENTFLAG_FALLING
- Apply gravity
- Detect landing → transition to Ground or Swimming
- Calculate fall damage

**StuckState.h/cpp:**
- Trigger recovery strategies
- Escalate on failure

---

## Phase 4: Stuck Detection

### 4.1 StuckDetector

Create `src/server/game/Movement/BotMovement/Detection/StuckDetector.h/cpp`:

```cpp
class StuckDetector
{
public:
    explicit StuckDetector(BotMovementController* controller);
    
    void Update(uint32 diff);
    void RecordPosition(Position const& pos);
    
    bool IsStuck() const;
    StuckType GetStuckType() const;
    uint32 GetStuckDuration() const;
    
private:
    std::deque<PositionSnapshot> _positionHistory;
    uint32 _stuckStartTime = 0;
    StuckType _stuckType = StuckType::None;
    
    bool DetectPositionStuck();
    bool DetectCollisionStuck();
    bool DetectPathStuck();
};
```

### 4.2 RecoveryStrategies

Create `src/server/game/Movement/BotMovement/Detection/RecoveryStrategies.h/cpp`:

```cpp
enum class RecoveryLevel
{
    Level1_RecalculatePath,
    Level2_BackupAndRetry,
    Level3_RandomNearby,
    Level4_Teleport,
    Level5_Evade
};

class RecoveryStrategies
{
public:
    static bool TryRecover(BotMovementController* controller, StuckType type);
    
private:
    static bool Level1_RecalculatePath(BotMovementController* controller);
    static bool Level2_BackupAndRetry(BotMovementController* controller);
    static bool Level3_RandomNearby(BotMovementController* controller);
    static bool Level4_Teleport(BotMovementController* controller);
    static bool Level5_Evade(BotMovementController* controller);
};
```

---

## Phase 5: Movement Generators

### 5.1 Base Generator

Create `src/server/game/Movement/BotMovement/Generators/BotMovementGeneratorBase.h/cpp`:

Inherit from `MovementGeneratorMedium<Player, BotMovementGeneratorBase>`.

### 5.2 Point Movement

Create `BotPointMovementGenerator.h/cpp`:
- Validate destination before movement
- Use ValidatedPathGenerator
- Integrate with state machine

### 5.3 Follow Movement

Create `BotFollowMovementGenerator.h/cpp`:
- Maintain distance from target
- Update path when target moves significantly
- Formation support

---

## Phase 6: Integration

### 6.1 Update BotMovementController

Integrate state machine and stuck detection:

```cpp
void BotMovementController::Update(uint32 diff)
{
    UpdateStateMachine(diff);
    UpdateStuckDetection(diff);
    SyncMovementFlags();
}
```

### 6.2 CMakeLists.txt

Add new files to `src/server/game/CMakeLists.txt`:

```cmake
# In Movement section, add:
Movement/BotMovement/Core/BotMovementManager.cpp
Movement/BotMovement/Core/BotMovementController.cpp
Movement/BotMovement/Core/BotMovementConfig.cpp
Movement/BotMovement/Validation/PositionValidator.cpp
Movement/BotMovement/Validation/GroundValidator.cpp
Movement/BotMovement/Validation/CollisionValidator.cpp
Movement/BotMovement/Validation/LiquidValidator.cpp
Movement/BotMovement/Pathfinding/ValidatedPathGenerator.cpp
Movement/BotMovement/Pathfinding/PathCache.cpp
Movement/BotMovement/StateMachine/MovementStateMachine.cpp
Movement/BotMovement/StateMachine/IdleState.cpp
Movement/BotMovement/StateMachine/GroundMovementState.cpp
Movement/BotMovement/StateMachine/SwimmingMovementState.cpp
Movement/BotMovement/StateMachine/FallingMovementState.cpp
Movement/BotMovement/StateMachine/StuckState.cpp
Movement/BotMovement/Detection/StuckDetector.cpp
Movement/BotMovement/Detection/RecoveryStrategies.cpp
Movement/BotMovement/Generators/BotMovementGeneratorBase.cpp
Movement/BotMovement/Generators/BotPointMovementGenerator.cpp
Movement/BotMovement/Generators/BotFollowMovementGenerator.cpp
```

---

## Build & Test

```bash
cd C:\TrinityBots\TrinityCore\build
cmake --build . --config RelWithDebInfo --target worldserver -j 8
```

---

## Critical Implementation Notes

1. **MOVEMENTFLAG_SWIMMING** - MUST be set when bot enters water, cleared when exiting
2. **MOVEMENTFLAG_FALLING** - MUST be set when bot falls, cleared on landing
3. **Edge Detection** - Check ground height ahead of movement direction
4. **Collision** - Use VMAP's isInLineOfSight() before every path segment
5. **State Transitions** - Must be deterministic and complete

---

## Expected Outcomes After Implementation

- ✅ Bots stop at walls (CollisionValidator)
- ✅ Bots stop at cliff edges (GroundMovementState edge detection)
- ✅ Bots SWIM in water (SwimmingMovementState + MOVEMENTFLAG_SWIMMING)
- ✅ Bots fall naturally (FallingMovementState + gravity)
- ✅ Bots recover from stuck (StuckDetector + RecoveryStrategies)
- ✅ Bots follow players smoothly (BotFollowMovementGenerator)

---

## Git Commit

```bash
git add -A
git commit -m "feat(movement): Enterprise-Grade Movement System Phase 2-6

Phase 2: Pathfinding Enhancement
- CollisionValidator: VMAP-based collision detection
- LiquidValidator: Water/liquid detection for swimming
- ValidatedPathGenerator: PathGenerator wrapper with validation
- PathCache: LRU caching for performance

Phase 3: State Machine
- MovementStateMachine with proper state transitions
- GroundMovementState with edge detection
- SwimmingMovementState with MOVEMENTFLAG_SWIMMING
- FallingMovementState with gravity
- StuckState for recovery handling

Phase 4: Stuck Detection & Recovery
- StuckDetector: Position/collision/path stuck detection
- RecoveryStrategies: 5-level escalating recovery

Phase 5: Movement Generators
- BotPointMovementGenerator: Validated point movement
- BotFollowMovementGenerator: Formation following

Phase 6: Integration
- CMakeLists.txt updated
- Full state machine integration

Fixes:
- Bots no longer walk through walls
- Bots no longer walk into void
- Bots now swim properly in water
- Bots no longer hop through air
- Bots recover from stuck situations"
```
