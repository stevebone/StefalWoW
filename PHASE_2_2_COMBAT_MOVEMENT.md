# Phase 2.2: Create CombatMovementStrategy

**Duration**: 1 week (2025-01-20 to 2025-01-27)
**Status**: ⏳ PENDING
**Owner**: Development Team

---

## Objectives

Create a comprehensive combat movement system that positions bots based on their role during combat:
1. Role detection (Tank, Healer, Melee DPS, Ranged DPS)
2. Dynamic position calculation per role
3. Mechanic avoidance (fire, poison, void zones)
4. Integration with existing movement systems

---

## Background

### Current Problem
- Bots use only LeaderFollowBehavior for movement (social following)
- No role-based combat positioning
- Melee bots stand at range, ranged bots run into melee
- No mechanic avoidance during combat
- Combat effectiveness severely reduced

### Solution
- Dedicated CombatMovementStrategy for combat-specific positioning
- Separate from LeaderFollowBehavior (which handles out-of-combat following)
- Role-based positioning logic
- Higher priority than social movement during combat
- Lightweight (<0.5ms per update)

---

## Technical Requirements

### Performance Constraints
- Position calculation: <0.3ms per bot per frame
- Path validation: <0.2ms per bot per frame
- Total combat movement overhead: <0.5ms per bot
- Must work efficiently with 5000+ bots

### Role Definitions

**Tank**:
- Position: Melee range (5 yards)
- Facing: Front of target
- Responsibility: Hold aggro, face boss away from group

**Melee DPS**:
- Position: Melee range (5 yards)
- Facing: Behind target when possible
- Responsibility: Maximize DPS, avoid frontal cleaves

**Ranged DPS**:
- Position: 20-30 yards from target
- Facing: Any
- Responsibility: Maximize DPS, maintain safe distance

**Healer**:
- Position: 15-20 yards (casting range)
- Facing: Group center
- Responsibility: Stay in range of all group members

### Integration Points
- Must coexist with LeaderFollowBehavior (different priorities)
- Must use MotionMaster API for movement
- Must respect existing path validation
- Must not conflict with other strategies

---

## Deliverables

### 1. CombatMovementStrategy.h
Location: `src/modules/Playerbot/AI/Strategy/CombatMovementStrategy.h`

**Content**:
```cpp
#pragma once

#include "Strategy.h"

namespace Playerbot
{

enum class FormationRole
{
    TANK,        // Front line, melee range, face boss
    MELEE_DPS,   // Behind boss, melee range
    RANGED_DPS,  // 20-30 yards, any position
    HEALER       // 15-20 yards, central position
};

/**
 * CombatMovementStrategy - Role-based combat positioning
 *
 * Handles all combat movement based on bot's role:
 * - Tanks maintain frontal position
 * - Melee DPS attack from behind
 * - Ranged DPS maintain 20-30 yard distance
 * - Healers stay central for group coverage
 *
 * This strategy has higher priority than LeaderFollowBehavior
 * and only activates during combat.
 */
class TC_GAME_API CombatMovementStrategy : public Strategy
{
public:
    CombatMovementStrategy();
    ~CombatMovementStrategy() override = default;

    // Strategy interface
    void InitializeActions() override;
    void InitializeTriggers() override;
    void InitializeValues() override;

    // Activation - only during combat
    void OnActivate(BotAI* ai) override;
    void OnDeactivate(BotAI* ai) override;
    bool IsActive(BotAI* ai) const override;

    // Main update loop
    void UpdateBehavior(BotAI* ai, uint32 diff) override;

private:
    // Role detection
    FormationRole DetermineRole(Player* bot) const;

    // Position calculations
    Position CalculateTankPosition(Player* bot, Unit* target) const;
    Position CalculateMeleePosition(Player* bot, Unit* target) const;
    Position CalculateRangedPosition(Player* bot, Unit* target) const;
    Position CalculateHealerPosition(Player* bot, Unit* target) const;

    // Movement execution
    void MoveToPosition(Player* bot, Position const& pos);
    bool IsInCorrectPosition(Player* bot, Position const& targetPos, float tolerance) const;

    // Mechanic avoidance
    bool IsStandingInDanger(Player* bot) const;
    Position FindSafePosition(Player* bot, Unit* target) const;

    // State
    FormationRole _currentRole = FormationRole::MELEE_DPS;
    uint32 _lastPositionUpdate = 0;
    uint32 _positionUpdateInterval = 100; // Update every 100ms
};

} // namespace Playerbot
```

### 2. CombatMovementStrategy.cpp
Location: `src/modules/Playerbot/AI/Strategy/CombatMovementStrategy.cpp`

**Implementation Requirements**:
- Complete role detection based on class/spec
- Full position calculation algorithms
- TrinityCore MotionMaster API integration
- Mechanic detection (AreaTriggers, DynamicObjects)
- Safe position pathfinding

### 3. Integration with BotAI
Location: `src/modules/Playerbot/AI/BotAI.cpp`

**Changes Required**:
```cpp
void BotAI::InitializeStrategies()
{
    // Combat movement (higher priority than follow)
    _combatMovementStrategy = std::make_unique<CombatMovementStrategy>();
    _combatMovementStrategy->SetPriority(80); // Higher than follow (60)

    // Social follow (lower priority)
    _leaderFollowBehavior = std::make_unique<LeaderFollowBehavior>();
    _leaderFollowBehavior->SetPriority(60);

    // Idle behavior (lowest priority)
    _idleStrategy = std::make_unique<IdleStrategy>();
    _idleStrategy->SetPriority(50);
}

void BotAI::UpdateStrategies(uint32 diff)
{
    // Priority sorting ensures combat movement overrides follow during combat
    SortStrategiesByPriority();

    for (auto& strategy : _activeStrategies)
    {
        if (strategy->IsActive(this))
            strategy->UpdateBehavior(this, diff);
    }
}
```

### 4. Unit Tests
Location: `tests/unit/AI/Strategy/CombatMovementStrategyTest.cpp`

**Test Coverage**:
- Role detection for all 13 classes
- Position calculations for each role
- Priority override during combat
- Deactivation when leaving combat
- Mechanic avoidance detection
- Safe position calculation

### 5. Integration Tests
Location: `tests/integration/CombatMovementIntegrationTest.cpp`

**Test Scenarios**:
- Solo bot attacking training dummy (correct positioning)
- Group of 5 bots in dungeon (all roles position correctly)
- Tank holds boss facing away from group
- Melee DPS attack from behind
- Ranged DPS maintain 25 yard distance
- Healer stays within healing range of all members
- Bots move out of fire/poison
- Transition from follow → combat → follow

### 6. Documentation
Location: `docs/COMBAT_MOVEMENT_GUIDE.md`

**Content**:
- Role detection algorithm explanation
- Position calculation formulas
- Priority system explanation
- Mechanic avoidance implementation
- Troubleshooting guide
- Performance tuning tips

---

## Implementation Steps

### Step 1: Create Strategy Framework (2 days)
1. Write CombatMovementStrategy.h with full documentation
2. Create skeleton .cpp with method stubs
3. Add to CMakeLists.txt
4. Verify compilation

### Step 2: Implement Role Detection (1 day)
```cpp
FormationRole CombatMovementStrategy::DetermineRole(Player* bot) const
{
    uint32 spec = bot->GetPrimaryTalentTree(bot->GetActiveSpec());
    uint8 playerClass = bot->getClass();

    // Tank specs
    if ((playerClass == CLASS_WARRIOR && spec == TALENT_TREE_WARRIOR_PROTECTION) ||
        (playerClass == CLASS_PALADIN && spec == TALENT_TREE_PALADIN_PROTECTION) ||
        (playerClass == CLASS_DEATH_KNIGHT && spec == TALENT_TREE_DEATH_KNIGHT_BLOOD) ||
        (playerClass == CLASS_DRUID && spec == TALENT_TREE_DRUID_FERAL_COMBAT && bot->HasAura(9634))) // Bear Form
    {
        return FormationRole::TANK;
    }

    // Healer specs
    if ((playerClass == CLASS_PRIEST && (spec == TALENT_TREE_PRIEST_DISCIPLINE || spec == TALENT_TREE_PRIEST_HOLY)) ||
        (playerClass == CLASS_PALADIN && spec == TALENT_TREE_PALADIN_HOLY) ||
        (playerClass == CLASS_SHAMAN && spec == TALENT_TREE_SHAMAN_RESTORATION) ||
        (playerClass == CLASS_DRUID && spec == TALENT_TREE_DRUID_RESTORATION))
    {
        return FormationRole::HEALER;
    }

    // Ranged DPS specs
    if (playerClass == CLASS_HUNTER || playerClass == CLASS_MAGE || playerClass == CLASS_WARLOCK ||
        (playerClass == CLASS_PRIEST && spec == TALENT_TREE_PRIEST_SHADOW) ||
        (playerClass == CLASS_SHAMAN && spec == TALENT_TREE_SHAMAN_ELEMENTAL) ||
        (playerClass == CLASS_DRUID && spec == TALENT_TREE_DRUID_BALANCE))
    {
        return FormationRole::RANGED_DPS;
    }

    // Melee DPS (default for remaining classes)
    return FormationRole::MELEE_DPS;
}
```

### Step 3: Implement Position Calculations (2 days)
1. Tank position - 5 yards in front
2. Melee DPS position - 5 yards behind (180° from front)
3. Ranged DPS position - 25 yards, optimal angle
4. Healer position - 18 yards, central to group
5. Path validation using TrinityCore APIs
6. Test each role independently

### Step 4: Implement Movement Execution (1 day)
```cpp
void CombatMovementStrategy::MoveToPosition(Player* bot, Position const& pos)
{
    if (!bot || !bot->IsInWorld())
        return;

    // Check if already in position (avoid unnecessary movement)
    if (IsInCorrectPosition(bot, pos, 2.0f))
        return;

    // Use MotionMaster to move
    bot->GetMotionMaster()->MovePoint(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
}

bool CombatMovementStrategy::IsInCorrectPosition(Player* bot, Position const& targetPos, float tolerance) const
{
    if (!bot)
        return false;

    float dist = bot->GetDistance2d(targetPos.GetPositionX(), targetPos.GetPositionY());
    return dist <= tolerance;
}
```

### Step 5: Implement Mechanic Avoidance (1 day)
```cpp
bool CombatMovementStrategy::IsStandingInDanger(Player* bot) const
{
    if (!bot)
        return false;

    // Check for dangerous AreaTriggers (fire, poison, etc.)
    std::list<AreaTrigger*> triggers;
    bot->GetAreaTriggersWithEntryInRange(triggers, 0, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), 5.0f);

    for (AreaTrigger* trigger : triggers)
    {
        if (trigger->IsUnitAffected(bot) && trigger->GetTemplate()->HasFlag(AREATRIGGER_FLAG_HOSTILE))
            return true;
    }

    // Check for DynamicObjects (AoE effects)
    std::list<DynamicObject*> dynObjects;
    bot->GetDynamicObjectsInRange(dynObjects, 5.0f);

    for (DynamicObject* obj : dynObjects)
    {
        if (obj->GetCasterGUID() != bot->GetGUID() && obj->IsHostileTo(bot))
            return true;
    }

    return false;
}
```

### Step 6: Write Tests (1 day)
1. Unit tests for role detection (all classes)
2. Unit tests for position calculations
3. Integration test with dummy target
4. Integration test with group composition

### Step 7: Integration with BotAI (1 day)
1. Add CombatMovementStrategy to BotAI
2. Set priority (80 - higher than follow)
3. Test priority system works
4. Test activation/deactivation during combat transitions

---

## Success Criteria

### Functional
- ✅ All 13 classes correctly detect their role
- ✅ Tanks position in front of target
- ✅ Melee DPS attack from behind
- ✅ Ranged DPS maintain 20-30 yard distance
- ✅ Healers stay within healing range of group
- ✅ Bots move out of dangerous ground effects
- ✅ Strategy activates on combat start
- ✅ Strategy deactivates on combat end
- ✅ Combat movement overrides follow behavior

### Performance
- ✅ Position calculation <0.3ms per bot
- ✅ Movement execution <0.2ms per bot
- ✅ Total overhead <0.5ms per bot per frame
- ✅ No performance degradation with 100+ bots in combat
- ✅ No memory leaks after 1000 combat cycles

### Code Quality
- ✅ Full documentation in header
- ✅ All public methods documented
- ✅ Follows TrinityCore coding conventions
- ✅ No compiler warnings
- ✅ 100% test coverage for role detection
- ✅ 90%+ test coverage for position calculations

---

## Dependencies

### Requires
- Phase 2.1 complete (BehaviorManager base class)
- Existing LeaderFollowBehavior (for priority comparison)
- TrinityCore MotionMaster API
- TrinityCore AreaTrigger/DynamicObject APIs

### Blocks
- Phase 2.6 (Integration testing needs combat movement)
- Full combat effectiveness testing

---

## Risk Mitigation

### Risk: Bots get stuck in terrain
**Mitigation**: Use TrinityCore's pathfinding validation before moving

### Risk: Movement conflicts with other systems
**Mitigation**: Priority-based strategy execution, combat movement has highest priority

### Risk: Performance degradation with many bots
**Mitigation**: Throttle position updates to 100ms intervals, profile with 500+ bots

### Risk: Role detection fails for hybrid classes
**Mitigation**: Comprehensive unit tests for all class/spec combinations

---

## Next Phase

After completion, proceed to **Phase 2.3: Fix Combat Activation (Universal ClassAI)**

---

**Last Updated**: 2025-01-13
**Next Review**: 2025-01-27
