/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Player.h"
#include "QuestDef.h"
#include "Creature.h"
#include <vector>
#include <unordered_set>

namespace Playerbot
{

/**
 * @brief Enterprise-Grade Quest Auto-Acceptance Manager
 *
 * Intelligent quest acceptance system that:
 * - Filters quests by eligibility (level, class, race, profession, reputation)
 * - Scores quests by priority (XP, gold, reputation, item rewards)
 * - Manages quest log capacity (drops low-priority quests when full)
 * - Avoids group/raid quests for solo bots
 * - Tracks quest chains and prerequisites
 * - Respects quest cooldowns and dailies
 */
class TC_GAME_API QuestAcceptanceManager
{
public:
    QuestAcceptanceManager(Player* bot);
    ~QuestAcceptanceManager() = default;

    // Main API
    void ProcessQuestGiver(Creature* questGiver);
    bool ShouldAcceptQuest(Quest const* quest) const;
    float CalculateQuestPriority(Quest const* quest) const;

    // Quest eligibility checks
    bool IsQuestEligible(Quest const* quest) const;
    bool MeetsLevelRequirement(Quest const* quest) const;
    bool MeetsClassRequirement(Quest const* quest) const;
    bool MeetsRaceRequirement(Quest const* quest) const;
    bool MeetsSkillRequirement(Quest const* quest) const;
    bool MeetsReputationRequirement(Quest const* quest) const;
    bool HasQuestLogSpace() const;
    bool IsGroupQuest(Quest const* quest) const;
    bool HasPrerequisites(Quest const* quest) const;

    // Quest management
    void AcceptQuest(Creature* questGiver, Quest const* quest);
    void DropLowestPriorityQuest();
    uint32 GetAvailableQuestLogSlots() const;

    // Quest priority factors
    float GetXPPriority(Quest const* quest) const;
    float GetGoldPriority(Quest const* quest) const;
    float GetReputationPriority(Quest const* quest) const;
    float GetItemRewardPriority(Quest const* quest) const;
    float GetZonePriority(Quest const* quest) const;
    float GetChainPriority(Quest const* quest) const;

    // Statistics
    uint32 GetQuestsAccepted() const { return _questsAccepted; }
    uint32 GetQuestsDropped() const { return _questsDropped; }

    // Quest blacklist (for quests abandoned due to missing items)
    void BlacklistQuest(uint32 questId);
    bool IsBlacklisted(uint32 questId) const;
    void ClearBlacklist() { _questBlacklist.clear(); }

private:
    Player* _bot;

    // Performance tracking
    uint32 _questsAccepted;
    uint32 _questsDropped;
    uint32 _lastAcceptTime;

    // Configuration
    static constexpr uint32 QUEST_ACCEPT_COOLDOWN = 1000; // 1 second between accepts
    static constexpr float MIN_QUEST_PRIORITY = 10.0f;     // Minimum priority to accept
    static constexpr uint32 RESERVE_QUEST_SLOTS = 2;       // Keep 2 slots free for important quests

    // Quest blacklist - quests that were abandoned due to missing items
    // These should not be re-accepted until the bot has the required items
    ::std::unordered_set<uint32> _questBlacklist;
};

} // namespace Playerbot
