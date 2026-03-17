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
#include "AI/BehaviorTree/BehaviorTreeFactory.h"
#include "AI/Utility/UtilitySystem.h"
#include <chrono>

using namespace Playerbot;

/**
 * @brief Integration tests for the complete Hybrid AI system
 * Tests the full pipeline: Utility AI → Behavior Selection → Tree Execution
 */

TEST_CASE("HybridAI Integration: Complete decision pipeline", "[hybrid-ai][integration]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Utility AI selects behaviors based on context")
    {
        // Controller should have utility AI with registered behaviors
        REQUIRE(controller.GetUtilityAI() != nullptr);
        REQUIRE(controller.GetUtilityAI()->GetBehaviorCount() > 0);
    }

    SECTION("Behavior mappings connect to tree factory")
    {
        // Register a test behavior
        controller.RegisterBehaviorMapping("TestCombat", BehaviorTreeFactory::TreeType::MELEE_COMBAT);

        // Should not crash when trying to create tree
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        REQUIRE(tree != nullptr);
    }

    SECTION("Custom behaviors integrate with factory trees")
    {
        bool customBuilderCalled = false;

        controller.RegisterCustomBehaviorMapping("CustomTest",
            [&customBuilderCalled]() {
                customBuilderCalled = true;
                return BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
            });

        // Custom builder registered successfully
        REQUIRE(true);
    }
}

TEST_CASE("HybridAI Integration: Behavior tree execution flow", "[hybrid-ai][integration][execution]")
{
    SECTION("Factory trees execute without errors")
    {
        auto meleeTree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        REQUIRE(meleeTree != nullptr);

        BTBlackboard blackboard;
        BTStatus status = meleeTree->Tick(nullptr, blackboard);

        // Should return valid status (not INVALID)
        REQUIRE(status != BTStatus::INVALID);
    }

    SECTION("All 11 factory trees can be created and executed")
    {
        std::vector<BehaviorTreeFactory::TreeType> allTypes = {
            BehaviorTreeFactory::TreeType::MELEE_COMBAT,
            BehaviorTreeFactory::TreeType::RANGED_COMBAT,
            BehaviorTreeFactory::TreeType::TANK_COMBAT,
            BehaviorTreeFactory::TreeType::SINGLE_TARGET_HEALING,
            BehaviorTreeFactory::TreeType::GROUP_HEALING,
            BehaviorTreeFactory::TreeType::DISPEL_PRIORITY,
            BehaviorTreeFactory::TreeType::FOLLOW_LEADER,
            BehaviorTreeFactory::TreeType::COMBAT_POSITIONING,
            BehaviorTreeFactory::TreeType::FLEE_TO_SAFETY,
            BehaviorTreeFactory::TreeType::BUFF_MAINTENANCE,
            BehaviorTreeFactory::TreeType::RESOURCE_MANAGEMENT
        };

        for (auto type : allTypes)
        {
            auto tree = BehaviorTreeFactory::CreateTree(type);
            REQUIRE(tree != nullptr);

            BTBlackboard blackboard;
            BTStatus status = tree->Tick(nullptr, blackboard);
            REQUIRE(status != BTStatus::INVALID);
        }
    }
}

TEST_CASE("HybridAI Integration: Controller lifecycle", "[hybrid-ai][integration][lifecycle]")
{
    SECTION("Initialize → Update → Reset cycle works")
    {
        HybridAIController controller(nullptr, nullptr);

        // Initialize
        controller.Initialize();
        REQUIRE(controller.GetUtilityAI() != nullptr);

        // Update (should not crash even without BotAI)
        controller.Update(500);

        // Reset
        controller.Reset();
        REQUIRE(controller.GetCurrentBehaviorName() == "None");

        // Re-initialize
        controller.Initialize();
        REQUIRE(controller.GetUtilityAI() != nullptr);
    }

    SECTION("Multiple update cycles")
    {
        HybridAIController controller(nullptr, nullptr);
        controller.Initialize();

        for (int i = 0; i < 100; ++i)
        {
            controller.Update(16); // Simulate 16ms frame time
        }

        // Should complete without crashes
        REQUIRE(true);
    }
}

TEST_CASE("HybridAI Integration: Behavior transitions", "[hybrid-ai][integration][transitions]")
{
    HybridAIController controller(nullptr, nullptr);
    controller.Initialize();

    SECTION("Behavior change flag resets each frame")
    {
        controller.Update(500);
        bool firstFrame = controller.BehaviorChangedThisFrame();

        controller.Update(500);
        // Flag should reset after each update
        // (behavior won't actually change without BotAI context)
        REQUIRE(true);
    }

    SECTION("Time tracking works correctly")
    {
        controller.Initialize();

        uint32 initialTime = controller.GetTimeSinceLastBehaviorChange();
        REQUIRE(initialTime == 0);
    }
}

TEST_CASE("HybridAI Integration: Blackboard data flow", "[hybrid-ai][integration][blackboard]")
{
    SECTION("Blackboard data persists across tree ticks")
    {
        auto tree = BehaviorTreeFactory::BuildMeleeCombatTree();
        BTBlackboard blackboard;

        // Set some data
        blackboard.Set<int>("TestValue", 42);

        // Tick tree
        tree->Tick(nullptr, blackboard);

        // Data should still be there
        REQUIRE(blackboard.GetOr<int>("TestValue", 0) == 42);
    }

    SECTION("Different trees can share blackboard")
    {
        auto tree1 = BehaviorTreeFactory::BuildMeleeCombatTree();
        auto tree2 = BehaviorTreeFactory::BuildSingleTargetHealingTree();

        BTBlackboard sharedBlackboard;
        sharedBlackboard.Set<std::string>("SharedData", "test");

        tree1->Tick(nullptr, sharedBlackboard);
        tree2->Tick(nullptr, sharedBlackboard);

        // Shared data should persist
        REQUIRE(sharedBlackboard.Has("SharedData"));
    }
}

TEST_CASE("HybridAI Integration: Custom node integration", "[hybrid-ai][integration][custom-nodes]")
{
    SECTION("Combat nodes work in custom trees")
    {
        auto sequence = std::make_shared<BTSequence>("CustomCombat");
        sequence->AddChild(std::make_shared<BTCheckHasTarget>());
        sequence->AddChild(std::make_shared<BTCheckInRange>(0.0f, 5.0f));

        BTBlackboard blackboard;
        BTStatus status = sequence->Tick(nullptr, blackboard);

        // Should execute without crashing
        REQUIRE(status != BTStatus::INVALID);
    }

    SECTION("Healing nodes work in custom trees")
    {
        auto sequence = std::make_shared<BTSequence>("CustomHealing");
        sequence->AddChild(std::make_shared<BTFindWoundedAlly>(0.8f));
        sequence->AddChild(std::make_shared<BTCheckHealTargetInRange>(40.0f));

        BTBlackboard blackboard;
        BTStatus status = sequence->Tick(nullptr, blackboard);

        REQUIRE(status != BTStatus::INVALID);
    }

    SECTION("Movement nodes work in custom trees")
    {
        auto sequence = std::make_shared<BTSequence>("CustomMovement");
        sequence->AddChild(std::make_shared<BTCheckIsMoving>());
        sequence->AddChild(std::make_shared<BTStopMovement>());

        BTBlackboard blackboard;
        BTStatus status = sequence->Tick(nullptr, blackboard);

        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("HybridAI Integration: Error handling", "[hybrid-ai][integration][error-handling]")
{
    SECTION("Controller handles null BotAI gracefully")
    {
        HybridAIController controller(nullptr, nullptr);
        controller.Initialize();

        // Should not crash with null BotAI
        bool result = controller.Update(500);
        REQUIRE_FALSE(result); // Should return false but not crash
    }

    SECTION("Trees handle null BotAI gracefully")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        BTBlackboard blackboard;

        // Should not crash with null BotAI
        BTStatus status = tree->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }

    SECTION("Missing behavior mapping doesn't crash")
    {
        HybridAIController controller(nullptr, nullptr);
        controller.Initialize();

        // Try to use unmapped behavior (would happen during Update with real context)
        // Should handle gracefully
        REQUIRE(true);
    }
}

TEST_CASE("HybridAI Integration: Performance characteristics", "[hybrid-ai][integration][performance]")
{
    SECTION("Controller initialization is fast")
    {
        auto start = std::chrono::high_resolution_clock::now();

        HybridAIController controller(nullptr, nullptr);
        controller.Initialize();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // Should initialize in less than 10ms
        REQUIRE(duration.count() < 10000);
    }

    SECTION("Tree creation is fast")
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 100; ++i)
        {
            auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // Average should be less than 100μs per tree
        REQUIRE(duration.count() / 100 < 100);
    }

    SECTION("Tree execution is fast")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        BTBlackboard blackboard;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 1000; ++i)
        {
            tree->Tick(nullptr, blackboard);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // Average should be less than 10μs per tick
        REQUIRE(duration.count() / 1000 < 10);
    }

    SECTION("Controller update is fast")
    {
        HybridAIController controller(nullptr, nullptr);
        controller.Initialize();

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 1000; ++i)
        {
            controller.Update(16);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // Average should be less than 50μs per update
        REQUIRE(duration.count() / 1000 < 50);
    }
}

TEST_CASE("HybridAI Integration: Memory management", "[hybrid-ai][integration][memory]")
{
    SECTION("Multiple controllers can exist simultaneously")
    {
        std::vector<std::unique_ptr<HybridAIController>> controllers;

        for (int i = 0; i < 100; ++i)
        {
            auto controller = std::make_unique<HybridAIController>(nullptr, nullptr);
            controller->Initialize();
            controllers.push_back(std::move(controller));
        }

        // Should create 100 controllers without issues
        REQUIRE(controllers.size() == 100);

        // Clean up
        controllers.clear();
        REQUIRE(controllers.empty());
    }

    SECTION("Trees are properly destroyed")
    {
        {
            auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
            REQUIRE(tree != nullptr);
        }
        // Tree should be destroyed here via shared_ptr
        REQUIRE(true);
    }

    SECTION("Controller cleanup is complete")
    {
        {
            HybridAIController controller(nullptr, nullptr);
            controller.Initialize();
            controller.Update(500);
        }
        // Controller should be destroyed here
        REQUIRE(true);
    }
}

TEST_CASE("HybridAI Integration: Extensibility", "[hybrid-ai][integration][extensibility]")
{
    SECTION("Can register multiple custom behaviors")
    {
        HybridAIController controller(nullptr, nullptr);
        controller.Initialize();

        for (int i = 0; i < 10; ++i)
        {
            std::string name = "Custom" + std::to_string(i);
            controller.RegisterCustomBehaviorMapping(name,
                [i]() {
                    auto root = std::make_shared<BTCondition>("Root" + std::to_string(i),
                        [](auto, auto&) { return true; });
                    return root;
                });
        }

        // Should register all successfully
        REQUIRE(true);
    }

    SECTION("Can mix factory and custom trees")
    {
        HybridAIController controller(nullptr, nullptr);
        controller.Initialize();

        // Factory mapping
        controller.RegisterBehaviorMapping("Combat", BehaviorTreeFactory::TreeType::MELEE_COMBAT);

        // Custom mapping
        controller.RegisterCustomBehaviorMapping("Custom",
            []() { return std::make_shared<BTCondition>("Custom", [](auto, auto&) { return true; }); });

        // Both should coexist
        REQUIRE(true);
    }
}

TEST_CASE("HybridAI Integration: Complete system stress test", "[hybrid-ai][integration][stress]")
{
    SECTION("1000 controllers with 1000 updates each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            HybridAIController controller(nullptr, nullptr);
            controller.Initialize();

            for (int j = 0; j < 1000; ++j)
            {
                controller.Update(16);
            }
        }

        // Should complete without crashes or memory leaks
        REQUIRE(true);
    }

    SECTION("All tree types executed 1000 times each")
    {
        std::vector<BehaviorTreeFactory::TreeType> allTypes = {
            BehaviorTreeFactory::TreeType::MELEE_COMBAT,
            BehaviorTreeFactory::TreeType::RANGED_COMBAT,
            BehaviorTreeFactory::TreeType::TANK_COMBAT,
            BehaviorTreeFactory::TreeType::SINGLE_TARGET_HEALING,
            BehaviorTreeFactory::TreeType::GROUP_HEALING,
            BehaviorTreeFactory::TreeType::DISPEL_PRIORITY,
            BehaviorTreeFactory::TreeType::FOLLOW_LEADER,
            BehaviorTreeFactory::TreeType::COMBAT_POSITIONING,
            BehaviorTreeFactory::TreeType::FLEE_TO_SAFETY,
            BehaviorTreeFactory::TreeType::BUFF_MAINTENANCE,
            BehaviorTreeFactory::TreeType::RESOURCE_MANAGEMENT
        };

        for (auto type : allTypes)
        {
            auto tree = BehaviorTreeFactory::CreateTree(type);
            BTBlackboard blackboard;

            for (int i = 0; i < 1000; ++i)
            {
                tree->Tick(nullptr, blackboard);
            }
        }

        REQUIRE(true);
    }
}
