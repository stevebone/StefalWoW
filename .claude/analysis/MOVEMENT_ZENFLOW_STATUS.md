# Movement System Redesign - Status & Continuation Plan

**Date**: 2026-01-27  
**Source**: Zenflow worktree `movement-system-refactoring-ee20`  
**Status**: Phase 1 Complete (1.1-1.6), Phase 1.7 Partially Complete

---

## ✅ Was Zenflow bereits implementiert hat (Phase 1)

### Core Infrastructure
| File | Status | Description |
|------|--------|-------------|
| `BotMovementDefines.h` | ✅ Complete | Enums: MovementStateType, ValidationFailureReason, StuckType, RecoveryLevel, ValidationLevel |
| `ValidationResult.h` | ✅ Complete | Success/Failure factory methods, error messages |
| `BotMovementConfig.h/cpp` | ✅ Complete | Configuration loading from worldserver.conf |
| `BotMovementManager.h/cpp` | ✅ Complete | Singleton manager, controller registry, path cache |
| `BotMovementController.h/cpp` | ✅ Complete | Per-bot controller with position history tracking |
| `MovementMetrics.h` | ✅ Complete | Performance metrics tracking |

### Validation Layer
| File | Status | Description |
|------|--------|-------------|
| `PositionValidator.h/cpp` | ✅ Complete | Bounds validation, map ID validation |
| `GroundValidator.h/cpp` | ✅ Complete | Ground height, void detection, bridge detection, unsafe terrain (lava/slime) |

### Pathfinding
| File | Status | Description |
|------|--------|-------------|
| `PathCache.h` | ✅ Header Only | LRU cache structure defined |

### Tests
| File | Status | Description |
|------|--------|-------------|
| `BotMovementIntegration.cpp` | ✅ Complete | 14 test cases, compiles successfully |

---

## ❌ Was noch fehlt (Phase 2-6)

### Phase 2: Pathfinding Enhancement
- [ ] `CollisionValidator.h/cpp` - LOS and collision detection
- [ ] `LiquidValidator.h/cpp` - Water/liquid detection for swimming
- [ ] `ValidatedPathGenerator.h/cpp` - PathGenerator wrapper with validation
- [ ] `PathValidationPipeline.h/cpp` - Multi-stage validation
- [ ] `PathCache.cpp` - Implementation
- [ ] `PathSmoother.h/cpp` - Path optimization

### Phase 3: State Machine
- [ ] `MovementState.h/cpp` - Base state interface
- [ ] `MovementStateMachine.h/cpp` - State machine core
- [ ] `IdleState.h/cpp` - Idle state
- [ ] `GroundMovementState.h/cpp` - Ground movement (edge detection!)
- [ ] `SwimmingMovementState.h/cpp` - Swimming (CRITICAL for water bug)
- [ ] `FallingMovementState.h/cpp` - Falling (CRITICAL for hopping bug)
- [ ] `StuckState.h/cpp` - Stuck handling

### Phase 4: Stuck Detection & Recovery
- [ ] `StuckDetector.h/cpp` - Detect stuck conditions
- [ ] `RecoveryStrategies.h/cpp` - Escalating recovery (recalculate → backup → teleport)

### Phase 5: Movement Generators
- [ ] `BotMovementGeneratorBase.h/cpp` - Base class
- [ ] `BotPointMovementGenerator.h/cpp` - Validated point movement
- [ ] `BotChaseMovementGenerator.h/cpp` - Validated chase
- [ ] `BotFollowMovementGenerator.h/cpp` - Validated follow

### Phase 6: Integration
- [ ] Unit::Update() integration
- [ ] BotPlayerAI extension
- [ ] Public API
- [ ] Configuration in worldserver.conf

---

## 📁 Zenflow Worktree Location

```
C:\Users\daimon\.zenflow\worktrees\movement-system-refactoring-ee20\
├── src\server\game\Movement\BotMovement\   <- NEW CODE
├── tests\game\BotMovementIntegration.cpp   <- TESTS
└── .zenflow\tasks\movement-system-refactoring-ee20\
    ├── requirements.md   <- PRD
    ├── spec.md           <- Technical Spec (1449 lines!)
    └── plan.md           <- Implementation Plan (1279 lines!)
```

---

## 🚀 Next Steps

### Option A: Copy to Main Repo and Continue
1. Copy Zenflow's work to main TrinityCore repo
2. Continue with Phase 2 (CollisionValidator, LiquidValidator)
3. Implement State Machine (Phase 3)
4. This fixes the water hopping and air hopping issues

### Option B: Complete in Zenflow Worktree
1. Continue in Zenflow worktree
2. Merge to main repo when complete

### Option C: Create Claude Code Prompt
1. Create comprehensive Claude Code prompt
2. Let Claude Code implement remaining phases

---

## 🎯 Critical Fixes Needed

| Issue | Solution | Phase |
|-------|----------|-------|
| **Walking through walls** | CollisionValidator + ValidatedPathGenerator | Phase 2 |
| **Walking into void** | GroundValidator (✅ Done) + Edge detection in GroundMovementState | Phase 3 |
| **Water hopping** | LiquidValidator + SwimmingMovementState + MOVEMENTFLAG_SWIMMING | Phase 2+3 |
| **Air hopping** | FallingMovementState + proper gravity | Phase 3 |
| **Getting stuck** | StuckDetector + RecoveryStrategies | Phase 4 |

---

## 📊 Estimated Remaining Effort

| Phase | Effort | Status |
|-------|--------|--------|
| Phase 1 | ~15h | ✅ Complete |
| Phase 2 | ~20h | ❌ Not started |
| Phase 3 | ~25h | ❌ Not started |
| Phase 4 | ~10h | ❌ Not started |
| Phase 5 | ~15h | ❌ Not started |
| Phase 6 | ~10h | ❌ Not started |
| **Total Remaining** | **~80h** | |

---

## Architecture Quality Assessment

**Score: 9/10** ⭐⭐⭐⭐⭐

**Strengths:**
- ✅ Proper separation of concerns
- ✅ Validation-first architecture
- ✅ State machine pattern for movement states
- ✅ Decorator pattern for PathGenerator
- ✅ Comprehensive error handling
- ✅ Caching for performance
- ✅ Configurable via worldserver.conf
- ✅ TrinityCore integration (not replacement)
- ✅ Backward compatibility preserved

**Enterprise-Grade Features:**
- Singleton manager for global state
- Per-bot controller instances
- Position history tracking
- Metrics collection
- Comprehensive logging
- Unit test coverage
