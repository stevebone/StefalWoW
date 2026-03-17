/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ObjectiveManager.h"
#include "BattlegroundCoordinator.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <algorithm>
#include <cmath>

namespace Playerbot {

ObjectiveManager::ObjectiveManager(BattlegroundCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void ObjectiveManager::Initialize()
{
    Reset();
    TC_LOG_DEBUG("playerbot", "ObjectiveManager::Initialize - Initialized");
}

void ObjectiveManager::Update(uint32 /*diff*/)
{
    UpdateNearbyPlayerCounts();
}

void ObjectiveManager::Reset()
{
    _objectives.clear();
}

// ============================================================================
// OBJECTIVE REGISTRATION
// ============================================================================

void ObjectiveManager::RegisterObjective(const BGObjective& objective)
{
    _objectives[objective.id] = objective;
    TC_LOG_DEBUG("playerbot", "ObjectiveManager: Registered objective %u (%s)",
                 objective.id, objective.name.c_str());
}

void ObjectiveManager::UnregisterObjective(uint32 objectiveId)
{
    _objectives.erase(objectiveId);
}

void ObjectiveManager::ClearObjectives()
{
    _objectives.clear();
}

// ============================================================================
// OBJECTIVE ACCESS
// ============================================================================

BGObjective* ObjectiveManager::GetObjective(uint32 objectiveId)
{
    auto it = _objectives.find(objectiveId);
    return it != _objectives.end() ? &it->second : nullptr;
}

const BGObjective* ObjectiveManager::GetObjective(uint32 objectiveId) const
{
    auto it = _objectives.find(objectiveId);
    return it != _objectives.end() ? &it->second : nullptr;
}

::std::vector<BGObjective*> ObjectiveManager::GetAllObjectives()
{
    ::std::vector<BGObjective*> result;
    for (auto& [id, obj] : _objectives)
    {
        result.push_back(&obj);
    }
    return result;
}

::std::vector<const BGObjective*> ObjectiveManager::GetAllObjectives() const
{
    ::std::vector<const BGObjective*> result;
    for (const auto& [id, obj] : _objectives)
    {
        result.push_back(&obj);
    }
    return result;
}

// ============================================================================
// OBJECTIVE QUERIES
// ============================================================================

::std::vector<BGObjective*> ObjectiveManager::GetObjectivesByType(ObjectiveType type)
{
    ::std::vector<BGObjective*> result;
    for (auto& [id, obj] : _objectives)
    {
        if (obj.type == type)
            result.push_back(&obj);
    }
    return result;
}

::std::vector<BGObjective*> ObjectiveManager::GetObjectivesByState(BGObjectiveState state)
{
    ::std::vector<BGObjective*> result;
    for (auto& [id, obj] : _objectives)
    {
        if (obj.state == state)
            result.push_back(&obj);
    }
    return result;
}

::std::vector<BGObjective*> ObjectiveManager::GetContestedObjectives()
{
    ::std::vector<BGObjective*> result;
    for (auto& [id, obj] : _objectives)
    {
        if (obj.isContested)
            result.push_back(&obj);
    }
    return result;
}

::std::vector<BGObjective*> ObjectiveManager::GetFriendlyObjectives()
{
    ::std::vector<BGObjective*> result;
    for (auto& [id, obj] : _objectives)
    {
        if (IsFriendlyState(obj.state))
            result.push_back(&obj);
    }
    return result;
}

::std::vector<BGObjective*> ObjectiveManager::GetEnemyObjectives()
{
    ::std::vector<BGObjective*> result;
    for (auto& [id, obj] : _objectives)
    {
        if (IsEnemyState(obj.state))
            result.push_back(&obj);
    }
    return result;
}

::std::vector<BGObjective*> ObjectiveManager::GetNeutralObjectives()
{
    return GetObjectivesByState(BGObjectiveState::NEUTRAL);
}

// ============================================================================
// OBJECTIVE NEAREST
// ============================================================================

BGObjective* ObjectiveManager::GetNearestObjective(float x, float y, float z) const
{
    BGObjective* nearest = nullptr;
    float minDist = std::numeric_limits<float>::max();

    for (auto& [id, obj] : _objectives)
    {
        float dist = GetDistance(x, y, z, obj.x, obj.y, obj.z);
        if (dist < minDist)
        {
            minDist = dist;
            nearest = const_cast<BGObjective*>(&obj);
        }
    }

    return nearest;
}

BGObjective* ObjectiveManager::GetNearestObjective(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return nullptr;

    return GetNearestObjective(p->GetPositionX(), p->GetPositionY(), p->GetPositionZ());
}

BGObjective* ObjectiveManager::GetNearestObjectiveOfType(ObjectGuid player, ObjectiveType type) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return nullptr;

    float x = p->GetPositionX();
    float y = p->GetPositionY();
    float z = p->GetPositionZ();

    BGObjective* nearest = nullptr;
    float minDist = std::numeric_limits<float>::max();

    for (auto& [id, obj] : _objectives)
    {
        if (obj.type != type)
            continue;

        float dist = GetDistance(x, y, z, obj.x, obj.y, obj.z);
        if (dist < minDist)
        {
            minDist = dist;
            nearest = const_cast<BGObjective*>(&obj);
        }
    }

    return nearest;
}

BGObjective* ObjectiveManager::GetNearestFriendlyObjective(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return nullptr;

    float x = p->GetPositionX();
    float y = p->GetPositionY();
    float z = p->GetPositionZ();

    BGObjective* nearest = nullptr;
    float minDist = std::numeric_limits<float>::max();

    for (auto& [id, obj] : _objectives)
    {
        if (!IsFriendlyState(obj.state))
            continue;

        float dist = GetDistance(x, y, z, obj.x, obj.y, obj.z);
        if (dist < minDist)
        {
            minDist = dist;
            nearest = const_cast<BGObjective*>(&obj);
        }
    }

    return nearest;
}

BGObjective* ObjectiveManager::GetNearestEnemyObjective(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return nullptr;

    float x = p->GetPositionX();
    float y = p->GetPositionY();
    float z = p->GetPositionZ();

    BGObjective* nearest = nullptr;
    float minDist = std::numeric_limits<float>::max();

    for (auto& [id, obj] : _objectives)
    {
        if (!IsEnemyState(obj.state))
            continue;

        float dist = GetDistance(x, y, z, obj.x, obj.y, obj.z);
        if (dist < minDist)
        {
            minDist = dist;
            nearest = const_cast<BGObjective*>(&obj);
        }
    }

    return nearest;
}

// ============================================================================
// PRIORITIZATION
// ============================================================================

::std::vector<ObjectivePriorityScore> ObjectiveManager::PrioritizeObjectives() const
{
    ::std::vector<ObjectivePriorityScore> scores;

    for (const auto& [id, obj] : _objectives)
    {
        scores.push_back(ScoreObjective(obj));
    }

    // Sort by score (highest first)
    ::std::sort(scores.begin(), scores.end(),
        [](const ObjectivePriorityScore& a, const ObjectivePriorityScore& b)
        {
            return a.totalScore > b.totalScore;
        });

    return scores;
}

ObjectivePriorityScore ObjectiveManager::ScoreObjective(const BGObjective& objective) const
{
    ObjectivePriorityScore score;
    score.objectiveId = objective.id;

    score.strategicScore = ScoreStrategicValue(objective) * _weightStrategic;
    score.contestabilityScore = ScoreContestability(objective) * _weightContestability;
    score.proximityScore = ScoreProximity(objective) * _weightProximity;
    score.resourceScore = ScoreResourceValue(objective) * _weightResource;

    score.totalScore = score.strategicScore + score.contestabilityScore +
                       score.proximityScore + score.resourceScore;

    return score;
}

BGObjective* ObjectiveManager::GetHighestPriorityAttackTarget() const
{
    auto scores = PrioritizeObjectives();

    for (const auto& score : scores)
    {
        const BGObjective* obj = GetObjective(score.objectiveId);
        if (obj && IsEnemyState(obj->state))
        {
            return const_cast<BGObjective*>(obj);
        }
    }

    // Fallback to neutral
    for (const auto& score : scores)
    {
        const BGObjective* obj = GetObjective(score.objectiveId);
        if (obj && obj->state == BGObjectiveState::NEUTRAL)
        {
            return const_cast<BGObjective*>(obj);
        }
    }

    return nullptr;
}

BGObjective* ObjectiveManager::GetHighestPriorityDefenseTarget() const
{
    auto scores = PrioritizeObjectives();

    for (const auto& score : scores)
    {
        const BGObjective* obj = GetObjective(score.objectiveId);
        if (obj && IsFriendlyState(obj->state))
        {
            return const_cast<BGObjective*>(obj);
        }
    }

    return nullptr;
}

// ============================================================================
// STATE TRACKING
// ============================================================================

void ObjectiveManager::OnObjectiveStateChanged(uint32 objectiveId, BGObjectiveState newState)
{
    BGObjective* obj = GetObjective(objectiveId);
    if (obj)
    {
        obj->state = newState;

        TC_LOG_DEBUG("playerbot", "ObjectiveManager: Objective %u state changed to %s",
                     objectiveId, BGObjectiveStateToString(newState));
    }
}

void ObjectiveManager::OnObjectiveContested(uint32 objectiveId)
{
    BGObjective* obj = GetObjective(objectiveId);
    if (obj)
    {
        obj->isContested = true;
        obj->contestedSince = 0;  // Would be current time
    }
}

void ObjectiveManager::OnObjectiveCaptured(uint32 objectiveId, uint32 faction)
{
    BGObjective* obj = GetObjective(objectiveId);
    if (obj)
    {
        obj->isContested = false;

        if (faction == ALLIANCE)
            obj->state = BGObjectiveState::ALLIANCE_CONTROLLED;
        else
            obj->state = BGObjectiveState::HORDE_CONTROLLED;

        TC_LOG_DEBUG("playerbot", "ObjectiveManager: Objective %u captured by faction %u",
                     objectiveId, faction);
    }
}

void ObjectiveManager::OnObjectiveLost(uint32 objectiveId)
{
    BGObjective* obj = GetObjective(objectiveId);
    if (obj)
    {
        // State will be updated by OnObjectiveCaptured
        TC_LOG_DEBUG("playerbot", "ObjectiveManager: Objective %u lost", objectiveId);
    }
}

// ============================================================================
// CAPTURE PREDICTION
// ============================================================================

uint32 ObjectiveManager::GetEstimatedCaptureTime(uint32 objectiveId) const
{
    const BGObjective* obj = GetObjective(objectiveId);
    if (!obj)
        return 0;

    return obj->captureTime;
}

bool ObjectiveManager::WillBeCaptured(uint32 objectiveId) const
{
    const BGObjective* obj = GetObjective(objectiveId);
    if (!obj)
        return false;

    return obj->captureProgress > 0.5f;
}

float ObjectiveManager::GetCaptureProgress(uint32 objectiveId) const
{
    const BGObjective* obj = GetObjective(objectiveId);
    return obj ? obj->captureProgress : 0.0f;
}

// ============================================================================
// ASSIGNMENT TRACKING
// ============================================================================

void ObjectiveManager::AssignToObjective(ObjectGuid player, uint32 objectiveId, bool isDefender)
{
    BGObjective* obj = GetObjective(objectiveId);
    if (!obj)
        return;

    if (isDefender)
        obj->assignedDefenders.push_back(player);
    else
        obj->assignedAttackers.push_back(player);
}

void ObjectiveManager::UnassignFromObjective(ObjectGuid player, uint32 objectiveId)
{
    BGObjective* obj = GetObjective(objectiveId);
    if (!obj)
        return;

    auto& defenders = obj->assignedDefenders;
    defenders.erase(::std::remove(defenders.begin(), defenders.end(), player), defenders.end());

    auto& attackers = obj->assignedAttackers;
    attackers.erase(::std::remove(attackers.begin(), attackers.end(), player), attackers.end());
}

::std::vector<ObjectGuid> ObjectiveManager::GetAssignedDefenders(uint32 objectiveId) const
{
    const BGObjective* obj = GetObjective(objectiveId);
    return obj ? obj->assignedDefenders : ::std::vector<ObjectGuid>();
}

::std::vector<ObjectGuid> ObjectiveManager::GetAssignedAttackers(uint32 objectiveId) const
{
    const BGObjective* obj = GetObjective(objectiveId);
    return obj ? obj->assignedAttackers : ::std::vector<ObjectGuid>();
}

uint32 ObjectiveManager::GetDefenderCount(uint32 objectiveId) const
{
    const BGObjective* obj = GetObjective(objectiveId);
    return obj ? static_cast<uint32>(obj->assignedDefenders.size()) : 0;
}

uint32 ObjectiveManager::GetAttackerCount(uint32 objectiveId) const
{
    const BGObjective* obj = GetObjective(objectiveId);
    return obj ? static_cast<uint32>(obj->assignedAttackers.size()) : 0;
}

// ============================================================================
// STATISTICS
// ============================================================================

uint32 ObjectiveManager::GetControlledCount() const
{
    uint32 count = 0;
    for (const auto& [id, obj] : _objectives)
    {
        if (IsFriendlyState(obj.state))
            count++;
    }
    return count;
}

uint32 ObjectiveManager::GetEnemyControlledCount() const
{
    uint32 count = 0;
    for (const auto& [id, obj] : _objectives)
    {
        if (IsEnemyState(obj.state))
            count++;
    }
    return count;
}

uint32 ObjectiveManager::GetContestedCount() const
{
    uint32 count = 0;
    for (const auto& [id, obj] : _objectives)
    {
        if (obj.isContested)
            count++;
    }
    return count;
}

uint32 ObjectiveManager::GetNeutralCount() const
{
    uint32 count = 0;
    for (const auto& [id, obj] : _objectives)
    {
        if (obj.state == BGObjectiveState::NEUTRAL)
            count++;
    }
    return count;
}

float ObjectiveManager::GetControlRatio() const
{
    if (_objectives.empty())
        return 0.5f;

    uint32 friendly = GetControlledCount();
    uint32 total = static_cast<uint32>(_objectives.size());

    return static_cast<float>(friendly) / total;
}

// ============================================================================
// SCORING HELPERS
// ============================================================================

float ObjectiveManager::ScoreStrategicValue(const BGObjective& objective) const
{
    return objective.strategicValue / 10.0f;
}

float ObjectiveManager::ScoreContestability(const BGObjective& objective) const
{
    // Easier to contest if more allies nearby
    if (objective.nearbyAllyCount > objective.nearbyEnemyCount)
        return 0.8f;

    // Contested objectives are higher priority
    if (objective.isContested)
        return 0.6f;

    return 0.3f;
}

float ObjectiveManager::ScoreProximity(const BGObjective& objective) const
{
    // Would score based on average distance of our bots
    (void)objective;
    return 0.5f;
}

float ObjectiveManager::ScoreResourceValue(const BGObjective& objective) const
{
    // Some objectives generate more resources
    return objective.strategicValue / 10.0f;
}

// ============================================================================
// UTILITY
// ============================================================================

void ObjectiveManager::UpdateNearbyPlayerCounts()
{
    // Would update nearbyEnemyCount and nearbyAllyCount for each objective
    // Based on player positions relative to objective positions
}

float ObjectiveManager::GetDistance(float x1, float y1, float z1, float x2, float y2, float z2) const
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

bool ObjectiveManager::IsFriendlyState(BGObjectiveState state) const
{
    uint32 faction = _coordinator->GetFaction();

    if (faction == ALLIANCE)
    {
        return state == BGObjectiveState::ALLIANCE_CONTROLLED ||
               state == BGObjectiveState::ALLIANCE_CONTESTED ||
               state == BGObjectiveState::ALLIANCE_CAPTURING;
    }

    return state == BGObjectiveState::HORDE_CONTROLLED ||
           state == BGObjectiveState::HORDE_CONTESTED ||
           state == BGObjectiveState::HORDE_CAPTURING;
}

bool ObjectiveManager::IsEnemyState(BGObjectiveState state) const
{
    uint32 faction = _coordinator->GetFaction();

    if (faction == ALLIANCE)
    {
        return state == BGObjectiveState::HORDE_CONTROLLED ||
               state == BGObjectiveState::HORDE_CONTESTED ||
               state == BGObjectiveState::HORDE_CAPTURING;
    }

    return state == BGObjectiveState::ALLIANCE_CONTROLLED ||
           state == BGObjectiveState::ALLIANCE_CONTESTED ||
           state == BGObjectiveState::ALLIANCE_CAPTURING;
}

} // namespace Playerbot
