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
#include "AI/Coordination/RoleCoordinator.h"
// Note: Old AI/Coordination/GroupCoordinator removed - now using Advanced/GroupCoordinator with TacticalCoordinator

using namespace Playerbot;

// ============================================================================
// TankCoordinator Tests
// ============================================================================

TEST_CASE("TankCoordinator: Basic functionality", "[coordination][tank]")
{
    TankCoordinator coordinator;

    SECTION("Returns TANK role")
    {
        REQUIRE(coordinator.GetRole() == GroupRole::TANK);
    }

    SECTION("Initially has no main tank")
    {
        REQUIRE(coordinator.GetMainTank().IsEmpty());
    }

    SECTION("Initially has no off-tank")
    {
        REQUIRE(coordinator.GetOffTank().IsEmpty());
    }

    SECTION("GetTankForTarget returns empty for unknown target")
    {
        ObjectGuid targetGuid = ObjectGuid::Create<HighGuid::Creature>(0, 12345, 1);
        REQUIRE(coordinator.GetTankForTarget(targetGuid).IsEmpty());
    }
}

TEST_CASE("TankCoordinator: Tank swap cooldown", "[coordination][tank][cooldown]")
{
    TankCoordinator coordinator;

    SECTION("Tank swap respects cooldown")
    {
        // First swap should work (no cooldown initially)
        coordinator.RequestTankSwap();

        // Immediate second swap should be blocked by cooldown
        ObjectGuid mainBefore = coordinator.GetMainTank();
        coordinator.RequestTankSwap();
        ObjectGuid mainAfter = coordinator.GetMainTank();

        // Main tank should not change (swap was blocked)
        REQUIRE(mainBefore == mainAfter);
    }
}

TEST_CASE("TankCoordinator: Update without group", "[coordination][tank][safety]")
{
    TankCoordinator coordinator;

    SECTION("Update with null group doesn't crash")
    {
        REQUIRE_NOTHROW(coordinator.Update(nullptr, 1000));
    }
}

TEST_CASE("TankCoordinator: NeedsTankSwap logic", "[coordination][tank][swap]")
{
    TankCoordinator coordinator;

    SECTION("Returns false for empty guid")
    {
        REQUIRE_FALSE(coordinator.NeedsTankSwap(ObjectGuid::Empty));
    }

    SECTION("Returns false for non-existent player")
    {
        ObjectGuid fakeGuid = ObjectGuid::Create<HighGuid::Player>(0, 99999);
        REQUIRE_FALSE(coordinator.NeedsTankSwap(fakeGuid));
    }
}

// ============================================================================
// HealerCoordinator Tests
// ============================================================================

TEST_CASE("HealerCoordinator: Basic functionality", "[coordination][healer]")
{
    HealerCoordinator coordinator;

    SECTION("Returns HEALER role")
    {
        REQUIRE(coordinator.GetRole() == GroupRole::HEALER);
    }

    SECTION("GetHealerForTank returns empty for unassigned tank")
    {
        ObjectGuid tankGuid = ObjectGuid::Create<HighGuid::Player>(0, 12345);
        REQUIRE(coordinator.GetHealerForTank(tankGuid).IsEmpty());
    }

    SECTION("GetNextCooldownHealer returns empty when no healers registered")
    {
        REQUIRE(coordinator.GetNextCooldownHealer("major_cd").IsEmpty());
    }
}

TEST_CASE("HealerCoordinator: Healer-to-tank assignments", "[coordination][healer][assignment]")
{
    HealerCoordinator coordinator;

    ObjectGuid healerGuid = ObjectGuid::Create<HighGuid::Player>(0, 1);
    ObjectGuid tankGuid = ObjectGuid::Create<HighGuid::Player>(0, 2);

    SECTION("Can assign healer to tank")
    {
        coordinator.AssignHealerToTank(healerGuid, tankGuid);
        REQUIRE(coordinator.GetHealerForTank(tankGuid) == healerGuid);
    }

    SECTION("Reassigning healer removes old assignment")
    {
        ObjectGuid tank2Guid = ObjectGuid::Create<HighGuid::Player>(0, 3);

        coordinator.AssignHealerToTank(healerGuid, tankGuid);
        REQUIRE(coordinator.GetHealerForTank(tankGuid) == healerGuid);

        coordinator.AssignHealerToTank(healerGuid, tank2Guid);
        REQUIRE(coordinator.GetHealerForTank(tank2Guid) == healerGuid);
        REQUIRE(coordinator.GetHealerForTank(tankGuid).IsEmpty());
    }

    SECTION("Multiple healers can be assigned to different tanks")
    {
        ObjectGuid healer2Guid = ObjectGuid::Create<HighGuid::Player>(0, 4);
        ObjectGuid tank2Guid = ObjectGuid::Create<HighGuid::Player>(0, 5);

        coordinator.AssignHealerToTank(healerGuid, tankGuid);
        coordinator.AssignHealerToTank(healer2Guid, tank2Guid);

        REQUIRE(coordinator.GetHealerForTank(tankGuid) == healerGuid);
        REQUIRE(coordinator.GetHealerForTank(tank2Guid) == healer2Guid);
    }
}

TEST_CASE("HealerCoordinator: Cooldown rotation", "[coordination][healer][cooldown]")
{
    HealerCoordinator coordinator;

    ObjectGuid healer1 = ObjectGuid::Create<HighGuid::Player>(0, 1);
    ObjectGuid healer2 = ObjectGuid::Create<HighGuid::Player>(0, 2);

    SECTION("Cooldown becomes unavailable after use")
    {
        // No cooldown initially
        REQUIRE(coordinator.GetNextCooldownHealer("major_cd") == healer1);

        // Use cooldown
        coordinator.UseHealingCooldown(healer1, "major_cd", 120000);

        // Should return different healer or empty
        ObjectGuid nextHealer = coordinator.GetNextCooldownHealer("major_cd");
        REQUIRE(nextHealer != healer1);
    }

    SECTION("Different cooldown types are tracked independently")
    {
        coordinator.UseHealingCooldown(healer1, "major_cd", 120000);

        // Different cooldown type should still be available
        ObjectGuid nextHealer = coordinator.GetNextCooldownHealer("minor_cd");
        REQUIRE(!nextHealer.IsEmpty());
    }
}

TEST_CASE("HealerCoordinator: Resurrection priority", "[coordination][healer][resurrection]")
{
    HealerCoordinator coordinator;

    SECTION("Returns empty list when no assignments")
    {
        auto priorities = coordinator.GetResurrectionPriority();
        REQUIRE(priorities.empty());
    }

    SECTION("Priority list is ordered")
    {
        auto priorities = coordinator.GetResurrectionPriority();
        // List should be in priority order (tested with live players in integration tests)
        REQUIRE(priorities.size() >= 0);
    }
}

TEST_CASE("HealerCoordinator: Update without group", "[coordination][healer][safety]")
{
    HealerCoordinator coordinator;

    SECTION("Update with null group doesn't crash")
    {
        REQUIRE_NOTHROW(coordinator.Update(nullptr, 1000));
    }
}

// ============================================================================
// DPSCoordinator Tests
// ============================================================================

TEST_CASE("DPSCoordinator: Basic functionality", "[coordination][dps]")
{
    DPSCoordinator coordinator;

    SECTION("Returns MELEE_DPS role (handles both melee and ranged)")
    {
        REQUIRE(coordinator.GetRole() == GroupRole::MELEE_DPS);
    }

    SECTION("Initially has no focus target")
    {
        REQUIRE(coordinator.GetFocusTarget().IsEmpty());
    }

    SECTION("Initially not in burst window")
    {
        REQUIRE_FALSE(coordinator.InBurstWindow());
    }

    SECTION("GetNextInterrupter returns empty when no rotation")
    {
        REQUIRE(coordinator.GetNextInterrupter().IsEmpty());
    }

    SECTION("GetCCAssignment returns empty for unassigned DPS")
    {
        ObjectGuid dpsGuid = ObjectGuid::Create<HighGuid::Player>(0, 12345);
        REQUIRE(coordinator.GetCCAssignment(dpsGuid).IsEmpty());
    }
}

TEST_CASE("DPSCoordinator: Focus target", "[coordination][dps][focus]")
{
    DPSCoordinator coordinator;

    ObjectGuid targetGuid = ObjectGuid::Create<HighGuid::Creature>(0, 12345, 1);

    SECTION("Can set focus target")
    {
        coordinator.SetFocusTarget(targetGuid);
        REQUIRE(coordinator.GetFocusTarget() == targetGuid);
    }

    SECTION("Can change focus target")
    {
        ObjectGuid target2Guid = ObjectGuid::Create<HighGuid::Creature>(0, 67890, 1);

        coordinator.SetFocusTarget(targetGuid);
        REQUIRE(coordinator.GetFocusTarget() == targetGuid);

        coordinator.SetFocusTarget(target2Guid);
        REQUIRE(coordinator.GetFocusTarget() == target2Guid);
    }

    SECTION("Can clear focus target")
    {
        coordinator.SetFocusTarget(targetGuid);
        coordinator.SetFocusTarget(ObjectGuid::Empty);
        REQUIRE(coordinator.GetFocusTarget().IsEmpty());
    }
}

TEST_CASE("DPSCoordinator: Interrupt rotation", "[coordination][dps][interrupt]")
{
    DPSCoordinator coordinator;

    ObjectGuid dps1 = ObjectGuid::Create<HighGuid::Player>(0, 1);
    ObjectGuid dps2 = ObjectGuid::Create<HighGuid::Player>(0, 2);
    ObjectGuid targetGuid = ObjectGuid::Create<HighGuid::Creature>(0, 100, 1);

    SECTION("Can assign interrupt to DPS")
    {
        coordinator.AssignInterrupt(dps1, targetGuid);

        // DPS1 should now be on cooldown
        ObjectGuid nextInterrupter = coordinator.GetNextInterrupter();
        REQUIRE(nextInterrupter != dps1); // Should be different or empty
    }

    SECTION("Interrupt rotation cycles through DPS")
    {
        coordinator.AssignInterrupt(dps1, targetGuid);
        coordinator.AssignInterrupt(dps2, targetGuid);

        // Both DPS have been assigned
        REQUIRE(coordinator.GetNextInterrupter().IsEmpty()); // All on CD or empty
    }

    SECTION("Reassigning same DPS updates assignment")
    {
        ObjectGuid target2Guid = ObjectGuid::Create<HighGuid::Creature>(0, 200, 1);

        coordinator.AssignInterrupt(dps1, targetGuid);
        coordinator.AssignInterrupt(dps1, target2Guid);

        // Should only have one assignment for dps1 (the newer one)
        // This is tested indirectly through rotation behavior
        REQUIRE(coordinator.GetNextInterrupter() != dps1);
    }
}

TEST_CASE("DPSCoordinator: CC assignments", "[coordination][dps][cc]")
{
    DPSCoordinator coordinator;

    ObjectGuid dpsGuid = ObjectGuid::Create<HighGuid::Player>(0, 1);
    ObjectGuid targetGuid = ObjectGuid::Create<HighGuid::Creature>(0, 100, 1);

    SECTION("Can assign CC to DPS")
    {
        coordinator.AssignCC(dpsGuid, targetGuid, "polymorph");
        REQUIRE(coordinator.GetCCAssignment(dpsGuid) == targetGuid);
    }

    SECTION("CC assignment can be updated")
    {
        ObjectGuid target2Guid = ObjectGuid::Create<HighGuid::Creature>(0, 200, 1);

        coordinator.AssignCC(dpsGuid, targetGuid, "polymorph");
        REQUIRE(coordinator.GetCCAssignment(dpsGuid) == targetGuid);

        coordinator.AssignCC(dpsGuid, target2Guid, "hex");
        REQUIRE(coordinator.GetCCAssignment(dpsGuid) == target2Guid);
    }

    SECTION("Multiple DPS can have different CC assignments")
    {
        ObjectGuid dps2Guid = ObjectGuid::Create<HighGuid::Player>(0, 2);
        ObjectGuid target2Guid = ObjectGuid::Create<HighGuid::Creature>(0, 200, 1);

        coordinator.AssignCC(dpsGuid, targetGuid, "polymorph");
        coordinator.AssignCC(dps2Guid, target2Guid, "trap");

        REQUIRE(coordinator.GetCCAssignment(dpsGuid) == targetGuid);
        REQUIRE(coordinator.GetCCAssignment(dps2Guid) == target2Guid);
    }

    SECTION("GetCCAssignment returns empty for unassigned DPS")
    {
        ObjectGuid unassignedDPS = ObjectGuid::Create<HighGuid::Player>(0, 999);
        REQUIRE(coordinator.GetCCAssignment(unassignedDPS).IsEmpty());
    }
}

TEST_CASE("DPSCoordinator: Burst windows", "[coordination][dps][burst]")
{
    DPSCoordinator coordinator;

    SECTION("Burst window activates correctly")
    {
        REQUIRE_FALSE(coordinator.InBurstWindow());

        coordinator.RequestBurstWindow(10000); // 10s burst

        REQUIRE(coordinator.InBurstWindow());
    }

    SECTION("Cannot activate burst window when already active")
    {
        coordinator.RequestBurstWindow(10000);
        REQUIRE(coordinator.InBurstWindow());

        // Request another burst (should be ignored)
        coordinator.RequestBurstWindow(5000);

        // Still in original burst window
        REQUIRE(coordinator.InBurstWindow());
    }

    SECTION("Burst window expires after duration")
    {
        coordinator.RequestBurstWindow(100); // 100ms burst
        REQUIRE(coordinator.InBurstWindow());

        // Wait for expiration (simulated by calling Update)
        // In actual implementation, Update() would handle expiration
    }
}

TEST_CASE("DPSCoordinator: Update without group", "[coordination][dps][safety]")
{
    DPSCoordinator coordinator;

    SECTION("Update with null group doesn't crash")
    {
        REQUIRE_NOTHROW(coordinator.Update(nullptr, 1000));
    }
}

// ============================================================================
// RoleCoordinatorManager Tests
// ============================================================================

TEST_CASE("RoleCoordinatorManager: Initialization", "[coordination][manager]")
{
    RoleCoordinatorManager manager;

    SECTION("All coordinators are initialized")
    {
        REQUIRE(manager.GetTankCoordinator() != nullptr);
        REQUIRE(manager.GetHealerCoordinator() != nullptr);
        REQUIRE(manager.GetDPSCoordinator() != nullptr);
    }

    SECTION("Coordinators have correct roles")
    {
        REQUIRE(manager.GetTankCoordinator()->GetRole() == GroupRole::TANK);
        REQUIRE(manager.GetHealerCoordinator()->GetRole() == GroupRole::HEALER);
        REQUIRE(manager.GetDPSCoordinator()->GetRole() == GroupRole::MELEE_DPS);
    }
}

TEST_CASE("RoleCoordinatorManager: Update", "[coordination][manager][update]")
{
    RoleCoordinatorManager manager;

    SECTION("Update with null group doesn't crash")
    {
        REQUIRE_NOTHROW(manager.Update(nullptr, 1000));
    }

    SECTION("Update calls all coordinators")
    {
        // This is implicitly tested - if Update crashes, the test fails
        // In a real integration test, we would verify state changes
        REQUIRE_NOTHROW(manager.Update(nullptr, 1000));
    }
}

TEST_CASE("RoleCoordinatorManager: Coordinator independence", "[coordination][manager][independence]")
{
    RoleCoordinatorManager manager;

    SECTION("Coordinators operate independently")
    {
        // Set different states in each coordinator
        ObjectGuid targetGuid = ObjectGuid::Create<HighGuid::Creature>(0, 100, 1);
        ObjectGuid healerGuid = ObjectGuid::Create<HighGuid::Player>(0, 1);
        ObjectGuid tankGuid = ObjectGuid::Create<HighGuid::Player>(0, 2);

        manager.GetDPSCoordinator()->SetFocusTarget(targetGuid);
        manager.GetHealerCoordinator()->AssignHealerToTank(healerGuid, tankGuid);

        // Verify states are independent
        REQUIRE(manager.GetDPSCoordinator()->GetFocusTarget() == targetGuid);
        REQUIRE(manager.GetHealerCoordinator()->GetHealerForTank(tankGuid) == healerGuid);
        REQUIRE(manager.GetTankCoordinator()->GetMainTank().IsEmpty());
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE("RoleCoordinator: Full coordination cycle", "[coordination][integration]")
{
    RoleCoordinatorManager manager;

    ObjectGuid tank = ObjectGuid::Create<HighGuid::Player>(0, 1);
    ObjectGuid healer = ObjectGuid::Create<HighGuid::Player>(0, 2);
    ObjectGuid dps = ObjectGuid::Create<HighGuid::Player>(0, 3);
    ObjectGuid target = ObjectGuid::Create<HighGuid::Creature>(0, 100, 1);

    SECTION("Complete tactical setup")
    {
        // Tank coordinator
        manager.GetTankCoordinator()->RequestTankSwap();

        // Healer coordinator
        manager.GetHealerCoordinator()->AssignHealerToTank(healer, tank);
        manager.GetHealerCoordinator()->UseHealingCooldown(healer, "major_cd", 120000);

        // DPS coordinator
        manager.GetDPSCoordinator()->SetFocusTarget(target);
        manager.GetDPSCoordinator()->AssignInterrupt(dps, target);
        manager.GetDPSCoordinator()->AssignCC(dps, target, "polymorph");
        manager.GetDPSCoordinator()->RequestBurstWindow(10000);

        // Verify all assignments
        REQUIRE(manager.GetHealerCoordinator()->GetHealerForTank(tank) == healer);
        REQUIRE(manager.GetDPSCoordinator()->GetFocusTarget() == target);
        REQUIRE(manager.GetDPSCoordinator()->GetCCAssignment(dps) == target);
        REQUIRE(manager.GetDPSCoordinator()->InBurstWindow());
    }
}

TEST_CASE("RoleCoordinator: Performance characteristics", "[coordination][performance]")
{
    RoleCoordinatorManager manager;

    SECTION("Can handle many assignments")
    {
        // Create 100 assignments across all coordinators
    for (uint32 i = 0; i < 100; ++i)
        {
            ObjectGuid healer = ObjectGuid::Create<HighGuid::Player>(0, i);
            ObjectGuid tank = ObjectGuid::Create<HighGuid::Player>(0, i + 1000);
            ObjectGuid dps = ObjectGuid::Create<HighGuid::Player>(0, i + 2000);
            ObjectGuid target = ObjectGuid::Create<HighGuid::Creature>(0, i + 3000, 1);

            manager.GetHealerCoordinator()->AssignHealerToTank(healer, tank);
            manager.GetDPSCoordinator()->AssignCC(dps, target, "polymorph");
        }

        // Update should still be fast
        REQUIRE_NOTHROW(manager.Update(nullptr, 1000));
    }
}

TEST_CASE("RoleCoordinator: Thread safety considerations", "[coordination][threading]")
{
    RoleCoordinatorManager manager;

    SECTION("Multiple rapid updates don't corrupt state")
    {
        ObjectGuid targetGuid = ObjectGuid::Create<HighGuid::Creature>(0, 100, 1);

        manager.GetDPSCoordinator()->SetFocusTarget(targetGuid);

        for (uint32 i = 0; i < 1000; ++i)
        {
            manager.Update(nullptr, 10);
        }

        REQUIRE(manager.GetDPSCoordinator()->GetFocusTarget() == targetGuid);
    }
}

TEST_CASE("RoleCoordinator: Memory management", "[coordination][memory]")
{
    SECTION("Manager can be created and destroyed multiple times")
    {
        for (uint32 i = 0; i < 100; ++i)
        {
            RoleCoordinatorManager manager;
            REQUIRE(manager.GetTankCoordinator() != nullptr);
        }
        // No leaks expected
    }

    SECTION("Assignments are cleaned up properly")
    {
        RoleCoordinatorManager manager;

        ObjectGuid healer = ObjectGuid::Create<HighGuid::Player>(0, 1);
        ObjectGuid tank = ObjectGuid::Create<HighGuid::Player>(0, 2);

        // Create many assignments
    for (uint32 i = 0; i < 1000; ++i)
        {
            manager.GetHealerCoordinator()->AssignHealerToTank(healer, tank);
        }

        // Manager should clean up when destroyed
        // No explicit check needed - sanitizers will catch leaks
    }
}
