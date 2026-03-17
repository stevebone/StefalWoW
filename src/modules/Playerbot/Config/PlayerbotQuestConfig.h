/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_PLAYERBOT_QUEST_CONFIG_H
#define TRINITYCORE_PLAYERBOT_QUEST_CONFIG_H

/**
 * Quest configuration additions for PlayerbotConfig
 *
 * Add these declarations to PlayerbotConfig.h:
 *
 * In public methods section:
 *     // Quest system configuration
 *     void LoadQuestConfig();
 *     bool IsQuestEnabled() const;
 *     bool IsQuestAutoAcceptEnabled() const;
 *     bool IsQuestAutoAcceptSharedEnabled() const;
 *     bool IsQuestAutoCompleteEnabled() const;
 *     uint32 GetQuestUpdateInterval() const;
 *     uint32 GetQuestCacheUpdateInterval() const;
 *     uint32 GetQuestMaxActive() const;
 *     uint32 GetQuestMaxTravelDistance() const;
 *     bool IsQuestDailyEnabled() const;
 *     bool IsQuestDungeonEnabled() const;
 *     bool IsQuestPrioritizeGroupEnabled() const;
 *     uint32 GetQuestStrategy() const;
 *
 * In private members section:
 *     // Quest configuration
 *     bool m_questEnabled;
 *     bool m_questAutoAccept;
 *     bool m_questAutoAcceptShared;
 *     bool m_questAutoComplete;
 *     uint32 m_questUpdateInterval;
 *     uint32 m_questCacheUpdateInterval;
 *     uint32 m_questMaxActive;
 *     uint32 m_questMaxTravelDistance;
 *     bool m_questDailyEnabled;
 *     bool m_questDungeonEnabled;
 *     bool m_questPrioritizeGroup;
 *     uint32 m_questStrategy;
 *
 * Call LoadQuestConfig() in PlayerbotConfig::Load() method
 */

#endif // TRINITYCORE_PLAYERBOT_QUEST_CONFIG_H