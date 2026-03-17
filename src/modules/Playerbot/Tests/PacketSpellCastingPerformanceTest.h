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

#ifndef _PLAYERBOT_PACKET_SPELL_CASTING_PERFORMANCE_TEST_H
#define _PLAYERBOT_PACKET_SPELL_CASTING_PERFORMANCE_TEST_H

#include "PerformanceValidator.h"
#include "TestUtilities.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <fstream>

namespace Playerbot
{
namespace Test
{

/**
 * @struct Week4TestMetrics
 * @brief Comprehensive metrics for Week 4 testing scenarios
 *
 * Captures all performance data required for validating packet-based
 * spell casting at scale (100, 500, 1000, 5000 bots).
 */
struct Week4TestMetrics
{
    // Test identification
    ::std::string testName;
    ::std::string scenario;
    uint32 botCount;
    ::std::chrono::system_clock::time_point startTime;
    ::std::chrono::system_clock::time_point endTime;

    // CPU metrics (%)
    float avgCpuUsage = 0.0f;
    float peakCpuUsage = 0.0f;
    float cpuPerBot = 0.0f;           // Average CPU per bot

    // Memory metrics (bytes)
    uint64 initialMemory = 0;
    uint64 peakMemory = 0;
    uint64 finalMemory = 0;
    uint64 memoryGrowth = 0;          // Final - Initial
    uint64 avgMemoryPerBot = 0;       // Average memory per bot

    // Spell casting metrics
    uint64 totalSpellCasts = 0;
    uint64 successfulCasts = 0;
    uint64 failedCasts = 0;
    float spellCastSuccessRate = 0.0f;

    // Latency metrics (microseconds)
    uint64 avgSpellCastLatency = 0;   // Time from queue to execution
    uint64 minSpellCastLatency = UINT64_MAX;
    uint64 maxSpellCastLatency = 0;
    uint64 p95SpellCastLatency = 0;   // 95th percentile
    uint64 p99SpellCastLatency = 0;   // 99th percentile

    // Packet queue metrics
    uint32 avgQueueDepth = 0;         // Average packets in queue
    uint32 maxQueueDepth = 0;         // Peak queue depth
    uint64 avgPacketProcessTime = 0;  // Time to process one packet (microseconds)
    uint64 maxPacketProcessTime = 0;  // Longest packet process time

    // Main thread metrics
    uint64 avgMainThreadCycleTime = 0; // Average World::Update() time (microseconds)
    uint64 maxMainThreadCycleTime = 0; // Longest blocking time
    uint32 mainThreadBlockingEvents = 0; // Count of >5ms blocking

    // Server metrics
    float avgTicksPerSecond = 0.0f;   // Server TPS
    float minTicksPerSecond = 0.0f;   // Minimum TPS during test
    uint64 uptime = 0;                // Total uptime (seconds)
    uint32 crashCount = 0;            // Number of crashes

    // Bot behavior metrics
    uint32 botsInCombat = 0;          // Bots actively fighting
    uint32 botsIdle = 0;              // Bots not engaged
    uint32 botsDead = 0;              // Dead bots
    uint32 botsResurrected = 0;       // Successful resurrections

    // Error metrics
    uint32 validationErrors = 0;      // Spell validation failures
    uint32 packetDrops = 0;           // Packets dropped (queue full)
    uint32 deadlocks = 0;             // Detected deadlocks
    uint32 memoryLeaks = 0;           // Memory leak incidents

    // Calculated metrics
    uint64 GetDuration() const
    {
        return ::std::chrono::duration_cast<::std::chrono::seconds>(endTime - startTime).count();
    }

    bool MeetsPerformanceTargets() const;
    ::std::string GenerateReport() const;
    void ExportToCSV(::std::string const& filename) const;
};

/**
 * @struct Week4TestScenario
 * @brief Configuration for a Week 4 test scenario
 */
struct Week4TestScenario
{
    ::std::string name;
    ::std::string description;
    uint32 botCount;
    uint32 durationMinutes;

    // Spawn configuration
    bool spawnGradually;              // Gradual spawn vs instant
    uint32 spawnIntervalSeconds;      // Time between spawns
    ::std::vector<::std::string> zones;   // Zones to spawn bots in

    // Activity configuration
    bool enableCombat;                // Engage in combat
    bool enableQuesting;              // Perform quests
    bool enableMovement;              // Move around world
    bool enableSpellCasting;          // Cast spells

    // Monitoring configuration
    uint32 metricSampleIntervalSeconds; // How often to sample metrics
    bool enableDetailedLogging;       // Log every spell cast
    bool enableMemoryProfiling;       // Track memory allocations

    // Success criteria
    float minSpellCastSuccessRate;    // Minimum 99% success
    float maxCpuPerBot;               // Maximum 0.1% per bot
    uint64 maxMemoryPerBot;           // Maximum 10MB per bot
    uint64 maxSpellCastLatency;       // Maximum 10ms latency
    uint32 maxMainThreadBlocking;     // Maximum 5ms blocking
};

/**
 * @class Week4PerformanceTest
 * @brief Enterprise-grade performance testing framework for Week 4 validation
 *
 * Purpose:
 * - Validate packet-based spell casting at scale (100, 500, 1000, 5000 bots)
 * - Measure CPU, memory, latency, and throughput metrics
 * - Generate comprehensive performance reports
 * - Identify bottlenecks and optimization opportunities
 *
 * Architecture:
 * - Scenario-based testing (5 scenarios from roadmap)
 * - Real-time metric collection via sampling
 * - CSV export for graphing and analysis
 * - Pass/fail criteria validation
 *
 * Quality Standards:
 * - NO shortcuts - Full metric collection
 * - Enterprise-grade reporting
 * - Production-ready validation logic
 */
class Week4PerformanceTest
{
public:
    /**
     * @brief Construct performance test framework
     */
    Week4PerformanceTest();
    ~Week4PerformanceTest() = default;

    /**
     * @brief Run all Week 4 test scenarios
     * @return true if all scenarios pass, false otherwise
     */
    bool RunAllScenarios();

    /**
     * @brief Run a specific scenario by index
     * @param scenarioIndex Index of scenario (0-4)
     * @return Metrics from test execution
     */
    Week4TestMetrics RunScenario(uint32 scenarioIndex);

    /**
     * @brief Run custom scenario
     * @param scenario Custom scenario configuration
     * @return Metrics from test execution
     */
    Week4TestMetrics RunScenario(Week4TestScenario const& scenario);

    /**
     * @brief Generate comprehensive test report from all scenarios
     * @param outputPath Path to save report
     */
    void GenerateComprehensiveReport(::std::string const& outputPath);

    /**
     * @brief Get predefined Week 4 scenarios from roadmap
     * @return Vector of 5 scenarios (100, 500, 1000, 5000 bots, 24-hour)
     */
    static ::std::vector<Week4TestScenario> GetWeek4Scenarios();

private:
    // Scenario execution
    Week4TestMetrics ExecuteScenario(Week4TestScenario const& scenario);

    // Bot management
    void SpawnBots(uint32 count, Week4TestScenario const& scenario);
    void DespawnAllBots();
    void ConfigureBotBehavior(Week4TestScenario const& scenario);

    // Metric collection
    void StartMetricCollection(Week4TestScenario const& scenario);
    void StopMetricCollection();
    void SampleMetrics();

    // CPU monitoring
    float GetCurrentCpuUsage();
    float GetBotSystemCpuUsage();

    // Memory monitoring
    uint64 GetCurrentMemoryUsage();
    uint64 GetBotSystemMemoryUsage();

    // Spell casting monitoring
    void TrackSpellCast(bool success, uint64 latencyMicroseconds);

    // Packet queue monitoring
    uint32 GetCurrentQueueDepth();
    uint64 GetAveragePacketProcessTime();

    // Main thread monitoring
    uint64 GetLastMainThreadCycleTime();
    bool IsMainThreadBlocking();

    // Server monitoring
    float GetCurrentTickRate();
    uint64 GetServerUptime();

    // Bot state monitoring
    void UpdateBotStates();

    // Reporting
    ::std::string FormatDuration(uint64 seconds) const;
    ::std::string FormatBytes(uint64 bytes) const;
    ::std::string FormatMicroseconds(uint64 microseconds) const;

    // Data members
    ::std::vector<Week4TestMetrics> _allResults;
    Week4TestMetrics _currentMetrics;
    ::std::chrono::system_clock::time_point _testStartTime;
    ::std::chrono::system_clock::time_point _lastSampleTime;

    // Metric sample history (for percentile calculations)
    ::std::vector<uint64> _spellCastLatencySamples;
    ::std::vector<uint32> _queueDepthSamples;
    ::std::vector<uint64> _mainThreadCycleSamples;
    ::std::vector<float> _cpuUsageSamples;
    ::std::vector<uint64> _memoryUsageSamples;

    // Configuration
    bool _metricsCollectionActive;
    uint32 _sampleInterval;
};

} // namespace Test
} // namespace Playerbot

#endif // _PLAYERBOT_PACKET_SPELL_CASTING_PERFORMANCE_TEST_H
