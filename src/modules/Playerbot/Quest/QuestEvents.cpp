/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QuestEvents.h"
#include <sstream>

namespace Playerbot
{

QuestEvent QuestEvent::QuestAccepted(ObjectGuid player, uint32 questId)
{
    QuestEvent event;
    event.type = QuestEventType::QUEST_CONFIRM_ACCEPT;
    event.priority = QuestEventPriority::HIGH;
    event.playerGuid = player;
    event.questId = questId;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::INCOMPLETE;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

QuestEvent QuestEvent::QuestCompleted(ObjectGuid player, uint32 questId)
{
    QuestEvent event;
    event.type = QuestEventType::QUEST_COMPLETED;
    event.priority = QuestEventPriority::HIGH;
    event.playerGuid = player;
    event.questId = questId;
    event.objectiveId = 0;
    event.objectiveCount = 0;
    event.state = QuestState::COMPLETE;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(10000);
    return event;
}

QuestEvent QuestEvent::ObjectiveProgress(ObjectGuid player, uint32 questId, uint32 objId, int32 count)
{
    QuestEvent event;
    event.type = QuestEventType::QUEST_OBJECTIVE_COMPLETE;
    event.priority = QuestEventPriority::MEDIUM;
    event.playerGuid = player;
    event.questId = questId;
    event.objectiveId = objId;
    event.objectiveCount = count;
    event.state = QuestState::INCOMPLETE;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(15000);
    return event;
}

std::string QuestEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[QuestEvent] Type: " << static_cast<uint32>(type)
        << ", Player: " << playerGuid.ToString()
        << ", Quest: " << questId
        << ", Objective: " << objectiveId
        << " (" << objectiveCount << ")";
    return oss.str();
}

} // namespace Playerbot
