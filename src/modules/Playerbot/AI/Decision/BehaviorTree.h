/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5 Enhancement: Behavior Tree System
 *
 * This system provides hierarchical combat flow decisions using behavior tree patterns.
 * Behavior trees allow complex AI logic to be structured in a clear, maintainable way.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "../Common/ActionScoringEngine.h"  // For CombatContext enum
#include <vector>
#include <memory>
#include <string>
#include <functional>

class Player;
class Unit;

namespace Playerbot {
namespace bot { namespace ai {

// Forward declarations
struct DecisionVote;

/**
 * @enum NodeStatus
 * @brief Return status for behavior tree nodes
 */
enum class NodeStatus : uint8
{
    SUCCESS,    // Node completed successfully
    FAILURE,    // Node failed
    RUNNING     // Node is still executing
};

/**
 * @enum NodeType
 * @brief Types of behavior tree nodes
 */
enum class NodeType : uint8
{
    COMPOSITE,  // Has children (Sequence, Selector, Parallel)
    DECORATOR,  // Modifies child behavior (Inverter, Repeater)
    LEAF        // Action or condition (no children)
};

/**
 * @class BehaviorNode
 * @brief Base class for all behavior tree nodes
 *
 * Behavior trees consist of nodes that execute in a hierarchical manner.
 * Each node returns SUCCESS, FAILURE, or RUNNING when ticked.
 */
class BehaviorNode
{
public:
    explicit BehaviorNode(const ::std::string& name)
        : _name(name)
        , _type(NodeType::LEAF)
    {}

    virtual ~BehaviorNode() = default;

    /**
     * @brief Execute this node
     * @param bot Bot executing the behavior
     * @param target Target of the behavior
     * @return Node status
     */
    virtual NodeStatus Tick(Player* bot, Unit* target) = 0;

    /**
     * @brief Reset this node to initial state
     */
    virtual void Reset() {}

    [[nodiscard]] const ::std::string& GetName() const { return _name; }
    [[nodiscard]] NodeType GetType() const { return _type; }

protected:
    ::std::string _name;
    NodeType _type;
};

// ============================================================================
// COMPOSITE NODES - Have children, control execution flow
// ============================================================================

/**
 * @class SequenceNode
 * @brief Executes children in order until one fails
 *
 * Returns SUCCESS if all children succeed
 * Returns FAILURE if any child fails
 * Returns RUNNING if current child is running
 */
class SequenceNode : public BehaviorNode
{
public:
    explicit SequenceNode(const ::std::string& name)
        : BehaviorNode(name)
        , _currentChild(0)
    {
        _type = NodeType::COMPOSITE;
    }

    void AddChild(::std::shared_ptr<BehaviorNode> child)
    {
        _children.push_back(child);
    }

    NodeStatus Tick(Player* bot, Unit* target) override
    {
        while (_currentChild < _children.size())
        {
            NodeStatus status = _children[_currentChild]->Tick(bot, target);

            if (status == NodeStatus::FAILURE)
            {
                Reset();
                return NodeStatus::FAILURE;
            }

            if (status == NodeStatus::RUNNING)
                return NodeStatus::RUNNING;

            // SUCCESS - move to next child
            ++_currentChild;
        }

        // All children succeeded
        Reset();
        return NodeStatus::SUCCESS;
    }

    void Reset() override
    {
        _currentChild = 0;
        for (auto& child : _children)
            child->Reset();
    }

private:
    ::std::vector<::std::shared_ptr<BehaviorNode>> _children;
    size_t _currentChild;
};

/**
 * @class SelectorNode
 * @brief Executes children in order until one succeeds
 *
 * Returns SUCCESS if any child succeeds
 * Returns FAILURE if all children fail
 * Returns RUNNING if current child is running
 */
class SelectorNode : public BehaviorNode
{
public:
    explicit SelectorNode(const ::std::string& name)
        : BehaviorNode(name)
        , _currentChild(0)
    {
        _type = NodeType::COMPOSITE;
    }

    void AddChild(::std::shared_ptr<BehaviorNode> child)
    {
        _children.push_back(child);
    }

    NodeStatus Tick(Player* bot, Unit* target) override
    {
        while (_currentChild < _children.size())
        {
            NodeStatus status = _children[_currentChild]->Tick(bot, target);

            if (status == NodeStatus::SUCCESS)
            {
                Reset();
                return NodeStatus::SUCCESS;
            }

            if (status == NodeStatus::RUNNING)
                return NodeStatus::RUNNING;

            // FAILURE - try next child
            ++_currentChild;
        }

        // All children failed
        Reset();
        return NodeStatus::FAILURE;
    }

    void Reset() override
    {
        _currentChild = 0;
        for (auto& child : _children)
            child->Reset();
    }

private:
    ::std::vector<::std::shared_ptr<BehaviorNode>> _children;
    size_t _currentChild;
};

// ============================================================================
// DECORATOR NODES - Modify child behavior
// ============================================================================

/**
 * @class InverterNode
 * @brief Inverts child result (SUCCESS ↔ FAILURE)
 */
class InverterNode : public BehaviorNode
{
public:
    InverterNode(const ::std::string& name, ::std::shared_ptr<BehaviorNode> child)
        : BehaviorNode(name)
        , _child(child)
    {
        _type = NodeType::DECORATOR;
    }

    NodeStatus Tick(Player* bot, Unit* target) override
    {
        NodeStatus status = _child->Tick(bot, target);

        if (status == NodeStatus::SUCCESS)
            return NodeStatus::FAILURE;
        if (status == NodeStatus::FAILURE)
            return NodeStatus::SUCCESS;

        return NodeStatus::RUNNING;
    }

    void Reset() override
    {
        _child->Reset();
    }

private:
    ::std::shared_ptr<BehaviorNode> _child;
};

/**
 * @class RepeaterNode
 * @brief Repeats child N times or until failure
 */
class RepeaterNode : public BehaviorNode
{
public:
    RepeaterNode(const ::std::string& name, ::std::shared_ptr<BehaviorNode> child, uint32 maxRepeats = 0)
        : BehaviorNode(name)
        , _child(child)
        , _maxRepeats(maxRepeats)
        , _currentRepeats(0)
    {
        _type = NodeType::DECORATOR;
    }

    NodeStatus Tick(Player* bot, Unit* target) override
    {
        // Check if we've hit max repeats (0 = infinite)
        if (_maxRepeats > 0 && _currentRepeats >= _maxRepeats)
        {
            Reset();
            return NodeStatus::SUCCESS;
        }

        NodeStatus status = _child->Tick(bot, target);

        if (status == NodeStatus::FAILURE)
        {
            Reset();
            return NodeStatus::FAILURE;
        }

        if (status == NodeStatus::SUCCESS)
        {
            ++_currentRepeats;
            _child->Reset();

            // Check if we should continue
            if (_maxRepeats == 0 || _currentRepeats < _maxRepeats)
                return NodeStatus::RUNNING;

            Reset();
            return NodeStatus::SUCCESS;
        }

        return NodeStatus::RUNNING;
    }

    void Reset() override
    {
        _currentRepeats = 0;
        _child->Reset();
    }

private:
    ::std::shared_ptr<BehaviorNode> _child;
    uint32 _maxRepeats;
    uint32 _currentRepeats;
};

// ============================================================================
// LEAF NODES - Actions and Conditions
// ============================================================================

/**
 * @class ConditionNode
 * @brief Checks a condition and returns SUCCESS/FAILURE
 */
class ConditionNode : public BehaviorNode
{
public:
    using ConditionFunc = ::std::function<bool(Player*, Unit*)>;

    ConditionNode(const ::std::string& name, ConditionFunc condition)
        : BehaviorNode(name)
        , _condition(condition)
    {
        _type = NodeType::LEAF;
    }

    NodeStatus Tick(Player* bot, Unit* target) override
    {
        return _condition(bot, target) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
    }

private:
    ConditionFunc _condition;
};

/**
 * @class ActionNode
 * @brief Executes an action and returns status
 */
class ActionNode : public BehaviorNode
{
public:
    using ActionFunc = ::std::function<NodeStatus(Player*, Unit*)>;

    ActionNode(const ::std::string& name, ActionFunc action)
        : BehaviorNode(name)
        , _action(action)
    {
        _type = NodeType::LEAF;
    }

    NodeStatus Tick(Player* bot, Unit* target) override
    {
        return _action(bot, target);
    }

private:
    ActionFunc _action;
};

/**
 * @class BehaviorTree
 * @brief Main behavior tree class
 *
 * A behavior tree is a hierarchical structure of nodes that execute to
 * produce intelligent bot behavior. Trees are evaluated each tick and
 * can maintain state across ticks (via RUNNING status).
 *
 * **Example: Healer Behavior Tree**
 * @code
 * auto tree = std::make_shared<BehaviorTree>("Healer");
 *
 * // Root selector: Try emergency heal, then tank heal, then DPS heal
 * auto root = std::make_shared<SelectorNode>("Root");
 *
 * // Emergency heal sequence
 * auto emergencySeq = std::make_shared<SequenceNode>("Emergency");
 * emergencySeq->AddChild(std::make_shared<ConditionNode>("Self < 30%",
 *     [](Player* bot, Unit*) { return bot->GetHealthPct() < 30.0f; }));
 * emergencySeq->AddChild(std::make_shared<ActionNode>("Cast Flash Heal",
 *     [](Player* bot, Unit*) {
 *         // Cast Flash Heal on self
 *         return NodeStatus::SUCCESS;
 *     }));
 * root->AddChild(emergencySeq);
 *
 * tree->SetRoot(root);
 * tree->Tick(bot, target);
 * @endcode
 */
class TC_GAME_API BehaviorTree
{
public:
    explicit BehaviorTree(const ::std::string& name);

    /**
     * @brief Set the root node of the tree
     */
    void SetRoot(::std::shared_ptr<BehaviorNode> root);

    /**
     * @brief Execute one tick of the behavior tree
     * @return Status of root node
     */
    NodeStatus Tick(Player* bot, Unit* target);

    /**
     * @brief Reset the entire tree
     */
    void Reset();

    /**
     * @brief Get DecisionVote for integration with DecisionFusion
     * @return Vote representing tree's recommended action
     */
    [[nodiscard]] DecisionVote GetVote(Player* bot, Unit* target, CombatContext context) const;

    /**
     * @brief Get tree name
     */
    [[nodiscard]] const ::std::string& GetName() const { return _name; }

    /**
     * @brief Check if tree is currently running
     */
    [[nodiscard]] bool IsRunning() const { return _lastStatus == NodeStatus::RUNNING; }

    /**
     * @brief Get last tick status
     */
    [[nodiscard]] NodeStatus GetLastStatus() const { return _lastStatus; }

    /**
     * @brief Enable/disable debug logging
     */
    void EnableDebugLogging(bool enable) { _debugLogging = enable; }

private:
    ::std::string _name;
    ::std::shared_ptr<BehaviorNode> _root;
    NodeStatus _lastStatus;
    bool _debugLogging;

    // For DecisionVote generation
    uint32 _recommendedAction;
    Unit* _recommendedTarget;
};

// ============================================================================
// HELPER FACTORY FUNCTIONS - Convenience builders for cleaner syntax
// ============================================================================

/**
 * @brief Create a Sequence node with children
 * @example auto seq = Sequence("MySequence", { childNode1, childNode2 });
 */
inline ::std::shared_ptr<SequenceNode> Sequence(const ::std::string& name, ::std::vector<::std::shared_ptr<BehaviorNode>> children = {})
{
    auto node = ::std::make_shared<SequenceNode>(name);
    for (auto& child : children)
        node->AddChild(child);
    return node;
}

/**
 * @brief Create a Selector node with children
 * @example auto sel = Selector("MySelector", { childNode1, childNode2 });
 */
inline ::std::shared_ptr<SelectorNode> Selector(const ::std::string& name, ::std::vector<::std::shared_ptr<BehaviorNode>> children = {})
{
    auto node = ::std::make_shared<SelectorNode>(name);
    for (auto& child : children)
        node->AddChild(child);
    return node;
}

/**
 * @brief Create a Condition node
 * @example auto cond = Condition("Check Health", [](Player* bot, Unit*) { return bot->GetHealthPct() < 50.0f; });
 */
inline ::std::shared_ptr<ConditionNode> Condition(const ::std::string& name, ::std::function<bool(Player*, Unit*)> condition)
{
    return ::std::make_shared<ConditionNode>(name, condition);
}

/**
 * @brief Create an Action node
 * @example auto act = Action("Cast Heal", [](Player* bot, Unit* target) { ... return NodeStatus::SUCCESS; });
 */
inline ::std::shared_ptr<ActionNode> Action(const ::std::string& name, ::std::function<NodeStatus(Player*, Unit*)> action)
{
    return ::std::make_shared<ActionNode>(name, action);
}

/**
 * @brief Create an Inverter node
 */
inline ::std::shared_ptr<InverterNode> Inverter(const ::std::string& name, ::std::shared_ptr<BehaviorNode> child)
{
    return ::std::make_shared<InverterNode>(name, child);
}

/**
 * @brief Create a Repeater node
 */
inline ::std::shared_ptr<RepeaterNode> Repeater(const ::std::string& name, ::std::shared_ptr<BehaviorNode> child, uint32 maxRepeats = 0)
{
    return ::std::make_shared<RepeaterNode>(name, child, maxRepeats);
}

}} // namespace bot::ai
} // namespace Playerbot
