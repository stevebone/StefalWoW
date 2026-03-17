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

#ifndef TRINITYCORE_HYBRID_AI_CONTROLLER_H
#define TRINITYCORE_HYBRID_AI_CONTROLLER_H

#include "Utility/UtilitySystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeFactory.h"
#include "Define.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace Playerbot
{

class BotAI;
class SharedBlackboard;

/**
 * @brief Hybrid AI Controller - Integrates Utility AI with Behavior Trees
 *
 * Architecture:
 * 1. Utility AI evaluates context and selects best behavior (WHAT to do)
 * 2. Controller maps selected behavior to appropriate Behavior Tree
 * 3. Behavior Tree executes the behavior (HOW to do it)
 *
 * Flow:
 * UpdateAI() → UtilityAI::SelectBehavior() → Map to BehaviorTree → BehaviorTree::Tick()
 *
 * Example:
 * Context: Low health (20%), in combat, enemies nearby
 * Utility AI: "Flee" behavior scores highest (0.95)
 * Controller: Maps "Flee" → FleeToSafetyTree
 * Behavior Tree: Executes flee sequence (find safe position → move → stop)
 */
class TC_GAME_API HybridAIController
{
public:
    /**
     * @brief Construct hybrid controller
     * @param ai Bot AI instance
     * @param blackboard Shared blackboard (optional)
     */
    HybridAIController(BotAI* ai, SharedBlackboard* blackboard = nullptr);

    /**
     * @brief Update AI decision and execution
     * Called every frame from BotAI::UpdateAI()
     *
     * @param diff Time since last update (ms)
     * @return True if AI executed successfully, false if error
     */
    bool Update(uint32 diff);

    /**
     * @brief Initialize AI systems
     * Creates Utility AI instance with default behaviors
     * Sets up behavior-to-tree mappings
     */
    void Initialize();

    /**
     * @brief Reset AI state
     * Clears current tree, resets utility AI, clears blackboard
     */
    void Reset();

    /**
     * @brief Get current behavior name
     * @return Name of active utility behavior or "None"
     */
    ::std::string GetCurrentBehaviorName() const;

    /**
     * @brief Get current tree execution status
     * @return BTStatus of active tree or INVALID if no tree
     */
    BTStatus GetCurrentTreeStatus() const;

    /**
     * @brief Get utility AI instance
     * For debugging and testing
     */
    UtilityAI* GetUtilityAI() { return _utilityAI.get(); }

    /**
     * @brief Get current behavior tree
     * For debugging and testing
     */
    BehaviorTree* GetCurrentTree() { return _currentTree.get(); }

    /**
     * @brief Register custom behavior-to-tree mapping
     * Allows external code to add custom behaviors with custom trees
     *
     * @param behaviorName Utility behavior name
     * @param treeType Factory tree type to execute
     */
    void RegisterBehaviorMapping(::std::string const& behaviorName, BehaviorTreeFactory::TreeType treeType);

    /**
     * @brief Register custom behavior with custom tree builder
     * @param behaviorName Utility behavior name
     * @param treeBuilder Lambda that builds custom tree
     */
    void RegisterCustomBehaviorMapping(::std::string const& behaviorName,
        ::std::function<::std::shared_ptr<BTNode>()> treeBuilder);

    /**
     * @brief Get time since last behavior change (in milliseconds)
     */
    uint32 GetTimeSinceLastBehaviorChange() const;

    /**
     * @brief Check if behavior changed this frame
     */
    bool BehaviorChangedThisFrame() const { return _behaviorChangedThisFrame; }

private:
    /**
     * @brief Create default behavior-to-tree mappings
     * Called during Initialize()
     */
    void CreateDefaultBehaviorMappings();

    /**
     * @brief Select best behavior using Utility AI
     * Evaluates current context and returns highest-scoring behavior
     *
     * @param context Current game state
     * @return Selected utility behavior or nullptr
     */
    UtilityBehavior* SelectBehavior(UtilityContext const& context);

    /**
     * @brief Get or create behavior tree for behavior
     * Looks up behavior name in mapping and creates appropriate tree
     *
     * @param behaviorName Name of utility behavior
     * @return Shared pointer to tree root or nullptr if not mapped
     */
    ::std::shared_ptr<BTNode> GetTreeForBehavior(::std::string const& behaviorName);

    /**
     * @brief Switch to new behavior tree
     * Resets old tree, creates new tree, updates tracking
     *
     * @param behaviorName Name of new behavior
     * @param tree New tree root
     */
    void SwitchBehaviorTree(::std::string const& behaviorName, ::std::shared_ptr<BTNode> tree);

    /**
     * @brief Execute current behavior tree
     * Ticks the active tree with bot AI instance
     *
     * @return Tree execution status
     */
    BTStatus ExecuteCurrentTree();

private:
    BotAI* _bot;
    SharedBlackboard* _blackboard;

    // Utility AI system (decision making)
    ::std::unique_ptr<UtilityAI> _utilityAI;

    // Behavior Tree execution
    ::std::unique_ptr<BehaviorTree> _currentTree;
    ::std::string _currentBehaviorName;
    BTStatus _lastTreeStatus;

    // Behavior-to-tree mappings
    ::std::unordered_map<::std::string, BehaviorTreeFactory::TreeType> _behaviorToTreeMap;
    ::std::unordered_map<::std::string, ::std::function<::std::shared_ptr<BTNode>()>> _customTreeBuilders;

    // Timing and tracking
    uint32 _lastDecisionTime;
    uint32 _decisionUpdateInterval; // Update decisions every N ms (default 500ms)
    uint32 _lastBehaviorChangeTime;
    bool _behaviorChangedThisFrame;

    // Performance tracking
    uint32 _totalDecisions;
    uint32 _totalTreeExecutions;
    uint32 _successfulExecutions;
    uint32 _failedExecutions;
};

} // namespace Playerbot

#endif // TRINITYCORE_HYBRID_AI_CONTROLLER_H
