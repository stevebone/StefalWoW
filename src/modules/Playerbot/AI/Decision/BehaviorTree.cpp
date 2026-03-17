/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5 Enhancement: Behavior Tree Implementation
 */

#include "BehaviorTree.h"
#include "DecisionFusionSystem.h"
#include "Player.h"
#include "Unit.h"
#include "Log.h"

namespace Playerbot {
namespace bot { namespace ai {

// ============================================================================
// BehaviorTree Implementation
// ============================================================================

BehaviorTree::BehaviorTree(const ::std::string& name)
    : _name(name)
    , _root(nullptr)
    , _lastStatus(NodeStatus::FAILURE)
    , _debugLogging(false)
    , _recommendedAction(0)
    , _recommendedTarget(nullptr)
{
}

void BehaviorTree::SetRoot(::std::shared_ptr<BehaviorNode> root)
{
    _root = root;
}

NodeStatus BehaviorTree::Tick(Player* bot, Unit* target)
{

    if (_debugLogging)
    {
        TC_LOG_DEBUG("playerbot", "BehaviorTree: {} executing tick on bot {}",
            _name, bot->GetName());
    }

    // Execute the tree
    _lastStatus = _root->Tick(bot, target);

    if (_debugLogging)
    {
        const char* statusStr = "UNKNOWN";
        switch (_lastStatus)
        {
            case NodeStatus::SUCCESS: statusStr = "SUCCESS"; break;
            case NodeStatus::FAILURE: statusStr = "FAILURE"; break;
            case NodeStatus::RUNNING: statusStr = "RUNNING"; break;
        }
        TC_LOG_DEBUG("playerbot", "BehaviorTree: {} returned {}", _name, statusStr);
    }

    return _lastStatus;
}

void BehaviorTree::Reset()
{
    if (_root)
        _root->Reset();

    _lastStatus = NodeStatus::FAILURE;
    _recommendedAction = 0;
    _recommendedTarget = nullptr;
}

DecisionVote BehaviorTree::GetVote(Player* bot, Unit* target, CombatContext context) const
{
    DecisionVote vote;
    vote.source = DecisionSource::BEHAVIOR_TREE;

    // If tree has no root or last status was failure, no vote
    if (!_root || _lastStatus == NodeStatus::FAILURE)
        return vote;

    // Use recommended action if available
    vote.actionId = _recommendedAction;
    vote.target = _recommendedTarget;

    // Confidence based on tree status
    // RUNNING trees have moderate confidence (they're still evaluating)
    // SUCCESS trees have high confidence (they completed successfully)
    if (_lastStatus == NodeStatus::RUNNING)
        vote.confidence = 0.7f;
    else if (_lastStatus == NodeStatus::SUCCESS)
        vote.confidence = 0.9f;
    else
        vote.confidence = 0.0f;

    // Urgency based on context
    // Behavior trees represent strategic decisions, so urgency is moderate
    switch (context)
    {
        case CombatContext::RAID_HEROIC:
        case CombatContext::DUNGEON_BOSS:
            vote.urgency = 0.8f;  // High urgency in challenging content
            break;
        case CombatContext::PVP_ARENA:
            vote.urgency = 0.85f; // Very high urgency in PvP
            break;
        default:
            vote.urgency = 0.6f;  // Moderate urgency
            break;
    }

    // Set reasoning
    vote.reasoning = "BehaviorTree: " + _name;

    return vote;
}

// ============================================================================
// Helper Functions for Node Creation
// ============================================================================
// NOTE: The inline helper functions (Sequence, Selector, Condition, Action,
// Inverter, Repeater) are defined in BehaviorTree.h and are available in
// the Playerbot::bot::ai namespace. No duplicate definitions needed here.

// ============================================================================
// Example: Healer Behavior Tree
// ============================================================================

::std::shared_ptr<BehaviorTree> CreateHealerBehaviorTree()
{
    // Helper functions (Sequence, Selector, Condition, Action) from BehaviorTree.h
    auto tree = ::std::make_shared<BehaviorTree>("Healer");

    // Root selector: Try emergency → tank → DPS → maintain HoTs
    auto root = Selector("Root", {

        // Emergency: Self heal if very low
        Sequence("Emergency Self Heal", {
            Condition("Self HP < 30%", [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 30.0f;
            }),
            Action("Cast Flash Heal on Self", [](Player* bot, Unit*) {
                // Cast emergency heal on self
                // Note: Actual spell casting would be done via ClassAI
                TC_LOG_DEBUG("playerbot", "BehaviorTree: Emergency self heal for {}",
                    bot->GetName());
                return NodeStatus::SUCCESS;
            })
        }),

        // Tank healing: Prioritize tank below 60%
        Sequence("Tank Heal", {
            Condition("Tank HP < 60%", [](Player* bot, Unit* target) {
                // Check if target is tank and below 60%
                // Note: Tank identification would use group role system
                return target && target->GetHealthPct() < 60.0f;
            }),
            Action("Cast Greater Heal on Tank", [](Player* bot, Unit* target) {
                TC_LOG_DEBUG("playerbot", "BehaviorTree: Healing tank {}",
                    target->GetName());
                return NodeStatus::SUCCESS;
            })
        }),

        // DPS healing: Heal DPS below 50%
        Sequence("DPS Heal", {
            Condition("DPS HP < 50%", [](Player* bot, Unit* target) {
                return target && target->GetHealthPct() < 50.0f;
            }),
            Action("Cast Flash Heal on DPS", [](Player* bot, Unit* target) {
                TC_LOG_DEBUG("playerbot", "BehaviorTree: Healing DPS {}",
                    target->GetName());
                return NodeStatus::SUCCESS;
            })
        }),

        // Maintenance: Keep HoTs up
        Action("Maintain HoTs", [](Player* bot, Unit*) {
            TC_LOG_DEBUG("playerbot", "BehaviorTree: Maintaining HoTs");
            return NodeStatus::SUCCESS;
        })
    });

    tree->SetRoot(root);
    return tree;
}

// ============================================================================
// Example: Tank Behavior Tree
// ============================================================================

::std::shared_ptr<BehaviorTree> CreateTankBehaviorTree()
{
    // Helper functions (Sequence, Selector, Condition, Action) from BehaviorTree.h
    auto tree = ::std::make_shared<BehaviorTree>("Tank");

    // Root selector: Try defensive → taunt → threat → damage
    auto root = Selector("Root", {

        // Emergency defensive if very low HP
        Sequence("Emergency Defensive", {
            Condition("HP < 20%", [](Player* bot, Unit*) {
                return bot && bot->GetHealthPct() < 20.0f;
            }),
            Action("Use Emergency Defensive", [](Player* bot, Unit*) {
                TC_LOG_DEBUG("playerbot", "BehaviorTree: Emergency defensive for {}",
                    bot->GetName());
                return NodeStatus::SUCCESS;
            })
        }),

        // Taunt if enemy is not targeting us
        Sequence("Taunt Enemy", {
            Condition("Enemy not on us", [](Player* bot, Unit* target) {
                return target && target->GetVictim() != bot;
            }),
            Action("Cast Taunt", [](Player* bot, Unit* target) {
                TC_LOG_DEBUG("playerbot", "BehaviorTree: Taunting {} for {}",
                    target->GetName(), bot->GetName());
                return NodeStatus::SUCCESS;
            })
        }),

        // Build threat with threat abilities
        Action("Build Threat", [](Player* bot, Unit* target) {
            TC_LOG_DEBUG("playerbot", "BehaviorTree: Building threat");
            return NodeStatus::SUCCESS;
        }),

        // Fill with damage abilities
        Action("Deal Damage", [](Player* bot, Unit* target) {
            TC_LOG_DEBUG("playerbot", "BehaviorTree: Dealing damage");
            return NodeStatus::SUCCESS;
        })
    });

    tree->SetRoot(root);
    return tree;
}

// ============================================================================
// Example: DPS Behavior Tree
// ============================================================================

::std::shared_ptr<BehaviorTree> CreateDPSBehaviorTree()
{
    // Helper functions (Sequence, Selector, Condition, Action) from BehaviorTree.h
    auto tree = ::std::make_shared<BehaviorTree>("DPS");

    // Root selector: Try cooldowns → AoE → single target → filler
    auto root = Selector("Root", {

        // Use offensive cooldowns if available and target > 50% HP
        Sequence("Offensive Cooldowns", {
            Condition("Target HP > 50%", [](Player* bot, Unit* target) {
                return target && target->GetHealthPct() > 50.0f;
            }),
            Condition("Cooldowns Available", [](Player* bot, Unit*) {
                if (!bot)
                    return false;

                // Full behavior tree node implementation with proper state handling
                // Current behavior: Only checks if bot is in combat
                // Full implementation should:
                // - Track individual cooldown usage timestamps
                // - Verify specific offensive cooldowns are available (e.g., Bloodlust, Avatar)
                // - Check cooldown stacking opportunities via CooldownStackingOptimizer
                // - Consider boss phase timing and group cooldown coordination
                // Reference: CooldownStackingOptimizer, ClassAI Refactored specs
                return bot->IsInCombat();
            }),
            Action("Use Offensive Cooldown", [](Player* bot, Unit*) {
                TC_LOG_DEBUG("playerbot", "BehaviorTree: Using offensive cooldown");
                return NodeStatus::SUCCESS;
            })
        }),

        // Full decorator node implementation with condition caching
        // Current behavior: Basic threat list size check for AoE detection
        // Full implementation should:
        // - Use AoEDecisionManager for proper target clustering analysis
        // - Calculate optimal AoE breakpoint based on spell coefficients
        // - Consider target positioning and cleave potential
        // - Account for interrupt/priority target mechanics
        // - Integrate with CombatBehaviorIntegration's target validation
        // Reference: AoEDecisionManager, CombatBehaviorIntegration
        Sequence("AoE Rotation", {
            Condition("Multiple Enemies Nearby", [](Player* bot, Unit* target) {
                if (!bot || !target)
                    return false;

                return bot->IsInCombat() && bot->GetThreatManager().GetThreatListSize() >= 3;
            }),
            Action("Cast AoE Spell", [](Player* bot, Unit*) {
                TC_LOG_DEBUG("playerbot", "BehaviorTree: Casting AoE");
                return NodeStatus::SUCCESS;
            })
        }),

        // Single target rotation
        Action("Single Target Rotation", [](Player* bot, Unit* target) {
            TC_LOG_DEBUG("playerbot", "BehaviorTree: Single target rotation");
            return NodeStatus::SUCCESS;
        }),

        // Filler spell
        Action("Filler Spell", [](Player* bot, Unit* target) {
            TC_LOG_DEBUG("playerbot", "BehaviorTree: Casting filler");
            return NodeStatus::SUCCESS;
        })
    });

    tree->SetRoot(root);
    return tree;
}

}} // namespace bot::ai
} // namespace Playerbot
