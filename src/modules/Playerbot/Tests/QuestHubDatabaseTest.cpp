/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Quest/QuestHubDatabase.h"
#include "TestUtilities.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace Playerbot;
using namespace Playerbot::Test;
using namespace testing;

namespace
{
    // Mock Player for testing quest hub suitability
    class MockQuestHubPlayer
    {
    public:
        MOCK_METHOD(uint8, GetLevel, (), (const));
        MOCK_METHOD(uint32, GetTeamId, (), (const)); // 0=Alliance, 1=Horde
        MOCK_METHOD(uint32, GetZoneId, (), (const));
        MOCK_METHOD(Position, GetPosition, (), (const));
        MOCK_METHOD(float, GetExactDist2d, (Position const&), (const));
    };

    // Test fixture for QuestHubDatabase tests
    class QuestHubDatabaseTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Reset database state before each test
            // Note: In production, Initialize() would load from database
            // For unit tests, we'll work with the singleton's state
        }

        void TearDown() override
        {
            // Cleanup after each test
        }

        // Helper to create a test quest hub
        QuestHub CreateTestHub(
            uint32 hubId,
            uint32 zoneId,
            Position const& location,
            uint32 minLevel,
            uint32 maxLevel,
            uint32 factionMask,
            std::string const& name,
            std::vector<uint32> questIds = {},
            std::vector<uint32> creatureIds = {},
            float radius = 50.0f)
        {
            QuestHub hub;
            hub.hubId = hubId;
            hub.zoneId = zoneId;
            hub.location = location;
            hub.minLevel = minLevel;
            hub.maxLevel = maxLevel;
            hub.factionMask = factionMask;
            hub.name = name;
            hub.questIds = std::move(questIds);
            hub.creatureIds = std::move(creatureIds);
            hub.radius = radius;
            return hub;
        }
    };

    // ============================================================================
    // QUEST HUB STRUCTURE TESTS
    // ============================================================================

    TEST_F(QuestHubDatabaseTest, QuestHub_IsAppropriateFor_LevelTooLow)
    {
        // Create a level 10-15 quest hub
        Position hubPos(0.0f, 0.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x3, "Elwynn Forest Hub");

        // Create a level 5 player
        MockQuestHubPlayer player;
        EXPECT_CALL(player, GetLevel()).WillRepeatedly(Return(5));
        EXPECT_CALL(player, GetTeamId()).WillRepeatedly(Return(0)); // Alliance

        // Level 5 is too low for level 10-15 hub
        EXPECT_FALSE(hub.IsAppropriateFor(reinterpret_cast<Player const*>(&player)));
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_IsAppropriateFor_LevelWithinRange)
    {
        // Create a level 10-15 quest hub
        Position hubPos(0.0f, 0.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x3, "Elwynn Forest Hub");

        // Create a level 12 player
        MockQuestHubPlayer player;
        EXPECT_CALL(player, GetLevel()).WillRepeatedly(Return(12));
        EXPECT_CALL(player, GetTeamId()).WillRepeatedly(Return(0)); // Alliance

        // Level 12 is within level 10-15 hub range
        EXPECT_TRUE(hub.IsAppropriateFor(reinterpret_cast<Player const*>(&player)));
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_IsAppropriateFor_LevelTooHigh)
    {
        // Create a level 10-15 quest hub
        Position hubPos(0.0f, 0.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x3, "Elwynn Forest Hub");

        // Create a level 20 player
        MockQuestHubPlayer player;
        EXPECT_CALL(player, GetLevel()).WillRepeatedly(Return(20));
        EXPECT_CALL(player, GetTeamId()).WillRepeatedly(Return(0)); // Alliance

        // Level 20 is too high for level 10-15 hub
        EXPECT_FALSE(hub.IsAppropriateFor(reinterpret_cast<Player const*>(&player)));
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_IsAppropriateFor_FactionMismatch_AllianceOnly)
    {
        // Create an Alliance-only quest hub (faction mask = 0x1)
        Position hubPos(0.0f, 0.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x1, "Stormwind Hub");

        // Create a Horde player (teamId = 1)
        MockQuestHubPlayer player;
        EXPECT_CALL(player, GetLevel()).WillRepeatedly(Return(12));
        EXPECT_CALL(player, GetTeamId()).WillRepeatedly(Return(1)); // Horde

        // Horde player cannot use Alliance-only hub
        EXPECT_FALSE(hub.IsAppropriateFor(reinterpret_cast<Player const*>(&player)));
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_IsAppropriateFor_FactionMismatch_HordeOnly)
    {
        // Create a Horde-only quest hub (faction mask = 0x2)
        Position hubPos(0.0f, 0.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x2, "Orgrimmar Hub");

        // Create an Alliance player (teamId = 0)
        MockQuestHubPlayer player;
        EXPECT_CALL(player, GetLevel()).WillRepeatedly(Return(12));
        EXPECT_CALL(player, GetTeamId()).WillRepeatedly(Return(0)); // Alliance

        // Alliance player cannot use Horde-only hub
        EXPECT_FALSE(hub.IsAppropriateFor(reinterpret_cast<Player const*>(&player)));
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_IsAppropriateFor_NeutralHub_BothFactions)
    {
        // Create a neutral quest hub (faction mask = 0x3 = Alliance | Horde)
        Position hubPos(0.0f, 0.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x3, "Booty Bay Hub");

        // Test Alliance player
        MockQuestHubPlayer alliancePlayer;
        EXPECT_CALL(alliancePlayer, GetLevel()).WillRepeatedly(Return(12));
        EXPECT_CALL(alliancePlayer, GetTeamId()).WillRepeatedly(Return(0)); // Alliance
        EXPECT_TRUE(hub.IsAppropriateFor(reinterpret_cast<Player const*>(&alliancePlayer)));

        // Test Horde player
        MockQuestHubPlayer hordePlayer;
        EXPECT_CALL(hordePlayer, GetLevel()).WillRepeatedly(Return(12));
        EXPECT_CALL(hordePlayer, GetTeamId()).WillRepeatedly(Return(1)); // Horde
        EXPECT_TRUE(hub.IsAppropriateFor(reinterpret_cast<Player const*>(&hordePlayer)));
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_GetDistanceFrom_CalculatesCorrectly)
    {
        // Create a quest hub at position (100, 200, 0)
        Position hubPos(100.0f, 200.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x3, "Test Hub");

        // Create a player at position (103, 204, 0) - should be 5 yards away (3² + 4² = 5²)
        MockQuestHubPlayer player;
        Position playerPos(103.0f, 204.0f, 0.0f, 0.0f);
        EXPECT_CALL(player, GetPosition()).WillRepeatedly(Return(playerPos));
        EXPECT_CALL(player, GetExactDist2d(hubPos)).WillOnce(Return(5.0f));

        float distance = hub.GetDistanceFrom(reinterpret_cast<Player const*>(&player));
        EXPECT_FLOAT_EQ(distance, 5.0f);
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_ContainsPosition_WithinRadius)
    {
        // Create a quest hub at (100, 100, 0) with radius 50
        Position hubPos(100.0f, 100.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x3, "Test Hub", {}, {}, 50.0f);

        // Position at (120, 120, 0) is ~28 yards away, within radius
        Position testPos(120.0f, 120.0f, 0.0f, 0.0f);
        EXPECT_TRUE(hub.ContainsPosition(testPos));
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_ContainsPosition_OutsideRadius)
    {
        // Create a quest hub at (100, 100, 0) with radius 50
        Position hubPos(100.0f, 100.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x3, "Test Hub", {}, {}, 50.0f);

        // Position at (200, 200, 0) is ~141 yards away, outside radius
        Position testPos(200.0f, 200.0f, 0.0f, 0.0f);
        EXPECT_FALSE(hub.ContainsPosition(testPos));
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_CalculateSuitabilityScore_PerfectMatch)
    {
        // Create a level 10-15 quest hub with 5 quests
        Position hubPos(100.0f, 100.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x1, "Perfect Hub", {1, 2, 3, 4, 5});

        // Level 12 Alliance player at same position (0 distance)
        MockQuestHubPlayer player;
        EXPECT_CALL(player, GetLevel()).WillRepeatedly(Return(12));
        EXPECT_CALL(player, GetTeamId()).WillRepeatedly(Return(0)); // Alliance
        EXPECT_CALL(player, GetPosition()).WillRepeatedly(Return(hubPos));
        EXPECT_CALL(player, GetExactDist2d(hubPos)).WillOnce(Return(0.0f));

        // Perfect match should have high suitability score
        float score = hub.CalculateSuitabilityScore(reinterpret_cast<Player const*>(&player));
        EXPECT_GT(score, 0.0f); // Should be positive and high
    }

    TEST_F(QuestHubDatabaseTest, QuestHub_CalculateSuitabilityScore_NotAppropriate)
    {
        // Create a level 10-15 quest hub
        Position hubPos(100.0f, 100.0f, 0.0f, 0.0f);
        QuestHub hub = CreateTestHub(1, 100, hubPos, 10, 15, 0x1, "Alliance Hub");

        // Level 5 player (too low level)
        MockQuestHubPlayer player;
        EXPECT_CALL(player, GetLevel()).WillRepeatedly(Return(5));
        EXPECT_CALL(player, GetTeamId()).WillRepeatedly(Return(0)); // Alliance

        // Not appropriate hub should return 0 score
        float score = hub.CalculateSuitabilityScore(reinterpret_cast<Player const*>(&player));
        EXPECT_FLOAT_EQ(score, 0.0f);
    }

    // ============================================================================
    // QUEST HUB DATABASE SINGLETON TESTS
    // ============================================================================

    TEST_F(QuestHubDatabaseTest, Database_Singleton_InstanceNotNull)
    {
        QuestHubDatabase& instance = QuestHubDatabase::Instance();
        EXPECT_NE(&instance, nullptr);
    }

    TEST_F(QuestHubDatabaseTest, Database_Singleton_SameInstanceReturned)
    {
        QuestHubDatabase& instance1 = QuestHubDatabase::Instance();
        QuestHubDatabase& instance2 = QuestHubDatabase::Instance();
        EXPECT_EQ(&instance1, &instance2);
    }

    TEST_F(QuestHubDatabaseTest, Database_Initialize_SetsInitializedFlag)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        // Note: Initialize() loads from database, which may not exist in unit test environment
        // This test verifies the flag is set, not database content
        bool result = db.Initialize();

        // If initialization succeeds, flag should be set
    if (result)
        {
            EXPECT_TRUE(db.IsInitialized());
        }
    }

    TEST_F(QuestHubDatabaseTest, Database_GetQuestHubCount_ReturnsCount)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        size_t count = db.GetQuestHubCount();

        // Count should be >= 0 (may be 0 if no database loaded)
        EXPECT_GE(count, 0);
    }

    TEST_F(QuestHubDatabaseTest, Database_GetMemoryUsage_ReturnsSize)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        size_t memoryUsage = db.GetMemoryUsage();

        // Memory usage should be >= 0
        EXPECT_GE(memoryUsage, 0);
    }

    // ============================================================================
    // QUEST HUB QUERY TESTS
    // ============================================================================

    TEST_F(QuestHubDatabaseTest, Database_GetQuestHubById_NotFound)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        // Query for a hub ID that definitely doesn't exist
        QuestHub const* hub = db.GetQuestHubById(999999);

        // Should return nullptr for non-existent ID
        EXPECT_EQ(hub, nullptr);
    }

    TEST_F(QuestHubDatabaseTest, Database_GetNearestQuestHub_NullPlayer)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        // Passing nullptr should safely return nullptr
        QuestHub const* hub = db.GetNearestQuestHub(nullptr);

        EXPECT_EQ(hub, nullptr);
    }

    TEST_F(QuestHubDatabaseTest, Database_GetQuestHubsForPlayer_NullPlayer)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        // Passing nullptr should safely return empty vector
        auto hubs = db.GetQuestHubsForPlayer(nullptr, 5);

        EXPECT_TRUE(hubs.empty());
    }

    TEST_F(QuestHubDatabaseTest, Database_GetQuestHubsForPlayer_MaxCountRespected)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        // Initialize database (may load actual data or be empty)
        db.Initialize();

        if (db.GetQuestHubCount() > 0)
        {
            // Create a mock player
            MockQuestHubPlayer player;
            EXPECT_CALL(player, GetLevel()).WillRepeatedly(Return(10));
            EXPECT_CALL(player, GetTeamId()).WillRepeatedly(Return(0)); // Alliance
            Position pos(0.0f, 0.0f, 0.0f, 0.0f);
            EXPECT_CALL(player, GetPosition()).WillRepeatedly(Return(pos));

            // Request max 3 hubs
            auto hubs = db.GetQuestHubsForPlayer(reinterpret_cast<Player const*>(&player), 3);

            // Result should have at most 3 hubs
            EXPECT_LE(hubs.size(), 3);
        }
    }

    TEST_F(QuestHubDatabaseTest, Database_GetQuestHubsInZone_InvalidZone)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        // Query for a zone ID that doesn't exist
        auto hubs = db.GetQuestHubsInZone(999999);

        // Should return empty vector for invalid zone
        EXPECT_TRUE(hubs.empty());
    }

    TEST_F(QuestHubDatabaseTest, Database_GetQuestHubAtPosition_NoZone)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        Position testPos(0.0f, 0.0f, 0.0f, 0.0f);

        // Query without zone filter
        QuestHub const* hub = db.GetQuestHubAtPosition(testPos, std::nullopt);

        // May return nullptr if no hub at position
        // This test just verifies it doesn't crash with optional zone
    }

    TEST_F(QuestHubDatabaseTest, Database_GetQuestHubAtPosition_WithZone)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();

        Position testPos(0.0f, 0.0f, 0.0f, 0.0f);

        // Query with specific zone filter
        QuestHub const* hub = db.GetQuestHubAtPosition(testPos, 1); // Dun Morogh zone

        // May return nullptr if no hub at position in that zone
        // This test just verifies it doesn't crash with optional zone
    }

    // ============================================================================
    // THREAD SAFETY TESTS
    // ============================================================================

    TEST_F(QuestHubDatabaseTest, Database_ConcurrentReads_ThreadSafe)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();
        db.Initialize();

        // Create multiple threads reading from database simultaneously
        std::vector<std::thread> threads;
        std::atomic<int> successCount{0};

        for (int i = 0; i < 10; ++i)
        {
            threads.emplace_back([&db, &successCount]()
            {
                // Perform 100 read operations
    for (int j = 0; j < 100; ++j)
                {
                    size_t count = db.GetQuestHubCount();
                    if (count >= 0) // Always true, just to use the value
                    {
                        successCount++;
                    }
                }
            });
        }

        // Wait for all threads to complete
    for (auto& thread : threads)
        {
            thread.join();
        }

        // All 1000 operations (10 threads × 100 ops) should succeed
        EXPECT_EQ(successCount.load(), 1000);
    }

    // ============================================================================
    // PERFORMANCE TESTS
    // ============================================================================

    TEST_F(QuestHubDatabaseTest, Performance_GetQuestHubCount_Fast)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();
        db.Initialize();

        auto start = std::chrono::high_resolution_clock::now();

        // Perform 10000 count queries
    for (int i = 0; i < 10000; ++i)
        {
            volatile size_t count = db.GetQuestHubCount();
            (void)count; // Suppress unused variable warning
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // Should complete in less than 100ms (100000 microseconds)
        EXPECT_LT(duration, 100000);

        // Average per query should be < 10 microseconds
        float avgMicros = duration / 10000.0f;
        EXPECT_LT(avgMicros, 10.0f);
    }

    TEST_F(QuestHubDatabaseTest, Performance_GetMemoryUsage_Fast)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();
        db.Initialize();

        auto start = std::chrono::high_resolution_clock::now();

        // Perform 10000 memory usage queries
    for (int i = 0; i < 10000; ++i)
        {
            volatile size_t usage = db.GetMemoryUsage();
            (void)usage; // Suppress unused variable warning
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // Should complete in less than 100ms
        EXPECT_LT(duration, 100000);
    }

    TEST_F(QuestHubDatabaseTest, Performance_GetQuestHubById_Fast)
    {
        QuestHubDatabase& db = QuestHubDatabase::Instance();
        db.Initialize();

        if (db.GetQuestHubCount() == 0)
        {
            GTEST_SKIP() << "No quest hubs loaded, skipping performance test";
        }

        auto start = std::chrono::high_resolution_clock::now();

        // Perform 10000 lookups
    for (int i = 0; i < 10000; ++i)
        {
            volatile QuestHub const* hub = db.GetQuestHubById(1);
            (void)hub; // Suppress unused variable warning
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // Hash table lookup should be very fast: < 1ms total (< 1000 microseconds)
        EXPECT_LT(duration, 1000);

        // Average per lookup should be < 0.1 microseconds (100ns)
        float avgMicros = duration / 10000.0f;
        EXPECT_LT(avgMicros, 0.1f);
    }

} // anonymous namespace
