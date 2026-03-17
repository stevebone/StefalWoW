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
#include "AI/BehaviorTree/BehaviorTree.h"

using namespace Playerbot;

TEST_CASE("BehaviorTree: Blackboard stores and retrieves typed data correctly", "[behavior-tree][blackboard]")
{
    BTBlackboard blackboard;

    SECTION("Store and retrieve int")
    {
        blackboard.Set<int>("TestInt", 42);

        int value = 0;
        REQUIRE(blackboard.Get<int>("TestInt", value));
        REQUIRE(value == 42);
    }

    SECTION("Store and retrieve float")
    {
        blackboard.Set<float>("TestFloat", 3.14f);

        float value = 0.0f;
        REQUIRE(blackboard.Get<float>("TestFloat", value));
        REQUIRE(value == 3.14f);
    }

    SECTION("Store and retrieve string")
    {
        blackboard.Set<std::string>("TestString", "Hello World");

        std::string value;
        REQUIRE(blackboard.Get<std::string>("TestString", value));
        REQUIRE(value == "Hello World");
    }

    SECTION("GetOr returns default if key doesn't exist")
    {
        int value = blackboard.GetOr<int>("NonExistent", 99);
        REQUIRE(value == 99);
    }

    SECTION("GetOr returns stored value if key exists")
    {
        blackboard.Set<int>("Existing", 42);
        int value = blackboard.GetOr<int>("Existing", 99);
        REQUIRE(value == 42);
    }

    SECTION("Has returns true for existing key")
    {
        blackboard.Set<int>("TestKey", 123);
        REQUIRE(blackboard.Has("TestKey"));
    }

    SECTION("Has returns false for non-existing key")
    {
        REQUIRE_FALSE(blackboard.Has("NonExistent"));
    }

    SECTION("Remove deletes key")
    {
        blackboard.Set<int>("ToRemove", 456);
        REQUIRE(blackboard.Has("ToRemove"));

        blackboard.Remove("ToRemove");
        REQUIRE_FALSE(blackboard.Has("ToRemove"));
    }

    SECTION("Clear removes all keys")
    {
        blackboard.Set<int>("Key1", 1);
        blackboard.Set<float>("Key2", 2.0f);
        blackboard.Set<std::string>("Key3", "three");

        blackboard.Clear();

        REQUIRE_FALSE(blackboard.Has("Key1"));
        REQUIRE_FALSE(blackboard.Has("Key2"));
        REQUIRE_FALSE(blackboard.Has("Key3"));
    }

    SECTION("Type mismatch returns false")
    {
        blackboard.Set<int>("TypeTest", 42);

        float value = 0.0f;
        REQUIRE_FALSE(blackboard.Get<float>("TypeTest", value));
    }
}

TEST_CASE("BehaviorTree: Condition node evaluates correctly", "[behavior-tree][condition]")
{
    BTBlackboard blackboard;

    SECTION("Condition returns SUCCESS when true")
    {
        BTCondition condition("AlwaysTrue", [](BotAI* ai, BTBlackboard& bb) { return true; });

        BTStatus status = condition.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);
    }

    SECTION("Condition returns FAILURE when false")
    {
        BTCondition condition("AlwaysFalse", [](BotAI* ai, BTBlackboard& bb) { return false; });

        BTStatus status = condition.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::FAILURE);
    }

    SECTION("Condition can read from blackboard")
    {
        blackboard.Set<int>("Value", 10);

        BTCondition condition("CheckValue", [](BotAI* ai, BTBlackboard& bb) {
            int value = 0;
            return bb.Get<int>("Value", value) && value > 5;
        });

        BTStatus status = condition.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);
    }
}

TEST_CASE("BehaviorTree: Action node executes correctly", "[behavior-tree][action]")
{
    BTBlackboard blackboard;

    SECTION("Action returns result from function")
    {
        BTAction action("SetValue", [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            bb.Set<int>("Result", 42);
            return BTStatus::SUCCESS;
        });

        BTStatus status = action.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);

        int value = 0;
        REQUIRE(blackboard.Get<int>("Result", value));
        REQUIRE(value == 42);
    }

    SECTION("Action can return RUNNING")
    {
        BTAction action("LongAction", [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            return BTStatus::RUNNING;
        });

        BTStatus status = action.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::RUNNING);
    }

    SECTION("Action can return FAILURE")
    {
        BTAction action("FailAction", [](BotAI* ai, BTBlackboard& bb) -> BTStatus {
            return BTStatus::FAILURE;
        });

        BTStatus status = action.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::FAILURE);
    }
}

TEST_CASE("BehaviorTree: Sequence node executes children in order", "[behavior-tree][sequence]")
{
    BTBlackboard blackboard;

    SECTION("Sequence returns SUCCESS when all children succeed")
    {
        auto sequence = std::make_shared<BTSequence>("TestSequence");

        sequence->AddChild(std::make_shared<BTCondition>("Check1", [](auto, auto&) { return true; }));
        sequence->AddChild(std::make_shared<BTCondition>("Check2", [](auto, auto&) { return true; }));
        sequence->AddChild(std::make_shared<BTCondition>("Check3", [](auto, auto&) { return true; }));

        BTStatus status = sequence->Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);
    }

    SECTION("Sequence returns FAILURE on first failure")
    {
        auto sequence = std::make_shared<BTSequence>("TestSequence");

        sequence->AddChild(std::make_shared<BTCondition>("Check1", [](auto, auto&) { return true; }));
        sequence->AddChild(std::make_shared<BTCondition>("Check2", [](auto, auto&) { return false; }));
        sequence->AddChild(std::make_shared<BTCondition>("Check3", [](auto, auto&) { return true; }));

        BTStatus status = sequence->Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::FAILURE);
    }

    SECTION("Sequence returns RUNNING when child is running")
    {
        auto sequence = std::make_shared<BTSequence>("TestSequence");

        sequence->AddChild(std::make_shared<BTCondition>("Check1", [](auto, auto&) { return true; }));
        sequence->AddChild(std::make_shared<BTAction>("Action", [](auto, auto&) { return BTStatus::RUNNING; }));
        sequence->AddChild(std::make_shared<BTCondition>("Check2", [](auto, auto&) { return true; }));

        BTStatus status = sequence->Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::RUNNING);
    }

    SECTION("Sequence executes children in order")
    {
        auto sequence = std::make_shared<BTSequence>("TestSequence");
        blackboard.Set<int>("Counter", 0);

        sequence->AddChild(std::make_shared<BTAction>("Inc1", [](BotAI* ai, BTBlackboard& bb) {
            int count = bb.GetOr<int>("Counter", 0);
            bb.Set<int>("Counter", count + 1);
            return BTStatus::SUCCESS;
        }));

        sequence->AddChild(std::make_shared<BTAction>("Inc2", [](BotAI* ai, BTBlackboard& bb) {
            int count = bb.GetOr<int>("Counter", 0);
            bb.Set<int>("Counter", count + 10);
            return BTStatus::SUCCESS;
        }));

        BTStatus status = sequence->Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);

        int count = blackboard.GetOr<int>("Counter", 0);
        REQUIRE(count == 11); // 0 + 1 + 10
    }
}

TEST_CASE("BehaviorTree: Selector node tries children until one succeeds", "[behavior-tree][selector]")
{
    BTBlackboard blackboard;

    SECTION("Selector returns SUCCESS on first success")
    {
        auto selector = std::make_shared<BTSelector>("TestSelector");

        selector->AddChild(std::make_shared<BTCondition>("Check1", [](auto, auto&) { return false; }));
        selector->AddChild(std::make_shared<BTCondition>("Check2", [](auto, auto&) { return true; }));
        selector->AddChild(std::make_shared<BTCondition>("Check3", [](auto, auto&) { return false; }));

        BTStatus status = selector->Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);
    }

    SECTION("Selector returns FAILURE when all children fail")
    {
        auto selector = std::make_shared<BTSelector>("TestSelector");

        selector->AddChild(std::make_shared<BTCondition>("Check1", [](auto, auto&) { return false; }));
        selector->AddChild(std::make_shared<BTCondition>("Check2", [](auto, auto&) { return false; }));
        selector->AddChild(std::make_shared<BTCondition>("Check3", [](auto, auto&) { return false; }));

        BTStatus status = selector->Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::FAILURE);
    }

    SECTION("Selector returns RUNNING when child is running")
    {
        auto selector = std::make_shared<BTSelector>("TestSelector");

        selector->AddChild(std::make_shared<BTCondition>("Check1", [](auto, auto&) { return false; }));
        selector->AddChild(std::make_shared<BTAction>("Action", [](auto, auto&) { return BTStatus::RUNNING; }));
        selector->AddChild(std::make_shared<BTCondition>("Check2", [](auto, auto&) { return true; }));

        BTStatus status = selector->Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::RUNNING);
    }

    SECTION("Selector doesn't execute children after first success")
    {
        auto selector = std::make_shared<BTSelector>("TestSelector");
        blackboard.Set<int>("Counter", 0);

        selector->AddChild(std::make_shared<BTCondition>("Fail", [](auto, auto&) { return false; }));

        selector->AddChild(std::make_shared<BTAction>("Success", [](BotAI* ai, BTBlackboard& bb) {
            int count = bb.GetOr<int>("Counter", 0);
            bb.Set<int>("Counter", count + 1);
            return BTStatus::SUCCESS;
        }));

        selector->AddChild(std::make_shared<BTAction>("ShouldNotRun", [](BotAI* ai, BTBlackboard& bb) {
            int count = bb.GetOr<int>("Counter", 0);
            bb.Set<int>("Counter", count + 100);
            return BTStatus::SUCCESS;
        }));

        BTStatus status = selector->Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);

        int count = blackboard.GetOr<int>("Counter", 0);
        REQUIRE(count == 1); // Should NOT be 101
    }
}

TEST_CASE("BehaviorTree: Inverter decorator inverts child result", "[behavior-tree][inverter]")
{
    BTBlackboard blackboard;

    SECTION("Inverter inverts SUCCESS to FAILURE")
    {
        auto child = std::make_shared<BTCondition>("AlwaysTrue", [](auto, auto&) { return true; });
        BTInverter inverter("Invert", child);

        BTStatus status = inverter.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::FAILURE);
    }

    SECTION("Inverter inverts FAILURE to SUCCESS")
    {
        auto child = std::make_shared<BTCondition>("AlwaysFalse", [](auto, auto&) { return false; });
        BTInverter inverter("Invert", child);

        BTStatus status = inverter.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);
    }

    SECTION("Inverter passes through RUNNING")
    {
        auto child = std::make_shared<BTAction>("Running", [](auto, auto&) { return BTStatus::RUNNING; });
        BTInverter inverter("Invert", child);

        BTStatus status = inverter.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::RUNNING);
    }

    SECTION("Inverter passes through INVALID")
    {
        auto child = std::make_shared<BTAction>("Invalid", [](auto, auto&) { return BTStatus::INVALID; });
        BTInverter inverter("Invert", child);

        BTStatus status = inverter.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::INVALID);
    }
}

TEST_CASE("BehaviorTree: Repeater decorator repeats child N times", "[behavior-tree][repeater]")
{
    BTBlackboard blackboard;

    SECTION("Repeater repeats child specified times")
    {
        blackboard.Set<int>("Counter", 0);

        auto child = std::make_shared<BTAction>("Increment", [](BotAI* ai, BTBlackboard& bb) {
            int count = bb.GetOr<int>("Counter", 0);
            bb.Set<int>("Counter", count + 1);
            return BTStatus::SUCCESS;
        });

        BTRepeater repeater("Repeat3", child, 3);

        BTStatus status = repeater.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::SUCCESS);

        int count = blackboard.GetOr<int>("Counter", 0);
        REQUIRE(count == 3);
    }

    SECTION("Repeater stops on child failure")
    {
        blackboard.Set<int>("Counter", 0);

        auto child = std::make_shared<BTAction>("FailAfter2", [](BotAI* ai, BTBlackboard& bb) {
            int count = bb.GetOr<int>("Counter", 0);
            bb.Set<int>("Counter", count + 1);
            return (count < 2) ? BTStatus::SUCCESS : BTStatus::FAILURE;
        });

        BTRepeater repeater("Repeat5", child, 5);

        BTStatus status = repeater.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::FAILURE);

        int count = blackboard.GetOr<int>("Counter", 0);
        REQUIRE(count == 3); // Failed on 3rd iteration
    }

    SECTION("Repeater infinite loop with negative count")
    {
        blackboard.Set<int>("Counter", 0);

        auto child = std::make_shared<BTAction>("FailAfter5", [](BotAI* ai, BTBlackboard& bb) {
            int count = bb.GetOr<int>("Counter", 0);
            bb.Set<int>("Counter", count + 1);
            return (count < 5) ? BTStatus::SUCCESS : BTStatus::FAILURE;
        });

        BTRepeater repeater("RepeatInfinite", child, -1);

        BTStatus status = repeater.Tick(nullptr, blackboard);
        REQUIRE(status == BTStatus::FAILURE);

        int count = blackboard.GetOr<int>("Counter", 0);
        REQUIRE(count == 6); // Stopped when child failed
    }
}

TEST_CASE("BehaviorTree: Complete tree execution", "[behavior-tree][integration]")
{
    SECTION("Simple combat tree")
    {
        BehaviorTree tree;
        BTBlackboard& blackboard = tree.GetBlackboard();

        blackboard.Set<bool>("HasTarget", true);
        blackboard.Set<float>("Distance", 5.0f);
        blackboard.Set<float>("Health", 0.8f);

        // Root: Selector (try combat or flee)
        auto root = std::make_shared<BTSelector>("Root");

        // Combat branch: Sequence (check target -> check range -> attack)
        auto combatSequence = std::make_shared<BTSequence>("Combat");

        combatSequence->AddChild(std::make_shared<BTCondition>("HasTarget",
            [](BotAI* ai, BTBlackboard& bb) {
                return bb.GetOr<bool>("HasTarget", false);
            }
        ));

        combatSequence->AddChild(std::make_shared<BTCondition>("InRange",
            [](BotAI* ai, BTBlackboard& bb) {
                float distance = bb.GetOr<float>("Distance", 999.0f);
                return distance < 10.0f;
            }
        ));

        combatSequence->AddChild(std::make_shared<BTAction>("Attack",
            [](BotAI* ai, BTBlackboard& bb) {
                bb.Set<bool>("Attacking", true);
                return BTStatus::SUCCESS;
            }
        ));

        root->AddChild(combatSequence);

        // Flee branch: Condition (low health)
        root->AddChild(std::make_shared<BTCondition>("Flee",
            [](BotAI* ai, BTBlackboard& bb) {
                float health = bb.GetOr<float>("Health", 1.0f);
                return health < 0.3f;
            }
        ));

        tree.SetRoot(root);

        BTStatus status = tree.Tick(nullptr);
        REQUIRE(status == BTStatus::SUCCESS);
        REQUIRE(blackboard.GetOr<bool>("Attacking", false) == true);
    }

    SECTION("Flee when low health")
    {
        BehaviorTree tree;
        BTBlackboard& blackboard = tree.GetBlackboard();

        blackboard.Set<bool>("HasTarget", true);
        blackboard.Set<float>("Distance", 5.0f);
        blackboard.Set<float>("Health", 0.2f); // Low health

        auto root = std::make_shared<BTSelector>("Root");

        // Flee branch checked first
        root->AddChild(std::make_shared<BTCondition>("Flee",
            [](BotAI* ai, BTBlackboard& bb) {
                float health = bb.GetOr<float>("Health", 1.0f);
                if (health < 0.3f)
                {
                    bb.Set<bool>("Fleeing", true);
                    return true;
                }
                return false;
            }
        ));

        // Combat branch
        auto combatSequence = std::make_shared<BTSequence>("Combat");
        combatSequence->AddChild(std::make_shared<BTCondition>("HasTarget",
            [](BotAI* ai, BTBlackboard& bb) { return bb.GetOr<bool>("HasTarget", false); }
        ));
        combatSequence->AddChild(std::make_shared<BTAction>("Attack",
            [](BotAI* ai, BTBlackboard& bb) {
                bb.Set<bool>("Attacking", true);
                return BTStatus::SUCCESS;
            }
        ));

        root->AddChild(combatSequence);

        tree.SetRoot(root);

        BTStatus status = tree.Tick(nullptr);
        REQUIRE(status == BTStatus::SUCCESS);
        REQUIRE(blackboard.GetOr<bool>("Fleeing", false) == true);
        REQUIRE(blackboard.GetOr<bool>("Attacking", false) == false); // Should NOT attack
    }

    SECTION("Tree resets properly")
    {
        BehaviorTree tree;
        BTBlackboard& blackboard = tree.GetBlackboard();

        blackboard.Set<int>("Value", 42);

        auto root = std::make_shared<BTCondition>("Test",
            [](BotAI* ai, BTBlackboard& bb) { return bb.Has("Value"); }
        );

        tree.SetRoot(root);

        BTStatus status1 = tree.Tick(nullptr);
        REQUIRE(status1 == BTStatus::SUCCESS);

        tree.Reset();

        REQUIRE_FALSE(blackboard.Has("Value"));
    }
}
