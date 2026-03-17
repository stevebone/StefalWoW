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

namespace Playerbot {

class BattlegroundCoordinator;

/**
 * @struct ObjectivePriorityScore
 * @brief Scoring for objective prioritization
 */
struct ObjectivePriorityScore
{
    uint32 objectiveId;
    float totalScore;
    float strategicScore;
    float contestabilityScore;
    float proximityScore;
    float resourceScore;
    ::std::string reason;

    ObjectivePriorityScore()
        : objectiveId(0), totalScore(0),
          strategicScore(0), contestabilityScore(0),
          proximityScore(0), resourceScore(0) {}
};

/**
 * @class ObjectiveManager
 * @brief Manages battleground objectives
 *
 * Tracks all objectives in the battleground and provides
 * prioritization, assignment recommendations, and state monitoring.
 */
class ObjectiveManager
{
public:
    ObjectiveManager(BattlegroundCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // OBJECTIVE REGISTRATION
    // ========================================================================

    void RegisterObjective(const BGObjective& objective);
    void UnregisterObjective(uint32 objectiveId);
    void ClearObjectives();

    // ========================================================================
    // OBJECTIVE ACCESS
    // ========================================================================

    BGObjective* GetObjective(uint32 objectiveId);
    const BGObjective* GetObjective(uint32 objectiveId) const;
    ::std::vector<BGObjective*> GetAllObjectives();
    ::std::vector<const BGObjective*> GetAllObjectives() const;

    // ========================================================================
    // OBJECTIVE QUERIES
    // ========================================================================

    ::std::vector<BGObjective*> GetObjectivesByType(ObjectiveType type);
    ::std::vector<BGObjective*> GetObjectivesByState(BGObjectiveState state);
    ::std::vector<BGObjective*> GetContestedObjectives();
    ::std::vector<BGObjective*> GetFriendlyObjectives();
    ::std::vector<BGObjective*> GetEnemyObjectives();
    ::std::vector<BGObjective*> GetNeutralObjectives();

    // ========================================================================
    // OBJECTIVE NEAREST
    // ========================================================================

    BGObjective* GetNearestObjective(float x, float y, float z) const;
    BGObjective* GetNearestObjective(ObjectGuid player) const;
    BGObjective* GetNearestObjectiveOfType(ObjectGuid player, ObjectiveType type) const;
    BGObjective* GetNearestFriendlyObjective(ObjectGuid player) const;
    BGObjective* GetNearestEnemyObjective(ObjectGuid player) const;

    // ========================================================================
    // PRIORITIZATION
    // ========================================================================

    ::std::vector<ObjectivePriorityScore> PrioritizeObjectives() const;
    ObjectivePriorityScore ScoreObjective(const BGObjective& objective) const;
    BGObjective* GetHighestPriorityAttackTarget() const;
    BGObjective* GetHighestPriorityDefenseTarget() const;

    // ========================================================================
    // STATE TRACKING
    // ========================================================================

    void OnObjectiveStateChanged(uint32 objectiveId, BGObjectiveState newState);
    void OnObjectiveContested(uint32 objectiveId);
    void OnObjectiveCaptured(uint32 objectiveId, uint32 faction);
    void OnObjectiveLost(uint32 objectiveId);

    // ========================================================================
    // CAPTURE PREDICTION
    // ========================================================================

    uint32 GetEstimatedCaptureTime(uint32 objectiveId) const;
    bool WillBeCaptured(uint32 objectiveId) const;
    float GetCaptureProgress(uint32 objectiveId) const;

    // ========================================================================
    // ASSIGNMENT TRACKING
    // ========================================================================

    void AssignToObjective(ObjectGuid player, uint32 objectiveId, bool isDefender);
    void UnassignFromObjective(ObjectGuid player, uint32 objectiveId);
    ::std::vector<ObjectGuid> GetAssignedDefenders(uint32 objectiveId) const;
    ::std::vector<ObjectGuid> GetAssignedAttackers(uint32 objectiveId) const;
    uint32 GetDefenderCount(uint32 objectiveId) const;
    uint32 GetAttackerCount(uint32 objectiveId) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    uint32 GetControlledCount() const;
    uint32 GetEnemyControlledCount() const;
    uint32 GetContestedCount() const;
    uint32 GetNeutralCount() const;
    float GetControlRatio() const;

private:
    BattlegroundCoordinator* _coordinator;

    // ========================================================================
    // OBJECTIVES
    // ========================================================================

    ::std::map<uint32, BGObjective> _objectives;

    // ========================================================================
    // SCORING WEIGHTS
    // ========================================================================

    float _weightStrategic = 2.0f;
    float _weightContestability = 1.5f;
    float _weightProximity = 1.0f;
    float _weightResource = 1.5f;

    // ========================================================================
    // SCORING HELPERS
    // ========================================================================

    float ScoreStrategicValue(const BGObjective& objective) const;
    float ScoreContestability(const BGObjective& objective) const;
    float ScoreProximity(const BGObjective& objective) const;
    float ScoreResourceValue(const BGObjective& objective) const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    void UpdateNearbyPlayerCounts();
    float GetDistance(float x1, float y1, float z1, float x2, float y2, float z2) const;
    bool IsFriendlyState(BGObjectiveState state) const;
    bool IsEnemyState(BGObjectiveState state) const;
};

} // namespace Playerbot
