/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QuestAcceptanceManager.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "World.h"
#include "ItemTemplate.h"
#include "GameTime.h"

namespace Playerbot
{

QuestAcceptanceManager::QuestAcceptanceManager(Player* bot)
    : _bot(bot)
    , _questsAccepted(0)
    , _questsDropped(0)
    , _lastAcceptTime(0)
{
    TC_LOG_DEBUG("module.playerbot.quest", "QuestAcceptanceManager initialized for bot {}",
        _bot ? _bot->GetName() : "NULL");
}

// ========================================================================
// MAIN API
// ========================================================================

void QuestAcceptanceManager::ProcessQuestGiver(Creature* questGiver)
{
    if (!_bot || !questGiver)
        return;

    if (!questGiver->IsQuestGiver())
        return;

    TC_LOG_DEBUG("module.playerbot.quest", "Bot {} processing quest giver {} (Entry: {})",
        _bot->GetName(), questGiver->GetName(), questGiver->GetEntry());

    // Get all available quests from this NPC
    QuestRelationResult objectQR = sObjectMgr->GetCreatureQuestRelations(questGiver->GetEntry());
    ::std::vector<::std::pair<Quest const*, float>> eligibleQuests;

    // Filter and score quests
    for (uint32 questId : objectQR)
    {
        Quest const* questTemplate = sObjectMgr->GetQuestTemplate(questId);
        if (!questTemplate)
            continue;

        // Check if quest is eligible
    if (!IsQuestEligible(questTemplate))
            continue;
        // Calculate priority score
        float priority = CalculateQuestPriority(questTemplate);
        if (priority >= MIN_QUEST_PRIORITY)
        {
            eligibleQuests.emplace_back(questTemplate, priority);
        }
    }

    if (eligibleQuests.empty())
    {
        TC_LOG_DEBUG("module.playerbot.quest", "Bot {} found no eligible quests from {}",
            _bot->GetName(), questGiver->GetName());
        return;
    }

    // Sort by priority (highest first)
    ::std::sort(eligibleQuests.begin(), eligibleQuests.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    TC_LOG_INFO("module.playerbot.quest", "Bot {} found {} eligible quests from {} (highest priority: {:.1f})",
        _bot->GetName(), eligibleQuests.size(), questGiver->GetName(), eligibleQuests[0].second);

    // Accept quests until quest log is full
    for (auto const& [quest, priority] : eligibleQuests)
    {
        // Check if we need to make space
    if (!HasQuestLogSpace())
        {
            if (priority > 50.0f) // Only drop quests for high-priority new quests
            {
                DropLowestPriorityQuest();
            }
            else
            {
                TC_LOG_DEBUG("module.playerbot.quest", "Bot {} quest log full, skipping lower priority quests",
                    _bot->GetName());
                break;
            }
        }

        // Accept the quest
        AcceptQuest(questGiver, quest);

        // Cooldown between accepts
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastAcceptTime < QUEST_ACCEPT_COOLDOWN)
        {
            break; // Don't spam quest accepts
        }
    }
}

bool QuestAcceptanceManager::ShouldAcceptQuest(Quest const* quest) const
{
    if (!quest || !_bot)
        return false;

    return IsQuestEligible(quest) && CalculateQuestPriority(quest) >= MIN_QUEST_PRIORITY;
}

float QuestAcceptanceManager::CalculateQuestPriority(Quest const* quest) const
{
    if (!quest || !_bot)
        return 0.0f;

    float priority = 0.0f;

    // XP value (most important for leveling)
    priority += GetXPPriority(quest);

    // Gold value (important at all levels)
    priority += GetGoldPriority(quest);

    // Reputation value (important for unlocks)
    priority += GetReputationPriority(quest);

    // Item rewards (important for gear)
    priority += GetItemRewardPriority(quest);

    // Zone proximity (prefer nearby quests)
    priority += GetZonePriority(quest);

    // Quest chain value (prefer starting chains)
    priority += GetChainPriority(quest);

    TC_LOG_TRACE("module.playerbot.quest", "Quest {} priority: {:.1f} (XP={:.1f}, Gold={:.1f}, Rep={:.1f}, Zone={:.1f})",
        quest->GetQuestId(), priority, GetXPPriority(quest), GetGoldPriority(quest),
        GetReputationPriority(quest), GetZonePriority(quest));

    return priority;
}

// ========================================================================
// QUEST ELIGIBILITY CHECKS
// ========================================================================

bool QuestAcceptanceManager::IsQuestEligible(Quest const* quest) const
{
    if (!quest || !_bot)
        return false;

    // Check blacklist first - quests abandoned due to missing items
    if (IsBlacklisted(quest->GetQuestId()))
    {
        TC_LOG_DEBUG("module.playerbot.quest", "🚫 Quest {} '{}' rejected - BLACKLISTED for bot {}",
                     quest->GetQuestId(), quest->GetLogTitle(), _bot->GetName());
        return false;
    }

    // CRITICAL FIX: Use TrinityCore's comprehensive quest validation
    // This includes ALL checks: class, race, level, skills, reputation,
    // prerequisites, exclusivegroups, conditions table, expansions, etc.
    // Player::CanTakeQuest() checks everything including the conditions table
    // which filters quests that require other quests to be completed first.
    if (!_bot->CanTakeQuest(quest, false))
    {
        TC_LOG_TRACE("module.playerbot.quest", "Quest {} '{}' rejected by CanTakeQuest for bot {}",
                     quest->GetQuestId(), quest->GetLogTitle(), _bot->GetName());
        return false;
    }

    // Additional bot-specific checks
    // Avoid group quests for solo bots
    if (IsGroupQuest(quest) && !_bot->GetGroup())
    {
        TC_LOG_TRACE("module.playerbot.quest", "Quest {} '{}' rejected - group quest for solo bot {}",
                     quest->GetQuestId(), quest->GetLogTitle(), _bot->GetName());
        return false;
    }

    return true;
}

bool QuestAcceptanceManager::MeetsLevelRequirement(Quest const* quest) const
{
    if (!quest || !_bot)
        return false;

    uint32 botLevel = _bot->GetLevel();
    // Quest max level check (quest becomes unavailable above this level)
    if (quest->GetMaxLevel() > 0 && botLevel > quest->GetMaxLevel())
        return false;

    // Check XP value - if quest gives no XP, it's too low level (gray quest)
    // This is TrinityCore's way of determining if quest is appropriate level
    if (quest->XPValue(_bot) == 0 && botLevel > 1)
    {
        // Don't accept gray quests (no XP) unless bot is level 1
        return false;
    }

    return true;
}

bool QuestAcceptanceManager::MeetsClassRequirement(Quest const* quest) const
{
    if (!quest || !_bot)
        return false;

    if (quest->GetAllowableClasses() == 0)
        return true; // No class requirement

    return (quest->GetAllowableClasses() & (1 << (_bot->GetClass() - 1))) != 0;
}

bool QuestAcceptanceManager::MeetsRaceRequirement(Quest const* quest) const
{
    if (!quest || !_bot)
        return false;

    auto allowableRaces = quest->GetAllowableRaces();
    if (allowableRaces.IsEmpty())
        return true; // No race requirement

    return allowableRaces.HasRace(_bot->GetRace());
}
bool QuestAcceptanceManager::MeetsSkillRequirement(Quest const* quest) const
{
    if (!quest || !_bot)
        return false;

    if (quest->GetRequiredSkill() == 0)
        return true; // No skill requirement

    return _bot->HasSkill(quest->GetRequiredSkill()) &&
           _bot->GetSkillValue(quest->GetRequiredSkill()) >= quest->GetRequiredSkillValue();
}

bool QuestAcceptanceManager::MeetsReputationRequirement(Quest const* quest) const
{
    if (!quest || !_bot)
        return false;

    if (quest->GetRequiredMinRepFaction() == 0)
        return true; // No reputation requirement

    return _bot->GetReputation(quest->GetRequiredMinRepFaction()) >=
           quest->GetRequiredMinRepValue();
}

bool QuestAcceptanceManager::HasQuestLogSpace() const
{
    if (!_bot)
        return false;

    return GetAvailableQuestLogSlots() > RESERVE_QUEST_SLOTS;
}

bool QuestAcceptanceManager::IsGroupQuest(Quest const* quest) const
{
    if (!quest)
        return false;

    // Check if quest requires a group (suggested players > 1) or is a raid quest
    return quest->GetSuggestedPlayers() > 1 ||
           quest->IsRaidQuest(DIFFICULTY_NORMAL) ||
           quest->HasFlag(QUEST_FLAGS_RAID_GROUP_OK);
}

bool QuestAcceptanceManager::HasPrerequisites(Quest const* quest) const
{
    if (!quest || !_bot)
        return false;
    // CRITICAL DEBUG: Log prerequisite check for all quests
    TC_LOG_ERROR("module.playerbot.quest", " HasPrerequisites: Quest {} '{}' - GetPrevQuestId()={}, GetNextQuestInChain()={}",
                 quest->GetQuestId(), quest->GetLogTitle(), quest->GetPrevQuestId(), quest->GetNextQuestInChain());

    // Check previous quest in chain
    if (quest->GetPrevQuestId() != 0)
    {
        // Positive = must complete, Negative = must NOT complete
        int32 prevQuestId = quest->GetPrevQuestId();

        TC_LOG_ERROR("module.playerbot.quest", " HasPrerequisites: Quest {} requires PrevQuestID={}, checking if bot has rewarded it...",
                     quest->GetQuestId(), prevQuestId);

        if (prevQuestId > 0)
        {
            bool hasRewarded = _bot->GetQuestRewardStatus(prevQuestId);
            TC_LOG_ERROR("module.playerbot.quest", " HasPrerequisites: Bot {} GetQuestRewardStatus({})={}, quest {} prerequisite check={}",
                         _bot->GetName(), prevQuestId, hasRewarded, quest->GetQuestId(), hasRewarded ? "PASS" : "FAIL");

            if (!hasRewarded)
            {
                TC_LOG_ERROR("module.playerbot.quest", " HasPrerequisites: Quest {} REJECTED - prerequisite quest {} not completed",
                             quest->GetQuestId(), prevQuestId);
                return false; // Previous quest not completed
            }
        }
        else
        {
            if (_bot->GetQuestRewardStatus(-prevQuestId))
            {
                TC_LOG_ERROR("module.playerbot.quest", " HasPrerequisites: Quest {} REJECTED - must NOT have completed quest {}",
                             quest->GetQuestId(), -prevQuestId);
                return false; // Must NOT have completed this quest
            }
        }
    }

    // Check breadcrumb quests
    if (quest->GetNextQuestInChain() != 0)
    {
        // If bot already has the next quest, don't accept breadcrumb
    if (_bot->GetQuestStatus(quest->GetNextQuestInChain()) != QUEST_STATUS_NONE)
            return false;
    }

    return true;
}

// ========================================================================
// QUEST MANAGEMENT
// ========================================================================

void QuestAcceptanceManager::AcceptQuest(Creature* questGiver, Quest const* quest)
{
    if (!_bot || !questGiver || !quest)
        return;

    // Use TrinityCore API to accept quest
    _bot->AddQuestAndCheckCompletion(quest, questGiver);

    _questsAccepted++;
    _lastAcceptTime = GameTime::GetGameTimeMS();

    TC_LOG_INFO("module.playerbot.quest",
        "Bot {} AUTO-ACCEPTED quest {} '{}' (Priority: {:.1f}, Quests: {}/{})",
        _bot->GetName(), quest->GetQuestId(), quest->GetLogTitle(),
        CalculateQuestPriority(quest),
        MAX_QUEST_LOG_SIZE - GetAvailableQuestLogSlots(),
        MAX_QUEST_LOG_SIZE);
}

void QuestAcceptanceManager::DropLowestPriorityQuest()
{
    if (!_bot)
        return;

    Quest const* lowestPriorityQuest = nullptr;
    float lowestPriority = 99999.0f;

    // Find lowest priority quest in quest log
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = _bot->GetQuestSlotQuestId(slot);
        if (questId == 0)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        float priority = CalculateQuestPriority(quest);
        if (priority < lowestPriority)
        {
            lowestPriority = priority;
            lowestPriorityQuest = quest;
        }
    }

    if (lowestPriorityQuest)
    {
        TC_LOG_INFO("module.playerbot.quest", "Bot {} dropping low-priority quest {} '{}' (Priority: {:.1f})",
            _bot->GetName(), lowestPriorityQuest->GetQuestId(),
            lowestPriorityQuest->GetLogTitle(), lowestPriority);

        _bot->AbandonQuest(lowestPriorityQuest->GetQuestId());
        _questsDropped++;
    }
}

uint32 QuestAcceptanceManager::GetAvailableQuestLogSlots() const
{
    if (!_bot)
        return 0;

    uint32 usedSlots = 0;
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        if (_bot->GetQuestSlotQuestId(slot) != 0)
            usedSlots++;
    }

    return MAX_QUEST_LOG_SIZE - usedSlots;
}

// ========================================================================
// QUEST PRIORITY FACTORS
// ========================================================================

float QuestAcceptanceManager::GetXPPriority(Quest const* quest) const
{
    if (!quest || !_bot)
        return 0.0f;

    // No XP at max level
    if (_bot->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        return 0.0f;

    // Calculate XP reward
    uint32 xp = quest->XPValue(_bot);
    if (xp == 0)
        return 0.0f;

    // Higher XP = higher priority
    // Scale: 1000 XP = 10 priority points
    return ::std::min(50.0f, xp / 100.0f);
}

float QuestAcceptanceManager::GetGoldPriority(Quest const* quest) const
{
    if (!quest || !_bot)
        return 0.0f;

    uint32 gold = quest->GetRewMoneyMaxLevel();
    if (gold == 0)
        return 0.0f;

    // Scale: 1 gold = 1 priority point
    return ::std::min(20.0f, gold / 10000.0f);
}

float QuestAcceptanceManager::GetReputationPriority(Quest const* quest) const
{
    if (!quest || !_bot)
        return 0.0f;

    // Check reputation rewards
    float repPriority = 0.0f;
    for (uint8 i = 0; i < QUEST_REWARD_REPUTATIONS_COUNT; ++i)
    {
        if (quest->RewardFactionId[i] > 0 && quest->RewardFactionValue[i] > 0)
        {
            repPriority += 5.0f; // Each reputation reward adds 5 priority
        }
    }

    return ::std::min(15.0f, repPriority);
}

float QuestAcceptanceManager::GetItemRewardPriority(Quest const* quest) const
{
    if (!quest || !_bot)
        return 0.0f;

    float itemPriority = 0.0f;

    // Check reward items
    for (uint8 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
    {
        if (quest->RewardItemId[i] > 0)
        {
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(quest->RewardItemId[i]);
            if (itemTemplate)
            {
                // Higher quality = higher priority
    if (itemTemplate->GetQuality() >= ITEM_QUALITY_RARE)
                    itemPriority += 10.0f;
                else if (itemTemplate->GetQuality() >= ITEM_QUALITY_UNCOMMON)
                    itemPriority += 5.0f;
                else
                    itemPriority += 2.0f;
            }
        }
    }

    // Check reward choices
    for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
    {
        if (quest->RewardChoiceItemId[i] > 0)
        {
            itemPriority += 3.0f; // Reward choice adds priority
        }
    }

    return ::std::min(25.0f, itemPriority);
}

float QuestAcceptanceManager::GetZonePriority(Quest const* quest) const
{
    if (!quest || !_bot)
        return 0.0f;

    // Prefer quests in current zone
    if (quest->GetZoneOrSort() == static_cast<int32>(_bot->GetZoneId()))
        return 10.0f;

    // Nearby zones get some priority
    return 0.0f;
}

float QuestAcceptanceManager::GetChainPriority(Quest const* quest) const
{
    if (!quest)
        return 0.0f;

    // Starting a quest chain is valuable
    if (quest->GetPrevQuestId() == 0 && quest->GetNextQuestInChain() != 0)
        return 5.0f;

    // Continuing a chain is also good
    if (quest->GetNextQuestInChain() != 0)
        return 3.0f;

    return 0.0f;
}

// ========================================================================
// QUEST BLACKLIST
// ========================================================================

void QuestAcceptanceManager::BlacklistQuest(uint32 questId)
{
    _questBlacklist.insert(questId);
    TC_LOG_INFO("module.playerbot.quest", "🚫 Bot {} BLACKLISTED quest {} - will not re-accept until cleared",
        _bot ? _bot->GetName() : "NULL", questId);
}

bool QuestAcceptanceManager::IsBlacklisted(uint32 questId) const
{
    return _questBlacklist.find(questId) != _questBlacklist.end();
}

} // namespace Playerbot
