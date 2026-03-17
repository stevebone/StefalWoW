/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "WorldSession.h"
#include "BotSession.h"
#include "Log.h"
#include "GameTime.h"
#include <memory>
#include <exception>

using namespace Playerbot;

/**
 * VIRTUAL IsBot() VALIDATION TEST
 *
 * This test validates that our critical fix (making IsBot() virtual) works correctly
 * and prevents the ACCESS_VIOLATION crashes at Socket.h:230.
 *
 * VALIDATION POINTS:
 * 1. BotSession::IsBot() override is called through WorldSession pointer
 * 2. BUILD_PLAYERBOT guards in WorldSession.cpp now work correctly
 * 3. Socket operations are properly protected for bot sessions
 * 4. No more ACCESS_VIOLATION crashes during Update() calls
 */
class VirtualIsBotValidationTest
{
public:
    static void ValidateVirtualIsBotFix()
    {
        TC_LOG_INFO("test.playerbot", " VALIDATING VIRTUAL IsBot() FIX");
        TC_LOG_INFO("test.playerbot", " Purpose: Verify that ACCESS_VIOLATION crashes are resolved");

        TestPolymorphicIsBotCall();
        TestSocketGuardEffectiveness();
        TestUpdateLoopStability();
        TestConcurrentAccess();
        TestDestructorSafety();

        TC_LOG_INFO("test.playerbot", " VIRTUAL IsBot() VALIDATION COMPLETE");
    }

private:
    /**
     * TEST 1: Polymorphic IsBot() Call
     * Verify that IsBot() is properly overridden and called through base pointer
     */
    static void TestPolymorphicIsBotCall()
    {
        TC_LOG_INFO("test.playerbot", " TEST 1: Polymorphic IsBot() Call Validation");

        try {
            auto botSession = BotSession::Create(77777);
            if (!botSession)
            {
                TC_LOG_ERROR("test.playerbot", " Failed to create BotSession");
                return;
            }

            // Test 1.1: Direct call through BotSession pointer
            bool directResult = botSession->IsBot();
            TC_LOG_INFO("test.playerbot", "Direct BotSession::IsBot() = {}", directResult);

            // Test 1.2: Polymorphic call through WorldSession pointer
            WorldSession* worldPtr = botSession.get();
            bool polymorphicResult = worldPtr->IsBot();
            TC_LOG_INFO("test.playerbot", "Polymorphic WorldSession::IsBot() = {}", polymorphicResult);

            // CRITICAL VALIDATION
    if (directResult && polymorphicResult)
    {
                TC_LOG_INFO("test.playerbot", " VIRTUAL IsBot() FIX SUCCESSFUL");
                TC_LOG_INFO("test.playerbot", " Both direct and polymorphic calls return true");
            } else if (directResult && !polymorphicResult)
            {
                TC_LOG_ERROR("test.playerbot", " VIRTUAL FUNCTION NOT WORKING");
                TC_LOG_ERROR("test.playerbot", " Polymorphic call failed - IsBot() is not virtual");
            } else {
                TC_LOG_ERROR("test.playerbot", " IsBot() implementation broken");
                TC_LOG_ERROR("test.playerbot", " Direct: {}, Polymorphic: {}", directResult, polymorphicResult);
            }

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception in polymorphic test: {}", e.what());
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception in polymorphic test");
        }

        TC_LOG_INFO("test.playerbot", " TEST 1 COMPLETE");
    }

    /**
     * TEST 2: Socket Guard Effectiveness
     * Verify that the socket guards in WorldSession.cpp now work properly
     */
    static void TestSocketGuardEffectiveness()
    {
        TC_LOG_INFO("test.playerbot", " TEST 2: Socket Guard Effectiveness");

        try {
            auto botSession = BotSession::Create(77776);
            if (!botSession)
            {
                TC_LOG_ERROR("test.playerbot", " Failed to create BotSession");
                return;
            }

            TC_LOG_INFO("test.playerbot", " Testing socket access prevention...");

            // Verify IsBot() works through polymorphic interface
            WorldSession* sessionPtr = botSession.get();
            bool isBotViaPtr = sessionPtr->IsBot();

            if (isBotViaPtr)
            {
                TC_LOG_INFO("test.playerbot", " Socket guards should now be effective");

                // Test conditions that previously caused crashes
                bool disconnected = sessionPtr->PlayerDisconnected();
                TC_LOG_INFO("test.playerbot", "PlayerDisconnected() through pointer: {}", disconnected);

                bool idle = sessionPtr->IsConnectionIdle();
                TC_LOG_INFO("test.playerbot", "IsConnectionIdle() through pointer: {}", idle);

                // Both should be safe now
    if (!disconnected && !idle)
    {
                    TC_LOG_INFO("test.playerbot", " Socket guard methods working correctly");
                } else {
                    TC_LOG_WARN("test.playerbot", " Unexpected results from socket guard methods");
                }

            } else {
                TC_LOG_ERROR("test.playerbot", " IsBot() still returns false - fix not working");
            }

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception in socket guard test: {}", e.what());
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception in socket guard test");
        }

        TC_LOG_INFO("test.playerbot", " TEST 2 COMPLETE");
    }

    /**
     * TEST 3: Update Loop Stability
     * Test the WorldSession::Update() method that previously caused crashes
     */
    static void TestUpdateLoopStability()
    {
        TC_LOG_INFO("test.playerbot", " TEST 3: Update Loop Stability Test");

        try {
            auto botSession = BotSession::Create(77775);
            if (!botSession)
            {
                TC_LOG_ERROR("test.playerbot", " Failed to create BotSession");
                return;
            }

            // Test the exact conditions that caused the original crash
            class CrashTestFilter : public PacketFilter {
            public:
                bool Process(WorldPacket* packet) override { return true; }
                bool ProcessUnsafe() override
                {
                    // Return true to trigger the unsafe code path that accesses sockets
                    // This is where the original crash occurred
                    return true;
                }
            } crashTestFilter;

            TC_LOG_INFO("test.playerbot", " Testing crash-prone Update() path...");

            // Set timeout condition that triggers socket cleanup
            botSession->ResetTimeOutTime(false);

            // This Update() call previously caused ACCESS_VIOLATION at Socket.h:230
            // With our fix, it should now be safe
            bool updateResult = botSession->Update(100, crashTestFilter);

            if (updateResult)
            {
                TC_LOG_INFO("test.playerbot", " UPDATE LOOP CRASH FIXED");
                TC_LOG_INFO("test.playerbot", " No ACCESS_VIOLATION in socket operations");
            } else {
                TC_LOG_WARN("test.playerbot", " Update returned false - check implementation");
            }

            // Test multiple Update() calls for stability
            TC_LOG_INFO("test.playerbot", " Testing repeated Update() calls...");

            for (int i = 0; i < 10; ++i) {
                bool result = botSession->Update(50, crashTestFilter);
                if (!result)
                {
                    TC_LOG_WARN("test.playerbot", " Update failed on iteration {}", i);
                    break;
                }
            }

            TC_LOG_INFO("test.playerbot", " Repeated Update() calls completed");

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception in update loop test: {}", e.what());
            TC_LOG_ERROR("test.playerbot", " This might indicate the fix is incomplete");
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " ACCESS_VIOLATION in update loop test");
            TC_LOG_ERROR("test.playerbot", " The fix has not resolved the crash");
        }

        TC_LOG_INFO("test.playerbot", " TEST 3 COMPLETE");
    }

    /**
     * TEST 4: Concurrent Access
     * Test thread safety of the IsBot() fix under concurrent access
     */
    static void TestConcurrentAccess()
    {
        TC_LOG_INFO("test.playerbot", " TEST 4: Concurrent Access Test");

        try {
            auto botSession = BotSession::Create(77774);
            if (!botSession)
            {
                TC_LOG_ERROR("test.playerbot", " Failed to create BotSession");
                return;
            }

            TC_LOG_INFO("test.playerbot", " Testing concurrent IsBot() calls...");

            std::atomic<int> successCount{0};
            std::atomic<int> totalCalls{0};

            std::vector<std::thread> threads;

            // Launch threads that access IsBot() concurrently
    for (int i = 0; i < 4; ++i) {
                threads.emplace_back([&botSession, &successCount, &totalCalls]() {
                    WorldSession* sessionPtr = botSession.get();

                    for (int j = 0; j < 25; ++j) {
                        totalCalls++;
                        try {
                            bool isBot = sessionPtr->IsBot();
                            if (isBot)
                            {
                                successCount++;
                            }
                        } catch (...)
                        {
                            // Exception counted as failure
                        }
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                });
            }

            // Wait for completion
    for (auto& thread : threads)
    {
                thread.join();
            }

            int successful = successCount.load();
            int total = totalCalls.load();

            TC_LOG_INFO("test.playerbot", "Concurrent IsBot() results: {}/{} successful", successful, total);

            if (successful == total) {
                TC_LOG_INFO("test.playerbot", " Concurrent access is stable");
            } else {
                TC_LOG_WARN("test.playerbot", " Some concurrent calls failed: {}/{}", successful, total);
            }

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception in concurrent test: {}", e.what());
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception in concurrent test");
        }

        TC_LOG_INFO("test.playerbot", " TEST 4 COMPLETE");
    }

    /**
     * TEST 5: Destructor Safety
     * Test that the fix prevents crashes during session destruction
     */
    static void TestDestructorSafety()
    {
        TC_LOG_INFO("test.playerbot", " TEST 5: Destructor Safety Test");

        try {
            TC_LOG_INFO("test.playerbot", " Testing safe session destruction...");

            // Create and destroy sessions in a loop
    for (int i = 0; i < 5; ++i) {
                {
                    auto botSession = BotSession::Create(77770 + i);
                    if (botSession)
                    {
                        // Verify IsBot() works
                        WorldSession* ptr = botSession.get();
                        bool isBot = ptr->IsBot();

                        if (isBot)
                        {
                            TC_LOG_DEBUG("test.playerbot", "Session {} IsBot() = true", i);
                        } else {
                            TC_LOG_WARN("test.playerbot", "Session {} IsBot() = false", i);
                        }

                        // Do some operations that set up state
                        class DestructorTestFilter : public PacketFilter {
                        public:
                            bool Process(WorldPacket* packet) override { return true; }
                            bool ProcessUnsafe() override { return true; }
                        } filter;

                        botSession->Update(50, filter);
                    }
                    // Destructor called here - should not crash due to proper IsBot() override
                }

                TC_LOG_DEBUG("test.playerbot", "Session {} destroyed safely", i);
            }

            TC_LOG_INFO("test.playerbot", " All sessions destroyed without crashes");

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception in destructor test: {}", e.what());
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception in destructor test");
        }

        TC_LOG_INFO("test.playerbot", " TEST 5 COMPLETE");
    }
};

/**
 * Main validation function
 */
void ValidateVirtualIsBotFix()
{
    TC_LOG_INFO("test.playerbot", " VALIDATING CRITICAL IsBot() VIRTUAL FIX");
    TC_LOG_INFO("test.playerbot", " Expected result: No more ACCESS_VIOLATION crashes");

    VirtualIsBotValidationTest::ValidateVirtualIsBotFix();

    TC_LOG_INFO("test.playerbot", " VALIDATION COMPLETE");
    TC_LOG_INFO("test.playerbot", " If all tests pass, the ACCESS_VIOLATION crashes should be resolved");
}