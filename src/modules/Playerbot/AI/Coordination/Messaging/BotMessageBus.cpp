/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotMessageBus.h"
#include "AI/BotAI.h"
#include "Log.h"

namespace Playerbot
{

BotMessageBus::BotMessageBus()
{
    TC_LOG_INFO("playerbot.messaging", "BotMessageBus initialized");
}

bool BotMessageBus::Subscribe(BotAI* botAI, ObjectGuid groupGuid, std::vector<BotMessageType> const& types,
                               uint8 role, uint8 subGroup)
{
    if (!botAI || groupGuid.IsEmpty())
        return false;

    Player* bot = botAI->GetBot();
    if (!bot)
        return false;

    ObjectGuid botGuid = bot->GetGUID();

    std::lock_guard lock(_mutex);

    GroupMessageQueue& group = GetOrCreateGroup(groupGuid);

    BotSubscription& sub = group.subscribers[botGuid];
    sub.botGuid = botGuid;
    sub.botAI = botAI;
    sub.subscribedTypes = types;
    sub.role = role;
    sub.subGroup = subGroup;

    group.lastActivity = std::chrono::steady_clock::now();

    _stats.activeSubscribers++;

    TC_LOG_DEBUG("playerbot.messaging", "BotMessageBus: {} subscribed to group {} (role={}, subGroup={})",
        botGuid.ToString(), groupGuid.ToString(), role, subGroup);

    return true;
}

void BotMessageBus::Unsubscribe(ObjectGuid botGuid, ObjectGuid groupGuid)
{
    std::lock_guard lock(_mutex);

    if (!groupGuid.IsEmpty())
    {
        auto it = _groups.find(groupGuid);
        if (it != _groups.end())
        {
            if (it->second.subscribers.erase(botGuid) > 0)
            {
                _stats.activeSubscribers--;
                TC_LOG_DEBUG("playerbot.messaging", "BotMessageBus: {} unsubscribed from group {}",
                    botGuid.ToString(), groupGuid.ToString());
            }
        }
    }
    else
    {
        // Unsubscribe from all groups
        for (auto& [gid, group] : _groups)
        {
            if (group.subscribers.erase(botGuid) > 0)
            {
                _stats.activeSubscribers--;
            }
        }
        TC_LOG_DEBUG("playerbot.messaging", "BotMessageBus: {} unsubscribed from all groups",
            botGuid.ToString());
    }

    // Also release any claims
    ClaimResolver::instance()->ReleaseAllClaims(botGuid);
}

void BotMessageBus::UpdateSubscription(ObjectGuid botGuid, ObjectGuid groupGuid, uint8 role, uint8 subGroup)
{
    std::lock_guard lock(_mutex);

    auto groupIt = _groups.find(groupGuid);
    if (groupIt == _groups.end())
        return;

    auto subIt = groupIt->second.subscribers.find(botGuid);
    if (subIt != groupIt->second.subscribers.end())
    {
        subIt->second.role = role;
        subIt->second.subGroup = subGroup;
        TC_LOG_DEBUG("playerbot.messaging", "BotMessageBus: {} updated in group {} (role={}, subGroup={})",
            botGuid.ToString(), groupGuid.ToString(), role, subGroup);
    }
}

bool BotMessageBus::Publish(BotMessage const& message)
{
    if (!message.IsValid())
    {
        TC_LOG_ERROR("playerbot.messaging", "BotMessageBus: Invalid message rejected: {}",
            GetMessageTypeName(message.type));
        _stats.totalMessagesDropped++;
        return false;
    }

    // Route claims through ClaimResolver
    if (message.IsClaim())
    {
        ClaimStatus status = PublishClaim(message, nullptr);
        return status == ClaimStatus::PENDING || status == ClaimStatus::GRANTED;
    }

    std::lock_guard lock(_mutex);

    auto groupIt = _groups.find(message.groupGuid);
    if (groupIt == _groups.end())
    {
        TC_LOG_DEBUG("playerbot.messaging", "BotMessageBus: No group {} for message {}",
            message.groupGuid.ToString(), GetMessageTypeName(message.type));
        _stats.totalMessagesDropped++;
        return false;
    }

    GroupMessageQueue& group = groupIt->second;

    // Check queue size
    if (group.messages.size() >= _maxQueueSize)
    {
        TC_LOG_WARN("playerbot.messaging", "BotMessageBus: Queue full for group {}, message dropped",
            message.groupGuid.ToString());
        _stats.totalMessagesDropped++;
        return false;
    }

    // Assign message ID
    BotMessage msg = message;
    msg.messageId = _nextMessageId++;

    group.messages.push(msg);
    group.lastActivity = std::chrono::steady_clock::now();

    _stats.totalMessagesPublished++;

    TC_LOG_DEBUG("playerbot.messaging", "BotMessageBus: Published {} to group {}",
        GetMessageTypeName(message.type), message.groupGuid.ToString());

    return true;
}

ClaimStatus BotMessageBus::PublishClaim(BotMessage const& message, ClaimResolver::ClaimCallback callback)
{
    if (!message.IsClaim())
    {
        TC_LOG_ERROR("playerbot.messaging", "BotMessageBus: Non-claim message passed to PublishClaim");
        return ClaimStatus::DENIED;
    }

    _stats.totalClaimsSubmitted++;

    // Submit to ClaimResolver
    return ClaimResolver::instance()->SubmitClaim(message, callback);
}

bool BotMessageBus::SendDirect(BotMessage message, ObjectGuid recipientGuid)
{
    message.scope = MessageScope::DIRECT;
    message.targetGuid = recipientGuid;

    std::lock_guard lock(_mutex);

    // Find the group containing this recipient
    for (auto& [groupGuid, group] : _groups)
    {
        auto subIt = group.subscribers.find(recipientGuid);
        if (subIt != group.subscribers.end())
        {
            message.groupGuid = groupGuid;
            message.messageId = _nextMessageId++;

            group.messages.push(message);
            group.lastActivity = std::chrono::steady_clock::now();

            _stats.totalMessagesPublished++;
            return true;
        }
    }

    TC_LOG_DEBUG("playerbot.messaging", "BotMessageBus: Recipient {} not found for direct message",
        recipientGuid.ToString());
    _stats.totalMessagesDropped++;
    return false;
}

uint32 BotMessageBus::ProcessMessages(uint32 maxMessages)
{
    auto now = std::chrono::steady_clock::now();

    // Process pending claims first
    ClaimResolver::instance()->ProcessPendingClaims(now);
    ClaimResolver::instance()->CleanupExpiredClaims(now);

    std::lock_guard lock(_mutex);

    uint32 totalProcessed = 0;

    for (auto& [groupGuid, group] : _groups)
    {
        uint32 processed = 0;

        while (!group.messages.empty() && processed < maxMessages)
        {
            BotMessage message = group.messages.top();
            group.messages.pop();

            // Skip expired messages
            if (message.IsExpired())
                continue;

            // Deliver to subscribers
            DeliverMessage(group, message);

            processed++;
            totalProcessed++;
            _stats.totalMessagesDelivered++;
        }
    }

    return totalProcessed;
}

void BotMessageBus::DeliverMessage(GroupMessageQueue& group, BotMessage const& message)
{
    for (auto& [botGuid, sub] : group.subscribers)
    {
        if (!ShouldDeliver(sub, message))
            continue;

        // Don't deliver to sender
        if (botGuid == message.senderGuid)
            continue;

        // Deliver via BotAI::HandleBotMessage
        if (sub.botAI)
        {
            try
            {
                sub.botAI->HandleBotMessage(message);
                TC_LOG_TRACE("playerbot.messaging", "BotMessageBus: Delivered {} to {}",
                    GetMessageTypeName(message.type), botGuid.ToString());
            }
            catch (std::exception const& e)
            {
                TC_LOG_ERROR("playerbot.messaging", "BotMessageBus: Delivery exception to {}: {}",
                    botGuid.ToString(), e.what());
            }
        }
    }
}

bool BotMessageBus::ShouldDeliver(BotSubscription const& sub, BotMessage const& message) const
{
    // Check scope
    switch (message.scope)
    {
        case MessageScope::GROUP_BROADCAST:
            // Deliver to everyone
            break;

        case MessageScope::ROLE_BROADCAST:
            // Only deliver to matching role
            if (sub.role != message.targetRole)
                return false;
            break;

        case MessageScope::SUBGROUP_BROADCAST:
            // Only deliver to matching subgroup
            if (sub.subGroup != message.subGroup)
                return false;
            break;

        case MessageScope::DIRECT:
            // Only deliver to specific target
            if (sub.botGuid != message.targetGuid)
                return false;
            break;

        case MessageScope::NEARBY_BROADCAST:
            // LIMITATION: Distance-based broadcast filtering not yet implemented.
            // Currently falls through to deliver to all subscribers of matching type.
            // Enhancement: Use SpatialGridManager::QueryNearbyPlayers() for proximity filtering.
            break;
    }

    // Check type subscription (empty = subscribe to all)
    if (!sub.subscribedTypes.empty())
    {
        bool found = false;
        for (auto type : sub.subscribedTypes)
        {
            if (type == message.type)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    return true;
}

GroupMessageQueue& BotMessageBus::GetOrCreateGroup(ObjectGuid groupGuid)
{
    auto it = _groups.find(groupGuid);
    if (it != _groups.end())
        return it->second;

    GroupMessageQueue& group = _groups[groupGuid];
    group.groupGuid = groupGuid;
    group.lastActivity = std::chrono::steady_clock::now();

    _stats.activeGroups++;

    TC_LOG_DEBUG("playerbot.messaging", "BotMessageBus: Created group queue for {}", groupGuid.ToString());

    return group;
}

uint32 BotMessageBus::GetGroupCount() const
{
    std::lock_guard lock(_mutex);
    return static_cast<uint32>(_groups.size());
}

uint32 BotMessageBus::GetSubscriberCount(ObjectGuid groupGuid) const
{
    std::lock_guard lock(_mutex);

    auto it = _groups.find(groupGuid);
    if (it == _groups.end())
        return 0;

    return static_cast<uint32>(it->second.subscribers.size());
}

uint32 BotMessageBus::GetQueueSize(ObjectGuid groupGuid) const
{
    std::lock_guard lock(_mutex);

    auto it = _groups.find(groupGuid);
    if (it == _groups.end())
        return 0;

    return static_cast<uint32>(it->second.messages.size());
}

uint32 BotMessageBus::CleanupInactiveGroups(uint32 inactiveThresholdSeconds)
{
    std::lock_guard lock(_mutex);

    auto now = std::chrono::steady_clock::now();
    auto threshold = std::chrono::seconds(inactiveThresholdSeconds);

    uint32 cleaned = 0;

    for (auto it = _groups.begin(); it != _groups.end(); )
    {
        if (it->second.subscribers.empty() &&
            now - it->second.lastActivity > threshold)
        {
            TC_LOG_DEBUG("playerbot.messaging", "BotMessageBus: Cleaning up inactive group {}",
                it->first.ToString());

            _stats.activeGroups--;
            it = _groups.erase(it);
            cleaned++;
        }
        else
        {
            ++it;
        }
    }

    return cleaned;
}

} // namespace Playerbot
