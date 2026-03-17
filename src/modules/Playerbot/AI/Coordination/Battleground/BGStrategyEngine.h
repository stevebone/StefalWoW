/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "BGState.h"
#include <vector>
#include <map>
#include <string>

namespace Playerbot {

class BattlegroundCoordinator;

/**
 * @enum BGStrategy
 * @brief High-level battleground strategies
 */
enum class BGStrategy : uint8
{
    BALANCED = 0,       // Standard play
    AGGRESSIVE = 1,     // Focus offense
    DEFENSIVE = 2,      // Focus defense
    TURTLE = 3,         // Maximum defense
    ALL_IN = 4,         // Full offense
    STALL = 5,          // Run out clock
    COMEBACK = 6        // Desperate measures when behind
};

/**
 * @struct StrategicDecision
 * @brief A strategic decision with reasoning
 */
struct StrategicDecision
{
    BGStrategy strategy;
    ::std::vector<uint32> attackObjectives;
    ::std::vector<uint32> defendObjectives;
    uint8 offenseAllocation;   // Percent of bots on offense
    uint8 defenseAllocation;   // Percent of bots on defense
    ::std::string reasoning;
    float confidence;

    StrategicDecision()
        : strategy(BGStrategy::BALANCED),
          offenseAllocation(50), defenseAllocation(50),
          confidence(0.5f) {}
};

/**
 * @struct StrategyScore
 * @brief Evaluation score for a strategy
 */
struct StrategyScore
{
    BGStrategy strategy;
    float totalScore;
    float winChanceScore;
    float resourceScore;
    float momentumScore;
    float riskScore;

    StrategyScore()
        : strategy(BGStrategy::BALANCED), totalScore(0),
          winChanceScore(0), resourceScore(0), momentumScore(0), riskScore(0) {}
};

/**
 * @class BGStrategyEngine
 * @brief Strategic decision making for battlegrounds
 *
 * Evaluates game state and recommends high-level strategy including:
 * - Offense vs defense allocation
 * - Objective prioritization
 * - Risk assessment
 * - Comeback mechanics
 */
class BGStrategyEngine
{
public:
    BGStrategyEngine(BattlegroundCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // STRATEGY
    // ========================================================================

    BGStrategy GetCurrentStrategy() const { return _currentStrategy; }
    StrategicDecision GetCurrentDecision() const { return _currentDecision; }
    void ForceStrategy(BGStrategy strategy);
    void ClearForcedStrategy();
    bool IsStrategyForced() const { return _strategyForced; }

    // ========================================================================
    // EVALUATION
    // ========================================================================

    ::std::vector<StrategyScore> EvaluateAllStrategies() const;
    StrategyScore EvaluateStrategy(BGStrategy strategy) const;
    StrategicDecision EvaluateBestStrategy() const;
    float ScoreStrategy(BGStrategy strategy) const;

    // ========================================================================
    // OBJECTIVE PRIORITY
    // ========================================================================

    ::std::vector<uint32> GetAttackPriorities() const;
    ::std::vector<uint32> GetDefendPriorities() const;
    uint8 GetObjectivePriority(uint32 objectiveId) const;
    void OverrideObjectivePriority(uint32 objectiveId, uint8 priority);
    void ClearPriorityOverrides();

    // ========================================================================
    // RESOURCE ALLOCATION
    // ========================================================================

    uint8 GetOffensePercent() const;
    uint8 GetDefensePercent() const;
    uint8 GetRoamerCount() const;
    void GetAllocation(uint8& offense, uint8& defense, uint8& roamer) const;

    // ========================================================================
    // TACTICAL QUERIES
    // ========================================================================

    bool ShouldContestNode(uint32 nodeId) const;
    bool ShouldAbandonNode(uint32 nodeId) const;
    bool ShouldRecall() const;
    bool ShouldPush() const;
    bool ShouldTurtle() const;
    bool ShouldAllIn() const;

    // ========================================================================
    // GAME STATE ANALYSIS
    // ========================================================================

    float GetWinProbability() const;
    bool IsWinning() const;
    bool IsLosing() const;
    bool IsCloseGame() const;
    bool IsTimeRunningOut() const;
    float GetMomentum() const;  // Positive = gaining, negative = losing

    // ========================================================================
    // BG-SPECIFIC STRATEGIES
    // ========================================================================

    StrategicDecision EvaluateWSGStrategy() const;
    StrategicDecision EvaluateABStrategy() const;
    StrategicDecision EvaluateAVStrategy() const;
    StrategicDecision EvaluateEOTSStrategy() const;

private:
    BattlegroundCoordinator* _coordinator;

    // ========================================================================
    // CURRENT STATE
    // ========================================================================

    BGStrategy _currentStrategy = BGStrategy::BALANCED;
    StrategicDecision _currentDecision;
    bool _strategyForced = false;
    uint32 _lastEvaluationTime = 0;
    uint32 _strategyChangedTime = 0;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    uint32 _evaluationInterval = 5000;  // 5 seconds
    uint32 _strategyChangeCooldown = 30000;  // 30 seconds
    float _strategyChangeThreshold = 0.2f;  // 20% better to switch

    // ========================================================================
    // PRIORITY OVERRIDES
    // ========================================================================

    ::std::map<uint32, uint8> _priorityOverrides;

    // ========================================================================
    // MOMENTUM TRACKING
    // ========================================================================

    struct MomentumEvent
    {
        uint32 timestamp;
        float value;  // Positive = good, negative = bad
    };
    ::std::vector<MomentumEvent> _momentumHistory;
    uint32 _momentumWindow = 60000;  // 1 minute

    // ========================================================================
    // STRATEGY EVALUATION
    // ========================================================================

    float EvaluateBalanced() const;
    float EvaluateAggressive() const;
    float EvaluateDefensive() const;
    float EvaluateTurtle() const;
    float EvaluateAllIn() const;
    float EvaluateStall() const;
    float EvaluateComeback() const;

    // ========================================================================
    // FACTORS
    // ========================================================================

    float GetScoreFactor() const;      // How score affects strategy
    float GetTimeFactor() const;       // How time remaining affects strategy
    float GetMomentumFactor() const;   // Recent captures/losses
    float GetStrengthFactor() const;   // Team strength comparison
    float GetObjectiveControlFactor() const;

    // ========================================================================
    // MOMENTUM TRACKING
    // ========================================================================

    void RecordMomentumEvent(float value);
    void CleanOldMomentumEvents();
    float CalculateMomentum() const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    void ApplyStrategy(BGStrategy strategy);
    StrategicDecision BuildDecision(BGStrategy strategy) const;
    ::std::vector<uint32> DetermineAttackTargets(BGStrategy strategy) const;
    ::std::vector<uint32> DetermineDefenseTargets(BGStrategy strategy) const;
};

} // namespace Playerbot
