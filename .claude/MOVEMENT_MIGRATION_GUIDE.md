# BotMovementController Migration Guide

## Overview

This guide documents the migration from direct `MotionMaster` calls to the new `BotMovementController` system for validated pathfinding.

## Status

**Task 3 Progress: PARTIAL (Core Files Complete)**

### ✅ Completed Files (3/33)
1. `src/modules/Playerbot/AI/Actions/CommonActions.cpp` - MoveToPosition & Follow actions
2. `src/modules/Playerbot/Quest/QuestCompletion.cpp` - Quest turn-in navigation
3. `src/modules/Playerbot/AI/BotAI.cpp` - BotAI integration (Task 1)

### ⏳ Remaining Files (30/33)
See section below for complete list and migration priority.

## Migration Pattern

### Before (Legacy MotionMaster):
```cpp
// Direct MotionMaster usage - NO VALIDATION
bot->GetMotionMaster()->MovePoint(0, x, y, z);
bot->GetMotionMaster()->MoveFollow(target, distance, angle);
bot->GetMotionMaster()->MoveChase(target);
```

### After (BotMovementController):
```cpp
// Pattern 1: From BotAI context (preferred)
if (BotAI* ai = GetBotAI(player))
{
    // Use validated pathfinding
    Position dest(x, y, z, 0.0f);
    if (!ai->MoveTo(dest, true))  // validated = true
    {
        // Fallback to legacy if validation fails
        player->GetMotionMaster()->MovePoint(0, dest);
    }
}
else
{
    // Non-bot player - use standard movement
    player->GetMotionMaster()->MovePoint(0, x, y, z);
}

// Pattern 2: Follow movement
if (BotAI* ai = GetBotAI(player))
{
    if (!ai->MoveToUnit(target, distance))
    {
        // Fallback to legacy
        float angle = GetFollowAngle();
        player->GetMotionMaster()->MoveFollow(target, distance, angle);
    }
}
```

### Key Principles:
1. **Always check for BotAI** - Use `GetBotAI(player)` helper
2. **Always provide fallback** - Legacy MotionMaster if validation fails
3. **Support non-bot players** - Check if BotAI exists before using it
4. **Enable validation by default** - Use `MoveTo(dest, true)`
5. **Keep logging** - Preserve existing debug/trace logs

## Helper Functions

### Getting BotAI from Player
```cpp
#include "Core/PlayerBotHelpers.h"

BotAI* GetBotAI(Player* player);
```

### Available BotAI Movement Methods
```cpp
// From BotAI.h:
bool MoveTo(Position const& dest, bool validated = true);
bool MoveToUnit(::Unit* target, float distance = 0.0f);
bool IsMovementBlocked() const;
bool IsStuck() const;
BotMovementController* GetMovementController();
```

## Remaining Files by Priority

### HIGH PRIORITY (Combat & Core Movement)
1. `src/modules/Playerbot/AI/Combat/RoleBasedCombatPositioning.cpp` (15 usages)
2. `src/modules/Playerbot/AI/Combat/FormationManager.cpp` (5 usages)
3. `src/modules/Playerbot/AI/Combat/KitingManager.cpp` (1 usage)
4. `src/modules/Playerbot/AI/Combat/InterruptManager.cpp` (3 usages)
5. `src/modules/Playerbot/AI/Combat/PositionManager.cpp` (1 usage)
6. `src/modules/Playerbot/AI/Combat/ObstacleAvoidanceManager.cpp` (2 usages)
7. `src/modules/Playerbot/AI/Combat/MechanicAwareness.cpp` (1 usage)
8. `src/modules/Playerbot/AI/Combat/MovementIntegration.cpp` (2 usages)

### MEDIUM PRIORITY (ClassAI & Spec-Specific)
9. `src/modules/Playerbot/AI/ClassAI/CombatSpecializationBase.cpp`
10. `src/modules/Playerbot/AI/ClassAI/Warlocks/WarlockAI.cpp`
11. `src/modules/Playerbot/AI/ClassAI/Priests/PriestAI.cpp`
12. `src/modules/Playerbot/AI/ClassAI/Hunters/HunterAI.cpp`
13. `src/modules/Playerbot/AI/ClassAI/Hunters/BeastMasteryHunter.h`

### MEDIUM PRIORITY (Actions & Behavior)
14. `src/modules/Playerbot/AI/Actions/Action.cpp`
15. `src/modules/Playerbot/AI/Actions/SpellInterruptAction.cpp`
16. `src/modules/Playerbot/Advanced/AdvancedBehaviorManager.cpp`
17. `src/modules/Playerbot/AI/EnhancedBotAI.cpp`

### LOW PRIORITY (Dungeon & Specialized)
18. `src/modules/Playerbot/Dungeon/DungeonAutonomyManager.cpp`
19. `src/modules/Playerbot/Dungeon/DungeonBehavior.cpp`
20. `src/modules/Playerbot/Dungeon/EncounterStrategy.cpp`
21. `src/modules/Playerbot/Dungeon/Scripts/Vanilla/StockadeScript.cpp`

### LOW PRIORITY (Quest & Travel)
22. `src/modules/Playerbot/Movement/QuestPathfinder.cpp`
23. `src/modules/Playerbot/Travel/TravelRouteManager.cpp`

### LOW PRIORITY (Other Systems)
24. `src/modules/Playerbot/Threading/BotActionProcessor.cpp`
25. `src/modules/Playerbot/Social/TradeSystem.cpp`
26. `src/modules/Playerbot/Interaction/Core/InteractionManager.cpp`
27. `src/modules/Playerbot/Companion/BattlePetManager.cpp`
28. `src/modules/Playerbot/AI/BehaviorTree/Nodes/MovementNodes.h`
29. `src/modules/Playerbot/Movement/BotMovementUtil.h`
30. `src/modules/Playerbot/Core/Services/BotNpcLocationService.h`

## Benefits of Migration

### Validation Features Enabled:
- ✅ **Ground Validation**: Prevents bots from walking into void/falling off cliffs
- ✅ **Collision Validation**: Prevents bots from walking through walls
- ✅ **Liquid Validation**: Proper swimming vs. walking detection
- ✅ **State Machine**: Automatic transitions (ground → swimming → falling)
- ✅ **Stuck Detection**: Automatic recovery when bot gets stuck

### Performance:
- No overhead when BotMovement system is disabled
- Minimal overhead when enabled (~5-10ms per path calculation)
- Same caching benefits as before (PathCache integration)

## Testing After Migration

### Test Cases for Each File:
1. **Water Test**: Bot should swim, not jump
2. **Wall Test**: Bot should stop or go around, not clip through
3. **Cliff Test**: Bot should stop at edge, not fall into void
4. **Stuck Test**: Bot should recover after 5 seconds
5. **Performance Test**: No significant frame time increase

### Logging:
```cpp
// Add debug logging for migration tracking
TC_LOG_DEBUG("module.playerbot.movement",
    "Using validated movement for bot {} (source: {})",
    bot->GetName(), __FUNCTION__);
```

## Configuration

The system respects `BotMovement.Enable` config:
```ini
# worldserver.conf
BotMovement.Enable = 1  # Enable validated pathfinding
BotMovement.Validation.Ground = 1
BotMovement.Validation.Collision = 1
BotMovement.Validation.Liquid = 1
```

## Future Work

### Phase 2 Migration (Remaining 30 Files):
- Batch update Combat files (high priority)
- Batch update ClassAI files (medium priority)
- Batch update Dungeon files (low priority)
- Comprehensive testing with 5000 bots

### Optimization Opportunities:
- Batch validation for formation movement
- Pre-validated common paths (quest hubs, dungeons)
- Dynamic validation level based on bot count

## Notes for Developers

1. **DO NOT** remove legacy MotionMaster fallback - it's a safety net
2. **DO NOT** use validated movement for emergency situations (use legacy)
3. **DO** test each file after migration
4. **DO** preserve existing movement behavior where possible
5. **DO** add comments explaining why validation is used/bypassed

## References

- **Task 1 (Complete)**: BotAI Integration
- **Task 2 (Complete)**: PathCache Migration
- **Task 3 (Partial)**: Movement Generator Replacement (3/33 files)
- **Related Prompt**: `.claude/prompts/MOVEMENT_INTEGRATION_PROMPT.md`

---

**Last Updated**: 2026-02-04
**Status**: IN PROGRESS (10% complete - 3/33 files)
**Next Action**: Batch migrate Combat files (high priority group)
