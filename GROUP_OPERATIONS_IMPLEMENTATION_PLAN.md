# Group-Level Operations - Implementation Plan

## Overview

This document details the complete implementation strategy for the remaining **11 TODO markers** 
from the Phase 7 legacy singleton call migration that require group-level coordination.

---

## ✅ COMPLETED: LFG Bot Selection (3/14 TODOs)

**Status**: ✅ Implemented and Committed (ddff3a92)  
**Solution**: Static utility methods with human player filtering  
**Impact**: Human players get instant LFG queue fills with proper role bots  

**Files Modified**:
- `LFGBotSelector.h` - Added static FindAvailableTanks/Healers/DPS methods
- `LFGBotSelector.cpp` - Implemented with group filtering logic
- `LFGBotManager.cpp` - Updated to use static methods

---

## 📋 REMAINING: 11 TODOs in 2 Categories

| Category | Count | Complexity | Priority |
|----------|-------|------------|----------|
| **Loot Distribution** | 4 | Medium | HIGH |
| **Instance Coordination** | 7 | High | CRITICAL |

---

## 1. Loot Distribution Group Methods (4 TODOs)

### Problem Statement

Four methods in `UnifiedLootManager::DistributionModule` coordinate group-wide loot but `LootDistribution` is now per-bot:

```cpp
void DistributeLoot(Group* group, LootItem const& item);
void ExecuteLootDistribution(Group* group, uint32 rollId);
void ResolveRollTies(Group* group, uint32 rollId);
void HandleLootNinja(Group* group, uint32 suspectedPlayer);
```

### WoW Loot Methods That Must Work

| Loot Method | Description | Bot Coordination Needed |
|-------------|-------------|------------------------|
| **Personal Loot** | Each player gets own loot | ✅ None (per-bot only) |
| **Group Loot** | Need/Greed/Pass rolling | ✅ Roll aggregation |
| **Master Loot** | Leader assigns loot | ✅ Leader decision with bot input |
| **Need Before Greed** | Need > Greed > Pass priority | ✅ Roll aggregation |
| **Round Robin** | Sequential assignment | ✅ Game handles (minimal bot logic) |
| **Free-for-All** | First to loot wins | ✅ Game handles (minimal bot logic) |

### Implementation Strategy: Hybrid Evaluation

**Principle**: Each bot evaluates items independently, group coordinator aggregates decisions

#### File: `UnifiedLootManager.cpp`

**Method 1: DistributeLoot** (Group Loot / Need Before Greed)
```cpp
void UnifiedLootManager::DistributionModule::DistributeLoot(Group* group, LootItem const& item)
{
    if (!group)
        return;

    LootMethod method = group->GetLootMethod();
    
    switch (method)
    {
        case LOOT_METHOD_MASTER_LOOT:
            HandleMasterLoot(group, item);
            break;
        case LOOT_METHOD_GROUP_LOOT:
        case LOOT_METHOD_NEED_BEFORE_GREED:
            HandleGroupLoot(group, item);
            break;
        default:
            // Personal/Free-for-all/Round-robin handled by game
            break;
    }
    
    _itemsDistributed++;
}

private:
void HandleMasterLoot(Group* group, LootItem const& item)
{
    Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
    if (!leader)
        return;
    
    // Collect upgrade evaluations from all bot members
    struct BotEvaluation {
        Player* bot;
        float upgradeValue;
        LootPriority priority;
    };
    
    std::vector<BotEvaluation> evaluations;
    
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member->IsRealPlayer())  // Skip humans
            continue;
        
        if (IGameSystemsManager* sys = GetGameSystems(member))
        {
            auto* lootDist = sys->GetLootDistribution();
            BotEvaluation eval;
            eval.bot = member;
            eval.upgradeValue = lootDist->CalculateUpgradeValue(item);
            eval.priority = lootDist->CalculateLootPriority(item);
            evaluations.push_back(eval);
        }
    }
    
    // Sort by priority then upgrade value
    std::sort(evaluations.begin(), evaluations.end(),
        [](const BotEvaluation& a, const BotEvaluation& b) {
            if (a.priority != b.priority)
                return a.priority > b.priority;
            return a.upgradeValue > b.upgradeValue;
        });
    
    // Award to highest priority bot
    if (!evaluations.empty())
    {
        Player* winner = evaluations[0].bot;
        TC_LOG_INFO("playerbot.loot", "Master Loot: {} awarded to {} (upgrade: {:.1f}%)",
            item.itemId, winner->GetName(), evaluations[0].upgradeValue);
        // Game will handle actual loot award
    }
}

void HandleGroupLoot(Group* group, LootItem const& item)
{
    // Each bot decides Need/Greed/Pass independently
    struct BotRoll {
        Player* bot;
        LootRollType rollType;
        uint32 rollValue;  // 1-100
    };
    
    std::vector<BotRoll> rolls;
    
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member->IsRealPlayer())
            continue;
        
        if (IGameSystemsManager* sys = GetGameSystems(member))
        {
            BotRoll roll;
            roll.bot = member;
            roll.rollType = sys->GetLootDistribution()->DetermineLootDecision(
                item, LootDecisionStrategy::NEED_BEFORE_GREED);
            roll.rollValue = (roll.rollType != LootRollType::PASS) ? urand(1, 100) : 0;
            rolls.push_back(roll);
            
            TC_LOG_DEBUG("playerbot.loot", "Bot {} rolled {} ({})",
                member->GetName(), roll.rollValue,
                roll.rollType == LootRollType::NEED ? "NEED" : 
                roll.rollType == LootRollType::GREED ? "GREED" : "PASS");
        }
    }
    
    // Determine winner: Need > Greed, highest roll wins
    DetermineGroupLootWinner(rolls, item);
}

void DetermineGroupLootWinner(std::vector<BotRoll>& rolls, LootItem const& item)
{
    // Separate by roll type
    std::vector<BotRoll> needRolls, greedRolls;
    for (const auto& roll : rolls)
    {
        if (roll.rollType == LootRollType::NEED)
            needRolls.push_back(roll);
        else if (roll.rollType == LootRollType::GREED)
            greedRolls.push_back(roll);
    }
    
    // Need rolls win
    auto* category = !needRolls.empty() ? &needRolls : 
                     !greedRolls.empty() ? &greedRolls : nullptr;
    
    if (!category)
        return;  // Everyone passed
    
    // Highest roll wins
    auto winner = std::max_element(category->begin(), category->end(),
        [](const BotRoll& a, const BotRoll& b) {
            return a.rollValue < b.rollValue;
        });
    
    if (winner != category->end())
    {
        TC_LOG_INFO("playerbot.loot", "Loot Winner: {} rolled {} ({})",
            winner->bot->GetName(), winner->rollValue,
            winner->rollType == LootRollType::NEED ? "NEED" : "GREED");
    }
}
```

**Method 2-4**: ExecuteLootDistribution, ResolveRollTies, HandleLootNinja  
These follow similar patterns using group iteration + per-bot evaluation.

### Testing Requirements

- ✅ Master Loot: Verify highest upgrade bot receives item
- ✅ Group Loot: Verify Need > Greed > Pass priority
- ✅ Roll Ties: Verify proper tiebreaker logic
- ✅ Human Players: Verify bots don't interfere with human loot decisions

---

## 2. Instance Coordination (7 TODOs) - CRITICAL

### Problem Statement

Seven methods in `DungeonBehavior.cpp` coordinate dungeon/raid group mechanics:

```cpp
void InitializeInstanceCoordination(Group* group, InstanceScript* instance);
void UpdateInstanceCoordination(Group* group, uint32 diff);
void HandleInstanceCompletion(Group* group);
void HandleInstanceFailure(Group* group);
void PrepareForEncounter(Group* group, uint32 encounterId);
void MonitorEncounterProgress(Group* group, uint32 encounterId);
void HandleEncounterRecovery(Group* group, uint32 encounterId);
```

**Critical Raid/Mythic+ Mechanics That Must Work**:
- Interrupt rotation (who interrupts next cast?)
- Tank swap timing (when to taunt?)
- Soak assignments (who soaks which mechanic?)
- Phase transition coordination
- Role assignments (tank/healer/DPS positioning)

### Implementation Strategy: Dual-Layer Architecture

**Two Separate Systems**:

1. **Per-Bot `InstanceCoordination`** (existing, keep as-is)
   - Individual bot positioning
   - Personal cooldown tracking
   - Bot's assigned role/job execution

2. **NEW: `GroupInstanceCoordinator`** (group-level singleton or group-attached)
   - Interrupt rotation management
   - Tank swap coordination
   - Soak assignments
   - Phase transitions

#### File: `src/modules/Playerbot/Dungeon/GroupInstanceCoordinator.h` (NEW)

```cpp
#pragma once

#include "Common.h"
#include "ObjectGuid.h"
#include <memory>
#include <unordered_map>
#include <queue>
#include <mutex>

class Group;
class Player;
class InstanceScript;

namespace Playerbot
{

/**
 * @brief Group-level instance coordination for raids/dungeons
 *
 * Manages group-wide mechanics that cannot be handled by individual
 * per-bot InstanceCoordination instances:
 * - Interrupt rotation (Mythic+ critical)
 * - Tank swap timing
 * - Soak assignments (boss mechanics)
 * - Phase transition coordination
 *
 * One coordinator per active group in instance.
 * Created when group enters, destroyed when group exits.
 */
class GroupInstanceCoordinator
{
public:
    // Factory methods (one coordinator per group)
    static GroupInstanceCoordinator* GetForGroup(ObjectGuid groupGuid);
    static void CreateForGroup(Group* group, InstanceScript* instance);
    static void RemoveForGroup(ObjectGuid groupGuid);

    // ========================================================================
    // INSTANCE LIFECYCLE
    // ========================================================================
    
    void InitializeInstance(InstanceScript* instance);
    void UpdateCoordination(uint32 diff);
    void HandleInstanceCompletion();
    void HandleInstanceFailure();

    // ========================================================================
    // ENCOUNTER COORDINATION
    // ========================================================================
    
    void PrepareForEncounter(uint32 encounterId);
    void MonitorEncounterProgress(uint32 encounterId);
    void HandleEncounterRecovery(uint32 encounterId);

    // ========================================================================
    // MECHANIC COORDINATION (Critical for Mythic+/Raids)
    // ========================================================================
    
    /**
     * @brief Get next bot in interrupt rotation
     * CRITICAL for Mythic+ where missed interrupts = wipe
     */
    Player* GetNextInterrupter(uint32 spellId);
    void RegisterInterruptUsed(Player* bot, uint32 spellId, uint32 cooldown);

    /**
     * @brief Coordinate tank swap
     * Determines when off-tank should taunt based on stack count
     */
    void CoordinateTankSwap(uint32 stackCount);
    Player* GetCurrentMainTank();
    Player* GetCurrentOffTank();
    void SwapTanks();

    /**
     * @brief Assign soakers for boss mechanics
     * Selects bots to soak damage/debuffs based on health/role
     */
    std::vector<Player*> AssignSoakers(Position soakPosition, uint32 count);

    /**
     * @brief Coordinate phase transition movement
     * Moves entire group to new position for boss phases
     */
    void TransitionToPhase(uint32 phaseId);
    void CoordinateMovement(Position targetPosition);

private:
    explicit GroupInstanceCoordinator(Group* group, InstanceScript* instance);
    ~GroupInstanceCoordinator();

    Group* _group;
    InstanceScript* _instance;

    // Current encounter state
    uint32 _currentEncounterId{0};
    uint32 _currentPhase{0};

    // Interrupt rotation
    std::queue<Player*> _interruptRotation;
    std::map<Player*, uint32> _interruptCooldowns;  // When ready (timestamp)

    // Tank assignments
    Player* _mainTank{nullptr};
    Player* _offTank{nullptr};
    uint32 _tankSwapStackThreshold{5};

    // Singleton storage (one per group)
    static std::unordered_map<ObjectGuid, std::unique_ptr<GroupInstanceCoordinator>> _coordinators;
    static std::mutex _mutex;
};

} // namespace Playerbot
```

#### File: `GroupInstanceCoordinator.cpp` (NEW)

**Key Methods**:

```cpp
Player* GroupInstanceCoordinator::GetNextInterrupter(uint32 spellId)
{
    uint32 now = GameTime::GetGameTimeMS();

    // Rebuild rotation if empty
    if (_interruptRotation.empty())
    {
        for (GroupReference* ref = _group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member || member->IsRealPlayer())
                continue;

            // Check if bot has interrupt available
            if (IGameSystemsManager* sys = GetGameSystems(member))
            {
                if (sys->GetInstanceCoordination()->HasInterruptAvailable())
                    _interruptRotation.push(member);
            }
        }
    }

    // Get next in rotation with available interrupt
    while (!_interruptRotation.empty())
    {
        Player* bot = _interruptRotation.front();
        _interruptRotation.pop();

        // Check cooldown
        auto it = _interruptCooldowns.find(bot);
        if (it == _interruptCooldowns.end() || it->second <= now)
        {
            // This bot can interrupt
            _interruptRotation.push(bot);  // Re-add to back of queue
            return bot;
        }

        // Cooldown not ready, try next
        _interruptRotation.push(bot);
    }

    return nullptr;  // No interrupts available
}

void GroupInstanceCoordinator::CoordinateTankSwap(uint32 stackCount)
{
    if (!_mainTank || !_offTank)
        return;

    if (stackCount >= _tankSwapStackThreshold)
    {
        TC_LOG_INFO("playerbot.dungeon", "Coordinating tank swap at {} stacks", stackCount);
        SwapTanks();
    }
}

void GroupInstanceCoordinator::SwapTanks()
{
    std::swap(_mainTank, _offTank);

    // Command off-tank (now main) to taunt
    if (IGameSystemsManager* sys = GetGameSystems(_mainTank))
    {
        sys->GetInstanceCoordination()->ExecuteTaunt();
    }

    TC_LOG_INFO("playerbot.dungeon", "Tank swap: {} is now main tank", 
        _mainTank->GetName());
}
```

#### File: `DungeonBehavior.cpp` (UPDATE)

**Update to use GroupInstanceCoordinator**:

```cpp
bool DungeonBehavior::EnterDungeon(Group* group, uint32 dungeonId)
{
    // ... existing code ...

    // Initialize GROUP coordinator (new)
    if (group->GetInstanceScript())
    {
        GroupInstanceCoordinator::CreateForGroup(group, group->GetInstanceScript());
    }

    // Each bot still has their OWN InstanceCoordination (existing, unchanged)
    // This is accessed via: botAI->GetGameSystems()->GetInstanceCoordination()

    return true;
}

void DungeonBehavior::UpdateDungeonProgress(Group* group)
{
    // Update GROUP coordination (new)
    if (GroupInstanceCoordinator* coordinator = GroupInstanceCoordinator::GetForGroup(group->GetGUID()))
    {
        coordinator->UpdateCoordination(1000);
    }

    // Each bot updates individually (existing, unchanged via BotAI::UpdateAI)
}

void DungeonBehavior::PrepareForEncounter(Group* group, uint32 encounterId)
{
    // Use GROUP coordinator (new)
    if (GroupInstanceCoordinator* coordinator = GroupInstanceCoordinator::GetForGroup(group->GetGUID()))
    {
        coordinator->PrepareForEncounter(encounterId);
    }
}
```

### Testing Requirements

- ✅ Interrupt Rotation: Verify no missed interrupts in Mythic+ scenario
- ✅ Tank Swaps: Verify proper taunt timing on stack mechanics
- ✅ Soak Assignments: Verify correct bots soak damage zones
- ✅ Phase Transitions: Verify coordinated group movement
- ✅ Human Players: Verify bots coordinate with human tanks/healers

---

## Implementation Timeline

| Phase | Task | Estimated Effort | Priority |
|-------|------|------------------|----------|
| **1** | ✅ LFG Bot Selection | 2 hours | HIGH |
| **2** | Loot Distribution | 4 hours | HIGH |
| **3** | GroupInstanceCoordinator | 8 hours | CRITICAL |
| **4** | Testing & Integration | 4 hours | CRITICAL |

**Total**: ~18 hours for complete implementation

---

## Success Criteria

### For Human Players

1. **LFG**: ✅ Human queues for dungeon → instant fill with proper role bots
2. **Loot**: ✅ All WoW loot methods work correctly with bots
3. **Raids/Mythic+**: ✅ Bots coordinate mechanics (interrupts, tank swaps, soaks)
4. **Performance**: ✅ No performance degradation from group coordination

### Technical

1. **0 Active Singleton Calls**: All Phase 7 managers use per-bot pattern
2. **Architectural Clarity**: Clear separation between per-bot and group-level concerns
3. **Maintainability**: Well-documented coordination patterns
4. **Scalability**: Supports 5-40 player groups without issues

---

## Approval Required

Please confirm:
1. ✅ LFG implementation approach is acceptable
2. ⏳ Loot distribution hybrid evaluation strategy approved?
3. ⏳ Dual-layer instance coordination architecture approved?

Once approved, I will implement phases 2-4 systematically.
