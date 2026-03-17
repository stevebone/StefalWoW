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

#ifndef TRINITYCORE_CLASS_BEHAVIOR_TREE_REGISTRY_H
#define TRINITYCORE_CLASS_BEHAVIOR_TREE_REGISTRY_H

#include "AI/BehaviorTree/BehaviorTree.h"
#include "Define.h"
#include <unordered_map>
#include <memory>
#include <functional>

namespace Playerbot
{

/**
 * @brief WoW class enumeration (matches TrinityCore Classes enum)
 */
enum class WowClass : uint8
{
    WARRIOR = 1,
    PALADIN = 2,
    HUNTER = 3,
    ROGUE = 4,
    PRIEST = 5,
    DEATH_KNIGHT = 6,
    SHAMAN = 7,
    MAGE = 8,
    WARLOCK = 9,
    MONK = 10,
    DRUID = 11,
    DEMON_HUNTER = 12,
    EVOKER = 13
};

/**
 * @brief Specialization role
 */
enum class SpecRole : uint8
{
    TANK,
    HEALER,
    MELEE_DPS,
    RANGED_DPS
};

/**
 * @brief Class-spec pair for tree lookup
 */
struct ClassSpec
{
    WowClass classId;
    uint8 specId; // 0, 1, 2 for the three specs

    bool operator==(ClassSpec const& other) const
    {
        return classId == other.classId && specId == other.specId;
    }
};

/**
 * @brief Hash function for ClassSpec
 */
struct ClassSpecHash
{
    ::std::size_t operator()(ClassSpec const& spec) const
    {
        return (static_cast<::std::size_t>(spec.classId) << 8) | spec.specId;
    }
};

/**
 * @brief Class Behavior Tree Builder
 * Function that builds a behavior tree for a specific class/spec
 */
using ClassTreeBuilder = ::std::function<::std::shared_ptr<BTNode>()>;

/**
 * @brief Class Behavior Tree Registry
 * Central registry for class-specific behavior trees
 *
 * Manages behavior trees for all 13 classes × 3 specs = 39 combinations
 */
class TC_GAME_API ClassBehaviorTreeRegistry
{
public:
    /**
     * @brief Register behavior tree for class/spec
     * @param classId Class ID
     * @param specId Spec ID (0-2)
     * @param builder Tree builder function
     */
    static void RegisterTree(WowClass classId, uint8 specId, ClassTreeBuilder builder);

    /**
     * @brief Get behavior tree for class/spec
     * @param classId Class ID
     * @param specId Spec ID
     * @return Behavior tree or nullptr if not found
     */
    static ::std::shared_ptr<BTNode> GetTree(WowClass classId, uint8 specId);

    /**
     * @brief Get role for class/spec
     * @param classId Class ID
     * @param specId Spec ID
     * @return Role
     */
    static SpecRole GetRole(WowClass classId, uint8 specId);

    /**
     * @brief Initialize all class trees
     * Called on server startup
     */
    static void Initialize();

    /**
     * @brief Clear all registrations
     */
    static void Clear();

private:
    static ::std::unordered_map<ClassSpec, ClassTreeBuilder, ClassSpecHash> _treeBuilders;
    static ::std::unordered_map<ClassSpec, SpecRole, ClassSpecHash> _specRoles;

    // Class-specific initialization
    static void InitializeWarrior();
    static void InitializePaladin();
    static void InitializeHunter();
    static void InitializeRogue();
    static void InitializePriest();
    static void InitializeDeathKnight();
    static void InitializeShaman();
    static void InitializeMage();
    static void InitializeWarlock();
    static void InitializeMonk();
    static void InitializeDruid();
    static void InitializeDemonHunter();
    static void InitializeEvoker();
};

/**
 * @brief Class-specific BT nodes base
 */
class TC_GAME_API ClassBTNode : public BTNode
{
public:
    ClassBTNode(::std::string const& name, WowClass classId)
        : BTNode(name)
        , _classId(classId)
    {
    }

    WowClass GetClass() const { return _classId; }

protected:
    WowClass _classId;
};

// ============================================================================
// Warrior Nodes
// ============================================================================

/**
 * @brief Warrior: Execute
 */
class TC_GAME_API BTWarriorExecute : public ClassBTNode
{
public:
    BTWarriorExecute() : ClassBTNode("WarriorExecute", WowClass::WARRIOR) {}
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard);
};

/**
 * @brief Warrior: Shield Block (Protection)
 */
class TC_GAME_API BTWarriorShieldBlock : public ClassBTNode
{
public:
    BTWarriorShieldBlock() : ClassBTNode("WarriorShieldBlock", WowClass::WARRIOR) {}
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard);
};

/**
 * @brief Warrior: Recklessness (Arms/Fury)
 */
class TC_GAME_API BTWarriorRecklessness : public ClassBTNode
{
public:
    BTWarriorRecklessness() : ClassBTNode("WarriorRecklessness", WowClass::WARRIOR) {}
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard);
};

// ============================================================================
// Priest Nodes
// ============================================================================

/**
 * @brief Priest: Power Word: Shield
 */
class TC_GAME_API BTPriestPowerWordShield : public ClassBTNode
{
public:
    BTPriestPowerWordShield() : ClassBTNode("PriestPowerWordShield", WowClass::PRIEST) {}
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard);
};

/**
 * @brief Priest: Prayer of Mending
 */
class TC_GAME_API BTPriestPrayerOfMending : public ClassBTNode
{
public:
    BTPriestPrayerOfMending() : ClassBTNode("PriestPrayerOfMending", WowClass::PRIEST) {}
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard);
};

/**
 * @brief Priest: Shadow Word: Pain
 */
class TC_GAME_API BTPriestShadowWordPain : public ClassBTNode
{
public:
    BTPriestShadowWordPain() : ClassBTNode("PriestShadowWordPain", WowClass::PRIEST) {}
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard);
};

// ============================================================================
// Mage Nodes
// ============================================================================

/**
 * @brief Mage: Arcane Blast
 */
class TC_GAME_API BTMageArcaneBlast : public ClassBTNode
{
public:
    BTMageArcaneBlast() : ClassBTNode("MageArcaneBlast", WowClass::MAGE) {}
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard);
};

/**
 * @brief Mage: Polymorph (CC)
 */
class TC_GAME_API BTMagePolymorph : public ClassBTNode
{
public:
    BTMagePolymorph() : ClassBTNode("MagePolymorph", WowClass::MAGE) {}
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard);
};

/**
 * @brief Mage: Arcane Intellect (Buff)
 */
class TC_GAME_API BTMageArcaneIntellect : public ClassBTNode
{
public:
    BTMageArcaneIntellect() : ClassBTNode("MageArcaneIntellect", WowClass::MAGE) {}
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard);
};

} // namespace Playerbot

#endif // TRINITYCORE_CLASS_BEHAVIOR_TREE_REGISTRY_H
