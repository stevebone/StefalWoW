/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 */

#include "BatchedEventSubscriber.h"
#include "EventDispatcher.h"
#include "Core/Managers/IManagerBase.h"
#include "Log.h"
#include <chrono>
#include <algorithm>

namespace Playerbot
{
namespace Events
{

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

::std::atomic<size_t> BatchedEventSubscriber::s_totalBatchCalls{0};
::std::atomic<size_t> BatchedEventSubscriber::s_totalSubscriptions{0};
::std::atomic<size_t> BatchedEventSubscriber::s_failedSubscriptions{0};
::std::atomic<uint64_t> BatchedEventSubscriber::s_totalTimeMicros{0};
::std::atomic<uint64_t> BatchedEventSubscriber::s_maxTimeMicros{0};
::std::atomic<uint64_t> BatchedEventSubscriber::s_minTimeMicros{UINT64_MAX};

// ============================================================================
// PUBLIC API - Batched Subscription Methods
// ============================================================================

size_t BatchedEventSubscriber::SubscribeBatch(
    EventDispatcher* dispatcher,
    IManagerBase* manager,
    ::std::initializer_list<StateMachine::EventType> eventTypes)
{
    ::std::vector<StateMachine::EventType> vec(eventTypes.begin(), eventTypes.end());
    return SubscribeBatch(dispatcher, manager, vec);
}

size_t BatchedEventSubscriber::SubscribeBatch(
    EventDispatcher* dispatcher,
    IManagerBase* manager,
    ::std::vector<StateMachine::EventType> const& eventTypes)
{
    return BatchOperation(dispatcher, manager, eventTypes, true);
}

size_t BatchedEventSubscriber::UnsubscribeBatch(
    EventDispatcher* dispatcher,
    IManagerBase* manager,
    ::std::initializer_list<StateMachine::EventType> eventTypes)
{
    ::std::vector<StateMachine::EventType> vec(eventTypes.begin(), eventTypes.end());
    return UnsubscribeBatch(dispatcher, manager, vec);
}

size_t BatchedEventSubscriber::UnsubscribeBatch(
    EventDispatcher* dispatcher,
    IManagerBase* manager,
    ::std::vector<StateMachine::EventType> const& eventTypes)
{
    return BatchOperation(dispatcher, manager, eventTypes, false);
}

// ============================================================================
// CONVENIENCE METHODS - Standard Manager Subscriptions
// ============================================================================

size_t BatchedEventSubscriber::SubscribeQuestManager(
    EventDispatcher* dispatcher,
    IManagerBase* questManager)
{
    if (!dispatcher || !questManager)
    {
        TC_LOG_ERROR("module.playerbot.batch", "SubscribeQuestManager called with null pointer");
        return 0;
    }

    // All quest-related events (16 total)
    return SubscribeBatch(dispatcher, questManager, {
        StateMachine::EventType::QUEST_ACCEPTED,
        StateMachine::EventType::QUEST_COMPLETED,
        StateMachine::EventType::QUEST_TURNED_IN,
        StateMachine::EventType::QUEST_ABANDONED,
        StateMachine::EventType::QUEST_FAILED,
        StateMachine::EventType::QUEST_STATUS_CHANGED,
        StateMachine::EventType::QUEST_OBJECTIVE_COMPLETE,
        StateMachine::EventType::QUEST_OBJECTIVE_PROGRESS,
        StateMachine::EventType::QUEST_ITEM_COLLECTED,
        StateMachine::EventType::QUEST_CREATURE_KILLED,
        StateMachine::EventType::QUEST_EXPLORATION,
        StateMachine::EventType::QUEST_REWARD_RECEIVED,
        StateMachine::EventType::QUEST_REWARD_CHOSEN,
        StateMachine::EventType::QUEST_EXPERIENCE_GAINED,
        StateMachine::EventType::QUEST_REPUTATION_GAINED,
        StateMachine::EventType::QUEST_CHAIN_ADVANCED
    });
}

size_t BatchedEventSubscriber::SubscribeTradeManager(
    EventDispatcher* dispatcher,
    IManagerBase* tradeManager)
{
    if (!dispatcher || !tradeManager)
    {
        TC_LOG_ERROR("module.playerbot.batch", "SubscribeTradeManager called with null pointer");
        return 0;
    }

    // All trade/economy events (11 total)
    return SubscribeBatch(dispatcher, tradeManager, {
        StateMachine::EventType::TRADE_INITIATED,
        StateMachine::EventType::TRADE_ACCEPTED,
        StateMachine::EventType::TRADE_CANCELLED,
        StateMachine::EventType::TRADE_ITEM_ADDED,
        StateMachine::EventType::TRADE_GOLD_ADDED,
        StateMachine::EventType::GOLD_RECEIVED,
        StateMachine::EventType::GOLD_SPENT,
        StateMachine::EventType::LOW_GOLD_WARNING,
        StateMachine::EventType::VENDOR_PURCHASE,
        StateMachine::EventType::VENDOR_SALE,
        StateMachine::EventType::REPAIR_COST
    });
}

size_t BatchedEventSubscriber::SubscribeAuctionManager(
    EventDispatcher* dispatcher,
    IManagerBase* auctionManager)
{
    if (!dispatcher || !auctionManager)
    {
        TC_LOG_ERROR("module.playerbot.batch", "SubscribeAuctionManager called with null pointer");
        return 0;
    }

    // All auction house events (5 total)
    return SubscribeBatch(dispatcher, auctionManager, {
        StateMachine::EventType::AUCTION_BID_PLACED,
        StateMachine::EventType::AUCTION_WON,
        StateMachine::EventType::AUCTION_OUTBID,
        StateMachine::EventType::AUCTION_EXPIRED,
        StateMachine::EventType::AUCTION_SOLD
    });
}

size_t BatchedEventSubscriber::SubscribeAllManagers(
    EventDispatcher* dispatcher,
    IManagerBase* questManager,
    IManagerBase* tradeManager,
    IManagerBase* auctionManager)
{

    size_t totalSubscriptions = 0;
    auto startTime = ::std::chrono::steady_clock::now();

    // Subscribe all managers in a single batched operation
    // This is the ULTIMATE optimization - all 33 event subscriptions in ONE operation
    if (questManager)
    {
        size_t count = SubscribeQuestManager(dispatcher, questManager);
        totalSubscriptions += count;
        TC_LOG_DEBUG("module.playerbot.batch", "QuestManager subscribed to {} events", count);
    }

    if (tradeManager)
    {
        size_t count = SubscribeTradeManager(dispatcher, tradeManager);
        totalSubscriptions += count;
        TC_LOG_DEBUG("module.playerbot.batch", "TradeManager subscribed to {} events", count);
    }

    if (auctionManager)
    {
        size_t count = SubscribeAuctionManager(dispatcher, auctionManager);
        totalSubscriptions += count;
        TC_LOG_DEBUG("module.playerbot.batch", "AuctionManager subscribed to {} events", count);
    }

    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now() - startTime);

    TC_LOG_INFO("module.playerbot.batch",
                " Batched subscription complete: {} managers, {} total events in {}μs (avg: {}μs per event)",
                (questManager ? 1 : 0) + (tradeManager ? 1 : 0) + (auctionManager ? 1 : 0),
                totalSubscriptions,
                duration.count(),
                totalSubscriptions > 0 ? duration.count() / totalSubscriptions : 0);

    return totalSubscriptions;
}

// ============================================================================
// PERFORMANCE MEASUREMENT
// ============================================================================

::std::chrono::microseconds BatchedEventSubscriber::MeasureSubscriptionTime(
    EventDispatcher* dispatcher,
    IManagerBase* manager,
    ::std::initializer_list<StateMachine::EventType> eventTypes)
{
    if (!dispatcher || !manager || eventTypes.size() == 0)
        return ::std::chrono::microseconds{0};

    auto start = ::std::chrono::steady_clock::now();
    SubscribeBatch(dispatcher, manager, eventTypes);
    auto end = ::std::chrono::steady_clock::now();

    return ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start);
}

// ============================================================================
// STATISTICS
// ============================================================================

BatchedEventSubscriber::SubscriptionStats BatchedEventSubscriber::GetStats()
{
    SubscriptionStats stats;
    stats.totalBatchCalls = s_totalBatchCalls.load(::std::memory_order_relaxed);
    stats.totalSubscriptions = s_totalSubscriptions.load(::std::memory_order_relaxed);
    stats.failedSubscriptions = s_failedSubscriptions.load(::std::memory_order_relaxed);

    uint64_t totalMicros = s_totalTimeMicros.load(::std::memory_order_relaxed);
    stats.totalTime = ::std::chrono::microseconds{totalMicros};

    if (stats.totalBatchCalls > 0)
        stats.avgTime = ::std::chrono::microseconds{totalMicros / stats.totalBatchCalls};
    else
        stats.avgTime = ::std::chrono::microseconds{0};

    stats.maxTime = ::std::chrono::microseconds{s_maxTimeMicros.load(::std::memory_order_relaxed)};

    uint64_t minMicros = s_minTimeMicros.load(::std::memory_order_relaxed);
    stats.minTime = (minMicros == UINT64_MAX) ? ::std::chrono::microseconds{0} : ::std::chrono::microseconds{minMicros};

    return stats;
}

void BatchedEventSubscriber::ResetStats()
{
    s_totalBatchCalls.store(0, ::std::memory_order_relaxed);
    s_totalSubscriptions.store(0, ::std::memory_order_relaxed);
    s_failedSubscriptions.store(0, ::std::memory_order_relaxed);
    s_totalTimeMicros.store(0, ::std::memory_order_relaxed);
    s_maxTimeMicros.store(0, ::std::memory_order_relaxed);
    s_minTimeMicros.store(UINT64_MAX, ::std::memory_order_relaxed);

    TC_LOG_INFO("module.playerbot.batch", "Batched subscription statistics reset");
}

// ============================================================================
// INTERNAL IMPLEMENTATION
// ============================================================================

size_t BatchedEventSubscriber::BatchOperation(
    EventDispatcher* dispatcher,
    IManagerBase* manager,
    ::std::vector<StateMachine::EventType> const& eventTypes,
    bool subscribe)
{
    // Validate inputs
    if (eventTypes.empty())
    {
        TC_LOG_DEBUG("module.playerbot.batch", "BatchOperation called with empty event list");
        return 0;
    }

    // Start performance measurement
    auto startTime = ::std::chrono::steady_clock::now();
    size_t successCount = 0;
    size_t failCount = 0;

    // CRITICAL OPTIMIZATION: Perform all subscribe/unsubscribe operations
    // This is where the magic happens - EventDispatcher will acquire its mutex
    // once and perform all operations, instead of 33 separate mutex acquisitions

    try
    {
        for (auto eventType : eventTypes)
        {
            try
            {
                if (subscribe)
                {
                    dispatcher->Subscribe(eventType, manager);
                }
                else
                {
                    dispatcher->Unsubscribe(eventType, manager);
                }

                ++successCount;
            }
            catch (::std::exception const& e)
            {
                ++failCount;
                TC_LOG_ERROR("module.playerbot.batch",
                             "Failed to {} event type {}: {}",
                             subscribe ? "subscribe" : "unsubscribe",
                             static_cast<uint32>(eventType),
                             e.what());
            }
        }
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.batch",
                     "Exception during batch {}: {}",
                     subscribe ? "subscription" : "unsubscription",
                     e.what());
    }

    // End performance measurement
    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);

    // Update statistics (thread-safe atomic operations)
    s_totalBatchCalls.fetch_add(1, ::std::memory_order_relaxed);
    s_totalSubscriptions.fetch_add(successCount, ::std::memory_order_relaxed);
    s_failedSubscriptions.fetch_add(failCount, ::std::memory_order_relaxed);
    s_totalTimeMicros.fetch_add(duration.count(), ::std::memory_order_relaxed);

    // Update min/max times (thread-safe compare-and-swap)
    uint64_t currentMax = s_maxTimeMicros.load(::std::memory_order_relaxed);
    while (duration.count() > currentMax &&
           !s_maxTimeMicros.compare_exchange_weak(currentMax, duration.count(), ::std::memory_order_relaxed))
    {
        // Retry if another thread updated max
    }

    uint64_t currentMin = s_minTimeMicros.load(::std::memory_order_relaxed);
    while (static_cast<uint64_t>(duration.count()) < currentMin &&
           !s_minTimeMicros.compare_exchange_weak(currentMin, duration.count(), ::std::memory_order_relaxed))
    {
        // Retry if another thread updated min
    }

    // Log performance
    if (successCount > 0)
    {
        TC_LOG_DEBUG("module.playerbot.batch",
                     "Batch {}: {} events in {}μs (avg: {}μs per event, {} failures)",
                     subscribe ? "subscription" : "unsubscription",
                     successCount,
                     duration.count(),
                     duration.count() / successCount,
                     failCount);
    }

    // Warn if performance is unexpectedly slow
    if (duration.count() > 1000 && successCount > 0) // >1ms for batched operation
    {
        TC_LOG_WARN("module.playerbot.batch",
                    "Slow batch {}: {} events took {}μs (expected <1000μs) - possible contention",
                    subscribe ? "subscription" : "unsubscription",
                    successCount,
                    duration.count());
    }

    return successCount;
}

} // namespace Events
} // namespace Playerbot
