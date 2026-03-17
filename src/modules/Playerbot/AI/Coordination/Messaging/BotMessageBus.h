/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_BOT_MESSAGE_BUS_H
#define PLAYERBOT_BOT_MESSAGE_BUS_H

#include "BotMessage.h"
#include "ClaimResolver.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <queue>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

namespace Playerbot
{

class BotAI;

/**
 * @brief Message handler callback
 */
using MessageHandler = std::function<void(BotMessage const&)>;

/**
 * @brief Subscription info for a bot
 */
struct BotSubscription
{
    ObjectGuid botGuid;
    BotAI* botAI;
    std::vector<BotMessageType> subscribedTypes;
    uint8 role;  // 0=tank, 1=healer, 2=dps
    uint8 subGroup;  // Raid subgroup 1-8
};

/**
 * @brief Per-group message queue
 */
struct GroupMessageQueue
{
    ObjectGuid groupGuid;
    std::priority_queue<BotMessage> messages;
    std::unordered_map<ObjectGuid, BotSubscription> subscribers;
    std::chrono::steady_clock::time_point lastActivity;
};

/**
 * @brief Bot Message Bus - Group-local bot-to-bot communication
 *
 * This is the main messaging system for bot-to-bot coordination.
 * Messages are scoped to groups/raids and delivered based on scope:
 *
 * - GROUP_BROADCAST: All bots in the group
 * - ROLE_BROADCAST: All bots with a specific role (tank/healer/dps)
 * - SUBGROUP_BROADCAST: All bots in a raid subgroup
 * - DIRECT: Specific bot by GUID
 *
 * Claims are automatically routed through ClaimResolver for conflict resolution.
 *
 * Thread Safety: All public methods are thread-safe.
 *
 * Performance:
 * - O(1) message publish
 * - O(n) message delivery where n = subscribers
 * - O(log n) queue operations
 * - Batched processing per tick
 *
 * Usage:
 *   // Subscribe a bot
 *   BotMessageBus::instance()->Subscribe(botAI, groupGuid, {
 *       BotMessageType::CLAIM_INTERRUPT,
 *       BotMessageType::CMD_FOCUS_TARGET
 *   }, role, subGroup);
 *
 *   // Publish a message
 *   BotMessageBus::instance()->Publish(message);
 *
 *   // Process messages (called every tick)
 *   BotMessageBus::instance()->ProcessMessages(100);
 */
class BotMessageBus
{
public:
    static BotMessageBus* instance()
    {
        static BotMessageBus instance;
        return &instance;
    }

    /**
     * @brief Subscribe a bot to receive messages
     *
     * @param botAI The bot AI to subscribe
     * @param groupGuid The group/raid GUID
     * @param types Message types to subscribe to (empty = all types)
     * @param role Bot's role (0=tank, 1=healer, 2=dps)
     * @param subGroup Raid subgroup (1-8, 0 for 5-man groups)
     * @return true if subscription successful
     */
    bool Subscribe(BotAI* botAI, ObjectGuid groupGuid, std::vector<BotMessageType> const& types,
                   uint8 role = 2, uint8 subGroup = 0);

    /**
     * @brief Unsubscribe a bot from all messages
     *
     * @param botGuid The bot's GUID
     * @param groupGuid The group GUID (optional, empty = all groups)
     */
    void Unsubscribe(ObjectGuid botGuid, ObjectGuid groupGuid = ObjectGuid::Empty);

    /**
     * @brief Update a bot's subscription (role/subgroup change)
     *
     * @param botGuid The bot's GUID
     * @param groupGuid The group GUID
     * @param role New role
     * @param subGroup New subgroup
     */
    void UpdateSubscription(ObjectGuid botGuid, ObjectGuid groupGuid, uint8 role, uint8 subGroup);

    /**
     * @brief Publish a message to the bus
     *
     * For claim messages, this automatically routes through ClaimResolver.
     * The message is delivered based on its scope.
     *
     * @param message The message to publish
     * @return true if message was queued successfully
     */
    bool Publish(BotMessage const& message);

    /**
     * @brief Publish a claim message with callback
     *
     * @param message The claim message
     * @param callback Callback when claim is resolved
     * @return ClaimStatus::PENDING if accepted for resolution
     */
    ClaimStatus PublishClaim(BotMessage const& message, ClaimResolver::ClaimCallback callback = nullptr);

    /**
     * @brief Send a direct message to a specific bot
     *
     * @param message The message to send
     * @param recipientGuid The recipient bot's GUID
     * @return true if message was queued
     */
    bool SendDirect(BotMessage message, ObjectGuid recipientGuid);

    /**
     * @brief Process queued messages and deliver to subscribers
     *
     * @param maxMessages Maximum messages to process per group
     * @return Total messages processed
     */
    uint32 ProcessMessages(uint32 maxMessages = 100);

    /**
     * @brief Get the number of active groups
     */
    uint32 GetGroupCount() const;

    /**
     * @brief Get the number of subscribers in a group
     */
    uint32 GetSubscriberCount(ObjectGuid groupGuid) const;

    /**
     * @brief Get the queue size for a group
     */
    uint32 GetQueueSize(ObjectGuid groupGuid) const;

    /**
     * @brief Clean up inactive groups
     *
     * @param inactiveThresholdSeconds Groups inactive for this long are removed
     * @return Number of groups cleaned up
     */
    uint32 CleanupInactiveGroups(uint32 inactiveThresholdSeconds = 300);

    /**
     * @brief Statistics
     */
    struct Statistics
    {
        std::atomic<uint64> totalMessagesPublished{0};
        std::atomic<uint64> totalMessagesDelivered{0};
        std::atomic<uint64> totalMessagesDropped{0};
        std::atomic<uint64> totalClaimsSubmitted{0};
        std::atomic<uint32> activeGroups{0};
        std::atomic<uint32> activeSubscribers{0};
    };

    Statistics const& GetStatistics() const { return _stats; }

private:
    BotMessageBus();
    ~BotMessageBus() = default;

    // Disable copy/move
    BotMessageBus(BotMessageBus const&) = delete;
    BotMessageBus& operator=(BotMessageBus const&) = delete;

    GroupMessageQueue& GetOrCreateGroup(ObjectGuid groupGuid);
    void DeliverMessage(GroupMessageQueue& group, BotMessage const& message);
    bool ShouldDeliver(BotSubscription const& sub, BotMessage const& message) const;

    std::unordered_map<ObjectGuid, GroupMessageQueue> _groups;
    mutable std::mutex _mutex;
    uint32 _nextMessageId{1};
    uint32 _maxQueueSize{1000};

    Statistics _stats;
};

// Convenience macro
#define sBotMessageBus BotMessageBus::instance()

} // namespace Playerbot

#endif // PLAYERBOT_BOT_MESSAGE_BUS_H
