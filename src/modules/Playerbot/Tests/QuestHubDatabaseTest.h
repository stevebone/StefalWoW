/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include <gtest/gtest.h>

namespace Playerbot
{
namespace Test
{

/**
 * @class QuestHubDatabaseTestRunner
 * @brief Test runner for Quest Hub Database comprehensive tests
 *
 * This class provides a comprehensive test suite for the Quest Hub Database system,
 * covering:
 * - Quest hub structure validation (IsAppropriateFor, GetDistanceFrom, etc.)
 * - Database singleton behavior
 * - Query operations (GetQuestHubById, GetNearestQuestHub, GetQuestHubsForPlayer)
 * - Thread safety with concurrent reads
 * - Performance benchmarks (< 0.5ms query time, < 2MB memory usage)
 * - Edge cases and error handling
 *
 * Performance Targets (from Phase 1.1 requirements):
 * - Query time: < 0.5ms per GetNearestQuestHub call
 * - Memory usage: < 2MB for ~500 quest hubs
 * - Thread-safe concurrent read access
 * - Hash table lookup: O(1) ~50ns
 * - Zone filtering: O(n) ~0.2ms
 *
 * Test Coverage:
 * -  QuestHub::IsAppropriateFor() - level and faction filtering
 * -  QuestHub::GetDistanceFrom() - distance calculation accuracy
 * -  QuestHub::ContainsPosition() - radius boundary checks
 * -  QuestHub::CalculateSuitabilityScore() - scoring algorithm
 * -  QuestHubDatabase::Instance() - singleton pattern
 * -  QuestHubDatabase::Initialize() - database loading
 * -  QuestHubDatabase::GetQuestHubById() - O(1) hash lookup
 * -  QuestHubDatabase::GetNearestQuestHub() - spatial query
 * -  QuestHubDatabase::GetQuestHubsForPlayer() - filtered query
 * -  QuestHubDatabase::GetQuestHubsInZone() - zone-based filtering
 * -  QuestHubDatabase::GetQuestHubAtPosition() - position-based query
 * -  Thread safety - concurrent read operations
 * -  Performance benchmarks - < 0.5ms query time
 * -  Memory usage - < 2MB target verification
 */
class QuestHubDatabaseTestRunner
{
public:
    /**
     * @brief Run all Quest Hub Database tests
     * @return True if all tests pass
     */
    static bool RunAllTests();

    /**
     * @brief Run structure validation tests
     * @return True if all structure tests pass
     */
    static bool RunStructureTests();

    /**
     * @brief Run database singleton tests
     * @return True if all singleton tests pass
     */
    static bool RunSingletonTests();

    /**
     * @brief Run query operation tests
     * @return True if all query tests pass
     */
    static bool RunQueryTests();

    /**
     * @brief Run thread safety tests
     * @return True if all thread safety tests pass
     */
    static bool RunThreadSafetyTests();

    /**
     * @brief Run performance benchmark tests
     * @return True if performance targets are met
     */
    static bool RunPerformanceTests();

    /**
     * @brief Run edge case and error handling tests
     * @return True if all edge case tests pass
     */
    static bool RunEdgeCaseTests();

    /**
     * @brief Generate test report with performance metrics
     * @return Test report as string
     */
    static ::std::string GenerateTestReport();

    /**
     * @brief Validate Quest Hub Database meets Phase 1.1 requirements
     * @return True if all Phase 1.1 requirements are met
     *
     * Phase 1.1 Requirements Validation:
     * -  Query time < 0.5ms per GetNearestQuestHub
     * -  Memory usage < 2MB for ~500 hubs
     * -  Thread-safe concurrent reads
     * -  O(1) hash table lookup (~50ns)
     * -  Zone filtering O(n) (~0.2ms)
     * -  Spatial indexing functional
     * -  Faction and level filtering working
     * -  DBSCAN clustering validation
     */
    static bool ValidatePhase1Requirements();

private:
    struct TestMetrics
    {
        uint32 totalTests = 0;
        uint32 passedTests = 0;
        uint32 failedTests = 0;
        float avgQueryTimeMicros = 0.0f;
        size_t memoryUsageBytes = 0;
        uint32 concurrentReadsCount = 0;
    };

    static TestMetrics s_testMetrics;
};

} // namespace Test
} // namespace Playerbot
