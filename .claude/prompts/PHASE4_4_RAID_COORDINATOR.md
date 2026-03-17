# Phase 4.4: RaidCoordinator - Detailed Implementation Plan

**Version**: 1.0  
**Date**: 2026-01-26  
**Prerequisites**: Phase 1-3 complete, Task 4.1 (shared components)  
**Duration**: ~100 hours (2.5 weeks)

---

## Overview

Raids are the most complex PvE content in WoW, requiring sophisticated coordination between 10-40 players. Unlike dungeons (1 tank, 1 healer, 3 DPS), raids require:

- **Multiple Tanks** (2-4) with swap mechanics, kiting, add pickup
- **Multiple Healers** (2-8) with healing assignments, cooldown rotation
- **Sub-Group Management** (8 groups of 5 players each)
- **Complex Encounter Mechanics** (splits, soaks, kiting, positioning)
- **Raid-Wide Cooldown Rotation** (Bloodlust, Rallying Cry, Spirit Link, etc.)

---

## Raid vs Dungeon Complexity

| Aspect | Dungeon | Raid |
|--------|---------|------|
| Group Size | 5 | 10-40 |
| Tanks | 1 | 2-4 |
| Healers | 1 | 2-8 |
| Sub-Groups | 1 | 2-8 |
| Tank Swaps | Rare | Common |
| Healing Assignments | Simple | Complex |
| Raid CDs | Few | Many (rotation required) |
| Split Mechanics | None | Common |
| Kiting | Simple | Complex (dedicated kiters) |
| Add Management | Kill order | Multi-target, off-tanks |

---

## Files to Create

```
src/modules/Playerbot/AI/Coordination/Raid/
├── RaidCoordinator.h
├── RaidCoordinator.cpp
├── RaidState.h
├── RaidGroupManager.h
├── RaidGroupManager.cpp
├── RaidAssignmentManager.h
├── RaidAssignmentManager.cpp
├── RaidTankCoordinator.h
├── RaidTankCoordinator.cpp
├── RaidHealCoordinator.h
├── RaidHealCoordinator.cpp
├── RaidCooldownRotation.h
├── RaidCooldownRotation.cpp
├── RaidEncounterManager.h
├── RaidEncounterManager.cpp
├── RaidPositioningManager.h
├── RaidPositioningManager.cpp
├── KitingManager.h
├── KitingManager.cpp
├── AddManagementSystem.h
├── AddManagementSystem.cpp
```

---

## 4.4.1: RaidState.h

```cpp
#pragma once

#include "ObjectGuid.h"
#include <cstdint>
#include <vector>
#include <map>
#include <string>

namespace Playerbot {

enum class RaidState : uint8 {
    IDLE = 0,
    FORMING = 1,          // Raid forming, invites going out
    READY_CHECK = 2,      // Ready check in progress
    BUFFING = 3,          // Pre-pull buffing
    PULLING = 4,          // Tank pulling boss
    COMBAT = 5,           // Active combat
    PHASE_TRANSITION = 6, // Boss phase transition
    INTERMISSION = 7,     // Boss intermission (untargetable)
    WIPED = 8,            // Raid wiped
    RECOVERING = 9,       // Rezzing, rebuffing
    LOOTING = 10,         // Post-kill looting
    TRASH_CLEAR = 11,     // Clearing trash
    COMPLETED = 12        // Raid complete
};

enum class RaidDifficulty : uint8 {
    LFR = 0,
    NORMAL = 1,
    HEROIC = 2,
    MYTHIC = 3
};

enum class RaidSize : uint8 {
    RAID_10 = 10,
    RAID_15 = 15,
    RAID_20 = 20,
    RAID_25 = 25,
    RAID_30 = 30,
    RAID_40 = 40,
    FLEX = 0  // Flexible sizing
};

enum class RaidRole : uint8 {
    UNASSIGNED = 0,
    MAIN_TANK = 1,
    OFF_TANK = 2,
    ADD_TANK = 3,
    KITER = 4,
    MAIN_HEALER = 5,
    RAID_HEALER = 6,
    TANK_HEALER = 7,
    DISPELLER = 8,
    MELEE_DPS = 9,
    RANGED_DPS = 10,
    INTERRUPTER = 11,
    SOAKER = 12,           // For soak mechanics
    SPLIT_GROUP_A = 13,    // For split mechanics
    SPLIT_GROUP_B = 14
};

enum class TankRole : uint8 {
    ACTIVE = 0,      // Currently tanking boss
    SWAP_READY = 1,  // Ready to taunt
    ADD_DUTY = 2,    // Handling adds
    KITING = 3,      // Kiting boss/add
    RECOVERING = 4   // Dropped stacks, recovering
};

enum class HealerAssignment : uint8 {
    RAID_HEALING = 0,
    TANK_1 = 1,
    TANK_2 = 2,
    TANK_3 = 3,
    GROUP_1 = 4,
    GROUP_2 = 5,
    GROUP_3 = 6,
    GROUP_4 = 7,
    GROUP_5 = 8,
    GROUP_6 = 9,
    GROUP_7 = 10,
    GROUP_8 = 11,
    SPOT_HEALING = 12,
    DISPEL_DUTY = 13
};

// Sub-group within raid
struct RaidSubGroup {
    uint8 groupNumber;  // 1-8
    std::vector<ObjectGuid> members;
    ObjectGuid groupLeader;
    
    // Composition
    uint8 tankCount = 0;
    uint8 healerCount = 0;
    uint8 dpsCount = 0;
    
    // Position assignment for spread mechanics
    float assignedPositionX = 0;
    float assignedPositionY = 0;
    float assignedPositionZ = 0;
    float spreadRadius = 0;
};

// Tank tracking
struct RaidTankInfo {
    ObjectGuid guid;
    TankRole role;
    uint8 debuffStacks = 0;       // Tank swap debuff stacks
    uint32 debuffExpireTime = 0;
    bool canTaunt = true;
    uint32 tauntCooldown = 0;
    ObjectGuid currentTarget;
    float threatOnBoss = 0;
    bool isKiting = false;
    std::vector<ObjectGuid> assignedAdds;
};

// Healer tracking
struct RaidHealerInfo {
    ObjectGuid guid;
    HealerAssignment assignment;
    float manaPercent;
    bool hasMajorCooldown;        // Tranq, Divine Hymn, etc.
    uint32 majorCooldownReady;
    bool hasExternalCooldown;     // Pain Supp, Sac, Ironbark
    uint32 externalCooldownReady;
    std::vector<ObjectGuid> assignedTargets;
};

// Raid-wide cooldown
struct RaidCooldown {
    uint32 spellId;
    std::string name;
    ObjectGuid owner;
    uint32 cooldownDuration;
    uint32 readyTime;
    bool isDefensive;      // Rallying Cry, Spirit Link
    bool isOffensive;      // Bloodlust, Power Infusion
    float effectRadius;    // 0 = single target
};

// Encounter-specific position
struct RaidPosition {
    std::string name;      // "Stack point", "Spread position 1"
    float x, y, z;
    float radius;          // Acceptable deviation
    std::vector<ObjectGuid> assignedPlayers;
    bool isRequired;       // Must be occupied
    uint8 minPlayers;
    uint8 maxPlayers;
};

// Kiting path
struct KitePath {
    uint32 pathId;
    std::string name;
    std::vector<std::tuple<float, float, float>> waypoints;
    bool isLoop;
    float recommendedSpeed;
    ObjectGuid assignedKiter;
    ObjectGuid kitedTarget;
};

// Add spawn
struct RaidAdd {
    ObjectGuid guid;
    uint32 creatureId;
    std::string name;
    uint8 priority;        // 1 = highest
    bool requiresTank;
    bool shouldBeKited;
    bool shouldBeInterrupted;
    bool shouldBeCCd;
    ObjectGuid assignedTank;
    ObjectGuid assignedInterrupter;
};

// Boss encounter data
struct RaidBossEncounter {
    uint32 bossId;
    uint32 encounterId;
    std::string name;
    RaidDifficulty difficulty;
    
    // Phase info
    uint8 currentPhase;
    uint8 totalPhases;
    std::vector<float> phaseTransitionHealth;
    
    // Timers
    bool hasEnrage;
    uint32 enrageTimeMs;
    bool hasBerserk;
    uint32 berserkTimeMs;
    
    // Tank requirements
    uint8 requiredTanks;
    bool requiresTankSwap;
    uint32 tankSwapSpellId;
    uint8 tankSwapStacks;
    uint32 tankSwapDebuffDuration;
    
    // Positioning
    std::vector<RaidPosition> positions;
    std::vector<KitePath> kitePaths;
    
    // Adds
    std::vector<std::pair<uint32, uint32>> addSpawnTimers;  // creatureId, time
    
    // Important spells
    std::vector<uint32> mustInterruptSpells;
    std::vector<uint32> mustDispelSpells;
    std::vector<uint32> raidDamageSpells;     // Use raid CDs
    std::vector<uint32> tankBusterSpells;     // Use tank CDs
};

const char* RaidStateToString(RaidState state);
const char* RaidRoleToString(RaidRole role);
const char* TankRoleToString(TankRole role);

} // namespace Playerbot
```

---

## 4.4.2: RaidCoordinator.h

```cpp
#pragma once

#include "RaidState.h"
#include "Core/Events/ICombatEventSubscriber.h"
#include <memory>
#include <vector>
#include <map>

class Group;
class Player;
class Creature;

namespace Playerbot {

class RaidGroupManager;
class RaidAssignmentManager;
class RaidTankCoordinator;
class RaidHealCoordinator;
class RaidCooldownRotation;
class RaidEncounterManager;
class RaidPositioningManager;
class KitingManager;
class AddManagementSystem;

class RaidCoordinator : public ICombatEventSubscriber {
public:
    RaidCoordinator(Group* raid);
    ~RaidCoordinator();
    
    // Lifecycle
    void Initialize();
    void Shutdown();
    void Update(uint32 diff);
    
    // State
    RaidState GetState() const { return _state; }
    void SetState(RaidState newState);
    bool IsInCombat() const;
    bool IsInEncounter() const;
    
    // Raid info
    RaidSize GetRaidSize() const;
    RaidDifficulty GetDifficulty() const;
    uint32 GetRaidMemberCount() const;
    
    // ===================
    // SUB-GROUP MANAGEMENT
    // ===================
    
    RaidSubGroup* GetSubGroup(uint8 groupNum);
    std::vector<RaidSubGroup>& GetAllSubGroups();
    void AssignToSubGroup(ObjectGuid player, uint8 groupNum);
    void OptimizeSubGroups();  // Balance tanks/healers across groups
    
    // Split mechanics
    void SplitRaid(uint8 numGroups);  // Split into N groups for mechanics
    void MergeRaid();                  // Merge back together
    std::vector<ObjectGuid> GetSplitGroup(uint8 groupId) const;
    
    // ===================
    // TANK COORDINATION
    // ===================
    
    // Tank management
    std::vector<RaidTankInfo> GetTanks() const;
    ObjectGuid GetMainTank() const;
    ObjectGuid GetOffTank() const;
    ObjectGuid GetActiveTank() const;  // Currently tanking boss
    
    // Tank swaps
    bool NeedsTankSwap() const;
    void CallTankSwap();
    void OnTankSwapCompleted(ObjectGuid newTank);
    uint8 GetTankDebuffStacks(ObjectGuid tank) const;
    
    // Tank assignments
    void AssignTankToBoss(ObjectGuid tank);
    void AssignTankToAdds(ObjectGuid tank, std::vector<ObjectGuid> adds);
    void AssignKiter(ObjectGuid kiter, ObjectGuid target);
    
    // ===================
    // HEALER COORDINATION
    // ===================
    
    std::vector<RaidHealerInfo> GetHealers() const;
    void AssignHealerToTank(ObjectGuid healer, ObjectGuid tank);
    void AssignHealerToGroup(ObjectGuid healer, uint8 groupNum);
    void AssignRaidHealer(ObjectGuid healer);
    
    // Healer cooldowns
    void RequestHealerCooldown(ObjectGuid target);
    void RequestExternalCooldown(ObjectGuid tank);
    ObjectGuid GetNextExternalCooldownOwner() const;
    
    // ===================
    // RAID COOLDOWNS
    // ===================
    
    // Offensive CDs
    void CallBloodlust();
    bool IsBloodlustActive() const;
    bool ShouldUseBloodlust() const;
    
    // Defensive CDs
    void CallRaidCooldown();
    void CallStackAndCooldown();  // Stack + raid CD combo
    std::vector<RaidCooldown> GetAvailableRaidCooldowns() const;
    RaidCooldown* GetNextRaidCooldown() const;
    
    // ===================
    // POSITIONING
    // ===================
    
    void AssignPosition(ObjectGuid player, const RaidPosition& pos);
    void CallSpread(float distance);
    void CallStack(float x, float y, float z);
    void CallMoveToPosition(const std::string& positionName);
    RaidPosition* GetAssignedPosition(ObjectGuid player) const;
    
    // ===================
    // KITING
    // ===================
    
    void StartKiting(ObjectGuid kiter, ObjectGuid target, uint32 pathId);
    void StopKiting(ObjectGuid kiter);
    bool IsKitingActive() const;
    ObjectGuid GetKiter() const;
    KitePath* GetKitePath(uint32 pathId);
    
    // ===================
    // ADD MANAGEMENT
    // ===================
    
    void OnAddSpawned(Creature* add);
    void OnAddDied(ObjectGuid addGuid);
    void AssignAddTank(ObjectGuid add, ObjectGuid tank);
    void AssignAddInterrupter(ObjectGuid add, ObjectGuid interrupter);
    void SetAddPriority(ObjectGuid add, uint8 priority);
    std::vector<RaidAdd> GetActiveAdds() const;
    ObjectGuid GetHighestPriorityAdd() const;
    
    // ===================
    // TARGET SELECTION
    // ===================
    
    // Primary target (boss)
    ObjectGuid GetBossTarget() const;
    void SetBossTarget(ObjectGuid boss);
    
    // Priority targets (adds, priority switches)
    void CallTargetSwitch(ObjectGuid target);
    void AddPriorityTarget(ObjectGuid target, uint8 priority);
    void RemovePriorityTarget(ObjectGuid target);
    std::vector<std::pair<ObjectGuid, uint8>> GetPriorityTargets() const;
    
    // DPS assignments
    void AssignDPSToTarget(ObjectGuid dps, ObjectGuid target);
    void AssignRangedToTarget(ObjectGuid target);  // All ranged switch
    void AssignMeleeToTarget(ObjectGuid target);   // All melee switch
    
    // ===================
    // ENCOUNTER
    // ===================
    
    const RaidBossEncounter* GetCurrentEncounter() const;
    void OnEncounterStart(uint32 encounterId);
    void OnEncounterEnd(bool success);
    void OnPhaseChange(uint8 newPhase);
    uint8 GetCurrentPhase() const;
    uint32 GetEncounterDuration() const;
    float GetBossHealthPercent() const;
    
    // ===================
    // WIPE RECOVERY
    // ===================
    
    void OnRaidWipe();
    void StartRecovery();
    ObjectGuid GetNextRezTarget() const;
    void OnPlayerRezzed(ObjectGuid player);
    bool IsRecoveryComplete() const;
    
    // ===================
    // EVENTS
    // ===================
    
    void OnCombatEvent(const CombatEvent& event) override;
    CombatEventType GetSubscribedEventTypes() const override;
    
private:
    // State
    RaidState _state = RaidState::IDLE;
    Group* _raid;
    
    // Sub-managers
    std::unique_ptr<RaidGroupManager> _groupManager;
    std::unique_ptr<RaidAssignmentManager> _assignmentManager;
    std::unique_ptr<RaidTankCoordinator> _tankCoordinator;
    std::unique_ptr<RaidHealCoordinator> _healCoordinator;
    std::unique_ptr<RaidCooldownRotation> _cooldownRotation;
    std::unique_ptr<RaidEncounterManager> _encounterManager;
    std::unique_ptr<RaidPositioningManager> _positioningManager;
    std::unique_ptr<KitingManager> _kitingManager;
    std::unique_ptr<AddManagementSystem> _addManager;
    
    // Current encounter
    RaidBossEncounter* _currentEncounter = nullptr;
    ObjectGuid _bossGuid;
    
    // State machine
    void UpdateState(uint32 diff);
    void TransitionTo(RaidState newState);
    void OnStateEnter(RaidState state);
    void OnStateExit(RaidState state);
    
    // Event handlers
    void HandleDamageTaken(const CombatEvent& event);
    void HandleAuraApplied(const CombatEvent& event);
    void HandleSpellCastStart(const CombatEvent& event);
    void HandleUnitDied(const CombatEvent& event);
    void HandleEncounterStateChanged(const CombatEvent& event);
};

} // namespace Playerbot
```

---

## 4.4.3: RaidTankCoordinator.h

```cpp
#pragma once

#include "RaidState.h"
#include <vector>
#include <map>

namespace Playerbot {

class RaidCoordinator;

struct TankSwapTrigger {
    uint32 spellId;           // Debuff spell that triggers swap
    uint8 stackThreshold;     // Swap at this many stacks
    uint32 debuffDuration;    // How long debuff lasts
    bool swapOnApplication;   // Swap immediately on application
};

class RaidTankCoordinator {
public:
    RaidTankCoordinator(RaidCoordinator* coordinator);
    
    void Initialize();
    void Update(uint32 diff);
    
    // Tank registration
    void RegisterTank(ObjectGuid tank, TankRole initialRole);
    void UnregisterTank(ObjectGuid tank);
    std::vector<RaidTankInfo>& GetTanks() { return _tanks; }
    
    // Role assignment
    void SetTankRole(ObjectGuid tank, TankRole role);
    TankRole GetTankRole(ObjectGuid tank) const;
    ObjectGuid GetTankWithRole(TankRole role) const;
    
    // Active tank management
    ObjectGuid GetActiveTank() const { return _activeTank; }
    void SetActiveTank(ObjectGuid tank);
    
    // Tank swap logic
    void ConfigureSwapTrigger(const TankSwapTrigger& trigger);
    bool NeedsTankSwap() const;
    void CallTankSwap();
    void ExecuteTankSwap();
    void OnTauntUsed(ObjectGuid tank, ObjectGuid target);
    void OnSwapDebuffApplied(ObjectGuid tank, uint32 spellId, uint8 stacks);
    void OnSwapDebuffRemoved(ObjectGuid tank, uint32 spellId);
    
    // Debuff tracking
    uint8 GetDebuffStacks(ObjectGuid tank) const;
    uint32 GetDebuffTimeRemaining(ObjectGuid tank) const;
    bool CanTankSafely(ObjectGuid tank) const;
    
    // Threat management
    void UpdateThreat(ObjectGuid tank, ObjectGuid target, float threat);
    float GetThreat(ObjectGuid tank, ObjectGuid target) const;
    ObjectGuid GetHighestThreatTank(ObjectGuid target) const;
    bool IsTankSecondThreat(ObjectGuid tank, ObjectGuid target) const;
    
    // Tank cooldowns
    void OnTankCooldownUsed(ObjectGuid tank, uint32 spellId);
    bool HasTankCooldownAvailable(ObjectGuid tank) const;
    void RequestTankCooldown(ObjectGuid tank);
    
    // Add tanking
    void AssignAddToTank(ObjectGuid add, ObjectGuid tank);
    void UnassignAdd(ObjectGuid add);
    std::vector<ObjectGuid> GetAssignedAdds(ObjectGuid tank) const;
    ObjectGuid GetTankForAdd(ObjectGuid add) const;
    
    // Positioning
    void SetTankPosition(ObjectGuid tank, float x, float y, float z, float facing);
    bool IsTankInPosition(ObjectGuid tank) const;
    
private:
    RaidCoordinator* _coordinator;
    
    std::vector<RaidTankInfo> _tanks;
    ObjectGuid _activeTank;
    ObjectGuid _nextTank;  // Tank to swap to
    
    TankSwapTrigger _swapTrigger;
    bool _swapPending = false;
    uint32 _lastSwapTime = 0;
    
    // Threat tracking per target
    std::map<ObjectGuid, std::map<ObjectGuid, float>> _threatTable;  // target -> tank -> threat
    
    // Add assignments
    std::map<ObjectGuid, ObjectGuid> _addAssignments;  // add -> tank
    
    void UpdateDebuffs(uint32 diff);
    void CheckSwapConditions();
    ObjectGuid FindBestSwapTarget() const;
};

} // namespace Playerbot
```

---

## 4.4.4: RaidHealCoordinator.h

```cpp
#pragma once

#include "RaidState.h"
#include <vector>
#include <queue>

namespace Playerbot {

class RaidCoordinator;

struct HealingAssignment {
    ObjectGuid healer;
    HealerAssignment type;
    std::vector<ObjectGuid> targets;  // Tank GUIDs or empty for raid healing
    uint8 priority;  // Lower = higher priority
};

struct ExternalCooldownRequest {
    ObjectGuid target;
    uint32 requestTime;
    uint8 urgency;  // 1-10
    bool fulfilled = false;
};

class RaidHealCoordinator {
public:
    RaidHealCoordinator(RaidCoordinator* coordinator);
    
    void Initialize();
    void Update(uint32 diff);
    
    // Healer registration
    void RegisterHealer(ObjectGuid healer);
    void UnregisterHealer(ObjectGuid healer);
    std::vector<RaidHealerInfo>& GetHealers() { return _healers; }
    
    // Assignments
    void AssignHealerToTank(ObjectGuid healer, ObjectGuid tank);
    void AssignHealerToGroup(ObjectGuid healer, uint8 groupNum);
    void AssignRaidHealer(ObjectGuid healer);
    void AssignDispeller(ObjectGuid healer);
    void ClearAssignment(ObjectGuid healer);
    
    HealerAssignment GetAssignment(ObjectGuid healer) const;
    std::vector<ObjectGuid> GetHealersForTarget(ObjectGuid target) const;
    std::vector<ObjectGuid> GetHealersForGroup(uint8 groupNum) const;
    
    // Auto-assignment
    void AutoAssignHealers();  // Smart assignment based on comp
    void OptimizeAssignments();  // Rebalance if needed
    
    // Cooldown management
    void RegisterMajorCooldown(ObjectGuid healer, uint32 spellId, uint32 cooldown);
    void RegisterExternalCooldown(ObjectGuid healer, uint32 spellId, uint32 cooldown);
    
    bool HasMajorCooldownAvailable(ObjectGuid healer) const;
    bool HasExternalCooldownAvailable(ObjectGuid healer) const;
    
    void OnCooldownUsed(ObjectGuid healer, uint32 spellId);
    uint32 GetCooldownReadyTime(ObjectGuid healer, uint32 spellId) const;
    
    // External cooldown coordination
    void RequestExternalCooldown(ObjectGuid target, uint8 urgency = 5);
    ObjectGuid GetNextExternalProvider() const;
    void UseExternalCooldown(ObjectGuid healer, ObjectGuid target);
    void CancelExternalRequest(ObjectGuid target);
    
    // Mana tracking
    void UpdateManaStatus(ObjectGuid healer, float manaPercent);
    float GetAverageMana() const;
    float GetLowestMana() const;
    bool ShouldCallManaBreak() const;
    std::vector<ObjectGuid> GetLowManaHealers(float threshold = 30.0f) const;
    
    // Healing metrics
    void OnHealingDone(ObjectGuid healer, ObjectGuid target, uint32 amount);
    uint32 GetHealingDone(ObjectGuid healer) const;
    uint32 GetOverhealing(ObjectGuid healer) const;
    
private:
    RaidCoordinator* _coordinator;
    
    std::vector<RaidHealerInfo> _healers;
    std::vector<HealingAssignment> _assignments;
    
    // External cooldown queue
    std::priority_queue<ExternalCooldownRequest, 
                        std::vector<ExternalCooldownRequest>,
                        std::function<bool(const ExternalCooldownRequest&, const ExternalCooldownRequest&)>> _externalQueue;
    
    // Cooldown tracking
    std::map<ObjectGuid, std::map<uint32, uint32>> _cooldowns;  // healer -> spell -> readyTime
    
    // Metrics
    std::map<ObjectGuid, uint32> _healingDone;
    std::map<ObjectGuid, uint32> _overhealing;
    
    void UpdateCooldowns(uint32 diff);
    void ProcessExternalQueue();
    uint8 CalculateHealerScore(ObjectGuid healer, HealerAssignment type) const;
};

} // namespace Playerbot
```

---

## 4.4.5: RaidCooldownRotation.h

```cpp
#pragma once

#include "RaidState.h"
#include <vector>
#include <queue>

namespace Playerbot {

class RaidCoordinator;

enum class CooldownCategory : uint8 {
    BLOODLUST = 0,        // Bloodlust/Heroism/Time Warp
    RAID_DEFENSIVE = 1,   // Rallying Cry, Spirit Link, Darkness
    EXTERNAL = 2,         // Pain Supp, Ironbark, Blessing of Sac
    RAID_HEALING = 3,     // Tranq, Divine Hymn, Revival
    MOVEMENT = 4,         // Stampeding Roar, Wind Rush
    BATTLE_REZ = 5        // Combat rez
};

struct CooldownSlot {
    uint32 triggerTime;      // When to use (relative to pull or phase)
    uint32 spellId;          // 0 = any from category
    CooldownCategory category;
    ObjectGuid preferredOwner;  // Empty = anyone with CD
    std::string note;        // "Use on Decimation"
};

struct CooldownPlan {
    uint32 encounterId;
    uint8 phase;
    std::vector<CooldownSlot> slots;
};

class RaidCooldownRotation {
public:
    RaidCooldownRotation(RaidCoordinator* coordinator);
    
    void Initialize();
    void Update(uint32 diff);
    
    // Cooldown registration
    void RegisterCooldown(const RaidCooldown& cd);
    void UnregisterCooldown(ObjectGuid owner, uint32 spellId);
    void OnCooldownUsed(ObjectGuid owner, uint32 spellId);
    void OnCooldownReady(ObjectGuid owner, uint32 spellId);
    
    // Available cooldowns
    std::vector<RaidCooldown> GetAvailableCooldowns() const;
    std::vector<RaidCooldown> GetAvailableCooldowns(CooldownCategory category) const;
    bool HasCooldownAvailable(CooldownCategory category) const;
    uint32 GetCooldownCount(CooldownCategory category) const;
    
    // Cooldown plan
    void LoadPlan(const CooldownPlan& plan);
    void ClearPlan();
    void SetPhase(uint8 phase);
    CooldownSlot* GetNextScheduledCooldown() const;
    
    // Execution
    void CallCooldown(CooldownCategory category);
    void CallBloodlust();
    void CallRaidDefensive();
    void CallMovementSpeed();
    void CallBattleRez(ObjectGuid target);
    
    ObjectGuid GetNextCooldownOwner(CooldownCategory category) const;
    void AssignCooldownToSlot(ObjectGuid owner, uint32 slotIndex);
    
    // Bloodlust logic
    bool IsBloodlustActive() const { return _bloodlustActive; }
    bool CanUseBloodlust() const;
    bool ShouldUseBloodlust() const;  // Based on boss health, phase, etc.
    void OnBloodlustApplied();
    void OnBloodlustExpired();
    uint32 GetBloodlustReadyTime() const;
    
    // Battle rez
    uint32 GetBattleRezCharges() const { return _battleRezCharges; }
    bool CanBattleRez() const { return _battleRezCharges > 0; }
    void OnBattleRezUsed();
    void OnCombatStart();  // Reset charges
    
private:
    RaidCoordinator* _coordinator;
    
    // All registered cooldowns
    std::vector<RaidCooldown> _cooldowns;
    
    // Current plan
    CooldownPlan _activePlan;
    uint32 _currentSlotIndex = 0;
    uint8 _currentPhase = 0;
    
    // Bloodlust state
    bool _bloodlustActive = false;
    bool _bloodlustUsed = false;
    uint32 _bloodlustExpireTime = 0;
    
    // Battle rez
    uint32 _battleRezCharges = 0;
    uint32 _maxBattleRezCharges = 1;  // Based on raid size
    
    // Rotation queue per category
    std::map<CooldownCategory, std::queue<ObjectGuid>> _rotationQueues;
    
    void BuildRotationQueues();
    void AdvanceRotation(CooldownCategory category);
    uint32 CalculateBattleRezCharges() const;
};

} // namespace Playerbot
```

---

## 4.4.6: KitingManager.h

```cpp
#pragma once

#include "RaidState.h"
#include <vector>
#include <map>

namespace Playerbot {

class RaidCoordinator;

enum class KiteState : uint8 {
    INACTIVE = 0,
    STARTING = 1,
    KITING = 2,
    PAUSING = 3,    // Temporary stop for mechanics
    RESETTING = 4,  // Kiter died or needs swap
    COMPLETED = 5
};

struct KiteAssignment {
    ObjectGuid kiter;
    ObjectGuid target;
    uint32 pathId;
    KiteState state;
    uint32 startTime;
    uint32 currentWaypointIndex;
    float distanceToTarget;
    bool needsSwap;
};

class KitingManager {
public:
    KitingManager(RaidCoordinator* coordinator);
    
    void Initialize();
    void Update(uint32 diff);
    
    // Path management
    void RegisterPath(const KitePath& path);
    void UnregisterPath(uint32 pathId);
    KitePath* GetPath(uint32 pathId);
    std::vector<KitePath>& GetAllPaths() { return _paths; }
    
    // Kite assignments
    void AssignKiter(ObjectGuid kiter, ObjectGuid target, uint32 pathId);
    void UnassignKiter(ObjectGuid kiter);
    void SwapKiter(ObjectGuid oldKiter, ObjectGuid newKiter);
    
    KiteAssignment* GetAssignment(ObjectGuid kiter);
    KiteAssignment* GetAssignmentForTarget(ObjectGuid target);
    bool IsBeingKited(ObjectGuid target) const;
    
    // Kite control
    void StartKiting(ObjectGuid kiter);
    void StopKiting(ObjectGuid kiter);
    void PauseKiting(ObjectGuid kiter);
    void ResumeKiting(ObjectGuid kiter);
    
    // State queries
    KiteState GetKiteState(ObjectGuid kiter) const;
    bool IsKitingActive() const;
    std::vector<ObjectGuid> GetActiveKiters() const;
    
    // Kiter selection
    ObjectGuid SelectBestKiter(ObjectGuid target) const;
    float CalculateKiterScore(ObjectGuid player, ObjectGuid target) const;
    
    // Distance management
    float GetRecommendedDistance(ObjectGuid kiter) const;
    bool IsTooClose(ObjectGuid kiter) const;
    bool IsTooFar(ObjectGuid kiter) const;
    void AdjustKiteSpeed(ObjectGuid kiter, float modifier);
    
    // Path following
    std::tuple<float, float, float> GetNextWaypoint(ObjectGuid kiter) const;
    std::tuple<float, float, float> GetCurrentPosition(ObjectGuid kiter) const;
    float GetProgressAlongPath(ObjectGuid kiter) const;  // 0.0 - 1.0
    
    // Emergency handling
    void OnKiterDied(ObjectGuid kiter);
    void OnKiterCC(ObjectGuid kiter);
    void RequestEmergencySwap(ObjectGuid kiter);
    
private:
    RaidCoordinator* _coordinator;
    
    std::vector<KitePath> _paths;
    std::map<ObjectGuid, KiteAssignment> _assignments;  // kiter -> assignment
    
    void UpdateKiteAssignment(KiteAssignment& assignment, uint32 diff);
    void AdvanceWaypoint(KiteAssignment& assignment);
    bool HasReachedWaypoint(const KiteAssignment& assignment) const;
    ObjectGuid FindBackupKiter(ObjectGuid target) const;
};

} // namespace Playerbot
```

---

## 4.4.7: AddManagementSystem.h

```cpp
#pragma once

#include "RaidState.h"
#include <vector>
#include <map>
#include <set>

namespace Playerbot {

class RaidCoordinator;

enum class AddState : uint8 {
    SPAWNING = 0,
    ACTIVE = 1,
    TANKED = 2,
    KITED = 3,
    CCD = 4,
    DYING = 5,
    DEAD = 6
};

struct AddInfo {
    ObjectGuid guid;
    uint32 creatureId;
    AddState state;
    
    // Priority
    uint8 priority;           // 1 = highest
    bool isMustKill;          // Must die for encounter progress
    bool isHighThreat;        // Dangerous to ignore
    
    // Handling
    bool requiresTank;
    bool shouldBeKited;
    bool shouldBeInterrupted;
    bool shouldBeCCd;
    
    // Assignments
    ObjectGuid assignedTank;
    ObjectGuid assignedKiter;
    ObjectGuid assignedInterrupter;
    std::set<ObjectGuid> assignedDPS;
    
    // Health
    float healthPercent;
    uint32 estimatedTTD;      // Time to death in ms
    
    // Position
    float x, y, z;
    float distanceToRaid;
};

class AddManagementSystem {
public:
    AddManagementSystem(RaidCoordinator* coordinator);
    
    void Initialize();
    void Update(uint32 diff);
    
    // Add tracking
    void OnAddSpawned(ObjectGuid guid, uint32 creatureId);
    void OnAddDied(ObjectGuid guid);
    void UpdateAddHealth(ObjectGuid guid, float healthPercent);
    void UpdateAddPosition(ObjectGuid guid, float x, float y, float z);
    
    AddInfo* GetAdd(ObjectGuid guid);
    std::vector<AddInfo>& GetAllAdds() { return _adds; }
    std::vector<AddInfo> GetActiveAdds() const;
    std::vector<AddInfo> GetAddsByPriority() const;
    
    // Priority management
    void SetAddPriority(ObjectGuid guid, uint8 priority);
    void SetAddMustKill(ObjectGuid guid, bool mustKill);
    void SetAddHighThreat(ObjectGuid guid, bool highThreat);
    ObjectGuid GetHighestPriorityAdd() const;
    std::vector<ObjectGuid> GetPriorityTargets(uint8 maxPriority) const;
    
    // Handling configuration
    void ConfigureAddHandling(uint32 creatureId, bool tank, bool kite, bool interrupt, bool cc);
    void SetDefaultHandling(ObjectGuid guid);
    
    // Assignments
    void AssignTankToAdd(ObjectGuid add, ObjectGuid tank);
    void AssignKiterToAdd(ObjectGuid add, ObjectGuid kiter);
    void AssignInterrupterToAdd(ObjectGuid add, ObjectGuid interrupter);
    void AssignDPSToAdd(ObjectGuid add, ObjectGuid dps);
    void UnassignFromAdd(ObjectGuid add, ObjectGuid player);
    void ClearAddAssignments(ObjectGuid add);
    
    std::vector<ObjectGuid> GetPlayerAssignments(ObjectGuid player) const;
    ObjectGuid GetTankForAdd(ObjectGuid add) const;
    
    // Target calls
    void CallSwitchToAdd(ObjectGuid add);
    void CallSwitchToHighestPriority();
    void CallAOE();  // All adds
    void CallCleave();  // Boss + adds
    void CallFocusBoss();  // Ignore adds
    
    // DPS distribution
    void DistributeDPSToAdds();  // Auto-assign DPS across adds
    void FocusAllDPSOnAdd(ObjectGuid add);
    uint8 GetDPSCountOnAdd(ObjectGuid add) const;
    
    // Wave management (for multi-wave encounters)
    void OnWaveStart(uint8 waveNumber);
    void OnWaveEnd(uint8 waveNumber);
    uint8 GetCurrentWave() const { return _currentWave; }
    bool IsWaveCleared() const;
    
private:
    RaidCoordinator* _coordinator;
    
    std::vector<AddInfo> _adds;
    std::map<uint32, AddInfo> _addTemplates;  // creatureId -> template config
    
    uint8 _currentWave = 0;
    uint32 _waveStartTime = 0;
    
    void UpdateAddStates(uint32 diff);
    void AutoAssignUnhandledAdds();
    AddState DetermineAddState(const AddInfo& add) const;
    void CleanupDeadAdds();
    float CalculateAddPriorityScore(const AddInfo& add) const;
};

} // namespace Playerbot
```

---

## Implementation Summary

| Component | Files | Key Features |
|-----------|-------|--------------|
| **RaidState.h** | 1 | All enums, structs for raid coordination |
| **RaidCoordinator** | 2 | Main coordinator, state machine, sub-manager orchestration |
| **RaidGroupManager** | 2 | 8 sub-groups, split mechanics |
| **RaidAssignmentManager** | 2 | Role assignments, auto-optimization |
| **RaidTankCoordinator** | 2 | Tank swaps, debuff tracking, threat management |
| **RaidHealCoordinator** | 2 | Healer assignments, external CDs, mana tracking |
| **RaidCooldownRotation** | 2 | Bloodlust, raid CDs, battle rez, cooldown plans |
| **RaidEncounterManager** | 2 | Boss mechanics, phases, timers |
| **RaidPositioningManager** | 2 | Spread/stack, position assignments |
| **KitingManager** | 2 | Kite paths, kiter assignments, distance management |
| **AddManagementSystem** | 2 | Add priorities, wave management, DPS distribution |
| **Total** | **21 files** | |

---

## Expected Outcomes

| Feature | Status |
|---------|--------|
| Multiple tank coordination | ✅ |
| Tank swap automation | ✅ |
| Healer assignments | ✅ |
| External cooldown rotation | ✅ |
| Raid cooldown rotation | ✅ |
| Sub-group management | ✅ |
| Split mechanics | ✅ |
| Kiting support | ✅ |
| Add management | ✅ |
| Multi-target switching | ✅ |
| Phase-aware tactics | ✅ |
| Position assignments | ✅ |
