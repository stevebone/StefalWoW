/*
 * Copyright (C) 2024 TrinityCore Playerbot Module
 *
 * Threading Stress Test Header
 */

#ifndef MODULE_PLAYERBOT_THREADING_STRESS_TEST_H
#define MODULE_PLAYERBOT_THREADING_STRESS_TEST_H

#include "Common.h"
#include <atomic>
#include <chrono>
#include <string>

namespace Playerbot
{
namespace Testing
{

class ThreadingStressTest
{
public:
    struct TestConfig
    {
        uint32 numBots{1000};
        uint32 numThreads{16};
        uint32 testDurationSeconds{60};
        uint32 spawnRate{10};
        uint32 updateFrequencyMs{50};
        bool enableChaosMode{false};
        bool enableDeadlockDetection{true};
        bool enableContentionAnalysis{true};
    };

    struct TestResults
    {
        // Performance metrics
        uint64 totalUpdates{0};
        uint64 totalSpawns{0};
        uint64 totalDespawns{0};
        double averageUpdateTimeMs{0};
        double maxUpdateTimeMs{0};
        double p99UpdateTimeMs{0};

        // Concurrency metrics
        uint32 deadlocksDetected{0};
        uint32 racesDetected{0};
        uint32 contentionEvents{0};
        double averageLockWaitMs{0};
        double maxLockWaitMs{0};

        // Scalability metrics
        double throughputOpsPerSec{0};
        double cpuUtilization{0};
        uint64 memoryUsedMB{0};
        double scalabilityFactor{0};

        // Error counts
        uint32 failedSpawns{0};
        uint32 crashCount{0};
        uint32 assertionFailures{0};

        bool TestPassed() const;
        ::std::string GetSummary() const;
    };

    // Main test entry points
    static TestResults RunStressTest(TestConfig const& config);
    static TestResults RunDeadlockTest();
    static TestResults RunRaceConditionTest();
    static TestResults RunScalabilityTest(uint32 minBots, uint32 maxBots);
    static TestResults RunContentionTest();

    // Console commands for manual testing
    static void RunConsoleCommand(::std::string const& command);
};

} // namespace Testing
} // namespace Playerbot

#endif // MODULE_PLAYERBOT_THREADING_STRESS_TEST_H