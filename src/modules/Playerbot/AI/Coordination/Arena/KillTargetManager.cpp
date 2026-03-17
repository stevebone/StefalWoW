/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "KillTargetManager.h"
#include "ArenaCoordinator.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>
#include <cmath>

namespace Playerbot {

KillTargetManager::KillTargetManager(ArenaCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void KillTargetManager::Initialize()
{
    Reset();
    TC_LOG_DEBUG("playerbot", "KillTargetManager::Initialize - Initialized");
}

void KillTargetManager::Update(uint32 diff)
{
    UpdateDamageTracking(diff);

    uint32 now = GameTime::GetGameTimeMS();

    // Periodic evaluation
    if (now - _lastEvaluationTime >= _evaluationInterval)
    {
        _lastEvaluationTime = now;

        // Check if we should switch targets
        if (ShouldSwitch())
        {
            ObjectGuid newTarget = GetSwitchTarget();
            if (!newTarget.IsEmpty() && newTarget != _killTarget)
            {
                TC_LOG_DEBUG("playerbot", "KillTargetManager: Auto-switching to better target");
                SetKillTarget(newTarget);
            }
        }
    }
}

void KillTargetManager::Reset()
{
    _killTarget = ObjectGuid::Empty;
    _previousTarget = ObjectGuid::Empty;
    _targetSetTime = 0;
    _lastEvaluationTime = 0;
    _switchCount = 0;
    _priorityBonuses.clear();
    _recentDamageDealt.clear();
}

// ============================================================================
// KILL TARGET
// ============================================================================

void KillTargetManager::SetKillTarget(ObjectGuid target)
{
    if (target == _killTarget)
        return;

    _previousTarget = _killTarget;
    _killTarget = target;
    _targetSetTime = GameTime::GetGameTimeMS();

    TC_LOG_DEBUG("playerbot", "KillTargetManager::SetKillTarget - New kill target set");

    // Broadcast focus target via BotMessageBus
    auto const& teammates = _coordinator->GetTeammates();
    if (!teammates.empty())
    {
        Player* leader = ObjectAccessor::FindPlayer(teammates.front().guid);
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            ObjectGuid senderGuid = leader->GetGroup()->GetLeaderGUID();
            sBotMessageBus->Publish(BotMessage::CommandFocusTarget(senderGuid, groupGuid, target));
        }
    }
}

void KillTargetManager::ClearKillTarget()
{
    _previousTarget = _killTarget;
    _killTarget = ObjectGuid::Empty;
    _targetSetTime = 0;
}

// ============================================================================
// TARGET EVALUATION
// ============================================================================

::std::vector<TargetScore> KillTargetManager::EvaluateAllTargets() const
{
    ::std::vector<TargetScore> scores;

    for (const ArenaEnemy& enemy : _coordinator->GetAliveEnemies())
    {
        if (!IsValidTarget(enemy))
            continue;

        scores.push_back(EvaluateTarget(enemy));
    }

    // Sort by score (highest first)
    ::std::sort(scores.begin(), scores.end(),
        [](const TargetScore& a, const TargetScore& b) { return a > b; });

    return scores;
}

TargetScore KillTargetManager::EvaluateTarget(const ArenaEnemy& enemy) const
{
    TargetScore score;
    score.target = enemy.guid;

    // Calculate component scores
    score.healthScore = ScoreHealth(enemy.healthPercent) * _weightHealth;
    score.cooldownScore = ScoreCooldowns(enemy) * _weightCooldowns;
    score.roleScore = ScoreRole(enemy.role) * _weightRole;
    score.positionScore = ScorePosition(enemy) * _weightPosition;
    score.momentumScore = ScoreMomentum(enemy.guid) * _weightMomentum;
    score.ccStatusScore = ScoreCCStatus(enemy) * _weightCCStatus;
    score.bonusScore = GetPriorityBonus(enemy.guid);

    // Calculate total
    score.totalScore = score.healthScore + score.cooldownScore + score.roleScore +
                       score.positionScore + score.momentumScore + score.ccStatusScore +
                       score.bonusScore;

    // Add stick-to-target bonus if this is current target
    if (enemy.guid == _killTarget)
    {
        score.totalScore += 0.5f;  // Small bonus for current target
        score.reason = "Current target";
    }

    return score;
}

float KillTargetManager::CalculateTargetScore(const ArenaEnemy& enemy) const
{
    return EvaluateTarget(enemy).totalScore;
}

ObjectGuid KillTargetManager::GetRecommendedTarget() const
{
    auto scores = EvaluateAllTargets();
    return scores.empty() ? ObjectGuid::Empty : scores[0].target;
}

// ============================================================================
// SWITCH LOGIC
// ============================================================================

bool KillTargetManager::ShouldSwitch() const
{
    if (_killTarget.IsEmpty())
        return true;

    // Don't switch too quickly
    uint32 timeOnTarget = GetTimeOnTarget();
    if (timeOnTarget < _minTimeOnTarget)
        return false;

    // Check if current target is still valid
    const ArenaEnemy* currentEnemy = _coordinator->GetEnemy(_killTarget);
    if (!currentEnemy || !IsValidTarget(*currentEnemy))
        return true;

    // Check if there's a significantly better target
    ObjectGuid recommendedTarget = GetRecommendedTarget();
    if (recommendedTarget.IsEmpty() || recommendedTarget == _killTarget)
        return false;

    // Compare scores
    float currentScore = CalculateTargetScore(*currentEnemy);

    const ArenaEnemy* recommended = _coordinator->GetEnemy(recommendedTarget);
    if (!recommended)
        return false;

    float recommendedScore = CalculateTargetScore(*recommended);

    // Switch if new target is significantly better
    return recommendedScore > currentScore * _switchThreshold;
}

ObjectGuid KillTargetManager::GetSwitchTarget() const
{
    return GetRecommendedTarget();
}

void KillTargetManager::OnSwitchCalled(ObjectGuid newTarget)
{
    SetKillTarget(newTarget);
    _switchCount++;

    TC_LOG_DEBUG("playerbot", "KillTargetManager::OnSwitchCalled - Switch #%u", _switchCount);
}

// ============================================================================
// PRIORITY MODIFIERS
// ============================================================================

void KillTargetManager::SetPriorityBonus(ObjectGuid target, float bonus, const ::std::string& reason)
{
    PriorityBonus pb;
    pb.bonus = bonus;
    pb.reason = reason;
    pb.setTime = GameTime::GetGameTimeMS();
    _priorityBonuses[target] = pb;
}

void KillTargetManager::ClearPriorityBonus(ObjectGuid target)
{
    _priorityBonuses.erase(target);
}

void KillTargetManager::ClearAllPriorityBonuses()
{
    _priorityBonuses.clear();
}

float KillTargetManager::GetPriorityBonus(ObjectGuid target) const
{
    auto it = _priorityBonuses.find(target);
    return it != _priorityBonuses.end() ? it->second.bonus : 0.0f;
}

// ============================================================================
// TARGET HISTORY
// ============================================================================

uint32 KillTargetManager::GetTimeOnTarget() const
{
    if (_targetSetTime == 0)
        return 0;

    return GameTime::GetGameTimeMS() - _targetSetTime;
}

float KillTargetManager::GetDamageDealtToTarget() const
{
    auto it = _recentDamageDealt.find(_killTarget);
    return it != _recentDamageDealt.end() ? it->second : 0.0f;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void KillTargetManager::SetWeights(float health, float cooldown, float role,
                                   float position, float momentum, float ccStatus)
{
    _weightHealth = health;
    _weightCooldowns = cooldown;
    _weightRole = role;
    _weightPosition = position;
    _weightMomentum = momentum;
    _weightCCStatus = ccStatus;
}

// ============================================================================
// SCORING FUNCTIONS
// ============================================================================

float KillTargetManager::ScoreHealth(float healthPercent) const
{
    // Lower health = higher score
    // Scale: 100% health = 0 score, 0% health = 1.0 score
    // Non-linear: favor targets that are already low
    float healthRatio = 1.0f - (healthPercent / 100.0f);

    // Exponential scaling for low health targets
    if (healthPercent < 30.0f)
    {
        return healthRatio * 1.5f;  // 50% bonus for execute range
    }
    else if (healthPercent < 50.0f)
    {
        return healthRatio * 1.2f;  // 20% bonus for low health
    }

    return healthRatio;
}

float KillTargetManager::ScoreCooldowns(const ArenaEnemy& enemy) const
{
    float score = 0.0f;

    // Trinket down = major bonus
    if (!enemy.trinketAvailable)
    {
        score += 0.5f;
    }

    // Defensive cooldowns down = bonus
    if (!enemy.isInDefensiveCooldown)
    {
        score += 0.3f;
    }

    // Major cooldowns on cooldown = bonus
    // (They can't stop us if they have no tools)
    uint32 cdsOnCooldown = enemy.majorCooldowns.size();
    score += cdsOnCooldown * 0.1f;

    return ::std::min(score, 1.0f);
}

float KillTargetManager::ScoreRole(ArenaRole role) const
{
    switch (role)
    {
        case ArenaRole::HEALER:
            return 1.0f;  // Healers are high priority
        case ArenaRole::RANGED_DPS:
            return 0.6f;  // Ranged DPS are medium priority
        case ArenaRole::MELEE_DPS:
            return 0.4f;  // Melee DPS are lower priority (harder to kite)
        case ArenaRole::HYBRID:
            return 0.5f;
        default:
            return 0.3f;
    }
}

float KillTargetManager::ScorePosition(const ArenaEnemy& enemy) const
{
    float score = 0.0f;

    // In LOS = bonus
    if (!enemy.isLOSBlocked)
    {
        score += 0.3f;
    }

    // Within range = bonus
    // This would need actual distance calculation
    // For now, use last seen time as proxy for visibility
    uint32 now = GameTime::GetGameTimeMS();
    if (now - enemy.lastSeenTime < 1000)
    {
        score += 0.2f;  // Recently seen
    }

    return score;
}

float KillTargetManager::ScoreMomentum(ObjectGuid target) const
{
    // Score based on recent damage dealt to this target
    auto it = _recentDamageDealt.find(target);
    if (it == _recentDamageDealt.end())
        return 0.0f;

    float damage = it->second;

    // Normalize based on expected damage in tracking window
    // Assume 10k DPS = good momentum
    float expectedDamage = 10000.0f * (_damageTrackingWindow / 1000.0f);

    return ::std::min(damage / expectedDamage, 1.0f);
}

float KillTargetManager::ScoreCCStatus(const ArenaEnemy& enemy) const
{
    // In CC = big negative score (don't attack CC'd targets)
    if (enemy.isInCC)
    {
        return -1.0f;
    }

    return 0.0f;
}

// ============================================================================
// UTILITY
// ============================================================================

void KillTargetManager::UpdateDamageTracking(uint32 diff)
{
    // Decay damage tracking over time
    float decayRate = static_cast<float>(diff) / _damageTrackingWindow;

    for (auto it = _recentDamageDealt.begin(); it != _recentDamageDealt.end(); )
    {
        it->second *= (1.0f - decayRate);

        // Remove entries with negligible damage
        if (it->second < 100.0f)
        {
            it = _recentDamageDealt.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void KillTargetManager::RecordDamage(ObjectGuid target, float amount)
{
    _recentDamageDealt[target] += amount;
}

bool KillTargetManager::IsValidTarget(const ArenaEnemy& enemy) const
{
    // Must be alive
    if (enemy.healthPercent <= 0)
        return false;

    // Must be visible (recently seen)
    uint32 now = GameTime::GetGameTimeMS();
    if (now - enemy.lastSeenTime > 5000)
        return false;  // Haven't seen in 5 seconds

    return true;
}

} // namespace Playerbot
