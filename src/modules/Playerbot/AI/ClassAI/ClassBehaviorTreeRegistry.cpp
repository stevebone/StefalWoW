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

#include "ClassBehaviorTreeRegistry.h"
#include "AI/BehaviorTree/Nodes/CombatNodes.h"
#include "AI/BehaviorTree/Nodes/HealingNodes.h"
#include "AI/BehaviorTree/Nodes/MovementNodes.h"
#include "Log.h"

namespace Playerbot
{

// Static member initialization
::std::unordered_map<ClassSpec, ClassTreeBuilder, ClassSpecHash> ClassBehaviorTreeRegistry::_treeBuilders;
::std::unordered_map<ClassSpec, SpecRole, ClassSpecHash> ClassBehaviorTreeRegistry::_specRoles;

// ============================================================================
// ClassBehaviorTreeRegistry
// ============================================================================

void ClassBehaviorTreeRegistry::RegisterTree(WowClass classId, uint8 specId, ClassTreeBuilder builder)
{
    ClassSpec spec = {classId, specId};
    _treeBuilders[spec] = builder;

    TC_LOG_INFO("playerbot.classai", "Registered behavior tree for class {} spec {}",
        static_cast<uint8>(classId), specId);
}

::std::shared_ptr<BTNode> ClassBehaviorTreeRegistry::GetTree(WowClass classId, uint8 specId)
{
    ClassSpec spec = {classId, specId};
    auto it = _treeBuilders.find(spec);
    if (it != _treeBuilders.end())
    {
        return it->second();
    }

    TC_LOG_ERROR("playerbot.classai", "No behavior tree found for class {} spec {}",
        static_cast<uint8>(classId), specId);
    return nullptr;
}

SpecRole ClassBehaviorTreeRegistry::GetRole(WowClass classId, uint8 specId)
{
    ClassSpec spec = {classId, specId};
    auto it = _specRoles.find(spec);
    if (it != _specRoles.end())
        return it->second;

    // Default to DPS
    return SpecRole::MELEE_DPS;
}

void ClassBehaviorTreeRegistry::Initialize()
{
    TC_LOG_INFO("playerbot.classai", "Initializing class behavior trees for 13 classes...");

    InitializeWarrior();
    InitializePaladin();
    InitializeHunter();
    InitializeRogue();
    InitializePriest();
    InitializeDeathKnight();
    InitializeShaman();
    InitializeMage();
    InitializeWarlock();
    InitializeMonk();
    InitializeDruid();
    InitializeDemonHunter();
    InitializeEvoker();

    TC_LOG_INFO("playerbot.classai", "Class behavior tree initialization complete ({} trees registered)",
        _treeBuilders.size());
}

void ClassBehaviorTreeRegistry::Clear()
{
    _treeBuilders.clear();
    _specRoles.clear();
}

// ============================================================================
// Warrior (Arms, Fury, Protection)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeWarrior()
{
    // Arms (Spec 0) - Melee DPS
    RegisterTree(WowClass::WARRIOR, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("WarriorArmsRoot");

        // Combat rotation
        auto combatSeq = ::std::make_shared<BTSequence>("ArmsCombat");
        combatSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        combatSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        combatSeq->AddChild(::std::make_shared<BTWarriorExecute>()); // Execute at <20% health
        combatSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(combatSeq);
        return root;
    });
    _specRoles[{WowClass::WARRIOR, 0}] = SpecRole::MELEE_DPS;

    // Fury (Spec 1) - Melee DPS
    RegisterTree(WowClass::WARRIOR, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("WarriorFuryRoot");

        auto combatSeq = ::std::make_shared<BTSequence>("FuryCombat");
        combatSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        combatSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        combatSeq->AddChild(::std::make_shared<BTWarriorRecklessness>()); // Burst CD
        combatSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(combatSeq);
        return root;
    });
    _specRoles[{WowClass::WARRIOR, 1}] = SpecRole::MELEE_DPS;

    // Protection (Spec 2) - Tank
    RegisterTree(WowClass::WARRIOR, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("WarriorProtectionRoot");

        auto tankSeq = ::std::make_shared<BTSequence>("ProtectionTank");
        tankSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        tankSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        tankSeq->AddChild(::std::make_shared<BTWarriorShieldBlock>()); // Defensive
        tankSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(tankSeq);
        return root;
    });
    _specRoles[{WowClass::WARRIOR, 2}] = SpecRole::TANK;
}

// ============================================================================
// Paladin (Holy, Protection, Retribution)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializePaladin()
{
    // Holy (Spec 0) - Healer
    RegisterTree(WowClass::PALADIN, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("PaladinHolyRoot");

        auto healSeq = std::make_shared<BTSequence>("HolyHeal");
        healSeq->AddChild(std::make_shared<BTFindWoundedAlly>());
        healSeq->AddChild(std::make_shared<BTCastHeal>(19750)); // Flash of Light

        root->AddChild(healSeq);
        return root;
    });
    _specRoles[{WowClass::PALADIN, 0}] = SpecRole::HEALER;

    // Protection (Spec 1) - Tank
    RegisterTree(WowClass::PALADIN, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("PaladinProtectionRoot");

        auto tankSeq = ::std::make_shared<BTSequence>("ProtectionTank");
        tankSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        tankSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        tankSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(tankSeq);
        return root;
    });
    _specRoles[{WowClass::PALADIN, 1}] = SpecRole::TANK;

    // Retribution (Spec 2) - Melee DPS
    RegisterTree(WowClass::PALADIN, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("PaladinRetributionRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("RetributionDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::PALADIN, 2}] = SpecRole::MELEE_DPS;
}

// ============================================================================
// Hunter (Beast Mastery, Marksmanship, Survival)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeHunter()
{
    // Beast Mastery (Spec 0) - Ranged DPS
    RegisterTree(WowClass::HUNTER, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("HunterBeastMasteryRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("BeastMasteryDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f)); // Ranged range

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::HUNTER, 0}] = SpecRole::RANGED_DPS;

    // Marksmanship (Spec 1) - Ranged DPS
    RegisterTree(WowClass::HUNTER, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("HunterMarksmanshipRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("MarksmanshipDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::HUNTER, 1}] = SpecRole::RANGED_DPS;

    // Survival (Spec 2) - Melee DPS
    RegisterTree(WowClass::HUNTER, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("HunterSurvivalRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("SurvivalDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::HUNTER, 2}] = SpecRole::MELEE_DPS;
}

// ============================================================================
// Rogue (Assassination, Outlaw, Subtlety)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeRogue()
{
    // Assassination (Spec 0) - Melee DPS
    RegisterTree(WowClass::ROGUE, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("RogueAssassinationRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("AssassinationDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTPositionBehindTarget>()); // Backstab positioning
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::ROGUE, 0}] = SpecRole::MELEE_DPS;

    // Outlaw (Spec 1) - Melee DPS
    RegisterTree(WowClass::ROGUE, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("RogueOutlawRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("OutlawDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::ROGUE, 1}] = SpecRole::MELEE_DPS;

    // Subtlety (Spec 2) - Melee DPS
    RegisterTree(WowClass::ROGUE, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("RogueSubtletyRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("SubtletyDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTPositionBehindTarget>());
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::ROGUE, 2}] = SpecRole::MELEE_DPS;
}

// ============================================================================
// Priest (Discipline, Holy, Shadow)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializePriest()
{
    // Discipline (Spec 0) - Healer
    RegisterTree(WowClass::PRIEST, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("PriestDisciplineRoot");

        auto healSeq = std::make_shared<BTSequence>("DisciplineHeal");
        healSeq->AddChild(std::make_shared<BTFindWoundedAlly>());
        healSeq->AddChild(std::make_shared<BTPriestPowerWordShield>()); // Shield first
        healSeq->AddChild(std::make_shared<BTCastHeal>(2061)); // Flash Heal

        root->AddChild(healSeq);
        return root;
    });
    _specRoles[{WowClass::PRIEST, 0}] = SpecRole::HEALER;

    // Holy (Spec 1) - Healer
    RegisterTree(WowClass::PRIEST, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("PriestHolyRoot");

        auto healSeq = ::std::make_shared<BTSequence>("HolyHeal");
        healSeq->AddChild(::std::make_shared<BTFindWoundedAlly>());
        healSeq->AddChild(::std::make_shared<BTPriestPrayerOfMending>()); // HoT
        healSeq->AddChild(::std::make_shared<BTCastHeal>(2061)); // Flash Heal

        root->AddChild(healSeq);
        return root;
    });
    _specRoles[{WowClass::PRIEST, 1}] = SpecRole::HEALER;

    // Shadow (Spec 2) - Ranged DPS
    RegisterTree(WowClass::PRIEST, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("PriestShadowRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("ShadowDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));
        dpsSeq->AddChild(::std::make_shared<BTPriestShadowWordPain>()); // DoT

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::PRIEST, 2}] = SpecRole::RANGED_DPS;
}

// ============================================================================
// Death Knight (Blood, Frost, Unholy)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeDeathKnight()
{
    // Blood (Spec 0) - Tank
    RegisterTree(WowClass::DEATH_KNIGHT, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("DeathKnightBloodRoot");

        auto tankSeq = ::std::make_shared<BTSequence>("BloodTank");
        tankSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        tankSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        tankSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(tankSeq);
        return root;
    });
    _specRoles[{WowClass::DEATH_KNIGHT, 0}] = SpecRole::TANK;

    // Frost (Spec 1) - Melee DPS
    RegisterTree(WowClass::DEATH_KNIGHT, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("DeathKnightFrostRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("FrostDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::DEATH_KNIGHT, 1}] = SpecRole::MELEE_DPS;

    // Unholy (Spec 2) - Melee DPS
    RegisterTree(WowClass::DEATH_KNIGHT, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("DeathKnightUnholyRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("UnholyDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::DEATH_KNIGHT, 2}] = SpecRole::MELEE_DPS;
}

// ============================================================================
// Shaman (Elemental, Enhancement, Restoration)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeShaman()
{
    // Elemental (Spec 0) - Ranged DPS
    RegisterTree(WowClass::SHAMAN, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("ShamanElementalRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("ElementalDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::SHAMAN, 0}] = SpecRole::RANGED_DPS;

    // Enhancement (Spec 1) - Melee DPS
    RegisterTree(WowClass::SHAMAN, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("ShamanEnhancementRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("EnhancementDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::SHAMAN, 1}] = SpecRole::MELEE_DPS;

    // Restoration (Spec 2) - Healer
    RegisterTree(WowClass::SHAMAN, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("ShamanRestorationRoot");

        auto healSeq = ::std::make_shared<BTSequence>("RestorationHeal");
        healSeq->AddChild(::std::make_shared<BTFindWoundedAlly>());
        healSeq->AddChild(::std::make_shared<BTCastHeal>(8004)); // Healing Surge

        root->AddChild(healSeq);
        return root;
    });
    _specRoles[{WowClass::SHAMAN, 2}] = SpecRole::HEALER;
}

// ============================================================================
// Mage (Arcane, Fire, Frost)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeMage()
{
    // Arcane (Spec 0) - Ranged DPS
    RegisterTree(WowClass::MAGE, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("MageArcaneRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("ArcaneDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));
        dpsSeq->AddChild(::std::make_shared<BTMageArcaneBlast>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::MAGE, 0}] = SpecRole::RANGED_DPS;

    // Fire (Spec 1) - Ranged DPS
    RegisterTree(WowClass::MAGE, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("MageFireRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("FireDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::MAGE, 1}] = SpecRole::RANGED_DPS;

    // Frost (Spec 2) - Ranged DPS
    RegisterTree(WowClass::MAGE, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("MageFrostRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("FrostDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::MAGE, 2}] = SpecRole::RANGED_DPS;
}

// ============================================================================
// Warlock (Affliction, Demonology, Destruction)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeWarlock()
{
    // Affliction (Spec 0) - Ranged DPS
    RegisterTree(WowClass::WARLOCK, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("WarlockAfflictionRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("AfflictionDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::WARLOCK, 0}] = SpecRole::RANGED_DPS;

    // Demonology (Spec 1) - Ranged DPS
    RegisterTree(WowClass::WARLOCK, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("WarlockDemonologyRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("DemonologyDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::WARLOCK, 1}] = SpecRole::RANGED_DPS;

    // Destruction (Spec 2) - Ranged DPS
    RegisterTree(WowClass::WARLOCK, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("WarlockDestructionRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("DestructionDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::WARLOCK, 2}] = SpecRole::RANGED_DPS;
}

// ============================================================================
// Monk (Brewmaster, Mistweaver, Windwalker)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeMonk()
{
    // Brewmaster (Spec 0) - Tank
    RegisterTree(WowClass::MONK, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("MonkBrewmasterRoot");

        auto tankSeq = ::std::make_shared<BTSequence>("BrewmasterTank");
        tankSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        tankSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        tankSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(tankSeq);
        return root;
    });
    _specRoles[{WowClass::MONK, 0}] = SpecRole::TANK;

    // Mistweaver (Spec 1) - Healer
    RegisterTree(WowClass::MONK, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("MonkMistweaverRoot");

        auto healSeq = ::std::make_shared<BTSequence>("MistweaverHeal");
        healSeq->AddChild(::std::make_shared<BTFindWoundedAlly>());
        healSeq->AddChild(::std::make_shared<BTCastHeal>(116670)); // Vivify

        root->AddChild(healSeq);
        return root;
    });
    _specRoles[{WowClass::MONK, 1}] = SpecRole::HEALER;

    // Windwalker (Spec 2) - Melee DPS
    RegisterTree(WowClass::MONK, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("MonkWindwalkerRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("WindwalkerDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::MONK, 2}] = SpecRole::MELEE_DPS;
}

// ============================================================================
// Druid (Balance, Feral, Guardian, Restoration)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeDruid()
{
    // Balance (Spec 0) - Ranged DPS
    RegisterTree(WowClass::DRUID, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("DruidBalanceRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("BalanceDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 40.0f));

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::DRUID, 0}] = SpecRole::RANGED_DPS;

    // Feral (Spec 1) - Melee DPS
    RegisterTree(WowClass::DRUID, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("DruidFeralRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("FeralDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTPositionBehindTarget>());
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::DRUID, 1}] = SpecRole::MELEE_DPS;

    // Guardian (Spec 2) - Tank
    RegisterTree(WowClass::DRUID, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("DruidGuardianRoot");

        auto tankSeq = ::std::make_shared<BTSequence>("GuardianTank");
        tankSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        tankSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        tankSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(tankSeq);
        return root;
    });
    _specRoles[{WowClass::DRUID, 2}] = SpecRole::TANK;

    // Note: Restoration would be spec 3, but we only have 3 specs in the system
    // This would require extending the system to support 4 specs for Druid
}

// ============================================================================
// Demon Hunter (Havoc, Vengeance)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeDemonHunter()
{
    // Havoc (Spec 0) - Melee DPS
    RegisterTree(WowClass::DEMON_HUNTER, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("DemonHunterHavocRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("HavocDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        dpsSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::DEMON_HUNTER, 0}] = SpecRole::MELEE_DPS;

    // Vengeance (Spec 1) - Tank
    RegisterTree(WowClass::DEMON_HUNTER, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("DemonHunterVengeanceRoot");

        auto tankSeq = ::std::make_shared<BTSequence>("VengeanceTank");
        tankSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        tankSeq->AddChild(::std::make_shared<BTCheckInRange>(0.0f, 5.0f));
        tankSeq->AddChild(::std::make_shared<BTMeleeAttack>());

        root->AddChild(tankSeq);
        return root;
    });
    _specRoles[{WowClass::DEMON_HUNTER, 1}] = SpecRole::TANK;

    // Demon Hunters only have 2 specs
}

// ============================================================================
// Evoker (Devastation, Preservation, Augmentation)
// ============================================================================

void ClassBehaviorTreeRegistry::InitializeEvoker()
{
    // Devastation (Spec 0) - Ranged DPS
    RegisterTree(WowClass::EVOKER, 0, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("EvokerDevastationRoot");

        auto dpsSeq = ::std::make_shared<BTSequence>("DevastationDPS");
        dpsSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        dpsSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 25.0f)); // Mid-range

        root->AddChild(dpsSeq);
        return root;
    });
    _specRoles[{WowClass::EVOKER, 0}] = SpecRole::RANGED_DPS;

    // Preservation (Spec 1) - Healer
    RegisterTree(WowClass::EVOKER, 1, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("EvokerPreservationRoot");

        auto healSeq = ::std::make_shared<BTSequence>("PreservationHeal");
        healSeq->AddChild(::std::make_shared<BTFindWoundedAlly>());
        healSeq->AddChild(::std::make_shared<BTCastHeal>(361469)); // Living Flame

        root->AddChild(healSeq);
        return root;
    });
    _specRoles[{WowClass::EVOKER, 1}] = SpecRole::HEALER;

    // Augmentation (Spec 2) - Support DPS
    RegisterTree(WowClass::EVOKER, 2, []() -> ::std::shared_ptr<BTNode> {
        auto root = ::std::make_shared<BTSelector>("EvokerAugmentationRoot");

        auto supportSeq = ::std::make_shared<BTSequence>("AugmentationSupport");
        supportSeq->AddChild(::std::make_shared<BTCheckHasTarget>());
        supportSeq->AddChild(::std::make_shared<BTCheckInRange>(5.0f, 25.0f));

        root->AddChild(supportSeq);
        return root;
    });
    _specRoles[{WowClass::EVOKER, 2}] = SpecRole::RANGED_DPS;
}

// ============================================================================
// Class-Specific Node Implementations
// ============================================================================

// Warrior nodes
BTStatus BTWarriorExecute::Tick(BotAI* /*ai*/, BTBlackboard& /*blackboard*/)
{
    // Would check target health < 20% and cast Execute
    return BTStatus::SUCCESS;
}

BTStatus BTWarriorShieldBlock::Tick(BotAI* /*ai*/, BTBlackboard& /*blackboard*/)
{
    // Would check if taking damage and cast Shield Block
    return BTStatus::SUCCESS;
}

BTStatus BTWarriorRecklessness::Tick(BotAI* /*ai*/, BTBlackboard& /*blackboard*/)
{
    // Would cast Recklessness (burst CD)
    return BTStatus::SUCCESS;
}

// Priest nodes
BTStatus BTPriestPowerWordShield::Tick(BotAI* /*ai*/, BTBlackboard& /*blackboard*/)
{
    // Would cast Power Word: Shield on target
    return BTStatus::SUCCESS;
}

BTStatus BTPriestPrayerOfMending::Tick(BotAI* /*ai*/, BTBlackboard& /*blackboard*/)
{
    // Would cast Prayer of Mending
    return BTStatus::SUCCESS;
}

BTStatus BTPriestShadowWordPain::Tick(BotAI* /*ai*/, BTBlackboard& /*blackboard*/)
{
    // Would cast Shadow Word: Pain (DoT)
    return BTStatus::SUCCESS;
}

// Mage nodes
BTStatus BTMageArcaneBlast::Tick(BotAI* /*ai*/, BTBlackboard& /*blackboard*/)
{
    // Would cast Arcane Blast
    return BTStatus::SUCCESS;
}

BTStatus BTMagePolymorph::Tick(BotAI* /*ai*/, BTBlackboard& /*blackboard*/)
{
    // Would cast Polymorph for CC
    return BTStatus::SUCCESS;
}

BTStatus BTMageArcaneIntellect::Tick(BotAI* /*ai*/, BTBlackboard& /*blackboard*/)
{
    // Would cast Arcane Intellect buff
    return BTStatus::SUCCESS;
}

} // namespace Playerbot
