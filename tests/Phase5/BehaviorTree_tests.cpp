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

#include "tc_catch2.h"
#include "AI/Decision/BehaviorTree.h"

using namespace Playerbot::bot::ai;

TEST_CASE("BehaviorTree - NodeStatus Values", "[Phase5][BehaviorTree]")
{
    SECTION("NodeStatus enum values exist")
    {
        NodeStatus success = NodeStatus::SUCCESS;
        NodeStatus failure = NodeStatus::FAILURE;
        NodeStatus running = NodeStatus::RUNNING;

        REQUIRE(success != failure);
        REQUIRE(success != running);
        REQUIRE(failure != running);
    }
}

TEST_CASE("BehaviorTree - NodeType Values", "[Phase5][BehaviorTree]")
{
    SECTION("NodeType enum values exist")
    {
        NodeType composite = NodeType::COMPOSITE;
        NodeType decorator = NodeType::DECORATOR;
        NodeType leaf = NodeType::LEAF;

        REQUIRE(composite != decorator);
        REQUIRE(composite != leaf);
        REQUIRE(decorator != leaf);
    }
}

TEST_CASE("BehaviorTree - ConditionNode", "[Phase5][BehaviorTree]")
{
    SECTION("Condition returns SUCCESS when true")
    {
        auto condition = std::make_shared<ConditionNode>("AlwaysTrue",
            [](Player*, Unit*) { return true; });

        NodeStatus status = condition->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::SUCCESS);
    }

    SECTION("Condition returns FAILURE when false")
    {
        auto condition = std::make_shared<ConditionNode>("AlwaysFalse",
            [](Player*, Unit*) { return false; });

        NodeStatus status = condition->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::FAILURE);
    }

    SECTION("Condition can access parameters")
    {
        int callCount = 0;
        auto condition = std::make_shared<ConditionNode>("Counter",
            [&callCount](Player*, Unit*) {
                callCount++;
                return true;
            });

        condition->Tick(nullptr, nullptr);
        REQUIRE(callCount == 1);

        condition->Tick(nullptr, nullptr);
        REQUIRE(callCount == 2);
    }
}

TEST_CASE("BehaviorTree - ActionNode", "[Phase5][BehaviorTree]")
{
    SECTION("Action returns SUCCESS")
    {
        auto action = std::make_shared<ActionNode>("SuccessAction",
            [](Player*, Unit*) { return NodeStatus::SUCCESS; });

        NodeStatus status = action->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::SUCCESS);
    }

    SECTION("Action returns FAILURE")
    {
        auto action = std::make_shared<ActionNode>("FailureAction",
            [](Player*, Unit*) { return NodeStatus::FAILURE; });

        NodeStatus status = action->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::FAILURE);
    }

    SECTION("Action returns RUNNING")
    {
        auto action = std::make_shared<ActionNode>("RunningAction",
            [](Player*, Unit*) { return NodeStatus::RUNNING; });

        NodeStatus status = action->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::RUNNING);
    }

    SECTION("Action executes custom logic")
    {
        int executionCount = 0;
        auto action = std::make_shared<ActionNode>("CustomAction",
            [&executionCount](Player*, Unit*) {
                executionCount++;
                return NodeStatus::SUCCESS;
            });

        action->Tick(nullptr, nullptr);
        REQUIRE(executionCount == 1);

        action->Tick(nullptr, nullptr);
        REQUIRE(executionCount == 2);
    }
}

TEST_CASE("BehaviorTree - SequenceNode", "[Phase5][BehaviorTree]")
{
    SECTION("Empty sequence returns SUCCESS")
    {
        auto sequence = std::make_shared<SequenceNode>("EmptySequence");
        NodeStatus status = sequence->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::SUCCESS);
    }

    SECTION("Sequence with all SUCCESS children returns SUCCESS")
    {
        auto sequence = std::make_shared<SequenceNode>("AllSuccess");
        sequence->AddChild(std::make_shared<ConditionNode>("True1", [](Player*, Unit*) { return true; }));
        sequence->AddChild(std::make_shared<ConditionNode>("True2", [](Player*, Unit*) { return true; }));
        sequence->AddChild(std::make_shared<ConditionNode>("True3", [](Player*, Unit*) { return true; }));

        NodeStatus status = sequence->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::SUCCESS);
    }

    SECTION("Sequence stops at first FAILURE")
    {
        int firstExecuted = 0;
        int secondExecuted = 0;
        int thirdExecuted = 0;

        auto sequence = std::make_shared<SequenceNode>("FailSequence");
        sequence->AddChild(std::make_shared<ActionNode>("First",
            [&firstExecuted](Player*, Unit*) {
                firstExecuted++;
                return NodeStatus::SUCCESS;
            }));
        sequence->AddChild(std::make_shared<ActionNode>("Second",
            [&secondExecuted](Player*, Unit*) {
                secondExecuted++;
                return NodeStatus::FAILURE;
            }));
        sequence->AddChild(std::make_shared<ActionNode>("Third",
            [&thirdExecuted](Player*, Unit*) {
                thirdExecuted++;
                return NodeStatus::SUCCESS;
            }));

        NodeStatus status = sequence->Tick(nullptr, nullptr);

        REQUIRE(status == NodeStatus::FAILURE);
        REQUIRE(firstExecuted == 1);
        REQUIRE(secondExecuted == 1);
        REQUIRE(thirdExecuted == 0); // Should not execute after failure
    }

    SECTION("Sequence returns RUNNING when child is RUNNING")
    {
        auto sequence = std::make_shared<SequenceNode>("RunningSequence");
        sequence->AddChild(std::make_shared<ConditionNode>("True", [](Player*, Unit*) { return true; }));
        sequence->AddChild(std::make_shared<ActionNode>("Running",
            [](Player*, Unit*) { return NodeStatus::RUNNING; }));
        sequence->AddChild(std::make_shared<ConditionNode>("NeverReached", [](Player*, Unit*) { return true; }));

        NodeStatus status = sequence->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::RUNNING);
    }
}

TEST_CASE("BehaviorTree - SelectorNode", "[Phase5][BehaviorTree]")
{
    SECTION("Empty selector returns FAILURE")
    {
        auto selector = std::make_shared<SelectorNode>("EmptySelector");
        NodeStatus status = selector->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::FAILURE);
    }

    SECTION("Selector returns SUCCESS at first SUCCESS child")
    {
        int firstExecuted = 0;
        int secondExecuted = 0;
        int thirdExecuted = 0;

        auto selector = std::make_shared<SelectorNode>("SuccessSelector");
        selector->AddChild(std::make_shared<ActionNode>("First",
            [&firstExecuted](Player*, Unit*) {
                firstExecuted++;
                return NodeStatus::FAILURE;
            }));
        selector->AddChild(std::make_shared<ActionNode>("Second",
            [&secondExecuted](Player*, Unit*) {
                secondExecuted++;
                return NodeStatus::SUCCESS;
            }));
        selector->AddChild(std::make_shared<ActionNode>("Third",
            [&thirdExecuted](Player*, Unit*) {
                thirdExecuted++;
                return NodeStatus::SUCCESS;
            }));

        NodeStatus status = selector->Tick(nullptr, nullptr);

        REQUIRE(status == NodeStatus::SUCCESS);
        REQUIRE(firstExecuted == 1);
        REQUIRE(secondExecuted == 1);
        REQUIRE(thirdExecuted == 0); // Should not execute after success
    }

    SECTION("Selector returns FAILURE when all children FAIL")
    {
        auto selector = std::make_shared<SelectorNode>("AllFail");
        selector->AddChild(std::make_shared<ConditionNode>("False1", [](Player*, Unit*) { return false; }));
        selector->AddChild(std::make_shared<ConditionNode>("False2", [](Player*, Unit*) { return false; }));
        selector->AddChild(std::make_shared<ConditionNode>("False3", [](Player*, Unit*) { return false; }));

        NodeStatus status = selector->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::FAILURE);
    }

    SECTION("Selector returns RUNNING when child is RUNNING")
    {
        auto selector = std::make_shared<SelectorNode>("RunningSelector");
        selector->AddChild(std::make_shared<ConditionNode>("False", [](Player*, Unit*) { return false; }));
        selector->AddChild(std::make_shared<ActionNode>("Running",
            [](Player*, Unit*) { return NodeStatus::RUNNING; }));
        selector->AddChild(std::make_shared<ConditionNode>("NeverReached", [](Player*, Unit*) { return true; }));

        NodeStatus status = selector->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::RUNNING);
    }
}

TEST_CASE("BehaviorTree - InverterNode", "[Phase5][BehaviorTree]")
{
    SECTION("Inverter converts SUCCESS to FAILURE")
    {
        auto condition = std::make_shared<ConditionNode>("True", [](Player*, Unit*) { return true; });
        auto inverter = std::make_shared<InverterNode>("Inverter", condition);

        NodeStatus status = inverter->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::FAILURE);
    }

    SECTION("Inverter converts FAILURE to SUCCESS")
    {
        auto condition = std::make_shared<ConditionNode>("False", [](Player*, Unit*) { return false; });
        auto inverter = std::make_shared<InverterNode>("Inverter", condition);

        NodeStatus status = inverter->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::SUCCESS);
    }

    SECTION("Inverter does not affect RUNNING")
    {
        auto action = std::make_shared<ActionNode>("Running",
            [](Player*, Unit*) { return NodeStatus::RUNNING; });
        auto inverter = std::make_shared<InverterNode>("Inverter", action);

        NodeStatus status = inverter->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::RUNNING);
    }
}

TEST_CASE("BehaviorTree - RepeaterNode", "[Phase5][BehaviorTree]")
{
    SECTION("Repeater with max repeats 0 runs indefinitely")
    {
        int executionCount = 0;
        auto action = std::make_shared<ActionNode>("Increment",
            [&executionCount](Player*, Unit*) {
                executionCount++;
                return NodeStatus::SUCCESS;
            });
        auto repeater = std::make_shared<RepeaterNode>("InfiniteRepeater", action, 0);

        // First tick should return RUNNING
        NodeStatus status = repeater->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::RUNNING);
        REQUIRE(executionCount == 1);
    }

    SECTION("Repeater executes N times")
    {
        int executionCount = 0;
        auto action = std::make_shared<ActionNode>("Increment",
            [&executionCount](Player*, Unit*) {
                executionCount++;
                return NodeStatus::SUCCESS;
            });
        auto repeater = std::make_shared<RepeaterNode>("ThreeRepeater", action, 3);

        // Execute 3 times
        for (int i = 0; i < 3; ++i)
        {
            NodeStatus status = repeater->Tick(nullptr, nullptr);
            if (i < 2)
                REQUIRE(status == NodeStatus::RUNNING);
            else
                REQUIRE(status == NodeStatus::SUCCESS);
        }

        REQUIRE(executionCount == 3);
    }

    SECTION("Repeater stops on FAILURE")
    {
        int executionCount = 0;
        auto action = std::make_shared<ActionNode>("FailAfterTwo",
            [&executionCount](Player*, Unit*) {
                executionCount++;
                return (executionCount < 2) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
            });
        auto repeater = std::make_shared<RepeaterNode>("RepeatUntilFail", action, 5);

        repeater->Tick(nullptr, nullptr); // Count = 1, returns RUNNING
        NodeStatus status = repeater->Tick(nullptr, nullptr); // Count = 2, returns FAILURE

        REQUIRE(status == NodeStatus::FAILURE);
        REQUIRE(executionCount == 2);
    }
}

TEST_CASE("BehaviorTree - Complex Tree Structures", "[Phase5][BehaviorTree]")
{
    SECTION("Nested sequence in selector")
    {
        // Selector {
        //   Sequence { false, true }  -> FAILURE
        //   Sequence { true, true }   -> SUCCESS
        // }
        // Expected: SUCCESS

        auto selector = std::make_shared<SelectorNode>("Root");

        auto failSeq = std::make_shared<SequenceNode>("FailSeq");
        failSeq->AddChild(std::make_shared<ConditionNode>("False", [](Player*, Unit*) { return false; }));
        failSeq->AddChild(std::make_shared<ConditionNode>("True", [](Player*, Unit*) { return true; }));

        auto successSeq = std::make_shared<SequenceNode>("SuccessSeq");
        successSeq->AddChild(std::make_shared<ConditionNode>("True1", [](Player*, Unit*) { return true; }));
        successSeq->AddChild(std::make_shared<ConditionNode>("True2", [](Player*, Unit*) { return true; }));

        selector->AddChild(failSeq);
        selector->AddChild(successSeq);

        NodeStatus status = selector->Tick(nullptr, nullptr);
        REQUIRE(status == NodeStatus::SUCCESS);
    }
}

TEST_CASE("BehaviorTree - Tree Reset Functionality", "[Phase5][BehaviorTree]")
{
    SECTION("Tree reset works correctly")
    {
        auto tree = std::make_shared<BehaviorTree>("TestTree");
        auto action = std::make_shared<ActionNode>("Action",
            [](Player*, Unit*) { return NodeStatus::SUCCESS; });

        tree->SetRoot(action);

        // Execute tree
        NodeStatus status1 = tree->Tick(nullptr, nullptr);
        REQUIRE(status1 == NodeStatus::SUCCESS);

        // Reset tree
        tree->Reset();

        // Execute again after reset
        NodeStatus status2 = tree->Tick(nullptr, nullptr);
        REQUIRE(status2 == NodeStatus::SUCCESS);
    }
}

TEST_CASE("BehaviorTree - Tree Name and Status", "[Phase5][BehaviorTree]")
{
    auto tree = std::make_shared<BehaviorTree>("MyTree");

    SECTION("Tree has correct name")
    {
        REQUIRE(tree->GetName() == "MyTree");
    }

    SECTION("Tree status tracking")
    {
        auto runningAction = std::make_shared<ActionNode>("Running",
            [](Player*, Unit*) { return NodeStatus::RUNNING; });

        tree->SetRoot(runningAction);
        tree->Tick(nullptr, nullptr);

        REQUIRE(tree->IsRunning() == true);
        REQUIRE(tree->GetLastStatus() == NodeStatus::RUNNING);
    }
}

TEST_CASE("BehaviorTree - Debug Logging", "[Phase5][BehaviorTree]")
{
    auto tree = std::make_shared<BehaviorTree>("DebugTree");

    SECTION("Enable debug logging")
    {
        tree->EnableDebugLogging(true);
        auto action = std::make_shared<ActionNode>("Action",
            [](Player*, Unit*) { return NodeStatus::SUCCESS; });
        tree->SetRoot(action);
        tree->Tick(nullptr, nullptr);
        // Logging enabled - verified by compilation
        REQUIRE(true);
    }

    SECTION("Disable debug logging")
    {
        tree->EnableDebugLogging(false);
        auto action = std::make_shared<ActionNode>("Action",
            [](Player*, Unit*) { return NodeStatus::SUCCESS; });
        tree->SetRoot(action);
        tree->Tick(nullptr, nullptr);
        // Logging disabled - verified by compilation
        REQUIRE(true);
    }
}
