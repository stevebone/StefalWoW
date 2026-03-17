# TrinityCore PlayerBot QuestPickup System - Complete Implementation Guide

## Overview
This document provides a complete reference for implementing the QuestPickup system for TrinityCore PlayerBot module, compatible with WoW 12.0 (The War Within) quest mechanics.

## Required Includes

```cpp
// Core TrinityCore includes
#include "Player.h"
#include "Quest.h"
#include "QuestDef.h"
#include "ObjectMgr.h"
#include "Creature.h"
#include "GameObject.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Language.h"
#include "Log.h"

// Stores
#include "DB2Stores.h"
```

## Quest Class API Reference

### Core Quest Accessors

```cpp
// Quest identification and basic info
uint32 GetQuestId() const;                              // Quest ID
uint32 GetQuestType() const;                            // Quest type (0=normal, etc)
uint32 GetQuestPackageID() const;                       // Quest package ID for grouping
uint32 GetContentTuningId() const;                      // Content tuning for scaling
int32  GetZoneOrSort() const;                           // Zone ID or negative sort ID
uint32 GetQuestInfoID() const;                          // Quest category/type info

// Level requirements
uint32 GetMaxLevel() const;                             // Maximum level to accept quest

// Class/Race requirements
uint32 GetAllowableClasses() const;                     // Bitmask of allowed classes
Trinity::RaceMask<uint64> GetAllowableRaces() const;    // Bitmask of allowed races

// Skill requirements
uint32 GetRequiredSkill() const;                        // Required skill ID
uint32 GetRequiredSkillValue() const;                   // Required skill points

// Reputation requirements
uint32 GetRequiredMinRepFaction() const;                // Faction for min rep requirement
int32  GetRequiredMinRepValue() const;                  // Minimum reputation value
uint32 GetRequiredMaxRepFaction() const;                // Faction for max rep requirement
int32  GetRequiredMaxRepValue() const;                  // Maximum reputation value

// Quest chain information
int32  GetPrevQuestId() const;                          // Previous quest in chain (negative = required)
uint32 GetNextQuestId() const;                          // Next quest offered after completion
int32  GetExclusiveGroup() const;                       // Exclusive group ID
int32  GetBreadcrumbForQuestId() const;                 // Quest this is breadcrumb for
uint32 GetNextQuestInChain() const;                     // Direct next quest in chain

// Source information
uint32 GetSrcItemId() const;                            // Source item that starts quest
uint32 GetSrcItemCount() const;                         // Required count of source item
uint32 GetSrcSpell() const;                             // Spell that gives quest

// Quest text and descriptions
std::string const& GetLogTitle() const;
std::string const& GetLogDescription() const;
std::string const& GetQuestDescription() const;
std::string const& GetAreaDescription() const;
std::string const& GetOfferRewardText() const;
std::string const& GetRequestItemsText() const;
std::string const& GetQuestCompletionLog() const;

// Quest objectives
QuestObjectives const& GetObjectives() const;           // Get all objectives

// Quest flags
uint32 GetFlags() const;                                // QUEST_FLAGS_*
uint32 GetFlagsEx() const;                              // QUEST_FLAGS_EX_*
uint32 GetFlagsEx2() const;                             // QUEST_FLAGS_EX2_*
uint32 GetSpecialFlags() const;                         // QUEST_SPECIAL_FLAGS_*

// Quest state checks
bool HasFlag(QuestFlags flag) const;
bool HasFlagEx(QuestFlagsEx flag) const;
bool HasFlagEx2(QuestFlagsEx2 flag) const;
bool HasSpecialFlag(QuestSpecialFlags flag) const;

// Quest type checks
bool IsDaily() const;
bool IsWeekly() const;
bool IsMonthly() const;
bool IsSeasonal() const;
bool IsRepeatable() const;
bool IsAutoAccept() const;
bool IsWorldQuest() const;
bool IsTurnIn() const;

// Quest availability
static bool IsTakingQuestEnabled(uint32 questId);      // Check if quest is globally available
```

### Quest Objective Structure

```cpp
struct QuestObjective
{
    uint32 ID;                     // Objective ID
    uint32 QuestID;                // Parent quest ID
    uint8  Type;                   // QUEST_OBJECTIVE_* type
    int8   StorageIndex;           // Storage slot index
    int32  ObjectID;               // Target object/creature/item ID
    int32  Amount;                 // Required amount
    uint32 Flags;                  // QUEST_OBJECTIVE_FLAG_*
    uint32 Flags2;                 // Additional flags
    float  ProgressBarWeight;      // Weight for progress bar
    std::string Description;       // Objective description

    bool IsStoringValue() const;  // Check if tracks numeric progress
    bool IsStoringFlag() const;   // Check if tracks boolean completion
};
```

## Player Quest API Reference

### Quest Status and State

```cpp
// Quest status checking
QuestStatus GetQuestStatus(uint32 quest_id) const;
bool GetQuestRewardStatus(uint32 quest_id) const;       // Is quest rewarded
bool IsQuestRewarded(uint32 quest_id) const;            // Alternative check
size_t GetRewardedQuestCount() const;                   // Total rewarded quests

// Quest log management
uint16 FindQuestSlot(uint32 quest_id) const;           // Find quest in log
uint32 GetQuestSlotQuestId(uint16 slot) const;         // Get quest at slot
uint32 GetQuestSlotState(uint16 slot) const;           // Get quest state flags
```

### Quest Eligibility Checking

```cpp
// Main eligibility checks
bool CanSeeStartQuest(Quest const* quest) const;        // Can see quest marker
bool CanTakeQuest(Quest const* quest, bool msg) const;  // Can accept quest
bool CanAddQuest(Quest const* quest, bool msg) const;   // Can add to quest log

// Detailed requirement checks
bool SatisfyQuestLevel(Quest const* qInfo, bool msg) const;
bool SatisfyQuestMinLevel(Quest const* qInfo, bool msg) const;
bool SatisfyQuestMaxLevel(Quest const* qInfo, bool msg) const;
bool SatisfyQuestClass(Quest const* qInfo, bool msg) const;
bool SatisfyQuestRace(Quest const* qInfo, bool msg) const;
bool SatisfyQuestSkill(Quest const* qInfo, bool msg) const;
bool SatisfyQuestReputation(Quest const* qInfo, bool msg) const;
bool SatisfyQuestMinReputation(Quest const* qInfo, bool msg) const;
bool SatisfyQuestMaxReputation(Quest const* qInfo, bool msg) const;

// Quest chain checks
bool SatisfyQuestPreviousQuest(Quest const* qInfo, bool msg) const;
bool SatisfyQuestDependentPreviousQuests(Quest const* qInfo, bool msg) const;
bool SatisfyQuestBreadcrumbQuest(Quest const* qInfo, bool msg) const;
bool SatisfyQuestDependentBreadcrumbQuests(Quest const* qInfo, bool msg) const;
bool SatisfyQuestDependentQuests(Quest const* qInfo, bool msg) const;
bool SatisfyQuestStatus(Quest const* qInfo, bool msg) const;
bool SatisfyQuestConditions(Quest const* qInfo, bool msg) const;

// Quest interaction
QuestGiverStatus GetQuestDialogStatus(Object const* questGiver) const;
Quest const* GetNextQuest(Object const* questGiver, Quest const* quest) const;
```

### Quest Addition and Completion

```cpp
// Adding quests
void AddQuest(Quest const* quest, Object* questGiver);
void AddQuestAndCheckCompletion(Quest const* quest, Object* questGiver);

// Completing quests
bool CanCompleteQuest(uint32 quest_id, uint32 ignoredQuestObjectiveId = 0);
void CompleteQuest(uint32 quest_id);
void RewardQuest(Quest const* quest, LootItemType rewardType, uint32 rewardId, Object* questGiver, bool announce = true);

// Abandoning quests
void AbandonQuest(uint32 quest_id);
```

## ObjectMgr Quest Relations API

### Quest Relation Queries

```cpp
// Get quests offered by creatures
QuestRelationResult GetCreatureQuestRelations(uint32 entry) const;
QuestRelationResult GetCreatureQuestInvolvedRelations(uint32 entry) const;

// Get quests offered by gameobjects
QuestRelationResult GetGOQuestRelations(uint32 entry) const;
QuestRelationResult GetGOQuestInvolvedRelations(uint32 entry) const;

// Iteration over quest relations
for (uint32 questId : sObjectMgr->GetCreatureQuestRelations(creatureEntry))
{
    // Process each quest offered by creature
}
```

## Quest Giver Types and Detection

### 1. Creature Quest Givers

```cpp
bool IsCreatureQuestGiver(Creature* creature)
{
    return creature && creature->IsQuestGiver();
}

std::vector<uint32> GetCreatureQuests(Creature* creature)
{
    std::vector<uint32> quests;
    if (!creature)
        return quests;

    // Get starter quests
    for (uint32 questId : sObjectMgr->GetCreatureQuestRelations(creature->GetEntry()))
    {
        if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            quests.push_back(questId);
    }

    // Get ender quests
    for (uint32 questId : sObjectMgr->GetCreatureQuestInvolvedRelations(creature->GetEntry()))
    {
        if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            quests.push_back(questId);
    }

    return quests;
}
```

### 2. GameObject Quest Givers

```cpp
bool IsGameObjectQuestGiver(GameObject* go)
{
    return go && go->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER;
}

std::vector<uint32> GetGameObjectQuests(GameObject* go)
{
    std::vector<uint32> quests;
    if (!go)
        return quests;

    // Get starter quests
    for (uint32 questId : sObjectMgr->GetGOQuestRelations(go->GetEntry()))
    {
        if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            quests.push_back(questId);
    }

    // Get ender quests
    for (uint32 questId : sObjectMgr->GetGOQuestInvolvedRelations(go->GetEntry()))
    {
        if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            quests.push_back(questId);
    }

    return quests;
}
```

### 3. Item Quest Starters

```cpp
bool IsQuestStarterItem(ItemTemplate const* itemTemplate)
{
    return itemTemplate && itemTemplate->GetStartQuest() != 0;
}

uint32 GetItemStartQuest(ItemTemplate const* itemTemplate)
{
    return itemTemplate ? itemTemplate->GetStartQuest() : 0;
}
```

## Scanning for Nearby Quest Givers

### Complete Quest Giver Scanner Implementation

```cpp
struct QuestGiverInfo
{
    ObjectGuid guid;
    uint32 entry;
    float distance;
    std::vector<uint32> availableQuests;
    QuestGiverStatus status;
};

class QuestGiverScanner
{
public:
    static std::vector<QuestGiverInfo> ScanNearbyQuestGivers(Player* bot, float range = 100.0f)
    {
        std::vector<QuestGiverInfo> questGivers;

        if (!bot || !bot->IsInWorld())
            return questGivers;

        // Scan for creature quest givers
        ScanCreatureQuestGivers(bot, range, questGivers);

        // Scan for gameobject quest givers
        ScanGameObjectQuestGivers(bot, range, questGivers);

        // Sort by distance
        std::sort(questGivers.begin(), questGivers.end(),
            [](const QuestGiverInfo& a, const QuestGiverInfo& b) {
                return a.distance < b.distance;
            });

        return questGivers;
    }

private:
    static void ScanCreatureQuestGivers(Player* bot, float range, std::vector<QuestGiverInfo>& questGivers)
    {
        std::list<Creature*> creatures;
        Trinity::CreatureListSearcher searcher(bot, creatures, [](Creature* c) {
            return c->IsQuestGiver() && c->IsAlive();
        });

        Cell::VisitGridObjects(bot, searcher, range);

        for (Creature* creature : creatures)
        {
            QuestGiverInfo info;
            info.guid = creature->GetGUID();
            info.entry = creature->GetEntry();
            info.distance = bot->GetDistance(creature);
            info.status = bot->GetQuestDialogStatus(creature);

            // Get available quests
            for (uint32 questId : sObjectMgr->GetCreatureQuestRelations(creature->GetEntry()))
            {
                if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
                {
                    if (bot->CanTakeQuest(quest, false))
                        info.availableQuests.push_back(questId);
                }
            }

            if (!info.availableQuests.empty() ||
                info.status != QuestGiverStatus::None)
            {
                questGivers.push_back(info);
            }
        }
    }

    static void ScanGameObjectQuestGivers(Player* bot, float range, std::vector<QuestGiverInfo>& questGivers)
    {
        std::list<GameObject*> gameObjects;
        Trinity::GameObjectListSearcher searcher(bot, gameObjects, [](GameObject* go) {
            return go->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER;
        });

        Cell::VisitGridObjects(bot, searcher, range);

        for (GameObject* go : gameObjects)
        {
            QuestGiverInfo info;
            info.guid = go->GetGUID();
            info.entry = go->GetEntry();
            info.distance = bot->GetDistance(go);
            info.status = bot->GetQuestDialogStatus(go);

            // Get available quests
            for (uint32 questId : sObjectMgr->GetGOQuestRelations(go->GetEntry()))
            {
                if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
                {
                    if (bot->CanTakeQuest(quest, false))
                        info.availableQuests.push_back(questId);
                }
            }

            if (!info.availableQuests.empty() ||
                info.status != QuestGiverStatus::None)
            {
                questGivers.push_back(info);
            }
        }
    }
};
```

## Complete Quest Pickup Implementation

### QuestPickupManager Class

```cpp
class QuestPickupManager
{
public:
    // Main quest pickup logic
    static bool PickupQuest(Player* bot, uint32 questId, Object* questGiver)
    {
        if (!bot || !questGiver)
            return false;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
        {
            TC_LOG_ERROR("module.playerbot", "QuestPickupManager: Quest {} not found", questId);
            return false;
        }

        // Check if bot can take quest
        if (!CanPickupQuest(bot, quest))
            return false;

        // Check quest giver validity
        if (!IsValidQuestGiver(questGiver, questId))
            return false;

        // Add quest to bot's log
        bot->AddQuestAndCheckCompletion(quest, questGiver);

        TC_LOG_DEBUG("module.playerbot", "Bot {} picked up quest {} from {}",
            bot->GetName(), quest->GetLogTitle(), questGiver->GetName());

        return true;
    }

    // Check if bot can pickup quest
    static bool CanPickupQuest(Player* bot, Quest const* quest)
    {
        if (!bot || !quest)
            return false;

        // Check if quest is globally enabled
        if (!Quest::IsTakingQuestEnabled(quest->GetQuestId()))
            return false;

        // Check if bot can see quest (level, class, race requirements)
        if (!bot->CanSeeStartQuest(quest))
            return false;

        // Check if bot can take quest (all requirements)
        if (!bot->CanTakeQuest(quest, false))
            return false;

        // Check if bot can add quest to log
        if (!bot->CanAddQuest(quest, false))
            return false;

        return true;
    }

    // Validate quest giver
    static bool IsValidQuestGiver(Object* questGiver, uint32 questId)
    {
        if (!questGiver)
            return false;

        switch (questGiver->GetTypeId())
        {
            case TYPEID_UNIT:
            {
                Creature* creature = questGiver->ToCreature();
                if (!creature->IsQuestGiver())
                    return false;

                // Check if creature offers this quest
                for (uint32 id : sObjectMgr->GetCreatureQuestRelations(creature->GetEntry()))
                {
                    if (id == questId)
                        return true;
                }
                break;
            }
            case TYPEID_GAMEOBJECT:
            {
                GameObject* go = questGiver->ToGameObject();
                if (go->GetGoType() != GAMEOBJECT_TYPE_QUESTGIVER)
                    return false;

                // Check if gameobject offers this quest
                for (uint32 id : sObjectMgr->GetGOQuestRelations(go->GetEntry()))
                {
                    if (id == questId)
                        return true;
                }
                break;
            }
            case TYPEID_ITEM:
            {
                Item* item = static_cast<Item*>(questGiver);
                ItemTemplate const* itemTemplate = item->GetTemplate();
                if (itemTemplate && itemTemplate->GetStartQuest() == questId)
                    return true;
                break;
            }
            default:
                return false;
        }

        return false;
    }

    // Get best quest to pickup from available ones
    static uint32 SelectBestQuest(Player* bot, std::vector<uint32> const& availableQuests)
    {
        if (availableQuests.empty())
            return 0;

        // Priority system for quest selection
        struct QuestPriority
        {
            uint32 questId;
            int32 priority;
        };

        std::vector<QuestPriority> priorities;

        for (uint32 questId : availableQuests)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            int32 priority = 0;

            // Higher priority for main story quests
            if (quest->HasFlag(QUEST_FLAGS_WELL_KNOWN))
                priority += 100;

            // Higher priority for class quests
            if (quest->GetAllowableClasses() != 0)
                priority += 50;

            // Higher priority for quests at bot's level
            int32 levelDiff = std::abs(static_cast<int32>(bot->GetLevel()) -
                                       static_cast<int32>(quest->GetQuestLevel()));
            priority -= levelDiff * 5;

            // Higher priority for quests in current zone
            if (quest->GetZoneOrSort() == bot->GetZoneId())
                priority += 30;

            // Lower priority for PvP quests if bot is PvE focused
            if (quest->HasFlag(QUEST_FLAGS_FLAGS_PVP))
                priority -= 20;

            priorities.push_back({questId, priority});
        }

        if (priorities.empty())
            return 0;

        // Sort by priority
        std::sort(priorities.begin(), priorities.end(),
            [](const QuestPriority& a, const QuestPriority& b) {
                return a.priority > b.priority;
            });

        return priorities[0].questId;
    }
};
```

## Quest Eligibility Deep Check Implementation

```cpp
class QuestEligibilityChecker
{
public:
    struct EligibilityResult
    {
        bool eligible;
        QuestFailedReason failReason;
        std::string failMessage;
    };

    static EligibilityResult CheckFullEligibility(Player* bot, Quest const* quest)
    {
        EligibilityResult result = {true, QUEST_ERR_NONE, ""};

        if (!bot || !quest)
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_NONE;
            result.failMessage = "Invalid bot or quest";
            return result;
        }

        // Check level requirements
        if (!CheckLevelRequirements(bot, quest, result))
            return result;

        // Check class/race requirements
        if (!CheckClassRaceRequirements(bot, quest, result))
            return result;

        // Check skill requirements
        if (!CheckSkillRequirements(bot, quest, result))
            return result;

        // Check reputation requirements
        if (!CheckReputationRequirements(bot, quest, result))
            return result;

        // Check quest chain requirements
        if (!CheckQuestChainRequirements(bot, quest, result))
            return result;

        // Check quest status
        if (!CheckQuestStatus(bot, quest, result))
            return result;

        // Check inventory space
        if (!CheckInventorySpace(bot, quest, result))
            return result;

        return result;
    }

private:
    static bool CheckLevelRequirements(Player* bot, Quest const* quest, EligibilityResult& result)
    {
        // Check minimum level
        if (!bot->SatisfyQuestMinLevel(quest, false))
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_FAILED_LOW_LEVEL;
            result.failMessage = "Level too low";
            return false;
        }

        // Check maximum level
        if (quest->GetMaxLevel() > 0 && bot->GetLevel() > quest->GetMaxLevel())
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_NONE;
            result.failMessage = "Level too high";
            return false;
        }

        return true;
    }

    static bool CheckClassRaceRequirements(Player* bot, Quest const* quest, EligibilityResult& result)
    {
        // Check class requirement
        if (!bot->SatisfyQuestClass(quest, false))
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_NONE;
            result.failMessage = "Wrong class";
            return false;
        }

        // Check race requirement
        if (!bot->SatisfyQuestRace(quest, false))
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_FAILED_WRONG_RACE;
            result.failMessage = "Wrong race";
            return false;
        }

        return true;
    }

    static bool CheckSkillRequirements(Player* bot, Quest const* quest, EligibilityResult& result)
    {
        if (!bot->SatisfyQuestSkill(quest, false))
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_NONE;
            result.failMessage = "Missing required skill";
            return false;
        }

        return true;
    }

    static bool CheckReputationRequirements(Player* bot, Quest const* quest, EligibilityResult& result)
    {
        if (!bot->SatisfyQuestReputation(quest, false))
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_NONE;
            result.failMessage = "Reputation requirements not met";
            return false;
        }

        return true;
    }

    static bool CheckQuestChainRequirements(Player* bot, Quest const* quest, EligibilityResult& result)
    {
        // Check previous quest requirements
        if (!bot->SatisfyQuestPreviousQuest(quest, false))
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_NONE;
            result.failMessage = "Previous quest not completed";
            return false;
        }

        // Check dependent quests
        if (!bot->SatisfyQuestDependentQuests(quest, false))
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_NONE;
            result.failMessage = "Dependent quests not satisfied";
            return false;
        }

        return true;
    }

    static bool CheckQuestStatus(Player* bot, Quest const* quest, EligibilityResult& result)
    {
        QuestStatus status = bot->GetQuestStatus(quest->GetQuestId());

        // Check if already on quest
        if (status == QUEST_STATUS_INCOMPLETE)
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_ALREADY_ON1;
            result.failMessage = "Already on quest";
            return false;
        }

        // Check if already completed (non-repeatable)
        if (status == QUEST_STATUS_COMPLETE || status == QUEST_STATUS_REWARDED)
        {
            if (!quest->IsRepeatable() && !quest->IsDaily() && !quest->IsWeekly())
            {
                result.eligible = false;
                result.failReason = QUEST_ERR_ALREADY_DONE;
                result.failMessage = "Quest already completed";
                return false;
            }
        }

        return true;
    }

    static bool CheckInventorySpace(Player* bot, Quest const* quest, EligibilityResult& result)
    {
        // Check if quest log has space
        if (bot->FindQuestSlot(0) >= MAX_QUEST_LOG_SIZE)
        {
            result.eligible = false;
            result.failReason = QUEST_ERR_NONE;
            result.failMessage = "Quest log full";
            return false;
        }

        return true;
    }
};
```

## Quest Status Enumeration Reference

```cpp
enum QuestStatus : uint8
{
    QUEST_STATUS_NONE       = 0,   // Player has no relation to quest
    QUEST_STATUS_COMPLETE   = 1,   // Player has completed all objectives
    QUEST_STATUS_INCOMPLETE = 3,   // Player is currently on quest
    QUEST_STATUS_FAILED     = 5,   // Player has failed the quest
    QUEST_STATUS_REWARDED   = 6,   // Player has been rewarded (DB only)
};
```

## Quest Giver Status Reference

```cpp
enum class QuestGiverStatus : uint64
{
    None                    = 0x000000000000,  // No interaction available
    Quest                   = 0x000000400000,  // Yellow exclamation mark
    DailyQuest             = 0x000000800000,  // Blue exclamation mark
    Reward                 = 0x000000002000,  // Yellow question mark
    RepeatableQuest        = 0x000001000000,  // Blue exclamation mark (repeatable)
    // Many more status types for different quest types
};
```

## Usage Example

```cpp
class PlayerBotQuestAI
{
public:
    void UpdateQuestPickup(Player* bot)
    {
        // Scan for nearby quest givers
        auto questGivers = QuestGiverScanner::ScanNearbyQuestGivers(bot, 50.0f);

        for (const auto& giverInfo : questGivers)
        {
            // Skip if no available quests
            if (giverInfo.availableQuests.empty())
                continue;

            // Get quest giver object
            Object* questGiver = nullptr;
            if (giverInfo.guid.IsCreature())
                questGiver = bot->GetMap()->GetCreature(giverInfo.guid);
            else if (giverInfo.guid.IsGameObject())
                questGiver = bot->GetMap()->GetGameObject(giverInfo.guid);

            if (!questGiver)
                continue;

            // Select best quest to pickup
            uint32 questId = QuestPickupManager::SelectBestQuest(bot, giverInfo.availableQuests);
            if (!questId)
                continue;

            // Try to pickup quest
            if (QuestPickupManager::PickupQuest(bot, questId, questGiver))
            {
                TC_LOG_INFO("module.playerbot", "Bot {} picked up quest {} from {}",
                    bot->GetName(), questId, questGiver->GetName());
                break; // Only pickup one quest per update
            }
        }
    }
};
```

## Important Notes

1. **Quest Log Limit**: Players can have maximum of 35 quests (MAX_QUEST_LOG_SIZE)
2. **Quest Status Updates**: Always use Player::SetQuestStatus() for status changes
3. **Quest Objectives**: Modern quests use objective-based system, not old item/kill counts
4. **Phase Handling**: Quest givers may be in different phases - use PhaseShift checks
5. **Scaling**: Use ContentTuningId for proper level scaling in modern content
6. **Spell Casts**: Some quests trigger spells on accept/complete via quest->GetSrcSpell()
7. **Quest Sharing**: Check QUEST_FLAGS_SHARABLE for group quest sharing
8. **Auto Accept**: Check quest->IsAutoAccept() for automatic quest acceptance
9. **World Quests**: Special handling needed for world quests (time-limited)
10. **Conditional Content**: Some quest text/objectives change based on player conditions

## Thread Safety Considerations

- All quest template data is read-only after server startup
- Player quest status must be modified on main thread only
- Use proper locking when accessing shared quest giver lists
- Grid searches should use visitor pattern for thread safety

## Performance Optimization

- Cache frequently accessed quest templates
- Limit quest giver scan range to reasonable distance
- Batch quest eligibility checks when possible
- Use early-out pattern in eligibility checking
- Consider quest priority caching for repeated checks