/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QueueShortageSubscriber.h"
#include "JITBotFactory.h"
#include "Core/Events/EventDispatcher.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR/DESTRUCTOR
// ============================================================================

QueueShortageSubscriber::QueueShortageSubscriber()
    : _enabled(true)
    , _eventThrottleSeconds(DEFAULT_EVENT_THROTTLE_SECONDS)
    , _eventsReceived(0)
    , _eventsProcessed(0)
    , _eventsThrottled(0)
    , _bgRequestsSubmitted(0)
    , _lfgRequestsSubmitted(0)
    , _arenaRequestsSubmitted(0)
{
}

QueueShortageSubscriber::~QueueShortageSubscriber() = default;

QueueShortageSubscriber* QueueShortageSubscriber::Instance()
{
    static QueueShortageSubscriber instance;
    return &instance;
}

// ============================================================================
// IManagerBase INTERFACE
// ============================================================================

bool QueueShortageSubscriber::Initialize()
{
    TC_LOG_INFO("playerbot.jit", "QueueShortageSubscriber: Initializing (throttle: {}s)",
        _eventThrottleSeconds);

    // Note: EventDispatcher subscription is done by the module initialization
    // after both systems are ready, to avoid circular dependency issues

    return true;
}

void QueueShortageSubscriber::Shutdown()
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    _lastEventTime.clear();

    TC_LOG_INFO("playerbot.jit", "QueueShortageSubscriber: Shutdown complete (processed {} events, throttled {})",
        _eventsProcessed.load(), _eventsThrottled.load());
}

void QueueShortageSubscriber::Update(uint32 /*diff*/)
{
    // No periodic work needed - all processing is event-driven
}

void QueueShortageSubscriber::OnEvent(StateMachine::EventType type, Events::BotEvent const& event)
{
    if (!_enabled)
        return;

    ++_eventsReceived;

    // Extract event data from the BotEvent
    // The event data is stored in eventData as std::any
    if (!event.eventData.has_value())
    {
        TC_LOG_DEBUG("playerbot.jit", "QueueShortageSubscriber: Event {} has no data", static_cast<uint32>(type));
        return;
    }

    // Handle based on event type
    switch (type)
    {
        case StateMachine::EventType::BG_QUEUE_SHORTAGE:
        {
            try
            {
                auto const* data = std::any_cast<Events::QueueShortageEventData const*>(event.eventData);
                if (data)
                    HandleBGShortage(data);
            }
            catch (std::bad_any_cast const&)
            {
                TC_LOG_ERROR("playerbot.jit", "QueueShortageSubscriber: Invalid event data for BG_QUEUE_SHORTAGE");
            }
            break;
        }
        case StateMachine::EventType::LFG_QUEUE_SHORTAGE:
        {
            try
            {
                auto const* data = std::any_cast<Events::QueueShortageEventData const*>(event.eventData);
                if (data)
                    HandleLFGShortage(data);
            }
            catch (std::bad_any_cast const&)
            {
                TC_LOG_ERROR("playerbot.jit", "QueueShortageSubscriber: Invalid event data for LFG_QUEUE_SHORTAGE");
            }
            break;
        }
        case StateMachine::EventType::ARENA_QUEUE_SHORTAGE:
        {
            try
            {
                auto const* data = std::any_cast<Events::QueueShortageEventData const*>(event.eventData);
                if (data)
                    HandleArenaShortage(data);
            }
            catch (std::bad_any_cast const&)
            {
                TC_LOG_ERROR("playerbot.jit", "QueueShortageSubscriber: Invalid event data for ARENA_QUEUE_SHORTAGE");
            }
            break;
        }
        default:
            break;
    }
}

// ============================================================================
// SHORTAGE HANDLING
// ============================================================================

void QueueShortageSubscriber::HandleBGShortage(Events::QueueShortageEventData const* data)
{
    if (!data)
        return;

    uint64 key = MakeQueueKey(data->contentType, data->contentId, data->bracketId);

    // Check throttling
    if (IsEventThrottled(key))
    {
        ++_eventsThrottled;
        TC_LOG_DEBUG("playerbot.jit", "QueueShortageSubscriber: BG shortage event throttled (content={})",
            data->contentId);
        return;
    }

    TC_LOG_INFO("playerbot.jit", "QueueShortageSubscriber: Processing BG shortage - Content={} Alliance need={} Horde need={}",
        data->contentId, data->allianceNeeded, data->hordeNeeded);

    // Submit JIT requests
    if (data->allianceNeeded > 0 || data->hordeNeeded > 0)
    {
        SubmitBGRequest(*data);
    }

    RecordEventHandled(key);
    ++_eventsProcessed;
}

void QueueShortageSubscriber::HandleLFGShortage(Events::QueueShortageEventData const* data)
{
    if (!data)
        return;

    uint64 key = MakeQueueKey(data->contentType, data->contentId, data->bracketId);

    if (IsEventThrottled(key))
    {
        ++_eventsThrottled;
        TC_LOG_DEBUG("playerbot.jit", "QueueShortageSubscriber: LFG shortage event throttled (dungeon={})",
            data->contentId);
        return;
    }

    TC_LOG_INFO("playerbot.jit", "QueueShortageSubscriber: Processing LFG shortage - Dungeon={} T:{} H:{} D:{}",
        data->contentId, data->tankNeeded, data->healerNeeded, data->dpsNeeded);

    if (data->tankNeeded > 0 || data->healerNeeded > 0 || data->dpsNeeded > 0)
    {
        SubmitLFGRequest(*data);
    }

    RecordEventHandled(key);
    ++_eventsProcessed;
}

void QueueShortageSubscriber::HandleArenaShortage(Events::QueueShortageEventData const* data)
{
    if (!data)
        return;

    uint64 key = MakeQueueKey(data->contentType, data->contentId, data->bracketId);

    if (IsEventThrottled(key))
    {
        ++_eventsThrottled;
        TC_LOG_DEBUG("playerbot.jit", "QueueShortageSubscriber: Arena shortage event throttled (type={})",
            data->contentId);
        return;
    }

    TC_LOG_INFO("playerbot.jit", "QueueShortageSubscriber: Processing Arena shortage - Type={} Alliance need={} Horde need={}",
        data->contentId, data->allianceNeeded, data->hordeNeeded);

    if (data->allianceNeeded > 0 || data->hordeNeeded > 0)
    {
        SubmitArenaRequest(*data);
    }

    RecordEventHandled(key);
    ++_eventsProcessed;
}

// ============================================================================
// JIT REQUEST SUBMISSION
// ============================================================================

void QueueShortageSubscriber::SubmitBGRequest(Events::QueueShortageEventData const& data)
{
    // Submit Alliance request if needed
    if (data.allianceNeeded > 0)
    {
        FactoryRequest request;
        request.instanceType = InstanceType::Battleground;
        request.contentId = data.contentId;
        request.playerFaction = Faction::Alliance;
        request.allianceNeeded = data.allianceNeeded;
        request.hordeNeeded = 0;
        request.priority = data.priority;
        request.createdAt = std::chrono::system_clock::now();

        request.onComplete = [contentId = data.contentId](std::vector<ObjectGuid> const& bots) {
            TC_LOG_DEBUG("playerbot.jit", "QueueShortageSubscriber: {} Alliance bots ready for BG {}",
                bots.size(), contentId);
        };

        request.onFailed = [contentId = data.contentId](std::string const& reason) {
            TC_LOG_WARN("playerbot.jit", "QueueShortageSubscriber: Failed to create Alliance bots for BG {}: {}",
                contentId, reason);
        };

        if (sJITBotFactory->SubmitRequest(std::move(request)) > 0)
        {
            ++_bgRequestsSubmitted;
        }
    }

    // Submit Horde request if needed
    if (data.hordeNeeded > 0)
    {
        FactoryRequest request;
        request.instanceType = InstanceType::Battleground;
        request.contentId = data.contentId;
        request.playerFaction = Faction::Horde;
        request.allianceNeeded = 0;
        request.hordeNeeded = data.hordeNeeded;
        request.priority = data.priority;
        request.createdAt = std::chrono::system_clock::now();

        request.onComplete = [contentId = data.contentId](std::vector<ObjectGuid> const& bots) {
            TC_LOG_DEBUG("playerbot.jit", "QueueShortageSubscriber: {} Horde bots ready for BG {}",
                bots.size(), contentId);
        };

        request.onFailed = [contentId = data.contentId](std::string const& reason) {
            TC_LOG_WARN("playerbot.jit", "QueueShortageSubscriber: Failed to create Horde bots for BG {}: {}",
                contentId, reason);
        };

        if (sJITBotFactory->SubmitRequest(std::move(request)) > 0)
        {
            ++_bgRequestsSubmitted;
        }
    }
}

void QueueShortageSubscriber::SubmitLFGRequest(Events::QueueShortageEventData const& data)
{
    FactoryRequest request;
    request.instanceType = InstanceType::Dungeon;
    request.contentId = data.contentId;
    request.tanksNeeded = data.tankNeeded;
    request.healersNeeded = data.healerNeeded;
    request.dpsNeeded = data.dpsNeeded;
    request.priority = data.priority;
    request.createdAt = std::chrono::system_clock::now();

    request.onComplete = [dungeonId = data.contentId](std::vector<ObjectGuid> const& bots) {
        TC_LOG_DEBUG("playerbot.jit", "QueueShortageSubscriber: {} bots ready for dungeon {}",
            bots.size(), dungeonId);
    };

    request.onFailed = [dungeonId = data.contentId](std::string const& reason) {
        TC_LOG_WARN("playerbot.jit", "QueueShortageSubscriber: Failed to create bots for dungeon {}: {}",
            dungeonId, reason);
    };

    if (sJITBotFactory->SubmitRequest(std::move(request)) > 0)
    {
        ++_lfgRequestsSubmitted;
    }
}

void QueueShortageSubscriber::SubmitArenaRequest(Events::QueueShortageEventData const& data)
{
    // Submit Alliance request if needed
    if (data.allianceNeeded > 0)
    {
        FactoryRequest request;
        request.instanceType = InstanceType::Arena;
        request.contentId = data.contentId;  // Arena type (2, 3, or 5)
        request.playerFaction = Faction::Alliance;
        request.allianceNeeded = data.allianceNeeded;
        request.priority = data.priority;
        request.createdAt = std::chrono::system_clock::now();

        request.onComplete = [arenaType = data.contentId](std::vector<ObjectGuid> const& bots) {
            TC_LOG_DEBUG("playerbot.jit", "QueueShortageSubscriber: {} Alliance bots ready for {}v{} arena",
                bots.size(), arenaType, arenaType);
        };

        if (sJITBotFactory->SubmitRequest(std::move(request)) > 0)
        {
            ++_arenaRequestsSubmitted;
        }
    }

    // Submit Horde request if needed
    if (data.hordeNeeded > 0)
    {
        FactoryRequest request;
        request.instanceType = InstanceType::Arena;
        request.contentId = data.contentId;
        request.playerFaction = Faction::Horde;
        request.hordeNeeded = data.hordeNeeded;
        request.priority = data.priority;
        request.createdAt = std::chrono::system_clock::now();

        request.onComplete = [arenaType = data.contentId](std::vector<ObjectGuid> const& bots) {
            TC_LOG_DEBUG("playerbot.jit", "QueueShortageSubscriber: {} Horde bots ready for {}v{} arena",
                bots.size(), arenaType, arenaType);
        };

        if (sJITBotFactory->SubmitRequest(std::move(request)) > 0)
        {
            ++_arenaRequestsSubmitted;
        }
    }
}

// ============================================================================
// THROTTLING
// ============================================================================

bool QueueShortageSubscriber::IsEventThrottled(uint64 queueKey) const
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto it = _lastEventTime.find(queueKey);
    if (it == _lastEventTime.end())
        return false;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);
    return elapsed.count() < static_cast<int64>(_eventThrottleSeconds);
}

void QueueShortageSubscriber::RecordEventHandled(uint64 queueKey)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    _lastEventTime[queueKey] = std::chrono::steady_clock::now();
}

uint64 QueueShortageSubscriber::MakeQueueKey(Events::ContentType type, uint32 contentId, uint32 bracketId)
{
    // Pack type (8 bits), contentId (32 bits), bracketId (24 bits) into 64-bit key
    return (static_cast<uint64>(type) << 56) |
           (static_cast<uint64>(contentId) << 24) |
           static_cast<uint64>(bracketId);
}

// ============================================================================
// STATISTICS
// ============================================================================

QueueShortageSubscriber::Statistics QueueShortageSubscriber::GetStatistics() const
{
    Statistics stats;
    stats.eventsReceived = _eventsReceived;
    stats.eventsProcessed = _eventsProcessed;
    stats.eventsThrottled = _eventsThrottled;
    stats.bgRequestsSubmitted = _bgRequestsSubmitted;
    stats.lfgRequestsSubmitted = _lfgRequestsSubmitted;
    stats.arenaRequestsSubmitted = _arenaRequestsSubmitted;
    return stats;
}

void QueueShortageSubscriber::ResetStatistics()
{
    _eventsReceived = 0;
    _eventsProcessed = 0;
    _eventsThrottled = 0;
    _bgRequestsSubmitted = 0;
    _lfgRequestsSubmitted = 0;
    _arenaRequestsSubmitted = 0;
}

} // namespace Playerbot
