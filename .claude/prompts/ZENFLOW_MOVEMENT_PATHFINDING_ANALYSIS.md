# Zenflow Task: Movement & Pathfinding System Analysis

**Priority**: P0 (Critical - Bots walking through walls/into void)  
**Estimated Duration**: 4-6 hours  
**Prerequisites**: None (standalone analysis)

---

## Context

Users report critical movement issues:
1. **Bots walk through walls** - Ignoring collision/geometry
2. **Bots walk into the void/off cliffs** - No ground detection
3. **Bots get stuck** - Unable to navigate around obstacles
4. **Bots take weird paths** - Inefficient or broken pathfinding

This analysis will identify root causes and create a fix plan.

---

## Phase 1: Movement System Architecture (1.5h)

### 1.1 Core Movement Components

Find and analyze these files:
```
src/modules/Playerbot/AI/
├── Movement/
│   ├── MovementManager.h/.cpp
│   ├── MovementGenerator.h/.cpp
│   ├── PathFinder.h/.cpp
│   ├── FollowMovement.h/.cpp
│   ├── ChaseMovement.h/.cpp
│   └── FormationMovement.h/.cpp
```

**Questions to answer:**
- How does the bot decide where to move?
- What triggers movement updates?
- How often is the path recalculated?
- Is there a movement state machine?

### 1.2 TrinityCore Movement Integration

Find how bots integrate with TrinityCore's movement:
```
src/server/game/Movement/
├── MotionMaster.h/.cpp
├── MovementGenerator.h
├── PathGenerator.h/.cpp
├── MMapManager.h/.cpp
└── MMapFactory.h/.cpp
```

**Questions to answer:**
- Do bots use TrinityCore's PathGenerator?
- Do bots use MMaps (navigation meshes)?
- Are bots using the same movement system as NPCs?
- Where does bot movement diverge from NPC movement?

### 1.3 Movement Commands

Find how movement is executed:
```
Search for:
- GetMotionMaster()
- MovePoint
- MoveFollow
- MoveChase
- MovePath
- SetWalk
- SetRun
```

**Questions to answer:**
- How are movement commands issued?
- Is there validation before moving?
- Are positions checked for validity?

---

## Phase 2: Pathfinding Analysis (1.5h)

### 2.1 Path Generation

Find pathfinding implementation:
```
Search for:
- PathGenerator
- CalculatePath
- GeneratePath
- FindPath
- dtNavMesh
- MMap
```

**Questions to answer:**
- Does bot pathfinding use MMaps?
- How are paths calculated?
- What happens when no path is found?
- Is there fallback behavior?

### 2.2 Navigation Mesh Usage

Check MMap integration:
```
Search for:
- MMAP
- navmesh
- dtNavMeshQuery
- findStraightPath
- findPath
```

**Questions to answer:**
- Are MMaps loaded for bot movement?
- Is the navmesh query correct?
- Are poly refs validated?
- What's the path smoothing logic?

### 2.3 Line of Sight & Collision

Find collision detection:
```
Search for:
- IsWithinLOS
- IsWithinLOSInMap
- GetHeight
- GetGroundZ
- CheckCollision
- CanSee
- HasInLine
```

**Questions to answer:**
- Is LOS checked before movement?
- Is ground height validated?
- What happens when destination is invalid?

---

## Phase 3: Common Failure Scenarios (1h)

### 3.1 Walking Through Walls

Investigate wall collision:
```
Search for:
- collision
- obstacle
- blocked
- CanReach
- IsReachable
```

**Questions to answer:**
- Is there wall collision detection?
- Does pathfinding respect geometry?
- Are shortcuts being taken that ignore walls?

### 3.2 Walking Into Void/Off Cliffs

Investigate ground validation:
```
Search for:
- GetHeight
- GetGroundZ
- INVALID_HEIGHT
- MAX_HEIGHT
- MIN_HEIGHT
- ValidateDestination
```

**Questions to answer:**
- Is destination height validated?
- What happens with invalid ground?
- Is there fall detection?

### 3.3 Getting Stuck

Investigate stuck detection:
```
Search for:
- IsStuck
- StuckDetection
- ClearStuck
- ResetMovement
- timeout
```

**Questions to answer:**
- Is there stuck detection?
- How is "stuck" defined?
- What's the recovery mechanism?

### 3.4 Follow/Formation Issues

Investigate group movement:
```
Search for:
- FollowTarget
- Formation
- GroupMovement
- MasterDistance
- FollowDistance
```

**Questions to answer:**
- How do bots follow the player?
- Is formation movement implemented?
- Are followers validated for position?

---

## Phase 4: TrinityCore Comparison (30min)

### 4.1 Compare with NPC Movement

Check how NPCs move vs bots:
```
src/server/game/AI/
├── CreatureAI.cpp
├── PetAI.cpp
└── SmartAI.cpp
```

**Questions to answer:**
- Do NPCs have the same issues?
- What's different in bot movement?
- Can bot movement use NPC patterns?

### 4.2 Check Existing Solutions

Look for movement fixes:
```
Search git history:
- git log --oneline --all -- "*Movement*" "*PathFind*"
- Look for recent movement-related commits
```

---

## Phase 5: Create Fix Recommendations

### 5.1 Document Findings

Create summary of:
1. **Root causes** - Why bots walk through walls/void
2. **Code locations** - Exact files and line numbers
3. **Dependencies** - MMap, PathGenerator, MotionMaster

### 5.2 Prioritize Fixes

Rank issues by:
- **Impact**: Walking through walls > Getting stuck
- **Severity**: Void/death > Inefficient paths
- **Effort**: Quick fixes vs. architectural changes

### 5.3 Create Implementation Plan

For each fix:
- Describe the change
- List files to modify
- Estimate effort (hours)
- Note any risks

---

## Deliverables

1. **MOVEMENT_ARCHITECTURE.md** - System overview and component map
2. **PATHFINDING_ANALYSIS.md** - Pathfinding deep dive
3. **COLLISION_ISSUES.md** - Wall/void walking analysis
4. **FIX_RECOMMENDATIONS.md** - Prioritized fix list with implementation plan

---

## Search Commands

```bash
# Find movement-related files
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "Movement\|PathFind\|MMap" | head -50

# Find bot movement specifically
grep -rn "GetMotionMaster\|MovePoint\|MoveFollow" src/modules/Playerbot --include="*.cpp" --include="*.h"

# Find pathfinding
grep -rn "PathGenerator\|CalculatePath\|GeneratePath" --include="*.cpp" --include="*.h"

# Find collision/LOS
grep -rn "IsWithinLOS\|GetHeight\|CheckCollision" --include="*.cpp" --include="*.h"

# Find MMap usage
grep -rn "MMAP\|dtNavMesh\|navmesh" --include="*.cpp" --include="*.h"

# Find stuck detection
grep -rn "IsStuck\|stuck\|Stuck" src/modules/Playerbot --include="*.cpp" --include="*.h"
```

---

## Key Files to Examine

Priority order:
1. `src/modules/Playerbot/AI/Movement/` - All files
2. `src/modules/Playerbot/AI/PlayerbotAI.cpp` - Main AI loop, movement calls
3. `src/modules/Playerbot/AI/Strategy/` - Movement strategies
4. `src/server/game/Movement/PathGenerator.cpp` - Core pathfinding
5. `src/server/game/Movement/MMapManager.cpp` - Navigation mesh

---

## Success Criteria

- [ ] Movement architecture fully documented
- [ ] Root cause of "walking through walls" identified
- [ ] Root cause of "walking into void" identified
- [ ] PathGenerator/MMap integration analyzed
- [ ] Collision detection gaps identified
- [ ] Fix recommendations created with effort estimates
- [ ] All findings documented in markdown files

---

## Notes

- This is a CRITICAL issue - bots dying to void/geometry is unacceptable
- Focus on WHY collision is ignored, not just WHERE
- Check if MMaps are being used at all for bot movement
- Compare with how TrinityCore NPCs handle the same scenarios
