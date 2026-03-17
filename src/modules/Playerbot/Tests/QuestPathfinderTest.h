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

#ifndef _PLAYERBOT_QUEST_PATHFINDER_TEST_H
#define _PLAYERBOT_QUEST_PATHFINDER_TEST_H

#include "Common.h"
#include "QuestPathfinder.h"
#include <string>
#include <vector>

class Player;

namespace Playerbot
{
namespace Test
{

/**
 * @struct QuestPathfinderTestResult
 * @brief Results from a single QuestPathfinder test case
 */
struct QuestPathfinderTestResult
{
    ::std::string testName;
    bool passed = false;
    QuestPathfindingResult pathfindingResult = QuestPathfindingResult::SUCCESS;
    float pathLength = 0.0f;
    float estimatedTime = 0.0f;
    uint32 pathPointCount = 0;
    uint32 targetHubId = 0;
    ::std::string errorMessage;

    [[nodiscard]] ::std::string GetStatusString() const
    {
        return passed ? "PASS" : "FAIL";
    }
};

/**
 * @class QuestPathfinderTest
 * @brief Comprehensive test suite for QuestPathfinder functionality
 *
 * Test Coverage:
 * 1. Basic pathfinding to quest hub (level-appropriate)
 * 2. Pathfinding to specific quest giver
 * 3. Hub selection strategies (nearest, most quests, suitability score)
 * 4. Edge cases (no hubs, invalid player, destination reached)
 * 5. Performance testing (pathfinding latency)
 * 6. Multi-bot concurrent pathfinding
 *
 * Quality Standards:
 * - NO shortcuts - Full test coverage
 * - Enterprise-grade validation
 * - Production-ready test harness
 */
class QuestPathfinderTest
{
public:
    QuestPathfinderTest() = default;
    ~QuestPathfinderTest() = default;

    /**
     * @brief Runs all Quest Pathfinder test cases
     * @return true if all tests pass, false otherwise
     */
    bool RunAllTests();

    /**
     * @brief Test: Basic pathfinding to appropriate quest hub
     *
     * Creates a level 10 bot and verifies it can pathfind to a level 10 quest hub.
     *
     * Success Criteria:
     * - QuestHubDatabase returns at least 1 appropriate hub
     * - PathGenerator successfully calculates path
     * - Path length > 0 and < 5000 yards
     * - Movement initiated successfully
     *
     * @return Test result
     */
    QuestPathfinderTestResult TestBasicQuestHubPathfinding();

    /**
     * @brief Test: Pathfinding to specific quest giver creature
     *
     * Tests direct pathfinding to a quest giver GUID.
     *
     * Success Criteria:
     * - Quest giver creature found in world
     * - Path calculated successfully
     * - Path ends at quest giver position
     *
     * @return Test result
     */
    QuestPathfinderTestResult TestPathfindingToQuestGiver();

    /**
     * @brief Test: Hub selection strategy comparison
     *
     * Tests all 3 hub selection strategies:
     * - NEAREST_FIRST
     * - MOST_QUESTS_FIRST
     * - BEST_SUITABILITY_SCORE
     *
     * Success Criteria:
     * - Each strategy returns a valid hub
     * - NEAREST_FIRST returns closest hub
     * - MOST_QUESTS_FIRST returns hub with most quests
     * - BEST_SUITABILITY_SCORE returns balanced choice
     *
     * @return Test result
     */
    QuestPathfinderTestResult TestHubSelectionStrategies();

    /**
     * @brief Test: Edge case - No quest hubs available
     *
     * Tests pathfinding for max-level character (should have no quest hubs).
     *
     * Success Criteria:
     * - Result = NO_QUEST_HUBS_AVAILABLE
     * - No crash or exception
     * - Error logged appropriately
     *
     * @return Test result
     */
    QuestPathfinderTestResult TestNoQuestHubsAvailable();

    /**
     * @brief Test: Edge case - Already at destination
     *
     * Tests pathfinding when bot is already within interaction range.
     *
     * Success Criteria:
     * - Result = ALREADY_AT_DESTINATION
     * - No movement initiated
     *
     * @return Test result
     */
    QuestPathfinderTestResult TestAlreadyAtDestination();

    /**
     * @brief Test: Edge case - Invalid player
     *
     * Tests pathfinding with nullptr player.
     *
     * Success Criteria:
     * - Result = PLAYER_INVALID
     * - No crash or exception
     *
     * @return Test result
     */
    QuestPathfinderTestResult TestInvalidPlayer();

    /**
     * @brief Test: Performance - Pathfinding latency
     *
     * Measures pathfinding performance for 100 bots.
     *
     * Success Criteria:
     * - Average pathfinding time < 6ms
     * - 95th percentile < 10ms
     * - No crashes or exceptions
     *
     * @return Test result
     */
    QuestPathfinderTestResult TestPathfindingPerformance();

    /**
     * @brief Test: Concurrent pathfinding for multiple bots
     *
     * Tests 50 bots pathfinding simultaneously.
     *
     * Success Criteria:
     * - All bots successfully pathfind
     * - No race conditions or deadlocks
     * - No crashes
     *
     * @return Test result
     */
    QuestPathfinderTestResult TestConcurrentPathfinding();

    /**
     * @brief Generates comprehensive test report
     * @param results Vector of all test results
     * @return Formatted markdown report
     */
    ::std::string GenerateTestReport(::std::vector<QuestPathfinderTestResult> const& results);

private:
    /**
     * @brief Helper: Creates a test bot at specified level and position
     *
     * @param level Bot level (1-70)
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param mapId Map ID
     * @return Pointer to created bot (caller must delete)
     */
    Player* CreateTestBot(uint32 level, float x, float y, float z, uint32 mapId);

    /**
     * @brief Helper: Cleans up test bot
     * @param bot Bot to delete
     */
    void DestroyTestBot(Player* bot);

    /**
     * @brief Helper: Finds a quest giver creature in the world
     * @return Quest giver GUID, or empty GUID if none found
     */
    ObjectGuid FindTestQuestGiver();

    /**
     * @brief Helper: Validates path structure
     * @param state Pathfinding state to validate
     * @return true if path is valid
     */
    bool ValidatePath(QuestPathfindingState const& state);
};

} // namespace Test
} // namespace Playerbot

#endif // _PLAYERBOT_QUEST_PATHFINDER_TEST_H
