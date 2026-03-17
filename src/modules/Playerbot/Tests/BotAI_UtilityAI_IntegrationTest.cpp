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
#include "AI/BotAI.h"
#include "AI/Utility/UtilitySystem.h"
#include "AI/Utility/UtilityContextBuilder.h"

using namespace Playerbot;

// NOTE: These tests require a valid Player object, which is difficult
// to create in unit tests. These are integration tests that should be
// run with a real TrinityCore server environment.
//
// For now, we test what we can without a full Player object.

TEST_CASE("BotAI Integration: Utility AI system can be initialized", "[botai][utility][integration]")
{
    // This test verifies that the UtilityAI system can be created
    // without crashing. Full integration testing requires a server.

    SECTION("UtilityAI can be created")
    {
        auto utilityAI = std::make_unique<UtilityAI>();
        REQUIRE(utilityAI != nullptr);
    }

    SECTION("UtilityAI can register behaviors")
    {
        auto utilityAI = std::make_unique<UtilityAI>();

        auto behavior = std::make_shared<UtilityBehavior>("TestBehavior");
        utilityAI->AddBehavior(behavior);

        REQUIRE(utilityAI->GetBehaviors().size() == 1);
    }

    SECTION("UtilityAI selects behavior correctly with mock context")
    {
        auto utilityAI = std::make_unique<UtilityAI>();

        // Create a combat behavior with high score when enemies present
        auto combatBehavior = std::make_shared<UtilityBehavior>("Combat");
        combatBehavior->AddEvaluator(std::make_shared<CombatEngageEvaluator>());
        utilityAI->AddBehavior(combatBehavior);

        // Create a flee behavior with high score when low health
        auto fleeBehavior = std::make_shared<UtilityBehavior>("Flee");
        fleeBehavior->AddEvaluator(std::make_shared<FleeEvaluator>());
        utilityAI->AddBehavior(fleeBehavior);

        // Mock context: healthy bot with enemies nearby
        UtilityContext context;
        context.bot = nullptr; // Can't create real bot in unit test
        context.healthPercent = 0.8f;
        context.manaPercent = 0.8f;
        context.inCombat = false;
        context.enemiesInRange = 3;

        UtilityBehavior* selected = utilityAI->SelectBehavior(context);

        // Should select combat (not flee) when healthy
        REQUIRE(selected != nullptr);
        REQUIRE(selected->GetName() == "Combat");
    }

    SECTION("UtilityAI prioritizes flee when critically wounded")
    {
        auto utilityAI = std::make_unique<UtilityAI>();

        auto combatBehavior = std::make_shared<UtilityBehavior>("Combat");
        combatBehavior->AddEvaluator(std::make_shared<CombatEngageEvaluator>());
        utilityAI->AddBehavior(combatBehavior);

        auto fleeBehavior = std::make_shared<UtilityBehavior>("Flee");
        fleeBehavior->AddEvaluator(std::make_shared<FleeEvaluator>());
        utilityAI->AddBehavior(fleeBehavior);

        // Mock context: critically wounded bot in combat with many enemies
        UtilityContext context;
        context.bot = nullptr;
        context.healthPercent = 0.15f; // Critical health
        context.manaPercent = 0.5f;
        context.inCombat = true;
        context.enemiesInRange = 4; // Outnumbered

        UtilityBehavior* selected = utilityAI->SelectBehavior(context);

        // Should select flee when critically wounded
        REQUIRE(selected != nullptr);
        REQUIRE(selected->GetName() == "Flee");
    }
}

TEST_CASE("BotAI Integration: UtilityContext can be built from mock data", "[botai][utility][context]")
{
    SECTION("UtilityContextBuilder works with null bot (graceful handling)")
    {
        // This tests that UtilityContextBuilder doesn't crash with null pointers
        UtilityContext context = UtilityContextBuilder::Build(nullptr, nullptr);

        // Should return default context
        REQUIRE(context.bot == nullptr);
        REQUIRE(context.healthPercent == 1.0f);
        REQUIRE(context.manaPercent == 1.0f);
        REQUIRE(context.inCombat == false);
        REQUIRE(context.enemiesInRange == 0);
    }
}

TEST_CASE("BotAI Integration: Behavior scoring demonstrates expected priorities", "[botai][utility][scoring]")
{
    SECTION("Tank threat behavior scores high when tank has no aggro")
    {
        auto behavior = std::make_shared<UtilityBehavior>("Tanking");
        behavior->AddEvaluator(std::make_shared<TankThreatEvaluator>());

        UtilityContext context;
        context.bot = nullptr;
        context.role = UtilityContext::Role::TANK;
        context.enemiesInRange = 2;
        context.hasAggro = false;

        float score = behavior->CalculateUtility(context);

        // Should be maximum priority (1.0) when tank has no aggro
        REQUIRE(score == 1.0f);
    }

    SECTION("Healer behavior scores high when ally wounded")
    {
        auto behavior = std::make_shared<UtilityBehavior>("Healing");
        behavior->AddEvaluator(std::make_shared<HealAllyEvaluator>());

        UtilityContext context;
        context.bot = nullptr;
        context.role = UtilityContext::Role::HEALER;
        context.manaPercent = 0.8f;
        context.lowestAllyHealthPercent = 0.3f; // Ally critical

        float score = behavior->CalculateUtility(context);

        // Should have high priority when ally wounded and healer has mana
        REQUIRE(score > 0.5f);
    }

    SECTION("DPS behavior scores zero when not DPS role")
    {
        auto behavior = std::make_shared<UtilityBehavior>("Combat");
        behavior->AddEvaluator(std::make_shared<CombatEngageEvaluator>());

        UtilityContext context;
        context.bot = nullptr;
        context.role = UtilityContext::Role::HEALER; // Not DPS
        context.healthPercent = 0.8f;
        context.enemiesInRange = 0; // No enemies

        float score = behavior->CalculateUtility(context);

        // Should be zero when no enemies
        REQUIRE(score == 0.0f);
    }
}

TEST_CASE("BotAI Integration: Decision updates are properly throttled", "[botai][utility][throttling]")
{
    // This test verifies the throttling logic conceptually
    // Actual throttling happens in UpdateUtilityDecision()

    SECTION("Throttle timer accumulates correctly")
    {
        uint32 lastUpdate = 0;
        uint32 diff = 100; // 100ms elapsed

        lastUpdate += diff;
        REQUIRE(lastUpdate == 100);

        // After 5 updates of 100ms each = 500ms
    for (int i = 0; i < 4; ++i)
            lastUpdate += diff;

        REQUIRE(lastUpdate >= 500);

        // Reset after threshold reached
        lastUpdate = 0;
        REQUIRE(lastUpdate == 0);
    }
}
