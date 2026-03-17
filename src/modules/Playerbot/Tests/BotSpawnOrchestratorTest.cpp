/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Lifecycle/BotSpawnOrchestrator.h"
#include "Lifecycle/BotResourcePool.h"
#include "Lifecycle/BotPerformanceMonitor.h"
#include "Lifecycle/BotPopulationManager.h"
#include "Lifecycle/BotCharacterSelector.h"
#include "Session/BotSessionFactory.h"
#include "BotSpawner.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace Playerbot
{
namespace Test
{

// Mock implementations for testing
class MockBotResourcePool : public BotResourcePool
{
public:
    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(void, Shutdown, (), (override));
    MOCK_METHOD(void, Update, (uint32 diff), (override));
    MOCK_METHOD(bool, CanAllocateSession, (), (const, override));
    MOCK_METHOD(::std::shared_ptr<BotSession>, AcquireSession, (), (override));
    MOCK_METHOD(void, ReturnSession, (ObjectGuid sessionGuid), (override));
    MOCK_METHOD(uint32, GetAvailableSessionCount, (), (const, override));
    MOCK_METHOD(void, CleanupIdleSessions, (), (override));
};

class MockBotPerformanceMonitor : public BotPerformanceMonitor
{
public:
    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(void, Shutdown, (), (override));
    MOCK_METHOD(void, Update, (uint32 diff), (override));
    MOCK_METHOD(bool, IsPerformanceHealthy, (), (const, override));
    MOCK_METHOD(PerformanceSnapshot, GetSnapshot, (), (const, override));
};

class MockBotPopulationManager : public BotPopulationManager
{
public:
    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(void, Shutdown, (), (override));
    MOCK_METHOD(void, Update, (uint32 diff), (override));
    MOCK_METHOD(bool, CanSpawnInZone, (uint32 zoneId, uint32 maxBotsPerZone), (const, override));
    MOCK_METHOD(uint32, GetTotalBotCount, (), (const, override));
    MOCK_METHOD(uint32, GetBotCountInZone, (uint32 zoneId), (const, override));
    MOCK_METHOD(void, AddBotToZone, (uint32 zoneId, ObjectGuid botGuid), (override));
    MOCK_METHOD(::std::vector<uint32>, GetUnderpopulatedZones, (), (const, override));
};

class MockBotCharacterSelector : public BotCharacterSelector
{
public:
    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(void, Shutdown, (), (override));
    MOCK_METHOD(void, SelectCharacterAsync, (SpawnRequest const& request, CharacterCallback callback), (override));
};

class MockBotSessionFactory : public BotSessionFactory
{
public:
    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(void, Shutdown, (), (override));
    MOCK_METHOD(::std::shared_ptr<BotSession>, CreateBotSession, (ObjectGuid characterGuid, SpawnRequest const& request), (override));
};

/**
 * @class BotSpawnOrchestratorTest
 * @brief Unit tests for BotSpawnOrchestrator component
 *
 * Tests the orchestrator's ability to coordinate bot spawning across
 * all specialized components, ensuring proper error handling, performance,
 * and scalability for 5000 concurrent bots.
 */
class BotSpawnOrchestratorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create orchestrator instance
        orchestrator = ::std::make_unique<BotSpawnOrchestrator>();

        // Setup mock expectations for successful initialization
        EXPECT_CALL(*mockResourcePool, Initialize())
            .WillOnce(::testing::Return(true));
        EXPECT_CALL(*mockPerformanceMonitor, Initialize())
            .WillOnce(::testing::Return(true));
        EXPECT_CALL(*mockPopulationManager, Initialize())
            .WillOnce(::testing::Return(true));
        EXPECT_CALL(*mockCharacterSelector, Initialize())
            .WillOnce(::testing::Return(true));
        EXPECT_CALL(*mockSessionFactory, Initialize())
            .WillOnce(::testing::Return(true));
    }

    void TearDown() override
    {
        if (orchestrator)
        {
            EXPECT_CALL(*mockResourcePool, Shutdown()).Times(1);
            EXPECT_CALL(*mockPerformanceMonitor, Shutdown()).Times(1);
            EXPECT_CALL(*mockPopulationManager, Shutdown()).Times(1);
            EXPECT_CALL(*mockCharacterSelector, Shutdown()).Times(1);
            EXPECT_CALL(*mockSessionFactory, Shutdown()).Times(1);

            orchestrator->Shutdown();
        }
    }

    // Test fixtures
    ::std::unique_ptr<BotSpawnOrchestrator> orchestrator;

    // Mock components (would need dependency injection in real implementation)
    ::std::shared_ptr<MockBotResourcePool> mockResourcePool = ::std::make_shared<MockBotResourcePool>();
    ::std::shared_ptr<MockBotPerformanceMonitor> mockPerformanceMonitor = ::std::make_shared<MockBotPerformanceMonitor>();
    ::std::shared_ptr<MockBotPopulationManager> mockPopulationManager = ::std::make_shared<MockBotPopulationManager>();
    ::std::shared_ptr<MockBotCharacterSelector> mockCharacterSelector = ::std::make_shared<MockBotCharacterSelector>();
    ::std::shared_ptr<MockBotSessionFactory> mockSessionFactory = ::std::make_shared<MockBotSessionFactory>();

    // Helper methods
    SpawnRequest CreateTestSpawnRequest(uint32 zoneId = 1, uint32 mapId = 0) const
    {
        SpawnRequest request;
        request.zoneId = zoneId;
        request.mapId = mapId;
        request.minLevel = 1;
        request.maxLevel = 80;
        request.accountId = 0; // Auto-assign
        request.maxBotsPerZone = 50;
        return request;
    }
};

// === INITIALIZATION TESTS ===

TEST_F(BotSpawnOrchestratorTest, InitializeSuccessfully)
{
    // Test successful initialization of orchestrator and all components
    EXPECT_TRUE(orchestrator->Initialize());
}

TEST_F(BotSpawnOrchestratorTest, InitializeFailsWhenComponentFails)
{
    // Test initialization failure when a component fails to initialize
    EXPECT_CALL(*mockResourcePool, Initialize())
        .WillOnce(::testing::Return(false));  // Simulate failure

    EXPECT_FALSE(orchestrator->Initialize());
}

// === SPAWNING TESTS ===

TEST_F(BotSpawnOrchestratorTest, SpawnBotSuccessfully)
{
    // Setup expectations for successful bot spawning
    EXPECT_CALL(*mockResourcePool, CanAllocateSession())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mockPopulationManager, CanSpawnInZone(1, 50))
        .WillOnce(::testing::Return(true));

    orchestrator->Initialize();

    SpawnRequest request = CreateTestSpawnRequest();
    EXPECT_TRUE(orchestrator->SpawnBot(request));
}

TEST_F(BotSpawnOrchestratorTest, SpawnBotFailsWhenNoResourcesAvailable)
{
    // Test spawn failure when resource pool is full
    EXPECT_CALL(*mockResourcePool, CanAllocateSession())
        .WillOnce(::testing::Return(false));

    orchestrator->Initialize();

    SpawnRequest request = CreateTestSpawnRequest();
    EXPECT_FALSE(orchestrator->SpawnBot(request));
}

TEST_F(BotSpawnOrchestratorTest, SpawnBotFailsWhenZoneAtCapacity)
{
    // Test spawn failure when zone population is at capacity
    EXPECT_CALL(*mockResourcePool, CanAllocateSession())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mockPopulationManager, CanSpawnInZone(1, 50))
        .WillOnce(::testing::Return(false));

    orchestrator->Initialize();

    SpawnRequest request = CreateTestSpawnRequest();
    EXPECT_FALSE(orchestrator->SpawnBot(request));
}

// === BATCH SPAWNING TESTS ===

TEST_F(BotSpawnOrchestratorTest, SpawnMultipleBotsSuccessfully)
{
    // Test batch spawning of multiple bots
    ::std::vector<SpawnRequest> requests;
    for (int i = 0; i < 5; ++i)
    {
        requests.push_back(CreateTestSpawnRequest(i + 1));
    }

    // Expect character selector to handle batch processing
    EXPECT_CALL(*mockCharacterSelector, ProcessBatchSelection(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](::std::vector<SpawnRequest> const& reqs,
                                     ::std::function<void(::std::vector<ObjectGuid>)> callback)
                                     {
            // Simulate successful character selection for all requests
            ::std::vector<ObjectGuid> characters;
            for (size_t i = 0; i < reqs.size(); ++i)
            {
                characters.push_back(ObjectGuid::Create<HighGuid::Player>(i + 1));
            }
            callback(characters);
        }));

    orchestrator->Initialize();

    uint32 successfulSpawns = orchestrator->SpawnBots(requests);
    EXPECT_EQ(successfulSpawns, 5);
}

// === POPULATION MANAGEMENT TESTS ===

TEST_F(BotSpawnOrchestratorTest, SpawnToPopulationTargetCreatesNeededBots)
{
    // Test that spawn-to-target creates bots for underpopulated zones
    ::std::vector<uint32> underpopulatedZones = {1, 2, 3};
    EXPECT_CALL(*mockPopulationManager, GetUnderpopulatedZones())
        .WillOnce(::testing::Return(underpopulatedZones));

    // Setup zone population data
    for (uint32 zoneId : underpopulatedZones)
    {
        ZonePopulation zonePopulation;
        zonePopulation.zoneId = zoneId;
        zonePopulation.mapId = 0;
        zonePopulation.botCount = 10;
        zonePopulation.targetBotCount = 20;  // Need 10 more bots
        zonePopulation.minLevel = 1;
        zonePopulation.maxLevel = 80;

        EXPECT_CALL(*mockPopulationManager, GetZonePopulation(zoneId))
            .WillRepeatedly(::testing::Return(zonePopulation));
    }

    // Expect spawn attempts for needed bots (limited to 10 per cycle)
    EXPECT_CALL(*mockResourcePool, CanAllocateSession())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockPopulationManager, CanSpawnInZone(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return(true));

    orchestrator->Initialize();
    orchestrator->SpawnToPopulationTarget();
}

// === QUERY TESTS ===

TEST_F(BotSpawnOrchestratorTest, GetActiveBotCountReturnsCorrectValue)
{
    // Test active bot count query
    EXPECT_CALL(*mockPopulationManager, GetTotalBotCount())
        .WillOnce(::testing::Return(150));

    orchestrator->Initialize();

    EXPECT_EQ(orchestrator->GetActiveBotCount(), 150);
}

TEST_F(BotSpawnOrchestratorTest, GetActiveBotCountForZoneReturnsCorrectValue)
{
    // Test zone-specific bot count query
    EXPECT_CALL(*mockPopulationManager, GetBotCountInZone(1))
        .WillOnce(::testing::Return(25));

    orchestrator->Initialize();

    EXPECT_EQ(orchestrator->GetActiveBotCount(1), 25);
}

TEST_F(BotSpawnOrchestratorTest, CanSpawnMoreReturnsTrueWhenResourcesAvailable)
{
    // Test spawn capacity check
    EXPECT_CALL(*mockResourcePool, CanAllocateSession())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mockPerformanceMonitor, IsPerformanceHealthy())
        .WillOnce(::testing::Return(true));

    orchestrator->Initialize();

    EXPECT_TRUE(orchestrator->CanSpawnMore());
}

TEST_F(BotSpawnOrchestratorTest, CanSpawnMoreReturnsFalseWhenPerformanceDegraded)
{
    // Test spawn capacity check with degraded performance
    EXPECT_CALL(*mockResourcePool, CanAllocateSession())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mockPerformanceMonitor, IsPerformanceHealthy())
        .WillOnce(::testing::Return(false));

    orchestrator->Initialize();

    EXPECT_FALSE(orchestrator->CanSpawnMore());
}

// === UPDATE CYCLE TESTS ===

TEST_F(BotSpawnOrchestratorTest, UpdateCallsAllComponentUpdates)
{
    // Test that update cycle calls all component updates
    EXPECT_CALL(*mockResourcePool, Update(1000)).Times(1);
    EXPECT_CALL(*mockPerformanceMonitor, Update(1000)).Times(1);
    EXPECT_CALL(*mockPopulationManager, Update(1000)).Times(1);

    orchestrator->Initialize();
    orchestrator->Update(1000);  // 1 second update
}

// === ERROR HANDLING TESTS ===

TEST_F(BotSpawnOrchestratorTest, HandlesComponentShutdownGracefully)
{
    // Test graceful shutdown even when components are in various states
    orchestrator->Initialize();

    // Shutdown should succeed even if orchestrator is in any state
    EXPECT_NO_THROW(orchestrator->Shutdown());
}

// === PERFORMANCE TESTS ===

TEST_F(BotSpawnOrchestratorTest, SpawnLatencyWithinAcceptableRange)
{
    // Test that spawn operations complete within acceptable time limits
    orchestrator->Initialize();

    auto start = ::std::chrono::high_resolution_clock::now();

    // Setup successful spawn path
    EXPECT_CALL(*mockResourcePool, CanAllocateSession())
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mockPopulationManager, CanSpawnInZone(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(true));

    SpawnRequest request = CreateTestSpawnRequest();
    orchestrator->SpawnBot(request);

    auto end = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start);

    // Spawn should complete in under 1ms for scalability
    EXPECT_LT(duration.count(), 1000);  // 1000 microseconds = 1ms
}

} // namespace Test
} // namespace Playerbot

// === TEST MAIN ===
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}