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

#include "ArenaState.h"
#include <vector>
#include <map>

namespace Playerbot {

class ArenaCoordinator;

/**
 * @struct TargetScore
 * @brief Represents the evaluation score for a potential kill target
 */
struct TargetScore
{
    ObjectGuid target;
    float totalScore;

    // Component scores for debugging/tuning
    float healthScore;
    float cooldownScore;
    float roleScore;
    float positionScore;
    float momentumScore;
    float ccStatusScore;
    float bonusScore;

    ::std::string reason;

    TargetScore()
        : target(), totalScore(0),
          healthScore(0), cooldownScore(0), roleScore(0),
          positionScore(0), momentumScore(0), ccStatusScore(0), bonusScore(0) {}

    bool operator>(const TargetScore& other) const
    {
        return totalScore > other.totalScore;
    }
};

/**
 * @class KillTargetManager
 * @brief Manages kill target selection and switching in arena
 *
 * Evaluates all potential kill targets using a weighted scoring system
 * that considers:
 * - Current health percentage
 * - Cooldown availability (trinket, defensives)
 * - Role (healer priority)
 * - Position (in LOS, distance)
 * - Current CC status
 * - Momentum (damage dealt recently)
 */
class KillTargetManager
{
public:
    KillTargetManager(ArenaCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // KILL TARGET
    // ========================================================================

    ObjectGuid GetKillTarget() const { return _killTarget; }
    void SetKillTarget(ObjectGuid target);
    void ClearKillTarget();
    bool HasKillTarget() const { return !_killTarget.IsEmpty(); }

    // ========================================================================
    // TARGET EVALUATION
    // ========================================================================

    ::std::vector<TargetScore> EvaluateAllTargets() const;
    TargetScore EvaluateTarget(const ArenaEnemy& enemy) const;
    float CalculateTargetScore(const ArenaEnemy& enemy) const;
    ObjectGuid GetRecommendedTarget() const;

    // ========================================================================
    // SWITCH LOGIC
    // ========================================================================

    bool ShouldSwitch() const;
    ObjectGuid GetSwitchTarget() const;
    void OnSwitchCalled(ObjectGuid newTarget);
    float GetSwitchThreshold() const { return _switchThreshold; }
    void SetSwitchThreshold(float threshold) { _switchThreshold = threshold; }

    // ========================================================================
    // PRIORITY MODIFIERS
    // ========================================================================

    void SetPriorityBonus(ObjectGuid target, float bonus, const ::std::string& reason);
    void ClearPriorityBonus(ObjectGuid target);
    void ClearAllPriorityBonuses();
    float GetPriorityBonus(ObjectGuid target) const;

    // ========================================================================
    // TARGET HISTORY
    // ========================================================================

    uint32 GetTimeOnTarget() const;
    float GetDamageDealtToTarget() const;
    uint32 GetSwitchCount() const { return _switchCount; }
    ObjectGuid GetPreviousTarget() const { return _previousTarget; }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    void SetWeights(float health, float cooldown, float role, float position, float momentum, float ccStatus);

private:
    ArenaCoordinator* _coordinator;

    // ========================================================================
    // CURRENT TARGET
    // ========================================================================

    ObjectGuid _killTarget;
    ObjectGuid _previousTarget;
    uint32 _targetSetTime = 0;
    uint32 _lastEvaluationTime = 0;
    uint32 _switchCount = 0;

    // ========================================================================
    // PRIORITY BONUSES
    // ========================================================================

    struct PriorityBonus
    {
        float bonus;
        ::std::string reason;
        uint32 setTime;
    };
    ::std::map<ObjectGuid, PriorityBonus> _priorityBonuses;

    // ========================================================================
    // DAMAGE TRACKING
    // ========================================================================

    ::std::map<ObjectGuid, float> _recentDamageDealt;  // target -> damage in last 5s
    uint32 _damageTrackingWindow = 5000;  // 5 seconds

    // ========================================================================
    // SCORING WEIGHTS
    // ========================================================================

    float _weightHealth = 2.0f;
    float _weightCooldowns = 1.5f;
    float _weightRole = 1.2f;
    float _weightPosition = 1.0f;
    float _weightMomentum = 1.0f;
    float _weightCCStatus = -2.0f;  // Negative = don't target CC'd

    // ========================================================================
    // SWITCH CONFIGURATION
    // ========================================================================

    float _switchThreshold = 1.5f;  // New target must score 50% better
    uint32 _minTimeOnTarget = 3000; // Minimum 3s before switching
    uint32 _evaluationInterval = 500; // Re-evaluate every 500ms

    // ========================================================================
    // SCORING FUNCTIONS
    // ========================================================================

    float ScoreHealth(float healthPercent) const;
    float ScoreCooldowns(const ArenaEnemy& enemy) const;
    float ScoreRole(ArenaRole role) const;
    float ScorePosition(const ArenaEnemy& enemy) const;
    float ScoreMomentum(ObjectGuid target) const;
    float ScoreCCStatus(const ArenaEnemy& enemy) const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    void UpdateDamageTracking(uint32 diff);
    void RecordDamage(ObjectGuid target, float amount);
    bool IsValidTarget(const ArenaEnemy& enemy) const;
};

} // namespace Playerbot
