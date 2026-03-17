/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch2/catch_test_macros.hpp"
#include "AI/Blackboard/SharedBlackboard.h"
#include <thread>
#include <atomic>

using namespace Playerbot;

// ============================================================================
// SharedBlackboard Basic Tests
// ============================================================================

TEST_CASE("SharedBlackboard: Basic set and get", "[blackboard][basic]")
{
    SharedBlackboard blackboard;

    SECTION("Can set and get int")
    {
        blackboard.Set("health", 100);

        int value = 0;
        REQUIRE(blackboard.Get("health", value));
        REQUIRE(value == 100);
    }

    SECTION("Can set and get float")
    {
        blackboard.Set("speed", 5.5f);

        float value = 0.0f;
        REQUIRE(blackboard.Get("speed", value));
        REQUIRE(value == 5.5f);
    }

    SECTION("Can set and get string")
    {
        blackboard.Set("name", std::string("TestBot"));

        std::string value;
        REQUIRE(blackboard.Get("name", value));
        REQUIRE(value == "TestBot");
    }

    SECTION("Can set and get bool")
    {
        blackboard.Set("inCombat", true);

        bool value = false;
        REQUIRE(blackboard.Get("inCombat", value));
        REQUIRE(value == true);
    }
}

TEST_CASE("SharedBlackboard: GetOr default values", "[blackboard][defaults]")
{
    SharedBlackboard blackboard;

    SECTION("Returns default for missing key")
    {
        int value = blackboard.GetOr("missing", 42);
        REQUIRE(value == 42);
    }

    SECTION("Returns stored value when present")
    {
        blackboard.Set("existing", 100);
        int value = blackboard.GetOr("existing", 42);
        REQUIRE(value == 100);
    }
}

TEST_CASE("SharedBlackboard: Has and Remove", "[blackboard][basic]")
{
    SharedBlackboard blackboard;

    SECTION("Has returns false for missing key")
    {
        REQUIRE_FALSE(blackboard.Has("missing"));
    }

    SECTION("Has returns true for existing key")
    {
        blackboard.Set("existing", 100);
        REQUIRE(blackboard.Has("existing"));
    }

    SECTION("Can remove key")
    {
        blackboard.Set("toRemove", 100);
        REQUIRE(blackboard.Has("toRemove"));

        blackboard.Remove("toRemove");
        REQUIRE_FALSE(blackboard.Has("toRemove"));
    }
}

TEST_CASE("SharedBlackboard: Clear", "[blackboard][basic]")
{
    SharedBlackboard blackboard;

    blackboard.Set("key1", 1);
    blackboard.Set("key2", 2);
    blackboard.Set("key3", 3);

    SECTION("Clear removes all keys")
    {
        REQUIRE(blackboard.GetKeys().size() == 3);

        blackboard.Clear();

        REQUIRE(blackboard.GetKeys().empty());
        REQUIRE_FALSE(blackboard.Has("key1"));
        REQUIRE_FALSE(blackboard.Has("key2"));
        REQUIRE_FALSE(blackboard.Has("key3"));
    }
}

TEST_CASE("SharedBlackboard: GetKeys", "[blackboard][keys]")
{
    SharedBlackboard blackboard;

    SECTION("Empty blackboard returns no keys")
    {
        REQUIRE(blackboard.GetKeys().empty());
    }

    SECTION("Returns all keys")
    {
        blackboard.Set("key1", 1);
        blackboard.Set("key2", 2);
        blackboard.Set("key3", 3);

        auto keys = blackboard.GetKeys();
        REQUIRE(keys.size() == 3);

        REQUIRE(std::find(keys.begin(), keys.end(), "key1") != keys.end());
        REQUIRE(std::find(keys.begin(), keys.end(), "key2") != keys.end());
        REQUIRE(std::find(keys.begin(), keys.end(), "key3") != keys.end());
    }
}

TEST_CASE("SharedBlackboard: Type safety", "[blackboard][types]")
{
    SharedBlackboard blackboard;

    blackboard.Set("value", 100); // int

    SECTION("Get with wrong type returns false")
    {
        float wrongType = 0.0f;
        REQUIRE_FALSE(blackboard.Get("value", wrongType));
    }

    SECTION("Get with correct type returns true")
    {
        int correctType = 0;
        REQUIRE(blackboard.Get("value", correctType));
        REQUIRE(correctType == 100);
    }
}

// ============================================================================
// SharedBlackboard Change Listener Tests
// ============================================================================

TEST_CASE("SharedBlackboard: Change listeners", "[blackboard][listeners]")
{
    SharedBlackboard blackboard;

    SECTION("Listener is called on value change")
    {
        bool listenerCalled = false;
        std::string changedKey;

        uint32 listenerId = blackboard.RegisterListener("testKey",
            [&](SharedBlackboard::ChangeEvent const& event) {
                listenerCalled = true;
                changedKey = event.key;
            });

        blackboard.Set("testKey", 100);

        REQUIRE(listenerCalled);
        REQUIRE(changedKey == "testKey");

        blackboard.UnregisterListener(listenerId);
    }

    SECTION("Global listener watches all keys")
    {
        int callCount = 0;

        uint32 listenerId = blackboard.RegisterListener("", // Empty = watch all
            [&](SharedBlackboard::ChangeEvent const& event) {
                callCount++;
            });

        blackboard.Set("key1", 1);
        blackboard.Set("key2", 2);
        blackboard.Set("key3", 3);

        REQUIRE(callCount == 3);

        blackboard.UnregisterListener(listenerId);
    }

    SECTION("Unregistered listener is not called")
    {
        int callCount = 0;

        uint32 listenerId = blackboard.RegisterListener("testKey",
            [&](SharedBlackboard::ChangeEvent const& event) {
                callCount++;
            });

        blackboard.Set("testKey", 1);
        REQUIRE(callCount == 1);

        blackboard.UnregisterListener(listenerId);

        blackboard.Set("testKey", 2);
        REQUIRE(callCount == 1); // Not called after unregister
    }
}

// ============================================================================
// SharedBlackboard Copy and Merge Tests
// ============================================================================

TEST_CASE("SharedBlackboard: CopyFrom", "[blackboard][copy]")
{
    SharedBlackboard source;
    source.Set("key1", 100);
    source.Set("key2", 200);

    SharedBlackboard dest;

    SECTION("CopyFrom copies all data")
    {
        dest.CopyFrom(source);

        int val1 = 0, val2 = 0;
        REQUIRE(dest.Get("key1", val1));
        REQUIRE(dest.Get("key2", val2));
        REQUIRE(val1 == 100);
        REQUIRE(val2 == 200);
    }
}

TEST_CASE("SharedBlackboard: MergeFrom", "[blackboard][merge]")
{
    SharedBlackboard source;
    source.Set("key1", 100);
    source.Set("key2", 200);

    SharedBlackboard dest;
    dest.Set("key2", 999); // Existing key
    dest.Set("key3", 300); // Key not in source

    SECTION("MergeFrom with overwrite=true")
    {
        dest.MergeFrom(source, true);

        int val1 = 0, val2 = 0, val3 = 0;
        REQUIRE(dest.Get("key1", val1));
        REQUIRE(dest.Get("key2", val2));
        REQUIRE(dest.Get("key3", val3));
        REQUIRE(val1 == 100);
        REQUIRE(val2 == 200); // Overwritten
        REQUIRE(val3 == 300); // Preserved
    }

    SECTION("MergeFrom with overwrite=false")
    {
        dest.MergeFrom(source, false);

        int val1 = 0, val2 = 0, val3 = 0;
        REQUIRE(dest.Get("key1", val1));
        REQUIRE(dest.Get("key2", val2));
        REQUIRE(dest.Get("key3", val3));
        REQUIRE(val1 == 100);
        REQUIRE(val2 == 999); // NOT overwritten
        REQUIRE(val3 == 300);
    }
}

// ============================================================================
// SharedBlackboard Thread Safety Tests
// ============================================================================

TEST_CASE("SharedBlackboard: Concurrent reads", "[blackboard][thread-safety]")
{
    SharedBlackboard blackboard;
    blackboard.Set("sharedValue", 42);

    SECTION("Multiple threads can read concurrently")
    {
        std::atomic<int> successCount{0};
        constexpr int numThreads = 10;
        constexpr int readsPerThread = 1000;

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&blackboard, &successCount]() {
                for (int j = 0; j < readsPerThread; ++j)
                {
                    int value = 0;
                    if (blackboard.Get("sharedValue", value) && value == 42)
                        successCount++;
                }
            });
        }

        for (auto& thread : threads)
            thread.join();

        REQUIRE(successCount == numThreads * readsPerThread);
    }
}

TEST_CASE("SharedBlackboard: Concurrent writes", "[blackboard][thread-safety]")
{
    SharedBlackboard blackboard;

    SECTION("Multiple threads can write concurrently without corruption")
    {
        constexpr int numThreads = 10;
        constexpr int writesPerThread = 100;

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&blackboard, i]() {
                for (int j = 0; j < writesPerThread; ++j)
                {
                    std::string key = "thread_" + std::to_string(i);
                    blackboard.Set(key, j);
                }
            });
        }

        for (auto& thread : threads)
            thread.join();

        // Verify all thread keys exist
    for (int i = 0; i < numThreads; ++i)
        {
            std::string key = "thread_" + std::to_string(i);
            REQUIRE(blackboard.Has(key));
        }
    }
}

// ============================================================================
// BlackboardManager Tests
// ============================================================================

TEST_CASE("BlackboardManager: Bot blackboards", "[blackboard][manager][bot]")
{
    BlackboardManager::ClearAll();

    ObjectGuid botGuid1 = ObjectGuid::Create<HighGuid::Player>(0, 1);
    ObjectGuid botGuid2 = ObjectGuid::Create<HighGuid::Player>(0, 2);

    SECTION("Can get bot blackboard")
    {
        auto* blackboard = BlackboardManager::GetBotBlackboard(botGuid1);
        REQUIRE(blackboard != nullptr);
    }

    SECTION("Same bot returns same blackboard")
    {
        auto* bb1 = BlackboardManager::GetBotBlackboard(botGuid1);
        auto* bb2 = BlackboardManager::GetBotBlackboard(botGuid1);
        REQUIRE(bb1 == bb2);
    }

    SECTION("Different bots return different blackboards")
    {
        auto* bb1 = BlackboardManager::GetBotBlackboard(botGuid1);
        auto* bb2 = BlackboardManager::GetBotBlackboard(botGuid2);
        REQUIRE(bb1 != bb2);
    }

    SECTION("Can remove bot blackboard")
    {
        auto* bb = BlackboardManager::GetBotBlackboard(botGuid1);
        bb->Set("test", 100);

        BlackboardManager::RemoveBotBlackboard(botGuid1);

        auto* newBb = BlackboardManager::GetBotBlackboard(botGuid1);
        REQUIRE_FALSE(newBb->Has("test")); // New blackboard
    }

    BlackboardManager::ClearAll();
}

TEST_CASE("BlackboardManager: Group blackboards", "[blackboard][manager][group]")
{
    BlackboardManager::ClearAll();

    SECTION("Can get group blackboard")
    {
        auto* blackboard = BlackboardManager::GetGroupBlackboard(1);
        REQUIRE(blackboard != nullptr);
    }

    SECTION("Same group returns same blackboard")
    {
        auto* bb1 = BlackboardManager::GetGroupBlackboard(1);
        auto* bb2 = BlackboardManager::GetGroupBlackboard(1);
        REQUIRE(bb1 == bb2);
    }

    SECTION("Different groups return different blackboards")
    {
        auto* bb1 = BlackboardManager::GetGroupBlackboard(1);
        auto* bb2 = BlackboardManager::GetGroupBlackboard(2);
        REQUIRE(bb1 != bb2);
    }

    BlackboardManager::ClearAll();
}

TEST_CASE("BlackboardManager: Raid blackboards", "[blackboard][manager][raid]")
{
    BlackboardManager::ClearAll();

    SECTION("Can get raid blackboard")
    {
        auto* blackboard = BlackboardManager::GetRaidBlackboard(1);
        REQUIRE(blackboard != nullptr);
    }

    SECTION("Same raid returns same blackboard")
    {
        auto* bb1 = BlackboardManager::GetRaidBlackboard(1);
        auto* bb2 = BlackboardManager::GetRaidBlackboard(1);
        REQUIRE(bb1 == bb2);
    }

    BlackboardManager::ClearAll();
}

TEST_CASE("BlackboardManager: Zone blackboards", "[blackboard][manager][zone]")
{
    BlackboardManager::ClearAll();

    SECTION("Can get zone blackboard")
    {
        auto* blackboard = BlackboardManager::GetZoneBlackboard(1519);
        REQUIRE(blackboard != nullptr);
    }

    SECTION("Same zone returns same blackboard")
    {
        auto* bb1 = BlackboardManager::GetZoneBlackboard(1519);
        auto* bb2 = BlackboardManager::GetZoneBlackboard(1519);
        REQUIRE(bb1 == bb2);
    }

    BlackboardManager::ClearAll();
}

TEST_CASE("BlackboardManager: Clear all", "[blackboard][manager]")
{
    BlackboardManager::ClearAll();

    ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(0, 1);

    auto* botBb = BlackboardManager::GetBotBlackboard(botGuid);
    auto* groupBb = BlackboardManager::GetGroupBlackboard(1);
    auto* raidBb = BlackboardManager::GetRaidBlackboard(1);
    auto* zoneBb = BlackboardManager::GetZoneBlackboard(1519);

    botBb->Set("test", 1);
    groupBb->Set("test", 2);
    raidBb->Set("test", 3);
    zoneBb->Set("test", 4);

    SECTION("ClearAll removes all blackboards")
    {
        BlackboardManager::ClearAll();

        // Getting new blackboards should return empty ones
        auto* newBotBb = BlackboardManager::GetBotBlackboard(botGuid);
        REQUIRE_FALSE(newBotBb->Has("test"));
    }

    BlackboardManager::ClearAll();
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE("SharedBlackboard: Real-world scenario", "[blackboard][integration]")
{
    BlackboardManager::ClearAll();

    ObjectGuid bot1 = ObjectGuid::Create<HighGuid::Player>(0, 1);
    ObjectGuid bot2 = ObjectGuid::Create<HighGuid::Player>(0, 2);

    auto* bot1Bb = BlackboardManager::GetBotBlackboard(bot1);
    auto* bot2Bb = BlackboardManager::GetBotBlackboard(bot2);
    auto* groupBb = BlackboardManager::GetGroupBlackboard(1);

    SECTION("Bots share data via group blackboard")
    {
        // Bot1 sets personal data
        bot1Bb->Set("myHealth", 100);
        bot1Bb->Set("myMana", 50);

        // Bot1 shares to group
        groupBb->Set("focusTarget", 12345);

        // Bot2 reads from group
        int focusTarget = 0;
        REQUIRE(groupBb->Get("focusTarget", focusTarget));
        REQUIRE(focusTarget == 12345);

        // Bot2 cannot see Bot1's personal data
        REQUIRE_FALSE(bot2Bb->Has("myHealth"));
    }

    BlackboardManager::ClearAll();
}

TEST_CASE("SharedBlackboard: Performance characteristics", "[blackboard][performance]")
{
    SharedBlackboard blackboard;

    SECTION("Can handle many keys")
    {
        for (int i = 0; i < 10000; ++i)
        {
            std::string key = "key_" + std::to_string(i);
            blackboard.Set(key, i);
        }

        REQUIRE(blackboard.GetKeys().size() == 10000);

        for (int i = 0; i < 10000; ++i)
        {
            std::string key = "key_" + std::to_string(i);
            int value = 0;
            REQUIRE(blackboard.Get(key, value));
            REQUIRE(value == i);
        }
    }
}
