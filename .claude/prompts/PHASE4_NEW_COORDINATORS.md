# Phase 4: New Coordinators - Detailed Implementation Plan

**Version**: 1.0  
**Date**: 2026-01-26  
**Prerequisites**: Phase 1, 2, 3 complete  
**Duration**: ~6 weeks (240 hours)

---

## Overview

Phase 4 implements the three missing coordinators identified in the Zenflow analysis:
- **DungeonCoordinator** - 5-man dungeon coordination
- **ArenaCoordinator** - 2v2, 3v3, 5v5 PvP coordination  
- **BattlegroundCoordinator** - Large-scale PvP objective coordination

These coordinators fill the critical gaps in context support:

| Context | Before Phase 4 | After Phase 4 |
|---------|----------------|---------------|
| Dungeon | 0% explicit | 100% |
| Arena | 8% | 80% |
| Battleground | 8% | 80% |

---

## Task 4.1: DungeonCoordinator Implementation

**Priority**: P0  
**Effort**: 80 hours (2 weeks)  
**Dependencies**: Phase 3 complete

### Files to Create

```
src/modules/Playerbot/AI/Coordination/Dungeon/DungeonCoordinator.h
src/modules/Playerbot/AI/Coordination/Dungeon/DungeonCoordinator.cpp
src/modules/Playerbot/AI/Coordination/Dungeon/DungeonState.h
src/modules/Playerbot/AI/Coordination/Dungeon/TrashPullManager.h
src/modules/Playerbot/AI/Coordination/Dungeon/TrashPullManager.cpp
src/modules/Playerbot/AI/Coordination/Dungeon/BossEncounterManager.h
src/modules/Playerbot/AI/Coordination/Dungeon/BossEncounterManager.cpp
src/modules/Playerbot/AI/Coordination/Dungeon/WipeRecoveryManager.h
src/modules/Playerbot/AI/Coordination/Dungeon/WipeRecoveryManager.cpp
src/modules/Playerbot/AI/Coordination/Dungeon/MythicPlusManager.h
src/modules/Playerbot/AI/Coordination/Dungeon/MythicPlusManager.cpp
```

### 4.1.1: Create DungeonState.h

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "ObjectGuid.h"

namespace Playerbot {

enum class DungeonState : uint8 {
    IDLE = 0,           // Not in dungeon
    ENTERING = 1,       // Zoning into dungeon
    READY_CHECK = 2,    // Waiting for group ready
    CLEARING_TRASH = 3, // Fighting trash mobs
    PRE_BOSS = 4,       // Preparing for boss (rebuff, mana)
    BOSS_COMBAT = 5,    // Fighting boss
    POST_BOSS = 6,      // Looting, recovering after boss
    WIPED = 7,          // Group wiped, recovery needed
    COMPLETED = 8       // Dungeon complete
};

enum class DungeonDifficulty : uint8 {
    NORMAL = 0,
    HEROIC = 1,
    MYTHIC = 2,
    MYTHIC_PLUS = 3
};

enum class TrashPackPriority : uint8 {
    SKIP = 0,       // Can be skipped
    OPTIONAL = 1,   // Kill if convenient
    REQUIRED = 2,   // Must kill for progress
    PATROL = 3,     // Patrol - timing dependent
    DANGEROUS = 4   // High priority dangerous pack
};

enum class RaidMarker : uint8 {
    NONE = 0,
    SKULL = 1,      // Kill first
    CROSS = 2,      // Kill second
    MOON = 3,       // Polymorph/CC
    SQUARE = 4,     // Trap/CC
    TRIANGLE = 5,   // Sap/CC
    DIAMOND = 6,    // CC
    CIRCLE = 7,     // CC
    STAR = 8        // Misc
};

struct TrashPack {
    uint32 packId;
    std::vector<ObjectGuid> members;
    TrashPackPriority priority;
    bool requiresCC;
    uint8 recommendedCCCount;
    bool isPatrol;
    float patrolPathLength;
    bool canBePulledWith;  // Can combine with another pack
    uint32 linkedPackId;    // Pack that comes if pulled
};

struct BossInfo {
    uint32 bossId;
    uint32 encounterId;
    std::string name;
    uint8 currentPhase;
    uint8 maxPhases;
    bool hasEnrageTimer;
    uint32 enrageTimeMs;
    uint32 combatStartTime;
    std::vector<uint32> importantSpellIds;  // Spells to interrupt/avoid
    std::vector<uint32> tankSwapSpellIds;   // Spells requiring tank swap
};

struct DungeonProgress {
    uint32 dungeonId;
    uint32 instanceId;
    DungeonDifficulty difficulty;
    uint8 bossesKilled;
    uint8 totalBosses;
    uint32 trashKilled;
    uint32 totalTrash;
    float completionPercent;
    
    // Mythic+ specific
    bool isMythicPlus;
    uint8 keystoneLevel;
    uint32 timeLimit;
    uint32 elapsedTime;
    uint32 deathCount;
    float enemyForcesPercent;
    float requiredEnemyForces;
};

const char* DungeonStateToString(DungeonState state);
const char* DifficultyToString(DungeonDifficulty diff);

} // namespace Playerbot
```

### 4.1.2: Create DungeonCoordinator.h

```cpp
#pragma once

#include "DungeonState.h"
#include "Core/Events/ICombatEventSubscriber.h"
#include "Core/DI/Interfaces/IGroupCoordinator.h"
#include <memory>
#include <vector>
#include <map>

class Group;
class Player;
class Map;
class Creature;

namespace Playerbot {

class TrashPullManager;
class BossEncounterManager;
class WipeRecoveryManager;
class MythicPlusManager;

class DungeonCoordinator : public ICombatEventSubscriber {
public:
    DungeonCoordinator(Group* group);
    ~DungeonCoordinator();
    
    // Lifecycle
    void Initialize();
    void Shutdown();
    void Update(uint32 diff);
    
    // State Management
    DungeonState GetState() const { return _state; }
    void SetState(DungeonState newState);
    bool IsInDungeon() const { return _state != DungeonState::IDLE; }
    
    // Progress
    const DungeonProgress& GetProgress() const { return _progress; }
    float GetCompletionPercent() const;
    
    // Trash Management
    TrashPack* GetCurrentPullTarget() const;
    void MarkPackForPull(uint32 packId);
    void AssignCCTargets(const TrashPack& pack);
    bool IsSafeToPull() const;
    void OnTrashPackCleared(uint32 packId);
    
    // Boss Management
    BossInfo* GetCurrentBoss() const;
    bool IsInBossEncounter() const { return _state == DungeonState::BOSS_COMBAT; }
    void OnBossEngaged(uint32 bossId);
    void OnBossDefeated(uint32 bossId);
    void OnBossWipe(uint32 bossId);
    uint8 GetBossPhase() const;
    
    // Wipe Recovery
    void OnGroupWipe();
    bool IsRecovering() const { return _state == DungeonState::WIPED; }
    ObjectGuid GetNextRezTarget() const;
    void OnPlayerRezzed(ObjectGuid playerGuid);
    
    // Role Queries
    ObjectGuid GetMainTank() const { return _mainTank; }
    ObjectGuid GetOffTank() const { return _offTank; }
    std::vector<ObjectGuid> GetHealers() const { return _healers; }
    
    // Mythic+ Specific
    bool IsMythicPlus() const { return _progress.isMythicPlus; }
    uint8 GetKeystoneLevel() const { return _progress.keystoneLevel; }
    uint32 GetRemainingTime() const;
    bool ShouldSkipPack(const TrashPack& pack) const;
    
    // ICombatEventSubscriber
    void OnCombatEvent(const CombatEvent& event) override;
    CombatEventType GetSubscribedEventTypes() const override;
    
private:
    // State
    DungeonState _state = DungeonState::IDLE;
    DungeonProgress _progress;
    
    // References
    Group* _group;
    Map* _dungeonMap;
    
    // Roles
    ObjectGuid _mainTank;
    ObjectGuid _offTank;
    std::vector<ObjectGuid> _healers;
    std::vector<ObjectGuid> _dps;
    
    // Sub-managers
    std::unique_ptr<TrashPullManager> _trashManager;
    std::unique_ptr<BossEncounterManager> _bossManager;
    std::unique_ptr<WipeRecoveryManager> _wipeManager;
    std::unique_ptr<MythicPlusManager> _mythicPlusManager;
    
    // State machine
    void UpdateState(uint32 diff);
    void TransitionTo(DungeonState newState);
    void OnStateEnter(DungeonState state);
    void OnStateExit(DungeonState state);
    
    // Event handlers
    void HandleUnitDied(const CombatEvent& event);
    void HandleCombatStarted(const CombatEvent& event);
    void HandleCombatEnded(const CombatEvent& event);
    void HandleEncounterStart(const CombatEvent& event);
    void HandleEncounterEnd(const CombatEvent& event);
    void HandleBossPhaseChanged(const CombatEvent& event);
    
    // Helpers
    void DetectRoles();
    void LoadDungeonData(uint32 dungeonId);
    bool IsGroupReady() const;
    bool IsGroupInCombat() const;
    float CalculateGroupHealth() const;
    float CalculateGroupMana() const;
};

} // namespace Playerbot
```

### 4.1.3: Create TrashPullManager.h

```cpp
#pragma once

#include "DungeonState.h"
#include <vector>
#include <map>
#include <queue>

namespace Playerbot {

class DungeonCoordinator;

struct PullPlan {
    uint32 packId;
    std::vector<std::pair<ObjectGuid, RaidMarker>> markerAssignments;
    std::vector<std::pair<ObjectGuid, ObjectGuid>> ccAssignments;  // CCer -> Target
    ObjectGuid puller;
    bool useLOS;
    float pullPosition[3];
};

class TrashPullManager {
public:
    TrashPullManager(DungeonCoordinator* coordinator);
    
    void Initialize(uint32 dungeonId);
    void Update(uint32 diff);
    
    // Pack Management
    void RegisterPack(const TrashPack& pack);
    void OnPackCleared(uint32 packId);
    void OnPackPulled(uint32 packId);
    
    // Pull Planning
    PullPlan* GetCurrentPullPlan();
    bool CreatePullPlan(uint32 packId);
    void ExecutePull(const PullPlan& plan);
    
    // CC Management
    void AssignCC(const TrashPack& pack);
    ObjectGuid GetCCResponsible(ObjectGuid target) const;
    bool IsTargetCCd(ObjectGuid target) const;
    void OnCCBroken(ObjectGuid target);
    
    // Marker Management
    void ApplyMarkers(const PullPlan& plan);
    void ClearMarkers();
    RaidMarker GetMarkerForTarget(ObjectGuid target) const;
    
    // Safety
    bool IsSafeToPull() const;
    bool IsGroupReadyForPull() const;
    uint32 GetEstimatedPullDifficulty(uint32 packId) const;
    
    // Pathing
    std::vector<uint32> GetOptimalClearOrder() const;
    bool CanSkipPack(uint32 packId) const;
    
private:
    DungeonCoordinator* _coordinator;
    
    std::map<uint32, TrashPack> _packs;
    std::vector<uint32> _clearedPacks;
    std::queue<uint32> _pullQueue;
    
    PullPlan _currentPlan;
    bool _hasPlan = false;
    
    // CC tracking
    std::map<ObjectGuid, ObjectGuid> _ccAssignments;  // target -> ccer
    std::map<ObjectGuid, uint32> _ccSpells;           // target -> spell used
    
    // Marker tracking
    std::map<ObjectGuid, RaidMarker> _markerAssignments;
    
    // Helper methods
    ObjectGuid SelectBestCCer(ObjectGuid target) const;
    bool CanCC(ObjectGuid player, ObjectGuid target) const;
    RaidMarker SelectMarkerForRole(uint8 killOrder, bool isCC) const;
    void CalculateKillOrder(const TrashPack& pack, std::vector<ObjectGuid>& order);
};

} // namespace Playerbot
```

### 4.1.4: Create BossEncounterManager.h

```cpp
#pragma once

#include "DungeonState.h"
#include <map>
#include <vector>
#include <functional>

namespace Playerbot {

class DungeonCoordinator;

enum class BossMechanic : uint8 {
    NONE = 0,
    TANK_SWAP = 1,          // Requires tank swap
    SPREAD = 2,             // Spread out
    STACK = 3,              // Stack together
    MOVE_OUT = 4,           // Move away from boss
    MOVE_IN = 5,            // Move to boss
    INTERRUPT = 6,          // Must interrupt
    DISPEL = 7,             // Requires dispel
    DODGE_AOE = 8,          // Dodge ground effect
    SOAK = 9,               // Soak mechanic
    KITE = 10,              // Kite add/boss
    SWITCH_TARGET = 11,     // Switch to add
    USE_EXTRA_BUTTON = 12,  // Use extra action button
    BLOODLUST = 13          // Use bloodlust/heroism
};

struct MechanicTrigger {
    uint32 spellId;         // Spell that triggers this
    BossMechanic mechanic;
    uint8 phase;            // 0 = all phases
    float healthThreshold;  // 0 = no threshold
    std::string description;
};

struct BossStrategy {
    uint32 bossId;
    std::string name;
    
    // Phase info
    uint8 totalPhases;
    std::vector<float> phaseTransitionHealth;  // Health % for phase transitions
    
    // Mechanics
    std::vector<MechanicTrigger> mechanics;
    
    // Positioning
    bool tankFaceAway;
    bool spreadInPhase[5];
    float spreadDistance;
    
    // Timers
    bool hasEnrage;
    uint32 enrageTimeMs;
    bool useBloodlustOnPull;
    float bloodlustHealthPercent;
    
    // Tank swap
    bool requiresTankSwap;
    uint32 tankSwapSpellId;
    uint8 tankSwapStacks;
    
    // Interrupts
    std::vector<uint32> mustInterruptSpells;
    std::vector<uint32> shouldInterruptSpells;
    
    // Priority targets
    std::vector<uint32> priorityAddIds;
};

class BossEncounterManager {
public:
    BossEncounterManager(DungeonCoordinator* coordinator);
    
    void Initialize(uint32 dungeonId);
    void Update(uint32 diff);
    
    // Encounter lifecycle
    void OnBossEngaged(uint32 bossId);
    void OnBossDefeated(uint32 bossId);
    void OnBossWipe(uint32 bossId);
    void OnPhaseChanged(uint8 newPhase);
    
    // Strategy
    const BossStrategy* GetCurrentStrategy() const;
    const BossStrategy* GetStrategy(uint32 bossId) const;
    void LoadBossStrategies(uint32 dungeonId);
    
    // Phase tracking
    uint8 GetCurrentPhase() const { return _currentPhase; }
    float GetPhaseProgress() const;
    bool IsPhaseTransition() const { return _inPhaseTransition; }
    
    // Mechanic handling
    void OnMechanicTriggered(uint32 spellId);
    BossMechanic GetActiveMechanic() const { return _activeMechanic; }
    bool ShouldSpread() const;
    bool ShouldStack() const;
    ObjectGuid GetStackTarget() const;
    
    // Tank swap
    bool NeedsTankSwap() const;
    void OnTankSwapCompleted();
    uint8 GetTankSwapStacks(ObjectGuid tank) const;
    
    // Interrupt priority
    bool ShouldInterrupt(uint32 spellId) const;
    uint8 GetInterruptPriority(uint32 spellId) const;
    
    // Bloodlust timing
    bool ShouldUseBloodlust() const;
    void OnBloodlustUsed();
    
    // Combat stats
    uint32 GetEncounterDuration() const;
    float GetBossHealthPercent() const;
    bool IsEnraging() const;
    uint32 GetTimeToEnrage() const;
    
private:
    DungeonCoordinator* _coordinator;
    
    // Current encounter
    uint32 _currentBossId = 0;
    uint8 _currentPhase = 0;
    bool _inPhaseTransition = false;
    BossMechanic _activeMechanic = BossMechanic::NONE;
    uint32 _encounterStartTime = 0;
    bool _bloodlustUsed = false;
    
    // Tank swap tracking
    std::map<ObjectGuid, uint8> _tankSwapStacks;
    ObjectGuid _currentTank;
    
    // Strategies
    std::map<uint32, BossStrategy> _strategies;
    
    // Mechanic handlers
    void HandleTankSwapMechanic(const MechanicTrigger& trigger);
    void HandleSpreadMechanic(const MechanicTrigger& trigger);
    void HandleStackMechanic(const MechanicTrigger& trigger);
    void HandleDodgeMechanic(const MechanicTrigger& trigger);
};

} // namespace Playerbot
```

### 4.1.5: Create WipeRecoveryManager.h

```cpp
#pragma once

#include "ObjectGuid.h"
#include <vector>
#include <queue>

namespace Playerbot {

class DungeonCoordinator;

enum class RecoveryPhase : uint8 {
    WAITING = 0,        // Waiting for combat to end
    REZZING = 1,        // Rezzing dead members
    REBUFFING = 2,      // Rebuffing group
    MANA_REGEN = 3,     // Waiting for mana
    READY = 4           // Ready to continue
};

struct RezPriority {
    ObjectGuid playerGuid;
    uint8 priority;     // Lower = higher priority
    bool hasRezSickness;
    uint32 distanceToCorpse;
    
    bool operator<(const RezPriority& other) const {
        return priority > other.priority;  // Min-heap
    }
};

class WipeRecoveryManager {
public:
    WipeRecoveryManager(DungeonCoordinator* coordinator);
    
    void Initialize();
    void Update(uint32 diff);
    
    // Wipe handling
    void OnGroupWipe();
    void OnCombatEnded();
    bool IsRecovering() const { return _phase != RecoveryPhase::READY; }
    RecoveryPhase GetPhase() const { return _phase; }
    
    // Rez management
    void BuildRezQueue();
    ObjectGuid GetNextRezTarget() const;
    void OnPlayerRezzed(ObjectGuid playerGuid);
    bool AllPlayersAlive() const;
    
    // Rez priority
    void SetRezPriority(ObjectGuid player, uint8 priority);
    uint8 GetRezPriority(ObjectGuid player) const;
    
    // Run back
    bool ShouldRunBack() const;
    void OnPlayerReachedCorpse(ObjectGuid playerGuid);
    
    // Ready check
    bool IsGroupReady() const;
    float GetGroupManaPercent() const;
    float GetGroupHealthPercent() const;
    bool AreBuffsComplete() const;
    
private:
    DungeonCoordinator* _coordinator;
    
    RecoveryPhase _phase = RecoveryPhase::READY;
    std::priority_queue<RezPriority> _rezQueue;
    std::vector<ObjectGuid> _deadPlayers;
    std::vector<ObjectGuid> _rezzedPlayers;
    
    uint32 _recoveryStartTime = 0;
    uint32 _phaseTimer = 0;
    
    // Rez order: Healer > Tank > DPS
    uint8 CalculateRezPriority(ObjectGuid playerGuid) const;
    void TransitionToPhase(RecoveryPhase newPhase);
};

} // namespace Playerbot
```

### 4.1.6: Create MythicPlusManager.h

```cpp
#pragma once

#include "DungeonState.h"
#include <vector>
#include <set>

namespace Playerbot {

class DungeonCoordinator;

enum class MythicPlusAffix : uint32 {
    // Level 4+
    BOLSTERING = 7,
    RAGING = 6,
    SANGUINE = 8,
    BURSTING = 11,
    
    // Level 7+
    TYRANNICAL = 9,
    FORTIFIED = 10,
    
    // Level 10+
    NECROTIC = 4,
    VOLCANIC = 3,
    EXPLOSIVE = 13,
    QUAKING = 14,
    GRIEVOUS = 12,
    
    // Seasonal
    AWAKENED = 120,
    PRIDEFUL = 121,
    TORMENTED = 128
};

struct KeystoneInfo {
    uint32 dungeonId;
    uint8 level;
    std::vector<MythicPlusAffix> affixes;
    uint32 timeLimit;       // In milliseconds
    float timeLimitMod;     // Multiplier for 2/3 chest
};

struct EnemyForces {
    uint32 creatureId;
    float forcesValue;      // Percentage this mob gives
    bool isPriority;        // High value target
};

class MythicPlusManager {
public:
    MythicPlusManager(DungeonCoordinator* coordinator);
    
    void Initialize(const KeystoneInfo& keystone);
    void Update(uint32 diff);
    
    // Keystone info
    uint8 GetKeystoneLevel() const { return _keystone.level; }
    bool HasAffix(MythicPlusAffix affix) const;
    std::vector<MythicPlusAffix> GetActiveAffixes() const { return _keystone.affixes; }
    
    // Timer
    uint32 GetTimeLimit() const { return _keystone.timeLimit; }
    uint32 GetElapsedTime() const;
    uint32 GetRemainingTime() const;
    bool IsOnTime() const;
    bool CanTwoChest() const;
    bool CanThreeChest() const;
    float GetTimeProgress() const;  // 0.0 - 1.0
    
    // Enemy forces
    float GetEnemyForcesPercent() const { return _enemyForces; }
    float GetRequiredEnemyForces() const { return 100.0f; }
    bool HasEnoughForces() const { return _enemyForces >= 100.0f; }
    void OnEnemyKilled(uint32 creatureId);
    float GetForcesValue(uint32 creatureId) const;
    
    // Death counter
    uint32 GetDeathCount() const { return _deathCount; }
    uint32 GetTimePenalty() const { return _deathCount * 5000; }  // 5 sec per death
    void OnPlayerDied();
    
    // Route optimization
    bool ShouldSkipPack(uint32 packId) const;
    bool ShouldPullExtra() const;
    std::vector<uint32> GetOptimalRoute() const;
    
    // Affix handling
    void OnAffixTriggered(MythicPlusAffix affix, ObjectGuid source);
    bool ShouldKillExplosive(ObjectGuid explosive) const;
    bool ShouldAvoidSanguine(float x, float y, float z) const;
    bool IsQuakingActive() const;
    
    // Strategy adjustments
    float GetDamageModifier() const;   // From Fortified/Tyrannical
    float GetHealthModifier() const;
    bool ShouldUseCooldowns() const;
    bool ShouldLust() const;
    
private:
    DungeonCoordinator* _coordinator;
    
    KeystoneInfo _keystone;
    uint32 _startTime = 0;
    float _enemyForces = 0.0f;
    uint32 _deathCount = 0;
    
    // Enemy forces data
    std::map<uint32, EnemyForces> _forcesTable;
    
    // Affix state
    bool _quakingActive = false;
    uint32 _quakingEndTime = 0;
    std::set<ObjectGuid> _sanguinePools;
    std::set<ObjectGuid> _explosiveOrbs;
    
    // Route
    std::vector<uint32> _plannedRoute;
    uint32 _currentRouteIndex = 0;
    
    void LoadForcesTable(uint32 dungeonId);
    void CalculateOptimalRoute();
};

} // namespace Playerbot
```

---

## Task 4.2: ArenaCoordinator Implementation

**Priority**: P1  
**Effort**: 80 hours (2 weeks)  
**Dependencies**: Phase 3 complete, DR tracking from Phase 2

### Files to Create

```
src/modules/Playerbot/AI/Coordination/Arena/ArenaCoordinator.h
src/modules/Playerbot/AI/Coordination/Arena/ArenaCoordinator.cpp
src/modules/Playerbot/AI/Coordination/Arena/ArenaState.h
src/modules/Playerbot/AI/Coordination/Arena/KillTargetManager.h
src/modules/Playerbot/AI/Coordination/Arena/KillTargetManager.cpp
src/modules/Playerbot/AI/Coordination/Arena/BurstCoordinator.h
src/modules/Playerbot/AI/Coordination/Arena/BurstCoordinator.cpp
src/modules/Playerbot/AI/Coordination/Arena/CCChainManager.h
src/modules/Playerbot/AI/Coordination/Arena/CCChainManager.cpp
src/modules/Playerbot/AI/Coordination/Arena/DefensiveCoordinator.h
src/modules/Playerbot/AI/Coordination/Arena/DefensiveCoordinator.cpp
src/modules/Playerbot/AI/Coordination/Arena/ArenaPositioning.h
src/modules/Playerbot/AI/Coordination/Arena/ArenaPositioning.cpp
```

### 4.2.1: Create ArenaState.h

```cpp
#pragma once

#include "ObjectGuid.h"
#include <cstdint>
#include <vector>
#include <map>

namespace Playerbot {

enum class ArenaState : uint8 {
    IDLE = 0,
    QUEUED = 1,
    PREPARATION = 2,    // In arena, gates closed
    GATES_OPENING = 3,  // 5 second countdown
    COMBAT = 4,         // Active combat
    VICTORY = 5,
    DEFEAT = 6
};

enum class ArenaType : uint8 {
    ARENA_2V2 = 2,
    ARENA_3V3 = 3,
    ARENA_5V5 = 5
};

enum class ArenaBracket : uint8 {
    SKIRMISH = 0,
    RATED = 1,
    SOLO_SHUFFLE = 2
};

enum class ArenaRole : uint8 {
    UNKNOWN = 0,
    HEALER = 1,
    MELEE_DPS = 2,
    RANGED_DPS = 3,
    HYBRID = 4      // Can swap roles
};

enum class TargetPriority : uint8 {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    KILL_TARGET = 3
};

struct ArenaEnemy {
    ObjectGuid guid;
    uint32 classId;
    uint32 specId;
    ArenaRole role;
    
    // Tracked cooldowns
    bool trinketAvailable;
    uint32 trinketCooldown;
    std::map<uint32, uint32> majorCooldowns;  // spellId -> readyTime
    
    // Status
    float healthPercent;
    float manaPercent;
    bool isInCC;
    bool isInDefensiveCooldown;
    TargetPriority currentPriority;
};

struct ArenaTeammate {
    ObjectGuid guid;
    uint32 classId;
    uint32 specId;
    ArenaRole role;
    
    // Resources
    float healthPercent;
    float manaPercent;
    
    // Cooldowns available
    bool hasDefensivesAvailable;
    bool hasCCAvailable;
    bool hasBurstAvailable;
    
    // Status
    bool needsPeel;
    bool isCC;
    uint8 ccBreakPriority;  // 0 = don't break, 3 = break immediately
};

struct BurstWindow {
    uint32 startTime;
    uint32 duration;
    ObjectGuid target;
    std::vector<ObjectGuid> participants;
    bool isActive;
};

const char* ArenaStateToString(ArenaState state);
const char* ArenaRoleToString(ArenaRole role);

} // namespace Playerbot
```

### 4.2.2: Create ArenaCoordinator.h

```cpp
#pragma once

#include "ArenaState.h"
#include "Core/Events/ICombatEventSubscriber.h"
#include <memory>
#include <vector>

class Battleground;
class Player;

namespace Playerbot {

class KillTargetManager;
class BurstCoordinator;
class CCChainManager;
class DefensiveCoordinator;
class ArenaPositioning;
class CrowdControlManager;  // From Phase 2 (DR tracking)

class ArenaCoordinator : public ICombatEventSubscriber {
public:
    ArenaCoordinator(Battleground* arena, std::vector<Player*> team);
    ~ArenaCoordinator();
    
    // Lifecycle
    void Initialize();
    void Shutdown();
    void Update(uint32 diff);
    
    // State
    ArenaState GetState() const { return _state; }
    ArenaType GetType() const { return _type; }
    bool IsInCombat() const { return _state == ArenaState::COMBAT; }
    
    // Kill target
    ObjectGuid GetKillTarget() const;
    void SetKillTarget(ObjectGuid target);
    void CallSwitch(ObjectGuid newTarget);
    TargetPriority GetTargetPriority(ObjectGuid target) const;
    
    // Burst coordination
    bool IsBurstWindowActive() const;
    void CallBurst(ObjectGuid target);
    void CallOffBurst();
    bool ShouldUseCooldowns() const;
    const BurstWindow* GetCurrentBurstWindow() const;
    
    // CC management
    void RequestCC(ObjectGuid target, uint32 durationMs);
    void CallCCChain(ObjectGuid target);
    bool CanCCTarget(ObjectGuid target) const;
    float GetExpectedCCDuration(ObjectGuid target, uint32 spellId) const;
    
    // Defensive coordination
    void RequestPeel(ObjectGuid teammate);
    void CallDefensives(ObjectGuid target);
    ObjectGuid GetPeelTarget() const;
    bool ShouldUseDefensive(ObjectGuid player) const;
    
    // Positioning
    void RequestReposition(ObjectGuid player, float x, float y, float z);
    bool ShouldLOS() const;
    float GetPillarDistance(ObjectGuid player) const;
    
    // Enemy tracking
    const ArenaEnemy* GetEnemy(ObjectGuid guid) const;
    std::vector<ArenaEnemy> GetEnemies() const;
    bool IsEnemyTrinketDown(ObjectGuid enemy) const;
    bool IsEnemyDefensiveDown(ObjectGuid enemy) const;
    
    // Teammate tracking
    const ArenaTeammate* GetTeammate(ObjectGuid guid) const;
    std::vector<ArenaTeammate> GetTeammates() const;
    bool IsTeammateInTrouble(ObjectGuid teammate) const;
    
    // ICombatEventSubscriber
    void OnCombatEvent(const CombatEvent& event) override;
    CombatEventType GetSubscribedEventTypes() const override;
    
private:
    // State
    ArenaState _state = ArenaState::IDLE;
    ArenaType _type;
    
    // References
    Battleground* _arena;
    std::vector<Player*> _team;
    
    // Enemy/teammate tracking
    std::vector<ArenaEnemy> _enemies;
    std::vector<ArenaTeammate> _teammates;
    
    // Sub-managers
    std::unique_ptr<KillTargetManager> _killTargetManager;
    std::unique_ptr<BurstCoordinator> _burstCoordinator;
    std::unique_ptr<CCChainManager> _ccChainManager;
    std::unique_ptr<DefensiveCoordinator> _defensiveCoordinator;
    std::unique_ptr<ArenaPositioning> _positioning;
    CrowdControlManager* _ccManager;  // Shared, for DR tracking
    
    // Current burst window
    BurstWindow _currentBurst;
    
    // State machine
    void UpdateState(uint32 diff);
    void TransitionTo(ArenaState newState);
    
    // Event handlers
    void HandleDamageTaken(const CombatEvent& event);
    void HandleSpellCastStart(const CombatEvent& event);
    void HandleSpellCastSuccess(const CombatEvent& event);
    void HandleAuraApplied(const CombatEvent& event);
    void HandleUnitDied(const CombatEvent& event);
    
    // Tracking
    void UpdateEnemyTracking(uint32 diff);
    void UpdateTeammateTracking(uint32 diff);
    void TrackCooldownUsage(ObjectGuid caster, uint32 spellId);
    
    // Decision making
    ObjectGuid EvaluateBestKillTarget() const;
    bool ShouldCallSwitch() const;
    bool ShouldCallBurst() const;
};

} // namespace Playerbot
```

### 4.2.3: Create KillTargetManager.h

```cpp
#pragma once

#include "ArenaState.h"
#include <vector>

namespace Playerbot {

class ArenaCoordinator;

struct TargetScore {
    ObjectGuid target;
    float score;
    std::string reason;
};

class KillTargetManager {
public:
    KillTargetManager(ArenaCoordinator* coordinator);
    
    void Update(uint32 diff);
    
    // Kill target
    ObjectGuid GetKillTarget() const { return _killTarget; }
    void SetKillTarget(ObjectGuid target);
    void ClearKillTarget();
    
    // Target evaluation
    std::vector<TargetScore> EvaluateAllTargets() const;
    float CalculateTargetScore(const ArenaEnemy& enemy) const;
    ObjectGuid GetRecommendedTarget() const;
    
    // Switch logic
    bool ShouldSwitch() const;
    ObjectGuid GetSwitchTarget() const;
    void OnSwitchCalled(ObjectGuid newTarget);
    
    // Priority factors
    void SetPriorityBonus(ObjectGuid target, float bonus);
    void ClearPriorityBonus(ObjectGuid target);
    
private:
    ArenaCoordinator* _coordinator;
    
    ObjectGuid _killTarget;
    uint32 _targetSetTime = 0;
    uint32 _lastEvaluationTime = 0;
    
    std::map<ObjectGuid, float> _priorityBonuses;
    
    // Scoring weights
    static constexpr float WEIGHT_HEALTH = 2.0f;
    static constexpr float WEIGHT_TRINKET_DOWN = 1.5f;
    static constexpr float WEIGHT_DEFENSIVE_DOWN = 1.5f;
    static constexpr float WEIGHT_HEALER = 1.2f;
    static constexpr float WEIGHT_IN_CC = -2.0f;  // Don't target CC'd
    static constexpr float WEIGHT_CURRENT_TARGET = 0.5f;  // Stick to target
    
    float ScoreHealth(float healthPercent) const;
    float ScoreCooldowns(const ArenaEnemy& enemy) const;
    float ScoreRole(ArenaRole role) const;
    float ScorePosition(ObjectGuid target) const;
};

} // namespace Playerbot
```

### 4.2.4: Create CCChainManager.h

```cpp
#pragma once

#include "ArenaState.h"
#include <queue>
#include <vector>

namespace Playerbot {

class ArenaCoordinator;
class CrowdControlManager;

struct CCLink {
    ObjectGuid caster;
    uint32 spellId;
    uint32 expectedDuration;
    uint8 drStack;          // DR stack when applied
};

struct CCChain {
    ObjectGuid target;
    std::vector<CCLink> links;
    uint32 totalDuration;
    uint32 startTime;
    bool isActive;
};

class CCChainManager {
public:
    CCChainManager(ArenaCoordinator* coordinator, CrowdControlManager* ccManager);
    
    void Update(uint32 diff);
    
    // Chain management
    bool StartChain(ObjectGuid target);
    void EndChain();
    bool IsChainActive() const { return _activeChain.isActive; }
    const CCChain& GetActiveChain() const { return _activeChain; }
    
    // Chain planning
    CCChain PlanChain(ObjectGuid target, uint32 desiredDuration) const;
    bool CanChainTarget(ObjectGuid target) const;
    uint32 GetMaxChainDuration(ObjectGuid target) const;
    
    // DR-aware CC
    uint32 GetExpectedDuration(ObjectGuid target, uint32 spellId) const;
    uint8 GetDRStacks(ObjectGuid target, uint32 spellId) const;
    bool IsImmune(ObjectGuid target, uint32 spellId) const;
    
    // Chain execution
    ObjectGuid GetNextCCer() const;
    uint32 GetNextCCSpell() const;
    uint32 GetTimeUntilNextCC() const;
    void OnCCApplied(ObjectGuid caster, ObjectGuid target, uint32 spellId);
    void OnCCBroken(ObjectGuid target);
    void OnCCExpired(ObjectGuid target);
    
    // Coordination
    void RequestCC(ObjectGuid caster, ObjectGuid target);
    bool ShouldOverlapCC() const;
    
private:
    ArenaCoordinator* _coordinator;
    CrowdControlManager* _ccManager;  // For DR tracking
    
    CCChain _activeChain;
    std::queue<CCLink> _pendingLinks;
    
    uint32 _chainIndex = 0;
    uint32 _lastCCEndTime = 0;
    
    // CC availability per player
    std::map<ObjectGuid, std::vector<uint32>> _availableCC;  // player -> spells
    std::map<ObjectGuid, std::map<uint32, uint32>> _ccCooldowns;  // player -> spell -> readyTime
    
    void LoadPlayerCCAbilities(ObjectGuid player);
    std::vector<uint32> GetAvailableCCSpells(ObjectGuid player) const;
    ObjectGuid FindBestCCer(ObjectGuid target, uint8 maxDR) const;
};

} // namespace Playerbot
```

---

## Task 4.3: BattlegroundCoordinator Implementation

**Priority**: P2  
**Effort**: 80 hours (2 weeks)  
**Dependencies**: Phase 3 complete

### Files to Create

```
src/modules/Playerbot/AI/Coordination/Battleground/BattlegroundCoordinator.h
src/modules/Playerbot/AI/Coordination/Battleground/BattlegroundCoordinator.cpp
src/modules/Playerbot/AI/Coordination/Battleground/BGState.h
src/modules/Playerbot/AI/Coordination/Battleground/ObjectiveManager.h
src/modules/Playerbot/AI/Coordination/Battleground/ObjectiveManager.cpp
src/modules/Playerbot/AI/Coordination/Battleground/BGRoleManager.h
src/modules/Playerbot/AI/Coordination/Battleground/BGRoleManager.cpp
src/modules/Playerbot/AI/Coordination/Battleground/FlagCarrierManager.h
src/modules/Playerbot/AI/Coordination/Battleground/FlagCarrierManager.cpp
src/modules/Playerbot/AI/Coordination/Battleground/NodeController.h
src/modules/Playerbot/AI/Coordination/Battleground/NodeController.cpp
src/modules/Playerbot/AI/Coordination/Battleground/BGStrategyEngine.h
src/modules/Playerbot/AI/Coordination/Battleground/BGStrategyEngine.cpp
```

### 4.3.1: Create BGState.h

```cpp
#pragma once

#include "ObjectGuid.h"
#include <cstdint>
#include <vector>
#include <string>

namespace Playerbot {

enum class BGState : uint8 {
    IDLE = 0,
    QUEUED = 1,
    PREPARATION = 2,
    ACTIVE = 3,
    OVERTIME = 4,
    VICTORY = 5,
    DEFEAT = 6
};

enum class BGType : uint32 {
    WARSONG_GULCH = 489,
    ARATHI_BASIN = 529,
    ALTERAC_VALLEY = 30,
    EYE_OF_THE_STORM = 566,
    STRAND_OF_THE_ANCIENTS = 607,
    ISLE_OF_CONQUEST = 628,
    TWIN_PEAKS = 726,
    BATTLE_FOR_GILNEAS = 761,
    SILVERSHARD_MINES = 727,
    TEMPLE_OF_KOTMOGU = 998,
    DEEPWIND_GORGE = 1105
};

enum class BGRole : uint8 {
    UNASSIGNED = 0,
    FLAG_CARRIER = 1,
    FLAG_ESCORT = 2,
    FLAG_HUNTER = 3,      // Hunt enemy FC
    NODE_ATTACKER = 4,
    NODE_DEFENDER = 5,
    ROAMER = 6,
    HEALER_OFFENSE = 7,
    HEALER_DEFENSE = 8,
    CART_PUSHER = 9,      // Silvershard Mines
    ORB_CARRIER = 10      // Temple of Kotmogu
};

enum class ObjectiveType : uint8 {
    FLAG = 1,
    NODE = 2,
    TOWER = 3,
    GRAVEYARD = 4,
    GATE = 5,
    CART = 6,
    ORB = 7,
    BOSS = 8
};

enum class ObjectiveState : uint8 {
    NEUTRAL = 0,
    ALLIANCE_CONTROLLED = 1,
    HORDE_CONTROLLED = 2,
    ALLIANCE_CONTESTED = 3,
    HORDE_CONTESTED = 4,
    ALLIANCE_CAPTURING = 5,
    HORDE_CAPTURING = 6
};

struct BGObjective {
    uint32 id;
    ObjectiveType type;
    ObjectiveState state;
    std::string name;
    float x, y, z;
    
    // Capture progress
    float captureProgress;  // 0.0 - 1.0
    uint32 captureTime;     // Time when capture will complete
    
    // Assignment
    std::vector<ObjectGuid> assignedDefenders;
    std::vector<ObjectGuid> assignedAttackers;
    
    // Strategic value
    uint8 strategicValue;   // 1-10
    bool isContested;
};

struct BGScoreInfo {
    uint32 allianceScore;
    uint32 hordeScore;
    uint32 maxScore;
    uint32 allianceResources;
    uint32 hordeResources;
    uint32 timeRemaining;
};

struct FlagInfo {
    ObjectGuid carrierGuid;
    bool isPickedUp;
    bool isAtBase;
    float x, y, z;
    uint8 stackCount;       // For debuffs
    uint32 pickupTime;
};

const char* BGStateToString(BGState state);
const char* BGRoleToString(BGRole role);
const char* BGTypeToString(BGType type);

} // namespace Playerbot
```

### 4.3.2: Create BattlegroundCoordinator.h

```cpp
#pragma once

#include "BGState.h"
#include "Core/Events/ICombatEventSubscriber.h"
#include <memory>
#include <vector>
#include <map>

class Battleground;
class Player;

namespace Playerbot {

class ObjectiveManager;
class BGRoleManager;
class FlagCarrierManager;
class NodeController;
class BGStrategyEngine;

class BattlegroundCoordinator : public ICombatEventSubscriber {
public:
    BattlegroundCoordinator(Battleground* bg, std::vector<Player*> bots);
    ~BattlegroundCoordinator();
    
    // Lifecycle
    void Initialize();
    void Shutdown();
    void Update(uint32 diff);
    
    // State
    BGState GetState() const { return _state; }
    BGType GetType() const { return _bgType; }
    bool IsActive() const { return _state == BGState::ACTIVE || _state == BGState::OVERTIME; }
    
    // Score
    const BGScoreInfo& GetScore() const { return _score; }
    bool IsWinning() const;
    float GetScoreAdvantage() const;
    
    // Objectives
    std::vector<BGObjective> GetObjectives() const;
    BGObjective* GetObjective(uint32 objectiveId);
    ObjectiveState GetObjectiveState(uint32 objectiveId) const;
    
    // Role management
    BGRole GetBotRole(ObjectGuid bot) const;
    void AssignRole(ObjectGuid bot, BGRole role);
    std::vector<ObjectGuid> GetBotsWithRole(BGRole role) const;
    
    // Flag management (CTF maps)
    bool HasFlag(ObjectGuid player) const;
    ObjectGuid GetFriendlyFC() const;
    ObjectGuid GetEnemyFC() const;
    const FlagInfo& GetFriendlyFlag() const;
    const FlagInfo& GetEnemyFlag() const;
    
    // Strategic commands
    void CommandAttack(uint32 objectiveId);
    void CommandDefend(uint32 objectiveId);
    void CommandRecall();
    void CommandPush();
    
    // Bot queries
    BGObjective* GetAssignment(ObjectGuid bot) const;
    float GetDistanceToAssignment(ObjectGuid bot) const;
    bool ShouldContestObjective(ObjectGuid bot) const;
    
    // ICombatEventSubscriber
    void OnCombatEvent(const CombatEvent& event) override;
    CombatEventType GetSubscribedEventTypes() const override;
    
private:
    // State
    BGState _state = BGState::IDLE;
    BGType _bgType;
    BGScoreInfo _score;
    
    // References
    Battleground* _battleground;
    std::vector<Player*> _bots;
    uint32 _faction;  // ALLIANCE or HORDE
    
    // Objectives
    std::vector<BGObjective> _objectives;
    
    // Sub-managers
    std::unique_ptr<ObjectiveManager> _objectiveManager;
    std::unique_ptr<BGRoleManager> _roleManager;
    std::unique_ptr<FlagCarrierManager> _flagManager;
    std::unique_ptr<NodeController> _nodeController;
    std::unique_ptr<BGStrategyEngine> _strategyEngine;
    
    // State machine
    void UpdateState(uint32 diff);
    void TransitionTo(BGState newState);
    
    // BG-specific initialization
    void InitializeWSG();
    void InitializeAB();
    void InitializeAV();
    void InitializeEOTS();
    void InitializeSOTA();
    void InitializeIOC();
    
    // Event handlers
    void HandleObjectiveCaptured(uint32 objectiveId, uint32 faction);
    void HandleFlagPickup(ObjectGuid player, bool isEnemyFlag);
    void HandleFlagDrop(ObjectGuid player);
    void HandleFlagCapture(ObjectGuid player);
    void HandlePlayerDied(ObjectGuid player);
    
    // Strategic decisions
    void EvaluateStrategy(uint32 diff);
    void ReassignRoles();
    void UpdateObjectivePriorities();
};

} // namespace Playerbot
```

### 4.3.3: Create BGStrategyEngine.h

```cpp
#pragma once

#include "BGState.h"
#include <vector>

namespace Playerbot {

class BattlegroundCoordinator;

enum class BGStrategy : uint8 {
    BALANCED = 0,       // Standard play
    AGGRESSIVE = 1,     // Focus offense
    DEFENSIVE = 2,      // Focus defense
    TURTLE = 3,         // Maximum defense
    ALL_IN = 4,         // Full offense
    STALL = 5           // Run out clock
};

struct StrategicDecision {
    BGStrategy strategy;
    std::vector<uint32> attackObjectives;
    std::vector<uint32> defendObjectives;
    uint8 offenseAllocation;   // Percent of bots on offense
    uint8 defenseAllocation;   // Percent of bots on defense
    std::string reasoning;
};

class BGStrategyEngine {
public:
    BGStrategyEngine(BattlegroundCoordinator* coordinator);
    
    void Update(uint32 diff);
    
    // Strategy
    BGStrategy GetCurrentStrategy() const { return _currentStrategy; }
    StrategicDecision GetCurrentDecision() const { return _currentDecision; }
    void ForceStrategy(BGStrategy strategy);
    
    // Evaluation
    StrategicDecision EvaluateBestStrategy() const;
    float ScoreStrategy(BGStrategy strategy) const;
    
    // Objective priority
    std::vector<uint32> GetAttackPriorities() const;
    std::vector<uint32> GetDefendPriorities() const;
    uint8 GetObjectivePriority(uint32 objectiveId) const;
    
    // Resource allocation
    uint8 GetOffensePercent() const;
    uint8 GetDefensePercent() const;
    uint8 GetRoamerCount() const;
    
    // Tactical queries
    bool ShouldContestNode(uint32 nodeId) const;
    bool ShouldAbandonNode(uint32 nodeId) const;
    bool ShouldRecall() const;
    bool ShouldPush() const;
    
private:
    BattlegroundCoordinator* _coordinator;
    
    BGStrategy _currentStrategy = BGStrategy::BALANCED;
    StrategicDecision _currentDecision;
    uint32 _lastEvaluationTime = 0;
    
    // Strategy evaluation
    float EvaluateBalanced() const;
    float EvaluateAggressive() const;
    float EvaluateDefensive() const;
    float EvaluateTurtle() const;
    float EvaluateAllIn() const;
    float EvaluateStall() const;
    
    // Factors
    float GetScoreFactor() const;      // How score affects strategy
    float GetTimeFactor() const;       // How time remaining affects strategy
    float GetMomentumFactor() const;   // Recent captures/losses
    float GetStrengthFactor() const;   // Team strength comparison
    
    // BG-specific strategies
    StrategicDecision EvaluateWSGStrategy() const;
    StrategicDecision EvaluateABStrategy() const;
    StrategicDecision EvaluateAVStrategy() const;
};

} // namespace Playerbot
```

---

## Implementation Order

| Week | Task | Deliverable |
|------|------|-------------|
| 1 | 4.1.1-4.1.3 | DungeonState, DungeonCoordinator, TrashPullManager |
| 2 | 4.1.4-4.1.6 | BossEncounterManager, WipeRecoveryManager, MythicPlusManager |
| 3 | 4.2.1-4.2.2 | ArenaState, ArenaCoordinator |
| 4 | 4.2.3-4.2.4 | KillTargetManager, CCChainManager |
| 5 | 4.3.1-4.3.2 | BGState, BattlegroundCoordinator |
| 6 | 4.3.3 + Integration | BGStrategyEngine, Integration testing |

---

## Summary

| Coordinator | Files | Key Features | Effort |
|-------------|-------|--------------|--------|
| DungeonCoordinator | 10 | Trash pulls, boss encounters, M+, wipe recovery | 80h |
| ArenaCoordinator | 12 | Kill targets, burst windows, CC chains, positioning | 80h |
| BattlegroundCoordinator | 12 | Objectives, roles, flags, strategic decisions | 80h |
| **Total** | 34 | | **240h** |

## Expected Outcomes

| Context | Before | After |
|---------|--------|-------|
| Dungeon explicit support | 0% | 100% |
| Arena coordination | 8% | 80% |
| BG coordination | 8% | 80% |
| M+ timer/affix support | 0% | 100% |
| CC chain coordination | 0% | 100% |
| BG strategic decisions | 0% | 100% |
