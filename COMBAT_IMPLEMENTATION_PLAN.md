# TrinityCore PlayerBot - Combat Implementation Plan
## Enterprise-Grade Combat Behavior System

**Date**: 2025-10-07
**Status**: 🚀 **READY TO IMPLEMENT**
**Target**: Complete, production-ready combat AI for 5000+ concurrent bots

---

## Executive Summary

Based on comprehensive analysis by 3 specialized agents (wow-mechanics-expert, cpp-architecture-optimizer, wow-bot-behavior-designer), we have identified what EXISTS in the combat system and what's MISSING. This plan implements the missing components to achieve enterprise-grade combat behavior.

### What We Found

**✅ EXISTING (Already Working)**:
- 119 ClassAI implementations with complete rotations
- Polling-based combat updates (100ms, <0.1ms decision time)
- Target selection system with role awareness
- Threat management with multi-target tracking
- Basic interrupt framework
- Combat movement strategy with positioning
- Resource management per class
- Cooldown tracking system

**❌ MISSING (Need Implementation)**:
- Defensive cooldown automation (no centralized manager)
- Dispel/Purge priority system (no coordination)
- AoE decision making (no intelligent switching)
- Advanced positioning (no mechanic prediction)
- Group coordination utilities (no synchronized actions)

### Key Architectural Decision

**COMBAT STAYS POLLING-BASED** - We do NOT add Manager Event Handlers to combat.

**Why?**:
- Combat needs <1ms reaction time; events add 100-300ms latency
- ClassAI inherits from BotAI (core AI), not BehaviorManager
- Combat state changes constantly (would fire hundreds of events/second)
- Current polling architecture works perfectly

**What We're Building**: Utility managers that ClassAI can call directly during OnCombatUpdate().

---

## Implementation Phases

### PHASE 1: Core Combat Utilities (Week 1 - Priority P0)

**Goal**: Implement missing coordination utilities that all ClassAI implementations can use.

#### 1.1 DefensiveBehaviorManager
**File**: `src/modules/Playerbot/AI/CombatBehaviors/DefensiveBehaviorManager.h/cpp`
**Status**: Header created (400 lines), needs .cpp implementation
**Lines**: ~800 lines implementation

**Features**:
- Health threshold tracking (role-specific)
- Incoming DPS calculation (3-second rolling window)
- Defensive priority evaluation (5 levels)
- Cooldown tier system (Immunity → Regeneration)
- External defensive coordination (Pain Suppression, etc.)
- Consumables management (potions, healthstones)

**Performance**: <0.02ms per update

**Integration**: Called from ClassAI::OnCombatUpdate()
```cpp
void WarriorAI::OnCombatUpdate(uint32 diff) {
    if (_defensiveManager->NeedsDefensive()) {
        if (uint32 spell = _defensiveManager->SelectDefensive()) {
            CastSpell(spell);
            return;
        }
    }
    // ... continue rotation
}
```

#### 1.2 InterruptRotationManager
**File**: `src/modules/Playerbot/AI/CombatBehaviors/InterruptRotationManager.h/cpp`
**Status**: Header created (350 lines), needs .cpp implementation
**Lines**: ~700 lines implementation

**Features**:
- Global interrupt spell database (priority 1-5)
- Rotation queue for fairness
- Delayed scheduling (wait for CD)
- Fallback strategies (Stun → Silence → LOS → Defensive)
- Group coordination (one interrupt per cast)

**Performance**: <0.01ms per cast evaluation

**Integration**: Event-driven from spell cast detection
```cpp
void OnEnemySpellCast(Unit* caster, uint32 spellId) {
    ObjectGuid interrupter = _interruptManager->SelectInterrupter(caster, spellId);
    if (interrupter == GetBotGuid()) {
        ExecuteInterrupt(caster);
    }
}
```

#### 1.3 DispelCoordinator
**File**: `src/modules/Playerbot/AI/CombatBehaviors/DispelCoordinator.h/cpp`
**Status**: Design complete, needs full implementation
**Lines**: ~600 lines total

**Features**:
- Debuff priority matrix (Death → Trivial)
- Dynamic priority adjustment (role, health, spread mechanics)
- Dispeller assignment (best available by mana, range, CD)
- Purge system (immunity > enrage > major buffs)

**Performance**: <0.012ms per check

---

### PHASE 2: Advanced Combat Logic (Week 2 - Priority P0)

#### 2.1 AoEDecisionManager
**File**: `src/modules/Playerbot/AI/CombatBehaviors/AoEDecisionManager.h/cpp`
**Status**: New component, full implementation needed
**Lines**: ~500 lines total

**Features**:
- Enemy clustering detection (spatial partitioning)
- AoE breakpoint calculation (2/3/5/8+ targets)
- Resource efficiency scoring (AoE vs single-target DPS)
- Cleave positioning optimization
- DoT spread prioritization

**Performance**: <0.015ms per evaluation

**Integration**:
```cpp
bool WarriorAI::ShouldUseWhirlwind() {
    return _aoeManager->GetOptimalStrategy() == AoEStrategy::FULL_AOE &&
           _aoeManager->GetTargetCount() >= 3;
}
```

#### 2.2 CooldownStackingOptimizer
**File**: `src/modules/Playerbot/AI/CombatBehaviors/CooldownStackingOptimizer.h/cpp`
**Status**: Design complete, needs implementation
**Lines**: ~700 lines total

**Features**:
- Boss phase detection (Normal/Burn/Defensive/Add/Transition/Execute)
- Cooldown stacking windows (calculate multiplicative buffs)
- Phase-based reservation (save for burn phases)
- Bloodlust alignment
- Diminishing returns calculation

**Performance**: <0.018ms per evaluation

---

### PHASE 3: Combat Intelligence (Week 3 - Priority P1)

#### 3.1 CombatStateAnalyzer
**File**: `src/modules/Playerbot/AI/CombatBehaviors/CombatStateAnalyzer.h/cpp`
**Status**: Design complete, needs implementation
**Lines**: ~800 lines total

**Features**:
- Combat situation detection (10 types: Normal, AoE Heavy, Burst Needed, etc.)
- Combat metrics tracking (group DPS, incoming DPS, health, resources)
- Historical analysis (last 10 updates)
- Emergency detection (tank dead, healer dead, wipe imminent)

**Performance**: <0.030ms per analysis

#### 3.2 AdaptiveBehaviorManager
**File**: `src/modules/Playerbot/AI/CombatBehaviors/AdaptiveBehaviorManager.h/cpp`
**Status**: Design complete, needs implementation
**Lines**: ~600 lines total

**Features**:
- Behavior profile system (conditions + actions)
- Automatic strategy switching
- Group composition adaptation (no tank/healer handling)
- Dynamic role assignment

**Performance**: <0.015ms per update

---

### PHASE 4: Integration & Testing (Week 4)

#### 4.1 Integration Layer
**File**: `src/modules/Playerbot/AI/CombatBehaviors/CombatBehaviorIntegration.h/cpp`
**Lines**: ~400 lines total

**Purpose**: Provide easy integration point for ClassAI implementations.

```cpp
class EnhancedCombatAI : public ClassAI {
protected:
    std::unique_ptr<CombatBehaviorIntegration> _combatBehaviors;

public:
    void OnCombatUpdate(uint32 diff) override {
        // Update all behaviors
        _combatBehaviors->Update(diff);

        // Check for high-priority actions
        if (_combatBehaviors->HandleEmergencies())
            return;

        // Standard rotation
        UpdateRotation(GetTargetUnit());
    }
};
```

#### 4.2 CMakeLists.txt Updates
**File**: `src/modules/Playerbot/CMakeLists.txt`

Add new combat behavior files:
```cmake
# Combat Behaviors (Phase 4: Complete Combat System)
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/DefensiveBehaviorManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/DefensiveBehaviorManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/InterruptRotationManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/InterruptRotationManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/DispelCoordinator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/DispelCoordinator.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/AoEDecisionManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/AoEDecisionManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/CooldownStackingOptimizer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/CooldownStackingOptimizer.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/CombatStateAnalyzer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/CombatStateAnalyzer.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/AdaptiveBehaviorManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/AdaptiveBehaviorManager.h
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/CombatBehaviorIntegration.cpp
${CMAKE_CURRENT_SOURCE_DIR}/AI/CombatBehaviors/CombatBehaviorIntegration.h
```

#### 4.3 Testing Plan
1. **Unit Tests** (per component):
   - Defensive priority calculation
   - Interrupt assignment fairness
   - Dispel coordination
   - AoE breakpoint detection
   - Cooldown stacking math

2. **Integration Tests**:
   - 5-man dungeon simulation
   - Boss encounter with mechanics
   - Group coordination scenarios
   - Edge cases (no tank, no healer)

3. **Performance Tests**:
   - 100 bots in combat simultaneously
   - CPU profiling per component
   - Memory leak detection
   - Scalability to 5000 bots

---

## File Structure

```
src/modules/Playerbot/AI/CombatBehaviors/
├── DefensiveBehaviorManager.h          (400 lines - EXISTS)
├── DefensiveBehaviorManager.cpp        (800 lines - TODO)
├── InterruptRotationManager.h          (350 lines - EXISTS)
├── InterruptRotationManager.cpp        (700 lines - TODO)
├── DispelCoordinator.h                 (350 lines - TODO)
├── DispelCoordinator.cpp               (600 lines - TODO)
├── AoEDecisionManager.h                (300 lines - TODO)
├── AoEDecisionManager.cpp              (500 lines - TODO)
├── CooldownStackingOptimizer.h         (350 lines - TODO)
├── CooldownStackingOptimizer.cpp       (700 lines - TODO)
├── CombatStateAnalyzer.h               (400 lines - TODO)
├── CombatStateAnalyzer.cpp             (800 lines - TODO)
├── AdaptiveBehaviorManager.h           (300 lines - TODO)
├── AdaptiveBehaviorManager.cpp         (600 lines - TODO)
├── CombatBehaviorIntegration.h         (200 lines - TODO)
├── CombatBehaviorIntegration.cpp       (400 lines - TODO)
└── COMBAT_BEHAVIOR_DESIGN.md          (1750 lines - EXISTS)
```

**Total New Code**: ~8,600 lines
**Existing Design Docs**: 3 comprehensive markdown files

---

## Performance Budget

**Per-Bot Combat Update** (100ms frequency):
```
Component                         Time      % of Budget
──────────────────────────────────────────────────────
DefensiveBehaviorManager         20μs      20%
InterruptRotationManager          8μs       8%
DispelCoordinator                12μs      12%
AoEDecisionManager               15μs      15%
CooldownStackingOptimizer        18μs      18%
CombatStateAnalyzer              30μs      30%
AdaptiveBehaviorManager          15μs      15%
──────────────────────────────────────────────────────
TOTAL                           118μs     118%
```

**Performance Optimization Needed**: 18% over budget (18μs)

**Optimization Strategies**:
1. Cache-friendly access patterns
2. Lazy evaluation for expensive checks
3. Batch processing for similar operations
4. Early exit conditions
5. Update frequency tiering (some checks every 200ms instead of 100ms)

**After Optimization**: Target 85μs total (<0.1ms)

---

## Memory Budget

**Per-Bot Memory Addition**:
```
Component                         Memory
────────────────────────────────────────
DefensiveState                   ~500 bytes
DamageHistory (30 entries)       ~1 KB
InterrupterTracking              ~800 bytes
DispelCoordination               ~1 KB
AoEDecision                      ~600 bytes
CooldownOptimizer                ~1.5 KB
CombatMetrics (10 history)       ~2 KB
AdaptiveBehavior                 ~800 bytes
────────────────────────────────────────
TOTAL                            ~8.2 KB
```

**Acceptable**: Well within 10MB per-bot budget

---

## Claude.md Compliance Checklist

**✅ File Modification Hierarchy**:
- [ ] Module-only implementation (preferred)
- [ ] All files in `src/modules/Playerbot/`
- [ ] No core file modifications
- [ ] Zero TrinityCore refactoring

**✅ No Shortcuts**:
- [ ] Complete implementations (no TODOs)
- [ ] Full error handling
- [ ] Comprehensive edge case coverage
- [ ] Production-ready code quality

**✅ Performance Requirements**:
- [ ] <0.1% CPU per bot target
- [ ] <10MB memory per bot
- [ ] Thread-safe operations
- [ ] Tested at scale (5000 bots)

**✅ Quality Requirements**:
- [ ] TrinityCore API usage validated
- [ ] Coding standards compliance
- [ ] Comprehensive documentation
- [ ] Unit tests for all components

---

## Implementation Sequence

### Week 1 (P0 - Critical Combat Utilities)
**Day 1-2**: DefensiveBehaviorManager.cpp (800 lines)
**Day 3-4**: InterruptRotationManager.cpp (700 lines)
**Day 5**: DispelCoordinator.h/cpp (950 lines)

### Week 2 (P0 - Advanced Combat Logic)
**Day 1-2**: AoEDecisionManager.h/cpp (800 lines)
**Day 3-4**: CooldownStackingOptimizer.h/cpp (1050 lines)
**Day 5**: Testing and bugfixes

### Week 3 (P1 - Combat Intelligence)
**Day 1-2**: CombatStateAnalyzer.h/cpp (1200 lines)
**Day 3-4**: AdaptiveBehaviorManager.h/cpp (900 lines)
**Day 5**: Integration layer (600 lines)

### Week 4 (Integration & Testing)
**Day 1**: CMakeLists.txt updates, compilation
**Day 2**: Unit tests
**Day 3**: Integration tests
**Day 4**: Performance profiling and optimization
**Day 5**: Documentation and commit

---

## Success Metrics

**Technical**:
- ✅ Clean compilation with zero errors
- ✅ All unit tests passing
- ✅ <0.1ms per-bot decision time
- ✅ Linear scaling to 5000 bots
- ✅ Zero memory leaks

**Quality**:
- ✅ 80% of human player combat effectiveness
- ✅ <200ms reaction time to threats
- ✅ 90% interrupt success rate
- ✅ 70% reduction in unnecessary deaths
- ✅ Coordinated group actions

**Integration**:
- ✅ Zero ClassAI implementations broken
- ✅ Backward compatible with existing code
- ✅ Optional usage (bots work without new features)
- ✅ Clean separation of concerns

---

## Next Steps

**IMMEDIATE** (Now):
1. Create CombatBehaviors directory
2. Implement DefensiveBehaviorManager.cpp
3. Implement InterruptRotationManager.cpp
4. Test compilation
5. Commit Phase 1

**This plan is READY TO EXECUTE**. All analysis complete, designs finalized, headers created. Time to implement!

---

## Related Documentation

- `WOW_112_BOT_COMBAT_REQUIREMENTS.md` - WoW 11.2 combat requirements (579 lines)
- `COMBAT_ARCHITECTURE_ANALYSIS.md` - Architecture analysis (407 lines)
- `COMBAT_BEHAVIOR_DESIGN.md` - Complete behavior designs (1760 lines)
- `DefensiveBehaviorManager.h` - Defensive system header (398 lines)
- `InterruptRotationManager.h` - Interrupt system header (422 lines)

**Total Documentation**: 3,566 lines of comprehensive design and analysis

---

**Status**: 📋 **PLAN COMPLETE - READY TO IMPLEMENT**
**Next Action**: Create DefensiveBehaviorManager.cpp
**Estimated Completion**: 4 weeks for full combat behavior system
