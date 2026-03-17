# Phase 2: Architecture Cleanup - Claude Code Start Prompt

Copy this entire prompt into Claude Code to begin Phase 2 implementation:

---

## PROMPT START

Phase 1 is complete. Now implement Phase 2: Architecture Cleanup.

### Context
- **Repository**: `C:\TrinityBots\TrinityCore`
- **Detailed Plan**: Read `.claude/prompts/PHASE2_ARCHITECTURE_CLEANUP.md` for full specifications
- **Master Plan**: Read `.claude/plans/COMBAT_REFACTORING_MASTER_PLAN.md`

### Phase 2 Goal
Eliminate feature overlaps by establishing single sources of truth. This fixes the 3-way interrupt overlap, 2-way CC overlap, and 3-way target selection overlap identified in the analysis.

### Phase 2 Tasks (Execute in Order)

---

**Task 2.1: Consolidate Interrupt Coordination** (8h)

Make `InterruptCoordinator` the SINGLE authority for all interrupt logic.

**Files to modify**:
- `src/modules/Playerbot/Advanced/TacticalCoordinator.h`
- `src/modules/Playerbot/Advanced/TacticalCoordinator.cpp`
- `src/modules/Playerbot/AI/Coordination/RoleCoordinator.h`
- `src/modules/Playerbot/AI/Coordination/RoleCoordinator.cpp`

**Changes**:
1. Add `InterruptCoordinator* _interruptCoordinator` to TacticalCoordinator
2. Add `SetInterruptCoordinator()` setter method
3. Change all interrupt methods to DELEGATE to InterruptCoordinator:
```cpp
void TacticalCoordinator::AssignInterrupt(ObjectGuid caster, uint32 spellId) {
    if (_interruptCoordinator) {
        _interruptCoordinator->OnEnemyCastStart(caster, spellId);
    }
}
```
4. REMOVE any internal interrupt tracking (queues, maps, rotation logic)
5. Do the same for DPSCoordinator in RoleCoordinator

---

**Task 2.2: Consolidate CC Coordination** (6h)

Make `CrowdControlManager` the SINGLE authority for all CC logic.

**Files to modify**:
- `src/modules/Playerbot/Advanced/TacticalCoordinator.h`
- `src/modules/Playerbot/Advanced/TacticalCoordinator.cpp`

**Changes**:
1. Add `CrowdControlManager* _ccManager` to TacticalCoordinator
2. Add `SetCCManager()` setter method
3. Change all CC methods to DELEGATE:
```cpp
void TacticalCoordinator::AssignCrowdControl(ObjectGuid target) {
    if (_ccManager) {
        _ccManager->ApplyCC(target, _ccManager->GetRecommendedSpell(target));
    }
}

bool TacticalCoordinator::IsTargetCrowdControlled(ObjectGuid target) const {
    return _ccManager ? _ccManager->IsTargetCCd(target) : false;
}
```
4. REMOVE any internal CC tracking (_ccTargets, _ccAssignments, etc.)

---

**Task 2.3: Consolidate Target Selection** (6h)

Make `TacticalCoordinator` the SINGLE authority for group target.

**Files to modify**:
- `src/modules/Playerbot/Advanced/TacticalCoordinator.h`
- `src/modules/Playerbot/Advanced/TacticalCoordinator.cpp`
- `src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp`

**Changes**:
1. Ensure TacticalCoordinator has complete target API:
   - `GetGroupTarget()`, `SetGroupTarget()`, `ClearGroupTarget()`
   - `AddPriorityTarget()`, `GetHighestPriorityTarget()`

2. Modify GroupCombatStrategy to QUERY TacticalCoordinator:
```cpp
Unit* GroupCombatStrategy::GetCombatTarget(BotAI* ai) {
    if (TacticalCoordinator* tc = GetTacticalCoordinator(ai)) {
        if (Unit* target = tc->GetGroupTarget()) {
            return target;
        }
    }
    return FindFallbackTarget(ai);  // Only if no coordinator
}
```

3. Move/simplify the complex target logic from FindGroupCombatTarget() to a simple fallback

---

**Task 2.4: Add DR Tracking** (8h)

Add Diminishing Returns tracking to CrowdControlManager for PvP.

**Files to modify**:
- `src/modules/Playerbot/AI/Combat/CrowdControlManager.h`
- `src/modules/Playerbot/AI/Combat/CrowdControlManager.cpp`

**Add**:
```cpp
enum class DRCategory : uint8 {
    NONE = 0, STUN = 1, INCAPACITATE = 2, DISORIENT = 3,
    SILENCE = 4, FEAR = 5, ROOT = 6, HORROR = 7, DISARM = 8
};

struct DRState {
    uint8 stacks = 0;  // 0, 1, 2, 3 (immune at 3)
    uint32 lastApplicationTime = 0;
    static constexpr uint32 DR_RESET_TIME_MS = 18000;  // 18 seconds
    
    float GetDurationMultiplier() const;  // 1.0, 0.5, 0.25, 0.0
    bool IsImmune() const { return stacks >= 3; }
    void Apply(uint32 currentTime);
    void Update(uint32 currentTime);  // Reset if 18s passed
};

// In class:
std::map<ObjectGuid, std::map<DRCategory, DRState>> _drTracking;

// Methods:
float GetDRMultiplier(ObjectGuid target, DRCategory category) const;
bool IsImmune(ObjectGuid target, DRCategory category) const;
void OnCCApplied(ObjectGuid target, DRCategory category);
void UpdateDR(uint32 currentTime);
```

**Modify GetRecommendedSpell()** to skip immune categories.

---

**Task 2.5: Verify Architecture** (2h)

Verify all consolidations are complete:
- [ ] Only InterruptCoordinator has interrupt state
- [ ] Only CrowdControlManager has CC state  
- [ ] Only TacticalCoordinator owns group target
- [ ] No new circular dependencies
- [ ] Project compiles and basic combat works

---

### Instructions
1. Read the detailed plan first: `.claude/prompts/PHASE2_ARCHITECTURE_CLEANUP.md`
2. Execute tasks in order (2.1 → 2.2 → 2.3 → 2.4 → 2.5)
3. Verify compilation after each task
4. Commit after each working task
5. Report any issues or questions

### Success Criteria
- [ ] Only 1 component tracks interrupts (InterruptCoordinator)
- [ ] Only 1 component tracks CC (CrowdControlManager)
- [ ] Only 1 component owns group target (TacticalCoordinator)
- [ ] DR tracking implemented and working
- [ ] Zero new circular dependencies
- [ ] Project compiles without errors

Start with Task 2.1 (Consolidate Interrupt Coordination).

---

## PROMPT END
