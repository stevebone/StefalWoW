# Zenflow Task: Movement & Pathfinding System - Complete Enterprise Redesign

**Priority**: P0 (Critical - Core System Broken)  
**Estimated Duration**: 8-12 hours analysis, then full implementation  
**Scope**: Complete system redesign - Enterprise Grade Quality

---

## Context

The current movement and pathfinding system is fundamentally broken:

### Critical Issues
1. **Bots walk THROUGH walls** - Collision detection broken/missing
2. **Bots walk INTO void/off cliffs** - No ground validation
3. **Bots DON'T swim** - They hop/jump over water surface
4. **Bots hop through air** - Random jumping, no proper flight/fall handling
5. **Bots get stuck** - No recovery mechanism
6. **Bots take weird paths** - Inefficient or completely broken pathfinding
7. **Formation breaks** - Group movement is chaotic

### Goal
**Complete redesign** of the movement system with:
- Enterprise-grade architecture
- Full TrinityCore integration (MMaps, PathGenerator, MotionMaster)
- Proper state machine (Ground, Swimming, Flying, Falling, Mounted)
- Robust collision detection
- Sustainable, maintainable code

---

## Phase 1: Complete Current System Audit (2h)

### 1.1 Map All Movement Components

Find EVERY file related to movement:
```bash
# Find all movement files in Playerbot
find src/modules/Playerbot -name "*.cpp" -o -name "*.h" | xargs grep -l -i "movement\|motion\|path\|follow\|chase\|swim\|fly\|jump" | sort -u

# Find movement directories
find src/modules/Playerbot -type d -name "*[Mm]ovement*" -o -name "*[Mm]otion*" -o -name "*[Pp]ath*"
```

Create a complete file inventory:
```
src/modules/Playerbot/
├── AI/Movement/           <- Document ALL files
├── AI/Strategy/           <- Movement strategies
├── AI/Actions/            <- Movement actions
├── AI/Triggers/           <- Movement triggers
└── [other locations]      <- Find them all
```

### 1.2 Document Current Architecture

For each component found:
- Purpose
- Dependencies
- How it's called
- Known issues
- Code quality assessment (1-5 stars)

### 1.3 Identify Integration Points

Map how bot movement connects to TrinityCore:
```
Bot Movement Code
      │
      ▼
[Integration Layer] ← Document this completely
      │
      ▼
TrinityCore Movement (MotionMaster, PathGenerator, MMap)
```

---

## Phase 2: Problem Root Cause Analysis (2h)

### 2.1 Wall Walking Analysis

```bash
grep -rn "collision\|Collision\|LOS\|LineOfSight\|CanReach\|IsReachable" src/modules/Playerbot --include="*.cpp" --include="*.h"
```

**Investigate:**
- Is PathGenerator being used? If not, why?
- Is collision checked BEFORE movement or AFTER?
- Are there shortcuts that bypass collision?
- Is MMap navmesh being queried?

**Document:**
- Exact code path from "decide to move" to "execute movement"
- Where collision SHOULD be checked but ISN'T
- Why bots can walk through walls

### 2.2 Void/Cliff Walking Analysis

```bash
grep -rn "GetHeight\|GetGroundZ\|GetFloor\|ValidatePos\|INVALID_HEIGHT\|IsInWater\|IsUnderWater" src/modules/Playerbot --include="*.cpp" --include="*.h"
```

**Investigate:**
- Is destination Z-coordinate validated?
- Is there ground detection?
- What happens when GetHeight returns invalid?
- Is there a check for "falling into void"?

**Document:**
- How Z-coordinate is currently handled
- Missing validation points
- Why bots walk off cliffs

### 2.3 Swimming Analysis

```bash
grep -rn "swim\|Swim\|SWIM\|water\|Water\|WATER\|IsInWater\|IsSwimming\|MOVEMENTFLAG_SWIMMING" src/modules/Playerbot --include="*.cpp" --include="*.h"
```

**Investigate:**
- Is swimming state detected?
- Are swimming movement flags set?
- Is water level checked?
- Why do bots HOP instead of swim?

**Document:**
- Current water handling (or lack thereof)
- Missing MOVEMENTFLAG_SWIMMING
- How TrinityCore NPCs handle swimming

### 2.4 Air Hopping Analysis

```bash
grep -rn "jump\|Jump\|JUMP\|fall\|Fall\|FALL\|fly\|Fly\|FLY\|hover\|Hover" src/modules/Playerbot --include="*.cpp" --include="*.h"
```

**Investigate:**
- Why are bots jumping randomly?
- Is there improper height interpolation?
- Are movement packets malformed?
- Is gravity being applied?

**Document:**
- What triggers the hopping behavior
- Missing fall/gravity handling
- Packet analysis if needed

### 2.5 Stuck Detection Analysis

```bash
grep -rn "stuck\|Stuck\|STUCK\|timeout\|Timeout\|reset\|Reset" src/modules/Playerbot --include="*.cpp" --include="*.h"
```

**Investigate:**
- Is there ANY stuck detection?
- What's the timeout before recovery?
- What recovery actions exist?
- Why doesn't recovery work?

---

## Phase 3: TrinityCore Movement Deep Dive (1.5h)

### 3.1 Core Movement Systems

Analyze TrinityCore's movement (the correct implementation):
```
src/server/game/Movement/
├── MotionMaster.h/.cpp        <- Movement state machine
├── MovementGenerator.h        <- Base movement interface
├── PathGenerator.h/.cpp       <- A* pathfinding with MMap
├── MMapManager.h/.cpp         <- Navigation mesh management
├── MMapFactory.h/.cpp         <- MMap loading
├── Spline/                    <- Smooth movement interpolation
└── MovementDefines.h          <- Movement flags and constants
```

**Document:**
- How MotionMaster manages movement states
- How PathGenerator uses MMaps
- How MovementGenerators work (Follow, Chase, Point, etc.)
- Movement flags (MOVEMENTFLAG_*)

### 3.2 NPC Movement Reference

Analyze how NPCs move correctly:
```
src/server/game/AI/
├── CreatureAI.cpp             <- NPC AI base
├── PetAI.cpp                  <- Pet movement (similar to bot needs)
├── SmartAI.cpp                <- Scripted movement
└── ScriptedAI/                <- Boss/encounter movement
```

**Document:**
- How NPCs use MotionMaster
- How NPCs handle swimming
- How NPCs avoid falling off cliffs
- How NPCs pathfind around obstacles

### 3.3 Movement Packets

Understand client-server movement sync:
```bash
grep -rn "SMSG_MONSTER_MOVE\|MSG_MOVE\|CMSG_MOVE" src/server --include="*.cpp" --include="*.h" | head -30
```

**Document:**
- Key movement opcodes
- Packet structure for smooth movement
- How splines work

---

## Phase 4: Enterprise Architecture Design (2h)

### 4.1 Design New Movement System

Create a complete enterprise-grade architecture:

```
┌─────────────────────────────────────────────────────────────────┐
│                    BotMovementManager                           │
│  (Singleton - Central coordination point)                       │
├─────────────────────────────────────────────────────────────────┤
│  - RegisterBot(bot)                                             │
│  - UnregisterBot(bot)                                           │
│  - Update(diff) - Called every tick                             │
│  - GetMovementState(bot) -> IMovementState*                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    BotMovementController                        │
│  (Per-bot instance - owns movement state)                       │
├─────────────────────────────────────────────────────────────────┤
│  - currentState: IMovementState*                                │
│  - stateHistory: deque<MovementStateRecord>                     │
│  - pathCache: PathCache                                         │
│  - stuckDetector: StuckDetector                                 │
│  - MoveTo(destination, options)                                 │
│  - Follow(target, distance, angle)                              │
│  - Stop()                                                       │
│  - Update(diff)                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
            ┌─────────────────┼─────────────────┐
            ▼                 ▼                 ▼
┌───────────────────┐ ┌───────────────────┐ ┌───────────────────┐
│  GroundMovement   │ │  SwimMovement     │ │  FlyMovement      │
│  State            │ │  State            │ │  State            │
├───────────────────┤ ├───────────────────┤ ├───────────────────┤
│ - Walking         │ │ - Surface swim    │ │ - Mounted flight  │
│ - Running         │ │ - Underwater      │ │ - Levitate        │
│ - Sprinting       │ │ - Diving          │ │ - Slow fall       │
│ - Mounted ground  │ │ - Ascending       │ │ - Freefall        │
└───────────────────┘ └───────────────────┘ └───────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    BotPathfinder                                │
│  (Wraps TrinityCore PathGenerator with bot-specific logic)      │
├─────────────────────────────────────────────────────────────────┤
│  - GeneratePath(start, end, options) -> Path                    │
│  - ValidatePosition(pos) -> bool                                │
│  - GetNearestValidPosition(pos) -> Position                     │
│  - IsPathClear(start, end) -> bool                              │
│  - Uses: PathGenerator, MMapManager, VMapManager                │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    MovementValidator                            │
│  (Validates ALL movement before execution)                      │
├─────────────────────────────────────────────────────────────────┤
│  - ValidateDestination(pos) -> ValidationResult                 │
│  - CheckCollision(start, end) -> bool                           │
│  - CheckGroundZ(pos) -> float                                   │
│  - CheckWaterLevel(pos) -> WaterData                            │
│  - CheckLOS(start, end) -> bool                                 │
│  - IsInVoid(pos) -> bool                                        │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    StuckDetector                                │
│  (Detects and recovers from stuck situations)                   │
├─────────────────────────────────────────────────────────────────┤
│  - Update(diff, currentPos)                                     │
│  - IsStuck() -> bool                                            │
│  - GetStuckDuration() -> uint32                                 │
│  - GetRecoveryAction() -> RecoveryAction                        │
│  - RecoveryActions: Teleport, Jump, Unstuck command, etc.       │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 Movement State Machine

Design complete state machine:

```
                    ┌──────────────┐
                    │    IDLE      │
                    └──────┬───────┘
                           │ MoveTo() called
                           ▼
                    ┌──────────────┐
          ┌─────────│  EVALUATING  │─────────┐
          │         └──────────────┘         │
          │ Ground        │ Water      Flight│
          ▼               ▼                  ▼
   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
   │   GROUND     │ │   SWIMMING   │ │   FLYING     │
   │   MOVEMENT   │ │              │ │              │
   ├──────────────┤ ├──────────────┤ ├──────────────┤
   │ - WALKING    │ │ - SURFACE    │ │ - ASCENDING  │
   │ - RUNNING    │ │ - UNDERWATER │ │ - CRUISING   │
   │ - MOUNTED    │ │ - DIVING     │ │ - DESCENDING │
   └──────┬───────┘ └──────┬───────┘ └──────┬───────┘
          │                │                │
          │ Destination    │                │
          │ reached        │                │
          └────────────────┴────────────────┘
                           │
                           ▼
                    ┌──────────────┐
                    │   ARRIVED    │
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────┐
                    │    IDLE      │
                    └──────────────┘

        Special States:
        ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
        │   FALLING    │ │    STUCK     │ │  RECOVERING  │
        └──────────────┘ └──────────────┘ └──────────────┘
```

### 4.3 Design Validation Pipeline

Every movement MUST pass through:

```
1. DESTINATION VALIDATION
   ├── Is position in valid map bounds?
   ├── Is there ground at destination?
   ├── Is destination accessible (not in wall)?
   └── Is destination in water/air (set appropriate state)?

2. PATH GENERATION
   ├── Use PathGenerator with MMap
   ├── Validate each path point
   ├── Smooth path with splines
   └── Cache path for efficiency

3. COLLISION CHECK
   ├── Check LOS to next waypoint
   ├── Check for obstacles
   ├── Check for height changes
   └── Adjust path if needed

4. MOVEMENT EXECUTION
   ├── Set correct movement flags
   ├── Use MotionMaster properly
   ├── Sync with client (packets)
   └── Update position server-side

5. CONTINUOUS VALIDATION
   ├── Monitor actual vs expected position
   ├── Detect stuck conditions
   ├── Trigger recovery if needed
   └── Log anomalies for debugging
```

---

## Phase 5: Implementation Specification (1.5h)

### 5.1 New File Structure

```
src/modules/Playerbot/AI/Movement/
├── Core/
│   ├── BotMovementManager.h/.cpp       <- Singleton manager
│   ├── BotMovementController.h/.cpp    <- Per-bot controller
│   ├── MovementDefines.h               <- Enums, constants
│   └── MovementConfig.h/.cpp           <- Configuration
│
├── States/
│   ├── IMovementState.h                <- State interface
│   ├── IdleState.h/.cpp
│   ├── GroundMovementState.h/.cpp
│   ├── SwimmingState.h/.cpp
│   ├── FlyingState.h/.cpp
│   ├── FallingState.h/.cpp
│   └── StuckState.h/.cpp
│
├── Pathfinding/
│   ├── BotPathfinder.h/.cpp            <- PathGenerator wrapper
│   ├── PathCache.h/.cpp                <- Path caching
│   ├── PathSmoother.h/.cpp             <- Spline smoothing
│   └── PathValidator.h/.cpp            <- Path validation
│
├── Validation/
│   ├── MovementValidator.h/.cpp        <- Position validation
│   ├── CollisionChecker.h/.cpp         <- Collision detection
│   ├── GroundChecker.h/.cpp            <- Ground/height checks
│   └── WaterChecker.h/.cpp             <- Water detection
│
├── Detection/
│   ├── StuckDetector.h/.cpp            <- Stuck detection
│   ├── EnvironmentDetector.h/.cpp      <- Ground/water/air detection
│   └── RecoveryManager.h/.cpp          <- Stuck recovery
│
├── Behaviors/
│   ├── FollowBehavior.h/.cpp           <- Follow player/target
│   ├── ChaseBehavior.h/.cpp            <- Chase enemy
│   ├── FormationBehavior.h/.cpp        <- Group formations
│   ├── PatrolBehavior.h/.cpp           <- Patrol paths
│   └── FleeeBehavior.h/.cpp            <- Flee from danger
│
└── Integration/
    ├── MotionMasterBridge.h/.cpp       <- TrinityCore MotionMaster integration
    ├── PacketBuilder.h/.cpp            <- Movement packet creation
    └── SplineIntegration.h/.cpp        <- Spline movement
```

### 5.2 Key Interfaces

```cpp
// IMovementState.h
class IMovementState
{
public:
    virtual ~IMovementState() = default;
    
    virtual MovementStateType GetType() const = 0;
    virtual void Enter(BotMovementController* controller) = 0;
    virtual void Exit(BotMovementController* controller) = 0;
    virtual void Update(BotMovementController* controller, uint32 diff) = 0;
    virtual bool CanTransitionTo(MovementStateType newState) const = 0;
};

// MovementValidator.h
struct ValidationResult
{
    bool isValid;
    Position adjustedPosition;  // Nearest valid position if invalid
    std::string reason;         // Why validation failed
    ValidationFlags flags;      // What was checked
};

class MovementValidator
{
public:
    static ValidationResult ValidateDestination(Unit* bot, const Position& dest);
    static bool CheckCollision(Unit* bot, const Position& start, const Position& end);
    static float GetValidGroundZ(uint32 mapId, float x, float y, float z);
    static WaterData GetWaterInfo(uint32 mapId, float x, float y, float z);
    static bool IsInVoid(uint32 mapId, const Position& pos);
};

// BotPathfinder.h
struct PathOptions
{
    bool allowSwimming = true;
    bool allowFlying = false;
    bool useStraightPath = false;
    float maxPathLength = 500.0f;
    uint32 maxPathPoints = 100;
};

class BotPathfinder
{
public:
    static PathResult GeneratePath(Unit* bot, const Position& dest, PathOptions options = {});
    static Position GetNearestValidPosition(Unit* bot, const Position& target);
    static bool IsDirectPathClear(Unit* bot, const Position& start, const Position& end);
};
```

### 5.3 Configuration Options

```cpp
// MovementConfig.h
struct MovementConfig
{
    // Validation
    float maxValidGroundDistance = 5.0f;      // Max distance to snap to ground
    float voidDetectionThreshold = -500.0f;   // Z below this = void
    
    // Stuck Detection
    uint32 stuckCheckInterval = 1000;         // ms between checks
    float stuckDistanceThreshold = 1.0f;      // Min movement in interval
    uint32 stuckTimeBeforeRecovery = 5000;    // ms before recovery action
    
    // Path Generation
    uint32 pathRecalcInterval = 2000;         // ms between path recalcs
    float pathPointSpacing = 3.0f;            // Distance between path points
    bool usePathSmoothing = true;
    
    // Swimming
    float swimSurfaceOffset = -1.0f;          // How deep below surface to swim
    float diveDepthMax = 50.0f;               // Max underwater depth
    
    // Following
    float defaultFollowDistance = 3.0f;
    float followAngleSpread = 30.0f;          // Degrees for formation
    float followUpdateThreshold = 2.0f;       // Min master move to update
    
    // Performance
    uint32 maxBotsUpdatedPerTick = 100;       // Stagger updates
    bool enablePathCaching = true;
    uint32 pathCacheLifetime = 5000;          // ms
};
```

---

## Deliverables

### Analysis Documents (in .claude/analysis/)

1. **MOVEMENT_CURRENT_SYSTEM_AUDIT.md**
   - Complete file inventory
   - Component relationships
   - Integration points with TrinityCore
   - Code quality assessment

2. **MOVEMENT_PROBLEM_ROOT_CAUSES.md**
   - Wall walking: exact cause and code locations
   - Void walking: exact cause and code locations
   - Water hopping: exact cause and code locations
   - Air hopping: exact cause and code locations
   - Stuck issues: exact cause and code locations

3. **MOVEMENT_TRINITYCORE_REFERENCE.md**
   - How TrinityCore movement works
   - Key classes and methods
   - Movement flags documentation
   - Packet structures

4. **MOVEMENT_ENTERPRISE_ARCHITECTURE.md**
   - Complete new system design
   - State machine specification
   - Interface definitions
   - Configuration options

5. **MOVEMENT_IMPLEMENTATION_PLAN.md**
   - Phase-by-phase implementation
   - File list with descriptions
   - Effort estimates
   - Risk assessment
   - Testing strategy

---

## Success Criteria

### Analysis Phase
- [ ] Every movement-related file in Playerbot documented
- [ ] Root cause identified for EACH problem (walls, void, water, air, stuck)
- [ ] TrinityCore movement system fully understood
- [ ] Enterprise architecture designed and documented

### Quality Requirements
- [ ] Architecture supports ALL movement types (ground, swim, fly, fall)
- [ ] Every position is validated before movement
- [ ] Stuck detection and recovery is robust
- [ ] System uses TrinityCore PathGenerator and MMaps properly
- [ ] Code is maintainable and well-documented
- [ ] Performance is acceptable (1000+ bots)

---

## Notes

- **DO NOT create placeholder implementations** - every function must be complete
- **Use TrinityCore's systems** - don't reinvent PathGenerator or MotionMaster
- **Validate EVERYTHING** - no movement without validation
- **State machine is critical** - proper transitions between ground/swim/fly/fall
- **This is a COMPLETE REWRITE** - old system can be entirely replaced
