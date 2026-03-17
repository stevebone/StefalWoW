/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BGStrategyEngine.h"
#include "BattlegroundCoordinator.h"
#include "ObjectiveManager.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot {

// ============================================================================
// CONSTRUCTOR
// ============================================================================

BGStrategyEngine::BGStrategyEngine(BattlegroundCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void BGStrategyEngine::Initialize()
{
    Reset();

    TC_LOG_DEBUG("playerbots.bg", "BGStrategyEngine::Initialize - Initialized strategy engine");
}

void BGStrategyEngine::Update(uint32 diff)
{
    // Check if it's time to re-evaluate strategy
    _lastEvaluationTime += diff;
    if (_lastEvaluationTime < _evaluationInterval)
        return;

    _lastEvaluationTime = 0;

    // Don't change strategy if forced
    if (_strategyForced)
        return;

    // Check strategy change cooldown
    _strategyChangedTime += diff;
    if (_strategyChangedTime < _strategyChangeCooldown)
        return;

    // Evaluate best strategy
    StrategicDecision bestDecision = EvaluateBestStrategy();

    // Check if strategy should change
    float currentScore = ScoreStrategy(_currentStrategy);
    float newScore = ScoreStrategy(bestDecision.strategy);

    // Only change if significantly better
    if (newScore > currentScore * (1.0f + _strategyChangeThreshold))
    {
        ApplyStrategy(bestDecision.strategy);
        _currentDecision = bestDecision;
        _strategyChangedTime = 0;

        TC_LOG_DEBUG("playerbots.bg", "BGStrategyEngine::Update - Strategy changed to %u (%.2f > %.2f)",
            static_cast<uint8>(bestDecision.strategy), newScore, currentScore);
    }

    // Clean old momentum events
    CleanOldMomentumEvents();
}

void BGStrategyEngine::Reset()
{
    _currentStrategy = BGStrategy::BALANCED;
    _currentDecision = StrategicDecision();
    _strategyForced = false;
    _lastEvaluationTime = 0;
    _strategyChangedTime = 0;
    _priorityOverrides.clear();
    _momentumHistory.clear();

    TC_LOG_DEBUG("playerbots.bg", "BGStrategyEngine::Reset - Reset strategy engine");
}

// ============================================================================
// STRATEGY CONTROL
// ============================================================================

void BGStrategyEngine::ForceStrategy(BGStrategy strategy)
{
    _currentStrategy = strategy;
    _currentDecision = BuildDecision(strategy);
    _strategyForced = true;

    TC_LOG_DEBUG("playerbots.bg", "BGStrategyEngine::ForceStrategy - Forced strategy to %u",
        static_cast<uint8>(strategy));
}

void BGStrategyEngine::ClearForcedStrategy()
{
    _strategyForced = false;
    _strategyChangedTime = _strategyChangeCooldown; // Allow immediate re-evaluation
}

// ============================================================================
// EVALUATION
// ============================================================================

std::vector<StrategyScore> BGStrategyEngine::EvaluateAllStrategies() const
{
    std::vector<StrategyScore> scores;

    scores.push_back(EvaluateStrategy(BGStrategy::BALANCED));
    scores.push_back(EvaluateStrategy(BGStrategy::AGGRESSIVE));
    scores.push_back(EvaluateStrategy(BGStrategy::DEFENSIVE));
    scores.push_back(EvaluateStrategy(BGStrategy::TURTLE));
    scores.push_back(EvaluateStrategy(BGStrategy::ALL_IN));
    scores.push_back(EvaluateStrategy(BGStrategy::STALL));
    scores.push_back(EvaluateStrategy(BGStrategy::COMEBACK));

    // Sort by total score
    std::sort(scores.begin(), scores.end(),
        [](const StrategyScore& a, const StrategyScore& b)
        {
            return a.totalScore > b.totalScore;
        });

    return scores;
}

StrategyScore BGStrategyEngine::EvaluateStrategy(BGStrategy strategy) const
{
    StrategyScore score;
    score.strategy = strategy;

    // Evaluate different factors
    score.winChanceScore = GetWinProbability() * 30.0f;
    score.resourceScore = GetObjectiveControlFactor() * 25.0f;
    score.momentumScore = GetMomentumFactor() * 20.0f;
    score.riskScore = 0.0f; // Will be adjusted per strategy

    // Get base strategy score
    float baseScore = 0.0f;
    switch (strategy)
    {
        case BGStrategy::BALANCED:
            baseScore = EvaluateBalanced();
            break;
        case BGStrategy::AGGRESSIVE:
            baseScore = EvaluateAggressive();
            break;
        case BGStrategy::DEFENSIVE:
            baseScore = EvaluateDefensive();
            break;
        case BGStrategy::TURTLE:
            baseScore = EvaluateTurtle();
            break;
        case BGStrategy::ALL_IN:
            baseScore = EvaluateAllIn();
            break;
        case BGStrategy::STALL:
            baseScore = EvaluateStall();
            break;
        case BGStrategy::COMEBACK:
            baseScore = EvaluateComeback();
            break;
    }

    score.totalScore = baseScore + score.winChanceScore + score.resourceScore + score.momentumScore;

    return score;
}

StrategicDecision BGStrategyEngine::EvaluateBestStrategy() const
{
    auto scores = EvaluateAllStrategies();
    if (scores.empty())
        return StrategicDecision();

    // Best is first (sorted)
    return BuildDecision(scores.front().strategy);
}

float BGStrategyEngine::ScoreStrategy(BGStrategy strategy) const
{
    return EvaluateStrategy(strategy).totalScore;
}

// ============================================================================
// OBJECTIVE PRIORITY
// ============================================================================

std::vector<uint32> BGStrategyEngine::GetAttackPriorities() const
{
    return DetermineAttackTargets(_currentStrategy);
}

std::vector<uint32> BGStrategyEngine::GetDefendPriorities() const
{
    return DetermineDefenseTargets(_currentStrategy);
}

uint8 BGStrategyEngine::GetObjectivePriority(uint32 objectiveId) const
{
    // Check for override
    auto it = _priorityOverrides.find(objectiveId);
    if (it != _priorityOverrides.end())
        return it->second;

    // Default based on objective
    const BGObjective* objective = _coordinator->GetObjective(objectiveId);
    if (!objective)
        return 5; // Default middle priority

    return static_cast<uint8>(objective->currentPriority);
}

void BGStrategyEngine::OverrideObjectivePriority(uint32 objectiveId, uint8 priority)
{
    _priorityOverrides[objectiveId] = priority;
}

void BGStrategyEngine::ClearPriorityOverrides()
{
    _priorityOverrides.clear();
}

// ============================================================================
// RESOURCE ALLOCATION
// ============================================================================

uint8 BGStrategyEngine::GetOffensePercent() const
{
    return _currentDecision.offenseAllocation;
}

uint8 BGStrategyEngine::GetDefensePercent() const
{
    return _currentDecision.defenseAllocation;
}

uint8 BGStrategyEngine::GetRoamerCount() const
{
    uint8 total = GetOffensePercent() + GetDefensePercent();
    if (total >= 100)
        return 0;

    // Convert remaining percent to player count
    uint32 teamSize = static_cast<uint32>(_coordinator->GetFriendlyPlayers().size());
    return static_cast<uint8>((100 - total) * teamSize / 100);
}

void BGStrategyEngine::GetAllocation(uint8& offense, uint8& defense, uint8& roamer) const
{
    offense = GetOffensePercent();
    defense = GetDefensePercent();
    roamer = GetRoamerCount();
}

// ============================================================================
// TACTICAL QUERIES
// ============================================================================

bool BGStrategyEngine::ShouldContestNode(uint32 nodeId) const
{
    // Always contest if we're aggressive
    if (_currentStrategy == BGStrategy::AGGRESSIVE || _currentStrategy == BGStrategy::ALL_IN)
        return true;

    // Don't contest if turtling
    if (_currentStrategy == BGStrategy::TURTLE)
        return false;

    // Check if node is in our attack priorities
    auto attackPriorities = GetAttackPriorities();
    return std::find(attackPriorities.begin(), attackPriorities.end(), nodeId) != attackPriorities.end();
}

bool BGStrategyEngine::ShouldAbandonNode(uint32 nodeId) const
{
    // Never abandon if defensive
    if (_currentStrategy == BGStrategy::DEFENSIVE || _currentStrategy == BGStrategy::TURTLE)
        return false;

    // Consider abandoning if losing badly and need to consolidate
    if (IsLosing() && _currentStrategy == BGStrategy::COMEBACK)
    {
        // Check if this node is lowest priority
        auto defendPriorities = GetDefendPriorities();
        if (!defendPriorities.empty() && defendPriorities.back() == nodeId)
            return true;
    }

    return false;
}

bool BGStrategyEngine::ShouldRecall() const
{
    // Recall if defensive and under attack
    if (_currentStrategy == BGStrategy::DEFENSIVE || _currentStrategy == BGStrategy::TURTLE)
    {
        // Check if any friendly objectives are under attack
        for (const auto& objective : _coordinator->GetObjectives())
        {
            if (objective.state == BGObjectiveState::CONTROLLED_FRIENDLY && objective.isContested)
                return true;
        }
    }

    return false;
}

bool BGStrategyEngine::ShouldPush() const
{
    return _currentStrategy == BGStrategy::AGGRESSIVE ||
           _currentStrategy == BGStrategy::ALL_IN ||
           _currentStrategy == BGStrategy::COMEBACK;
}

bool BGStrategyEngine::ShouldTurtle() const
{
    return _currentStrategy == BGStrategy::TURTLE;
}

bool BGStrategyEngine::ShouldAllIn() const
{
    return _currentStrategy == BGStrategy::ALL_IN;
}

// ============================================================================
// GAME STATE ANALYSIS
// ============================================================================

float BGStrategyEngine::GetWinProbability() const
{
    float probability = 0.5f; // Start neutral

    // Score factor
    probability += GetScoreFactor() * 0.3f;

    // Objective control
    probability += (GetObjectiveControlFactor() - 0.5f) * 0.2f;

    // Momentum
    probability += GetMomentum() * 0.1f;

    // Time factor
    probability += GetTimeFactor() * 0.1f;

    return std::clamp(probability, 0.0f, 1.0f);
}

bool BGStrategyEngine::IsWinning() const
{
    return GetWinProbability() > 0.6f;
}

bool BGStrategyEngine::IsLosing() const
{
    return GetWinProbability() < 0.4f;
}

bool BGStrategyEngine::IsCloseGame() const
{
    float prob = GetWinProbability();
    return prob >= 0.4f && prob <= 0.6f;
}

bool BGStrategyEngine::IsTimeRunningOut() const
{
    const BGMatchStats& stats = _coordinator->GetMatchStats();

    // Consider time running out if < 3 minutes remain
    return stats.remainingTime < 180000;
}

float BGStrategyEngine::GetMomentum() const
{
    return CalculateMomentum();
}

// ============================================================================
// BG-SPECIFIC STRATEGIES
// ============================================================================

StrategicDecision BGStrategyEngine::EvaluateWSGStrategy() const
{
    StrategicDecision decision;

    // WSG is capture the flag - different logic
    const BGScoreInfo& score = _coordinator->GetScoreInfo();

    int32 scoreDiff = static_cast<int32>(score.friendlyScore) - static_cast<int32>(score.enemyScore);

    if (scoreDiff >= 2)
    {
        // Winning 2-0, play defensive
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.offenseAllocation = 30;
        decision.defenseAllocation = 70;
        decision.reasoning = "Leading 2-0, protect the lead";
    }
    else if (scoreDiff <= -2)
    {
        // Losing 0-2, must be aggressive
        decision.strategy = BGStrategy::ALL_IN;
        decision.offenseAllocation = 80;
        decision.defenseAllocation = 20;
        decision.reasoning = "Down 0-2, must push aggressively";
    }
    else if (scoreDiff > 0)
    {
        // Winning 1-0 or 2-1
        decision.strategy = BGStrategy::BALANCED;
        decision.offenseAllocation = 45;
        decision.defenseAllocation = 55;
        decision.reasoning = "Slight lead, balanced with defensive lean";
    }
    else if (scoreDiff < 0)
    {
        // Losing
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.offenseAllocation = 60;
        decision.defenseAllocation = 40;
        decision.reasoning = "Behind, need to be aggressive";
    }
    else
    {
        // Tied
        decision.strategy = BGStrategy::BALANCED;
        decision.offenseAllocation = 50;
        decision.defenseAllocation = 50;
        decision.reasoning = "Tied game, balanced approach";
    }

    decision.confidence = 0.8f;
    return decision;
}

StrategicDecision BGStrategyEngine::EvaluateABStrategy() const
{
    StrategicDecision decision;

    // AB is resource race - need 3 nodes to win
    uint32 friendlyNodes = 0;
    uint32 enemyNodes = 0;

    for (const auto& objective : _coordinator->GetObjectives())
    {
        if (objective.type == ObjectiveType::CONTROL_POINT)
        {
            if (objective.state == BGObjectiveState::CONTROLLED_FRIENDLY)
                friendlyNodes++;
            else if (objective.state == BGObjectiveState::CONTROLLED_ENEMY)
                enemyNodes++;
        }
    }

    if (friendlyNodes >= 3)
    {
        // Have 3+, defend what we have
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.offenseAllocation = 20;
        decision.defenseAllocation = 80;
        decision.defendObjectives = DetermineDefenseTargets(decision.strategy);
        decision.reasoning = "Holding 3+ nodes, defend";
    }
    else if (friendlyNodes == 2)
    {
        // Need 1 more
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.offenseAllocation = 55;
        decision.defenseAllocation = 45;
        decision.attackObjectives = DetermineAttackTargets(decision.strategy);
        decision.defendObjectives = DetermineDefenseTargets(decision.strategy);
        decision.reasoning = "Have 2, pushing for 3rd";
    }
    else if (friendlyNodes == 1)
    {
        // Critical - need to push
        decision.strategy = BGStrategy::ALL_IN;
        decision.offenseAllocation = 70;
        decision.defenseAllocation = 30;
        decision.attackObjectives = DetermineAttackTargets(decision.strategy);
        decision.reasoning = "Only 1 node, must push";
    }
    else
    {
        // No nodes - emergency
        decision.strategy = BGStrategy::ALL_IN;
        decision.offenseAllocation = 85;
        decision.defenseAllocation = 15;
        decision.attackObjectives = DetermineAttackTargets(decision.strategy);
        decision.reasoning = "No nodes! Full assault";
    }

    decision.confidence = 0.85f;
    return decision;
}

StrategicDecision BGStrategyEngine::EvaluateAVStrategy() const
{
    StrategicDecision decision;

    // AV has multiple objectives - towers, graveyards, bosses
    const BGScoreInfo& score = _coordinator->GetScoreInfo();

    int32 resourceDiff = static_cast<int32>(score.friendlyScore) - static_cast<int32>(score.enemyScore);

    // AV is primarily a race to kill the boss
    if (resourceDiff > 100)
    {
        // Big lead - push for boss
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.offenseAllocation = 70;
        decision.defenseAllocation = 30;
        decision.reasoning = "Resource lead, push boss";
    }
    else if (resourceDiff < -100)
    {
        // Behind - try to stall
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.offenseAllocation = 30;
        decision.defenseAllocation = 70;
        decision.reasoning = "Resource deficit, defend";
    }
    else
    {
        // Close game
        decision.strategy = BGStrategy::BALANCED;
        decision.offenseAllocation = 50;
        decision.defenseAllocation = 50;
        decision.reasoning = "Close game, balanced";
    }

    decision.confidence = 0.75f;
    return decision;
}

StrategicDecision BGStrategyEngine::EvaluateEOTSStrategy() const
{
    StrategicDecision decision;

    // EOTS combines flags and nodes
    uint32 friendlyNodes = 0;

    for (const auto& objective : _coordinator->GetObjectives())
    {
        if (objective.type == ObjectiveType::CONTROL_POINT &&
            objective.state == BGObjectiveState::CONTROLLED_FRIENDLY)
        {
            friendlyNodes++;
        }
    }

    if (friendlyNodes >= 3)
    {
        // Good node control - can focus on flag
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.offenseAllocation = 60;
        decision.defenseAllocation = 40;
        decision.reasoning = "Node control good, push flag";
    }
    else if (friendlyNodes >= 2)
    {
        decision.strategy = BGStrategy::BALANCED;
        decision.offenseAllocation = 50;
        decision.defenseAllocation = 50;
        decision.reasoning = "Decent node control, balanced";
    }
    else
    {
        // Need more nodes first
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.offenseAllocation = 65;
        decision.defenseAllocation = 35;
        decision.reasoning = "Need more nodes, push";
    }

    decision.confidence = 0.8f;
    return decision;
}

// ============================================================================
// STRATEGY EVALUATION (PRIVATE)
// ============================================================================

float BGStrategyEngine::EvaluateBalanced() const
{
    // Balanced is good for close games
    float score = 50.0f;

    if (IsCloseGame())
        score += 20.0f;

    // Less good for extreme situations
    if (IsTimeRunningOut() && IsLosing())
        score -= 30.0f;

    return score;
}

float BGStrategyEngine::EvaluateAggressive() const
{
    float score = 40.0f;

    // Good when slightly behind
    if (IsLosing() && !IsTimeRunningOut())
        score += 25.0f;

    // Good with positive momentum
    if (GetMomentum() > 0.3f)
        score += 15.0f;

    // Bad when winning big
    if (IsWinning() && GetWinProbability() > 0.7f)
        score -= 20.0f;

    return score;
}

float BGStrategyEngine::EvaluateDefensive() const
{
    float score = 40.0f;

    // Good when winning
    if (IsWinning())
        score += 25.0f;

    // Good when time is running out and ahead
    if (IsTimeRunningOut() && IsWinning())
        score += 30.0f;

    // Bad when losing
    if (IsLosing())
        score -= 30.0f;

    return score;
}

float BGStrategyEngine::EvaluateTurtle() const
{
    float score = 20.0f; // Low base - very situational

    // Only good when winning big and time running out
    if (IsWinning() && GetWinProbability() > 0.75f && IsTimeRunningOut())
        score += 50.0f;

    // Very bad otherwise
    if (IsLosing())
        score -= 50.0f;

    return score;
}

float BGStrategyEngine::EvaluateAllIn() const
{
    float score = 20.0f;

    // Good when losing badly with time running out
    if (IsLosing() && IsTimeRunningOut())
        score += 40.0f;

    // Good when very behind
    if (GetWinProbability() < 0.3f)
        score += 30.0f;

    // Bad when winning or even
    if (IsWinning())
        score -= 40.0f;

    return score;
}

float BGStrategyEngine::EvaluateStall() const
{
    float score = 10.0f; // Very situational

    // Good only when ahead and time running out
    if (IsWinning() && IsTimeRunningOut())
        score += 40.0f;

    // Bad in all other cases
    if (!IsWinning())
        score -= 30.0f;

    return score;
}

float BGStrategyEngine::EvaluateComeback() const
{
    float score = 15.0f;

    // Good when behind but with time remaining
    if (IsLosing() && !IsTimeRunningOut())
        score += 35.0f;

    // Great with positive momentum while behind
    if (IsLosing() && GetMomentum() > 0.2f)
        score += 25.0f;

    // Bad when winning
    if (IsWinning())
        score -= 40.0f;

    return score;
}

// ============================================================================
// FACTORS (PRIVATE)
// ============================================================================

float BGStrategyEngine::GetScoreFactor() const
{
    const BGScoreInfo& score = _coordinator->GetScoreInfo();

    if (score.maxScore == 0)
        return 0.0f;

    float friendlyRatio = static_cast<float>(score.friendlyScore) / score.maxScore;
    float enemyRatio = static_cast<float>(score.enemyScore) / score.maxScore;

    return friendlyRatio - enemyRatio;
}

float BGStrategyEngine::GetTimeFactor() const
{
    const BGMatchStats& stats = _coordinator->GetMatchStats();

    // If winning, more time = better
    // If losing, less time = worse
    float timeRemaining = static_cast<float>(stats.remainingTime) / 1800000.0f; // 30 min max

    // CRITICAL FIX: Use score-based heuristic instead of IsWinning() to break infinite recursion
    // IsWinning() → GetWinProbability() → GetTimeFactor() → IsWinning() (infinite loop!)
    // GetScoreFactor() returns positive if ahead, negative if behind (no recursion)
    bool currentlyAhead = (GetScoreFactor() > 0.0f);

    if (currentlyAhead)
        return timeRemaining * 0.5f;
    else
        return -timeRemaining * 0.5f;
}

float BGStrategyEngine::GetMomentumFactor() const
{
    return GetMomentum();
}

float BGStrategyEngine::GetStrengthFactor() const
{
    // Compare team sizes
    uint32 friendly = static_cast<uint32>(_coordinator->GetFriendlyPlayers().size());
    uint32 enemy = static_cast<uint32>(_coordinator->GetEnemyPlayers().size());

    if (enemy == 0)
        return 1.0f;

    return static_cast<float>(friendly) / static_cast<float>(enemy) - 1.0f;
}

float BGStrategyEngine::GetObjectiveControlFactor() const
{
    uint32 friendlyObjectives = 0;
    uint32 totalObjectives = 0;

    for (const auto& objective : _coordinator->GetObjectives())
    {
        if (objective.type == ObjectiveType::CONTROL_POINT ||
            objective.type == ObjectiveType::CAPTURABLE)
        {
            totalObjectives++;
            if (objective.state == BGObjectiveState::CONTROLLED_FRIENDLY)
                friendlyObjectives++;
        }
    }

    if (totalObjectives == 0)
        return 0.5f;

    return static_cast<float>(friendlyObjectives) / static_cast<float>(totalObjectives);
}

// ============================================================================
// MOMENTUM TRACKING (PRIVATE)
// ============================================================================

void BGStrategyEngine::RecordMomentumEvent(float value)
{
    MomentumEvent event;
    event.timestamp = 0; // Would use actual game time
    event.value = value;

    _momentumHistory.push_back(event);
}

void BGStrategyEngine::CleanOldMomentumEvents()
{
    // Remove events older than momentum window
    uint32 cutoffTime = _momentumWindow;

    auto it = std::remove_if(_momentumHistory.begin(), _momentumHistory.end(),
        [cutoffTime](const MomentumEvent& e) { return e.timestamp > cutoffTime; });

    _momentumHistory.erase(it, _momentumHistory.end());
}

float BGStrategyEngine::CalculateMomentum() const
{
    if (_momentumHistory.empty())
        return 0.0f;

    float total = 0.0f;
    for (const auto& event : _momentumHistory)
    {
        // Weight more recent events higher
        float weight = 1.0f; // Simplified - would decay over time
        total += event.value * weight;
    }

    return std::clamp(total / 10.0f, -1.0f, 1.0f); // Normalize
}

// ============================================================================
// UTILITY (PRIVATE)
// ============================================================================

void BGStrategyEngine::ApplyStrategy(BGStrategy strategy)
{
    _currentStrategy = strategy;
    _currentDecision = BuildDecision(strategy);

    TC_LOG_DEBUG("playerbots.bg", "BGStrategyEngine::ApplyStrategy - Applied strategy %u: %s",
        static_cast<uint8>(strategy), _currentDecision.reasoning.c_str());
}

StrategicDecision BGStrategyEngine::BuildDecision(BGStrategy strategy) const
{
    StrategicDecision decision;
    decision.strategy = strategy;

    // Set allocations based on strategy
    switch (strategy)
    {
        case BGStrategy::BALANCED:
            decision.offenseAllocation = 50;
            decision.defenseAllocation = 50;
            decision.reasoning = "Balanced offense and defense";
            break;

        case BGStrategy::AGGRESSIVE:
            decision.offenseAllocation = 65;
            decision.defenseAllocation = 35;
            decision.reasoning = "Aggressive push, light defense";
            break;

        case BGStrategy::DEFENSIVE:
            decision.offenseAllocation = 30;
            decision.defenseAllocation = 70;
            decision.reasoning = "Defensive stance, protect objectives";
            break;

        case BGStrategy::TURTLE:
            decision.offenseAllocation = 15;
            decision.defenseAllocation = 85;
            decision.reasoning = "Maximum defense, minimal offense";
            break;

        case BGStrategy::ALL_IN:
            decision.offenseAllocation = 85;
            decision.defenseAllocation = 15;
            decision.reasoning = "Full offense, minimal defense";
            break;

        case BGStrategy::STALL:
            decision.offenseAllocation = 20;
            decision.defenseAllocation = 60;
            decision.reasoning = "Stalling tactics, run out clock";
            break;

        case BGStrategy::COMEBACK:
            decision.offenseAllocation = 70;
            decision.defenseAllocation = 30;
            decision.reasoning = "Comeback push, calculated aggression";
            break;
    }

    // Determine objectives
    decision.attackObjectives = DetermineAttackTargets(strategy);
    decision.defendObjectives = DetermineDefenseTargets(strategy);
    decision.confidence = 0.7f;

    return decision;
}

std::vector<uint32> BGStrategyEngine::DetermineAttackTargets(BGStrategy strategy) const
{
    std::vector<uint32> targets;

    // Get all enemy and neutral objectives
    for (const auto& objective : _coordinator->GetObjectives())
    {
        if (objective.type != ObjectiveType::CONTROL_POINT &&
            objective.type != ObjectiveType::CAPTURABLE)
            continue;

        if (objective.state == BGObjectiveState::CONTROLLED_ENEMY ||
            objective.state == BGObjectiveState::NEUTRAL ||
            objective.state == BGObjectiveState::CONTESTED)
        {
            targets.push_back(objective.objectiveId);
        }
    }

    // Sort by priority
    std::sort(targets.begin(), targets.end(),
        [this](uint32 a, uint32 b)
        {
            return GetObjectivePriority(a) > GetObjectivePriority(b);
        });

    // Limit based on strategy
    uint32 maxTargets = 1;
    switch (strategy)
    {
        case BGStrategy::BALANCED:
        case BGStrategy::DEFENSIVE:
            maxTargets = 1;
            break;
        case BGStrategy::AGGRESSIVE:
        case BGStrategy::COMEBACK:
            maxTargets = 2;
            break;
        case BGStrategy::ALL_IN:
            maxTargets = 3;
            break;
        case BGStrategy::TURTLE:
        case BGStrategy::STALL:
            maxTargets = 0;
            break;
    }

    if (targets.size() > maxTargets)
        targets.resize(maxTargets);

    return targets;
}

std::vector<uint32> BGStrategyEngine::DetermineDefenseTargets(BGStrategy strategy) const
{
    std::vector<uint32> targets;

    // Get all friendly objectives
    for (const auto& objective : _coordinator->GetObjectives())
    {
        if (objective.type != ObjectiveType::CONTROL_POINT &&
            objective.type != ObjectiveType::CAPTURABLE)
            continue;

        if (objective.state == BGObjectiveState::CONTROLLED_FRIENDLY)
        {
            targets.push_back(objective.objectiveId);
        }
    }

    // Sort by priority (highest first)
    std::sort(targets.begin(), targets.end(),
        [this](uint32 a, uint32 b)
        {
            return GetObjectivePriority(a) > GetObjectivePriority(b);
        });

    return targets;
}

} // namespace Playerbot
