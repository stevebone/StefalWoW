# GOD-TIER BOT SYSTEM - Vollständiger Implementierungsplan

**Version**: 1.0  
**Date**: 2026-01-27  
**Total Effort**: ~250h  
**Priority**: MAXIMUM

---

## 📋 ÜBERSICHT ALLER 14 TASKS

| # | Task | Priority | Effort | Dependencies |
|---|------|----------|--------|--------------|
| 1 | Quest System: Alle 21 Objective Types | P0 | 30h | - |
| 2 | RidingManager (automatisch lernen) | P0 | 15h | - |
| 3 | HumanizationManager Core | P0 | 40h | - |
| 4 | GatheringSessionManager (30+ Min) | P0 | 12h | #3 |
| 5 | CityLifeBehaviorManager | P0 | 10h | #3 |
| 6 | AchievementManager (Basic) | P0 | 20h | - |
| 7 | Mount Collection System | P1 | 15h | #2, #6 |
| 8 | Pet Collection System | P1 | 15h | #6 |
| 9 | ReputationGrindManager | P1 | 15h | #1 |
| 10 | Achievement Grinding | P1 | 15h | #6 |
| 11 | FishingSessionManager | P1 | 10h | #3 |
| 12 | AFKSimulator | P1 | 10h | #3 |
| 13 | GoldFarmingManager | P1 | 20h | #1, #9 |
| 14 | Instance Automation | P1 | 25h | #6 |

---

## TASK 1: Quest System - Alle 21 Objective Types (30h)

### 1.1 Ziel
Vollständige Abdeckung aller TrinityCore Quest Objective Types.

### 1.2 Zu implementierende Handler

```
src/modules/Playerbot/Quest/QuestCompletion.cpp
├── HandleCurrencyObjective()           # Type 4 - NEW
├── HandleLearnSpellObjective()         # Type 5 - FIX STUB
├── HandleMoneyObjective()              # Type 8 - NEW
├── HandlePlayerKillsObjective()        # Type 9 - NEW
├── HandlePetBattleNPCObjective()       # Type 11 - NEW
├── HandleDefeatBattlePetObjective()    # Type 12 - NEW
├── HandlePvPPetBattlesObjective()      # Type 13 - NEW
├── HandleCriteriaTreeObjective()       # Type 14 - FIX
├── HandleProgressBarObjective()        # Type 15 - FIX
├── HandleHaveCurrencyObjective()       # Type 16 - NEW
├── HandleObtainCurrencyObjective()     # Type 17 - NEW
└── HandleIncreaseReputationObjective() # Type 18 - NEW
```

### 1.3 Detaillierte Implementierung

#### 1.3.1 HandleCurrencyObjective (Type 4) - 2h

```cpp
/**
 * @brief Handle QUEST_OBJECTIVE_CURRENCY
 * Beispiele: "Sammle 500 Ehre", "Sammle 100 Tapferkeitsmarken"
 * 
 * Ablauf:
 * 1. Hole Currency-ID und benötigte Menge aus Quest-Daten
 * 2. Prüfe aktuelle Currency des Bots
 * 3. Wenn nicht genug: Starte passende Aktivität (PvP für Ehre, Dungeons für Marken)
 * 4. Tracke Progress
 */
void QuestCompletion::HandleCurrencyObjective(Player* bot, QuestObjectiveData& objective)
{
    uint32 currencyId = objective.targetId;
    uint32 requiredAmount = objective.requiredCount;
    
    // Get current currency
    uint32 currentAmount = bot->GetCurrency(currencyId);
    objective.currentCount = currentAmount;
    
    if (currentAmount >= requiredAmount)
    {
        objective.status = ObjectiveStatus::COMPLETED;
        return;
    }
    
    // Determine how to get this currency
    CurrencyTypesEntry const* currency = sCurrencyTypesStore.LookupEntry(currencyId);
    if (!currency)
        return;
    
    // Start appropriate activity based on currency type
    switch (currencyId)
    {
        case CURRENCY_HONOR:
            // Queue for BG or do PvP world quests
            StartPvPActivity(bot);
            break;
        case CURRENCY_CONQUEST:
            // Queue for rated PvP
            StartRatedPvPActivity(bot);
            break;
        case CURRENCY_VALOR:
        case CURRENCY_JUSTICE:
            // Run dungeons
            StartDungeonActivity(bot);
            break;
        default:
            // Check if world quests give this currency
            StartCurrencyWorldQuests(bot, currencyId);
            break;
    }
}
```

#### 1.3.2 HandleLearnSpellObjective (Type 5) - 3h

```cpp
/**
 * @brief Handle QUEST_OBJECTIVE_LEARNSPELL
 * Beispiele: "Lerne Reiten", "Lerne Kräuterkunde"
 * 
 * Ablauf:
 * 1. Hole Spell-ID aus Quest-Daten
 * 2. Prüfe ob Bot Spell bereits kennt
 * 3. Wenn nicht: Finde Trainer, reise hin, lerne Spell
 */
void QuestCompletion::HandleLearnSpellObjective(Player* bot, QuestObjectiveData& objective)
{
    uint32 spellId = objective.targetId;
    
    // Check if already learned
    if (bot->HasSpell(spellId))
    {
        objective.status = ObjectiveStatus::COMPLETED;
        objective.currentCount = 1;
        return;
    }
    
    // Find trainer for this spell
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return;
    
    // Determine spell type and find appropriate trainer
    Creature* trainer = FindTrainerForSpell(bot, spellId);
    if (!trainer)
    {
        // No trainer found - might be a profession discovery or quest reward
        LOG_DEBUG("playerbot", "No trainer found for spell %u", spellId);
        return;
    }
    
    // Navigate to trainer
    if (bot->GetDistance(trainer) > INTERACTION_DISTANCE)
    {
        NavigateToCreature(bot, trainer);
        return;
    }
    
    // Learn spell from trainer
    if (CanLearnSpellFromTrainer(bot, trainer, spellId))
    {
        LearnSpellFromTrainer(bot, trainer, spellId);
        objective.currentCount = 1;
        objective.status = ObjectiveStatus::COMPLETED;
    }
}

Creature* QuestCompletion::FindTrainerForSpell(Player* bot, uint32 spellId)
{
    // Check riding trainers
    if (IsRidingSpell(spellId))
        return FindRidingTrainer(bot);
    
    // Check profession trainers
    SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellId);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
    {
        uint32 skillId = itr->second->SkillLine;
        return FindProfessionTrainer(bot, skillId);
    }
    
    // Check class trainers
    return FindClassTrainer(bot);
}
```

#### 1.3.3 HandleMoneyObjective (Type 8) - 1h

```cpp
/**
 * @brief Handle QUEST_OBJECTIVE_MONEY
 * Beispiele: "Sammle 10 Gold"
 */
void QuestCompletion::HandleMoneyObjective(Player* bot, QuestObjectiveData& objective)
{
    uint64 requiredMoney = objective.requiredCount;  // In copper
    uint64 currentMoney = bot->GetMoney();
    
    objective.currentCount = static_cast<uint32>(std::min(currentMoney, (uint64)UINT32_MAX));
    
    if (currentMoney >= requiredMoney)
    {
        objective.status = ObjectiveStatus::COMPLETED;
        return;
    }
    
    // Need to earn money - trigger gold farming behavior
    // This will be handled by GoldFarmingManager
    if (auto* goldMgr = GetGoldFarmingManager(bot))
    {
        goldMgr->SetMinimumGold(requiredMoney);
        goldMgr->StartGoldFarm(GoldFarmingStrategy::EFFICIENT);
    }
}
```

#### 1.3.4 HandlePlayerKillsObjective (Type 9) - 3h

```cpp
/**
 * @brief Handle QUEST_OBJECTIVE_PLAYERKILLS
 * Beispiele: "Töte 10 Spieler in Schlachtfeldern"
 */
void QuestCompletion::HandlePlayerKillsObjective(Player* bot, QuestObjectiveData& objective)
{
    uint32 requiredKills = objective.requiredCount;
    
    // Get current honorable kills from quest progress
    // This is tracked by the server, we just read it
    if (Quest const* quest = sObjectMgr->GetQuestTemplate(objective.questId))
    {
        uint16 slot = bot->FindQuestSlot(objective.questId);
        if (slot < MAX_QUEST_LOG_SIZE)
        {
            objective.currentCount = bot->GetQuestSlotCounter(slot, objective.objectiveIndex);
        }
    }
    
    if (objective.currentCount >= requiredKills)
    {
        objective.status = ObjectiveStatus::COMPLETED;
        return;
    }
    
    // Queue for PvP
    if (!bot->InBattleground() && !bot->InArena())
    {
        // Queue for random BG
        QueueForBattleground(bot, BATTLEGROUND_QUEUE_RB);
    }
    else
    {
        // Already in PvP - focus on killing players
        if (BotAI* ai = GetBotAI(bot))
        {
            ai->SetPvPMode(true);
            ai->SetPriorityTarget(PriorityTargetType::PLAYER);
        }
    }
}
```

#### 1.3.5 Pet Battle Objectives (Types 11, 12, 13) - 6h

```cpp
/**
 * @brief Handle QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC (Type 11)
 */
void QuestCompletion::HandlePetBattleNPCObjective(Player* bot, QuestObjectiveData& objective)
{
    uint32 npcId = objective.targetId;
    
    // Check if already completed
    if (objective.currentCount >= objective.requiredCount)
    {
        objective.status = ObjectiveStatus::COMPLETED;
        return;
    }
    
    // Find the pet battle NPC
    Creature* battleNpc = FindCreatureForPetBattle(bot, npcId);
    if (!battleNpc)
    {
        // NPC not found in current area - need to travel
        Position npcPos = GetCreatureSpawnPosition(npcId);
        NavigateToPosition(bot, npcPos);
        return;
    }
    
    // Navigate to NPC if not close
    if (bot->GetDistance(battleNpc) > INTERACTION_DISTANCE)
    {
        NavigateToCreature(bot, battleNpc);
        return;
    }
    
    // Start pet battle
    if (PetBattleManager* pbMgr = GetPetBattleManager(bot))
    {
        pbMgr->StartPetBattle(battleNpc);
    }
}

/**
 * @brief Handle QUEST_OBJECTIVE_DEFEATBATTLEPET (Type 12)
 */
void QuestCompletion::HandleDefeatBattlePetObjective(Player* bot, QuestObjectiveData& objective)
{
    uint32 petSpeciesId = objective.targetId;
    
    // Find wild pet of this species
    Position petLocation = FindWildPetLocation(petSpeciesId);
    if (petLocation.IsEmpty())
    {
        LOG_DEBUG("playerbot", "Cannot find wild pet species %u", petSpeciesId);
        return;
    }
    
    // Travel to location
    if (bot->GetDistance(petLocation) > 50.0f)
    {
        NavigateToPosition(bot, petLocation);
        return;
    }
    
    // Find the actual pet in the world
    if (Creature* wildPet = FindNearbyWildPet(bot, petSpeciesId))
    {
        if (PetBattleManager* pbMgr = GetPetBattleManager(bot))
        {
            pbMgr->StartWildPetBattle(wildPet);
        }
    }
}

/**
 * @brief Handle QUEST_OBJECTIVE_WINPVPPETBATTLES (Type 13)
 */
void QuestCompletion::HandlePvPPetBattlesObjective(Player* bot, QuestObjectiveData& objective)
{
    // Queue for PvP pet battles
    if (PetBattleManager* pbMgr = GetPetBattleManager(bot))
    {
        pbMgr->QueueForPvPBattle();
    }
}
```

#### 1.3.6 HandleCriteriaTreeObjective (Type 14) - 4h

```cpp
/**
 * @brief Handle QUEST_OBJECTIVE_CRITERIA_TREE
 * Achievement-ähnliche Kriterien
 */
void QuestCompletion::HandleCriteriaTreeObjective(Player* bot, QuestObjectiveData& objective)
{
    uint32 criteriaTreeId = objective.targetId;
    
    // Get criteria tree from DB
    CriteriaTree const* tree = sCriteriaMgr->GetCriteriaTree(criteriaTreeId);
    if (!tree)
        return;
    
    // Check completion
    bool completed = bot->GetAchievementMgr()->IsCompletedCriteriaTree(tree);
    if (completed)
    {
        objective.status = ObjectiveStatus::COMPLETED;
        objective.currentCount = objective.requiredCount;
        return;
    }
    
    // Get progress
    CriteriaProgress const* progress = bot->GetAchievementMgr()->GetCriteriaProgress(
        tree->Criteria);
    if (progress)
    {
        objective.currentCount = progress->Counter;
    }
    
    // Determine what actions to take based on criteria type
    Criteria const* criteria = sCriteriaMgr->GetCriteria(tree->Criteria->ID);
    if (criteria)
    {
        ExecuteCriteriaAction(bot, criteria, objective);
    }
}

void QuestCompletion::ExecuteCriteriaAction(Player* bot, Criteria const* criteria, 
    QuestObjectiveData& objective)
{
    switch (criteria->Entry->Type)
    {
        case CRITERIA_TYPE_KILL_CREATURE:
            // Kill specific creature
            objective.type = QuestObjectiveType::KILL_CREATURE;
            objective.targetId = criteria->Entry->Asset.CreatureID;
            HandleKillObjective(bot, objective);
            break;
            
        case CRITERIA_TYPE_COMPLETE_QUEST:
            // Complete specific quest
            if (!bot->GetQuestRewardStatus(criteria->Entry->Asset.QuestID))
            {
                // Accept and complete this quest
                AcceptQuest(bot, criteria->Entry->Asset.QuestID);
            }
            break;
            
        case CRITERIA_TYPE_EXPLORE_AREA:
            // Explore specific area
            NavigateToArea(bot, criteria->Entry->Asset.WorldMapOverlayID);
            break;
            
        case CRITERIA_TYPE_REACH_LEVEL:
            // Need to level up - continue normal leveling
            break;
            
        case CRITERIA_TYPE_LEARN_SPELL:
            objective.type = QuestObjectiveType::CAST_SPELL;
            objective.targetId = criteria->Entry->Asset.SpellID;
            HandleLearnSpellObjective(bot, objective);
            break;
            
        // ... more criteria types
    }
}
```

#### 1.3.7 Currency Validation/Acquisition (Types 16, 17, 18) - 4h

```cpp
/**
 * @brief Handle QUEST_OBJECTIVE_HAVE_CURRENCY (Type 16)
 * Validation: Bot muss X Currency haben für Turn-in
 */
void QuestCompletion::HandleHaveCurrencyObjective(Player* bot, QuestObjectiveData& objective)
{
    uint32 currencyId = objective.targetId;
    uint32 requiredAmount = objective.requiredCount;
    
    uint32 currentAmount = bot->GetCurrency(currencyId);
    objective.currentCount = currentAmount;
    
    if (currentAmount >= requiredAmount)
    {
        objective.status = ObjectiveStatus::COMPLETED;
    }
    else
    {
        // Same as Type 4 - need to earn currency
        HandleCurrencyObjective(bot, objective);
    }
}

/**
 * @brief Handle QUEST_OBJECTIVE_OBTAIN_CURRENCY (Type 17)
 * Währung WÄHREND der Quest erhalten (nicht vorher haben)
 */
void QuestCompletion::HandleObtainCurrencyObjective(Player* bot, QuestObjectiveData& objective)
{
    // This is tracked by server - we just need to do activities that give currency
    HandleCurrencyObjective(bot, objective);
}

/**
 * @brief Handle QUEST_OBJECTIVE_INCREASE_REPUTATION (Type 18)
 * Ruf bei Fraktion erhöhen
 */
void QuestCompletion::HandleIncreaseReputationObjective(Player* bot, QuestObjectiveData& objective)
{
    uint32 factionId = objective.targetId;
    int32 requiredRepGain = static_cast<int32>(objective.requiredCount);
    
    // Get initial rep from quest start (stored in objective data)
    int32 initialRep = objective.alternativeTargets.empty() ? 
        bot->GetReputation(factionId) : objective.alternativeTargets[0];
    
    // Store initial rep if not set
    if (objective.alternativeTargets.empty())
    {
        objective.alternativeTargets.push_back(initialRep);
    }
    
    int32 currentRep = bot->GetReputation(factionId);
    int32 gainedRep = currentRep - initialRep;
    
    objective.currentCount = std::max(0, gainedRep);
    
    if (gainedRep >= requiredRepGain)
    {
        objective.status = ObjectiveStatus::COMPLETED;
        return;
    }
    
    // Start reputation grind for this faction
    if (auto* repMgr = GetReputationManager(bot))
    {
        repMgr->StartReputationGrind(factionId, requiredRepGain - gainedRep);
    }
}
```

### 1.4 Dispatcher Update

```cpp
// In QuestCompletion::ExecuteObjective() - Update switch statement

void QuestCompletion::ExecuteObjective(Player* bot, QuestObjectiveData& objective)
{
    switch (objective.type)
    {
        // Existing handlers
        case QuestObjectiveType::KILL_CREATURE:
            HandleKillObjective(bot, objective);
            break;
        case QuestObjectiveType::COLLECT_ITEM:
            HandleCollectObjective(bot, objective);
            break;
        case QuestObjectiveType::TALK_TO_NPC:
            HandleTalkToNpcObjective(bot, objective);
            break;
        case QuestObjectiveType::REACH_LOCATION:
            HandleLocationObjective(bot, objective);
            break;
        case QuestObjectiveType::USE_GAMEOBJECT:
            HandleGameObjectObjective(bot, objective);
            break;
        case QuestObjectiveType::CAST_SPELL:
            HandleSpellCastObjective(bot, objective);
            break;
        case QuestObjectiveType::EMOTE_AT_TARGET:
            HandleEmoteObjective(bot, objective);
            break;
        case QuestObjectiveType::ESCORT_NPC:
            HandleEscortObjective(bot, objective);
            break;
            
        // NEW handlers
        case QuestObjectiveType::CURRENCY:
            HandleCurrencyObjective(bot, objective);
            break;
        case QuestObjectiveType::LEARN_SPELL:
            HandleLearnSpellObjective(bot, objective);
            break;
        case QuestObjectiveType::MONEY:
            HandleMoneyObjective(bot, objective);
            break;
        case QuestObjectiveType::PLAYER_KILLS:
            HandlePlayerKillsObjective(bot, objective);
            break;
        case QuestObjectiveType::PET_BATTLE_NPC:
            HandlePetBattleNPCObjective(bot, objective);
            break;
        case QuestObjectiveType::DEFEAT_BATTLE_PET:
            HandleDefeatBattlePetObjective(bot, objective);
            break;
        case QuestObjectiveType::PVP_PET_BATTLES:
            HandlePvPPetBattlesObjective(bot, objective);
            break;
        case QuestObjectiveType::CRITERIA_TREE:
            HandleCriteriaTreeObjective(bot, objective);
            break;
        case QuestObjectiveType::PROGRESS_BAR:
            HandleProgressBarObjective(bot, objective);
            break;
        case QuestObjectiveType::HAVE_CURRENCY:
            HandleHaveCurrencyObjective(bot, objective);
            break;
        case QuestObjectiveType::OBTAIN_CURRENCY:
            HandleObtainCurrencyObjective(bot, objective);
            break;
        case QuestObjectiveType::INCREASE_REPUTATION:
            HandleIncreaseReputationObjective(bot, objective);
            break;
            
        default:
            LOG_WARN("playerbot", "Unknown quest objective type: %u", 
                static_cast<uint8>(objective.type));
            break;
    }
}
```

### 1.5 Enum Update

```cpp
// In QuestCompletion.h - Update enum

enum class QuestObjectiveType : uint8
{
    KILL_CREATURE       = 0,   // QUEST_OBJECTIVE_MONSTER
    COLLECT_ITEM        = 1,   // QUEST_OBJECTIVE_ITEM
    USE_GAMEOBJECT      = 2,   // QUEST_OBJECTIVE_GAMEOBJECT
    TALK_TO_NPC         = 3,   // QUEST_OBJECTIVE_TALKTO
    CURRENCY            = 4,   // QUEST_OBJECTIVE_CURRENCY - NEW
    LEARN_SPELL         = 5,   // QUEST_OBJECTIVE_LEARNSPELL - NEW
    MIN_REPUTATION      = 6,   // QUEST_OBJECTIVE_MIN_REPUTATION
    MAX_REPUTATION      = 7,   // QUEST_OBJECTIVE_MAX_REPUTATION
    MONEY               = 8,   // QUEST_OBJECTIVE_MONEY - NEW
    PLAYER_KILLS        = 9,   // QUEST_OBJECTIVE_PLAYERKILLS - NEW
    REACH_LOCATION      = 10,  // QUEST_OBJECTIVE_AREATRIGGER
    PET_BATTLE_NPC      = 11,  // QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC - NEW
    DEFEAT_BATTLE_PET   = 12,  // QUEST_OBJECTIVE_DEFEATBATTLEPET - NEW
    PVP_PET_BATTLES     = 13,  // QUEST_OBJECTIVE_WINPVPPETBATTLES - NEW
    CRITERIA_TREE       = 14,  // QUEST_OBJECTIVE_CRITERIA_TREE - NEW
    PROGRESS_BAR        = 15,  // QUEST_OBJECTIVE_PROGRESS_BAR - NEW
    HAVE_CURRENCY       = 16,  // QUEST_OBJECTIVE_HAVE_CURRENCY - NEW
    OBTAIN_CURRENCY     = 17,  // QUEST_OBJECTIVE_OBTAIN_CURRENCY - NEW
    INCREASE_REPUTATION = 18,  // QUEST_OBJECTIVE_INCREASE_REPUTATION - NEW
    AREA_TRIGGER_ENTER  = 19,  // QUEST_OBJECTIVE_AREA_TRIGGER_ENTER
    AREA_TRIGGER_EXIT   = 20,  // QUEST_OBJECTIVE_AREA_TRIGGER_EXIT
    
    // Legacy/Custom
    CAST_SPELL          = 100,
    EMOTE_AT_TARGET     = 101,
    ESCORT_NPC          = 102,
    DEFEND_AREA         = 103,
    SURVIVE_TIME        = 104,
    WIN_BATTLEGROUND    = 105,
    COMPLETE_DUNGEON    = 106,
    GAIN_EXPERIENCE     = 107,
    CUSTOM_OBJECTIVE    = 255
};
```

---

## TASK 2: RidingManager (15h)

### 2.1 Neue Dateien

```
src/modules/Playerbot/Humanization/Mounts/
├── RidingManager.h
├── RidingManager.cpp
├── MountDatabase.h
├── MountDatabase.cpp
└── CMakeLists.txt
```

### 2.2 RidingManager.h

```cpp
#pragma once

#include "Define.h"
#include "Position.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

class Player;
class Creature;

namespace Playerbot
{

class BotAI;

/**
 * @brief Riding skill levels matching WoW progression
 */
enum class RidingSkillLevel : uint8
{
    NONE = 0,
    APPRENTICE = 1,      // 60% ground (Level 10, 1g)
    JOURNEYMAN = 2,      // 100% ground (Level 20, 5g)
    EXPERT = 3,          // 150% flying (Level 30, 50g)
    ARTISAN = 4,         // 280% flying (Level 40, 250g)
    MASTER = 5,          // 310% flying (Level 50, requires achieve)
    COLD_WEATHER = 6,    // Northrend flying
    FLIGHT_MASTERS = 7,  // Cataclysm flying
    WISDOM_WINDS = 8,    // Pandaria flying
    DRAENOR = 9,         // Draenor Pathfinder
    BROKEN_ISLES = 10,   // Legion Pathfinder
    BFA = 11,            // BfA Pathfinder
    SHADOWLANDS = 12,    // Shadowlands flying
    DRAGONFLIGHT = 13,   // Dragonriding
    TWW = 14             // TWW flying
};

/**
 * @brief Riding trainer info
 */
struct RidingTrainerInfo
{
    uint32 creatureId;
    uint32 mapId;
    Position position;
    uint32 factionMask;  // Alliance/Horde
    RidingSkillLevel maxLevel;  // Highest level this trainer teaches
};

/**
 * @brief Mount info
 */
struct MountInfo
{
    uint32 spellId;
    uint32 itemId;           // Item that teaches this mount (if any)
    uint32 cost;             // Cost in copper (if vendor)
    RidingSkillLevel requiredRiding;
    bool isFlying;
    bool isAquatic;
    uint32 speed;            // Speed percentage
    
    enum class Source : uint8
    {
        VENDOR,
        DROP,
        QUEST,
        ACHIEVEMENT,
        PROFESSION,
        REPUTATION,
        PROMOTION,
        UNKNOWN
    };
    Source source;
    uint32 sourceId;         // Vendor NPC, Boss ID, Quest ID, etc.
};

/**
 * @brief Manages riding skills and mounts for bots
 */
class TC_GAME_API RidingManager
{
public:
    explicit RidingManager(Player* bot, BotAI* ai);
    ~RidingManager();
    
    // Non-copyable
    RidingManager(RidingManager const&) = delete;
    RidingManager& operator=(RidingManager const&) = delete;
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    void Initialize();
    void Update(uint32 diff);
    
    // ========================================================================
    // RIDING SKILL
    // ========================================================================
    
    /**
     * @brief Get current riding skill level
     */
    RidingSkillLevel GetRidingLevel() const;
    
    /**
     * @brief Get next learnable riding level
     */
    RidingSkillLevel GetNextRidingLevel() const;
    
    /**
     * @brief Check if can learn next riding level
     * Checks: level requirement, gold, trainer availability
     */
    bool CanLearnNextRiding() const;
    
    /**
     * @brief Get cost for riding level (in copper)
     */
    uint64 GetRidingCost(RidingSkillLevel level) const;
    
    /**
     * @brief Get level requirement for riding level
     */
    uint8 GetRidingLevelRequirement(RidingSkillLevel level) const;
    
    /**
     * @brief Attempt to learn next riding level
     * Will travel to trainer if needed
     * @return true if learning initiated or in progress
     */
    bool LearnNextRiding();
    
    /**
     * @brief Check if currently learning riding
     */
    bool IsLearningRiding() const { return _learningRiding; }
    
    // ========================================================================
    // TRAINERS
    // ========================================================================
    
    /**
     * @brief Find riding trainer for bot's faction
     */
    Creature* FindRidingTrainer() const;
    
    /**
     * @brief Get trainer info for faction
     */
    RidingTrainerInfo const* GetTrainerInfo() const;
    
    /**
     * @brief Get all riding trainers
     */
    std::vector<RidingTrainerInfo> GetAllTrainers() const;
    
    // ========================================================================
    // MOUNTS
    // ========================================================================
    
    /**
     * @brief Get all known mounts
     */
    std::vector<uint32> GetKnownMounts() const;
    
    /**
     * @brief Get number of known mounts
     */
    uint32 GetMountCount() const;
    
    /**
     * @brief Check if bot knows a specific mount
     */
    bool KnowsMount(uint32 spellId) const;
    
    /**
     * @brief Get best mount for current situation
     * Considers: zone (flying allowed?), terrain, speed
     */
    uint32 GetBestMount() const;
    
    /**
     * @brief Get best ground mount
     */
    uint32 GetBestGroundMount() const;
    
    /**
     * @brief Get best flying mount
     */
    uint32 GetBestFlyingMount() const;
    
    /**
     * @brief Get random mount (for variety)
     */
    uint32 GetRandomMount() const;
    
    /**
     * @brief Mount up with best available mount
     * @return true if mount initiated
     */
    bool MountUp();
    
    /**
     * @brief Mount with specific mount
     */
    bool MountUp(uint32 mountSpellId);
    
    /**
     * @brief Dismount
     */
    void Dismount();
    
    /**
     * @brief Check if should be mounted
     * Based on: distance to target, combat state, zone
     */
    bool ShouldMount() const;
    
    /**
     * @brief Check if should dismount
     */
    bool ShouldDismount() const;
    
    // ========================================================================
    // FLYING
    // ========================================================================
    
    /**
     * @brief Check if flying is allowed in current zone
     */
    bool CanFlyInCurrentZone() const;
    
    /**
     * @brief Check if has pathfinder for specific zone
     */
    bool HasPathfinder(uint32 mapId) const;
    
    /**
     * @brief Get required achievement for zone flying
     */
    uint32 GetPathfinderAchievement(uint32 mapId) const;
    
    // ========================================================================
    // MOUNT COLLECTION (Future)
    // ========================================================================
    
    /**
     * @brief Get mounts that can be obtained
     */
    std::vector<MountInfo> GetObtainableMounts() const;
    
    /**
     * @brief Start farming specific mount
     */
    void FarmMount(uint32 mountSpellId);
    
    /**
     * @brief Get mount info
     */
    MountInfo const* GetMountInfo(uint32 spellId) const;
    
private:
    // Initialization helpers
    void LoadTrainerData();
    void LoadMountData();
    void CacheKnownMounts();
    
    // Learning helpers
    bool TravelToTrainer();
    bool InteractWithTrainer(Creature* trainer);
    bool PurchaseRidingSkill(Creature* trainer, RidingSkillLevel level);
    bool PurchaseFirstMount(Creature* vendor);
    
    // Mount selection helpers
    uint32 SelectMountForZone() const;
    uint32 SelectMountForSpeed() const;
    bool IsMountUsableInZone(uint32 spellId) const;
    
    Player* _bot;
    BotAI* _ai;
    
    // State
    bool _initialized = false;
    bool _learningRiding = false;
    RidingSkillLevel _targetLevel = RidingSkillLevel::NONE;
    
    // Cache
    mutable RidingSkillLevel _cachedLevel = RidingSkillLevel::NONE;
    mutable uint32 _lastLevelCheck = 0;
    std::unordered_set<uint32> _knownMounts;
    uint32 _lastMountCacheUpdate = 0;
    
    // Static data
    static std::vector<RidingTrainerInfo> _trainerData;
    static std::unordered_map<uint32, MountInfo> _mountData;
    static bool _staticDataLoaded;
    
    // Constants
    static constexpr uint32 MOUNT_CACHE_INTERVAL = 60000;  // 1 minute
    static constexpr uint32 LEVEL_CHECK_INTERVAL = 5000;   // 5 seconds
    static constexpr float MOUNT_DISTANCE_THRESHOLD = 50.0f;  // Mount if going >50 yards
};

} // namespace Playerbot
```

### 2.3 RidingManager.cpp (Kernmethoden)

```cpp
#include "RidingManager.h"
#include "Player.h"
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "MountMgr.h"
#include "BotAI.h"

namespace Playerbot
{

// Static data initialization
std::vector<RidingTrainerInfo> RidingManager::_trainerData;
std::unordered_map<uint32, MountInfo> RidingManager::_mountData;
bool RidingManager::_staticDataLoaded = false;

RidingManager::RidingManager(Player* bot, BotAI* ai)
    : _bot(bot), _ai(ai)
{
    if (!_staticDataLoaded)
    {
        LoadTrainerData();
        LoadMountData();
        _staticDataLoaded = true;
    }
}

void RidingManager::Initialize()
{
    CacheKnownMounts();
    _initialized = true;
}

void RidingManager::Update(uint32 diff)
{
    if (!_initialized)
        return;
    
    // Update mount cache periodically
    if (GameTime::GetGameTimeMS() - _lastMountCacheUpdate > MOUNT_CACHE_INTERVAL)
    {
        CacheKnownMounts();
    }
    
    // Continue learning process if active
    if (_learningRiding)
    {
        ContinueLearningProcess();
    }
    
    // Auto-mount/dismount
    if (ShouldMount() && !_bot->IsMounted())
    {
        MountUp();
    }
    else if (ShouldDismount() && _bot->IsMounted())
    {
        Dismount();
    }
}

RidingSkillLevel RidingManager::GetRidingLevel() const
{
    // Check cache
    if (GameTime::GetGameTimeMS() - _lastLevelCheck < LEVEL_CHECK_INTERVAL)
        return _cachedLevel;
    
    _lastLevelCheck = GameTime::GetGameTimeMS();
    
    // Check riding skills (TWW skill IDs)
    // Expert Riding (310% flying)
    if (_bot->HasSpell(90265))
        _cachedLevel = RidingSkillLevel::MASTER;
    // Artisan Riding (280% flying)
    else if (_bot->HasSpell(34091))
        _cachedLevel = RidingSkillLevel::ARTISAN;
    // Expert Riding (150% flying)
    else if (_bot->HasSpell(34090))
        _cachedLevel = RidingSkillLevel::EXPERT;
    // Journeyman Riding (100% ground)
    else if (_bot->HasSpell(33391))
        _cachedLevel = RidingSkillLevel::JOURNEYMAN;
    // Apprentice Riding (60% ground)
    else if (_bot->HasSpell(33388))
        _cachedLevel = RidingSkillLevel::APPRENTICE;
    else
        _cachedLevel = RidingSkillLevel::NONE;
    
    return _cachedLevel;
}

bool RidingManager::CanLearnNextRiding() const
{
    RidingSkillLevel current = GetRidingLevel();
    RidingSkillLevel next = static_cast<RidingSkillLevel>(
        static_cast<uint8>(current) + 1);
    
    if (next > RidingSkillLevel::MASTER)
        return false;  // Beyond basic riding
    
    // Check level requirement
    uint8 reqLevel = GetRidingLevelRequirement(next);
    if (_bot->GetLevel() < reqLevel)
        return false;
    
    // Check gold
    uint64 cost = GetRidingCost(next);
    if (_bot->GetMoney() < cost)
        return false;
    
    return true;
}

bool RidingManager::LearnNextRiding()
{
    if (!CanLearnNextRiding())
        return false;
    
    RidingSkillLevel next = GetNextRidingLevel();
    
    // Find trainer
    Creature* trainer = FindRidingTrainer();
    if (!trainer)
    {
        // Trainer not in world - need to spawn or find another
        LOG_DEBUG("playerbot", "Bot %s cannot find riding trainer", 
            _bot->GetName().c_str());
        return false;
    }
    
    // Check distance
    if (_bot->GetDistance(trainer) > INTERACTION_DISTANCE)
    {
        // Need to travel to trainer
        _learningRiding = true;
        _targetLevel = next;
        return TravelToTrainer();
    }
    
    // At trainer - learn skill
    if (PurchaseRidingSkill(trainer, next))
    {
        // Invalidate cache
        _cachedLevel = RidingSkillLevel::NONE;
        _lastLevelCheck = 0;
        
        // If first riding, also buy a mount
        if (next == RidingSkillLevel::APPRENTICE && GetMountCount() == 0)
        {
            PurchaseFirstMount(trainer);
        }
        
        _learningRiding = false;
        return true;
    }
    
    return false;
}

uint32 RidingManager::GetBestMount() const
{
    if (CanFlyInCurrentZone() && GetRidingLevel() >= RidingSkillLevel::EXPERT)
    {
        return GetBestFlyingMount();
    }
    return GetBestGroundMount();
}

bool RidingManager::MountUp()
{
    if (_bot->IsMounted())
        return true;
    
    if (_bot->IsInCombat())
        return false;
    
    uint32 mountSpell = GetBestMount();
    if (mountSpell == 0)
        return false;
    
    // Cast mount spell
    _bot->CastSpell(_bot, mountSpell, false);
    return true;
}

bool RidingManager::ShouldMount() const
{
    // Don't mount in combat
    if (_bot->IsInCombat())
        return false;
    
    // Don't mount if no riding skill
    if (GetRidingLevel() == RidingSkillLevel::NONE)
        return false;
    
    // Don't mount if no mounts known
    if (GetMountCount() == 0)
        return false;
    
    // Don't mount indoors (usually)
    if (_bot->IsIndoors() && !CanFlyInCurrentZone())
        return false;
    
    // Mount if traveling long distance
    if (_ai && _ai->HasDestination())
    {
        float dist = _bot->GetDistance(_ai->GetDestination());
        if (dist > MOUNT_DISTANCE_THRESHOLD)
            return true;
    }
    
    return false;
}

void RidingManager::LoadTrainerData()
{
    _trainerData = {
        // Alliance
        {43769, 0, {-8837.0f, 482.0f, 109.6f, 0}, ALLIANCE, RidingSkillLevel::ARTISAN}, // Stormwind
        {35101, 0, {-5024.0f, -952.0f, 495.0f, 0}, ALLIANCE, RidingSkillLevel::ARTISAN}, // Ironforge
        {4753, 1, {9836.0f, 924.0f, 1309.0f, 0}, ALLIANCE, RidingSkillLevel::ARTISAN}, // Darnassus
        
        // Horde
        {44919, 1, {1576.0f, -4225.0f, 56.0f, 0}, HORDE, RidingSkillLevel::ARTISAN}, // Orgrimmar
        {7953, 0, {1780.0f, 293.0f, -47.0f, 0}, HORDE, RidingSkillLevel::ARTISAN}, // Undercity
        {7954, 1, {-1201.0f, -53.0f, 159.0f, 0}, HORDE, RidingSkillLevel::ARTISAN}, // Thunder Bluff
        
        // ... more trainers
    };
}

uint64 RidingManager::GetRidingCost(RidingSkillLevel level) const
{
    // TWW costs (may vary with rep discounts)
    switch (level)
    {
        case RidingSkillLevel::APPRENTICE: return 1 * GOLD;      // 1g
        case RidingSkillLevel::JOURNEYMAN: return 5 * GOLD;      // 5g
        case RidingSkillLevel::EXPERT:     return 50 * GOLD;     // 50g
        case RidingSkillLevel::ARTISAN:    return 250 * GOLD;    // 250g
        case RidingSkillLevel::MASTER:     return 5000 * GOLD;   // 5000g
        default: return 0;
    }
}

uint8 RidingManager::GetRidingLevelRequirement(RidingSkillLevel level) const
{
    // TWW level requirements
    switch (level)
    {
        case RidingSkillLevel::APPRENTICE: return 10;
        case RidingSkillLevel::JOURNEYMAN: return 20;
        case RidingSkillLevel::EXPERT:     return 30;
        case RidingSkillLevel::ARTISAN:    return 40;
        case RidingSkillLevel::MASTER:     return 50;
        default: return 80;
    }
}

} // namespace Playerbot
```

---

## TASK 3: HumanizationManager Core (40h)

### 3.1 Verzeichnisstruktur

```
src/modules/Playerbot/Humanization/
├── CMakeLists.txt
├── Core/
│   ├── HumanizationManager.h
│   ├── HumanizationManager.cpp
│   ├── HumanizationConfig.h
│   ├── HumanizationConfig.cpp
│   ├── PersonalityProfile.h
│   ├── PersonalityProfile.cpp
│   ├── ActivityType.h
│   └── HumanizationDefines.h
├── Sessions/
│   ├── ActivitySession.h
│   ├── ActivitySessionManager.h
│   ├── ActivitySessionManager.cpp
│   ├── SessionTransitions.h
│   └── SessionTransitions.cpp
└── Interfaces/
    ├── IHumanizationManager.h
    └── ISessionManager.h
```

### 3.2 ActivityType.h

```cpp
#pragma once

#include "Define.h"

namespace Playerbot
{

/**
 * @brief Activity types for session management
 */
enum class ActivityType : uint8
{
    NONE = 0,
    
    // Primary activities (long sessions 30-90 min)
    QUESTING,           // Quest completion
    GRINDING,           // Mob grinding for XP
    GATHERING,          // Mining, Herbalism, Skinning
    FISHING,            // Fishing (hobby mode)
    DUNGEON,            // Running dungeons
    RAID,               // Raiding
    PVP_BG,             // Battlegrounds
    PVP_ARENA,          // Arenas
    
    // Secondary activities (medium sessions 10-30 min)
    CRAFTING,           // Profession crafting
    COOKING,            // Cooking specifically
    CITY_LIFE,          // AH, mailbox, vendors
    ACHIEVEMENT_HUNT,   // Achievement grinding
    MOUNT_FARM,         // Mount farming
    TRANSMOG_FARM,      // Transmog farming
    REP_GRIND,          // Reputation grinding
    
    // Short activities (1-10 min)
    EXPLORING,          // Just wandering around
    SOCIALIZING,        // Interacting with others
    VENDOR_RUN,         // Quick vendor trip
    TRAINER_VISIT,      // Learning new skills
    
    // Passive/Idle
    RESTING,            // In inn, gaining rested XP
    AFK,                // AFK simulation
    TRAVELING,          // Moving between zones
    WAITING,            // Waiting for group/event
    
    MAX_ACTIVITY
};

/**
 * @brief Get human-readable name for activity
 */
inline char const* GetActivityName(ActivityType type)
{
    switch (type)
    {
        case ActivityType::NONE: return "None";
        case ActivityType::QUESTING: return "Questing";
        case ActivityType::GRINDING: return "Grinding";
        case ActivityType::GATHERING: return "Gathering";
        case ActivityType::FISHING: return "Fishing";
        case ActivityType::DUNGEON: return "Dungeon";
        case ActivityType::RAID: return "Raid";
        case ActivityType::PVP_BG: return "Battleground";
        case ActivityType::PVP_ARENA: return "Arena";
        case ActivityType::CRAFTING: return "Crafting";
        case ActivityType::COOKING: return "Cooking";
        case ActivityType::CITY_LIFE: return "City Life";
        case ActivityType::ACHIEVEMENT_HUNT: return "Achievement Hunting";
        case ActivityType::MOUNT_FARM: return "Mount Farming";
        case ActivityType::TRANSMOG_FARM: return "Transmog Farming";
        case ActivityType::REP_GRIND: return "Reputation Grind";
        case ActivityType::EXPLORING: return "Exploring";
        case ActivityType::SOCIALIZING: return "Socializing";
        case ActivityType::VENDOR_RUN: return "Vendor Run";
        case ActivityType::TRAINER_VISIT: return "Trainer Visit";
        case ActivityType::RESTING: return "Resting";
        case ActivityType::AFK: return "AFK";
        case ActivityType::TRAVELING: return "Traveling";
        case ActivityType::WAITING: return "Waiting";
        default: return "Unknown";
    }
}

/**
 * @brief Activity category for grouping
 */
enum class ActivityCategory : uint8
{
    PRIMARY,        // Main gameplay activities
    SECONDARY,      // Supporting activities
    SHORT,          // Quick tasks
    PASSIVE         // Idle/waiting
};

/**
 * @brief Get category for activity type
 */
inline ActivityCategory GetActivityCategory(ActivityType type)
{
    switch (type)
    {
        case ActivityType::QUESTING:
        case ActivityType::GRINDING:
        case ActivityType::GATHERING:
        case ActivityType::FISHING:
        case ActivityType::DUNGEON:
        case ActivityType::RAID:
        case ActivityType::PVP_BG:
        case ActivityType::PVP_ARENA:
            return ActivityCategory::PRIMARY;
            
        case ActivityType::CRAFTING:
        case ActivityType::COOKING:
        case ActivityType::CITY_LIFE:
        case ActivityType::ACHIEVEMENT_HUNT:
        case ActivityType::MOUNT_FARM:
        case ActivityType::TRANSMOG_FARM:
        case ActivityType::REP_GRIND:
            return ActivityCategory::SECONDARY;
            
        case ActivityType::EXPLORING:
        case ActivityType::SOCIALIZING:
        case ActivityType::VENDOR_RUN:
        case ActivityType::TRAINER_VISIT:
            return ActivityCategory::SHORT;
            
        default:
            return ActivityCategory::PASSIVE;
    }
}

} // namespace Playerbot
```

### 3.3 HumanizationManager.h

```cpp
#pragma once

#include "Define.h"
#include "ActivityType.h"
#include "PersonalityProfile.h"
#include "Duration.h"
#include <memory>
#include <functional>

class Player;

namespace Playerbot
{

class BotAI;
class ActivitySessionManager;
class CityLifeBehaviorManager;
class AFKSimulator;
class GatheringSessionManager;
class FishingSessionManager;

/**
 * @brief Central coordinator for human-like bot behavior
 * 
 * Responsible for:
 * - Managing activity sessions (what the bot is doing and for how long)
 * - Coordinating transitions between activities
 * - Applying personality-based modifications
 * - Managing sub-systems (City Life, AFK, etc.)
 */
class TC_GAME_API HumanizationManager
{
public:
    explicit HumanizationManager(Player* bot, BotAI* ai);
    ~HumanizationManager();
    
    // Non-copyable
    HumanizationManager(HumanizationManager const&) = delete;
    HumanizationManager& operator=(HumanizationManager const&) = delete;
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    void Initialize();
    void Update(uint32 diff);
    void Shutdown();
    
    /**
     * @brief Check if humanization is enabled
     */
    bool IsEnabled() const;
    
    /**
     * @brief Enable/disable humanization
     */
    void SetEnabled(bool enabled);
    
    // ========================================================================
    // ACTIVITY SESSION MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Start a new activity session
     * @param type Activity type
     * @param minDuration Minimum session duration (0 = use default)
     * @param maxDuration Maximum session duration (0 = use default)
     * @return true if session started
     */
    bool StartActivity(ActivityType type, 
                       Milliseconds minDuration = Milliseconds(0),
                       Milliseconds maxDuration = Milliseconds(0));
    
    /**
     * @brief End current activity session
     * @param startNext If true, automatically starts next recommended activity
     */
    void EndCurrentActivity(bool startNext = true);
    
    /**
     * @brief Get current activity type
     */
    ActivityType GetCurrentActivity() const;
    
    /**
     * @brief Check if in active session
     */
    bool HasActiveSession() const;
    
    /**
     * @brief Get remaining session time
     */
    Milliseconds GetRemainingSessionTime() const;
    
    /**
     * @brief Get session progress (0.0 - 1.0)
     */
    float GetSessionProgress() const;
    
    /**
     * @brief Should we switch to a different activity?
     * Based on: session time, boredom, interrupts
     */
    bool ShouldSwitchActivity() const;
    
    /**
     * @brief Get recommended next activity
     */
    ActivityType GetRecommendedNextActivity() const;
    
    /**
     * @brief Force activity switch (for external events)
     */
    void ForceActivitySwitch(ActivityType newActivity);
    
    // ========================================================================
    // SUB-SYSTEM ACCESS
    // ========================================================================
    
    ActivitySessionManager* GetSessionManager() const { return _sessionManager.get(); }
    CityLifeBehaviorManager* GetCityLifeManager() const { return _cityLifeManager.get(); }
    AFKSimulator* GetAFKSimulator() const { return _afkSimulator.get(); }
    GatheringSessionManager* GetGatheringManager() const { return _gatheringManager.get(); }
    FishingSessionManager* GetFishingManager() const { return _fishingManager.get(); }
    
    // ========================================================================
    // PERSONALITY
    // ========================================================================
    
    /**
     * @brief Get personality profile
     */
    PersonalityProfile const& GetPersonality() const { return _personality; }
    
    /**
     * @brief Set personality profile
     */
    void SetPersonality(PersonalityProfile const& profile);
    
    /**
     * @brief Generate random personality
     */
    void GenerateRandomPersonality();
    
    /**
     * @brief Apply personality modifier to duration
     */
    Milliseconds ApplyPersonalityModifier(Milliseconds baseDuration) const;
    
    // ========================================================================
    // AFK SIMULATION
    // ========================================================================
    
    /**
     * @brief Check if should go AFK
     */
    bool ShouldGoAFK() const;
    
    /**
     * @brief Start AFK behavior
     */
    void StartAFK(Milliseconds duration = Milliseconds(0));
    
    /**
     * @brief End AFK behavior
     */
    void EndAFK();
    
    /**
     * @brief Check if currently AFK
     */
    bool IsAFK() const;
    
    // ========================================================================
    // CITY LIFE
    // ========================================================================
    
    /**
     * @brief Check if in city and should do city activities
     */
    bool ShouldDoCityLife() const;
    
    /**
     * @brief Start city life behaviors
     */
    void StartCityLife();
    
    /**
     * @brief Check if in city life mode
     */
    bool IsInCityLife() const;
    
    // ========================================================================
    // CALLBACKS
    // ========================================================================
    
    using ActivityCallback = std::function<void(ActivityType oldActivity, ActivityType newActivity)>;
    
    /**
     * @brief Register callback for activity changes
     */
    void OnActivityChange(ActivityCallback callback);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct HumanizationStats
    {
        uint32 sessionsStarted = 0;
        uint32 sessionsCompleted = 0;
        uint32 activitySwitches = 0;
        uint32 afkCount = 0;
        Milliseconds totalPlayTime{0};
        Milliseconds totalAFKTime{0};
        std::unordered_map<ActivityType, Milliseconds> timePerActivity;
    };
    
    HumanizationStats const& GetStats() const { return _stats; }
    
private:
    // Update helpers
    void UpdateSession(uint32 diff);
    void UpdateAFK(uint32 diff);
    void CheckForActivitySwitch();
    
    // Transition helpers
    ActivityType SelectNextActivity() const;
    bool CanTransitionTo(ActivityType from, ActivityType to) const;
    float GetTransitionProbability(ActivityType from, ActivityType to) const;
    
    // Notification
    void NotifyActivityChange(ActivityType oldActivity, ActivityType newActivity);
    
    Player* _bot;
    BotAI* _ai;
    
    // Sub-managers
    std::unique_ptr<ActivitySessionManager> _sessionManager;
    std::unique_ptr<CityLifeBehaviorManager> _cityLifeManager;
    std::unique_ptr<AFKSimulator> _afkSimulator;
    std::unique_ptr<GatheringSessionManager> _gatheringManager;
    std::unique_ptr<FishingSessionManager> _fishingManager;
    
    // Personality
    PersonalityProfile _personality;
    
    // State
    bool _initialized = false;
    bool _enabled = true;
    ActivityType _currentActivity = ActivityType::NONE;
    
    // Callbacks
    std::vector<ActivityCallback> _activityCallbacks;
    
    // Statistics
    HumanizationStats _stats;
    
    // Timing
    uint32 _lastUpdateTime = 0;
    static constexpr uint32 UPDATE_INTERVAL = 1000;  // 1 second
};

} // namespace Playerbot
```

[Fortsetzung folgt in Teil 2 wegen Länge...]
