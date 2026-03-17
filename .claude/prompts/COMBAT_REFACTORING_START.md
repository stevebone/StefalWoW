# Combat System Refactoring - Claude Code Start Prompt

Copy this entire prompt into Claude Code to begin implementation:

---

## PROMPT START

I need you to implement Phase 1 of the Combat System Refactoring for TrinityCore Playerbot.

### Context
- **Repository**: `C:\TrinityBots\TrinityCore`
- **Analysis**: Read `.zenflow/worktrees/analyze-all-existing-combat-coor-358d/.zenflow/tasks/analyze-all-existing-combat-coor-358d/ANALYSIS_REPORT.md`
- **Plan**: Read `.claude/plans/COMBAT_REFACTORING_MASTER_PLAN.md`
- **Detailed Prompt**: Read `.claude/prompts/COMBAT_REFACTORING_PROMPT.md`

### Critical Problems to Fix
1. **GroupCombatStrategy**: O(N²) = 175,000 ops → O(N) with member caching
2. **RoleCoordinator**: O(H×T) = 62,500 ops → O((H+T)log T) with priority queue
3. **Strategy Base**: 100k updates/sec → throttle to ~15k/sec
4. **ThreatCoordinator**: Fix 2 circular dependencies
5. **RaidOrchestrator/RoleCoordinator**: Fix layer violations with IGroupCoordinator interface

### Phase 1 Tasks (Execute in Order)

**Task 1.1**: Create `src/modules/Playerbot/Core/Combat/CombatContextDetector.h`
- Enum: SOLO, GROUP, DUNGEON, RAID, ARENA, BATTLEGROUND
- Static methods: Detect(), ToString(), RequiresCoordination(), GetRecommendedUpdateInterval()

**Task 1.2**: Add throttling to `Strategy.h/cpp`
- Add: `_behaviorUpdateInterval`, `_timeSinceLastBehaviorUpdate`
- Add: `MaybeUpdateBehavior()`, `NeedsEveryFrameUpdate()`
- Update call site in BotAI.cpp

**Task 1.3**: Fix `ThreatCoordinator.h` circular deps
- Remove: `#include "BotThreatManager.h"` and `#include "InterruptCoordinator.h"`
- Add forward declarations
- Move includes to .cpp file

**Task 1.4**: Create `Core/DI/Interfaces/IGroupCoordinator.h`
- Interface with: Update(), IsInGroup(), IsInRaid(), GetRole(), GetGroupTarget()
- Make GroupCoordinator implement it
- Change RaidOrchestrator/RoleCoordinator to use interface

**Task 1.5**: Fix `GroupCombatStrategy.h/cpp`
- Add: `std::vector<ObjectGuid> _memberCache`
- Add: `RefreshMemberCache()`, `OnGroupChanged()`
- Use cache in UpdateBehavior() - single iteration

**Task 1.6**: Fix `RoleCoordinator.cpp` UpdateHealingAssignments()
- Replace nested H×T loop with std::priority_queue
- O(H+T)log T instead of O(H×T)

**Task 1.7-1.9**: Optional quick wins
- Skip formations for solo bots
- Context-aware BotThreatManager MAX_ENTRIES
- Strategy CalculateRelevance()

### Instructions
1. Read the analysis documents first
2. Create/modify files one task at a time
3. Verify compilation after each task: `cmake --build build --target Playerbot`
4. Commit after each working task
5. Report progress and any issues

### Success Criteria
- [ ] Zero circular dependencies
- [ ] Zero layer violations  
- [ ] Project compiles without errors
- [ ] Strategy updates reduced ~90%
- [ ] GroupCombatStrategy ops reduced ~97%
- [ ] RoleCoordinator ops reduced ~84%

Start with Task 1.1 (CombatContextDetector) and proceed sequentially.

---

## PROMPT END
