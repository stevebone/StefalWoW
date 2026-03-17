/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotConfig.h"
#include "Config/PlayerbotConfig.h"
#include "Log.h"

namespace Playerbot
{
    /**
     * Quest system configuration implementation
     *
     * Add these methods to PlayerbotConfig class:
     */

    void PlayerbotConfig::LoadQuestConfig()
    {
        // Quest system enable/disable
        m_questEnabled = sPlayerbotConfig->GetBool("Playerbot.Quest.Enable", true);

        // Quest automation settings
        m_questAutoAccept = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoAccept", true);
        m_questAutoAcceptShared = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoAcceptShared", true);
        m_questAutoComplete = sPlayerbotConfig->GetBool("Playerbot.Quest.AutoComplete", true);

        // Quest system timings
        m_questUpdateInterval = sPlayerbotConfig->GetInt("Playerbot.Quest.UpdateInterval", 5000);
        m_questCacheUpdateInterval = sPlayerbotConfig->GetInt("Playerbot.Quest.CacheUpdateInterval", 30000);

        // Quest limits
        m_questMaxActive = sPlayerbotConfig->GetInt("Playerbot.Quest.MaxActiveQuests", 20);
        m_questMaxTravelDistance = sPlayerbotConfig->GetInt("Playerbot.Quest.MaxTravelDistance", 1000);

        // Quest types
        m_questDailyEnabled = sPlayerbotConfig->GetBool("Playerbot.Quest.PrioritizeDaily", true);
        m_questDungeonEnabled = sPlayerbotConfig->GetBool("Playerbot.Quest.AcceptDungeon", false);
        m_questPrioritizeGroup = sPlayerbotConfig->GetBool("Playerbot.Quest.PrioritizeGroup", true);

        // Quest strategy
        ::std::string strategy = sPlayerbotConfig->GetString("Playerbot.Quest.SelectionStrategy", "optimal");
        if (strategy == "simple")
            m_questStrategy = 0;
        else if (strategy == "optimal")
            m_questStrategy = 1;
        else if (strategy == "group")
            m_questStrategy = 2;
        else if (strategy == "completionist")
            m_questStrategy = 3;
        else if (strategy == "speed")
            m_questStrategy = 4;
        else
            m_questStrategy = 1; // Default to optimal

        TC_LOG_INFO("bot.playerbot", "Loaded quest configuration:");
        TC_LOG_INFO("bot.playerbot", "  Quest System: %s", m_questEnabled ? "Enabled" : "Disabled");
        TC_LOG_INFO("bot.playerbot", "  Auto Accept: %s", m_questAutoAccept ? "Yes" : "No");
        TC_LOG_INFO("bot.playerbot", "  Auto Complete: %s", m_questAutoComplete ? "Yes" : "No");
        TC_LOG_INFO("bot.playerbot", "  Update Interval: %u ms", m_questUpdateInterval);
        TC_LOG_INFO("bot.playerbot", "  Max Active Quests: %u", m_questMaxActive);
        TC_LOG_INFO("bot.playerbot", "  Accept Dailies: %s", m_questDailyEnabled ? "Yes" : "No");
        TC_LOG_INFO("bot.playerbot", "  Accept Dungeon Quests: %s", m_questDungeonEnabled ? "Yes" : "No");
    }

    // Quest system getters
    bool PlayerbotConfig::IsQuestEnabled() const { return m_questEnabled; }
    bool PlayerbotConfig::IsQuestAutoAcceptEnabled() const { return m_questAutoAccept; }
    bool PlayerbotConfig::IsQuestAutoAcceptSharedEnabled() const { return m_questAutoAcceptShared; }
    bool PlayerbotConfig::IsQuestAutoCompleteEnabled() const { return m_questAutoComplete; }
    uint32 PlayerbotConfig::GetQuestUpdateInterval() const { return m_questUpdateInterval; }
    uint32 PlayerbotConfig::GetQuestCacheUpdateInterval() const { return m_questCacheUpdateInterval; }
    uint32 PlayerbotConfig::GetQuestMaxActive() const { return m_questMaxActive; }
    uint32 PlayerbotConfig::GetQuestMaxTravelDistance() const { return m_questMaxTravelDistance; }
    bool PlayerbotConfig::IsQuestDailyEnabled() const { return m_questDailyEnabled; }
    bool PlayerbotConfig::IsQuestDungeonEnabled() const { return m_questDungeonEnabled; }
    bool PlayerbotConfig::IsQuestPrioritizeGroupEnabled() const { return m_questPrioritizeGroup; }
    uint32 PlayerbotConfig::GetQuestStrategy() const { return m_questStrategy; }

} // namespace Playerbot