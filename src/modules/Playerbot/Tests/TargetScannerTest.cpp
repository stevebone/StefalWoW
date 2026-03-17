/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Player.h"
#include "Creature.h"
#include "Log.h"
#include "../AI/Combat/TargetScanner.h"

namespace Playerbot
{
    namespace Tests
    {
        /**
         * Test harness for TargetScanner functionality
         *
         * Tests:
         * 1. Solo bot finding nearest hostile
         * 2. Range-based scanning for different classes
         * 3. Target priority system
         * 4. Blacklist functionality
         * 5. Performance with multiple targets
         */
        class TargetScannerTest
        {
        public:
            static void RunAllTests(Player* testBot)
            {

                TC_LOG_INFO("playerbot.test", "=== Starting TargetScanner Tests ===");

                TestBasicScanning(testBot);
                TestRangeByClass(testBot);
                TestTargetPriority(testBot);
                TestBlacklist(testBot);
                TestScanModes(testBot);

                TC_LOG_INFO("playerbot.test", "=== TargetScanner Tests Complete ===");
            }

        private:
            static void TestBasicScanning(Player* bot)
            {
                TC_LOG_INFO("playerbot.test", "Test 1: Basic Hostile Scanning");

                TargetScanner scanner(bot);

                // Test with default range
                Unit* nearestHostile = scanner.FindNearestHostile();
                if (nearestHostile)
                {
                    TC_LOG_INFO("playerbot.test",
                        " Found hostile: {} at distance {:.1f}",
                        nearestHostile->GetName(),
                        bot->GetDistance(nearestHostile));
                }
                else
                {
                    TC_LOG_INFO("playerbot.test", " No hostile found within scan range");
                }

                // Test with custom range
                Unit* closeHostile = scanner.FindNearestHostile(10.0f);
                if (closeHostile)
                {
                    TC_LOG_INFO("playerbot.test",
                        " Found close hostile within 10 yards: {}",
                        closeHostile->GetName());
                }

                // Test best target selection
                Unit* bestTarget = scanner.FindBestTarget();
                if (bestTarget)
                {
                    TC_LOG_INFO("playerbot.test",
                        " Best target selected: {} (priority: {})",
                        bestTarget->GetName(),
                        scanner.GetTargetPriority(bestTarget));
                }
            }

            static void TestRangeByClass(Player* bot)
            {
                TC_LOG_INFO("playerbot.test", "Test 2: Class-Based Range Testing");

                TargetScanner scanner(bot);
                float scanRadius = scanner.GetScanRadius();
                float maxRange = scanner.GetMaxEngageRange();

                TC_LOG_INFO("playerbot.test",
                    "Bot class {} - Scan radius: {:.1f}, Max engage: {:.1f}",
                    bot->GetClass(), scanRadius, maxRange);

                // Verify ranges make sense for class
                bool rangeCorrect = false;
                switch (bot->GetClass())
                {
                    case CLASS_HUNTER:
                        rangeCorrect = (scanRadius >= 30.0f && maxRange >= 40.0f);
                        break;
                    case CLASS_MAGE:
                    case CLASS_WARLOCK:
                        rangeCorrect = (scanRadius >= 25.0f && maxRange >= 35.0f);
                        break;
                    case CLASS_WARRIOR:
                    case CLASS_ROGUE:
                        rangeCorrect = (scanRadius <= 20.0f);
                        break;
                    default:
                        rangeCorrect = true; // Accept default ranges
                        break;
                }

                if (rangeCorrect)
                {
                    TC_LOG_INFO("playerbot.test", " Class ranges configured correctly");
                }
                else
                {
                    TC_LOG_ERROR("playerbot.test", " Class ranges seem incorrect");
                }
            }

            static void TestTargetPriority(Player* bot)
            {
                TC_LOG_INFO("playerbot.test", "Test 3: Target Priority System");

                TargetScanner scanner(bot);
                ::std::vector<Unit*> allHostiles = scanner.FindAllHostiles();

                TC_LOG_INFO("playerbot.test", "Found {} hostile targets", allHostiles.size());

                // Check priority for each target
    for (Unit* hostile : allHostiles)
                {
                    uint8 priority = scanner.GetTargetPriority(hostile);
                    float threat = scanner.GetThreatValue(hostile);
                    bool shouldEngage = scanner.ShouldEngage(hostile);

                    TC_LOG_DEBUG("playerbot.test",
                        "Target: {} - Priority: {}, Threat: {:.1f}, Engage: {}",
                        hostile->GetName(),
                        priority,
                        threat,
                        shouldEngage ? "YES" : "NO");
                }

                if (!allHostiles.empty())
                {
                    TC_LOG_INFO("playerbot.test", " Priority system evaluated {} targets",
                        allHostiles.size());
                }
            }

            static void TestBlacklist(Player* bot)
            {
                TC_LOG_INFO("playerbot.test", "Test 4: Blacklist Functionality");

                TargetScanner scanner(bot);

                // Find a target to blacklist
                Unit* target = scanner.FindNearestHostile();
                if (target)
                {
                    ObjectGuid guid = target->GetGUID();
                    // Add to blacklist
                    scanner.AddToBlacklist(guid, 5000); // 5 second blacklist

                    // Verify it's blacklisted
    if (scanner.IsBlacklisted(guid))
                    {
                        TC_LOG_INFO("playerbot.test", " Target successfully blacklisted");
                    }
                    else
                    {
                        TC_LOG_ERROR("playerbot.test", " Blacklist add failed");
                    }

                    // Verify blacklisted target is not returned
                    Unit* newTarget = scanner.FindNearestHostile();
                    if (newTarget && newTarget->GetGUID() != guid)
                    {
                        TC_LOG_INFO("playerbot.test",
                            " Blacklisted target ignored, found alternative: {}",
                            newTarget->GetName());
                    }

                    // Remove from blacklist
                    scanner.RemoveFromBlacklist(guid);
                    if (!scanner.IsBlacklisted(guid))
                    {
                        TC_LOG_INFO("playerbot.test", " Target removed from blacklist");
                    }
                }
            }

            static void TestScanModes(Player* bot)
            {
                TC_LOG_INFO("playerbot.test", "Test 5: Scan Mode Testing");

                TargetScanner scanner(bot);

                // Test passive mode
                scanner.SetScanMode(ScanMode::PASSIVE);
                Unit* passiveTarget = scanner.FindNearestHostile();
                if (!passiveTarget)
                {
                    TC_LOG_INFO("playerbot.test", " Passive mode correctly returns no targets");
                }
                else
                {
                    TC_LOG_ERROR("playerbot.test", " Passive mode returned target when it shouldn't");
                }

                // Test defensive mode
                scanner.SetScanMode(ScanMode::DEFENSIVE);
                Unit* defensiveTarget = scanner.FindNearestHostile();
                TC_LOG_INFO("playerbot.test",
                    "Defensive mode: {} targets",
                    defensiveTarget ? "found" : "no");

                // Test aggressive mode
                scanner.SetScanMode(ScanMode::AGGRESSIVE);
                Unit* aggressiveTarget = scanner.FindNearestHostile();
                TC_LOG_INFO("playerbot.test",
                    "Aggressive mode: {} targets",
                    aggressiveTarget ? "found" : "no");

                TC_LOG_INFO("playerbot.test", " Scan modes tested");
            }
        };
    }
}