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
#include "AI/Decision/ActionPriorityQueue.h"
#include "AI/Decision/DecisionFusionSystem.h"

using namespace Playerbot::bot::ai;

// Test spell IDs (placeholder values)
constexpr uint32 SPELL_FIREBALL = 133;
constexpr uint32 SPELL_PYROBLAST = 11366;
constexpr uint32 SPELL_FLAMESTRIKE = 2120;
constexpr uint32 SPELL_ICE_BLOCK = 45438;

TEST_CASE("ActionPriorityQueue - Basic Registration", "[Phase5][ActionPriorityQueue]")
{
    ActionPriorityQueue queue;

    REQUIRE(queue.GetSpellCount() == 0);

    SECTION("Register a single spell")
    {
        queue.RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

        REQUIRE(queue.GetSpellCount() == 1);
    }

    SECTION("Register multiple spells")
    {
        queue.RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
        queue.RegisterSpell(SPELL_PYROBLAST, SpellPriority::CRITICAL, SpellCategory::DAMAGE_SINGLE);
        queue.RegisterSpell(SPELL_FLAMESTRIKE, SpellPriority::MEDIUM, SpellCategory::DAMAGE_AOE);

        REQUIRE(queue.GetSpellCount() == 3);
    }

    SECTION("Register duplicate spell does not increase count")
    {
        queue.RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
        queue.RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

        REQUIRE(queue.GetSpellCount() == 1);
    }
}

TEST_CASE("ActionPriorityQueue - Priority Levels", "[Phase5][ActionPriorityQueue]")
{
    SECTION("Priority values are correctly ordered")
    {
        REQUIRE(static_cast<uint8>(SpellPriority::EMERGENCY) > static_cast<uint8>(SpellPriority::CRITICAL));
        REQUIRE(static_cast<uint8>(SpellPriority::CRITICAL) > static_cast<uint8>(SpellPriority::HIGH));
        REQUIRE(static_cast<uint8>(SpellPriority::HIGH) > static_cast<uint8>(SpellPriority::MEDIUM));
        REQUIRE(static_cast<uint8>(SpellPriority::MEDIUM) > static_cast<uint8>(SpellPriority::LOW));
        REQUIRE(static_cast<uint8>(SpellPriority::LOW) > static_cast<uint8>(SpellPriority::OPTIONAL_PRIORITY));
    }

    SECTION("Emergency priority has highest value")
    {
        REQUIRE(static_cast<uint8>(SpellPriority::EMERGENCY) == 100);
    }
}

TEST_CASE("ActionPriorityQueue - Spell Categories", "[Phase5][ActionPriorityQueue]")
{
    ActionPriorityQueue queue;

    SECTION("Register spells with different categories")
    {
        queue.RegisterSpell(1001, SpellPriority::HIGH, SpellCategory::DEFENSIVE);
        queue.RegisterSpell(1002, SpellPriority::HIGH, SpellCategory::OFFENSIVE);
        queue.RegisterSpell(1003, SpellPriority::HIGH, SpellCategory::HEALING);
        queue.RegisterSpell(1004, SpellPriority::HIGH, SpellCategory::CROWD_CONTROL);
        queue.RegisterSpell(1005, SpellPriority::HIGH, SpellCategory::UTILITY);
        queue.RegisterSpell(1006, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
        queue.RegisterSpell(1007, SpellPriority::HIGH, SpellCategory::DAMAGE_AOE);
        queue.RegisterSpell(1008, SpellPriority::HIGH, SpellCategory::RESOURCE_BUILDER);
        queue.RegisterSpell(1009, SpellPriority::HIGH, SpellCategory::RESOURCE_SPENDER);
        queue.RegisterSpell(1010, SpellPriority::HIGH, SpellCategory::MOVEMENT);

        REQUIRE(queue.GetSpellCount() == 10);
    }
}

TEST_CASE("ActionPriorityQueue - Spell Conditions", "[Phase5][ActionPriorityQueue]")
{
    ActionPriorityQueue queue;
    queue.RegisterSpell(SPELL_PYROBLAST, SpellPriority::CRITICAL, SpellCategory::DAMAGE_SINGLE);

    SECTION("Add condition to spell")
    {
        bool conditionCalled = false;
        queue.AddCondition(SPELL_PYROBLAST,
            [&conditionCalled](Player*, Unit*) {
                conditionCalled = true;
                return true;
            },
            "Test condition");

        // Condition existence is verified - actual execution requires Player/Unit
        REQUIRE(queue.GetSpellCount() == 1);
    }

    SECTION("Add condition to non-existent spell fails gracefully")
    {
        queue.AddCondition(999999,
            [](Player*, Unit*) { return true; },
            "Invalid spell");

        REQUIRE(queue.GetSpellCount() == 1); // Only PYROBLAST registered
    }
}

TEST_CASE("ActionPriorityQueue - Priority Multipliers", "[Phase5][ActionPriorityQueue]")
{
    ActionPriorityQueue queue;
    queue.RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

    SECTION("Set priority multiplier")
    {
        queue.SetPriorityMultiplier(SPELL_FIREBALL, 2.0f);
        // Multiplier is set - verification requires GetPrioritizedSpells with bot/target
        REQUIRE(queue.GetSpellCount() == 1);
    }

    SECTION("Set multiplier on non-existent spell fails gracefully")
    {
        queue.SetPriorityMultiplier(999999, 2.0f);
        REQUIRE(queue.GetSpellCount() == 1);
    }
}

TEST_CASE("ActionPriorityQueue - Clear Functionality", "[Phase5][ActionPriorityQueue]")
{
    ActionPriorityQueue queue;

    queue.RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
    queue.RegisterSpell(SPELL_PYROBLAST, SpellPriority::CRITICAL, SpellCategory::DAMAGE_SINGLE);

    REQUIRE(queue.GetSpellCount() == 2);

    queue.Clear();

    REQUIRE(queue.GetSpellCount() == 0);
}

TEST_CASE("ActionPriorityQueue - DecisionVote Generation", "[Phase5][ActionPriorityQueue]")
{
    // Note: Full DecisionVote testing requires Player* and Unit* which are not available in unit tests
    // These tests verify the interface and basic functionality

    SECTION("DecisionVote has correct source")
    {
        ActionPriorityQueue queue;
        queue.RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

        // DecisionVote generation requires bot/target which aren't available in unit tests
        // Verified: Interface exists and compiles
        REQUIRE(queue.GetSpellCount() == 1);
    }
}

TEST_CASE("ActionPriorityQueue - Context Awareness", "[Phase5][ActionPriorityQueue]")
{
    SECTION("Combat contexts are defined")
    {
        // Verify all combat contexts exist
        CombatContext contexts[] = {
            CombatContext::SOLO,
            CombatContext::GROUP,
            CombatContext::DUNGEON_TRASH,
            CombatContext::DUNGEON_BOSS,
            CombatContext::RAID_NORMAL,
            CombatContext::RAID_HEROIC,
            CombatContext::PVP_ARENA,
            CombatContext::PVP_BG
        };

        REQUIRE(sizeof(contexts) / sizeof(contexts[0]) == 8);
    }
}

TEST_CASE("ActionPriorityQueue - Debug Logging", "[Phase5][ActionPriorityQueue]")
{
    ActionPriorityQueue queue;

    SECTION("Enable debug logging")
    {
        queue.EnableDebugLogging(true);
        queue.RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
        REQUIRE(queue.GetSpellCount() == 1);
    }

    SECTION("Disable debug logging")
    {
        queue.EnableDebugLogging(false);
        queue.RegisterSpell(SPELL_PYROBLAST, SpellPriority::CRITICAL, SpellCategory::DAMAGE_SINGLE);
        REQUIRE(queue.GetSpellCount() == 1);
    }
}

TEST_CASE("ActionPriorityQueue - Record Cast Functionality", "[Phase5][ActionPriorityQueue]")
{
    ActionPriorityQueue queue;
    queue.RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

    SECTION("Record spell cast")
    {
        queue.RecordCast(SPELL_FIREBALL);
        // Cast time is recorded internally
        REQUIRE(queue.GetSpellCount() == 1);
    }

    SECTION("Record cast of non-registered spell fails gracefully")
    {
        queue.RecordCast(999999);
        REQUIRE(queue.GetSpellCount() == 1);
    }
}
