/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_QUEST_EVENTS_H
#define PLAYERBOT_QUEST_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>

namespace Playerbot
{

/**
 * @brief Quest event types
 */
enum class QuestEventType : uint8
{
    QUEST_GIVER_STATUS = 0,
    QUEST_LIST_RECEIVED,
    QUEST_DETAILS_RECEIVED,
    QUEST_REQUEST_ITEMS,
    QUEST_OFFER_REWARD,
    QUEST_COMPLETED,
    QUEST_FAILED,
    QUEST_CREDIT_ADDED,
    QUEST_OBJECTIVE_COMPLETE,
    QUEST_UPDATE_FAILED,
    QUEST_CONFIRM_ACCEPT,
    QUEST_POI_RECEIVED,
    MAX_QUEST_EVENT
};

/**
 * @brief Quest event priorities
 */
enum class QuestEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

/**
 * @brief Quest states
 */
enum class QuestState : uint8
{
    NONE = 0,
    COMPLETE,
    UNAVAILABLE,
    INCOMPLETE,
    AVAILABLE,
    FAILED
};

/**
 * @brief Quest event structure
 *
 * Represents a single quest-related event that can be published and processed
 * by the QuestEventBus system.
 */
struct QuestEvent
{
    using EventType = QuestEventType;
    using Priority = QuestEventPriority;

    QuestEventType type;
    QuestEventPriority priority;
    ObjectGuid playerGuid;
    uint32 questId;
    uint32 objectiveId;
    int32 objectiveCount;
    QuestState state;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    /**
     * @brief Check if event is valid
     * @return true if event has valid data
     */
    bool IsValid() const
    {
        if (type >= QuestEventType::MAX_QUEST_EVENT)
            return false;
        if (timestamp.time_since_epoch().count() == 0)
            return false;
        if (playerGuid.IsEmpty())
            return false;
        return true;
    }

    /**
     * @brief Check if event has expired
     * @return true if current time >= expiry time
     */
    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }

    /**
     * @brief Convert event to string for logging
     * @return String representation of event
     */
    std::string ToString() const;

    /**
     * @brief Priority comparison for priority queue
     * @note Lower priority value = higher priority (CRITICAL > HIGH > MEDIUM > LOW)
     */
    bool operator<(QuestEvent const& other) const
    {
        return priority > other.priority;
    }

    // ========================================================================
    // HELPER CONSTRUCTORS
    // ========================================================================

    /**
     * @brief Create a quest accepted event
     * @param player Player who accepted the quest
     * @param questId Quest ID
     * @return Constructed QuestEvent
     */
    static QuestEvent QuestAccepted(ObjectGuid player, uint32 questId);

    /**
     * @brief Create a quest completed event
     * @param player Player who completed the quest
     * @param questId Quest ID
     * @return Constructed QuestEvent
     */
    static QuestEvent QuestCompleted(ObjectGuid player, uint32 questId);

    /**
     * @brief Create an objective progress event
     * @param player Player making progress
     * @param questId Quest ID
     * @param objId Objective ID
     * @param count Current objective count
     * @return Constructed QuestEvent
     */
    static QuestEvent ObjectiveProgress(ObjectGuid player, uint32 questId, uint32 objId, int32 count);
};

} // namespace Playerbot

#endif // PLAYERBOT_QUEST_EVENTS_H
