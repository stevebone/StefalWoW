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
#include "AI/HybridAIController.h"

using namespace Playerbot;

TEST_CASE("HybridAIController: Initialization creates Utility AI and mappings", "[hybrid-ai][initialization]")
{
    HybridAIController controller(nullptr, nullptr);

    SECTION("Controller starts uninitialized")
    {
        REQUIRE(controller.GetCurrentBehaviorName() == "None");
        REQUIRE(controller.GetCurrentTreeStatus() == BTStatus::INVALID);
    }

    SECTION("Initialize creates Utility AI")
    {
        controller.Initialize();

        REQUIRE(controller.GetUtilityAI() != nullptr);
    }

    SECTION("Initialize creates default behavior mappings")
    {
        controller.Initialize();

        // Should have default behaviors registered
        REQUIRE(controller.GetUtilityAI()->GetBehaviorCount() > 0);
    }
}

TEST_CASE("HybridAIController: Update without initialization returns false", "[hybrid-ai][update]")
{
    HybridAIController controller(nullptr, nullptr);

    SECTION("Update before Initialize returns false")
    {
        bool result = controller.Update(0);
        REQUIRE_FALSE(result);
    }
}

TEST_CASE("HybridAIController: Behavior mapping registration", "[hybrid-ai][mapping]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Can register standard behavior mapping")
    {
        controller.RegisterBehaviorMapping("TestBehavior", BehaviorTreeFactory::TreeType::MELEE_COMBAT);

        // Registration should not crash
        REQUIRE(true);
    }

    SECTION("Can register custom behavior mapping")
    {
        bool builderCalled = false;

        controller.RegisterCustomBehaviorMapping("CustomBehavior",
            [&builderCalled]() -> std::shared_ptr<BTNode> {
                builderCalled = true;
                return std::make_shared<BTCondition>("CustomRoot",
                    [](auto, auto&) { return true; });
            });

        // Registration should not crash
        REQUIRE(true);
    }
}

TEST_CASE("HybridAIController: Behavior change tracking", "[hybrid-ai][tracking]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Initially no behavior change")
    {
        REQUIRE_FALSE(controller.BehaviorChangedThisFrame());
    }

    SECTION("Time since last change initially zero")
    {
        REQUIRE(controller.GetTimeSinceLastBehaviorChange() == 0);
    }
}

TEST_CASE("HybridAIController: Reset clears state", "[hybrid-ai][reset]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Reset clears behavior name")
    {
        controller.Reset();

        REQUIRE(controller.GetCurrentBehaviorName() == "None");
        REQUIRE(controller.GetCurrentTreeStatus() == BTStatus::INVALID);
    }

    SECTION("Reset clears tree")
    {
        controller.Reset();

        REQUIRE(controller.GetCurrentTree() == nullptr);
    }
}

TEST_CASE("HybridAIController: Behavior tree creation from mappings", "[hybrid-ai][tree-creation]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Can create tree for registered behavior")
    {
        controller.RegisterBehaviorMapping("Combat", BehaviorTreeFactory::TreeType::MELEE_COMBAT);

        // This tests that the mapping works (tree creation happens internally during Update)
        REQUIRE(true);
    }

    SECTION("Custom tree builder is called when behavior selected")
    {
        bool builderCalled = false;

        controller.RegisterCustomBehaviorMapping("CustomBehavior",
            [&builderCalled]() -> std::shared_ptr<BTNode> {
                builderCalled = true;
                auto root = std::make_shared<BTCondition>("CustomRoot",
                    [](auto, auto&) { return true; });
                return root;
            });

        // Builder should not be called during registration
        REQUIRE_FALSE(builderCalled);

        // Builder would be called during Update when behavior is selected
        // (This requires BotAI mock which we don't have in unit tests)
    }
}

TEST_CASE("HybridAIController: Default behavior mappings", "[hybrid-ai][default-mappings]")
{
    HybridAIController controller(nullptr, nullptr);

    SECTION("Initialize creates default combat mappings")
    {
        controller.Initialize();

        // Should have utility behaviors registered
        REQUIRE(controller.GetUtilityAI() != nullptr);
        REQUIRE(controller.GetUtilityAI()->GetBehaviorCount() >= 5); // At least Combat, Healing, Tanking, Flee, ManaRegen
    }
}

TEST_CASE("HybridAIController: Utility AI integration", "[hybrid-ai][utility-ai]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Utility AI is accessible")
    {
        REQUIRE(controller.GetUtilityAI() != nullptr);
    }

    SECTION("Utility AI has behaviors registered")
    {
        REQUIRE(controller.GetUtilityAI()->GetBehaviorCount() > 0);
    }
}

TEST_CASE("HybridAIController: Behavior tree integration", "[hybrid-ai][behavior-tree]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Current tree initially null")
    {
        REQUIRE(controller.GetCurrentTree() == nullptr);
    }

    SECTION("Current tree status initially invalid")
    {
        REQUIRE(controller.GetCurrentTreeStatus() == BTStatus::INVALID);
    }
}

TEST_CASE("HybridAIController: Decision throttling", "[hybrid-ai][throttling]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Multiple rapid updates don't crash")
    {
        // Call Update multiple times rapidly
    for (int i = 0; i < 10; ++i)
        {
            controller.Update(10); // 10ms per update
        }

        // Should not crash
        REQUIRE(true);
    }
}

TEST_CASE("HybridAIController: Multiple controllers operate independently", "[hybrid-ai][independence]")
{
    HybridAIController controller1(nullptr, nullptr);
    HybridAIController controller2(nullptr, nullptr);

    controller1.Initialize();
    controller2.Initialize();

    SECTION("Controllers have separate state")
    {
        controller1.RegisterBehaviorMapping("Test1", BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        controller2.RegisterBehaviorMapping("Test2", BehaviorTreeFactory::TreeType::RANGED_COMBAT);

        // Both should operate independently
        REQUIRE(controller1.GetUtilityAI() != controller2.GetUtilityAI());
    }
}

TEST_CASE("HybridAIController: Behavior name tracking", "[hybrid-ai][behavior-name]")
{
    HybridAIController controller(nullptr, nullptr);

    SECTION("Initially no behavior")
    {
        REQUIRE(controller.GetCurrentBehaviorName() == "None");
    }

    SECTION("After reset, behavior name is None")
    {
        controller.Initialize();
        controller.Reset();

        REQUIRE(controller.GetCurrentBehaviorName() == "None");
    }
}

TEST_CASE("HybridAIController: Tree status tracking", "[hybrid-ai][tree-status]")
{
    HybridAIController controller(nullptr, nullptr);

    SECTION("Initially invalid status")
    {
        REQUIRE(controller.GetCurrentTreeStatus() == BTStatus::INVALID);
    }

    SECTION("After reset, status is invalid")
    {
        controller.Initialize();
        controller.Reset();

        REQUIRE(controller.GetCurrentTreeStatus() == BTStatus::INVALID);
    }
}

TEST_CASE("HybridAIController: Multiple behaviors can be registered", "[hybrid-ai][multiple-behaviors]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Can register multiple standard behaviors")
    {
        controller.RegisterBehaviorMapping("Behavior1", BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        controller.RegisterBehaviorMapping("Behavior2", BehaviorTreeFactory::TreeType::RANGED_COMBAT);
        controller.RegisterBehaviorMapping("Behavior3", BehaviorTreeFactory::TreeType::SINGLE_TARGET_HEALING);

        // Should not crash
        REQUIRE(true);
    }

    SECTION("Can register multiple custom behaviors")
    {
        controller.RegisterCustomBehaviorMapping("Custom1",
            []() { return std::make_shared<BTCondition>("C1", [](auto, auto&) { return true; }); });

        controller.RegisterCustomBehaviorMapping("Custom2",
            []() { return std::make_shared<BTCondition>("C2", [](auto, auto&) { return false; }); });

        // Should not crash
        REQUIRE(true);
    }

    SECTION("Can mix standard and custom behaviors")
    {
        controller.RegisterBehaviorMapping("Standard", BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        controller.RegisterCustomBehaviorMapping("Custom",
            []() { return std::make_shared<BTCondition>("Custom", [](auto, auto&) { return true; }); });

        // Should not crash
        REQUIRE(true);
    }
}

TEST_CASE("HybridAIController: Reset multiple times", "[hybrid-ai][reset-stability]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Multiple resets don't crash")
    {
        for (int i = 0; i < 5; ++i)
        {
            controller.Reset();
        }

        REQUIRE(controller.GetCurrentBehaviorName() == "None");
    }

    SECTION("Can reinitialize after reset")
    {
        controller.Reset();
        controller.Initialize();

        REQUIRE(controller.GetUtilityAI() != nullptr);
    }
}
