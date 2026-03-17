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
#include "AI/Coordination/ZoneOrchestrator.h"

using namespace Playerbot;
using namespace Playerbot::Coordination;

// ============================================================================
// ZoneObjective Tests
// ============================================================================

TEST_CASE("ZoneObjective: Activity and completion tracking", "[coordination][zone][objective]")
{
    ZoneObjective objective;
    objective.objectiveType = "kill_boss";
    objective.priority = 90;
    objective.assignedBots = 0;
    objective.requiredBots = 40;
    objective.timestamp = GameTime::GetGameTimeMS();
    objective.expirationTime = objective.timestamp + 60000; // 1 minute

    SECTION("Objective is active when not expired and not complete")
    {
        REQUIRE(objective.IsActive());
        REQUIRE_FALSE(objective.IsComplete());
    }

    SECTION("Objective completes when enough bots assigned")
    {
        objective.assignedBots = 40;
        REQUIRE(objective.IsComplete());
    }

    SECTION("Objective is not active when complete")
    {
        objective.assignedBots = 40;
        REQUIRE_FALSE(objective.IsActive());
    }
}

// ============================================================================
// ZoneOrchestrator Tests
// ============================================================================

TEST_CASE("ZoneOrchestrator: Basic functionality", "[coordination][zone]")
{
    ZoneOrchestrator orchestrator(1519); // Stormwind

    SECTION("Returns correct zone ID")
    {
        REQUIRE(orchestrator.GetZoneId() == 1519);
    }

    SECTION("Initially no bots")
    {
        REQUIRE(orchestrator.GetBotCount() == 0);
        REQUIRE(orchestrator.GetBots().empty());
    }

    SECTION("Initially no raids")
    {
        REQUIRE(orchestrator.GetRaidCount() == 0);
    }

    SECTION("Initially IDLE activity")
    {
        REQUIRE(orchestrator.GetActivity() == ZoneActivity::IDLE);
    }

    SECTION("Initially PEACEFUL threat level")
    {
        REQUIRE(orchestrator.GetThreatLevel() == ThreatLevel::PEACEFUL);
    }
}

TEST_CASE("ZoneOrchestrator: Bot registration", "[coordination][zone][bot]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Can register bot")
    {
        orchestrator.RegisterBot(nullptr); // Would normally be a Player*
        // With nullptr, registration is skipped
        REQUIRE(orchestrator.GetBotCount() == 0);
    }

    SECTION("Can unregister bot")
    {
        ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(0, 1);
        orchestrator.UnregisterBot(botGuid);
        // Should handle gracefully even if bot wasn't registered
        REQUIRE(orchestrator.GetBotCount() == 0);
    }
}

TEST_CASE("ZoneOrchestrator: Activity management", "[coordination][zone][activity]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Can change activity")
    {
        orchestrator.SetActivity(ZoneActivity::WORLD_BOSS);
        REQUIRE(orchestrator.GetActivity() == ZoneActivity::WORLD_BOSS);
    }

    SECTION("All activity types are supported")
    {
        orchestrator.SetActivity(ZoneActivity::WORLD_BOSS);
        REQUIRE(orchestrator.GetActivity() == ZoneActivity::WORLD_BOSS);

        orchestrator.SetActivity(ZoneActivity::ZONE_EVENT);
        REQUIRE(orchestrator.GetActivity() == ZoneActivity::ZONE_EVENT);

        orchestrator.SetActivity(ZoneActivity::CITY_RAID);
        REQUIRE(orchestrator.GetActivity() == ZoneActivity::CITY_RAID);

        orchestrator.SetActivity(ZoneActivity::RESOURCE_FARMING);
        REQUIRE(orchestrator.GetActivity() == ZoneActivity::RESOURCE_FARMING);

        orchestrator.SetActivity(ZoneActivity::QUESTING);
        REQUIRE(orchestrator.GetActivity() == ZoneActivity::QUESTING);

        orchestrator.SetActivity(ZoneActivity::IDLE);
        REQUIRE(orchestrator.GetActivity() == ZoneActivity::IDLE);
    }
}

TEST_CASE("ZoneOrchestrator: Threat level management", "[coordination][zone][threat]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Can change threat level")
    {
        orchestrator.SetThreatLevel(ThreatLevel::CRITICAL);
        REQUIRE(orchestrator.GetThreatLevel() == ThreatLevel::CRITICAL);
    }

    SECTION("All threat levels are supported")
    {
        orchestrator.SetThreatLevel(ThreatLevel::PEACEFUL);
        REQUIRE(orchestrator.GetThreatLevel() == ThreatLevel::PEACEFUL);

        orchestrator.SetThreatLevel(ThreatLevel::LOW);
        REQUIRE(orchestrator.GetThreatLevel() == ThreatLevel::LOW);

        orchestrator.SetThreatLevel(ThreatLevel::MODERATE);
        REQUIRE(orchestrator.GetThreatLevel() == ThreatLevel::MODERATE);

        orchestrator.SetThreatLevel(ThreatLevel::HIGH);
        REQUIRE(orchestrator.GetThreatLevel() == ThreatLevel::HIGH);

        orchestrator.SetThreatLevel(ThreatLevel::CRITICAL);
        REQUIRE(orchestrator.GetThreatLevel() == ThreatLevel::CRITICAL);
    }
}

TEST_CASE("ZoneOrchestrator: Objective management", "[coordination][zone][objective]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Initially no objectives")
    {
        REQUIRE(orchestrator.GetActiveObjectives().empty());
    }

    SECTION("Can create objective")
    {
        ZoneObjective objective;
        objective.objectiveType = "defend_city";
        objective.priority = 80;
        objective.requiredBots = 20;
        objective.assignedBots = 0;
        objective.timestamp = GameTime::GetGameTimeMS();
        objective.expirationTime = objective.timestamp + 60000;

        orchestrator.CreateObjective(objective);

        auto active = orchestrator.GetActiveObjectives();
        REQUIRE(active.size() == 1);
        REQUIRE(active[0].objectiveType == "defend_city");
    }

    SECTION("Can create multiple objectives")
    {
        ZoneObjective obj1;
        obj1.objectiveType = "objective1";
        obj1.priority = 90;
        obj1.requiredBots = 10;
        obj1.assignedBots = 0;
        obj1.timestamp = GameTime::GetGameTimeMS();
        obj1.expirationTime = obj1.timestamp + 60000;

        ZoneObjective obj2;
        obj2.objectiveType = "objective2";
        obj2.priority = 70;
        obj2.requiredBots = 15;
        obj2.assignedBots = 0;
        obj2.timestamp = GameTime::GetGameTimeMS();
        obj2.expirationTime = obj2.timestamp + 60000;

        orchestrator.CreateObjective(obj1);
        orchestrator.CreateObjective(obj2);

        REQUIRE(orchestrator.GetActiveObjectives().size() == 2);
    }

    SECTION("Can complete objective")
    {
        ZoneObjective objective;
        objective.objectiveType = "test_objective";
        objective.priority = 50;
        objective.requiredBots = 5;
        objective.assignedBots = 0;
        objective.timestamp = GameTime::GetGameTimeMS();
        objective.expirationTime = objective.timestamp + 60000;

        orchestrator.CreateObjective(objective);
        REQUIRE(orchestrator.GetActiveObjectives().size() == 1);

        orchestrator.CompleteObjective("test_objective");
        REQUIRE(orchestrator.GetActiveObjectives().empty());
    }

    SECTION("Can assign bots to objective")
    {
        ZoneObjective objective;
        objective.objectiveType = "gather_resources";
        objective.priority = 60;
        objective.requiredBots = 10;
        objective.assignedBots = 0;
        objective.timestamp = GameTime::GetGameTimeMS();
        objective.expirationTime = objective.timestamp + 60000;

        orchestrator.CreateObjective(objective);

        uint32 assigned = orchestrator.AssignBotsToObjective("gather_resources", 5);
        // Will be 0 because we have no bots registered
        REQUIRE(assigned == 0);
    }
}

TEST_CASE("ZoneOrchestrator: Raid management", "[coordination][zone][raid]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Can add raid")
    {
        orchestrator.AddRaid(nullptr); // Would normally be a RaidOrchestrator*
        // With nullptr, addition is skipped
        REQUIRE(orchestrator.GetRaidCount() == 0);
    }

    SECTION("GetRaid returns nullptr for invalid index")
    {
        REQUIRE(orchestrator.GetRaid(0) == nullptr);
        REQUIRE(orchestrator.GetRaid(99) == nullptr);
    }
}

TEST_CASE("ZoneOrchestrator: Zone statistics", "[coordination][zone][stats]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Can retrieve zone stats")
    {
        auto stats = orchestrator.GetZoneStats();

        REQUIRE(stats.totalBots == 0);
        REQUIRE(stats.activeBots == 0);
        REQUIRE(stats.idleBots == 0);
        REQUIRE(stats.raidCount == 0);
        REQUIRE(stats.activeObjectives == 0);
        REQUIRE(stats.threatLevel == ThreatLevel::PEACEFUL);
        REQUIRE(stats.currentActivity == ZoneActivity::IDLE);
    }

    SECTION("Stats reflect zone state")
    {
        orchestrator.SetActivity(ZoneActivity::WORLD_BOSS);
        orchestrator.SetThreatLevel(ThreatLevel::CRITICAL);

        auto stats = orchestrator.GetZoneStats();
        REQUIRE(stats.threatLevel == ThreatLevel::CRITICAL);
        REQUIRE(stats.currentActivity == ZoneActivity::WORLD_BOSS);
    }
}

TEST_CASE("ZoneOrchestrator: Broadcasting", "[coordination][zone][broadcast]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Can broadcast message")
    {
        REQUIRE_NOTHROW(orchestrator.BroadcastMessage("Test message", 50));
    }

    SECTION("Broadcast with different priorities")
    {
        REQUIRE_NOTHROW(orchestrator.BroadcastMessage("Low priority", 10));
        REQUIRE_NOTHROW(orchestrator.BroadcastMessage("High priority", 90));
    }
}

TEST_CASE("ZoneOrchestrator: Assembly requests", "[coordination][zone][assembly]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Can request assembly")
    {
        Position pos(0.0f, 0.0f, 0.0f, 0.0f);
        uint32 responding = orchestrator.RequestAssembly(pos, 50.0f);
        REQUIRE(responding == 0); // No bots registered
    }
}

TEST_CASE("ZoneOrchestrator: Bot distribution balancing", "[coordination][zone][balance]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Balance doesn't crash with no bots")
    {
        REQUIRE_NOTHROW(orchestrator.BalanceBotDistribution());
    }
}

TEST_CASE("ZoneOrchestrator: Update", "[coordination][zone][update]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Update doesn't crash")
    {
        REQUIRE_NOTHROW(orchestrator.Update(1000));
    }

    SECTION("Multiple updates")
    {
        for (uint32 i = 0; i < 100; ++i)
        {
            REQUIRE_NOTHROW(orchestrator.Update(100));
        }
    }
}

// ============================================================================
// ZoneOrchestratorManager Tests
// ============================================================================

TEST_CASE("ZoneOrchestratorManager: Orchestrator creation", "[coordination][zone][manager]")
{
    ZoneOrchestratorManager::Clear();

    SECTION("Can create orchestrator")
    {
        auto* orchestrator = ZoneOrchestratorManager::CreateOrchestrator(1519);
        REQUIRE(orchestrator != nullptr);
        REQUIRE(orchestrator->GetZoneId() == 1519);
    }

    SECTION("Can retrieve orchestrator")
    {
        ZoneOrchestratorManager::CreateOrchestrator(1519);
        auto* orchestrator = ZoneOrchestratorManager::GetOrchestrator(1519);
        REQUIRE(orchestrator != nullptr);
        REQUIRE(orchestrator->GetZoneId() == 1519);
    }

    SECTION("Returns nullptr for non-existent zone")
    {
        auto* orchestrator = ZoneOrchestratorManager::GetOrchestrator(99999);
        REQUIRE(orchestrator == nullptr);
    }

    SECTION("Can create multiple orchestrators")
    {
        ZoneOrchestratorManager::CreateOrchestrator(1519); // Stormwind
        ZoneOrchestratorManager::CreateOrchestrator(1637); // Orgrimmar

        REQUIRE(ZoneOrchestratorManager::GetOrchestrator(1519) != nullptr);
        REQUIRE(ZoneOrchestratorManager::GetOrchestrator(1637) != nullptr);
    }

    SECTION("Can remove orchestrator")
    {
        ZoneOrchestratorManager::CreateOrchestrator(1519);
        REQUIRE(ZoneOrchestratorManager::GetOrchestrator(1519) != nullptr);

        ZoneOrchestratorManager::RemoveOrchestrator(1519);
        REQUIRE(ZoneOrchestratorManager::GetOrchestrator(1519) == nullptr);
    }

    SECTION("Can clear all orchestrators")
    {
        ZoneOrchestratorManager::CreateOrchestrator(1519);
        ZoneOrchestratorManager::CreateOrchestrator(1637);

        ZoneOrchestratorManager::Clear();

        REQUIRE(ZoneOrchestratorManager::GetOrchestrator(1519) == nullptr);
        REQUIRE(ZoneOrchestratorManager::GetOrchestrator(1637) == nullptr);
    }

    ZoneOrchestratorManager::Clear(); // Cleanup
}

TEST_CASE("ZoneOrchestratorManager: Update all", "[coordination][zone][manager]")
{
    ZoneOrchestratorManager::Clear();

    SECTION("UpdateAll doesn't crash with no orchestrators")
    {
        REQUIRE_NOTHROW(ZoneOrchestratorManager::UpdateAll(1000));
    }

    SECTION("UpdateAll updates all orchestrators")
    {
        ZoneOrchestratorManager::CreateOrchestrator(1519);
        ZoneOrchestratorManager::CreateOrchestrator(1637);

        REQUIRE_NOTHROW(ZoneOrchestratorManager::UpdateAll(1000));
    }

    ZoneOrchestratorManager::Clear();
}

TEST_CASE("ZoneOrchestratorManager: Global statistics", "[coordination][zone][manager][stats]")
{
    ZoneOrchestratorManager::Clear();

    SECTION("Global stats with no orchestrators")
    {
        auto stats = ZoneOrchestratorManager::GetGlobalStats();
        REQUIRE(stats.totalZones == 0);
        REQUIRE(stats.totalBots == 0);
        REQUIRE(stats.totalRaids == 0);
        REQUIRE(stats.activeObjectives == 0);
        REQUIRE(stats.criticalZones == 0);
    }

    SECTION("Global stats with orchestrators")
    {
        auto* zone1 = ZoneOrchestratorManager::CreateOrchestrator(1519);
        auto* zone2 = ZoneOrchestratorManager::CreateOrchestrator(1637);

        zone1->SetThreatLevel(ThreatLevel::CRITICAL);
        zone2->SetThreatLevel(ThreatLevel::PEACEFUL);

        auto stats = ZoneOrchestratorManager::GetGlobalStats();
        REQUIRE(stats.totalZones == 2);
        REQUIRE(stats.criticalZones == 1);
    }

    ZoneOrchestratorManager::Clear();
}

TEST_CASE("ZoneOrchestratorManager: GetAll", "[coordination][zone][manager]")
{
    ZoneOrchestratorManager::Clear();

    SECTION("GetAll returns empty map initially")
    {
        auto const& all = ZoneOrchestratorManager::GetAll();
        REQUIRE(all.empty());
    }

    SECTION("GetAll returns all orchestrators")
    {
        ZoneOrchestratorManager::CreateOrchestrator(1519);
        ZoneOrchestratorManager::CreateOrchestrator(1637);

        auto const& all = ZoneOrchestratorManager::GetAll();
        REQUIRE(all.size() == 2);
        REQUIRE(all.find(1519) != all.end());
        REQUIRE(all.find(1637) != all.end());
    }

    ZoneOrchestratorManager::Clear();
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE("ZoneOrchestrator: Complete zone scenario", "[coordination][zone][integration]")
{
    ZoneOrchestratorManager::Clear();

    auto* orchestrator = ZoneOrchestratorManager::CreateOrchestrator(1519);
    REQUIRE(orchestrator != nullptr);

    SECTION("Full zone activity flow")
    {
        // Setup
        orchestrator->SetActivity(ZoneActivity::IDLE);
        orchestrator->SetThreatLevel(ThreatLevel::PEACEFUL);

        // World boss appears
        orchestrator->SetActivity(ZoneActivity::WORLD_BOSS);
        orchestrator->SetThreatLevel(ThreatLevel::CRITICAL);

        ZoneObjective objective;
        objective.objectiveType = "kill_world_boss";
        objective.priority = 100;
        objective.requiredBots = 40;
        objective.assignedBots = 0;
        objective.timestamp = GameTime::GetGameTimeMS();
        objective.expirationTime = objective.timestamp + 3600000;

        orchestrator->CreateObjective(objective);

        // Request assembly
        Position bossPos(0.0f, 0.0f, 0.0f, 0.0f);
        orchestrator->RequestAssembly(bossPos, 100.0f);

        // Complete boss
        orchestrator->CompleteObjective("kill_world_boss");
        orchestrator->SetActivity(ZoneActivity::IDLE);
        orchestrator->SetThreatLevel(ThreatLevel::PEACEFUL);

        // Verify final state
        REQUIRE(orchestrator->GetActivity() == ZoneActivity::IDLE);
        REQUIRE(orchestrator->GetThreatLevel() == ThreatLevel::PEACEFUL);
        REQUIRE(orchestrator->GetActiveObjectives().empty());
    }

    ZoneOrchestratorManager::Clear();
}

TEST_CASE("ZoneOrchestrator: Performance characteristics", "[coordination][zone][performance]")
{
    ZoneOrchestrator orchestrator(1519);

    SECTION("Can handle many objectives")
    {
        for (uint32 i = 0; i < 100; ++i)
        {
            ZoneObjective objective;
            objective.objectiveType = "objective_" + std::to_string(i);
            objective.priority = i;
            objective.requiredBots = 10;
            objective.assignedBots = 0;
            objective.timestamp = GameTime::GetGameTimeMS();
            objective.expirationTime = objective.timestamp + 60000;

            orchestrator.CreateObjective(objective);
        }

        REQUIRE(orchestrator.GetActiveObjectives().size() == 100);
    }

    SECTION("Multiple rapid updates don't corrupt state")
    {
        orchestrator.SetActivity(ZoneActivity::QUESTING);

        for (uint32 i = 0; i < 1000; ++i)
        {
            orchestrator.Update(10);
        }

        REQUIRE(orchestrator.GetActivity() == ZoneActivity::QUESTING);
    }
}

TEST_CASE("ZoneOrchestratorManager: Scalability", "[coordination][zone][manager][scalability]")
{
    ZoneOrchestratorManager::Clear();

    SECTION("Can handle many zones")
    {
        for (uint32 i = 0; i < 100; ++i)
        {
            ZoneOrchestratorManager::CreateOrchestrator(1000 + i);
        }

        auto const& all = ZoneOrchestratorManager::GetAll();
        REQUIRE(all.size() == 100);

        ZoneOrchestratorManager::UpdateAll(1000);

        auto stats = ZoneOrchestratorManager::GetGlobalStats();
        REQUIRE(stats.totalZones == 100);
    }

    ZoneOrchestratorManager::Clear();
}
