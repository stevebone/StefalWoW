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

#ifndef TRINITYCORE_BEHAVIOR_TREE_FACTORY_H
#define TRINITYCORE_BEHAVIOR_TREE_FACTORY_H

#include "BehaviorTree.h"
#include "Nodes/CombatNodes.h"
#include "Nodes/HealingNodes.h"
#include "Nodes/MovementNodes.h"
#include "AI/BotAI.h"
#include <memory>
#include <unordered_map>
#include <functional>

namespace Playerbot
{

/**
 * @brief Factory for creating pre-defined Behavior Trees
 * Provides builder methods for common bot behaviors (combat, healing, movement)
 */
class TC_GAME_API BehaviorTreeFactory
{
public:
    /**
     * @brief Tree types enum for factory creation
     */
    enum class TreeType
    {
        // Combat trees
        MELEE_COMBAT,
        RANGED_COMBAT,
        TANK_COMBAT,

        // Healing trees
        SINGLE_TARGET_HEALING,
        GROUP_HEALING,
        DISPEL_PRIORITY,

        // Movement trees
        FOLLOW_LEADER,
        COMBAT_POSITIONING,
        FLEE_TO_SAFETY,

        // Utility trees
        BUFF_MAINTENANCE,
        RESOURCE_MANAGEMENT
    };

    /**
     * @brief Create a behavior tree by type
     * @param type Tree type to create
     * @return Shared pointer to root node
     */
    static ::std::shared_ptr<BTNode> CreateTree(TreeType type);

    /**
     * @brief Build melee DPS combat tree
     * Root: Selector
     * - Flee if critically wounded
     * - Sequence: Combat
     *   - Check has target
     *   - Check in range (0-5 yards)
     *   - Use defensive CDs if low health
     *   - Face target
     *   - Execute melee attack
     */
    static ::std::shared_ptr<BTNode> BuildMeleeCombatTree();

    /**
     * @brief Build ranged DPS combat tree
     * Root: Selector
     * - Flee if critically wounded
     * - Sequence: Combat
     *   - Check has target
     *   - Check in range (5-40 yards)
     *   - Move to optimal range if needed
     *   - Use defensive CDs if low health
     *   - Face target
     *   - Cast ranged spells
     */
    static ::std::shared_ptr<BTNode> BuildRangedCombatTree();

    /**
     * @brief Build tank combat tree
     * Root: Selector
     * - Use defensive CDs if critical
     * - Sequence: Tanking
     *   - Check has target
     *   - Check in range (0-10 yards)
     *   - Face target
     *   - Use threat generation abilities
     *   - Maintain defensive stance
     */
    static ::std::shared_ptr<BTNode> BuildTankCombatTree();

    /**
     * @brief Build single-target healing tree
     * Root: Selector
     * - Heal self if critical
     * - Sequence: Heal ally
     *   - Find most wounded ally
     *   - Check in range (0-40 yards)
     *   - Check line of sight
     *   - Select appropriate heal spell
     *   - Cast heal
     */
    static ::std::shared_ptr<BTNode> BuildSingleTargetHealingTree();

    /**
     * @brief Build group healing tree (AoE heals)
     * Root: Selector
     * - Heal self if critical
     * - Sequence: AoE Heal
     *   - Check multiple wounded (3+ allies <80% health)
     *   - Check mana sufficient
     *   - Cast AoE heal
     * - Fallback: Single target heal
     */
    static ::std::shared_ptr<BTNode> BuildGroupHealingTree();

    /**
     * @brief Build dispel priority tree
     * Root: Selector
     * - Dispel magic debuffs (high priority)
     * - Dispel curse debuffs
     * - Dispel disease debuffs
     * - Dispel poison debuffs
     */
    static ::std::shared_ptr<BTNode> BuildDispelPriorityTree();

    /**
     * @brief Build follow leader movement tree
     * Root: Sequence
     * - Check not in combat
     * - Check too far from leader (>10 yards)
     * - Move to leader
     */
    static ::std::shared_ptr<BTNode> BuildFollowLeaderTree();

    /**
     * @brief Build combat positioning tree
     * Root: Selector
     * - Sequence: Melee positioning
     *   - Check is melee class
     *   - Check too far from target (>5 yards)
     *   - Move to target (0-5 yards)
     * - Sequence: Ranged positioning
     *   - Check is ranged class
     *   - Check not in optimal range (20-35 yards)
     *   - Move to optimal range
     */
    static ::std::shared_ptr<BTNode> BuildCombatPositioningTree();

    /**
     * @brief Build flee to safety tree
     * Root: Sequence
     * - Check health below 20%
     * - Check in combat
     * - Find safe position
     * - Move to safe position
     * - Stop movement when safe
     */
    static ::std::shared_ptr<BTNode> BuildFleeToSafetyTree();

    /**
     * @brief Build buff maintenance tree
     * Root: Sequence
     * - Check not in combat OR combat duration < 5 seconds
     * - Check missing self buff
     * - Cast self buff
     */
    static ::std::shared_ptr<BTNode> BuildBuffMaintenanceTree();

    /**
     * @brief Build resource management tree (mana/rage/energy)
     * Root: Selector
     * - Sequence: Drink/Eat
     *   - Check not in combat
     *   - Check mana < 30%
     *   - Use consumable
     * - Sequence: Conserve mana
     *   - Check mana < 50%
     *   - Avoid expensive spells
     */
    static ::std::shared_ptr<BTNode> BuildResourceManagementTree();

    /**
     * @brief Register custom tree builder
     * Allows external code to register custom tree types
     * @param name Unique tree name
     * @param builder Lambda that builds the tree
     */
    static void RegisterCustomTree(::std::string const& name,
        ::std::function<::std::shared_ptr<BTNode>()> builder);

    /**
     * @brief Create custom tree by name
     * @param name Registered tree name
     * @return Shared pointer to root node or nullptr if not found
     */
    static ::std::shared_ptr<BTNode> CreateCustomTree(::std::string const& name);

private:
    // Registry of custom tree builders
    static ::std::unordered_map<::std::string, ::std::function<::std::shared_ptr<BTNode>()>> _customTreeBuilders;
};

} // namespace Playerbot

#endif // TRINITYCORE_BEHAVIOR_TREE_FACTORY_H
