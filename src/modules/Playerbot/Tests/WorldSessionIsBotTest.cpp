/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "WorldSession.h"
#include "BotSession.h"
#include "Log.h"
#include <memory>
#include <exception>

using namespace Playerbot;

/**
 * WORLDSESSION ISBOT() INVESTIGATION
 *
 * This class investigates a potential critical bug in the IsBot() implementation.
 *
 * HYPOTHESIS: The ACCESS_VIOLATION crash at Socket.h:230 occurs because the
 * BUILD_PLAYERBOT guards in WorldSession.cpp are not working. This could happen if:
 *
 * 1. The IsBot() method is not properly overridden in BotSession
 * 2. The _isBot member variable is not properly initialized
 * 3. There's a mismatch between compile-time and runtime BUILD_PLAYERBOT flags
 * 4. The WorldSession constructor doesn't properly set the bot flag
 */
class WorldSessionIsBotInvestigation
{
public:
    static void InvestigateIsBotImplementation()
    {
        TC_LOG_INFO("test.playerbot", " INVESTIGATING IsBot() IMPLEMENTATION");
        TC_LOG_INFO("test.playerbot", " Purpose: Verify that BUILD_PLAYERBOT guards are functional");

        TestWorldSessionConstructorBotFlag();
        TestIsBotOverrideConsistency();
        TestCompileTimeVsRuntimeFlags();
        TestGuardEffectivenessInWorldSession();
        AnalyzeSocketAccessCodePaths();

        TC_LOG_INFO("test.playerbot", " IsBot() INVESTIGATION COMPLETE");
    }

private:
    /**
     * TEST 1: WorldSession Constructor Bot Flag
     * Verify that the WorldSession constructor properly sets the _isBot flag
     */
    static void TestWorldSessionConstructorBotFlag()
    {
        TC_LOG_INFO("test.playerbot", " TEST 1: WorldSession Constructor Bot Flag");

        try {
            // Create a BotSession and examine its WorldSession base
            auto botSession = BotSession::Create(88888);
            if (!botSession)
            {
                TC_LOG_ERROR("test.playerbot", " Failed to create BotSession");
                return;
            }

            // Test the IsBot() method directly
            bool isBotResult = botSession->IsBot();
            TC_LOG_INFO("test.playerbot", "BotSession::IsBot() returns: {}", isBotResult);

            if (!isBotResult)
            {
                TC_LOG_ERROR("test.playerbot", " CRITICAL BUG FOUND: IsBot() returns false!");
                TC_LOG_ERROR("test.playerbot", " This means BUILD_PLAYERBOT guards will NOT protect against socket access");
                TC_LOG_ERROR("test.playerbot", " ROOT CAUSE OF CRASH IDENTIFIED");

                // Investigate why IsBot() is returning false
                AnalyzeIsBotFailure(botSession);
            } else {
                TC_LOG_INFO("test.playerbot", " IsBot() correctly returns true");
            }

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception in constructor test: {}", e.what());
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception in constructor test");
        }

        TC_LOG_INFO("test.playerbot", " TEST 1 COMPLETE");
    }

    /**
     * TEST 2: IsBot() Override Consistency
     * Test if BotSession properly overrides the WorldSession IsBot() method
     */
    static void TestIsBotOverrideConsistency()
    {
        TC_LOG_INFO("test.playerbot", " TEST 2: IsBot() Override Consistency");

        try {
            auto botSession = BotSession::Create(88887);
            if (!botSession)
            {
                TC_LOG_ERROR("test.playerbot", " Failed to create BotSession");
                return;
            }

            // Test IsBot() through BotSession pointer
            bool botSessionIsBot = botSession->IsBot();
            TC_LOG_INFO("test.playerbot", "BotSession pointer IsBot(): {}", botSessionIsBot);

            // Test IsBot() through WorldSession pointer (polymorphic call)
            WorldSession* worldSessionPtr = botSession.get();
            bool worldSessionIsBot = worldSessionPtr->IsBot();
            TC_LOG_INFO("test.playerbot", "WorldSession pointer IsBot(): {}", worldSessionIsBot);

            if (botSessionIsBot != worldSessionIsBot) {
                TC_LOG_ERROR("test.playerbot", " INCONSISTENCY: IsBot() results differ!");
                TC_LOG_ERROR("test.playerbot", " This indicates a virtual function override issue");
            } else if (!botSessionIsBot || !worldSessionIsBot)
            {
                TC_LOG_ERROR("test.playerbot", " Both calls return false - IsBot() not working");
            } else {
                TC_LOG_INFO("test.playerbot", " IsBot() override is consistent");
            }

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception in override test: {}", e.what());
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception in override test");
        }

        TC_LOG_INFO("test.playerbot", " TEST 2 COMPLETE");
    }

    /**
     * TEST 3: Compile-time vs Runtime Flags
     * Verify BUILD_PLAYERBOT is consistently defined
     */
    static void TestCompileTimeVsRuntimeFlags()
    {
        TC_LOG_INFO("test.playerbot", " TEST 3: Compile-time vs Runtime Flags");

#ifdef BUILD_PLAYERBOT
        TC_LOG_INFO("test.playerbot", " BUILD_PLAYERBOT is defined at compile time");

        // Test if the bot flag is actually used in WorldSession constructor
        try {
            auto botSession = BotSession::Create(88886);
            if (!botSession)
            {
                TC_LOG_ERROR("test.playerbot", " Failed to create BotSession");
                return;
            }

            bool isBot = botSession->IsBot();
            if (isBot)
            {
                TC_LOG_INFO("test.playerbot", " Runtime IsBot() matches compile-time flag");
            } else {
                TC_LOG_ERROR("test.playerbot", " MISMATCH: Compile-time BUILD_PLAYERBOT defined but runtime IsBot() false");
                TC_LOG_ERROR("test.playerbot", " This suggests the WorldSession constructor is not using the bot parameter");
            }

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception testing runtime flags: {}", e.what());
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception testing runtime flags");
        }

#else
        TC_LOG_ERROR("test.playerbot", " BUILD_PLAYERBOT is NOT defined at compile time");
        TC_LOG_ERROR("test.playerbot", " This explains why IsBot() guards don't work - recompile with -DBUILD_PLAYERBOT=1");
#endif

        TC_LOG_INFO("test.playerbot", " TEST 3 COMPLETE");
    }

    /**
     * TEST 4: Guard Effectiveness in WorldSession
     * Test the actual guards in WorldSession.cpp to see if they work
     */
    static void TestGuardEffectivenessInWorldSession()
    {
        TC_LOG_INFO("test.playerbot", " TEST 4: Guard Effectiveness in WorldSession");

        try {
            auto botSession = BotSession::Create(88885);
            if (!botSession)
            {
                TC_LOG_ERROR("test.playerbot", " Failed to create BotSession");
                return;
            }

            // Test specific WorldSession methods that have BUILD_PLAYERBOT guards
            TC_LOG_INFO("test.playerbot", " Testing guarded socket operations...");

            // Test PlayerDisconnected() - should return false for bots due to override
            bool disconnected = botSession->PlayerDisconnected();
            TC_LOG_INFO("test.playerbot", "PlayerDisconnected(): {}", disconnected);

            if (disconnected)
            {
                TC_LOG_WARN("test.playerbot", " PlayerDisconnected() returns true for bot - might indicate guard failure");
            }

            // Test IsConnectionIdle() - should return false for bots
            bool idle = botSession->IsConnectionIdle();
            TC_LOG_INFO("test.playerbot", "IsConnectionIdle(): {}", idle);

            if (idle)
            {
                TC_LOG_WARN("test.playerbot", " IsConnectionIdle() returns true for bot - might indicate guard failure");
            }

            // Test the Update method which contains the critical socket access code
            TC_LOG_INFO("test.playerbot", " Testing Update method with unsafe filter...");

            class UnsafeTestFilter : public PacketFilter {
            public:
                bool Process(WorldPacket* packet) override { return true; }
                bool ProcessUnsafe() override { return true; } // Force unsafe path
            } unsafeFilter;

            // This should trigger the socket cleanup code that causes the crash
            // If the guards work, this should not crash
            bool updateResult = botSession->Update(100, unsafeFilter);
            TC_LOG_INFO("test.playerbot", "Unsafe Update result: {}", updateResult);

            if (!updateResult)
            {
                TC_LOG_WARN("test.playerbot", " Update returned false - might indicate guard intervention");
            }

            TC_LOG_INFO("test.playerbot", " Guard effectiveness test completed without crash");

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception in guard test: {}", e.what());
            TC_LOG_ERROR("test.playerbot", " This might indicate a guard failure leading to socket access");
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception in guard test");
            TC_LOG_ERROR("test.playerbot", " This might be the ACCESS_VIOLATION we're looking for");
        }

        TC_LOG_INFO("test.playerbot", " TEST 4 COMPLETE");
    }

    /**
     * TEST 5: Analyze Socket Access Code Paths
     * Examine the specific code paths that lead to Socket::CloseSocket()
     */
    static void AnalyzeSocketAccessCodePaths()
    {
        TC_LOG_INFO("test.playerbot", " TEST 5: Socket Access Code Path Analysis");

        try {
            auto botSession = BotSession::Create(88884);
            if (!botSession)
            {
                TC_LOG_ERROR("test.playerbot", " Failed to create BotSession");
                return;
            }

            TC_LOG_INFO("test.playerbot", " Analyzing code paths that lead to Socket::CloseSocket()...");

            // Path 1: WorldSession::Update() timeout handling
            TC_LOG_INFO("test.playerbot", "Path 1: Timeout handling in WorldSession::Update()");

            // Set up conditions for timeout
            botSession->ResetTimeOutTime(false);

            // The timeout path in WorldSession::Update() checks IsConnectionIdle()
            // and then calls m_Socket[CONNECTION_TYPE_REALM]->CloseSocket()
            // Our guards should prevent this

            class TimeoutTestFilter : public PacketFilter {
            public:
                bool Process(WorldPacket* packet) override { return true; }
                bool ProcessUnsafe() override { return false; } // Safe path first
            } timeoutFilter;

            bool timeoutResult = botSession->Update(100, timeoutFilter);
            TC_LOG_INFO("test.playerbot", "Timeout path result: {}", timeoutResult);

            // Path 2: WorldSession::Update() socket cleanup
            TC_LOG_INFO("test.playerbot", "Path 2: Socket cleanup in WorldSession::Update()");

            class CleanupTestFilter : public PacketFilter {
            public:
                bool Process(WorldPacket* packet) override { return true; }
                bool ProcessUnsafe() override { return true; } // Unsafe path triggers cleanup
            } cleanupFilter;

            // This triggers the socket cleanup code around line 549-575 in WorldSession.cpp
            // which contains multiple CloseSocket() calls that should be guarded
            bool cleanupResult = botSession->Update(100, cleanupFilter);
            TC_LOG_INFO("test.playerbot", "Cleanup path result: {}", cleanupResult);

            // Path 3: WorldSession destructor
            TC_LOG_INFO("test.playerbot", "Path 3: WorldSession destructor cleanup");

            {
                auto tempSession = BotSession::Create(88883);
                if (tempSession)
                {
                    TC_LOG_INFO("test.playerbot", "Created temporary session for destructor test");
                }
                // Destructor called here - should not crash due to guards
            }

            TC_LOG_INFO("test.playerbot", " All socket access paths tested without crash");

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception in path analysis: {}", e.what());
            TC_LOG_ERROR("test.playerbot", " Found problematic socket access path");
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception in path analysis");
            TC_LOG_ERROR("test.playerbot", " This is likely the ACCESS_VIOLATION crash point");
        }

        TC_LOG_INFO("test.playerbot", " TEST 5 COMPLETE");
    }

    /**
     * Helper function to analyze why IsBot() is returning false
     */
    static void AnalyzeIsBotFailure(std::shared_ptr<BotSession> const& botSession)
    {
        TC_LOG_INFO("test.playerbot", " ANALYZING IsBot() FAILURE");

        // Check if the BotSession class has the IsBot() override
        TC_LOG_INFO("test.playerbot", "BotSession class should override IsBot() to return true");

        // Check the WorldSession constructor call
        TC_LOG_INFO("test.playerbot", "Checking WorldSession constructor parameters...");

        // The BotSession constructor should call:
        // WorldSession(..., true) where the last parameter is the bot flag

        try {
            uint32 accountId = botSession->GetAccountId();
            TC_LOG_INFO("test.playerbot", "Bot session account ID: {}", accountId);

            if (accountId == 0) {
                TC_LOG_ERROR("test.playerbot", " Account ID is 0 - constructor might have failed");
            }

        } catch (std::exception const& e)
        {
            TC_LOG_ERROR("test.playerbot", " Exception checking account ID: {}", e.what());
        } catch (...)
        {
            TC_LOG_ERROR("test.playerbot", " Unknown exception checking account ID");
        }

        TC_LOG_INFO("test.playerbot", " IsBot() FAILURE ANALYSIS COMPLETE");
    }
};

/**
 * Main entry point for the IsBot() investigation
 */
void InvestigateIsBotImplementation()
{
    TC_LOG_INFO("test.playerbot", " STARTING IsBot() IMPLEMENTATION INVESTIGATION");
    TC_LOG_INFO("test.playerbot", " CRITICAL: If IsBot() returns false, ALL socket guards will fail");
    TC_LOG_INFO("test.playerbot", " This would directly cause ACCESS_VIOLATION at Socket.h:230");

    WorldSessionIsBotInvestigation::InvestigateIsBotImplementation();

    TC_LOG_INFO("test.playerbot", " IsBot() INVESTIGATION COMPLETE");
    TC_LOG_INFO("test.playerbot", " Check logs above for critical findings");
}