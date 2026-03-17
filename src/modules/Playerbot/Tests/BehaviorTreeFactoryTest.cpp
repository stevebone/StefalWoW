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
#include "AI/BehaviorTree/BehaviorTreeFactory.h"

using namespace Playerbot;

TEST_CASE("BehaviorTreeFactory: All tree types can be created", "[behavior-tree][factory]")
{
    SECTION("Melee combat tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "MeleeCombatRoot");
    }

    SECTION("Ranged combat tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::RANGED_COMBAT);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "RangedCombatRoot");
    }

    SECTION("Tank combat tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::TANK_COMBAT);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "TankCombatRoot");
    }

    SECTION("Single target healing tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::SINGLE_TARGET_HEALING);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "SingleTargetHealingRoot");
    }

    SECTION("Group healing tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::GROUP_HEALING);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "GroupHealingRoot");
    }

    SECTION("Dispel priority tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::DISPEL_PRIORITY);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "DispelPriority");
    }

    SECTION("Follow leader tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::FOLLOW_LEADER);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "FollowLeader");
    }

    SECTION("Combat positioning tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::COMBAT_POSITIONING);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "CombatPositioning");
    }

    SECTION("Flee to safety tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::FLEE_TO_SAFETY);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "FleeToSafety");
    }

    SECTION("Buff maintenance tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::BUFF_MAINTENANCE);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "BuffMaintenance");
    }

    SECTION("Resource management tree")
    {
        auto tree = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::RESOURCE_MANAGEMENT);
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "ResourceManagement");
    }
}

TEST_CASE("BehaviorTreeFactory: Melee combat tree structure", "[behavior-tree][factory][melee]")
{
    auto root = BehaviorTreeFactory::BuildMeleeCombatTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a selector (tries multiple options)")
    {
        // Root should be BTSelector
        auto selector = std::dynamic_pointer_cast<BTSelector>(root);
        REQUIRE(selector != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        // Should return either SUCCESS or FAILURE (not INVALID)
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Ranged combat tree structure", "[behavior-tree][factory][ranged]")
{
    auto root = BehaviorTreeFactory::BuildRangedCombatTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a selector")
    {
        auto selector = std::dynamic_pointer_cast<BTSelector>(root);
        REQUIRE(selector != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Tank combat tree structure", "[behavior-tree][factory][tank]")
{
    auto root = BehaviorTreeFactory::BuildTankCombatTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a selector")
    {
        auto selector = std::dynamic_pointer_cast<BTSelector>(root);
        REQUIRE(selector != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Single target healing tree structure", "[behavior-tree][factory][healing]")
{
    auto root = BehaviorTreeFactory::BuildSingleTargetHealingTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a selector")
    {
        auto selector = std::dynamic_pointer_cast<BTSelector>(root);
        REQUIRE(selector != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Group healing tree structure", "[behavior-tree][factory][group-healing]")
{
    auto root = BehaviorTreeFactory::BuildGroupHealingTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a selector")
    {
        auto selector = std::dynamic_pointer_cast<BTSelector>(root);
        REQUIRE(selector != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Dispel priority tree structure", "[behavior-tree][factory][dispel]")
{
    auto root = BehaviorTreeFactory::BuildDispelPriorityTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a selector (tries dispels in priority order)")
    {
        auto selector = std::dynamic_pointer_cast<BTSelector>(root);
        REQUIRE(selector != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Follow leader tree structure", "[behavior-tree][factory][follow]")
{
    auto root = BehaviorTreeFactory::BuildFollowLeaderTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a sequence")
    {
        auto sequence = std::dynamic_pointer_cast<BTSequence>(root);
        REQUIRE(sequence != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Combat positioning tree structure", "[behavior-tree][factory][positioning]")
{
    auto root = BehaviorTreeFactory::BuildCombatPositioningTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a selector (tries melee or ranged positioning)")
    {
        auto selector = std::dynamic_pointer_cast<BTSelector>(root);
        REQUIRE(selector != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Flee to safety tree structure", "[behavior-tree][factory][flee]")
{
    auto root = BehaviorTreeFactory::BuildFleeToSafetyTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a sequence")
    {
        auto sequence = std::dynamic_pointer_cast<BTSequence>(root);
        REQUIRE(sequence != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Buff maintenance tree structure", "[behavior-tree][factory][buff]")
{
    auto root = BehaviorTreeFactory::BuildBuffMaintenanceTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a sequence")
    {
        auto sequence = std::dynamic_pointer_cast<BTSequence>(root);
        REQUIRE(sequence != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Resource management tree structure", "[behavior-tree][factory][resource]")
{
    auto root = BehaviorTreeFactory::BuildResourceManagementTree();
    REQUIRE(root != nullptr);

    BTBlackboard blackboard;

    SECTION("Tree is a selector")
    {
        auto selector = std::dynamic_pointer_cast<BTSelector>(root);
        REQUIRE(selector != nullptr);
    }

    SECTION("Tree executes without crashing")
    {
        BTStatus status = root->Tick(nullptr, blackboard);
        REQUIRE(status != BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTreeFactory: Custom tree registration and creation", "[behavior-tree][factory][custom]")
{
    SECTION("Can register custom tree")
    {
        BehaviorTreeFactory::RegisterCustomTree("TestTree",
            []() -> std::shared_ptr<BTNode> {
                return std::make_shared<BTCondition>("CustomRoot",
                    [](auto, auto&) { return true; });
            }
        );

        auto tree = BehaviorTreeFactory::CreateCustomTree("TestTree");
        REQUIRE(tree != nullptr);
        REQUIRE(tree->GetName() == "CustomRoot");
    }

    SECTION("Returns nullptr for non-existent custom tree")
    {
        auto tree = BehaviorTreeFactory::CreateCustomTree("NonExistentTree");
        REQUIRE(tree == nullptr);
    }

    SECTION("Can execute custom tree")
    {
        BehaviorTreeFactory::RegisterCustomTree("ExecutableTree",
            []() -> std::shared_ptr<BTNode> {
                auto sequence = std::make_shared<BTSequence>("CustomSequence");
                sequence->AddChild(std::make_shared<BTCondition>("Check1",
                    [](auto, auto&) { return true; }));
                sequence->AddChild(std::make_shared<BTAction>("Action1",
                    [](auto, BTBlackboard& bb) {
                        bb.Set<int>("Result", 42);
                        return BTStatus::SUCCESS;
                    }));
                return sequence;
            }
        );

        auto tree = BehaviorTreeFactory::CreateCustomTree("ExecutableTree");
        REQUIRE(tree != nullptr);

        BTBlackboard blackboard;
        BTStatus status = tree->Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);
        REQUIRE(blackboard.GetOr<int>("Result", 0) == 42);
    }
}

TEST_CASE("BehaviorTreeFactory: Tree reusability", "[behavior-tree][factory][reusability]")
{
    SECTION("Same tree can be created multiple times")
    {
        auto tree1 = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        auto tree2 = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);

        REQUIRE(tree1 != nullptr);
        REQUIRE(tree2 != nullptr);

        // Different instances
        REQUIRE(tree1.get() != tree2.get());

        // But same structure
        REQUIRE(tree1->GetName() == tree2->GetName());
    }

    SECTION("Multiple trees can execute independently")
    {
        auto tree1 = BehaviorTreeFactory::BuildSingleTargetHealingTree();
        auto tree2 = BehaviorTreeFactory::BuildSingleTargetHealingTree();

        BTBlackboard blackboard1;
        BTBlackboard blackboard2;

        blackboard1.Set<int>("ID", 1);
        blackboard2.Set<int>("ID", 2);

        tree1->Tick(nullptr, blackboard1);
        tree2->Tick(nullptr, blackboard2);

        REQUIRE(blackboard1.GetOr<int>("ID", 0) == 1);
        REQUIRE(blackboard2.GetOr<int>("ID", 0) == 2);
    }
}

TEST_CASE("BehaviorTreeFactory: Trees work in BehaviorTree container", "[behavior-tree][factory][integration]")
{
    SECTION("Factory tree works in BehaviorTree container")
    {
        BehaviorTree tree;

        auto root = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        tree.SetRoot(root);

        BTStatus status = tree.Tick(nullptr);
        REQUIRE(status != BTStatus::INVALID);
    }

    SECTION("Factory tree respects container reset")
    {
        BehaviorTree tree;

        auto root = BehaviorTreeFactory::CreateTree(BehaviorTreeFactory::TreeType::MELEE_COMBAT);
        tree.SetRoot(root);

        tree.GetBlackboard().Set<int>("TestValue", 123);
        REQUIRE(tree.GetBlackboard().Has("TestValue"));

        tree.Reset();
        REQUIRE_FALSE(tree.GetBlackboard().Has("TestValue"));
    }
}
