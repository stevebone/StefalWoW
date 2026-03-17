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
#include "GameTime.h"
#include "AI/Coordination/RaidOrchestrator.h"

using namespace Playerbot;
using namespace Playerbot::Coordination;

// ============================================================================
// RaidDirective Tests
// ============================================================================

TEST_CASE("RaidDirective: Activity tracking", "[coordination][raid][directive]")
{
    RaidDirective directive;
    directive.directiveType = "test";
    directive.priority = 50;
    directive.timestamp = GameTime::GetGameTimeMS();
    directive.duration = 5000; // 5s

    SECTION("Directive is active within duration")
    {
        REQUIRE(directive.IsActive());
    }

    SECTION("Directive parameters can be set")
    {
        directive.parameters["value1"] = 10.0f;
        directive.parameters["value2"] = 20.5f;

        REQUIRE(directive.parameters["value1"] == 10.0f);
        REQUIRE(directive.parameters["value2"] == 20.5f);
    }
}

// ============================================================================
// RaidOrchestrator Tests
// ============================================================================

TEST_CASE("RaidOrchestrator: Basic functionality", "[coordination][raid]")
{
    SECTION("Update with null raid doesn't crash")
    {
        RaidOrchestrator orchestrator(nullptr);
        REQUIRE_NOTHROW(orchestrator.Update(1000));
    }

    SECTION("Initially not in combat")
    {
        RaidOrchestrator orchestrator(nullptr);
        REQUIRE_FALSE(orchestrator.IsInCombat());
    }

    SECTION("Initially no groups")
    {
        RaidOrchestrator orchestrator(nullptr);
        REQUIRE(orchestrator.GetGroupCount() == 0);
    }

    SECTION("Initially defensive formation")
    {
        RaidOrchestrator orchestrator(nullptr);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::DEFENSIVE);
    }

    SECTION("Initially normal phase")
    {
        RaidOrchestrator orchestrator(nullptr);
        REQUIRE(orchestrator.GetEncounterPhase() == EncounterPhase::NORMAL);
    }
}

TEST_CASE("RaidOrchestrator: Directive management", "[coordination][raid][directive]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Can issue directives")
    {
        RaidDirective directive;
        directive.directiveType = "spread";
        directive.priority = 80;
        directive.timestamp = GameTime::GetGameTimeMS();
        directive.duration = 10000;

        orchestrator.IssueDirective(directive);

        auto active = orchestrator.GetActiveDirectives();
        REQUIRE(active.size() == 1);
        REQUIRE(active[0].directiveType == "spread");
    }

    SECTION("Multiple directives can be issued")
    {
        RaidDirective d1;
        d1.directiveType = "spread";
        d1.priority = 80;
        d1.timestamp = GameTime::GetGameTimeMS();
        d1.duration = 10000;

        RaidDirective d2;
        d2.directiveType = "bloodlust";
        d2.priority = 100;
        d2.timestamp = GameTime::GetGameTimeMS();
        d2.duration = 40000;

        orchestrator.IssueDirective(d1);
        orchestrator.IssueDirective(d2);

        auto active = orchestrator.GetActiveDirectives();
        REQUIRE(active.size() == 2);
    }

    SECTION("Expired directives are filtered out")
    {
        RaidDirective directive;
        directive.directiveType = "test";
        directive.priority = 50;
        directive.timestamp = GameTime::GetGameTimeMS() - 20000; // 20s ago
        directive.duration = 5000; // 5s duration (expired)

        orchestrator.IssueDirective(directive);

        auto active = orchestrator.GetActiveDirectives();
        REQUIRE(active.empty());
    }
}

TEST_CASE("RaidOrchestrator: Formation management", "[coordination][raid][formation]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Can change formation")
    {
        orchestrator.SetFormation(RaidFormation::SPREAD);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::SPREAD);
    }

    SECTION("Formation change issues directive")
    {
        orchestrator.SetFormation(RaidFormation::STACKED);

        auto directives = orchestrator.GetActiveDirectives();
        bool found = false;
        for (auto const& d : directives)
        {
            if (d.directiveType == "formation_change")
            {
                found = true;
                break;
            }
        }
        REQUIRE(found);
    }

    SECTION("All formation types are supported")
    {
        orchestrator.SetFormation(RaidFormation::SPREAD);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::SPREAD);

        orchestrator.SetFormation(RaidFormation::STACKED);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::STACKED);

        orchestrator.SetFormation(RaidFormation::RANGED_SPLIT);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::RANGED_SPLIT);

        orchestrator.SetFormation(RaidFormation::MELEE_HEAVY);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::MELEE_HEAVY);

        orchestrator.SetFormation(RaidFormation::DEFENSIVE);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::DEFENSIVE);
    }
}

TEST_CASE("RaidOrchestrator: Encounter phase management", "[coordination][raid][phase]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Can change encounter phase")
    {
        orchestrator.SetEncounterPhase(EncounterPhase::BURN);
        REQUIRE(orchestrator.GetEncounterPhase() == EncounterPhase::BURN);
    }

    SECTION("All phases are supported")
    {
        orchestrator.SetEncounterPhase(EncounterPhase::NORMAL);
        REQUIRE(orchestrator.GetEncounterPhase() == EncounterPhase::NORMAL);

        orchestrator.SetEncounterPhase(EncounterPhase::BURN);
        REQUIRE(orchestrator.GetEncounterPhase() == EncounterPhase::BURN);

        orchestrator.SetEncounterPhase(EncounterPhase::ADD_PHASE);
        REQUIRE(orchestrator.GetEncounterPhase() == EncounterPhase::ADD_PHASE);

        orchestrator.SetEncounterPhase(EncounterPhase::TRANSITION);
        REQUIRE(orchestrator.GetEncounterPhase() == EncounterPhase::TRANSITION);

        orchestrator.SetEncounterPhase(EncounterPhase::INTERMISSION);
        REQUIRE(orchestrator.GetEncounterPhase() == EncounterPhase::INTERMISSION);
    }
}

TEST_CASE("RaidOrchestrator: Bloodlust management", "[coordination][raid][bloodlust]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Initially bloodlust not active")
    {
        REQUIRE_FALSE(orchestrator.IsBloodlustActive());
    }

    SECTION("Can request bloodlust")
    {
        bool success = orchestrator.RequestBloodlust();
        REQUIRE(success);
        REQUIRE(orchestrator.IsBloodlustActive());
    }

    SECTION("Bloodlust request issues directive")
    {
        orchestrator.RequestBloodlust();

        auto directives = orchestrator.GetActiveDirectives();
        bool found = false;
        for (auto const& d : directives)
        {
            if (d.directiveType == "bloodlust")
            {
                found = true;
                REQUIRE(d.priority == 100); // Highest priority
                break;
            }
        }
        REQUIRE(found);
    }

    SECTION("Cannot activate bloodlust when already active")
    {
        orchestrator.RequestBloodlust();
        REQUIRE(orchestrator.IsBloodlustActive());

        bool success = orchestrator.RequestBloodlust();
        REQUIRE_FALSE(success); // Second request fails
    }
}

TEST_CASE("RaidOrchestrator: Defensive cooldown management", "[coordination][raid][cooldown]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Can request defensive cooldown")
    {
        bool success = orchestrator.RequestRaidDefensiveCooldown("barrier");
        REQUIRE(success);
    }

    SECTION("Defensive CD request issues directive")
    {
        orchestrator.RequestRaidDefensiveCooldown("aura");

        auto directives = orchestrator.GetActiveDirectives();
        bool found = false;
        for (auto const& d : directives)
        {
            if (d.directiveType == "defensive_cd")
            {
                found = true;
                REQUIRE(d.priority == 90);
                break;
            }
        }
        REQUIRE(found);
    }

    SECTION("Different cooldown types are tracked independently")
    {
        bool success1 = orchestrator.RequestRaidDefensiveCooldown("barrier");
        bool success2 = orchestrator.RequestRaidDefensiveCooldown("aura");

        REQUIRE(success1);
        REQUIRE(success2);
    }

    SECTION("Same cooldown type respects cooldown")
    {
        orchestrator.RequestRaidDefensiveCooldown("barrier");
        bool success = orchestrator.RequestRaidDefensiveCooldown("barrier");
        REQUIRE_FALSE(success); // On cooldown
    }
}

TEST_CASE("RaidOrchestrator: Add priority management", "[coordination][raid][adds]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Initially no add priorities")
    {
        auto priorities = orchestrator.GetAddPriorities();
        REQUIRE(priorities.empty());
    }

    SECTION("Can set add priorities")
    {
        std::vector<ObjectGuid> adds;
        adds.push_back(ObjectGuid::Create<HighGuid::Creature>(0, 1, 1));
        adds.push_back(ObjectGuid::Create<HighGuid::Creature>(0, 2, 1));
        adds.push_back(ObjectGuid::Create<HighGuid::Creature>(0, 3, 1));

        orchestrator.SetAddPriorities(adds);

        auto priorities = orchestrator.GetAddPriorities();
        REQUIRE(priorities.size() == 3);
        REQUIRE(priorities[0] == adds[0]);
        REQUIRE(priorities[1] == adds[1]);
        REQUIRE(priorities[2] == adds[2]);
    }

    SECTION("Add priorities can be updated")
    {
        std::vector<ObjectGuid> adds1;
        adds1.push_back(ObjectGuid::Create<HighGuid::Creature>(0, 1, 1));

        std::vector<ObjectGuid> adds2;
        adds2.push_back(ObjectGuid::Create<HighGuid::Creature>(0, 2, 1));
        adds2.push_back(ObjectGuid::Create<HighGuid::Creature>(0, 3, 1));

        orchestrator.SetAddPriorities(adds1);
        REQUIRE(orchestrator.GetAddPriorities().size() == 1);

        orchestrator.SetAddPriorities(adds2);
        REQUIRE(orchestrator.GetAddPriorities().size() == 2);
    }
}

TEST_CASE("RaidOrchestrator: Raid statistics", "[coordination][raid][stats]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Can retrieve raid stats")
    {
        auto stats = orchestrator.GetRaidStats();
        REQUIRE(stats.totalBots >= 0);
        REQUIRE(stats.aliveBots >= 0);
        REQUIRE(stats.deadBots >= 0);
    }

    SECTION("Stats are consistent")
    {
        auto stats = orchestrator.GetRaidStats();
        REQUIRE(stats.totalBots == stats.aliveBots + stats.deadBots);
    }
}

TEST_CASE("RaidOrchestrator: Combat duration", "[coordination][raid][combat]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Combat duration is zero when not in combat")
    {
        REQUIRE(orchestrator.GetCombatDuration() == 0);
    }
}

TEST_CASE("RaidOrchestrator: Group coordinator access", "[coordination][raid][groups]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("GetGroupCoordinator returns nullptr for invalid index")
    {
        REQUIRE(orchestrator.GetGroupCoordinator(99) == nullptr);
    }

    SECTION("GetGroupCoordinator returns nullptr when no groups")
    {
        REQUIRE(orchestrator.GetGroupCoordinator(0) == nullptr);
    }
}

TEST_CASE("RaidOrchestrator: Role coordinator access", "[coordination][raid][roles]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Role coordinator manager is accessible")
    {
        auto* manager = orchestrator.GetRoleCoordinatorManager();
        REQUIRE(manager != nullptr);
    }

    SECTION("All role coordinators are available")
    {
        auto* manager = orchestrator.GetRoleCoordinatorManager();
        REQUIRE(manager->GetTankCoordinator() != nullptr);
        REQUIRE(manager->GetHealerCoordinator() != nullptr);
        REQUIRE(manager->GetDPSCoordinator() != nullptr);
    }
}

// ============================================================================
// BossEncounterStrategy Tests
// ============================================================================

TEST_CASE("BossEncounterStrategy: Default phase detection", "[coordination][raid][boss]")
{
    class TestStrategy : public BossEncounterStrategy
    {
    public:
        uint32 GetBossEntry() const override { return 12345; }
        void Execute(RaidOrchestrator*, EncounterPhase) override {}
    };

    TestStrategy strategy;

    SECTION("Burn phase at <20% health")
    {
        REQUIRE(strategy.DetectPhase(15.0f) == EncounterPhase::BURN);
    }

    SECTION("Normal phase at >20% health")
    {
        REQUIRE(strategy.DetectPhase(50.0f) == EncounterPhase::NORMAL);
    }

    SECTION("Burn phase threshold at exactly 20%")
    {
        auto phase = strategy.DetectPhase(20.0f);
        // Either NORMAL or BURN is acceptable at threshold
        REQUIRE((phase == EncounterPhase::NORMAL || phase == EncounterPhase::BURN));
    }
}

// ============================================================================
// BossStrategyRegistry Tests
// ============================================================================

TEST_CASE("BossStrategyRegistry: Strategy registration", "[coordination][raid][boss][registry]")
{
    BossStrategyRegistry::Clear();

    SECTION("Can register boss strategy")
    {
        class TestStrategy : public BossEncounterStrategy
        {
        public:
            uint32 GetBossEntry() const override { return 12345; }
            void Execute(RaidOrchestrator*, EncounterPhase) override {}
        };

        auto strategy = std::make_shared<TestStrategy>();
        BossStrategyRegistry::RegisterStrategy(12345, strategy);

        auto retrieved = BossStrategyRegistry::GetStrategy(12345);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->GetBossEntry() == 12345);
    }

    SECTION("Returns nullptr for unregistered boss")
    {
        auto strategy = BossStrategyRegistry::GetStrategy(99999);
        REQUIRE(strategy == nullptr);
    }

    SECTION("Can register multiple strategies")
    {
        class Strategy1 : public BossEncounterStrategy
        {
        public:
            uint32 GetBossEntry() const override { return 111; }
            void Execute(RaidOrchestrator*, EncounterPhase) override {}
        };

        class Strategy2 : public BossEncounterStrategy
        {
        public:
            uint32 GetBossEntry() const override { return 222; }
            void Execute(RaidOrchestrator*, EncounterPhase) override {}
        };

        BossStrategyRegistry::RegisterStrategy(111, std::make_shared<Strategy1>());
        BossStrategyRegistry::RegisterStrategy(222, std::make_shared<Strategy2>());

        REQUIRE(BossStrategyRegistry::GetStrategy(111) != nullptr);
        REQUIRE(BossStrategyRegistry::GetStrategy(222) != nullptr);
    }

    SECTION("Clear removes all strategies")
    {
        class TestStrategy : public BossEncounterStrategy
        {
        public:
            uint32 GetBossEntry() const override { return 12345; }
            void Execute(RaidOrchestrator*, EncounterPhase) override {}
        };

        BossStrategyRegistry::RegisterStrategy(12345, std::make_shared<TestStrategy>());
        REQUIRE(BossStrategyRegistry::GetStrategy(12345) != nullptr);

        BossStrategyRegistry::Clear();
        REQUIRE(BossStrategyRegistry::GetStrategy(12345) == nullptr);
    }

    BossStrategyRegistry::Clear(); // Cleanup
}

// ============================================================================
// OnyxiaStrategy Tests
// ============================================================================

TEST_CASE("OnyxiaStrategy: Boss entry", "[coordination][raid][boss][onyxia]")
{
    OnyxiaStrategy strategy;

    SECTION("Returns correct boss entry")
    {
        REQUIRE(strategy.GetBossEntry() == 10184); // Onyxia
    }
}

TEST_CASE("OnyxiaStrategy: Phase detection", "[coordination][raid][boss][onyxia]")
{
    OnyxiaStrategy strategy;

    SECTION("Phase 1 (ground) at high health")
    {
        REQUIRE(strategy.DetectPhase(80.0f) == EncounterPhase::NORMAL);
    }

    SECTION("Phase 2 (air) at 65-40% health")
    {
        REQUIRE(strategy.DetectPhase(50.0f) == EncounterPhase::TRANSITION);
    }

    SECTION("Phase 3 (burn) at <40% health")
    {
        REQUIRE(strategy.DetectPhase(30.0f) == EncounterPhase::BURN);
    }

    SECTION("Phase boundaries are correct")
    {
        // >65% = Phase 1
        REQUIRE(strategy.DetectPhase(66.0f) == EncounterPhase::NORMAL);

        // <65% = Phase 2
        REQUIRE(strategy.DetectPhase(64.0f) == EncounterPhase::TRANSITION);

        // <40% = Phase 3
        REQUIRE(strategy.DetectPhase(39.0f) == EncounterPhase::BURN);
    }
}

TEST_CASE("OnyxiaStrategy: Execution", "[coordination][raid][boss][onyxia]")
{
    OnyxiaStrategy strategy;
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Phase 1 sets defensive formation")
    {
        strategy.Execute(&orchestrator, EncounterPhase::NORMAL);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::DEFENSIVE);
    }

    SECTION("Phase 2 sets spread formation")
    {
        strategy.Execute(&orchestrator, EncounterPhase::TRANSITION);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::SPREAD);
    }

    SECTION("Phase 3 activates bloodlust and defensive formation")
    {
        strategy.Execute(&orchestrator, EncounterPhase::BURN);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::DEFENSIVE);
        REQUIRE(orchestrator.IsBloodlustActive());
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE("RaidOrchestrator: Complete raid scenario", "[coordination][raid][integration]")
{
    BossStrategyRegistry::Clear();

    RaidOrchestrator orchestrator(nullptr);

    SECTION("Full encounter flow")
    {
        // Setup
        orchestrator.SetFormation(RaidFormation::DEFENSIVE);
        REQUIRE(orchestrator.GetFormation() == RaidFormation::DEFENSIVE);

        // Normal phase
        orchestrator.SetEncounterPhase(EncounterPhase::NORMAL);
        REQUIRE(orchestrator.GetEncounterPhase() == EncounterPhase::NORMAL);

        // Add phase
        orchestrator.SetEncounterPhase(EncounterPhase::ADD_PHASE);
        std::vector<ObjectGuid> adds;
        adds.push_back(ObjectGuid::Create<HighGuid::Creature>(0, 1, 1));
        orchestrator.SetAddPriorities(adds);
        REQUIRE(orchestrator.GetAddPriorities().size() == 1);

        // Burn phase
        orchestrator.SetEncounterPhase(EncounterPhase::BURN);
        orchestrator.RequestBloodlust();
        REQUIRE(orchestrator.IsBloodlustActive());
    }

    BossStrategyRegistry::Clear();
}

TEST_CASE("RaidOrchestrator: Performance characteristics", "[coordination][raid][performance]")
{
    RaidOrchestrator orchestrator(nullptr);

    SECTION("Can handle many directives")
    {
        for (uint32 i = 0; i < 100; ++i)
        {
            RaidDirective directive;
            directive.directiveType = "test";
            directive.priority = 50;
            directive.timestamp = GameTime::GetGameTimeMS();
            directive.duration = 10000;

            orchestrator.IssueDirective(directive);
        }

        auto active = orchestrator.GetActiveDirectives();
        REQUIRE(active.size() == 100);
    }

    SECTION("Multiple rapid updates don't corrupt state")
    {
        orchestrator.SetFormation(RaidFormation::SPREAD);

        for (uint32 i = 0; i < 1000; ++i)
        {
            orchestrator.Update(10);
        }

        REQUIRE(orchestrator.GetFormation() == RaidFormation::SPREAD);
    }
}
