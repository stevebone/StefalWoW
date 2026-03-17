/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QuestValidation.h"
#include "Player.h"
#include "QuestDef.h"
#include "ObjectMgr.h"
#include "Group.h"
#include "DatabaseEnv.h"
#include "World.h"
#include "Map.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "Log.h"
#include "GameTime.h"
#include "ReputationMgr.h"
#include <sstream>
#include <iomanip>

namespace Playerbot
{

QuestValidation::QuestValidation(Player* bot) : _bot(bot)
{
    if (!_bot) TC_LOG_ERROR("playerbot.quest", "QuestValidation: null bot!");
}

QuestValidation::~QuestValidation() {}

bool QuestValidation::ValidateQuestAcceptance(uint32 questId, Player* bot)
{

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);

    // Check cached validation first
    if (_enableCaching)
    {
        ValidationResult cached = GetCachedValidation(questId, bot->GetGUID().GetCounter());
        if (cached.cacheExpiry > GameTime::GetGameTimeMS())
        {
            _metrics.cacheHits++;
            return cached.isValid;
        }
        _metrics.cacheMisses++;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Core validations
    bool valid = true;
    ValidationResult result;

    // Level check
    if (!ValidateLevelRequirements(questId, bot))
    {
        int32 minLevel = bot->GetQuestMinLevel(quest);
        if (bot->GetLevel() < static_cast<uint32>(minLevel))
            result.eligibility = QuestEligibility::LEVEL_TOO_LOW;
        else
            result.eligibility = QuestEligibility::LEVEL_TOO_HIGH;
        result.errors.push_back("Level requirement not met");
        valid = false;
    }

    // Class/Race check
    if (valid && !ValidateClassRequirements(questId, bot))
    {
        result.eligibility = QuestEligibility::CLASS_LOCKED;
        result.errors.push_back("Class requirement not met");
        valid = false;
    }

    if (valid && !ValidateRaceRequirements(questId, bot))
    {
        result.eligibility = QuestEligibility::RACE_LOCKED;
        result.errors.push_back("Race requirement not met");
        valid = false;
    }

    // Prerequisites
    if (valid && !ValidateQuestPrerequisites(questId, bot))
    {
        result.eligibility = QuestEligibility::MISSING_PREREQ;
        result.errors.push_back("Prerequisite quests not completed");
        valid = false;
    }

    // Status checks
    if (valid && !ValidateQuestStatus(questId, bot))
    {
        if (IsQuestAlreadyCompleted(questId, bot))
            result.eligibility = QuestEligibility::ALREADY_DONE;
        else if (IsQuestInProgress(questId, bot))
            result.eligibility = QuestEligibility::ALREADY_HAVE;
        else if (IsQuestLogFull(bot))
            result.eligibility = QuestEligibility::QUEST_LOG_FULL;

        result.errors.push_back("Quest status invalid");
        valid = false;
    }

    // Faction/Reputation
    if (valid && !ValidateFactionRequirements(questId, bot))
    {
        result.eligibility = QuestEligibility::FACTION_LOCKED;
        result.errors.push_back("Faction requirement not met");
        valid = false;
    }

    // Items
    if (valid && !ValidateRequiredItems(questId, bot))
    {
        result.eligibility = QuestEligibility::ITEM_REQUIRED;
        result.errors.push_back("Required items missing");
        valid = false;
    }

    // Inventory space
    if (valid && !ValidateInventorySpace(questId, bot))
    {
        result.eligibility = QuestEligibility::QUEST_LOG_FULL;
        result.errors.push_back("Insufficient inventory space");
        valid = false;
    }

    // Skills
    if (valid && !ValidateSkillRequirements(questId, bot))
    {
        result.eligibility = QuestEligibility::SKILL_REQUIRED;
        result.errors.push_back("Skill requirement not met");
        valid = false;
    }

    // Availability
    if (valid && !ValidateQuestAvailability(questId, bot))
    {
        result.eligibility = QuestEligibility::NOT_AVAILABLE;
        result.errors.push_back("Quest not currently available");
        valid = false;
    }

    result.isValid = valid;
    if (valid)
        result.eligibility = QuestEligibility::ELIGIBLE;

    // Cache result
    if (_enableCaching)
        CacheValidationResult(questId, bot->GetGUID().GetCounter(), result);
    // Update metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    _metrics.totalValidations++;
    if (valid)
        _metrics.passedValidations++;
    else
        _metrics.failedValidations++;

    // Update average time (exponential moving average)
    float currentAvg = _metrics.averageValidationTime;
    _metrics.averageValidationTime = currentAvg * 0.9f + duration * 0.1f;

    return valid;
}

QuestEligibility QuestValidation::GetDetailedEligibility(uint32 questId, Player* bot)
{
    if (!bot)
        return QuestEligibility::NOT_AVAILABLE;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return QuestEligibility::NOT_AVAILABLE;

    // Check in priority order
    if (!ValidateLevelRequirements(questId, bot))
    {
        int32 minLevel = bot->GetQuestMinLevel(quest);
        if (bot->GetLevel() < static_cast<uint32>(minLevel))
            return QuestEligibility::LEVEL_TOO_LOW;
        else
            return QuestEligibility::LEVEL_TOO_HIGH;
    }

    if (!ValidateClassRequirements(questId, bot))
        return QuestEligibility::CLASS_LOCKED;

    if (!ValidateRaceRequirements(questId, bot))
        return QuestEligibility::RACE_LOCKED;

    if (!ValidateQuestPrerequisites(questId, bot))
        return QuestEligibility::MISSING_PREREQ;

    if (IsQuestAlreadyCompleted(questId, bot))
        return QuestEligibility::ALREADY_DONE;

    if (IsQuestInProgress(questId, bot))
        return QuestEligibility::ALREADY_HAVE;
    if (IsQuestLogFull(bot))
        return QuestEligibility::QUEST_LOG_FULL;

    if (!ValidateFactionRequirements(questId, bot))
        return QuestEligibility::FACTION_LOCKED;

    if (!ValidateSkillRequirements(questId, bot))
        return QuestEligibility::SKILL_REQUIRED;

    if (!ValidateRequiredItems(questId, bot))
        return QuestEligibility::ITEM_REQUIRED;

    if (!ValidateQuestAvailability(questId, bot))
        return QuestEligibility::NOT_AVAILABLE;

    return QuestEligibility::ELIGIBLE;
}

std::vector<std::string> QuestValidation::GetValidationErrors(uint32 questId, Player* bot)
{
    std::vector<std::string> errors;

    if (!bot)
    {
        errors.push_back("Invalid bot pointer");
        return errors;
    }

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
    {
        errors.push_back("Quest template not found");
        return errors;
    }

    if (!ValidateLevelRequirements(questId, bot))
    {
        std::ostringstream oss;
        int32 minLevel = bot->GetQuestMinLevel(quest);
        oss << "Level requirement: " << minLevel << " (bot level: " << bot->GetLevel() << ")";
        errors.push_back(oss.str());
    }

    if (!ValidateClassRequirements(questId, bot))
        errors.push_back("Class requirement not met");

    if (!ValidateRaceRequirements(questId, bot))
        errors.push_back("Race requirement not met");

    if (!ValidateQuestPrerequisites(questId, bot))
    {
        auto missing = GetMissingPrerequisites(questId, bot);
        if (!missing.empty())
        {
            std::ostringstream oss;
            oss << "Missing prerequisites: ";
            for (size_t i = 0; i < missing.size(); ++i)
            {
                if (i > 0) oss << ", ";
                oss << missing[i];
            }
            errors.push_back(oss.str());
        }
    }

    if (!ValidateRequiredItems(questId, bot))
    {
        auto missingItems = GetMissingItems(questId, bot);
        if (!missingItems.empty())
        {
            std::ostringstream oss;
            oss << "Missing items: ";
            for (size_t i = 0; i < missingItems.size(); ++i)
            {
                if (i > 0) oss << ", ";
                oss << missingItems[i].second << "x item " << missingItems[i].first;
            }
            errors.push_back(oss.str());
        }
    }

    if (!ValidateFactionRequirements(questId, bot))
        errors.push_back("Faction/reputation requirement not met");

    if (!ValidateSkillRequirements(questId, bot))
        errors.push_back("Skill requirement not met");
    if (IsQuestLogFull(bot))
        errors.push_back("Quest log is full");

    if (!ValidateQuestAvailability(questId, bot))
        errors.push_back("Quest not currently available");

    return errors;
}

bool QuestValidation::CanQuestBeStarted(uint32 questId, Player* bot)
{
    return ValidateQuestAcceptance(questId, bot);
}

// ========== Requirement Validation ==========

bool QuestValidation::ValidateLevelRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    uint32 botLevel = bot->GetLevel();
    int32 minLevel = bot->GetQuestMinLevel(quest);
    uint32 maxLevel = quest->GetMaxLevel();

    // Check minimum level
    if (minLevel > 0 && botLevel < static_cast<uint32>(minLevel))
        return false;

    // Check maximum level (if set)
    if (maxLevel > 0 && botLevel > maxLevel)
        return false;

    return true;
}

bool QuestValidation::ValidateClassRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if quest has class requirements
    uint32 reqClasses = quest->GetAllowableClasses();
    if (reqClasses == 0)
        return true; // No class restriction

    uint32 botClass = bot->GetClass();
    return (reqClasses & (1 << (botClass - 1))) != 0;
}

bool QuestValidation::ValidateRaceRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if quest has race requirements
    auto allowableRaces = quest->GetAllowableRaces();
    if (allowableRaces.IsEmpty())
        return true; // No race restriction

    uint8 botRace = bot->GetRace();
    return allowableRaces.HasRace(botRace);
}

bool QuestValidation::ValidateFactionRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Validate reputation requirements
    return ValidateReputationRequirements(questId, bot);
}

bool QuestValidation::ValidateSkillRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check required skill
    uint32 reqSkill = quest->GetRequiredSkill();
    if (reqSkill == 0)
        return true; // No skill requirement

    uint32 reqSkillValue = quest->GetRequiredSkillValue();
    uint16 botSkillValue = bot->GetSkillValue(reqSkill);

    return botSkillValue >= reqSkillValue;
}

// ========== Prerequisite Validation ==========

bool QuestValidation::ValidateQuestPrerequisites(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check previous quest requirement
    uint32 prevQuestId = quest->GetPrevQuestId();
    if (prevQuestId != 0)
    {
        // If prevQuestId is negative, it's an alternative prerequisite
        if (prevQuestId > 0)
        {
            if (!bot->GetQuestRewardStatus(prevQuestId))
                return false;
        }
    }

    // Check next quest requirement
    uint32 nextQuestId = quest->GetNextQuestId();
    if (nextQuestId != 0)
    {
        // Quest is part of a chain, check if bot should be at this step
        if (nextQuestId < 0) // Exclusive group
        {
            // Check if any quest in exclusive group is completed
            // (Implementation depends on TrinityCore version)
        }
    }

    return true;
}

bool QuestValidation::ValidateQuestChainPosition(uint32 questId, Player* bot)
{
    return ValidateQuestPrerequisites(questId, bot);
}

std::vector<uint32> QuestValidation::GetMissingPrerequisites(uint32 questId, Player* bot)
{
    std::vector<uint32> missing;

    if (!bot)
        return missing;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return missing;

    uint32 prevQuestId = quest->GetPrevQuestId();
    if (prevQuestId > 0 && !bot->GetQuestRewardStatus(prevQuestId))
    {
        missing.push_back(prevQuestId);
    }

    return missing;
}

bool QuestValidation::HasCompletedPrerequisiteQuests(uint32 questId, Player* bot)
{
    auto missing = GetMissingPrerequisites(questId, bot);
    return missing.empty();
}

// ========== Item and Inventory Validation ==========

bool QuestValidation::ValidateRequiredItems(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check required source item
    uint32 srcItem = quest->GetSrcItemId();
    if (srcItem != 0)
    {
        uint32 srcCount = quest->GetSrcItemCount();
        if (!bot->HasItemCount(srcItem, srcCount))
            return false;
    }

    return true;
}

bool QuestValidation::ValidateInventorySpace(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if bot can store quest reward items
    for (uint8 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
    {
        if (quest->RewardItemId[i] != 0)
        {
            ItemPosCountVec dest;
            InventoryResult result = bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, quest->RewardItemId[i], quest->RewardItemCount[i]);
            if (result != EQUIP_ERR_OK)
                return false;
        }
    }

    return true;
}

bool QuestValidation::ValidateQuestItemRequirements(uint32 questId, Player* bot)
{
    return ValidateRequiredItems(questId, bot);
}

std::vector<std::pair<uint32, uint32>> QuestValidation::GetMissingItems(uint32 questId, Player* bot)
{
    std::vector<std::pair<uint32, uint32>> missingItems;

    if (!bot)
        return missingItems;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return missingItems;

    uint32 srcItem = quest->GetSrcItemId();
    if (srcItem != 0)
    {
        uint32 srcCount = quest->GetSrcItemCount();
        uint32 botCount = bot->GetItemCount(srcItem);
        if (botCount < srcCount)
        {
            missingItems.push_back(std::make_pair(srcItem, srcCount - botCount));
        }
    }

    return missingItems;
}

// ========== Status and State Validation ==========

bool QuestValidation::ValidateQuestStatus(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    // Quest must not be already completed (unless repeatable)
    if (IsQuestAlreadyCompleted(questId, bot) && !IsQuestRepeatable(questId, bot))
        return false;

    // Quest must not be in progress
    if (IsQuestInProgress(questId, bot))
        return false;

    // Quest log must not be full
    if (IsQuestLogFull(bot))
        return false;

    return true;
}

bool QuestValidation::IsQuestAlreadyCompleted(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    return bot->GetQuestRewardStatus(questId);
}

bool QuestValidation::IsQuestInProgress(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    QuestStatus status = bot->GetQuestStatus(questId);
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}

bool QuestValidation::IsQuestLogFull(Player* bot)
{
    if (!bot)
        return true;

    return bot->getQuestStatusMap().size() >= MAX_QUEST_LOG_SIZE;
}

bool QuestValidation::IsQuestRepeatable(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    return quest->IsRepeatable() || quest->IsDaily() || quest->IsWeekly() || quest->IsMonthly();
}

// ========== Reputation and Standing Validation ==========

bool QuestValidation::ValidateReputationRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check minimum reputation
    if (!ValidateMinimumReputation(questId, bot))
        return false;

    // Check maximum reputation
    if (!ValidateMaximumReputation(questId, bot))
        return false;

    return true;
}

bool QuestValidation::ValidateMinimumReputation(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    uint32 reqFaction = quest->GetRequiredMinRepFaction();
    if (reqFaction == 0)
        return true; // No minimum reputation requirement

    int32 reqValue = quest->GetRequiredMinRepValue();
    int32 botRep = bot->GetReputationMgr().GetReputation(reqFaction);

    return botRep >= reqValue;
}

bool QuestValidation::ValidateMaximumReputation(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    uint32 reqFaction = quest->GetRequiredMaxRepFaction();
    if (reqFaction == 0)
        return true; // No maximum reputation requirement

    int32 reqValue = quest->GetRequiredMaxRepValue();
    int32 botRep = bot->GetReputationMgr().GetReputation(reqFaction);

    return botRep <= reqValue;
}

std::vector<std::pair<uint32, int32>> QuestValidation::GetReputationRequirements(uint32 questId)
{
    std::vector<std::pair<uint32, int32>> requirements;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return requirements;

    uint32 minFaction = quest->GetRequiredMinRepFaction();
    if (minFaction != 0)
    {
        int32 minValue = quest->GetRequiredMinRepValue();
        requirements.push_back(std::make_pair(minFaction, minValue));
    }

    uint32 maxFaction = quest->GetRequiredMaxRepFaction();
    if (maxFaction != 0)
    {
        int32 maxValue = quest->GetRequiredMaxRepValue();
        requirements.push_back(std::make_pair(maxFaction, -maxValue)); // Negative to indicate maximum
    }

    return requirements;
}

// ========== Time and Availability Validation ==========

bool QuestValidation::ValidateQuestAvailability(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    // Check seasonal availability
    if (!ValidateSeasonalAvailability(questId))
        return false;

    // Check daily quest limits
    if (!ValidateDailyQuestLimits(questId, bot))
        return false;

    // Check quest timer
    if (!ValidateQuestTimer(questId, bot))
        return false;

    return true;
}

bool QuestValidation::ValidateSeasonalAvailability(uint32 questId)
{
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if quest is seasonal/holiday quest
    // (Implementation depends on TrinityCore event system)
    return true; // Simplified - always available for now
}

bool QuestValidation::ValidateDailyQuestLimits(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    if (quest->IsDaily())
    {
        // Check if bot has already completed this daily quest today
        if (bot->IsDailyQuestDone(questId))
            return false;
    }

    return true;
}

bool QuestValidation::ValidateQuestTimer(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if quest has a timer and validate it
    // (Most quests don't have acceptance timers)
    return true;
}

// ========== Zone and Location Validation ==========

bool QuestValidation::ValidateZoneRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check zone requirement
    // (Implementation depends on quest data structure)
    return true; // Simplified
}

bool QuestValidation::ValidateAreaRequirements(uint32 questId, Player* bot)
{
    return ValidateZoneRequirements(questId, bot);
}

bool QuestValidation::IsInCorrectZone(uint32 questId, Player* bot)
{
    return ValidateZoneRequirements(questId, bot);
}

bool QuestValidation::CanQuestBeStartedAtLocation(uint32 questId, const Position& location)
{
    // Validate quest can be started at given location
    // (Depends on quest giver location data)
    return true; // Simplified
}

// ========== Group and Party Validation ==========

bool QuestValidation::ValidateGroupRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if quest suggests group (via SuggestedPlayers field)
    if (quest->GetSuggestedPlayers() > 1)
    {
        Group* group = bot->GetGroup();
        if (!group || group->GetMembersCount() < quest->GetSuggestedPlayers())
            return false;
    }

    return true;
}

bool QuestValidation::ValidatePartyQuestRequirements(uint32 questId, Player* bot)
{
    return ValidateGroupRequirements(questId, bot);
}

bool QuestValidation::ValidateRaidQuestRequirements(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if quest requires raid (via RAID_GROUP_OK flag means it CAN be done in raid, not that it REQUIRES it)
    // For simplicity, assume quests with suggested players > 5 might be raid quests
    if (quest->GetSuggestedPlayers() > 5)
    {
        Group* group = bot->GetGroup();
        if (!group || !group->isRaidGroup())
            return false;
    }

    return true;
}

bool QuestValidation::CanGroupMemberShareQuest(uint32 questId, Player* sharer, Player* receiver)
{
    if (!sharer || !receiver)
        return false;

    // Both players must meet quest requirements
    if (!ValidateQuestAcceptance(questId, receiver))
        return false;

    // Must be in same group
    Group* group = sharer->GetGroup();
    if (!group || group != receiver->GetGroup())
        return false;

    return true;
}

// ========== Advanced Validation ==========

bool QuestValidation::ValidateWithContext(ValidationContext& context)
{
    if (!context.bot || context.questId == 0)
    {
        context.errors.push_back("Invalid validation context");
        return false;
    }

    context.quest = sObjectMgr->GetQuestTemplate(context.questId);
    if (!context.quest)
    {
        context.errors.push_back("Quest template not found");
        return false;
    }

    bool result = ValidateQuestAcceptance(context.questId, context.bot);

    // Populate context with detailed errors
    if (!result)
    {
        context.errors = GetValidationErrors(context.questId, context.bot);
    }

    return result;
}

bool QuestValidation::ValidateQuestObjectives(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Validate objectives are achievable by bot
    // (Detailed objective validation)
    return true;
}

bool QuestValidation::ValidateQuestRewards(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Validate bot can receive rewards
    return ValidateInventorySpace(questId, bot);
}

bool QuestValidation::ValidateQuestDifficulty(uint32 questId, Player* bot)
{
    if (!bot)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if quest is appropriate difficulty for bot level
    uint32 botLevel = bot->GetLevel();
    int32 questLevel = bot->GetQuestLevel(quest);

    // Quest should be within reasonable range
    int32 levelDiff = questLevel - static_cast<int32>(botLevel);
    return (levelDiff >= -5 && levelDiff <= 10); // Flexible range
}

// ========== Validation Caching and Optimization ==========

ValidationResult QuestValidation::GetCachedValidation(uint32 questId, uint32 botGuid)
{
    uint64_t key = ((uint64_t)questId << 32) | botGuid;
    auto it = _validationCache.find(key);

    if (it != _validationCache.end() && it->second.cacheExpiry > GameTime::GetGameTimeMS())
    {
        return it->second;
    }

    return ValidationResult(); // Invalid result
}

void QuestValidation::CacheValidationResult(uint32 questId, uint32 botGuid, const ValidationResult& result)
{
    uint64_t key = ((uint64_t)questId << 32) | botGuid;
    ValidationResult cached = result;
    cached.cacheExpiry = GameTime::GetGameTimeMS() + _cacheTimeoutMs;
    _validationCache[key] = cached;
}

void QuestValidation::InvalidateValidationCache(uint32 botGuid)
{
    // Remove all entries for this bot
    auto it = _validationCache.begin();
    while (it != _validationCache.end())
    {
        uint32_t cachedBot = it->first & 0xFFFFFFFF;
        if (cachedBot == botGuid)
            it = _validationCache.erase(it);
        else
            ++it;
    }
}

void QuestValidation::CleanupExpiredCache()
{
    uint32 now = GameTime::GetGameTimeMS();
    auto it = _validationCache.begin();
    while (it != _validationCache.end())
    {
        if (it->second.cacheExpiry < now)
            it = _validationCache.erase(it);
        else
            ++it;
    }
}

// ========== Batch Validation ==========

std::unordered_map<uint32, ValidationResult> QuestValidation::ValidateMultipleQuests(
    const std::vector<uint32>& questIds, Player* bot)
{
    std::unordered_map<uint32, ValidationResult> results;

    for (uint32 questId : questIds)
    {
        ValidationResult result;
        result.isValid = ValidateQuestAcceptance(questId, bot);
        result.eligibility = GetDetailedEligibility(questId, bot);
        result.errors = GetValidationErrors(questId, bot);
        results[questId] = result;
    }

    return results;
}

std::vector<uint32> QuestValidation::FilterValidQuests(const std::vector<uint32>& questIds, Player* bot)
{
    std::vector<uint32> valid;

    for (uint32 questId : questIds)
    {
        if (ValidateQuestAcceptance(questId, bot))
            valid.push_back(questId);
    }

    return valid;
}

std::vector<uint32> QuestValidation::GetEligibleQuests(Player* bot, const std::vector<uint32>& candidates)
{
    return FilterValidQuests(candidates, bot);
}

// ========== Error Reporting and Diagnostics ==========

std::string QuestValidation::GetDetailedValidationReport(uint32 questId, Player* bot)
{
    std::ostringstream report;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
    {
        report << "Quest " << questId << " not found\n";
        return report.str();
    }

    report << "Quest Validation Report for Quest " << questId << " (" << quest->GetLogTitle() << ")\n";
    report << "Bot: " << (bot ? bot->GetName() : "NULL") << "\n\n";

    if (!bot)
    {
        report << "ERROR: Bot pointer is null\n";
        return report.str();
    }

    QuestEligibility eligibility = GetDetailedEligibility(questId, bot);
    report << "Eligibility: ";
    switch (eligibility)
    {
        case QuestEligibility::ELIGIBLE: report << "ELIGIBLE\n"; break;
        case QuestEligibility::LEVEL_TOO_LOW: report << "LEVEL_TOO_LOW\n"; break;
        case QuestEligibility::LEVEL_TOO_HIGH: report << "LEVEL_TOO_HIGH\n"; break;
        case QuestEligibility::MISSING_PREREQ: report << "MISSING_PREREQ\n"; break;
        case QuestEligibility::ALREADY_HAVE: report << "ALREADY_HAVE\n"; break;
        case QuestEligibility::ALREADY_DONE: report << "ALREADY_DONE\n"; break;
        case QuestEligibility::QUEST_LOG_FULL: report << "QUEST_LOG_FULL\n"; break;
        case QuestEligibility::FACTION_LOCKED: report << "FACTION_LOCKED\n"; break;
        case QuestEligibility::CLASS_LOCKED: report << "CLASS_LOCKED\n"; break;
        case QuestEligibility::RACE_LOCKED: report << "RACE_LOCKED\n"; break;
        case QuestEligibility::SKILL_REQUIRED: report << "SKILL_REQUIRED\n"; break;
        case QuestEligibility::ITEM_REQUIRED: report << "ITEM_REQUIRED\n"; break;
        default: report << "NOT_AVAILABLE\n"; break;
    }

    auto errors = GetValidationErrors(questId, bot);
    if (!errors.empty())
    {
        report << "\nValidation Errors:\n";
        for (const auto& error : errors)
        {
            report << "  - " << error << "\n";
        }
    }

    return report.str();
}

void QuestValidation::LogValidationFailure(uint32 questId, Player* bot, const std::string& reason)
{
    if (bot)
    {
        TC_LOG_WARN("module.playerbot", "Quest validation failed for bot {} (quest {}): {}",
            bot->GetName(), questId, reason);
    }
}

std::vector<std::string> QuestValidation::GetRecommendationsForFailedQuest(uint32 questId, Player* bot)
{
    std::vector<std::string> recommendations;

    if (!bot)
    {
        recommendations.push_back("Invalid bot");
        return recommendations;
    }

    QuestEligibility eligibility = GetDetailedEligibility(questId, bot);

    switch (eligibility)
    {
        case QuestEligibility::LEVEL_TOO_LOW:
            recommendations.push_back("Level up before attempting this quest");
            break;
        case QuestEligibility::MISSING_PREREQ:
        {
            auto missing = GetMissingPrerequisites(questId, bot);
            if (!missing.empty())
            {
                std::ostringstream oss;
                oss << "Complete prerequisite quests: ";
                for (size_t i = 0; i < missing.size(); ++i)
                {
                    if (i > 0) oss << ", ";
                    oss << missing[i];
                }
                recommendations.push_back(oss.str());
            }
            break;
        }
        case QuestEligibility::QUEST_LOG_FULL:
            recommendations.push_back("Complete or abandon some quests to free up quest log space");
            break;
        case QuestEligibility::ITEM_REQUIRED:
        {
            auto missingItems = GetMissingItems(questId, bot);
            if (!missingItems.empty())
            {
                std::ostringstream oss;
                oss << "Acquire required items: ";
                for (size_t i = 0; i < missingItems.size(); ++i)
                {
                    if (i > 0) oss << ", ";
                    oss << missingItems[i].second << "x item " << missingItems[i].first;
                }
                recommendations.push_back(oss.str());
            }
            break;
        }
        default:
            recommendations.push_back("Quest cannot be accepted at this time");
            break;
    }

    return recommendations;
}

/**
 * @brief Update validation system
 * @param diff Time difference in milliseconds
 */
void QuestValidation::Update(uint32 diff)
{
    static uint32 cacheCleanupTimer = 0;
    cacheCleanupTimer += diff;

    // Periodically clean up expired cache entries
    if (cacheCleanupTimer >= CACHE_CLEANUP_INTERVAL)
    {
        CleanupExpiredCache();
        cacheCleanupTimer = 0;

        TC_LOG_DEBUG("playerbot", "QuestValidation::Update - Cleaned up expired cache entries");
    }

    // Update validation success rate
    uint32 totalValidations = _metrics.totalValidations;
    uint32 passedValidations = _metrics.passedValidations;
    if (totalValidations > 0)
    {
        float successRate = static_cast<float>(passedValidations) / totalValidations;
        _metrics.validationSuccessRate = successRate;
    }

    // Limit cache size if it grows too large
    if (_validationCache.size() > MAX_CACHE_SIZE)
    {
        TC_LOG_WARN("playerbot", "QuestValidation::Update - Cache size exceeded limit (%zu entries), cleaning up oldest entries",
            _validationCache.size());

        // Remove oldest entries (simple approach: clear half the cache)
        size_t targetSize = MAX_CACHE_SIZE / 2;
        auto it = _validationCache.begin();
        size_t toRemove = _validationCache.size() - targetSize;

        for (size_t i = 0; i < toRemove && it != _validationCache.end(); ++i)
        {
            it = _validationCache.erase(it);
        }
    }

    // Log performance warnings if validation is slow
    float avgTime = _metrics.averageValidationTime;
    if (avgTime > VALIDATION_TIME_WARNING_THRESHOLD)
    {
        TC_LOG_WARN("playerbot", "QuestValidation::Update - Average validation time is high: %.2f ms (threshold: %.2f ms)",
            avgTime, VALIDATION_TIME_WARNING_THRESHOLD);
    }
}

} // namespace Playerbot
