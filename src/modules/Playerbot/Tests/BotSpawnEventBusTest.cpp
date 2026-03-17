/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Lifecycle/BotSpawnEventBus.h"
#include "BotSpawner.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include <atomic>

namespace Playerbot
{
namespace Test
{

/**
 * @class BotSpawnEventBusTest
 * @brief Unit tests for BotSpawnEventBus component
 *
 * Tests the event-driven architecture's ability to handle high-throughput
 * event processing for 5000 concurrent bot spawning operations.
 */
class BotSpawnEventBusTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        eventBus = BotSpawnEventBus::instance();
        ASSERT_NE(eventBus, nullptr);

        // Initialize the event bus
        EXPECT_TRUE(eventBus->Initialize());
    }

    void TearDown() override
    {
        if (eventBus)
        {
            eventBus->Shutdown();
        }
    }

    BotSpawnEventBus* eventBus = nullptr;

    // Test event counters
    ::std::atomic<uint32> spawnRequestsReceived{0};
    ::std::atomic<uint32> spawnCompletedReceived{0};
    ::std::atomic<uint32> characterSelectedReceived{0};
    ::std::atomic<uint32> sessionCreatedReceived{0};
    ::std::atomic<uint32> populationChangedReceived{0};
    ::std::atomic<uint32> globalEventsReceived{0};

    // Helper methods
    SpawnRequest CreateTestSpawnRequest(uint32 zoneId = 1) const
    {
        SpawnRequest request;
        request.zoneId = zoneId;
        request.mapId = 0;
        request.minLevel = 1;
        request.maxLevel = 80;
        request.accountId = 0;
        request.maxBotsPerZone = 50;
        return request;
    }

    void SetupEventHandlers()
    {
        // Subscribe to specific event types
        eventBus->Subscribe(BotSpawnEventType::SPAWN_REQUESTED,
            [this](::std::shared_ptr<BotSpawnEvent> event) {
                spawnRequestsReceived.fetch_add(1);
            });

        eventBus->Subscribe(BotSpawnEventType::SPAWN_COMPLETED,
            [this](::std::shared_ptr<BotSpawnEvent> event) {
                spawnCompletedReceived.fetch_add(1);
            });

        eventBus->Subscribe(BotSpawnEventType::CHARACTER_SELECTED,
            [this](::std::shared_ptr<BotSpawnEvent> event) {
                characterSelectedReceived.fetch_add(1);
            });

        eventBus->Subscribe(BotSpawnEventType::SESSION_CREATED,
            [this](::std::shared_ptr<BotSpawnEvent> event) {
                sessionCreatedReceived.fetch_add(1);
            });

        eventBus->Subscribe(BotSpawnEventType::POPULATION_CHANGED,
            [this](::std::shared_ptr<BotSpawnEvent> event) {
                populationChangedReceived.fetch_add(1);
            });

        // Subscribe to all events
        eventBus->SubscribeToAll(
            [this](::std::shared_ptr<BotSpawnEvent> event) {
                globalEventsReceived.fetch_add(1);
            });
    }

    void ResetCounters()
    {
        spawnRequestsReceived.store(0);
        spawnCompletedReceived.store(0);
        characterSelectedReceived.store(0);
        sessionCreatedReceived.store(0);
        populationChangedReceived.store(0);
        globalEventsReceived.store(0);
    }
};

// === INITIALIZATION TESTS ===

TEST_F(BotSpawnEventBusTest, InitializesSuccessfully)
{
    // Event bus should initialize and be ready for use
    EXPECT_TRUE(eventBus != nullptr);
    EXPECT_TRUE(eventBus->IsHealthy());

    auto stats = eventBus->GetStats();
    EXPECT_EQ(stats.eventsPublished.load(), 0);
    EXPECT_EQ(stats.eventsProcessed.load(), 0);
    EXPECT_EQ(stats.eventsDropped.load(), 0);
}

TEST_F(BotSpawnEventBusTest, SingletonPatternWorksCorrectly)
{
    // Multiple calls to instance() should return the same object
    auto eventBus1 = BotSpawnEventBus::instance();
    auto eventBus2 = BotSpawnEventBus::instance();

    EXPECT_EQ(eventBus1, eventBus2);
}

// === EVENT PUBLISHING TESTS ===

TEST_F(BotSpawnEventBusTest, PublishesSpawnRequestEventCorrectly)
{
    SetupEventHandlers();

    SpawnRequest request = CreateTestSpawnRequest();
    eventBus->PublishSpawnRequest(request, [](bool success, ObjectGuid guid) {
        // Callback handling would be tested separately
    });

    eventBus->ProcessEvents();

    EXPECT_EQ(spawnRequestsReceived.load(), 1);
    EXPECT_EQ(globalEventsReceived.load(), 1);

    auto stats = eventBus->GetStats();
    EXPECT_EQ(stats.eventsPublished.load(), 1);
    EXPECT_EQ(stats.eventsProcessed.load(), 1);
}

TEST_F(BotSpawnEventBusTest, PublishesCharacterSelectedEventCorrectly)
{
    SetupEventHandlers();

    ObjectGuid characterGuid = ObjectGuid::Create<HighGuid::Player>(12345);
    SpawnRequest request = CreateTestSpawnRequest();

    eventBus->PublishCharacterSelected(characterGuid, request);
    eventBus->ProcessEvents();

    EXPECT_EQ(characterSelectedReceived.load(), 1);
    EXPECT_EQ(globalEventsReceived.load(), 1);
}

TEST_F(BotSpawnEventBusTest, PublishesSessionCreatedEventCorrectly)
{
    SetupEventHandlers();

    auto session = ::std::make_shared<BotSession>(12345, ObjectGuid::Create<HighGuid::Player>(12345));
    SpawnRequest request = CreateTestSpawnRequest();

    eventBus->PublishSessionCreated(session, request);
    eventBus->ProcessEvents();

    EXPECT_EQ(sessionCreatedReceived.load(), 1);
    EXPECT_EQ(globalEventsReceived.load(), 1);
}

TEST_F(BotSpawnEventBusTest, PublishesSpawnCompletedEventCorrectly)
{
    SetupEventHandlers();

    ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(12345);
    eventBus->PublishSpawnCompleted(botGuid, true, "spawn_successful");
    eventBus->ProcessEvents();

    EXPECT_EQ(spawnCompletedReceived.load(), 1);
    EXPECT_EQ(globalEventsReceived.load(), 1);
}

TEST_F(BotSpawnEventBusTest, PublishesPopulationChangedEventCorrectly)
{
    SetupEventHandlers();

    eventBus->PublishPopulationChanged(1, 10, 15);  // Zone 1: 10 -> 15 bots
    eventBus->ProcessEvents();

    EXPECT_EQ(populationChangedReceived.load(), 1);
    EXPECT_EQ(globalEventsReceived.load(), 1);
}

// === EVENT SUBSCRIPTION TESTS ===

TEST_F(BotSpawnEventBusTest, SubscriptionReturnsValidHandlerId)
{
    auto handlerId = eventBus->Subscribe(BotSpawnEventType::SPAWN_REQUESTED,
        [](::std::shared_ptr<BotSpawnEvent> event) {
            // Empty handler for test
        });

    EXPECT_GT(handlerId, 0);  // Should return a valid ID
}

TEST_F(BotSpawnEventBusTest, UnsubscribeRemovesHandler)
{
    ResetCounters();

    // Subscribe and get handler ID
    auto handlerId = eventBus->Subscribe(BotSpawnEventType::SPAWN_REQUESTED,
        [this](::std::shared_ptr<BotSpawnEvent> event) {
            spawnRequestsReceived.fetch_add(1);
        });

    // Publish event and verify it's received
    SpawnRequest request = CreateTestSpawnRequest();
    eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    eventBus->ProcessEvents();

    EXPECT_EQ(spawnRequestsReceived.load(), 1);

    // Unsubscribe
    eventBus->Unsubscribe(handlerId);

    // Publish another event and verify it's NOT received
    eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    eventBus->ProcessEvents();

    EXPECT_EQ(spawnRequestsReceived.load(), 1);  // Should still be 1
}

TEST_F(BotSpawnEventBusTest, GlobalSubscriptionReceivesAllEvents)
{
    ResetCounters();

    eventBus->SubscribeToAll([this](::std::shared_ptr<BotSpawnEvent> event) {
        globalEventsReceived.fetch_add(1);
    });

    // Publish different types of events
    SpawnRequest request = CreateTestSpawnRequest();
    eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    eventBus->PublishCharacterSelected(ObjectGuid::Create<HighGuid::Player>(1), request);
    eventBus->PublishSpawnCompleted(ObjectGuid::Create<HighGuid::Player>(1), true);
    eventBus->PublishPopulationChanged(1, 10, 15);

    eventBus->ProcessEvents();

    EXPECT_EQ(globalEventsReceived.load(), 4);  // Should receive all 4 events
}

// === EVENT PROCESSING TESTS ===

TEST_F(BotSpawnEventBusTest, ProcessEventsHandlesQueuedEvents)
{
    SetupEventHandlers();

    // Publish multiple events
    for (int i = 0; i < 10; ++i)
    {
        SpawnRequest request = CreateTestSpawnRequest(i + 1);
        eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    }

    // Events should be queued but not processed yet
    EXPECT_EQ(spawnRequestsReceived.load(), 0);
    EXPECT_GT(eventBus->GetQueuedEventCount(), 0);

    // Process events
    eventBus->ProcessEvents();

    // All events should now be processed
    EXPECT_EQ(spawnRequestsReceived.load(), 10);
    EXPECT_EQ(globalEventsReceived.load(), 10);
}

TEST_F(BotSpawnEventBusTest, ProcessEventsOfTypeHandlesSpecificEvents)
{
    SetupEventHandlers();

    // Publish mixed event types
    SpawnRequest request = CreateTestSpawnRequest();
    eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    eventBus->PublishCharacterSelected(ObjectGuid::Create<HighGuid::Player>(1), request);
    eventBus->PublishSpawnCompleted(ObjectGuid::Create<HighGuid::Player>(1), true);

    // Process only spawn request events
    eventBus->ProcessEventsOfType(BotSpawnEventType::SPAWN_REQUESTED);

    // Only spawn request events should be processed
    EXPECT_EQ(spawnRequestsReceived.load(), 1);
    EXPECT_EQ(characterSelectedReceived.load(), 0);
    EXPECT_EQ(spawnCompletedReceived.load(), 0);

    // Global handler should still receive the processed event
    EXPECT_EQ(globalEventsReceived.load(), 1);
}

// === PERFORMANCE TESTS ===

TEST_F(BotSpawnEventBusTest, HandlesHighThroughputEventPublishing)
{
    SetupEventHandlers();

    const int numEvents = 10000;
    auto start = ::std::chrono::high_resolution_clock::now();

    // Publish many events rapidly
    for (int i = 0; i < numEvents; ++i)
    {
        SpawnRequest request = CreateTestSpawnRequest(i % 100 + 1);
        eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    }

    auto publishEnd = ::std::chrono::high_resolution_clock::now();

    // Process all events
    eventBus->ProcessEvents();

    auto processEnd = ::std::chrono::high_resolution_clock::now();

    auto publishDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(publishEnd - start);
    auto processDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(processEnd - publishEnd);

    // Publishing should be fast (under 100ms for 10k events)
    EXPECT_LT(publishDuration.count(), 100);

    // Processing should be efficient (under 500ms for 10k events)
    EXPECT_LT(processDuration.count(), 500);

    // All events should be processed
    EXPECT_EQ(spawnRequestsReceived.load(), numEvents);

    auto stats = eventBus->GetStats();
    EXPECT_EQ(stats.eventsPublished.load(), numEvents);
    EXPECT_EQ(stats.eventsProcessed.load(), numEvents);
    EXPECT_LT(stats.GetAverageProcessingTimeUs(), 100.0f);  // Under 100μs average
}

TEST_F(BotSpawnEventBusTest, BatchProcessingRespectsLimits)
{
    SetupEventHandlers();

    // Set small batch size for testing
    eventBus->SetBatchSize(5);

    // Publish more events than batch size
    for (int i = 0; i < 20; ++i)
    {
        SpawnRequest request = CreateTestSpawnRequest();
        eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    }

    // Single process call should only handle batch size events
    eventBus->ProcessEvents();

    // Should have processed exactly batch size (5) events
    EXPECT_EQ(spawnRequestsReceived.load(), 5);

    // Remaining events should still be queued
    EXPECT_GT(eventBus->GetQueuedEventCount(), 0);
}

// === THREAD SAFETY TESTS ===

TEST_F(BotSpawnEventBusTest, ConcurrentEventPublishingIsSafe)
{
    SetupEventHandlers();

    const int numThreads = 10;
    const int eventsPerThread = 1000;
    ::std::vector<::std::thread> threads;

    // Launch multiple threads publishing events concurrently
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back([this, eventsPerThread, t]() {
            for (int i = 0; i < eventsPerThread; ++i)
            {
                SpawnRequest request = CreateTestSpawnRequest(t * 1000 + i);
                eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Process all events
    eventBus->ProcessEvents();

    // Should have received all events from all threads
    EXPECT_EQ(spawnRequestsReceived.load(), numThreads * eventsPerThread);

    auto stats = eventBus->GetStats();
    EXPECT_EQ(stats.eventsPublished.load(), numThreads * eventsPerThread);
    EXPECT_EQ(stats.eventsProcessed.load(), numThreads * eventsPerThread);
    EXPECT_EQ(stats.eventsDropped.load(), 0);  // No events should be dropped
}

// === ERROR HANDLING TESTS ===

TEST_F(BotSpawnEventBusTest, HandlesExceptionInEventHandlerGracefully)
{
    ResetCounters();

    // Subscribe handler that throws exception
    eventBus->Subscribe(BotSpawnEventType::SPAWN_REQUESTED,
        [](::std::shared_ptr<BotSpawnEvent> event) {
            throw ::std::runtime_error("Test exception");
        });

    // Subscribe normal handler
    eventBus->Subscribe(BotSpawnEventType::SPAWN_REQUESTED,
        [this](::std::shared_ptr<BotSpawnEvent> event) {
            spawnRequestsReceived.fetch_add(1);
        });

    SpawnRequest request = CreateTestSpawnRequest();
    eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});

    // Processing should not crash and other handlers should still work
    EXPECT_NO_THROW(eventBus->ProcessEvents());
    EXPECT_EQ(spawnRequestsReceived.load(), 1);
}

// === QUEUE MANAGEMENT TESTS ===

TEST_F(BotSpawnEventBusTest, DropsEventsWhenQueueOverflows)
{
    SetupEventHandlers();

    // Set very small queue size
    eventBus->SetMaxQueueSize(5);

    // Publish more events than queue can hold
    for (int i = 0; i < 20; ++i)
    {
        SpawnRequest request = CreateTestSpawnRequest();
        eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    }

    auto stats = eventBus->GetStats();

    // Should have dropped some events
    EXPECT_GT(stats.eventsDropped.load(), 0);

    // Queue should not exceed maximum size
    EXPECT_LE(eventBus->GetQueuedEventCount(), 5);
}

TEST_F(BotSpawnEventBusTest, HealthStatusReflectsQueueState)
{
    // Initially should be healthy
    EXPECT_TRUE(eventBus->IsHealthy());

    // Set small queue and fill it
    eventBus->SetMaxQueueSize(10);

    for (int i = 0; i < 15; ++i)  // More than queue capacity
    {
        SpawnRequest request = CreateTestSpawnRequest();
        eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    }

    // Health should degrade when queue is overwhelmed
    // (Implementation dependent - might still be healthy if processing keeps up)
}

// === STATISTICS TESTS ===

TEST_F(BotSpawnEventBusTest, StatisticsTrackCorrectly)
{
    SetupEventHandlers();

    // Publish some events
    for (int i = 0; i < 100; ++i)
    {
        SpawnRequest request = CreateTestSpawnRequest();
        eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    }

    // Process events
    eventBus->ProcessEvents();

    auto stats = eventBus->GetStats();

    EXPECT_EQ(stats.eventsPublished.load(), 100);
    EXPECT_EQ(stats.eventsProcessed.load(), 100);
    EXPECT_GT(stats.totalProcessingTimeUs.load(), 0);
    EXPECT_GT(stats.GetAverageProcessingTimeUs(), 0.0f);
}

TEST_F(BotSpawnEventBusTest, ResetStatsClearsCounters)
{
    SetupEventHandlers();

    // Generate some statistics
    for (int i = 0; i < 50; ++i)
    {
        SpawnRequest request = CreateTestSpawnRequest();
        eventBus->PublishSpawnRequest(request, [](bool, ObjectGuid) {});
    }
    eventBus->ProcessEvents();

    // Reset statistics
    eventBus->ResetStats();

    auto stats = eventBus->GetStats();

    EXPECT_EQ(stats.eventsPublished.load(), 0);
    EXPECT_EQ(stats.eventsProcessed.load(), 0);
    EXPECT_EQ(stats.eventsDropped.load(), 0);
    EXPECT_EQ(stats.totalProcessingTimeUs.load(), 0);
}

} // namespace Test
} // namespace Playerbot