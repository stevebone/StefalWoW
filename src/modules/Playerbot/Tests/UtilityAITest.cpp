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
#include "AI/Utility/UtilitySystem.h"
#include "AI/Utility/Evaluators/CombatEvaluators.h"

using namespace Playerbot;

TEST_CASE("UtilityAI: Utility curve functions work correctly", "[utility][curves]")
{
    SECTION("Linear curve")
    {
        REQUIRE(UtilityEvaluator::Linear(0.0f) == 0.0f);
        REQUIRE(UtilityEvaluator::Linear(0.5f) == 0.5f);
        REQUIRE(UtilityEvaluator::Linear(1.0f) == 1.0f);
    }

    SECTION("Quadratic curve")
    {
        REQUIRE(UtilityEvaluator::Quadratic(0.0f) == 0.0f);
        REQUIRE(UtilityEvaluator::Quadratic(0.5f) == 0.25f);
        REQUIRE(UtilityEvaluator::Quadratic(1.0f) == 1.0f);
    }

    SECTION("Inverse linear curve")
    {
        REQUIRE(UtilityEvaluator::InverseLinear(0.0f) == 1.0f);
        REQUIRE(UtilityEvaluator::InverseLinear(0.5f) == 0.5f);
        REQUIRE(UtilityEvaluator::InverseLinear(1.0f) == 0.0f);
    }

    SECTION("Clamp function")
    {
        REQUIRE(UtilityEvaluator::Clamp(-0.5f) == 0.0f);
        REQUIRE(UtilityEvaluator::Clamp(0.5f) == 0.5f);
        REQUIRE(UtilityEvaluator::Clamp(1.5f) == 1.0f);
    }
}

TEST_CASE("UtilityAI: Combat evaluator scores correctly", "[utility][combat]")
{
    UtilityContext context;
    context.bot = nullptr; // Not needed for these tests
    context.healthPercent = 0.8f;
    context.inCombat = false;
    context.enemiesInRange = 3;

    CombatEngageEvaluator evaluator;

    SECTION("High health with enemies gives positive score")
    {
        float score = evaluator.Evaluate(context);
        REQUIRE(score > 0.0f);
        REQUIRE(score <= 1.0f);
    }

    SECTION("No enemies gives zero score")
    {
        context.enemiesInRange = 0;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("Already in combat maintains high score")
    {
        context.inCombat = true;
        float score = evaluator.Evaluate(context);
        REQUIRE(score >= 0.8f);
    }

    SECTION("Low health reduces combat engagement score")
    {
        context.healthPercent = 0.2f;
        context.inCombat = false;
        float score = evaluator.Evaluate(context);
        // Low health should reduce willingness to engage
        REQUIRE(score < 0.5f);
    }
}

TEST_CASE("UtilityAI: Healer evaluator prioritizes wounded allies", "[utility][healing]")
{
    UtilityContext context;
    context.bot = nullptr;
    context.role = UtilityContext::Role::HEALER;
    context.manaPercent = 0.8f;
    context.lowestAllyHealthPercent = 0.3f; // Wounded ally

    HealAllyEvaluator evaluator;

    SECTION("Wounded ally with mana gives high score")
    {
        float score = evaluator.Evaluate(context);
        REQUIRE(score > 0.5f);
    }

    SECTION("No mana gives zero score")
    {
        context.manaPercent = 0.05f;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("Non-healer gives zero score")
    {
        context.role = UtilityContext::Role::DPS;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("Healthy allies reduce healing priority")
    {
        context.lowestAllyHealthPercent = 0.95f; // All allies healthy
        float score = evaluator.Evaluate(context);
        // Should be very low since allies are healthy
        REQUIRE(score < 0.2f);
    }
}

TEST_CASE("UtilityAI: Tank threat evaluator works correctly", "[utility][tanking]")
{
    UtilityContext context;
    context.bot = nullptr;
    context.role = UtilityContext::Role::TANK;
    context.enemiesInRange = 2;
    context.hasAggro = false;

    TankThreatEvaluator evaluator;

    SECTION("Tank without aggro gets critical score")
    {
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 1.0f); // Maximum priority
    }

    SECTION("Tank with aggro gets moderate score")
    {
        context.hasAggro = true;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.6f); // Maintain threat
    }

    SECTION("Non-tank gets zero score")
    {
        context.role = UtilityContext::Role::DPS;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("No enemies means no threat needed")
    {
        context.enemiesInRange = 0;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }
}

TEST_CASE("UtilityAI: Defensive cooldown evaluator prioritizes low health", "[utility][defensive]")
{
    UtilityContext context;
    context.bot = nullptr;
    context.inCombat = true;
    context.healthPercent = 0.3f;
    context.timeSinceCombatStart = 10000; // 10 seconds

    DefensiveCooldownEvaluator evaluator;

    SECTION("Low health in combat gives high score")
    {
        float score = evaluator.Evaluate(context);
        REQUIRE(score > 0.5f);
    }

    SECTION("High health reduces defensive priority")
    {
        context.healthPercent = 0.9f;
        float score = evaluator.Evaluate(context);
        REQUIRE(score < 0.3f);
    }

    SECTION("Not in combat gives zero score")
    {
        context.inCombat = false;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("Longer combat time increases priority")
    {
        context.timeSinceCombatStart = 35000; // 35 seconds
        float longCombatScore = evaluator.Evaluate(context);

        context.timeSinceCombatStart = 5000; // 5 seconds
        float shortCombatScore = evaluator.Evaluate(context);

        REQUIRE(longCombatScore > shortCombatScore);
    }
}

TEST_CASE("UtilityAI: Utility behavior combines evaluators correctly", "[utility][behavior]")
{
    UtilityContext context;
    context.bot = nullptr;
    context.healthPercent = 0.8f;
    context.inCombat = true;
    context.enemiesInRange = 2;

    auto behavior = std::make_shared<UtilityBehavior>("TestCombat");

    SECTION("Empty behavior returns zero score")
    {
        float score = behavior->CalculateUtility(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("Single evaluator returns weighted score")
    {
        auto evaluator = std::make_shared<CombatEngageEvaluator>();
        behavior->AddEvaluator(evaluator);

        float score = behavior->CalculateUtility(context);
        REQUIRE(score > 0.0f);
        REQUIRE(score <= 1.0f);
    }

    SECTION("Multiple evaluators multiply scores")
    {
        auto combat = std::make_shared<CombatEngageEvaluator>();
        auto defensive = std::make_shared<DefensiveCooldownEvaluator>();

        behavior->AddEvaluator(combat);
        behavior->AddEvaluator(defensive);

        float score = behavior->CalculateUtility(context);

        // Score should be product of individual evaluator scores
        REQUIRE(score > 0.0f);
        REQUIRE(score <= 1.0f);
    }

    SECTION("Cached score is stored and retrievable")
    {
        auto evaluator = std::make_shared<CombatEngageEvaluator>();
        behavior->AddEvaluator(evaluator);

        float score1 = behavior->CalculateUtility(context);
        float cachedScore = behavior->GetCachedScore();

        REQUIRE(score1 == cachedScore);
    }
}

TEST_CASE("UtilityAI: Utility AI selects highest-scoring behavior", "[utility][ai]")
{
    UtilityContext context;
    context.bot = nullptr;

    UtilityAI ai;

    SECTION("Empty AI returns nullptr")
    {
        UtilityBehavior* selected = ai.SelectBehavior(context);
        REQUIRE(selected == nullptr);
    }

    SECTION("Single behavior is selected")
    {
        auto behavior = std::make_shared<UtilityBehavior>("Combat");
        behavior->AddEvaluator(std::make_shared<CombatEngageEvaluator>());
        ai.AddBehavior(behavior);

        context.healthPercent = 0.8f;
        context.enemiesInRange = 2;

        UtilityBehavior* selected = ai.SelectBehavior(context);
        REQUIRE(selected != nullptr);
        REQUIRE(selected->GetName() == "Combat");
    }

    SECTION("Highest-scoring behavior is selected")
    {
        // Combat behavior (high score with enemies)
        auto combat = std::make_shared<UtilityBehavior>("Combat");
        combat->AddEvaluator(std::make_shared<CombatEngageEvaluator>());
        ai.AddBehavior(combat);

        // Healing behavior (low score when healthy)
        auto healing = std::make_shared<UtilityBehavior>("Healing");
        healing->AddEvaluator(std::make_shared<HealAllyEvaluator>());
        ai.AddBehavior(healing);

        context.healthPercent = 0.8f;
        context.enemiesInRange = 3;
        context.role = UtilityContext::Role::HEALER;
        context.manaPercent = 0.8f;
        context.lowestAllyHealthPercent = 0.9f; // Allies healthy

        UtilityBehavior* selected = ai.SelectBehavior(context);
        REQUIRE(selected != nullptr);
        // Combat should score higher than healing when allies are healthy
        REQUIRE(selected->GetName() == "Combat");
    }

    SECTION("GetRankedBehaviors returns sorted list")
    {
        auto combat = std::make_shared<UtilityBehavior>("Combat");
        combat->AddEvaluator(std::make_shared<CombatEngageEvaluator>());
        ai.AddBehavior(combat);

        auto flee = std::make_shared<UtilityBehavior>("Flee");
        flee->AddEvaluator(std::make_shared<FleeEvaluator>());
        ai.AddBehavior(flee);

        context.healthPercent = 0.15f; // Very low health
        context.inCombat = true;
        context.enemiesInRange = 4; // Outnumbered

        auto ranked = ai.GetRankedBehaviors(context);

        REQUIRE(ranked.size() == 2);
        // Flee should score higher than combat when heavily wounded and outnumbered
        REQUIRE(ranked[0].first->GetName() == "Flee");
        REQUIRE(ranked[0].second >= ranked[1].second); // Scores in descending order
    }
}

TEST_CASE("UtilityAI: AoE damage evaluator scales with enemy count", "[utility][aoe]")
{
    UtilityContext context;
    context.bot = nullptr;
    context.manaPercent = 0.8f;

    AoEDamageEvaluator evaluator;

    SECTION("Less than 3 enemies returns zero")
    {
        context.enemiesInRange = 2;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("3+ enemies returns positive score")
    {
        context.enemiesInRange = 4;
        float score = evaluator.Evaluate(context);
        REQUIRE(score > 0.0f);
    }

    SECTION("More enemies increases score")
    {
        context.enemiesInRange = 3;
        float score3 = evaluator.Evaluate(context);

        context.enemiesInRange = 6;
        float score6 = evaluator.Evaluate(context);

        REQUIRE(score6 > score3);
    }

    SECTION("Low mana reduces AoE priority")
    {
        context.enemiesInRange = 5;
        context.manaPercent = 0.2f; // Low mana
        float score = evaluator.Evaluate(context);

        // Should still be positive but reduced
        REQUIRE(score > 0.0f);
        REQUIRE(score < 0.5f);
    }
}

TEST_CASE("UtilityAI: Flee evaluator triggers at critical health", "[utility][flee]")
{
    UtilityContext context;
    context.bot = nullptr;
    context.inCombat = true;

    FleeEvaluator evaluator;

    SECTION("High health returns zero")
    {
        context.healthPercent = 0.8f;
        context.enemiesInRange = 2;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("Critical health triggers flee")
    {
        context.healthPercent = 0.15f; // Critical
        context.enemiesInRange = 2;
        float score = evaluator.Evaluate(context);
        REQUIRE(score > 0.5f);
    }

    SECTION("More enemies increases flee priority")
    {
        context.healthPercent = 0.18f;

        context.enemiesInRange = 1;
        float score1 = evaluator.Evaluate(context);

        context.enemiesInRange = 4;
        float score4 = evaluator.Evaluate(context);

        REQUIRE(score4 > score1);
    }

    SECTION("Not in combat returns zero")
    {
        context.inCombat = false;
        context.healthPercent = 0.1f;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }
}
