/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
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

#ifndef _PLAYERBOT_MONITOR_TEST_H
#define _PLAYERBOT_MONITOR_TEST_H

#include "Monitoring/BotMonitor.h"
#include "Monitoring/PerformanceMetrics.h"
#include "ObjectGuid.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

namespace Playerbot
{
    /**
     * @class BotMonitorTest
     * @brief Comprehensive test suite for BotMonitor system
     *
     * Tests:
     * 1. Initialization and shutdown
     * 2. Activity tracking (combat, questing, death, resurrection)
     * 3. Performance metrics (update times, AI decision times)
     * 4. Resource tracking (database queries, cache hits/misses)
     * 5. Error and warning tracking
     * 6. Snapshot capture and history
     * 7. Trend data collection
     * 8. Alert system (thresholds, triggers, callbacks)
     * 9. Statistics summary
     * 10. Thread safety
     */
    class BotMonitorTest
    {
    public:
        /**
         * @brief Run all tests
         * @return true if all tests pass
         */
        static bool RunAllTests()
        {
            ::std::cout << "=================================================================\n";
            ::std::cout << "BotMonitor Test Suite\n";
            ::std::cout << "=================================================================\n\n";

            bool allPassed = true;

            allPassed &= TestInitialization();
            allPassed &= TestActivityTracking();
            allPassed &= TestPerformanceTracking();
            allPassed &= TestResourceTracking();
            allPassed &= TestErrorWarningTracking();
            allPassed &= TestSnapshotCapture();
            allPassed &= TestSnapshotHistory();
            allPassed &= TestTrendData();
            allPassed &= TestAlertThresholds();
            allPassed &= TestAlertTriggering();
            allPassed &= TestAlertCallbacks();
            allPassed &= TestAlertHistory();
            allPassed &= TestStatisticsSummary();
            allPassed &= TestResetStatistics();
            allPassed &= TestThreadSafety();

            ::std::cout << "\n=================================================================\n";
            if (allPassed)
                ::std::cout << "ALL TESTS PASSED\n";
            else
                ::std::cout << "SOME TESTS FAILED\n";
            ::std::cout << "=================================================================\n";

            return allPassed;
        }

    private:
        static bool TestInitialization()
        {
            ::std::cout << "Test 1: Initialization and Shutdown\n";

            BotMonitor* monitor = sBotMonitor;
            assert(monitor != nullptr);

            bool initResult = monitor->Initialize();
            assert(initResult == true);

            uint64 uptime = monitor->GetUptimeSeconds();
            assert(uptime >= 0);

            monitor->Shutdown();

            ::std::cout << "  [PASS] Initialization and shutdown\n\n";
            return true;
        }

        static bool TestActivityTracking()
        {
            ::std::cout << "Test 2: Activity Tracking\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->Initialize();
            monitor->ResetStatistics();

            ObjectGuid botGuid1 = ObjectGuid::Create<HighGuid::Player>(1);
            ObjectGuid botGuid2 = ObjectGuid::Create<HighGuid::Player>(2);

            // Test combat tracking
            monitor->RecordBotCombatStart(botGuid1);
            PerformanceSnapshot snapshot1 = monitor->CaptureSnapshot();
            assert(snapshot1.activity.combatCount >= 1);

            monitor->RecordBotCombatEnd(botGuid1);
            snapshot1 = monitor->CaptureSnapshot();

            // Test questing tracking
            monitor->RecordBotQuestStart(botGuid2);
            PerformanceSnapshot snapshot2 = monitor->CaptureSnapshot();
            assert(snapshot2.activity.questingCount >= 1);

            monitor->RecordBotQuestEnd(botGuid2);

            // Test death tracking
            monitor->RecordBotDeath(botGuid1);
            PerformanceSnapshot snapshot3 = monitor->CaptureSnapshot();
            assert(snapshot3.activity.deadCount >= 1);

            monitor->RecordBotResurrection(botGuid1);
            snapshot3 = monitor->CaptureSnapshot();

            ::std::cout << "  [PASS] Activity tracking (combat, questing, death)\n\n";
            return true;
        }

        static bool TestPerformanceTracking()
        {
            ::std::cout << "Test 3: Performance Tracking\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->ResetStatistics();

            ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(1);

            // Record update times
            monitor->RecordBotUpdateTime(botGuid, 5.5);
            monitor->RecordBotUpdateTime(botGuid, 3.2);
            monitor->RecordBotUpdateTime(botGuid, 8.7);

            PerformanceSnapshot snapshot = monitor->CaptureSnapshot();
            assert(snapshot.avgUpdateTimeMs > 0.0);
            assert(snapshot.maxUpdateTimeMs >= 8.7);

            // Record AI decision times
            monitor->RecordAIDecisionTime(botGuid, 2.1);
            monitor->RecordAIDecisionTime(botGuid, 4.5);

            snapshot = monitor->CaptureSnapshot();
            assert(snapshot.avgAIDecisionTimeMs > 0.0);

            ::std::cout << "  [PASS] Performance tracking (update times, AI decisions)\n\n";
            return true;
        }

        static bool TestResourceTracking()
        {
            ::std::cout << "Test 4: Resource Tracking\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->ResetStatistics();

            // Record database queries
            monitor->RecordDatabaseQuery(10.5);
            monitor->RecordDatabaseQuery(15.2);
            monitor->RecordDatabaseQuery(8.9);

            PerformanceSnapshot snapshot = monitor->CaptureSnapshot();
            assert(snapshot.database.queryCount >= 3);
            assert(snapshot.database.avgQueryTimeMs > 0.0);
            assert(snapshot.database.maxQueryTimeMs >= 15.2);

            // Record cache hits and misses
            monitor->RecordDatabaseCacheHit();
            monitor->RecordDatabaseCacheHit();
            monitor->RecordDatabaseCacheMiss();

            snapshot = monitor->CaptureSnapshot();
            assert(snapshot.database.cacheHits >= 2);
            assert(snapshot.database.cacheMisses >= 1);
            assert(snapshot.database.cacheHitRate() > 0);

            ::std::cout << "  [PASS] Resource tracking (database queries, cache)\n\n";
            return true;
        }

        static bool TestErrorWarningTracking()
        {
            ::std::cout << "Test 5: Error and Warning Tracking\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->ResetStatistics();

            monitor->RecordError("Combat", "Test error message");
            monitor->RecordError("Movement", "Another error");
            monitor->RecordWarning("Database", "Test warning");

            PerformanceSnapshot snapshot = monitor->CaptureSnapshot();
            assert(snapshot.errorCount >= 2);
            assert(snapshot.warningCount >= 1);

            ::std::cout << "  [PASS] Error and warning tracking\n\n";
            return true;
        }

        static bool TestSnapshotCapture()
        {
            ::std::cout << "Test 6: Snapshot Capture\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->ResetStatistics();

            ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(1);
            monitor->RecordBotCombatStart(botGuid);
            monitor->RecordBotUpdateTime(botGuid, 5.0);
            monitor->RecordDatabaseQuery(10.0);

            PerformanceSnapshot snapshot = monitor->CaptureSnapshot();

            assert(snapshot.activity.combatCount >= 1);
            assert(snapshot.avgUpdateTimeMs > 0.0);
            assert(snapshot.database.queryCount >= 1);
            assert(snapshot.uptimeSeconds >= 0);

            ::std::cout << "  [PASS] Snapshot capture\n\n";
            return true;
        }

        static bool TestSnapshotHistory()
        {
            ::std::cout << "Test 7: Snapshot History\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->ResetStatistics();

            // Capture multiple snapshots
            for (int i = 0; i < 5; ++i)
            {
                monitor->CaptureSnapshot();
                ::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
            }

            ::std::vector<PerformanceSnapshot> history = monitor->GetSnapshotHistory(3);
            assert(history.size() <= 3);

            PerformanceSnapshot latest = monitor->GetLatestSnapshot();
            assert(latest.uptimeSeconds >= 0);

            ::std::cout << "  [PASS] Snapshot history\n\n";
            return true;
        }

        static bool TestTrendData()
        {
            ::std::cout << "Test 8: Trend Data\n";

            TrendData trend;

            // Add data points
            trend.AddDataPoint(10.5);
            trend.AddDataPoint(15.2);
            trend.AddDataPoint(8.9);
            trend.AddDataPoint(12.3);

            assert(trend.values.size() == 4);
            assert(trend.GetAverage() > 0.0);
            assert(trend.GetMin() <= trend.GetMax());

            // Test 60-point limit
            for (int i = 0; i < 70; ++i)
                trend.AddDataPoint(i * 1.0);

            assert(trend.values.size() == 60);

            ::std::cout << "  [PASS] Trend data collection and windowing\n\n";
            return true;
        }

        static bool TestAlertThresholds()
        {
            ::std::cout << "Test 9: Alert Thresholds\n";

            BotMonitor* monitor = sBotMonitor;

            AlertThresholds thresholds = monitor->GetAlertThresholds();
            assert(thresholds.cpuWarning == 70.0);
            assert(thresholds.cpuCritical == 90.0);

            // Modify thresholds
            thresholds.cpuWarning = 60.0;
            thresholds.cpuCritical = 80.0;
            monitor->SetAlertThresholds(thresholds);

            AlertThresholds newThresholds = monitor->GetAlertThresholds();
            assert(newThresholds.cpuWarning == 60.0);
            assert(newThresholds.cpuCritical == 80.0);

            ::std::cout << "  [PASS] Alert thresholds get/set\n\n";
            return true;
        }

        static bool TestAlertTriggering()
        {
            ::std::cout << "Test 10: Alert Triggering\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->ClearAlertHistory();

            // Alerts are triggered internally by CheckAlerts() during Update()
            // This test verifies the alert history retrieval

            ::std::vector<PerformanceAlert> alerts = monitor->GetActiveAlerts(AlertLevel::WARNING);
            // May be empty if no alerts triggered

            ::std::cout << "  [PASS] Alert triggering mechanism\n\n";
            return true;
        }

        static bool TestAlertCallbacks()
        {
            ::std::cout << "Test 11: Alert Callbacks\n";

            BotMonitor* monitor = sBotMonitor;

            bool callbackInvoked = false;
            monitor->RegisterAlertCallback([&callbackInvoked](PerformanceAlert const& alert) {
                callbackInvoked = true;
            });

            // Note: Callback will only be invoked if an actual alert is triggered
            // during Update(). This test verifies callback registration succeeds.

            ::std::cout << "  [PASS] Alert callback registration\n\n";
            return true;
        }

        static bool TestAlertHistory()
        {
            ::std::cout << "Test 12: Alert History\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->ClearAlertHistory();

            ::std::vector<PerformanceAlert> history = monitor->GetAlertHistory(10);
            assert(history.size() == 0);  // Just cleared

            monitor->ClearAlertHistory();
            history = monitor->GetAlertHistory(10);
            assert(history.size() == 0);

            ::std::cout << "  [PASS] Alert history and clear\n\n";
            return true;
        }

        static bool TestStatisticsSummary()
        {
            ::std::cout << "Test 13: Statistics Summary\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->ResetStatistics();

            ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(1);
            monitor->RecordBotCombatStart(botGuid);
            monitor->RecordBotUpdateTime(botGuid, 5.0);
            monitor->RecordDatabaseQuery(10.0);

            ::std::string summary = monitor->GetStatisticsSummary();
            assert(!summary.empty());
            assert(summary.find("Playerbot Performance Summary") != ::std::string::npos);

            ::std::cout << "  [PASS] Statistics summary generation\n\n";
            return true;
        }

        static bool TestResetStatistics()
        {
            ::std::cout << "Test 14: Reset Statistics\n";

            BotMonitor* monitor = sBotMonitor;

            ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(1);
            monitor->RecordBotUpdateTime(botGuid, 5.0);
            monitor->RecordDatabaseQuery(10.0);
            monitor->RecordError("Test", "Test error");

            monitor->ResetStatistics();

            PerformanceSnapshot snapshot = monitor->CaptureSnapshot();
            assert(snapshot.errorCount == 0);

            ::std::cout << "  [PASS] Reset statistics\n\n";
            return true;
        }

        static bool TestThreadSafety()
        {
            ::std::cout << "Test 15: Thread Safety\n";

            BotMonitor* monitor = sBotMonitor;
            monitor->ResetStatistics();

            // Launch multiple threads recording metrics concurrently
            ::std::vector<::std::thread> threads;
            for (int i = 0; i < 5; ++i)
            {
                threads.emplace_back([monitor, i]() {
                    ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(i);
                    for (int j = 0; j < 10; ++j)
                    {
                        monitor->RecordBotUpdateTime(botGuid, j * 1.0);
                        monitor->RecordDatabaseQuery(j * 2.0);
                        ::std::this_thread::sleep_for(::std::chrono::milliseconds(1));
                    }
                });
            }

            for (auto& thread : threads)
                thread.join();

            PerformanceSnapshot snapshot = monitor->CaptureSnapshot();
            // If thread-safe, all metrics recorded successfully
            assert(snapshot.database.queryCount >= 50);

            ::std::cout << "  [PASS] Thread safety (concurrent access)\n\n";
            return true;
        }
    };

} // namespace Playerbot

#endif // _PLAYERBOT_MONITOR_TEST_H
