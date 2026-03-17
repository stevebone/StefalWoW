/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QuestManager.h"
#include "AI/BotAI.h"
#include "Events/BotEventData.h"
#include "Events/BotEventTypes.h"
#include "Player.h"
#include "QuestDef.h"
#include "Log.h"
#include <any>

namespace Playerbot
{
    using namespace Events;
    using namespace StateMachine;

    /**
     * QuestManager::OnEventInternal Implementation
     *
     * Handles 13 quest-related events dispatched from observers.
     * Phase 7.2: Complete implementation - extracts event data and calls manager methods
     *
     * Quest Events:
     * - QUEST_ACCEPTED, QUEST_COMPLETED, QUEST_ABANDONED, QUEST_FAILED
     * - QUEST_OBJECTIVE_COMPLETE, QUEST_STATUS_CHANGED, QUEST_SHARED
     * - QUEST_AVAILABLE, QUEST_TURNED_IN, QUEST_REWARD_CHOSEN
     * - QUEST_ITEM_COLLECTED, QUEST_CREATURE_KILLED, QUEST_EXPLORATION
     */
    void QuestManager::OnEventInternal(Events::BotEvent const& event)
    {
        // Early exit for non-quest events
    if (!event.IsQuestEvent())
            return;

        Player* bot = GetBot();
        if (!bot || !bot->IsInWorld())
            return;

        // Handle quest events with full implementation
    switch (event.type)
        {
            case StateMachine::EventType::QUEST_ACCEPTED:
            {
                // Extract quest data from event
    if (!event.eventData.has_value())
                {
                    TC_LOG_ERROR("module.playerbot", "QuestManager::OnEventInternal: QUEST_ACCEPTED event {} missing data", event.eventId);
                    return;
                }

                QuestEventData questData;
                try
                {
                    questData = ::std::any_cast<QuestEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const& e)
                {
                    TC_LOG_ERROR("module.playerbot", "QuestManager::OnEventInternal: Failed to cast QUEST_ACCEPTED data for event {}: {}",
                        event.eventId, e.what());
                    return;
                }

                TC_LOG_INFO("module.playerbot", "QuestManager: Bot {} accepted quest {} ({}daily, {}weekly)",
                    bot->GetName(), questData.questId,
                    questData.isDaily ? "" : "not ", questData.isWeekly ? "" : "not ");

                // Force immediate update to refresh quest log
                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_COMPLETED:
            {
                // Extract quest data
    if (!event.eventData.has_value())
                {
                    TC_LOG_ERROR("module.playerbot", "QuestManager::OnEventInternal: QUEST_COMPLETED event {} missing data", event.eventId);
                    return;
                }

                QuestEventData questData;
                try
                {
                    questData = ::std::any_cast<QuestEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const& e)
                {
                    TC_LOG_ERROR("module.playerbot", "QuestManager::OnEventInternal: Failed to cast QUEST_COMPLETED data: {}", e.what());
                    return;
                }

                TC_LOG_INFO("module.playerbot", "QuestManager: Bot {} completed quest {} (XP: {}, Gold: {}, Rep: {})",
                    bot->GetName(), questData.questId, questData.experienceGained,
                    questData.goldReward, questData.reputationGained);

                // Attempt automatic turn-in if quest is complete
    if (questData.isComplete && IsQuestComplete(questData.questId))
                {
                    TC_LOG_DEBUG("module.playerbot", "QuestManager: Attempting auto turn-in for completed quest {}", questData.questId);
                    // Turn in will happen on next Update() cycle
                }

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_ABANDONED:
            {
                // Extract quest data
    if (!event.eventData.has_value())
                {
                    TC_LOG_WARN("module.playerbot", "QuestManager::OnEventInternal: QUEST_ABANDONED event {} missing data", event.eventId);
                    ForceUpdate();
                    return;
                }

                QuestEventData questData;
                try
                {
                    questData = ::std::any_cast<QuestEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const&)
                {
                    TC_LOG_WARN("module.playerbot", "QuestManager::OnEventInternal: Failed to cast QUEST_ABANDONED data");
                    ForceUpdate();
                    return;
                }
                TC_LOG_INFO("module.playerbot", "QuestManager: Bot {} abandoned quest {}",
                    bot->GetName(), questData.questId);

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_FAILED:
            {
                if (event.eventData.has_value())
                {
                    try
                    {
                        QuestEventData questData = ::std::any_cast<QuestEventData>(event.eventData);
                        TC_LOG_WARN("module.playerbot", "QuestManager: Bot {} failed quest {}",
                            bot->GetName(), questData.questId);
                    }
                    catch (::std::bad_any_cast const&)
                    {
                        TC_LOG_WARN("module.playerbot", "QuestManager: Bot {} failed quest (no data)", bot->GetName());
                    }
                }

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_OBJECTIVE_COMPLETE:
            {
                // Extract objective progress data
    if (!event.eventData.has_value())
                {
                    TC_LOG_DEBUG("module.playerbot", "QuestManager::OnEventInternal: QUEST_OBJECTIVE_COMPLETE missing data");
                    ForceUpdate();
                    return;
                }

                QuestEventData questData;
                try
                {
                    questData = ::std::any_cast<QuestEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const&)
                {
                    TC_LOG_WARN("module.playerbot", "QuestManager::OnEventInternal: Failed to cast QUEST_OBJECTIVE_COMPLETE data");
                    ForceUpdate();
                    return;
                }

                TC_LOG_DEBUG("module.playerbot", "QuestManager: Bot {} completed objective {} for quest {} ({}/{})",
                    bot->GetName(), questData.objectiveIndex, questData.questId,
                    questData.objectiveCount, questData.objectiveRequired);

                // Check if all objectives are now complete
                UpdateQuestProgress();

                if (IsQuestComplete(questData.questId))
                {
                    TC_LOG_INFO("module.playerbot", "QuestManager: Quest {} is now complete after objective completion",
                        questData.questId);
                }

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_STATUS_CHANGED:
            {
                if (event.eventData.has_value())
                {
                    try
                    {
                        QuestEventData questData = ::std::any_cast<QuestEventData>(event.eventData);
                        TC_LOG_DEBUG("module.playerbot", "QuestManager: Bot {} quest {} status changed (complete: {})",
                            bot->GetName(), questData.questId, questData.isComplete);
                    }
                    catch (::std::bad_any_cast const&) { }
                }
                UpdateQuestProgress();
                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_SHARED:
            {
                // Extract shared quest data
    if (!event.eventData.has_value())
                {
                    TC_LOG_DEBUG("module.playerbot", "QuestManager::OnEventInternal: QUEST_SHARED missing data");
                    return;
                }

                QuestEventData questData;
                try
                {
                    questData = ::std::any_cast<QuestEventData>(event.eventData);
                }
                catch (::std::bad_any_cast const&)
                {
                    TC_LOG_WARN("module.playerbot", "QuestManager::OnEventInternal: Failed to cast QUEST_SHARED data");
                    return;
                }

                TC_LOG_INFO("module.playerbot", "QuestManager: Bot {} received shared quest {}",
                    bot->GetName(), questData.questId);

                // Accept shared quest if eligible
    if (CanAcceptQuest(questData.questId))
                {
                    if (AcceptSharedQuest(questData.questId))
                    {
                        TC_LOG_INFO("module.playerbot", "QuestManager: Bot {} accepted shared quest {}",
                            bot->GetName(), questData.questId);
                    }
                    else
                    {
                        TC_LOG_DEBUG("module.playerbot", "QuestManager: Bot {} declined shared quest {} (not eligible or log full)",
                            bot->GetName(), questData.questId);
                    }
                }
                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_AVAILABLE:
            {
                TC_LOG_DEBUG("module.playerbot", "QuestManager: Quest giver available near bot {}", bot->GetName());

                // Trigger quest scan on next update
                ScanForQuests();
                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_TURNED_IN:
            {
                // Extract turn-in data
    if (event.eventData.has_value())
                {
                    try
                    {
                        QuestEventData questData = ::std::any_cast<QuestEventData>(event.eventData);
                        TC_LOG_INFO("module.playerbot", "QuestManager: Bot {} turned in quest {} (Reward: item {}, XP: {}, Gold: {})",
                            bot->GetName(), questData.questId, questData.rewardItemId,
                            questData.experienceGained, questData.goldReward);
                        // Check for quest chain continuation
    if (questData.nextQuestId != 0)
                        {
                            TC_LOG_DEBUG("module.playerbot", "QuestManager: Quest {} has follow-up quest {}",
                                questData.questId, questData.nextQuestId);

                            // Will auto-accept on next scan if available
                            ScanForQuests();
                        }
                    }
                    catch (::std::bad_any_cast const&)
                    {
                        TC_LOG_DEBUG("module.playerbot", "QuestManager: Bot {} turned in quest (no details)", bot->GetName());
                    }
                }

                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_REWARD_CHOSEN:
            {
                if (event.eventData.has_value())
                {
                    try
                    {
                        QuestEventData questData = ::std::any_cast<QuestEventData>(event.eventData);
                        TC_LOG_DEBUG("module.playerbot", "QuestManager: Bot {} chose reward item {} for quest {}",
                            bot->GetName(), questData.rewardItemId, questData.questId);
                    }
                    catch (::std::bad_any_cast const&) { }
                }
                break;
            }

            case StateMachine::EventType::QUEST_ITEM_COLLECTED:
            {
                // Extract item collection data
    if (event.eventData.has_value())
                {
                    try
                    {
                        QuestEventData questData = ::std::any_cast<QuestEventData>(event.eventData);
                        TC_LOG_DEBUG("module.playerbot", "QuestManager: Bot {} collected quest item for quest {} ({}/{})",
                            bot->GetName(), questData.questId,
                            questData.objectiveCount, questData.objectiveRequired);
                    }
                    catch (::std::bad_any_cast const&) { }
                }

                UpdateQuestProgress();
                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_CREATURE_KILLED:
            {
                // Extract kill credit data
    if (event.eventData.has_value())
                {
                    try
                    {
                        QuestEventData questData = ::std::any_cast<QuestEventData>(event.eventData);
                        TC_LOG_DEBUG("module.playerbot", "QuestManager: Bot {} kill credit for quest {} ({}/{})",
                            bot->GetName(), questData.questId,
                            questData.objectiveCount, questData.objectiveRequired);
                    }
                    catch (::std::bad_any_cast const&) { }
                }

                UpdateQuestProgress();
                ForceUpdate();
                break;
            }

            case StateMachine::EventType::QUEST_EXPLORATION:
            {
                // Extract exploration data
    if (event.eventData.has_value())
                {
                    try
                    {
                        QuestEventData questData = ::std::any_cast<QuestEventData>(event.eventData);
                        TC_LOG_DEBUG("module.playerbot", "QuestManager: Bot {} explored area for quest {}",
                            bot->GetName(), questData.questId);
                    }
                    catch (::std::bad_any_cast const&) { }
                }

                UpdateQuestProgress();
                ForceUpdate();
                break;
            }

            default:
                break;
        }
    }

} // namespace Playerbot
