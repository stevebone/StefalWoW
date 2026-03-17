/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "UnifiedQuestManager.h"
#include "QuestValidation.h"  // For complete ValidationContext type
#include "QuestTurnIn.h"  // For complete TurnInMetrics type
#include "Core/PlayerBotHelpers.h"  // GetBotAI, GetGameSystems
#include "Core/Managers/GameSystemsManager.h"  // Complete type for IGameSystemsManager
#include "Log.h"
#include "Timer.h"
#include <sstream>
#include "GameTime.h"
#include "ObjectMgr.h"  // For sObjectMgr->GetQuestTemplate
#include "Player.h"  // For Player::GetReputationRank, HasItemCount, etc.
#include "QuestDef.h"  // For Quest template and objective types
#include "Bag.h"  // For inventory bag checks
#include "RaceMask.h"  // For Trinity::RaceMask

namespace Playerbot
{

// ============================================================================
// SINGLETON MANAGEMENT
// ============================================================================

UnifiedQuestManager* UnifiedQuestManager::instance()
{
    static UnifiedQuestManager instance;
    return &instance;
}

UnifiedQuestManager::UnifiedQuestManager()
    : _pickup(std::make_unique<PickupModule>())
    , _completion(std::make_unique<CompletionModule>())
    , _validation(std::make_unique<ValidationModule>())
    , _turnIn(std::make_unique<TurnInModule>())
    , _dynamic(std::make_unique<DynamicModule>())
{
    TC_LOG_INFO("playerbot.quest", "UnifiedQuestManager initialized - consolidating 5 quest managers");
}

UnifiedQuestManager::~UnifiedQuestManager()
{
    TC_LOG_INFO("playerbot.quest", "UnifiedQuestManager destroyed");
}

// ============================================================================
// PICKUP MODULE IMPLEMENTATION (delegates to QuestPickup)
// ============================================================================

bool UnifiedQuestManager::PickupModule::PickupQuest(uint32 questId, Player* bot, uint32 questGiverGuid)
{
    _questsPickedUp++;
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->PickupQuest(questId, bot, questGiverGuid);
    return {};
}

bool UnifiedQuestManager::PickupModule::PickupQuestFromGiver(Player* bot, uint32 questGiverGuid, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->PickupQuestFromGiver(bot, questGiverGuid, questId);
    return {};
}

void UnifiedQuestManager::PickupModule::PickupAvailableQuests(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestPickup()->PickupAvailableQuests(bot);
}

void UnifiedQuestManager::PickupModule::PickupQuestsInArea(Player* bot, float radius)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestPickup()->PickupQuestsInArea(bot, radius);
}

std::vector<uint32> UnifiedQuestManager::PickupModule::DiscoverNearbyQuests(Player* bot, float scanRadius)
{
    _questsDiscovered++;
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->DiscoverNearbyQuests(bot, scanRadius);
    return {};
}

std::vector<QuestGiverInfo> UnifiedQuestManager::PickupModule::ScanForQuestGivers(Player* bot, float scanRadius)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->ScanForQuestGivers(bot, scanRadius);
    return {};
}

std::vector<uint32> UnifiedQuestManager::PickupModule::GetAvailableQuestsFromGiver(uint32 questGiverGuid, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->GetAvailableQuestsFromGiver(questGiverGuid, bot);
    return {};
}

QuestEligibility UnifiedQuestManager::PickupModule::CheckQuestEligibility(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->CheckQuestEligibility(questId, bot);
    return {};
}

bool UnifiedQuestManager::PickupModule::CanAcceptQuest(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->CanAcceptQuest(questId, bot);
    return {};
}

bool UnifiedQuestManager::PickupModule::MeetsQuestRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->MeetsQuestRequirements(questId, bot);
    return {};
}

std::vector<uint32> UnifiedQuestManager::PickupModule::FilterQuests(const std::vector<uint32>& questIds, Player* bot, const QuestPickupFilter& filter)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->FilterQuests(questIds, bot, filter);
    return {};
}

std::vector<uint32> UnifiedQuestManager::PickupModule::PrioritizeQuests(const std::vector<uint32>& questIds, Player* bot, QuestAcceptanceStrategy strategy)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->PrioritizeQuests(questIds, bot, strategy);
    return {};
}

bool UnifiedQuestManager::PickupModule::ShouldAcceptQuest(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestPickup()->ShouldAcceptQuest(questId, bot);
    return {};
}

// ============================================================================
// COMPLETION MODULE IMPLEMENTATION (delegates to QuestCompletion)
// ============================================================================

bool UnifiedQuestManager::CompletionModule::StartQuestCompletion(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestCompletion()->StartQuestCompletion(questId, bot);
    return {};
}

void UnifiedQuestManager::CompletionModule::UpdateQuestProgress(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->UpdateQuestProgress(bot);
}

void UnifiedQuestManager::CompletionModule::CompleteQuest(uint32 questId, Player* bot)
{
    _questsCompleted++;
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->CompleteQuest(questId, bot);
}

bool UnifiedQuestManager::CompletionModule::TurnInQuest(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestCompletion()->TurnInQuest(questId, bot);
    return {};
}

void UnifiedQuestManager::CompletionModule::TrackQuestObjectives(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->TrackQuestObjectives(bot);
}

void UnifiedQuestManager::CompletionModule::ExecuteObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->ExecuteObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::UpdateObjectiveProgress(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->UpdateObjectiveProgress(bot, questId, objectiveIndex);
}

bool UnifiedQuestManager::CompletionModule::IsObjectiveComplete(const QuestObjectiveData& objective)
{
    _objectivesCompleted++;
    // Full quest objective tracking with progress percentage calculation
    // Returns false as default fallback behavior
    // Full implementation should:
    // - Accept Player* parameter to check player-specific quest progress
    // - Query IQuestCompletion interface for objective state (see Core/DI/Interfaces/IQuestCompletion.h)
    // - Check QuestObjectiveData against player's quest log via TrinityCore Quest API
    // - Handle all objective types: QUEST_OBJECTIVE_MONSTER (kill), QUEST_OBJECTIVE_ITEM (collect), etc.
    // - Validate objective criteria counts match required counts
    // Reference: Phase 2 refactoring to add player context to QuestObjectiveData
    return false;
}

void UnifiedQuestManager::CompletionModule::HandleKillObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->HandleKillObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::HandleCollectObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->HandleCollectObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::HandleTalkToNpcObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->HandleTalkToNpcObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::HandleLocationObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->HandleLocationObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::HandleGameObjectObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->HandleGameObjectObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::HandleSpellCastObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->HandleSpellCastObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::HandleEmoteObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->HandleEmoteObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::HandleEscortObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->HandleEscortObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::NavigateToObjective(Player* bot, const QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->NavigateToObjective(bot, objective);
}

bool UnifiedQuestManager::CompletionModule::FindObjectiveTarget(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestCompletion()->FindObjectiveTarget(bot, objective);
    return {};
}

std::vector<Position> UnifiedQuestManager::CompletionModule::GetObjectiveLocations(const QuestObjectiveData& objective)
{
    // INTEGRATION REQUIRED: Add Player* parameter for objective location lookup
    // Implementation depends on:
    // - IQuestCompletion interface update to include GetObjectiveLocations method with Player* parameter
    // - Access to bot's current map and phase for accurate location data
    // See: src/modules/Playerbot/Core/DI/Interfaces/IQuestCompletion.h for required interface
    return {};
}

Position UnifiedQuestManager::CompletionModule::GetOptimalObjectivePosition(Player* bot, const QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestCompletion()->GetOptimalObjectivePosition(bot, objective);
    return {};
}

void UnifiedQuestManager::CompletionModule::CoordinateGroupQuestCompletion(Group* group, uint32 questId)
{
    // INTEGRATION REQUIRED: Add Player* parameter or redesign for group-level operations
    // Implementation depends on:
    // - IQuestCompletion interface update to support group coordination
    // - Group member iteration logic to access individual bot contexts
    // See: src/modules/Playerbot/Core/DI/Interfaces/IQuestCompletion.h for required interface
}

void UnifiedQuestManager::CompletionModule::ShareObjectiveProgress(Group* group, uint32 questId)
{
    // INTEGRATION REQUIRED: Add Player* parameter or redesign for group-level operations
    // Implementation depends on:
    // - IQuestCompletion interface update to support progress sharing
    // - Group member iteration logic to access individual bot contexts
    // See: src/modules/Playerbot/Core/DI/Interfaces/IQuestCompletion.h for required interface
}

void UnifiedQuestManager::CompletionModule::SynchronizeGroupObjectives(Group* group, uint32 questId)
{
    // INTEGRATION REQUIRED: Add Player* parameter or redesign for group-level operations
    // Implementation depends on:
    // - IQuestCompletion interface update to support objective synchronization
    // - Group member iteration logic to access individual bot contexts
    // See: src/modules/Playerbot/Core/DI/Interfaces/IQuestCompletion.h for required interface
}

void UnifiedQuestManager::CompletionModule::HandleGroupObjectiveConflict(Group* group, uint32 questId, uint32 objectiveIndex)
{
    // INTEGRATION REQUIRED: Add Player* parameter or redesign for group-level operations
    // Implementation depends on:
    // - IQuestCompletion interface update to support conflict resolution
    // - Group member iteration logic to access individual bot contexts
    // See: src/modules/Playerbot/Core/DI/Interfaces/IQuestCompletion.h for required interface
}

void UnifiedQuestManager::CompletionModule::OptimizeQuestCompletionOrder(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->OptimizeQuestCompletionOrder(bot);
}

void UnifiedQuestManager::CompletionModule::OptimizeObjectiveSequence(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->OptimizeObjectiveSequence(bot, questId);
}

void UnifiedQuestManager::CompletionModule::FindEfficientCompletionPath(Player* bot, const std::vector<uint32>& questIds)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->FindEfficientCompletionPath(bot, questIds);
}

void UnifiedQuestManager::CompletionModule::MinimizeTravelTime(Player* bot, const std::vector<QuestObjectiveData>& objectives)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->MinimizeTravelTime(bot, objectives);
}

void UnifiedQuestManager::CompletionModule::DetectStuckState(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->DetectStuckState(bot, questId);
}

void UnifiedQuestManager::CompletionModule::HandleStuckObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->HandleStuckObjective(bot, objective);
}

void UnifiedQuestManager::CompletionModule::RecoverFromStuckState(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->RecoverFromStuckState(bot, questId);
}

void UnifiedQuestManager::CompletionModule::SkipProblematicObjective(Player* bot, QuestObjectiveData& objective)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestCompletion()->SkipProblematicObjective(bot, objective);
}

// ============================================================================
// VALIDATION MODULE IMPLEMENTATION (delegates to QuestValidation)
// ============================================================================

bool UnifiedQuestManager::ValidationModule::ValidateQuest(uint32 questId, Player* bot)
{
    _validationsPerformed++;
    bool result = false;
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        result = systems->GetQuestValidation()->ValidateQuestAcceptance(questId, bot);
    if (result)
        _validationsPassed++;
    return result;
}

bool UnifiedQuestManager::ValidationModule::ValidateQuestRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateLevelRequirements(questId, bot);
    return false;
}

std::vector<std::string> UnifiedQuestManager::ValidationModule::GetValidationErrors(uint32 questId, Player* bot)
{
    // INTEGRATION REQUIRED: Implement validation error reporting
    // Implementation depends on:
    // - IQuestValidation interface update to include GetValidationErrors method
    // - Centralized validation error collection and formatting
    // See: src/modules/Playerbot/Core/DI/Interfaces/IQuestValidation.h for required interface
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateLevelRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateLevelRequirements(questId, bot);
    return false;
}

bool UnifiedQuestManager::ValidationModule::ValidateClassRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateClassRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateRaceRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateRaceRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateSkillRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateSkillRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateQuestPrerequisites(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateQuestPrerequisites(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateQuestChain(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateQuestChainPosition(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::HasCompletedPrerequisites(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->HasCompletedPrerequisiteQuests(questId, bot);
    return {};
}

std::vector<uint32> UnifiedQuestManager::ValidationModule::GetMissingPrerequisites(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->GetMissingPrerequisites(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateReputationRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateReputationRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateFactionRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateFactionRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::HasRequiredReputation(uint32 questId, Player* bot, uint32 factionId)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // If a specific faction is requested, check that faction
    if (factionId != 0)
    {
        // For faction-specific checks, we need the required rep value from quest objectives
        for (QuestObjective const& objective : quest->GetObjectives())
        {
            if (objective.Type == QUEST_OBJECTIVE_MIN_REPUTATION && static_cast<uint32>(objective.ObjectID) == factionId)
            {
                ReputationRank currentRank = bot->GetReputationRank(factionId);
                // objective.Amount contains the required reputation value
                int32 currentRepValue = bot->GetReputation(factionId);
                if (currentRepValue < objective.Amount)
                    return false;
            }
            else if (objective.Type == QUEST_OBJECTIVE_MAX_REPUTATION && static_cast<uint32>(objective.ObjectID) == factionId)
            {
                int32 currentRepValue = bot->GetReputation(factionId);
                if (currentRepValue > objective.Amount)
                    return false;
            }
        }
        return true;
    }

    // Check quest's required min reputation faction
    uint32 requiredMinRepFaction = quest->GetRequiredMinRepFaction();
    if (requiredMinRepFaction)
    {
        int32 requiredMinRepValue = quest->GetRequiredMinRepValue();
        int32 currentRepValue = bot->GetReputation(requiredMinRepFaction);
        if (currentRepValue < requiredMinRepValue)
        {
            TC_LOG_DEBUG("playerbot.quest", "Bot {} lacks required min reputation ({}/{}) for faction {} for quest {}",
                bot->GetName(), currentRepValue, requiredMinRepValue, requiredMinRepFaction, questId);
            return false;
        }
    }

    // Check quest's required max reputation faction
    uint32 requiredMaxRepFaction = quest->GetRequiredMaxRepFaction();
    if (requiredMaxRepFaction)
    {
        int32 requiredMaxRepValue = quest->GetRequiredMaxRepValue();
        int32 currentRepValue = bot->GetReputation(requiredMaxRepFaction);
        if (currentRepValue > requiredMaxRepValue)
        {
            TC_LOG_DEBUG("playerbot.quest", "Bot {} exceeds max reputation ({}/{}) for faction {} for quest {}",
                bot->GetName(), currentRepValue, requiredMaxRepValue, requiredMaxRepFaction, questId);
            return false;
        }
    }

    return true;
}

bool UnifiedQuestManager::ValidationModule::ValidateItemRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check quest item objectives - items player needs to collect
    for (QuestObjective const& objective : quest->GetObjectives())
    {
        if (objective.Type == QUEST_OBJECTIVE_ITEM)
        {
            uint32 itemId = objective.ObjectID;
            uint32 requiredCount = objective.Amount;

            // Check if player has the item (for quest items that need to be in inventory at turn-in)
            uint32 currentCount = bot->GetItemCount(itemId, true);  // true = include bank
            if (currentCount < requiredCount)
            {
                TC_LOG_DEBUG("playerbot.quest", "Bot {} missing item {} ({}/{}) for quest {}",
                    bot->GetName(), itemId, currentCount, requiredCount, questId);
                // This is expected during quest - item objectives are collected
                // Only fail if this is meant for turn-in validation
            }
        }
    }

    // Check source item (quest starter item that should be in inventory)
    uint32 sourceItemId = quest->GetSrcItemId();
    if (sourceItemId)
    {
        uint32 sourceItemCount = quest->GetSrcItemCount();
        if (sourceItemCount == 0)
            sourceItemCount = 1;

        if (!bot->HasItemCount(sourceItemId, sourceItemCount))
        {
            TC_LOG_DEBUG("playerbot.quest", "Bot {} missing source item {} for quest {}",
                bot->GetName(), sourceItemId, questId);
            return false;
        }
    }

    // Check ItemDrop requirements (items that must drop during the quest)
    for (uint32 i = 0; i < QUEST_ITEM_DROP_COUNT; ++i)
    {
        if (quest->ItemDrop[i] != 0)
        {
            // ItemDrop items are collected during quest, no need to validate at acceptance
            // They will be validated when checking quest completion
        }
    }

    return true;
}

bool UnifiedQuestManager::ValidationModule::HasRequiredItems(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check all item objectives
    for (QuestObjective const& objective : quest->GetObjectives())
    {
        if (objective.Type == QUEST_OBJECTIVE_ITEM)
        {
            uint32 itemId = objective.ObjectID;
            int32 requiredCount = objective.Amount;

            if (requiredCount > 0)
            {
                uint32 currentCount = bot->GetItemCount(itemId, true);  // true = include bank
                if (static_cast<int32>(currentCount) < requiredCount)
                {
                    TC_LOG_DEBUG("playerbot.quest", "Bot {} has {}/{} of item {} for quest {}",
                        bot->GetName(), currentCount, requiredCount, itemId, questId);
                    return false;
                }
            }
        }
    }

    // Check ItemDrop items (items that can drop during quest, may be required for turn-in)
    for (uint32 i = 0; i < QUEST_ITEM_DROP_COUNT; ++i)
    {
        uint32 itemId = quest->ItemDrop[i];
        uint32 requiredCount = quest->ItemDropQuantity[i];

        if (itemId != 0 && requiredCount > 0)
        {
            uint32 currentCount = bot->GetItemCount(itemId, true);
            if (currentCount < requiredCount)
            {
                TC_LOG_DEBUG("playerbot.quest", "Bot {} has {}/{} of drop item {} for quest {}",
                    bot->GetName(), currentCount, requiredCount, itemId, questId);
                return false;
            }
        }
    }

    return true;
}

bool UnifiedQuestManager::ValidationModule::HasInventorySpace(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Count how many reward items will be received
    uint32 rewardSlots = 0;

    // Count guaranteed reward items
    for (uint32 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
    {
        if (quest->RewardItemId[i] != 0 && quest->RewardItemCount[i] > 0)
        {
            // Each unique item type needs at least one slot (may need more if stack doesn't fit)
            ++rewardSlots;
        }
    }

    // Note: Choice rewards only give ONE item, not all of them
    // So we don't count choice items as additional slots needed

    // Source item (quest starter item) - will be in inventory already
    if (quest->GetSrcItemId() != 0)
    {
        // Player already has this item, no additional slot needed
    }

    // Check if player has enough free slots
    // Simple check: compare to empty bag slots
    uint32 freeSlots = 0;

    // Count free bag slots
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag* pBag = bot->GetBagByPos(bag))
        {
            for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                if (!pBag->GetItemByPos(slot))
                    ++freeSlots;
            }
        }
    }

    // Check main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        if (!bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            ++freeSlots;
    }

    if (freeSlots < rewardSlots)
    {
        TC_LOG_DEBUG("playerbot.quest", "Bot {} has insufficient inventory space ({}/{}) for quest {} rewards",
            bot->GetName(), freeSlots, rewardSlots, questId);
        return false;
    }

    return true;
}

std::vector<uint32> UnifiedQuestManager::ValidationModule::GetMissingQuestItems(uint32 questId, Player* bot)
{
    std::vector<uint32> missingItems;

    if (!bot)
        return missingItems;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return missingItems;

    // Check item objectives
    for (QuestObjective const& objective : quest->GetObjectives())
    {
        if (objective.Type == QUEST_OBJECTIVE_ITEM)
        {
            uint32 itemId = static_cast<uint32>(objective.ObjectID);
            int32 requiredCount = objective.Amount;

            if (requiredCount > 0)
            {
                uint32 currentCount = bot->GetItemCount(itemId, true);  // true = include bank
                if (static_cast<int32>(currentCount) < requiredCount)
                {
                    // Calculate how many more are needed
                    uint32 missing = requiredCount - currentCount;
                    // Add itemId to missing list (once per item type, not per missing count)
                    missingItems.push_back(itemId);

                    TC_LOG_DEBUG("playerbot.quest", "Bot {} missing {} of item {} for quest {}",
                        bot->GetName(), missing, itemId, questId);
                }
            }
        }
    }

    // Check ItemDrop items (items that can drop during quest)
    for (uint32 i = 0; i < QUEST_ITEM_DROP_COUNT; ++i)
    {
        uint32 itemId = quest->ItemDrop[i];
        uint32 requiredCount = quest->ItemDropQuantity[i];

        if (itemId != 0 && requiredCount > 0)
        {
            uint32 currentCount = bot->GetItemCount(itemId, true);
            if (currentCount < requiredCount)
            {
                // Avoid duplicates if same item is in both objectives and ItemDrop
                bool alreadyListed = false;
                for (uint32 existingId : missingItems)
                {
                    if (existingId == itemId)
                    {
                        alreadyListed = true;
                        break;
                    }
                }

                if (!alreadyListed)
                {
                    missingItems.push_back(itemId);
                    TC_LOG_DEBUG("playerbot.quest", "Bot {} missing {} of drop item {} for quest {}",
                        bot->GetName(), requiredCount - currentCount, itemId, questId);
                }
            }
        }
    }

    return missingItems;
}

bool UnifiedQuestManager::ValidationModule::ValidateQuestAvailability(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateQuestAvailability(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateSeasonalAvailability(uint32 questId)
{
    // Note: This method doesn't have bot context, using nullptr for GetGameSystems
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestValidation()->ValidateSeasonalAvailability(questId);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateDailyQuestLimits(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateDailyQuestLimits(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateQuestTimer(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateQuestTimer(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateZoneRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateZoneRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateAreaRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateAreaRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::IsInCorrectZone(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->IsInCorrectZone(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::CanQuestBeStartedAtLocation(uint32 questId, const Position& location)
{
    // Note: This method doesn't have bot context, using nullptr for GetGameSystems
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestValidation()->CanQuestBeStartedAtLocation(questId, location);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateGroupRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateGroupRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidatePartyQuestRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidatePartyQuestRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateRaidQuestRequirements(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateRaidQuestRequirements(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::CanGroupMemberShareQuest(uint32 questId, Player* sharer, Player* receiver)
{
    if (IGameSystemsManager* systems = GetGameSystems(sharer))
        return systems->GetQuestValidation()->CanGroupMemberShareQuest(questId, sharer, receiver);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateWithContext(ValidationContext& context)
{
    // Validate inputs
    if (!context.bot)
    {
        context.errors.push_back("No bot provided for validation");
        return false;
    }

    if (context.questId == 0)
    {
        context.errors.push_back("Invalid quest ID (0)");
        return false;
    }

    // Get quest template if not already in context
    if (!context.quest)
    {
        context.quest = sObjectMgr->GetQuestTemplate(context.questId);
        if (!context.quest)
        {
            context.errors.push_back("Quest template not found for ID " + std::to_string(context.questId));
            return false;
        }
    }

    Player* bot = context.bot;
    const Quest* quest = context.quest;
    uint32 questId = context.questId;
    bool hasErrors = false;

    // Record validation start time
    context.validationTime = GameTime::GetGameTimeMS();

    // 1. Level requirements
    uint8 botLevel = bot->GetLevel();
    // Use Player::GetQuestMinLevel which uses ContentTuning system for level scaling
    int32 minLevel = bot->GetQuestMinLevel(quest);
    uint8 maxLevel = quest->GetMaxLevel();

    if (static_cast<int32>(botLevel) < minLevel)
    {
        context.errors.push_back("Bot level " + std::to_string(botLevel) + " is below minimum " + std::to_string(minLevel));
        hasErrors = true;
    }
    if (maxLevel > 0 && botLevel > maxLevel)
    {
        context.errors.push_back("Bot level " + std::to_string(botLevel) + " exceeds maximum " + std::to_string(maxLevel));
        hasErrors = true;
    }

    // 2. Class requirements
    uint32 requiredClasses = quest->GetAllowableClasses();
    if (requiredClasses != 0 && !(requiredClasses & (1 << (bot->GetClass() - 1))))
    {
        context.errors.push_back("Bot class not allowed for this quest");
        hasErrors = true;
    }

    // 3. Race requirements - GetAllowableRaces returns Trinity::RaceMask<uint64>
    Trinity::RaceMask<uint64> requiredRaces = quest->GetAllowableRaces();
    if (!requiredRaces.IsEmpty() && !requiredRaces.HasRace(bot->GetRace()))
    {
        context.errors.push_back("Bot race not allowed for this quest");
        hasErrors = true;
    }

    // 4. Quest status check
    QuestStatus questStatus = bot->GetQuestStatus(questId);
    if (questStatus == QUEST_STATUS_COMPLETE || questStatus == QUEST_STATUS_REWARDED)
    {
        if (!quest->IsRepeatable() && !quest->IsDailyOrWeekly())
        {
            context.errors.push_back("Quest already completed and is not repeatable");
            hasErrors = true;
        }
    }
    else if (questStatus == QUEST_STATUS_INCOMPLETE)
    {
        context.warnings.push_back("Quest is already in progress");
    }

    // 5. Quest log full check
    if (bot->GetQuestSlotQuestId(MAX_QUEST_LOG_SIZE - 1) != 0)
    {
        context.warnings.push_back("Quest log may be full");
    }

    // 6. Reputation requirements (if strict validation)
    if (context.strictValidation)
    {
        uint32 requiredMinRepFaction = quest->GetRequiredMinRepFaction();
        if (requiredMinRepFaction)
        {
            int32 requiredMinRepValue = quest->GetRequiredMinRepValue();
            int32 currentRepValue = bot->GetReputation(requiredMinRepFaction);
            if (currentRepValue < requiredMinRepValue)
            {
                context.errors.push_back("Insufficient reputation with faction " + std::to_string(requiredMinRepFaction));
                hasErrors = true;
            }
        }

        uint32 requiredMaxRepFaction = quest->GetRequiredMaxRepFaction();
        if (requiredMaxRepFaction)
        {
            int32 requiredMaxRepValue = quest->GetRequiredMaxRepValue();
            int32 currentRepValue = bot->GetReputation(requiredMaxRepFaction);
            if (currentRepValue > requiredMaxRepValue)
            {
                context.errors.push_back("Reputation too high with faction " + std::to_string(requiredMaxRepFaction));
                hasErrors = true;
            }
        }
    }

    // 7. Prerequisite quest check
    int32 prevQuestId = quest->GetPrevQuestId();
    if (prevQuestId != 0)
    {
        if (prevQuestId > 0)
        {
            // Must have completed previous quest
            if (!bot->GetQuestRewardStatus(static_cast<uint32>(prevQuestId)))
            {
                context.errors.push_back("Prerequisite quest " + std::to_string(prevQuestId) + " not completed");
                hasErrors = true;
            }
        }
        else
        {
            // Must NOT have completed the quest (negative ID)
            if (bot->GetQuestRewardStatus(static_cast<uint32>(-prevQuestId)))
            {
                context.errors.push_back("Conflicting quest " + std::to_string(-prevQuestId) + " already completed");
                hasErrors = true;
            }
        }
    }

    // 8. Check optional requirements if requested
    if (context.checkOptionalRequirements)
    {
        // Source item check (quest starter item)
        uint32 sourceItemId = quest->GetSrcItemId();
        if (sourceItemId)
        {
            uint32 sourceItemCount = quest->GetSrcItemCount();
            if (sourceItemCount == 0)
                sourceItemCount = 1;

            if (!bot->HasItemCount(sourceItemId, sourceItemCount))
            {
                context.warnings.push_back("Missing source item " + std::to_string(sourceItemId));
            }
        }

        // Inventory space for rewards
        uint32 rewardSlots = 0;
        for (uint32 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
        {
            if (quest->RewardItemId[i] != 0 && quest->RewardItemCount[i] > 0)
                ++rewardSlots;
        }

        if (rewardSlots > 0)
        {
            uint32 freeSlots = 0;
            for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
            {
                if (Bag* pBag = bot->GetBagByPos(bag))
                {
                    for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
                    {
                        if (!pBag->GetItemByPos(slot))
                            ++freeSlots;
                    }
                }
            }
            for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            {
                if (!bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    ++freeSlots;
            }

            if (freeSlots < rewardSlots)
            {
                context.warnings.push_back("May have insufficient inventory space for " + std::to_string(rewardSlots) + " reward items");
            }
        }
    }

    // 9. Validate future requirements if requested (helpful for quest planning)
    if (context.validateFutureRequirements)
    {
        // Check item objectives that will need to be collected
        for (QuestObjective const& objective : quest->GetObjectives())
        {
            if (objective.Type == QUEST_OBJECTIVE_ITEM)
            {
                uint32 itemId = objective.ObjectID;
                uint32 currentCount = bot->GetItemCount(itemId, true);
                int32 requiredCount = objective.Amount;

                if (static_cast<int32>(currentCount) < requiredCount)
                {
                    context.warnings.push_back("Will need to collect " + std::to_string(requiredCount - currentCount) +
                        " more of item " + std::to_string(itemId));
                }
            }
        }
    }

    // Update validation time to reflect completion
    context.validationTime = GameTime::GetGameTimeMS() - context.validationTime;

    TC_LOG_DEBUG("playerbot.quest", "ValidateWithContext for bot {} quest {}: {} errors, {} warnings, took {}ms",
        bot->GetName(), questId, context.errors.size(), context.warnings.size(), context.validationTime);

    return !hasErrors;
}

bool UnifiedQuestManager::ValidationModule::ValidateQuestObjectives(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateQuestObjectives(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateQuestRewards(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateQuestRewards(questId, bot);
    return {};
}

bool UnifiedQuestManager::ValidationModule::ValidateQuestDifficulty(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateQuestDifficulty(questId, bot);
    return {};
}

ValidationResult UnifiedQuestManager::ValidationModule::GetCachedValidation(uint32 questId, uint32 botGuid)
{
    // Note: This method doesn't have bot context, using nullptr for GetGameSystems
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestValidation()->GetCachedValidation(questId, botGuid);
    return {};
}

void UnifiedQuestManager::ValidationModule::CacheValidationResult(uint32 questId, uint32 botGuid, const ValidationResult& result)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestValidation()->CacheValidationResult(questId, botGuid, result);
}

void UnifiedQuestManager::ValidationModule::InvalidateValidationCache(uint32 botGuid)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestValidation()->InvalidateValidationCache(botGuid);
}

void UnifiedQuestManager::ValidationModule::CleanupExpiredCache()
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestValidation()->CleanupExpiredCache();
}

std::unordered_map<uint32, ValidationResult> UnifiedQuestManager::ValidationModule::ValidateMultipleQuests(
    const std::vector<uint32>& questIds, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->ValidateMultipleQuests(questIds, bot);
    return {};
}

std::vector<uint32> UnifiedQuestManager::ValidationModule::FilterValidQuests(const std::vector<uint32>& questIds, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->FilterValidQuests(questIds, bot);
    return {};
}

std::vector<uint32> UnifiedQuestManager::ValidationModule::GetEligibleQuests(Player* bot, const std::vector<uint32>& candidates)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->GetEligibleQuests(bot, candidates);
    return {};
}

std::string UnifiedQuestManager::ValidationModule::GetDetailedValidationReport(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->GetDetailedValidationReport(questId, bot);
    return {};
}

void UnifiedQuestManager::ValidationModule::LogValidationFailure(uint32 questId, Player* bot, const std::string& reason)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestValidation()->LogValidationFailure(questId, bot, reason);
}

std::vector<std::string> UnifiedQuestManager::ValidationModule::GetRecommendationsForFailedQuest(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestValidation()->GetRecommendationsForFailedQuest(questId, bot);
    return {};
}

ValidationMetrics UnifiedQuestManager::ValidationModule::GetValidationMetrics()
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestValidation()->GetValidationMetrics();
    return {};
}

// ============================================================================
// TURNIN MODULE IMPLEMENTATION (delegates to QuestTurnIn)
// ============================================================================

bool UnifiedQuestManager::TurnInModule::TurnInQuestWithReward(uint32 questId, Player* bot)
{
    _questsTurnedIn++;
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->TurnInQuest(questId, bot);
    return {};
}

void UnifiedQuestManager::TurnInModule::ProcessQuestTurnIn(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ProcessQuestTurnIn(bot, questId);
}

void UnifiedQuestManager::TurnInModule::ProcessBatchTurnIn(Player* bot, const TurnInBatch& batch)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ProcessBatchTurnIn(bot, batch);
}

void UnifiedQuestManager::TurnInModule::ScheduleQuestTurnIn(Player* bot, uint32 questId, uint32 delayMs)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ScheduleQuestTurnIn(bot, questId, delayMs);
}

std::vector<uint32> UnifiedQuestManager::TurnInModule::GetCompletedQuests(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->GetCompletedQuests(bot);
    return {};
}

bool UnifiedQuestManager::TurnInModule::IsQuestReadyForTurnIn(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->IsQuestReadyForTurnIn(questId, bot);
    return {};
}

void UnifiedQuestManager::TurnInModule::MonitorQuestCompletion(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->MonitorQuestCompletion(bot);
}

void UnifiedQuestManager::TurnInModule::HandleQuestCompletion(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->HandleQuestCompletion(bot, questId);
}

void UnifiedQuestManager::TurnInModule::PlanOptimalTurnInRoute(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->PlanOptimalTurnInRoute(bot);
}

TurnInBatch UnifiedQuestManager::TurnInModule::CreateTurnInBatch(Player* bot, const std::vector<uint32>& questIds)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->CreateTurnInBatch(bot, questIds);

    return TurnInBatch(0);  // Return empty batch with botGuid=0
}

void UnifiedQuestManager::TurnInModule::OptimizeTurnInSequence(Player* bot, std::vector<QuestTurnInData>& turnIns)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->OptimizeTurnInSequence(bot, turnIns);
}

void UnifiedQuestManager::TurnInModule::MinimizeTurnInTravel(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->MinimizeTurnInTravel(bot);
}

bool UnifiedQuestManager::TurnInModule::FindQuestTurnInNpc(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->FindQuestTurnInNpc(bot, questId);
    return {};
}

Position UnifiedQuestManager::TurnInModule::GetQuestTurnInLocation(uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestTurnIn()->GetQuestTurnInLocation(questId);
    return {};
}

bool UnifiedQuestManager::TurnInModule::NavigateToQuestGiver(Player* bot, uint32 questGiverGuid)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->NavigateToQuestGiver(bot, questGiverGuid);
    return {};
}

bool UnifiedQuestManager::TurnInModule::IsAtQuestGiver(Player* bot, uint32 questGiverGuid)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->IsAtQuestGiver(bot, questGiverGuid);
    return {};
}

void UnifiedQuestManager::TurnInModule::AnalyzeQuestRewards(QuestTurnInData& turnInData, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->AnalyzeQuestRewards(turnInData, bot);
}

uint32 UnifiedQuestManager::TurnInModule::SelectOptimalReward(const std::vector<QuestRewardItem>& rewards, Player* bot, RewardSelectionStrategy strategy)
{
    _rewardsSelected++;
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->SelectOptimalReward(rewards, bot, strategy);
    return {};
}

void UnifiedQuestManager::TurnInModule::EvaluateItemUpgrades(const std::vector<QuestRewardItem>& rewards, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->EvaluateItemUpgrades(rewards, bot);
}

float UnifiedQuestManager::TurnInModule::CalculateItemValue(const QuestRewardItem& reward, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetQuestTurnIn()->CalculateItemValue(reward, bot);
    return {};
}

void UnifiedQuestManager::TurnInModule::CoordinateGroupTurnIns(Group* group)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->CoordinateGroupTurnIns(group);
}

void UnifiedQuestManager::TurnInModule::SynchronizeGroupRewardSelection(Group* group, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->SynchronizeGroupRewardSelection(group, questId);
}

void UnifiedQuestManager::TurnInModule::HandleGroupTurnInConflicts(Group* group, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->HandleGroupTurnInConflicts(group, questId);
}

void UnifiedQuestManager::TurnInModule::ShareTurnInProgress(Group* group)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->ShareTurnInProgress(group);
}

void UnifiedQuestManager::TurnInModule::HandleQuestGiverDialog(Player* bot, uint32 questGiverGuid, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->HandleQuestGiverDialog(bot, questGiverGuid, questId);
}

void UnifiedQuestManager::TurnInModule::SelectQuestReward(Player* bot, uint32 questId, uint32 rewardIndex)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->SelectQuestReward(bot, questId, rewardIndex);
}

void UnifiedQuestManager::TurnInModule::ConfirmQuestTurnIn(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ConfirmQuestTurnIn(bot, questId);
}

void UnifiedQuestManager::TurnInModule::HandleTurnInDialog(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->HandleTurnInDialog(bot, questId);
}

void UnifiedQuestManager::TurnInModule::ExecuteImmediateTurnInStrategy(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ExecuteImmediateTurnInStrategy(bot);
}

void UnifiedQuestManager::TurnInModule::ExecuteBatchTurnInStrategy(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ExecuteBatchTurnInStrategy(bot);
}

void UnifiedQuestManager::TurnInModule::ExecuteOptimalRoutingStrategy(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ExecuteOptimalRoutingStrategy(bot);
}

void UnifiedQuestManager::TurnInModule::ExecuteGroupCoordinationStrategy(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ExecuteGroupCoordinationStrategy(bot);
}

void UnifiedQuestManager::TurnInModule::ExecuteRewardOptimizationStrategy(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ExecuteRewardOptimizationStrategy(bot);
}

void UnifiedQuestManager::TurnInModule::ExecuteChainContinuationStrategy(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ExecuteChainContinuationStrategy(bot);
}

void UnifiedQuestManager::TurnInModule::HandleQuestChainProgression(Player* bot, uint32 completedQuestId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->HandleQuestChainProgression(bot, completedQuestId);
}

uint32 UnifiedQuestManager::TurnInModule::GetNextQuestInChain(uint32 completedQuestId)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestTurnIn()->GetNextQuestInChain(completedQuestId);
    return {};
}

void UnifiedQuestManager::TurnInModule::AutoAcceptFollowUpQuests(Player* bot, uint32 completedQuestId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->AutoAcceptFollowUpQuests(bot, completedQuestId);
}

void UnifiedQuestManager::TurnInModule::PrioritizeChainQuests(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->PrioritizeChainQuests(bot);
}

void UnifiedQuestManager::TurnInModule::SetTurnInStrategy(uint32 botGuid, TurnInStrategy strategy)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->SetTurnInStrategy(botGuid, strategy);
}

TurnInStrategy UnifiedQuestManager::TurnInModule::GetTurnInStrategy(uint32 botGuid)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestTurnIn()->GetTurnInStrategy(botGuid);
    return {};
}

void UnifiedQuestManager::TurnInModule::SetRewardSelectionStrategy(uint32 botGuid, RewardSelectionStrategy strategy)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->SetRewardSelectionStrategy(botGuid, strategy);
}

RewardSelectionStrategy UnifiedQuestManager::TurnInModule::GetRewardSelectionStrategy(uint32 botGuid)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestTurnIn()->GetRewardSelectionStrategy(botGuid);
    return {};
}

void UnifiedQuestManager::TurnInModule::SetBatchTurnInThreshold(uint32 botGuid, uint32 threshold)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->SetBatchTurnInThreshold(botGuid, threshold);
}

void UnifiedQuestManager::TurnInModule::HandleTurnInError(Player* bot, uint32 questId, const std::string& error)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->HandleTurnInError(bot, questId, error);
}

void UnifiedQuestManager::TurnInModule::RecoverFromTurnInFailure(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->RecoverFromTurnInFailure(bot, questId);
}

void UnifiedQuestManager::TurnInModule::RetryFailedTurnIn(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->RetryFailedTurnIn(bot, questId);
}

void UnifiedQuestManager::TurnInModule::ValidateTurnInState(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->ValidateTurnInState(bot, questId);
}

QuestTurnIn::TurnInMetricsSnapshot UnifiedQuestManager::TurnInModule::GetBotTurnInMetrics(uint32 botGuid)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestTurnIn()->GetBotTurnInMetrics(botGuid);
    return {};
}

QuestTurnIn::TurnInMetricsSnapshot UnifiedQuestManager::TurnInModule::GetGlobalTurnInMetrics()
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetQuestTurnIn()->GetGlobalTurnInMetrics();
    return {};
}

// ============================================================================
// DYNAMIC MODULE IMPLEMENTATION (delegates to DynamicQuestSystem)
// ============================================================================

std::vector<uint32> UnifiedQuestManager::DynamicModule::DiscoverAvailableQuests(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->DiscoverAvailableQuests(bot);
    return {};
}

std::vector<uint32> UnifiedQuestManager::DynamicModule::GetRecommendedQuests(Player* bot, QuestSelectionStrategy strategy)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->GetRecommendedQuests(bot, strategy);
    return {};
}

bool UnifiedQuestManager::DynamicModule::AssignQuestToBot(uint32 questId, Player* bot)
{
    _questsAssigned++;
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->AssignQuestToBot(questId, bot);
    return {};
}

void UnifiedQuestManager::DynamicModule::AutoAssignQuests(Player* bot, uint32 maxQuests)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->AutoAssignQuests(bot, maxQuests);
}

QuestPriority UnifiedQuestManager::DynamicModule::CalculateQuestPriority(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->CalculateQuestPriority(questId, bot);
    return {};
}

std::vector<uint32> UnifiedQuestManager::DynamicModule::SortQuestsByPriority(const std::vector<uint32>& questIds, Player* bot)
{
    _questsOptimized++;
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->SortQuestsByPriority(questIds, bot);
    return {};
}

bool UnifiedQuestManager::DynamicModule::ShouldAbandonQuest(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->ShouldAbandonQuest(questId, bot);
    return {};
}

void UnifiedQuestManager::DynamicModule::UpdateQuestProgressDynamic(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->UpdateQuestProgress(bot);
}

void UnifiedQuestManager::DynamicModule::ExecuteQuestObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->ExecuteQuestObjective(bot, questId, objectiveIndex);
}

bool UnifiedQuestManager::DynamicModule::CanCompleteQuestObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->CanCompleteQuestObjective(bot, questId, objectiveIndex);
    return {};
}

void UnifiedQuestManager::DynamicModule::HandleQuestCompletionDynamic(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->HandleQuestCompletion(bot, questId);
}

bool UnifiedQuestManager::DynamicModule::FormQuestGroup(uint32 questId, Player* initiator)
{
    if (IGameSystemsManager* systems = GetGameSystems(initiator))
        return systems->GetDynamicQuestSystem()->FormQuestGroup(questId, initiator);
    return {};
}

void UnifiedQuestManager::DynamicModule::CoordinateGroupQuest(Group* group, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetDynamicQuestSystem()->CoordinateGroupQuest(group, questId);
}

void UnifiedQuestManager::DynamicModule::ShareQuestProgress(Group* group, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetDynamicQuestSystem()->ShareQuestProgress(group, questId);
}

bool UnifiedQuestManager::DynamicModule::CanShareQuest(uint32 questId, Player* from, Player* to)
{
    if (IGameSystemsManager* systems = GetGameSystems(from))
        return systems->GetDynamicQuestSystem()->CanShareQuest(questId, from, to);
    return {};
}

Position UnifiedQuestManager::DynamicModule::GetNextQuestLocation(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->GetNextQuestLocation(bot, questId);
    return {};
}

std::vector<Position> UnifiedQuestManager::DynamicModule::GenerateQuestPath(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->GenerateQuestPath(bot, questId);
    return {};
}

void UnifiedQuestManager::DynamicModule::HandleQuestNavigation(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->HandleQuestNavigation(bot, questId);
}

bool UnifiedQuestManager::DynamicModule::IsQuestLocationReachable(Player* bot, const Position& location)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->IsQuestLocationReachable(bot, location);
    return {};
}

void UnifiedQuestManager::DynamicModule::AdaptQuestDifficulty(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->AdaptQuestDifficulty(questId, bot);
}

void UnifiedQuestManager::DynamicModule::HandleQuestStuckState(Player* bot, uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->HandleQuestStuckState(bot, questId);
}

void UnifiedQuestManager::DynamicModule::RetryFailedObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->RetryFailedObjective(bot, questId, objectiveIndex);
}

void UnifiedQuestManager::DynamicModule::OptimizeQuestOrder(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->OptimizeQuestOrder(bot);
}

void UnifiedQuestManager::DynamicModule::TrackQuestChains(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->TrackQuestChains(bot);
}

std::vector<uint32> UnifiedQuestManager::DynamicModule::GetQuestChain(uint32 questId)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetDynamicQuestSystem()->GetQuestChain(questId);
    return {};
}

uint32 UnifiedQuestManager::DynamicModule::GetNextQuestInChainDynamic(uint32 completedQuestId)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetDynamicQuestSystem()->GetNextQuestInChain(completedQuestId);
    return {};
}

void UnifiedQuestManager::DynamicModule::AdvanceQuestChain(Player* bot, uint32 completedQuestId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->AdvanceQuestChain(bot, completedQuestId);
}

void UnifiedQuestManager::DynamicModule::OptimizeZoneQuests(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->OptimizeZoneQuests(bot);
}

std::vector<uint32> UnifiedQuestManager::DynamicModule::GetZoneQuests(uint32 zoneId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->GetZoneQuests(zoneId, bot);
    return {};
}

void UnifiedQuestManager::DynamicModule::PlanZoneCompletion(Player* bot, uint32 zoneId)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetDynamicQuestSystem()->PlanZoneCompletion(bot, zoneId);
}

bool UnifiedQuestManager::DynamicModule::ShouldMoveToNewZone(Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->ShouldMoveToNewZone(bot);
    return {};
}

QuestReward UnifiedQuestManager::DynamicModule::AnalyzeQuestReward(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->AnalyzeQuestReward(questId, bot);
    return {};
}

float UnifiedQuestManager::DynamicModule::CalculateQuestValue(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->CalculateQuestValue(questId, bot);
    return {};
}

bool UnifiedQuestManager::DynamicModule::IsQuestWorthwhile(uint32 questId, Player* bot)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        return systems->GetDynamicQuestSystem()->IsQuestWorthwhile(questId, bot);
    return {};
}

void UnifiedQuestManager::DynamicModule::SetQuestStrategy(uint32 botGuid, QuestSelectionStrategy strategy)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetDynamicQuestSystem()->SetQuestStrategy(botGuid, strategy);
}

QuestSelectionStrategy UnifiedQuestManager::DynamicModule::GetQuestStrategy(uint32 botGuid)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetDynamicQuestSystem()->GetQuestStrategy(botGuid);
    return {};
}

void UnifiedQuestManager::DynamicModule::SetMaxConcurrentQuests(uint32 botGuid, uint32 maxQuests)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetDynamicQuestSystem()->SetMaxConcurrentQuests(botGuid, maxQuests);
}

void UnifiedQuestManager::DynamicModule::EnableQuestGrouping(uint32 botGuid, bool enable)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetDynamicQuestSystem()->EnableQuestGrouping(botGuid, enable);
}

QuestMetrics UnifiedQuestManager::DynamicModule::GetBotQuestMetrics(uint32 botGuid)
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetDynamicQuestSystem()->GetBotQuestMetrics(botGuid);
    return {};
}

QuestMetrics UnifiedQuestManager::DynamicModule::GetGlobalQuestMetrics()
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        return systems->GetDynamicQuestSystem()->GetGlobalQuestMetrics();
    return {};
}

// ============================================================================
// UNIFIED QUEST MANAGER INTERFACE DELEGATION
// ============================================================================

// Pickup Module delegation
bool UnifiedQuestManager::PickupQuest(uint32 questId, Player* bot, uint32 questGiverGuid)
{ return _pickup->PickupQuest(questId, bot, questGiverGuid); }

bool UnifiedQuestManager::PickupQuestFromGiver(Player* bot, uint32 questGiverGuid, uint32 questId)
{ return _pickup->PickupQuestFromGiver(bot, questGiverGuid, questId); }

void UnifiedQuestManager::PickupAvailableQuests(Player* bot)
{ _pickup->PickupAvailableQuests(bot); }

void UnifiedQuestManager::PickupQuestsInArea(Player* bot, float radius)
{ _pickup->PickupQuestsInArea(bot, radius); }

std::vector<uint32> UnifiedQuestManager::DiscoverNearbyQuests(Player* bot, float scanRadius)
{ return _pickup->DiscoverNearbyQuests(bot, scanRadius); }

std::vector<QuestGiverInfo> UnifiedQuestManager::ScanForQuestGivers(Player* bot, float scanRadius)
{ return _pickup->ScanForQuestGivers(bot, scanRadius); }

std::vector<uint32> UnifiedQuestManager::GetAvailableQuestsFromGiver(uint32 questGiverGuid, Player* bot)
{ return _pickup->GetAvailableQuestsFromGiver(questGiverGuid, bot); }

QuestEligibility UnifiedQuestManager::CheckQuestEligibility(uint32 questId, Player* bot)
{ return _pickup->CheckQuestEligibility(questId, bot); }

bool UnifiedQuestManager::CanAcceptQuest(uint32 questId, Player* bot)
{ return _pickup->CanAcceptQuest(questId, bot); }

bool UnifiedQuestManager::MeetsQuestRequirements(uint32 questId, Player* bot)
{ return _pickup->MeetsQuestRequirements(questId, bot); }

std::vector<uint32> UnifiedQuestManager::FilterQuests(const std::vector<uint32>& questIds, Player* bot, const QuestPickupFilter& filter)
{ return _pickup->FilterQuests(questIds, bot, filter); }

std::vector<uint32> UnifiedQuestManager::PrioritizeQuests(const std::vector<uint32>& questIds, Player* bot, QuestAcceptanceStrategy strategy)
{ return _pickup->PrioritizeQuests(questIds, bot, strategy); }

bool UnifiedQuestManager::ShouldAcceptQuest(uint32 questId, Player* bot)
{ return _pickup->ShouldAcceptQuest(questId, bot); }

// Completion Module delegation
bool UnifiedQuestManager::StartQuestCompletion(uint32 questId, Player* bot)
{ return _completion->StartQuestCompletion(questId, bot); }

void UnifiedQuestManager::UpdateQuestProgress(Player* bot)
{ _completion->UpdateQuestProgress(bot); }

void UnifiedQuestManager::CompleteQuest(uint32 questId, Player* bot)
{ _completion->CompleteQuest(questId, bot); }

bool UnifiedQuestManager::TurnInQuest(uint32 questId, Player* bot)
{ return _completion->TurnInQuest(questId, bot); }

void UnifiedQuestManager::TrackQuestObjectives(Player* bot)
{ _completion->TrackQuestObjectives(bot); }

void UnifiedQuestManager::ExecuteObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->ExecuteObjective(bot, objective); }

void UnifiedQuestManager::UpdateObjectiveProgress(Player* bot, uint32 questId, uint32 objectiveIndex)
{ _completion->UpdateObjectiveProgress(bot, questId, objectiveIndex); }

bool UnifiedQuestManager::IsObjectiveComplete(const QuestObjectiveData& objective)
{ return _completion->IsObjectiveComplete(objective); }

void UnifiedQuestManager::HandleKillObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->HandleKillObjective(bot, objective); }

void UnifiedQuestManager::HandleCollectObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->HandleCollectObjective(bot, objective); }

void UnifiedQuestManager::HandleTalkToNpcObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->HandleTalkToNpcObjective(bot, objective); }

void UnifiedQuestManager::HandleLocationObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->HandleLocationObjective(bot, objective); }

void UnifiedQuestManager::HandleGameObjectObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->HandleGameObjectObjective(bot, objective); }

void UnifiedQuestManager::HandleSpellCastObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->HandleSpellCastObjective(bot, objective); }

void UnifiedQuestManager::HandleEmoteObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->HandleEmoteObjective(bot, objective); }

void UnifiedQuestManager::HandleEscortObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->HandleEscortObjective(bot, objective); }

void UnifiedQuestManager::NavigateToObjective(Player* bot, const QuestObjectiveData& objective)
{ _completion->NavigateToObjective(bot, objective); }

bool UnifiedQuestManager::FindObjectiveTarget(Player* bot, QuestObjectiveData& objective)
{ return _completion->FindObjectiveTarget(bot, objective); }

std::vector<Position> UnifiedQuestManager::GetObjectiveLocations(const QuestObjectiveData& objective)
{ return _completion->GetObjectiveLocations(objective); }

Position UnifiedQuestManager::GetOptimalObjectivePosition(Player* bot, const QuestObjectiveData& objective)
{ return _completion->GetOptimalObjectivePosition(bot, objective); }

void UnifiedQuestManager::CoordinateGroupQuestCompletion(Group* group, uint32 questId)
{ _completion->CoordinateGroupQuestCompletion(group, questId); }

void UnifiedQuestManager::ShareObjectiveProgress(Group* group, uint32 questId)
{ _completion->ShareObjectiveProgress(group, questId); }

void UnifiedQuestManager::SynchronizeGroupObjectives(Group* group, uint32 questId)
{ _completion->SynchronizeGroupObjectives(group, questId); }

void UnifiedQuestManager::HandleGroupObjectiveConflict(Group* group, uint32 questId, uint32 objectiveIndex)
{ _completion->HandleGroupObjectiveConflict(group, questId, objectiveIndex); }

void UnifiedQuestManager::OptimizeQuestCompletionOrder(Player* bot)
{ _completion->OptimizeQuestCompletionOrder(bot); }

void UnifiedQuestManager::OptimizeObjectiveSequence(Player* bot, uint32 questId)
{ _completion->OptimizeObjectiveSequence(bot, questId); }

void UnifiedQuestManager::FindEfficientCompletionPath(Player* bot, const std::vector<uint32>& questIds)
{ _completion->FindEfficientCompletionPath(bot, questIds); }

void UnifiedQuestManager::MinimizeTravelTime(Player* bot, const std::vector<QuestObjectiveData>& objectives)
{ _completion->MinimizeTravelTime(bot, objectives); }

void UnifiedQuestManager::DetectStuckState(Player* bot, uint32 questId)
{ _completion->DetectStuckState(bot, questId); }

void UnifiedQuestManager::HandleStuckObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->HandleStuckObjective(bot, objective); }

void UnifiedQuestManager::RecoverFromStuckState(Player* bot, uint32 questId)
{ _completion->RecoverFromStuckState(bot, questId); }

void UnifiedQuestManager::SkipProblematicObjective(Player* bot, QuestObjectiveData& objective)
{ _completion->SkipProblematicObjective(bot, objective); }

// Validation Module delegation (abbreviated due to length - pattern continues)
bool UnifiedQuestManager::ValidateQuest(uint32 questId, Player* bot)
{ return _validation->ValidateQuest(questId, bot); }

bool UnifiedQuestManager::ValidateQuestRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateQuestRequirements(questId, bot); }

std::vector<std::string> UnifiedQuestManager::GetValidationErrors(uint32 questId, Player* bot)
{ return _validation->GetValidationErrors(questId, bot); }

bool UnifiedQuestManager::ValidateLevelRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateLevelRequirements(questId, bot); }

bool UnifiedQuestManager::ValidateClassRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateClassRequirements(questId, bot); }

bool UnifiedQuestManager::ValidateRaceRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateRaceRequirements(questId, bot); }

bool UnifiedQuestManager::ValidateSkillRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateSkillRequirements(questId, bot); }

bool UnifiedQuestManager::ValidateQuestPrerequisites(uint32 questId, Player* bot)
{ return _validation->ValidateQuestPrerequisites(questId, bot); }

bool UnifiedQuestManager::ValidateQuestChain(uint32 questId, Player* bot)
{ return _validation->ValidateQuestChain(questId, bot); }

bool UnifiedQuestManager::HasCompletedPrerequisites(uint32 questId, Player* bot)
{ return _validation->HasCompletedPrerequisites(questId, bot); }

std::vector<uint32> UnifiedQuestManager::GetMissingPrerequisites(uint32 questId, Player* bot)
{ return _validation->GetMissingPrerequisites(questId, bot); }

bool UnifiedQuestManager::ValidateReputationRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateReputationRequirements(questId, bot); }

bool UnifiedQuestManager::ValidateFactionRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateFactionRequirements(questId, bot); }

bool UnifiedQuestManager::HasRequiredReputation(uint32 questId, Player* bot, uint32 factionId)
{ return _validation->HasRequiredReputation(questId, bot, factionId); }

bool UnifiedQuestManager::ValidateItemRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateItemRequirements(questId, bot); }

bool UnifiedQuestManager::HasRequiredItems(uint32 questId, Player* bot)
{ return _validation->HasRequiredItems(questId, bot); }

bool UnifiedQuestManager::HasInventorySpace(uint32 questId, Player* bot)
{ return _validation->HasInventorySpace(questId, bot); }

std::vector<uint32> UnifiedQuestManager::GetMissingQuestItems(uint32 questId, Player* bot)
{ return _validation->GetMissingQuestItems(questId, bot); }

bool UnifiedQuestManager::ValidateQuestAvailability(uint32 questId, Player* bot)
{ return _validation->ValidateQuestAvailability(questId, bot); }

bool UnifiedQuestManager::ValidateSeasonalAvailability(uint32 questId)
{ return _validation->ValidateSeasonalAvailability(questId); }

bool UnifiedQuestManager::ValidateDailyQuestLimits(uint32 questId, Player* bot)
{ return _validation->ValidateDailyQuestLimits(questId, bot); }

bool UnifiedQuestManager::ValidateQuestTimer(uint32 questId, Player* bot)
{ return _validation->ValidateQuestTimer(questId, bot); }

bool UnifiedQuestManager::ValidateZoneRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateZoneRequirements(questId, bot); }

bool UnifiedQuestManager::ValidateAreaRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateAreaRequirements(questId, bot); }

bool UnifiedQuestManager::IsInCorrectZone(uint32 questId, Player* bot)
{ return _validation->IsInCorrectZone(questId, bot); }

bool UnifiedQuestManager::CanQuestBeStartedAtLocation(uint32 questId, const Position& location)
{ return _validation->CanQuestBeStartedAtLocation(questId, location); }

bool UnifiedQuestManager::ValidateGroupRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateGroupRequirements(questId, bot); }

bool UnifiedQuestManager::ValidatePartyQuestRequirements(uint32 questId, Player* bot)
{ return _validation->ValidatePartyQuestRequirements(questId, bot); }

bool UnifiedQuestManager::ValidateRaidQuestRequirements(uint32 questId, Player* bot)
{ return _validation->ValidateRaidQuestRequirements(questId, bot); }

bool UnifiedQuestManager::CanGroupMemberShareQuest(uint32 questId, Player* sharer, Player* receiver)
{ return _validation->CanGroupMemberShareQuest(questId, sharer, receiver); }

bool UnifiedQuestManager::ValidateWithContext(ValidationContext& context)
{ return _validation->ValidateWithContext(context); }

bool UnifiedQuestManager::ValidateQuestObjectives(uint32 questId, Player* bot)
{ return _validation->ValidateQuestObjectives(questId, bot); }

bool UnifiedQuestManager::ValidateQuestRewards(uint32 questId, Player* bot)
{ return _validation->ValidateQuestRewards(questId, bot); }

bool UnifiedQuestManager::ValidateQuestDifficulty(uint32 questId, Player* bot)
{ return _validation->ValidateQuestDifficulty(questId, bot); }

ValidationResult UnifiedQuestManager::GetCachedValidation(uint32 questId, uint32 botGuid)
{ return _validation->GetCachedValidation(questId, botGuid); }

void UnifiedQuestManager::CacheValidationResult(uint32 questId, uint32 botGuid, const ValidationResult& result)
{ _validation->CacheValidationResult(questId, botGuid, result); }

void UnifiedQuestManager::InvalidateValidationCache(uint32 botGuid)
{ _validation->InvalidateValidationCache(botGuid); }

void UnifiedQuestManager::CleanupExpiredCache()
{ _validation->CleanupExpiredCache(); }

std::unordered_map<uint32, ValidationResult> UnifiedQuestManager::ValidateMultipleQuests(
    const std::vector<uint32>& questIds, Player* bot)
{ return _validation->ValidateMultipleQuests(questIds, bot); }

std::vector<uint32> UnifiedQuestManager::FilterValidQuests(const std::vector<uint32>& questIds, Player* bot)
{ return _validation->FilterValidQuests(questIds, bot); }

std::vector<uint32> UnifiedQuestManager::GetEligibleQuests(Player* bot, const std::vector<uint32>& candidates)
{ return _validation->GetEligibleQuests(bot, candidates); }

std::string UnifiedQuestManager::GetDetailedValidationReport(uint32 questId, Player* bot)
{ return _validation->GetDetailedValidationReport(questId, bot); }

void UnifiedQuestManager::LogValidationFailure(uint32 questId, Player* bot, const std::string& reason)
{ _validation->LogValidationFailure(questId, bot, reason); }

std::vector<std::string> UnifiedQuestManager::GetRecommendationsForFailedQuest(uint32 questId, Player* bot)
{ return _validation->GetRecommendationsForFailedQuest(questId, bot); }

// TurnIn Module delegation (continuing pattern)
bool UnifiedQuestManager::TurnInQuestWithReward(uint32 questId, Player* bot)
{ return _turnIn->TurnInQuestWithReward(questId, bot); }

void UnifiedQuestManager::ProcessQuestTurnIn(Player* bot, uint32 questId)
{ _turnIn->ProcessQuestTurnIn(bot, questId); }

void UnifiedQuestManager::ProcessBatchTurnIn(Player* bot, const TurnInBatch& batch)
{ _turnIn->ProcessBatchTurnIn(bot, batch); }

void UnifiedQuestManager::ScheduleQuestTurnIn(Player* bot, uint32 questId, uint32 delayMs)
{ _turnIn->ScheduleQuestTurnIn(bot, questId, delayMs); }

std::vector<uint32> UnifiedQuestManager::GetCompletedQuests(Player* bot)
{ return _turnIn->GetCompletedQuests(bot); }

bool UnifiedQuestManager::IsQuestReadyForTurnIn(uint32 questId, Player* bot)
{ return _turnIn->IsQuestReadyForTurnIn(questId, bot); }

void UnifiedQuestManager::MonitorQuestCompletion(Player* bot)
{ _turnIn->MonitorQuestCompletion(bot); }

void UnifiedQuestManager::HandleQuestCompletion(Player* bot, uint32 questId)
{ _turnIn->HandleQuestCompletion(bot, questId); }

void UnifiedQuestManager::PlanOptimalTurnInRoute(Player* bot)
{ _turnIn->PlanOptimalTurnInRoute(bot); }

TurnInBatch UnifiedQuestManager::CreateTurnInBatch(Player* bot, const std::vector<uint32>& questIds)
{ return _turnIn->CreateTurnInBatch(bot, questIds); }

void UnifiedQuestManager::OptimizeTurnInSequence(Player* bot, std::vector<QuestTurnInData>& turnIns)
{ _turnIn->OptimizeTurnInSequence(bot, turnIns); }

void UnifiedQuestManager::MinimizeTurnInTravel(Player* bot)
{ _turnIn->MinimizeTurnInTravel(bot); }

bool UnifiedQuestManager::FindQuestTurnInNpc(Player* bot, uint32 questId)
{ return _turnIn->FindQuestTurnInNpc(bot, questId); }

Position UnifiedQuestManager::GetQuestTurnInLocation(uint32 questId)
{ return _turnIn->GetQuestTurnInLocation(questId); }

bool UnifiedQuestManager::NavigateToQuestGiver(Player* bot, uint32 questGiverGuid)
{ return _turnIn->NavigateToQuestGiver(bot, questGiverGuid); }

bool UnifiedQuestManager::IsAtQuestGiver(Player* bot, uint32 questGiverGuid)
{ return _turnIn->IsAtQuestGiver(bot, questGiverGuid); }

void UnifiedQuestManager::AnalyzeQuestRewards(QuestTurnInData& turnInData, Player* bot)
{ _turnIn->AnalyzeQuestRewards(turnInData, bot); }

uint32 UnifiedQuestManager::SelectOptimalReward(const std::vector<QuestRewardItem>& rewards, Player* bot, RewardSelectionStrategy strategy)
{ return _turnIn->SelectOptimalReward(rewards, bot, strategy); }

void UnifiedQuestManager::EvaluateItemUpgrades(const std::vector<QuestRewardItem>& rewards, Player* bot)
{ _turnIn->EvaluateItemUpgrades(rewards, bot); }

float UnifiedQuestManager::CalculateItemValue(const QuestRewardItem& reward, Player* bot)
{ return _turnIn->CalculateItemValue(reward, bot); }

void UnifiedQuestManager::CoordinateGroupTurnIns(Group* group)
{ _turnIn->CoordinateGroupTurnIns(group); }

void UnifiedQuestManager::SynchronizeGroupRewardSelection(Group* group, uint32 questId)
{ _turnIn->SynchronizeGroupRewardSelection(group, questId); }

void UnifiedQuestManager::HandleGroupTurnInConflicts(Group* group, uint32 questId)
{ _turnIn->HandleGroupTurnInConflicts(group, questId); }

void UnifiedQuestManager::ShareTurnInProgress(Group* group)
{ _turnIn->ShareTurnInProgress(group); }

void UnifiedQuestManager::HandleQuestGiverDialog(Player* bot, uint32 questGiverGuid, uint32 questId)
{ _turnIn->HandleQuestGiverDialog(bot, questGiverGuid, questId); }

void UnifiedQuestManager::SelectQuestReward(Player* bot, uint32 questId, uint32 rewardIndex)
{ _turnIn->SelectQuestReward(bot, questId, rewardIndex); }

void UnifiedQuestManager::ConfirmQuestTurnIn(Player* bot, uint32 questId)
{ _turnIn->ConfirmQuestTurnIn(bot, questId); }

void UnifiedQuestManager::HandleTurnInDialog(Player* bot, uint32 questId)
{ _turnIn->HandleTurnInDialog(bot, questId); }

void UnifiedQuestManager::ExecuteImmediateTurnInStrategy(Player* bot)
{ _turnIn->ExecuteImmediateTurnInStrategy(bot); }

void UnifiedQuestManager::ExecuteBatchTurnInStrategy(Player* bot)
{ _turnIn->ExecuteBatchTurnInStrategy(bot); }

void UnifiedQuestManager::ExecuteOptimalRoutingStrategy(Player* bot)
{ _turnIn->ExecuteOptimalRoutingStrategy(bot); }

void UnifiedQuestManager::ExecuteGroupCoordinationStrategy(Player* bot)
{ _turnIn->ExecuteGroupCoordinationStrategy(bot); }

void UnifiedQuestManager::ExecuteRewardOptimizationStrategy(Player* bot)
{ _turnIn->ExecuteRewardOptimizationStrategy(bot); }

void UnifiedQuestManager::ExecuteChainContinuationStrategy(Player* bot)
{ _turnIn->ExecuteChainContinuationStrategy(bot); }

void UnifiedQuestManager::HandleQuestChainProgression(Player* bot, uint32 completedQuestId)
{ _turnIn->HandleQuestChainProgression(bot, completedQuestId); }

uint32 UnifiedQuestManager::GetNextQuestInChain(uint32 completedQuestId)
{ return _turnIn->GetNextQuestInChain(completedQuestId); }

void UnifiedQuestManager::AutoAcceptFollowUpQuests(Player* bot, uint32 completedQuestId)
{ _turnIn->AutoAcceptFollowUpQuests(bot, completedQuestId); }

void UnifiedQuestManager::PrioritizeChainQuests(Player* bot)
{ _turnIn->PrioritizeChainQuests(bot); }

void UnifiedQuestManager::SetTurnInStrategy(uint32 botGuid, TurnInStrategy strategy)
{ _turnIn->SetTurnInStrategy(botGuid, strategy); }

TurnInStrategy UnifiedQuestManager::GetTurnInStrategy(uint32 botGuid)
{ return _turnIn->GetTurnInStrategy(botGuid); }

void UnifiedQuestManager::SetRewardSelectionStrategy(uint32 botGuid, RewardSelectionStrategy strategy)
{ _turnIn->SetRewardSelectionStrategy(botGuid, strategy); }

RewardSelectionStrategy UnifiedQuestManager::GetRewardSelectionStrategy(uint32 botGuid)
{ return _turnIn->GetRewardSelectionStrategy(botGuid); }

void UnifiedQuestManager::SetBatchTurnInThreshold(uint32 botGuid, uint32 threshold)
{ _turnIn->SetBatchTurnInThreshold(botGuid, threshold); }

void UnifiedQuestManager::HandleTurnInError(Player* bot, uint32 questId, const std::string& error)
{ _turnIn->HandleTurnInError(bot, questId, error); }

void UnifiedQuestManager::RecoverFromTurnInFailure(Player* bot, uint32 questId)
{ _turnIn->RecoverFromTurnInFailure(bot, questId); }

void UnifiedQuestManager::RetryFailedTurnIn(Player* bot, uint32 questId)
{ _turnIn->RetryFailedTurnIn(bot, questId); }

void UnifiedQuestManager::ValidateTurnInState(Player* bot, uint32 questId)
{ _turnIn->ValidateTurnInState(bot, questId); }

// Dynamic Module delegation (continuing pattern)
std::vector<uint32> UnifiedQuestManager::DiscoverAvailableQuests(Player* bot)
{ return _dynamic->DiscoverAvailableQuests(bot); }

std::vector<uint32> UnifiedQuestManager::GetRecommendedQuests(Player* bot, QuestSelectionStrategy strategy)
{ return _dynamic->GetRecommendedQuests(bot, strategy); }

bool UnifiedQuestManager::AssignQuestToBot(uint32 questId, Player* bot)
{ return _dynamic->AssignQuestToBot(questId, bot); }

void UnifiedQuestManager::AutoAssignQuests(Player* bot, uint32 maxQuests)
{ _dynamic->AutoAssignQuests(bot, maxQuests); }

QuestPriority UnifiedQuestManager::CalculateQuestPriority(uint32 questId, Player* bot)
{ return _dynamic->CalculateQuestPriority(questId, bot); }

std::vector<uint32> UnifiedQuestManager::SortQuestsByPriority(const std::vector<uint32>& questIds, Player* bot)
{ return _dynamic->SortQuestsByPriority(questIds, bot); }

bool UnifiedQuestManager::ShouldAbandonQuest(uint32 questId, Player* bot)
{ return _dynamic->ShouldAbandonQuest(questId, bot); }

void UnifiedQuestManager::UpdateQuestProgressDynamic(Player* bot)
{ _dynamic->UpdateQuestProgressDynamic(bot); }

void UnifiedQuestManager::ExecuteQuestObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{ _dynamic->ExecuteQuestObjective(bot, questId, objectiveIndex); }

bool UnifiedQuestManager::CanCompleteQuestObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{ return _dynamic->CanCompleteQuestObjective(bot, questId, objectiveIndex); }

void UnifiedQuestManager::HandleQuestCompletionDynamic(Player* bot, uint32 questId)
{ _dynamic->HandleQuestCompletionDynamic(bot, questId); }

bool UnifiedQuestManager::FormQuestGroup(uint32 questId, Player* initiator)
{ return _dynamic->FormQuestGroup(questId, initiator); }

void UnifiedQuestManager::CoordinateGroupQuest(Group* group, uint32 questId)
{ _dynamic->CoordinateGroupQuest(group, questId); }

void UnifiedQuestManager::ShareQuestProgress(Group* group, uint32 questId)
{ _dynamic->ShareQuestProgress(group, questId); }

bool UnifiedQuestManager::CanShareQuest(uint32 questId, Player* from, Player* to)
{ return _dynamic->CanShareQuest(questId, from, to); }

Position UnifiedQuestManager::GetNextQuestLocation(Player* bot, uint32 questId)
{ return _dynamic->GetNextQuestLocation(bot, questId); }

std::vector<Position> UnifiedQuestManager::GenerateQuestPath(Player* bot, uint32 questId)
{ return _dynamic->GenerateQuestPath(bot, questId); }

void UnifiedQuestManager::HandleQuestNavigation(Player* bot, uint32 questId)
{ _dynamic->HandleQuestNavigation(bot, questId); }

bool UnifiedQuestManager::IsQuestLocationReachable(Player* bot, const Position& location)
{ return _dynamic->IsQuestLocationReachable(bot, location); }

void UnifiedQuestManager::AdaptQuestDifficulty(uint32 questId, Player* bot)
{ _dynamic->AdaptQuestDifficulty(questId, bot); }

void UnifiedQuestManager::HandleQuestStuckState(Player* bot, uint32 questId)
{ _dynamic->HandleQuestStuckState(bot, questId); }

void UnifiedQuestManager::RetryFailedObjective(Player* bot, uint32 questId, uint32 objectiveIndex)
{ _dynamic->RetryFailedObjective(bot, questId, objectiveIndex); }

void UnifiedQuestManager::OptimizeQuestOrder(Player* bot)
{ _dynamic->OptimizeQuestOrder(bot); }

void UnifiedQuestManager::TrackQuestChains(Player* bot)
{ _dynamic->TrackQuestChains(bot); }

std::vector<uint32> UnifiedQuestManager::GetQuestChain(uint32 questId)
{ return _dynamic->GetQuestChain(questId); }

uint32 UnifiedQuestManager::GetNextQuestInChainDynamic(uint32 completedQuestId)
{ return _dynamic->GetNextQuestInChainDynamic(completedQuestId); }

void UnifiedQuestManager::AdvanceQuestChain(Player* bot, uint32 completedQuestId)
{ _dynamic->AdvanceQuestChain(bot, completedQuestId); }

void UnifiedQuestManager::OptimizeZoneQuests(Player* bot)
{ _dynamic->OptimizeZoneQuests(bot); }

std::vector<uint32> UnifiedQuestManager::GetZoneQuests(uint32 zoneId, Player* bot)
{ return _dynamic->GetZoneQuests(zoneId, bot); }

void UnifiedQuestManager::PlanZoneCompletion(Player* bot, uint32 zoneId)
{ _dynamic->PlanZoneCompletion(bot, zoneId); }

bool UnifiedQuestManager::ShouldMoveToNewZone(Player* bot)
{ return _dynamic->ShouldMoveToNewZone(bot); }

QuestReward UnifiedQuestManager::AnalyzeQuestReward(uint32 questId, Player* bot)
{ return _dynamic->AnalyzeQuestReward(questId, bot); }

float UnifiedQuestManager::CalculateQuestValue(uint32 questId, Player* bot)
{ return _dynamic->CalculateQuestValue(questId, bot); }

bool UnifiedQuestManager::IsQuestWorthwhile(uint32 questId, Player* bot)
{ return _dynamic->IsQuestWorthwhile(questId, bot); }

void UnifiedQuestManager::SetQuestStrategy(uint32 botGuid, QuestSelectionStrategy strategy)
{ _dynamic->SetQuestStrategy(botGuid, strategy); }

QuestSelectionStrategy UnifiedQuestManager::GetQuestStrategy(uint32 botGuid)
{ return _dynamic->GetQuestStrategy(botGuid); }

void UnifiedQuestManager::SetMaxConcurrentQuests(uint32 botGuid, uint32 maxQuests)
{ _dynamic->SetMaxConcurrentQuests(botGuid, maxQuests); }

void UnifiedQuestManager::EnableQuestGrouping(uint32 botGuid, bool enable)
{ _dynamic->EnableQuestGrouping(botGuid, enable); }

// ============================================================================
// UNIFIED OPERATIONS IMPLEMENTATION
// ============================================================================

void UnifiedQuestManager::ProcessCompleteQuestFlow(Player* bot)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    auto startTime = GameTime::GetGameTimeMS();
    _totalOperations++;

    // 1. Discovery and validation
    auto availableQuests = _pickup->DiscoverNearbyQuests(bot, 100.0f);
    auto validQuests = _validation->FilterValidQuests(availableQuests, bot);

    // 2. Assignment and prioritization
    auto recommendedQuests = _dynamic->GetRecommendedQuests(bot, QuestSelectionStrategy::LEVEL_PROGRESSION);

    // 3. Execution and tracking
    _completion->UpdateQuestProgress(bot);

    // 4. Turn-in and reward selection
    auto completedQuests = _turnIn->GetCompletedQuests(bot);
    if (!completedQuests.empty())
    {
        _turnIn->ExecuteImmediateTurnInStrategy(bot);
    }

    auto endTime = GameTime::GetGameTimeMS();
    _totalProcessingTimeMs += (endTime - startTime);
}

std::string UnifiedQuestManager::GetQuestRecommendation(Player* bot, uint32 questId)
{
    std::ostringstream oss;

    // Validate eligibility
    bool isValid = _validation->ValidateQuest(questId, bot);
    auto eligibility = _pickup->CheckQuestEligibility(questId, bot);

    // Calculate priority
    auto priority = _dynamic->CalculateQuestPriority(questId, bot);

    // Analyze reward
    auto reward = _dynamic->AnalyzeQuestReward(questId, bot);
    float questValue = _dynamic->CalculateQuestValue(questId, bot);

    oss << "Quest " << questId << " Recommendation:\n";
    oss << "  Valid: " << (isValid ? "Yes" : "No") << "\n";
    oss << "  Priority: " << static_cast<uint32>(priority) << "\n";
    oss << "  Value: " << questValue << "\n";
    oss << "  Experience: " << reward.experience << "\n";
    oss << "  Gold: " << reward.gold << "\n";

    return oss.str();
}

void UnifiedQuestManager::OptimizeBotQuestLoad(Player* bot)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    // Optimize quest order
    _dynamic->OptimizeQuestOrder(bot);

    // Optimize completion paths
    _completion->OptimizeQuestCompletionOrder(bot);

    // Optimize turn-in routing
    _turnIn->PlanOptimalTurnInRoute(bot);

    // Optimize zone quests
    _dynamic->OptimizeZoneQuests(bot);
}

std::string UnifiedQuestManager::GetQuestStatistics() const
{
    std::ostringstream oss;
    oss << "=== Unified Quest Manager Statistics ===\n";
    oss << "Total Operations: " << _totalOperations.load() << "\n";
    oss << "Total Processing Time (ms): " << _totalProcessingTimeMs.load() << "\n";
    oss << "Quests Picked Up: " << _pickup->_questsPickedUp.load() << "\n";
    oss << "Quests Discovered: " << _pickup->_questsDiscovered.load() << "\n";
    oss << "Objectives Completed: " << _completion->_objectivesCompleted.load() << "\n";
    oss << "Quests Completed: " << _completion->_questsCompleted.load() << "\n";
    oss << "Validations Performed: " << _validation->_validationsPerformed.load() << "\n";
    oss << "Validations Passed: " << _validation->_validationsPassed.load() << "\n";
    oss << "Quests Turned In: " << _turnIn->_questsTurnedIn.load() << "\n";
    oss << "Rewards Selected: " << _turnIn->_rewardsSelected.load() << "\n";
    oss << "Quests Assigned: " << _dynamic->_questsAssigned.load() << "\n";
    oss << "Quests Optimized: " << _dynamic->_questsOptimized.load() << "\n";
    return oss.str();
}

QuestMetrics UnifiedQuestManager::GetBotQuestMetrics(uint32 botGuid)
{
    return _dynamic->GetBotQuestMetrics(botGuid);
}

QuestMetrics UnifiedQuestManager::GetGlobalQuestMetrics()
{
    return _dynamic->GetGlobalQuestMetrics();
}

QuestTurnIn::TurnInMetricsSnapshot UnifiedQuestManager::GetBotTurnInMetrics(uint32 botGuid)
{
    return _turnIn->GetBotTurnInMetrics(botGuid);
}

QuestTurnIn::TurnInMetricsSnapshot UnifiedQuestManager::GetGlobalTurnInMetrics()
{
    return _turnIn->GetGlobalTurnInMetrics();
}

ValidationMetrics UnifiedQuestManager::GetValidationMetrics()
{
    return _validation->GetValidationMetrics();
}

void UnifiedQuestManager::Update(uint32 diff)
{
    // Delegate updates to individual managers
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestCompletion()->Update(diff);
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->Update(diff);
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetDynamicQuestSystem()->Update(diff);
}

void UnifiedQuestManager::UpdateBotTurnIns(Player* bot, uint32 diff)
{
    if (IGameSystemsManager* systems = GetGameSystems(bot))
        systems->GetQuestTurnIn()->UpdateBotTurnIns(bot, diff);
}

void UnifiedQuestManager::ProcessScheduledTurnIns()
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->ProcessScheduledTurnIns();
}

void UnifiedQuestManager::CleanupCompletedTurnIns()
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetQuestTurnIn()->CleanupCompletedTurnIns();
}

void UnifiedQuestManager::CleanupCompletedQuests()
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetDynamicQuestSystem()->CleanupCompletedQuests();
}

void UnifiedQuestManager::ValidateQuestStates()
{
    if (IGameSystemsManager* systems = GetGameSystems(nullptr))
        systems->GetDynamicQuestSystem()->ValidateQuestStates();
}

} // namespace Playerbot
