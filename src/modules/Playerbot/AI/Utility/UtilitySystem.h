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

#ifndef TRINITYCORE_UTILITY_SYSTEM_H
#define TRINITYCORE_UTILITY_SYSTEM_H

#include "Define.h"
#include "GameTime.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <algorithm>
#include <cmath>

namespace Playerbot
{

class BotAI;
class SharedBlackboard;

/**
 * @brief Context for utility evaluation
 * Contains all relevant state for decision-making
 */
struct UtilityContext
{
    BotAI* bot;
    SharedBlackboard* blackboard;

    // Bot state
    float healthPercent;
    float manaPercent;
    bool inCombat;
    bool hasAggro;

    // Group state
    bool inGroup;
    uint32 groupSize;
    float lowestAllyHealthPercent;
    uint32 enemiesInRange;

    // Role state
    enum class Role
    {
        TANK,
        HEALER,
        DPS,
        SUPPORT
    } role;

    // Timing
    uint32 timeSinceCombatStart;
    uint32 lastDecisionTime;

    UtilityContext()
        : bot(nullptr)
        , blackboard(nullptr)
        , healthPercent(1.0f)
        , manaPercent(1.0f)
        , inCombat(false)
        , hasAggro(false)
        , inGroup(false)
        , groupSize(1)
        , lowestAllyHealthPercent(1.0f)
        , enemiesInRange(0)
        , role(Role::DPS)
        , timeSinceCombatStart(0)
        , lastDecisionTime(0)
    {}
};

/**
 * @brief Base class for utility evaluators
 * Scores a specific behavior based on context
 */
class TC_GAME_API UtilityEvaluator
{
public:
    UtilityEvaluator(::std::string const& name, float weight = 1.0f)
        : _name(name), _weight(weight) {}

    virtual ~UtilityEvaluator() = default;

    /**
     * @brief Calculate utility score for this behavior
     * @param context Current bot/world state
     * @return Score between 0.0 (useless) and 1.0 (critical)
     */
    virtual float Evaluate(UtilityContext const& context) const = 0;

    /**
     * @brief Get the name of this evaluator
     */
    ::std::string const& GetName() const { return _name; }

    /**
     * @brief Get the weight multiplier
     */
    float GetWeight() const { return _weight; }

    /**
     * @brief Set the weight multiplier
     */
    void SetWeight(float weight) { _weight = weight; }

    /**
     * @brief Calculate weighted score
     */
    float GetWeightedScore(UtilityContext const& context) const
    {
        return Evaluate(context) * _weight;
    }

protected:
    ::std::string _name;
    float _weight;

    // Utility curve functions
    static float Linear(float x) { return x; }
    static float Quadratic(float x) { return x * x; }
    static float Cubic(float x) { return x * x * x; }
    static float InverseLinear(float x) { return 1.0f - x; }
    static float Logistic(float x, float steepness = 10.0f)
    {
        return 1.0f / (1.0f + ::std::exp(-steepness * (x - 0.5f)));
    }
    static float Clamp(float value, float min = 0.0f, float max = 1.0f)
    {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
};

/**
 * @brief Utility-based behavior
 * Combines multiple evaluators to score a behavior
 */
class TC_GAME_API UtilityBehavior
{
public:
    UtilityBehavior(::std::string const& name)
        : _name(name), _cachedScore(0.0f), _lastEvalTime(0) {}

    /**
     * @brief Add an evaluator to this behavior
     */
    void AddEvaluator(::std::shared_ptr<UtilityEvaluator> evaluator)
    {
        _evaluators.push_back(evaluator);
    }

    /**
     * @brief Calculate total utility score
     * Combines all evaluator scores using multiplicative combination
     */
    float CalculateUtility(UtilityContext const& context)
    {
        if (_evaluators.empty())
            return 0.0f;

        // Multiplicative combination (all factors must be favorable)
        float score = 1.0f;
        for (auto const& evaluator : _evaluators)
        {
            score *= evaluator->GetWeightedScore(context);
        }

        _cachedScore = score;
        _lastEvalTime = GameTime::GetGameTimeMS();

        return score;
    }

    /**
     * @brief Get the behavior name
     */
    ::std::string const& GetName() const { return _name; }

    /**
     * @brief Get cached score (no recalculation)
     */
    float GetCachedScore() const { return _cachedScore; }

    /**
     * @brief Get time since last evaluation
     */
    uint32 GetTimeSinceEval() const { return GameTime::GetGameTimeMS() - _lastEvalTime; }

    /**
     * @brief Get all evaluators
     */
    ::std::vector<::std::shared_ptr<UtilityEvaluator>> const& GetEvaluators() const
    {
        return _evaluators;
    }

private:
    ::std::string _name;
    ::std::vector<::std::shared_ptr<UtilityEvaluator>> _evaluators;
    float _cachedScore;
    uint32 _lastEvalTime;
};

/**
 * @brief Utility-based decision maker
 * Selects highest-scoring behavior
 */
class TC_GAME_API UtilityAI
{
public:
    UtilityAI() = default;

    /**
     * @brief Register a behavior
     */
    void AddBehavior(::std::shared_ptr<UtilityBehavior> behavior)
    {
        _behaviors.push_back(behavior);
    }

    /**
     * @brief Select the best behavior based on current context
     * @return Pointer to highest-scoring behavior, or nullptr
     */
    UtilityBehavior* SelectBehavior(UtilityContext const& context)
    {
        if (_behaviors.empty())
            return nullptr;

        float bestScore = -1.0f;
        UtilityBehavior* bestBehavior = nullptr;

        for (auto& behavior : _behaviors)
        {
            float score = behavior->CalculateUtility(context);

            if (score > bestScore)
            {
                bestScore = score;
                bestBehavior = behavior.get();
            }
        }

        return bestBehavior;
    }

    /**
     * @brief Get all behaviors sorted by score (descending)
     */
    ::std::vector<::std::pair<UtilityBehavior*, float>> GetRankedBehaviors(UtilityContext const& context)
    {
        ::std::vector<::std::pair<UtilityBehavior*, float>> ranked;

        for (auto& behavior : _behaviors)
        {
            float score = behavior->CalculateUtility(context);
            ranked.emplace_back(behavior.get(), score);
        }

        ::std::sort(ranked.begin(), ranked.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        return ranked;
    }

    /**
     * @brief Get all registered behaviors
     */
    ::std::vector<::std::shared_ptr<UtilityBehavior>> const& GetBehaviors() const
    {
        return _behaviors;
    }

    /**
     * @brief Clear all behaviors
     */
    void Clear()
    {
        _behaviors.clear();
    }

private:
    ::std::vector<::std::shared_ptr<UtilityBehavior>> _behaviors;
};

} // namespace Playerbot

#endif // TRINITYCORE_UTILITY_SYSTEM_H
