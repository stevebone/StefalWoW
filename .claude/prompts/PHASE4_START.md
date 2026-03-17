# Phase 4: New Coordinators - Claude Code Start Prompt (Updated)

Copy this entire prompt into Claude Code to begin Phase 4 implementation:

---

## PROMPT START

Phases 1, 2, and 3 are complete. Now implement Phase 4: New Coordinators.

### Context
- **Repository**: `C:\TrinityBots\TrinityCore`
- **Detailed Plans**: 
  - `.claude/prompts/PHASE4_NEW_COORDINATORS.md` (Dungeon, Arena, BG)
  - `.claude/prompts/PHASE4_4_RAID_COORDINATOR.md` (Raid)
- **Master Plan**: `.claude/plans/COMBAT_REFACTORING_MASTER_PLAN.md`

### Phase 4 Goal
Implement the FOUR missing coordinators identified in the analysis:
- **DungeonCoordinator** - 5-man dungeon coordination
- **ArenaCoordinator** - 2v2/3v3/5v5 PvP coordination
- **BattlegroundCoordinator** - Large-scale PvP objectives
- **RaidCoordinator** - 10-40 man raid coordination (MOST COMPLEX)

---

## Phase 4 Overview (8-9 weeks total)

| Task | Coordinator | Effort | Key Features |
|------|-------------|--------|--------------|
| 4.1 | DungeonCoordinator | 80h | Trash, bosses, M+, wipe recovery |
| 4.2 | ArenaCoordinator | 80h | Kill targets, burst, CC chains |
| 4.3 | BattlegroundCoordinator | 80h | Objectives, roles, strategy |
| 4.4 | **RaidCoordinator** | **100h** | **Tanks, healers, kiting, adds, CDs** |
| **Total** | | **340h** | |

---

## Task 4.1: DungeonCoordinator (80h)

**Files to CREATE** in `src/modules/Playerbot/AI/Coordination/Dungeon/`:

1. **DungeonState.h** - States, enums, structs
2. **DungeonCoordinator.h/.cpp** - Main coordinator
3. **TrashPullManager.h/.cpp** - Pull planning, CC, markers
4. **BossEncounterManager.h/.cpp** - Boss mechanics, phases
5. **WipeRecoveryManager.h/.cpp** - Rez priority, rebuff
6. **MythicPlusManager.h/.cpp** - Timer, affixes, forces

---

## Task 4.2: ArenaCoordinator (80h)

**Files to CREATE** in `src/modules/Playerbot/AI/Coordination/Arena/`:

1. **ArenaState.h** - States, enemy/teammate tracking
2. **ArenaCoordinator.h/.cpp** - Main coordinator
3. **KillTargetManager.h/.cpp** - Target scoring, switches
4. **CCChainManager.h/.cpp** - DR-aware CC chains
5. **BurstCoordinator.h/.cpp** - Burst windows
6. **DefensiveCoordinator.h/.cpp** - Peel, externals

---

## Task 4.3: BattlegroundCoordinator (80h)

**Files to CREATE** in `src/modules/Playerbot/AI/Coordination/Battleground/`:

1. **BGState.h** - States, objectives, roles
2. **BattlegroundCoordinator.h/.cpp** - Main coordinator
3. **BGRoleManager.h/.cpp** - Role assignment
4. **FlagCarrierManager.h/.cpp** - CTF coordination
5. **NodeController.h/.cpp** - Node capture
6. **BGStrategyEngine.h/.cpp** - Strategic decisions

---

## Task 4.4: RaidCoordinator (100h) ⭐ MOST COMPLEX

**Files to CREATE** in `src/modules/Playerbot/AI/Coordination/Raid/`:

### 4.4.1: RaidState.h
All raid-specific enums and structs:
```cpp
enum class RaidState : uint8 { IDLE, FORMING, BUFFING, PULLING, COMBAT, PHASE_TRANSITION, WIPED, RECOVERING };
enum class TankRole : uint8 { ACTIVE, SWAP_READY, ADD_DUTY, KITING, RECOVERING };
enum class HealerAssignment : uint8 { RAID_HEALING, TANK_1, TANK_2, GROUP_1-8, DISPEL_DUTY };

struct RaidTankInfo { ObjectGuid guid; TankRole role; uint8 debuffStacks; ObjectGuid currentTarget; };
struct RaidHealerInfo { ObjectGuid guid; HealerAssignment assignment; float manaPercent; };
struct RaidSubGroup { uint8 groupNumber; std::vector<ObjectGuid> members; };
struct KitePath { std::vector<waypoints>; ObjectGuid assignedKiter; };
struct RaidAdd { ObjectGuid guid; uint8 priority; bool requiresTank; ObjectGuid assignedTank; };
```

### 4.4.2: RaidCoordinator.h/.cpp
Main orchestrator with sub-managers:
- State machine (IDLE → FORMING → COMBAT → WIPED → RECOVERING)
- Inherits `ICombatEventSubscriber`
- Manages 9 sub-components

### 4.4.3: RaidTankCoordinator.h/.cpp
**CRITICAL - Tank swap automation:**
```cpp
void ConfigureSwapTrigger(uint32 spellId, uint8 stackThreshold);
bool NeedsTankSwap() const;
void CallTankSwap();
void OnSwapDebuffApplied(ObjectGuid tank, uint32 spellId, uint8 stacks);
void AssignTankToBoss(ObjectGuid tank);
void AssignTankToAdds(ObjectGuid tank, std::vector<ObjectGuid> adds);
```

### 4.4.4: RaidHealCoordinator.h/.cpp
**Healer assignments + external CDs:**
```cpp
void AssignHealerToTank(ObjectGuid healer, ObjectGuid tank);
void AssignHealerToGroup(ObjectGuid healer, uint8 groupNum);
void RequestExternalCooldown(ObjectGuid target, uint8 urgency);
ObjectGuid GetNextExternalProvider() const;
void AutoAssignHealers();  // Smart auto-assignment
```

### 4.4.5: RaidCooldownRotation.h/.cpp
**Raid-wide CD management:**
```cpp
void CallBloodlust();
void CallRaidDefensive();  // Spirit Link, Rallying Cry, etc.
void CallBattleRez(ObjectGuid target);
bool ShouldUseBloodlust() const;  // Based on phase/health
void LoadCooldownPlan(const CooldownPlan& plan);  // Pre-planned rotation
```

### 4.4.6: RaidGroupManager.h/.cpp
**8 sub-groups + split mechanics:**
```cpp
RaidSubGroup* GetSubGroup(uint8 groupNum);
void AssignToSubGroup(ObjectGuid player, uint8 groupNum);
void SplitRaid(uint8 numGroups);  // For split mechanics
void MergeRaid();
void OptimizeSubGroups();  // Balance tank/healer distribution
```

### 4.4.7: KitingManager.h/.cpp
**Dedicated kiting support:**
```cpp
void RegisterPath(const KitePath& path);
void AssignKiter(ObjectGuid kiter, ObjectGuid target, uint32 pathId);
void SwapKiter(ObjectGuid oldKiter, ObjectGuid newKiter);
float GetRecommendedDistance(ObjectGuid kiter) const;
void OnKiterDied(ObjectGuid kiter);  // Emergency swap
std::tuple<float, float, float> GetNextWaypoint(ObjectGuid kiter) const;
```

### 4.4.8: AddManagementSystem.h/.cpp
**Multi-add handling:**
```cpp
void OnAddSpawned(ObjectGuid guid, uint32 creatureId);
void SetAddPriority(ObjectGuid guid, uint8 priority);
void AssignTankToAdd(ObjectGuid add, ObjectGuid tank);
void AssignDPSToAdd(ObjectGuid add, ObjectGuid dps);
void CallSwitchToAdd(ObjectGuid add);
void DistributeDPSToAdds();  // Auto-balance DPS across adds
ObjectGuid GetHighestPriorityAdd() const;
```

### 4.4.9: RaidPositioningManager.h/.cpp
**Position assignments:**
```cpp
void AssignPosition(ObjectGuid player, float x, float y, float z);
void CallSpread(float distance);
void CallStack(float x, float y, float z);
void CallMoveToPosition(const std::string& positionName);
```

### 4.4.10: RaidEncounterManager.h/.cpp
**Boss mechanics + phases:**
```cpp
void OnPhaseChange(uint8 newPhase);
void OnMechanicTriggered(uint32 spellId);
uint32 GetTimeToEnrage() const;
bool ShouldUseBloodlustNow() const;
```

---

## Implementation Order (8-9 weeks)

| Week | Task | Focus |
|------|------|-------|
| 1 | 4.1 | DungeonCoordinator core + TrashPullManager |
| 2 | 4.1 | BossEncounterManager + WipeRecoveryManager + M+ |
| 3 | 4.2 | ArenaCoordinator + KillTargetManager |
| 4 | 4.2 | CCChainManager + BurstCoordinator |
| 5 | 4.3 | BattlegroundCoordinator + BGRoleManager |
| 6 | 4.3 | FlagCarrierManager + NodeController + Strategy |
| 7 | 4.4 | **RaidCoordinator core + RaidTankCoordinator** |
| 8 | 4.4 | **RaidHealCoordinator + RaidCooldownRotation** |
| 9 | 4.4 | **KitingManager + AddManagementSystem + Positioning** |

---

## Success Criteria

### Dungeon (4.1)
- [ ] TrashPullManager assigns CC targets
- [ ] BossEncounterManager tracks phases
- [ ] WipeRecoveryManager orders rezzes correctly
- [ ] MythicPlusManager tracks timer + forces

### Arena (4.2)
- [ ] KillTargetManager scores targets
- [ ] CCChainManager uses Phase 2 DR tracking
- [ ] BurstCoordinator coordinates cooldowns

### Battleground (4.3)
- [ ] BGRoleManager assigns FC, defenders, etc.
- [ ] NodeController handles capture coordination
- [ ] BGStrategyEngine makes strategic decisions

### Raid (4.4) ⭐
- [ ] **RaidTankCoordinator automates tank swaps**
- [ ] **RaidHealCoordinator assigns healers to tanks/groups**
- [ ] **RaidCooldownRotation manages Bloodlust timing**
- [ ] **KitingManager supports dedicated kiters with paths**
- [ ] **AddManagementSystem handles add priorities + DPS distribution**
- [ ] **RaidGroupManager handles 8 sub-groups + splits**

### General
- [ ] All coordinators use event system (ICombatEventSubscriber)
- [ ] Project compiles without errors

---

## Critical Notes

1. **Raid is MOST COMPLEX** - 21 files, 100 hours
   - Multiple tanks with automatic swap detection
   - Healer assignments per tank/group
   - Kiting with waypoint paths
   - Add management with priority system
   - Raid CD rotation (Bloodlust, Spirit Link, etc.)

2. **Use existing systems**:
   - Phase 2: DR tracking for Arena CC chains
   - Phase 3: Event system for all coordinators

3. **Tank swap is critical**: Many raid encounters require automated tank swaps at X debuff stacks

4. **Kiting is complex**: Kiters need paths, distance management, emergency swaps

5. **Healer assignments matter**: Tank healers vs raid healers vs group healers

Start with Task 4.1 (DungeonCoordinator), then proceed in order.

---

## PROMPT END
