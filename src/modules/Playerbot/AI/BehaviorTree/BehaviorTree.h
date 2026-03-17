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

#ifndef TRINITYCORE_BEHAVIOR_TREE_H
#define TRINITYCORE_BEHAVIOR_TREE_H

#include "Define.h"
#include "Log.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <any>
#include <functional>
#include <algorithm>

namespace Playerbot
{

class BotAI;

/**
 * @brief Behavior Tree node execution status
 */
enum class BTStatus : uint8
{
    SUCCESS,    // Node completed successfully
    FAILURE,    // Node failed
    RUNNING,    // Node is still executing (async)
    INVALID     // Node is invalid or error occurred
};

/**
 * @brief Blackboard for sharing data between BT nodes
 * Similar to BehaviorTree.CPP blackboard but simplified
 */
class TC_GAME_API BTBlackboard
{
public:
    BTBlackboard() = default;

    template<typename T>
    void Set(::std::string const& key, T const& value)
    {
        _data[key] = value;
    }

    template<typename T>
    bool Get(::std::string const& key, T& outValue) const
    {
        auto it = _data.find(key);
        if (it == _data.end())
            return false;

        try
        {
            outValue = ::std::any_cast<T>(it->second);
            return true;
        }
        catch (::std::bad_any_cast const&)
        {
            return false;
        }
    }

    template<typename T>
    T GetOr(::std::string const& key, T const& defaultValue) const
    {
        T value;
        return Get(key, value) ? value : defaultValue;
    }

    bool Has(::std::string const& key) const
    {
        return _data.find(key) != _data.end();
    }

    void Remove(::std::string const& key)
    {
        _data.erase(key);
    }

    void Clear()
    {
        _data.clear();
    }

    /**
     * @brief Get all keys in the blackboard
     * @return Vector of all key names
     */
    ::std::vector<::std::string> GetKeys() const
    {
        ::std::vector<::std::string> keys;
        keys.reserve(_data.size());
        for (auto const& pair : _data)
            keys.push_back(pair.first);
        return keys;
    }

    /**
     * @brief Get raw std::any value for a key
     * @param key Data key
     * @param outValue Output value
     * @return True if key exists
     */
    bool GetAny(::std::string const& key, ::std::any& outValue) const
    {
        auto it = _data.find(key);
        if (it == _data.end())
            return false;
        outValue = it->second;
        return true;
    }

    /**
     * @brief Set raw std::any value
     * @param key Data key
     * @param value Value to store
     */
    void SetAny(::std::string const& key, ::std::any const& value)
    {
        _data[key] = value;
    }

private:
    ::std::unordered_map<::std::string, ::std::any> _data;
};

/**
 * @brief Base class for all Behavior Tree nodes
 */
class TC_GAME_API BTNode
{
public:
    BTNode(::std::string const& name) : _name(name), _status(BTStatus::INVALID) {}
    virtual ~BTNode() = default;

    /**
     * @brief Execute this node
     * @param ai Bot AI instance
     * @param blackboard Shared data blackboard
     * @return Node execution status
     */
    virtual BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) = 0;

    /**
     * @brief Reset node state (called when tree restarts)
     */
    virtual void Reset()
    {
        _status = BTStatus::INVALID;
    }

    /**
     * @brief Get node name
     */
    ::std::string const& GetName() const { return _name; }

    /**
     * @brief Get last execution status
     */
    BTStatus GetStatus() const { return _status; }

protected:
    ::std::string _name;
    BTStatus _status;
};

/**
 * @brief Composite node - has multiple children
 */
class TC_GAME_API BTComposite : public BTNode
{
public:
    BTComposite(::std::string const& name) : BTNode(name), _currentChild(0) {}

    void AddChild(::std::shared_ptr<BTNode> child)
    {
        _children.push_back(child);
    }

    void Reset() override
    {
        BTNode::Reset();
        _currentChild = 0;
        for (auto& child : _children)
            child->Reset();
    }

protected:
    ::std::vector<::std::shared_ptr<BTNode>> _children;
    size_t _currentChild;
};

/**
 * @brief Sequence node - executes children in order until one fails
 * Returns SUCCESS only if ALL children succeed
 */
class TC_GAME_API BTSequence : public BTComposite
{
public:
    BTSequence(::std::string const& name) : BTComposite(name) {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        while (_currentChild < _children.size())
        {
            BTStatus status = _children[_currentChild]->Tick(ai, blackboard);

            if (status == BTStatus::FAILURE)
            {
                _status = BTStatus::FAILURE;
                Reset(); // Reset for next execution
                return _status;
            }

            if (status == BTStatus::RUNNING)
            {
                _status = BTStatus::RUNNING;
                return _status;
            }

            // SUCCESS - move to next child
            _currentChild++;
        }

        // All children succeeded
        _status = BTStatus::SUCCESS;
        Reset(); // Reset for next execution
        return _status;
    }
};

/**
 * @brief Selector/Fallback node - executes children until one succeeds
 * Returns SUCCESS if ANY child succeeds
 */
class TC_GAME_API BTSelector : public BTComposite
{
public:
    BTSelector(::std::string const& name) : BTComposite(name) {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        while (_currentChild < _children.size())
        {
            BTStatus status = _children[_currentChild]->Tick(ai, blackboard);

            if (status == BTStatus::SUCCESS)
            {
                _status = BTStatus::SUCCESS;
                Reset(); // Reset for next execution
                return _status;
            }

            if (status == BTStatus::RUNNING)
            {
                _status = BTStatus::RUNNING;
                return _status;
            }

            // FAILURE - move to next child
            _currentChild++;
        }

        // All children failed
        _status = BTStatus::FAILURE;
        Reset(); // Reset for next execution
        return _status;
    }
};

/**
 * @brief Scored Selector node - evaluates children by score, executes highest scoring child
 * This enables utility-based AI decision making with multi-criteria action scoring
 *
 * Usage Example:
 * @code
 * auto selector = std::make_shared<BTScoredSelector>("SmartHeal");
 *
 * // Add heal tank child with scoring function
 * selector->AddChild(healTankAction, [](BotAI* ai, BTBlackboard& bb) -> float {
 *     float healthUrgency = (100.0f - GetTankHealthPct()) / 100.0f;
 *     float rolePriority = 2.0f; // Tanks are 2x priority
 *     return healthUrgency * rolePriority * 100.0f;
 * });
 *
 * // Add heal DPS child with scoring function
 * selector->AddChild(healDPSAction, [](BotAI* ai, BTBlackboard& bb) -> float {
 *     float healthUrgency = (100.0f - GetDPSHealthPct()) / 100.0f;
 *     return healthUrgency * 100.0f;
 * });
 * @endcode
 */
class TC_GAME_API BTScoredSelector : public BTComposite
{
public:
    using ScoringFunction = ::std::function<float(BotAI*, BTBlackboard&)>;

    /**
     * @brief Construct scored selector node
     * @param name Node name for debugging
     */
    BTScoredSelector(::std::string const& name)
        : BTComposite(name)
        , _debugLogging(false)
    {
    }

    /**
     * @brief Add child node with scoring function
     * @param child Child node to execute if highest scoring
     * @param scoringFunc Function that returns action score (0.0 = lowest, higher = better)
     */
    void AddChild(::std::shared_ptr<BTNode> child, ScoringFunction scoringFunc)
    {
        BTComposite::AddChild(child);
        _scoringFunctions.push_back(scoringFunc);
    }

    /**
     * @brief Execute highest scoring child
     * @param ai Bot AI instance
     * @param blackboard Shared data blackboard
     * @return BTStatus::SUCCESS if highest scoring child succeeds, BTStatus::FAILURE if all fail
     */
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (_children.empty())
        {
            _status = BTStatus::FAILURE;
            return _status;
        }

        // Score all children
        ::std::vector<::std::pair<size_t, float>> scores;
        scores.reserve(_children.size());

        for (size_t i = 0; i < _children.size(); ++i)
        {
            float score = _scoringFunctions[i](ai, blackboard);
            scores.emplace_back(i, score);

            if (_debugLogging)
            {
                TC_LOG_DEBUG("playerbot.bt", "BTScoredSelector [{}]: Child '{}' scored {:.2f}",
                    _name, _children[i]->GetName(), score);
            }
        }

        // Sort by score (highest first)
        ::std::sort(scores.begin(), scores.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        // Try highest scoring children first
        for (const auto& [index, score] : scores)
        {
            if (score <= 0.0f)
                continue;  // Skip non-viable actions

            BTStatus status = _children[index]->Tick(ai, blackboard);

            if (status == BTStatus::SUCCESS)
            {
                if (_debugLogging)
                {
                    TC_LOG_DEBUG("playerbot.bt", "BTScoredSelector [{}]: Executed '{}' (score {:.2f})",
                        _name, _children[index]->GetName(), score);
                }
                _status = BTStatus::SUCCESS;
                Reset();
                return _status;
            }

            if (status == BTStatus::RUNNING)
            {
                _status = BTStatus::RUNNING;
                return _status;
            }

            // FAILURE - try next highest scoring child
        }

        // All viable children failed
        _status = BTStatus::FAILURE;
        Reset();
        return _status;
    }

    /**
     * @brief Enable/disable debug logging for score visualization
     * @param enable True to enable debug logging
     */
    void SetDebugLogging(bool enable)
    {
        _debugLogging = enable;
    }

    /**
     * @brief Check if debug logging is enabled
     * @return True if debug logging enabled
     */
    bool IsDebugLoggingEnabled() const
    {
        return _debugLogging;
    }

private:
    ::std::vector<ScoringFunction> _scoringFunctions;
    bool _debugLogging;
};

/**
 * @brief Decorator node - has single child and modifies its behavior
 */
class TC_GAME_API BTDecorator : public BTNode
{
public:
    BTDecorator(::std::string const& name, ::std::shared_ptr<BTNode> child)
        : BTNode(name), _child(child) {}

    void Reset() override
    {
        BTNode::Reset();
        if (_child)
            _child->Reset();
    }

protected:
    ::std::shared_ptr<BTNode> _child;
};

/**
 * @brief Inverter decorator - inverts child's SUCCESS/FAILURE
 */
class TC_GAME_API BTInverter : public BTDecorator
{
public:
    BTInverter(::std::string const& name, ::std::shared_ptr<BTNode> child)
        : BTDecorator(name, child) {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!_child)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        BTStatus childStatus = _child->Tick(ai, blackboard);

        if (childStatus == BTStatus::SUCCESS)
            _status = BTStatus::FAILURE;
        else if (childStatus == BTStatus::FAILURE)
            _status = BTStatus::SUCCESS;
        else
            _status = childStatus; // RUNNING or INVALID

        return _status;
    }
};

/**
 * @brief Repeater decorator - repeats child N times or until failure
 */
class TC_GAME_API BTRepeater : public BTDecorator
{
public:
    BTRepeater(::std::string const& name, ::std::shared_ptr<BTNode> child, int32 count = -1)
        : BTDecorator(name, child), _maxRepeats(count), _currentRepeat(0) {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!_child)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        // Infinite repeat if maxRepeats < 0
        while (_maxRepeats < 0 || _currentRepeat < _maxRepeats)
        {
            BTStatus childStatus = _child->Tick(ai, blackboard);

            if (childStatus == BTStatus::FAILURE)
            {
                _status = BTStatus::FAILURE;
                _currentRepeat = 0;
                return _status;
            }

            if (childStatus == BTStatus::RUNNING)
            {
                _status = BTStatus::RUNNING;
                return _status;
            }

            // SUCCESS - increment and continue
            _currentRepeat++;
            _child->Reset();
        }

        // Completed all repeats
        _status = BTStatus::SUCCESS;
        _currentRepeat = 0;
        return _status;
    }

private:
    int32 _maxRepeats;
    int32 _currentRepeat;
};

/**
 * @brief Leaf node - performs actual work (condition or action)
 */
class TC_GAME_API BTLeaf : public BTNode
{
public:
    BTLeaf(::std::string const& name) : BTNode(name) {}
};

/**
 * @brief Condition node - tests a condition
 */
class TC_GAME_API BTCondition : public BTLeaf
{
public:
    using ConditionFunc = ::std::function<bool(BotAI*, BTBlackboard&)>;

    BTCondition(::std::string const& name, ConditionFunc func)
        : BTLeaf(name), _func(func) {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!_func)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        bool result = _func(ai, blackboard);
        _status = result ? BTStatus::SUCCESS : BTStatus::FAILURE;
        return _status;
    }

private:
    ConditionFunc _func;
};

/**
 * @brief Action node - performs an action
 */
class TC_GAME_API BTAction : public BTLeaf
{
public:
    using ActionFunc = ::std::function<BTStatus(BotAI*, BTBlackboard&)>;

    BTAction(::std::string const& name, ActionFunc func)
        : BTLeaf(name), _func(func) {}

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (!_func)
        {
            _status = BTStatus::INVALID;
            return _status;
        }

        _status = _func(ai, blackboard);
        return _status;
    }

private:
    ActionFunc _func;
};

/**
 * @brief Behavior Tree - root container for tree structure
 */
class TC_GAME_API BehaviorTree
{
public:
    BehaviorTree() : _root(nullptr) {}

    void SetRoot(::std::shared_ptr<BTNode> root)
    {
        _root = root;
    }

    BTStatus Tick(BotAI* ai)
    {
        if (!_root)
            return BTStatus::INVALID;

        return _root->Tick(ai, _blackboard);
    }

    void Reset()
    {
        if (_root)
            _root->Reset();
        _blackboard.Clear();
    }

    BTBlackboard& GetBlackboard() { return _blackboard; }
    BTBlackboard const& GetBlackboard() const { return _blackboard; }

private:
    ::std::shared_ptr<BTNode> _root;
    BTBlackboard _blackboard;
};

} // namespace Playerbot

#endif // TRINITYCORE_BEHAVIOR_TREE_H
