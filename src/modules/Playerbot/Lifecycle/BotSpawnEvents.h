/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_BOTSPAWN_EVENTS_H
#define PLAYERBOT_BOTSPAWN_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Lifecycle/SpawnRequest.h"
#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace Playerbot
{

// Forward declarations
class BotSession;

/**
 * @brief Event types for bot spawning workflow
 */
enum class BotSpawnEventType : uint32
{
    SPAWN_REQUESTED = 1,
    CHARACTER_SELECTED = 2,
    SESSION_CREATED = 3,
    SPAWN_COMPLETED = 4,
    SPAWN_FAILED = 5,
    POPULATION_CHANGED = 6,
    PERFORMANCE_ALERT = 7
};

enum class BotSpawnEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

/**
 * @brief Base class for all bot spawn events
 */
struct BotSpawnEvent
{
    using EventType = BotSpawnEventType;
    using Priority = BotSpawnEventPriority;

    BotSpawnEventType type;
    std::chrono::steady_clock::time_point timestamp;
    uint64 eventId;

    BotSpawnEvent(BotSpawnEventType eventType)
        : type(eventType), timestamp(std::chrono::steady_clock::now()), eventId(0) {}

    virtual ~BotSpawnEvent() = default;
};

/**
 * @brief Spawn request event
 */
struct SpawnRequestEvent : public BotSpawnEvent
{
    SpawnRequest request;
    std::function<void(bool, ObjectGuid)> callback;

    SpawnRequestEvent(SpawnRequest req, std::function<void(bool, ObjectGuid)> cb)
        : BotSpawnEvent(BotSpawnEventType::SPAWN_REQUESTED), request(std::move(req)), callback(std::move(cb)) {}
};

/**
 * @brief Character selected event
 */
struct CharacterSelectedEvent : public BotSpawnEvent
{
    ObjectGuid characterGuid;
    SpawnRequest originalRequest;

    CharacterSelectedEvent(ObjectGuid guid, SpawnRequest req)
        : BotSpawnEvent(BotSpawnEventType::CHARACTER_SELECTED), characterGuid(guid), originalRequest(std::move(req)) {}
};

/**
 * @brief Session created event
 */
struct SessionCreatedEvent : public BotSpawnEvent
{
    std::shared_ptr<BotSession> session;
    SpawnRequest originalRequest;

    SessionCreatedEvent(std::shared_ptr<BotSession> sess, SpawnRequest req)
        : BotSpawnEvent(BotSpawnEventType::SESSION_CREATED), session(std::move(sess)), originalRequest(std::move(req)) {}
};

/**
 * @brief Spawn completed event
 */
struct SpawnCompletedEvent : public BotSpawnEvent
{
    ObjectGuid botGuid;
    bool success;
    std::string details;

    SpawnCompletedEvent(ObjectGuid guid, bool succeeded, std::string info = "")
        : BotSpawnEvent(BotSpawnEventType::SPAWN_COMPLETED), botGuid(guid), success(succeeded), details(std::move(info)) {}
};

/**
 * @brief Population changed event
 */
struct PopulationChangedEvent : public BotSpawnEvent
{
    uint32 zoneId;
    uint32 oldBotCount;
    uint32 newBotCount;

    PopulationChangedEvent(uint32 zone, uint32 oldCount, uint32 newCount)
        : BotSpawnEvent(BotSpawnEventType::POPULATION_CHANGED), zoneId(zone), oldBotCount(oldCount), newBotCount(newCount) {}
};

/**
 * @brief Spawn failed event
 */
struct SpawnFailedEvent : public BotSpawnEvent
{
    ObjectGuid characterGuid;
    std::string reason;
    uint32 errorCode;

    SpawnFailedEvent(ObjectGuid guid, std::string failureReason, uint32 code = 0)
        : BotSpawnEvent(BotSpawnEventType::SPAWN_FAILED), characterGuid(guid), reason(std::move(failureReason)), errorCode(code) {}
};

/**
 * @brief Performance alert event
 */
struct PerformanceAlertEvent : public BotSpawnEvent
{
    std::string alertMessage;
    uint32 severity;  // 1=info, 2=warning, 3=critical
    uint32 affectedBotCount;

    PerformanceAlertEvent(std::string message, uint32 sev, uint32 botCount = 0)
        : BotSpawnEvent(BotSpawnEventType::PERFORMANCE_ALERT), alertMessage(std::move(message)), severity(sev), affectedBotCount(botCount) {}
};

} // namespace Playerbot

#endif // PLAYERBOT_BOTSPAWN_EVENTS_H
