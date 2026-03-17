/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#include "KitingManager.h"
#include "RaidCoordinator.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot {

KitingManager::KitingManager(RaidCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void KitingManager::Initialize()
{
    Reset();
    TC_LOG_DEBUG("playerbots.raid", "KitingManager::Initialize - Initialized");
}

void KitingManager::Update(uint32 /*diff*/)
{
    // Update kiter positions and distances
    for (auto& assignment : _assignments)
    {
        if (!assignment.isActive)
            continue;

        // Check distance
        if (IsTooClose(assignment.kiterGuid))
        {
            TC_LOG_DEBUG("playerbots.raid", "KitingManager::Update - Kiter too close!");
        }
    }
}

void KitingManager::Reset()
{
    _paths.clear();
    _assignments.clear();
    _nextPathId = 1;
}

void KitingManager::RegisterPath(const KitePath& path)
{
    KitePath newPath = path;
    if (newPath.pathId == 0)
    {
        newPath.pathId = _nextPathId++;
    }
    _paths[newPath.pathId] = newPath;
    TC_LOG_DEBUG("playerbots.raid", "KitingManager::RegisterPath - Registered path %u with %zu waypoints",
        newPath.pathId, newPath.waypoints.size());
}

void KitingManager::UnregisterPath(uint32 pathId)
{
    _paths.erase(pathId);
}

const KitePath* KitingManager::GetPath(uint32 pathId) const
{
    auto it = _paths.find(pathId);
    return it != _paths.end() ? &it->second : nullptr;
}

std::vector<uint32> KitingManager::GetAllPathIds() const
{
    std::vector<uint32> ids;
    for (const auto& pair : _paths)
    {
        ids.push_back(pair.first);
    }
    return ids;
}

void KitingManager::AssignKiter(ObjectGuid kiter, ObjectGuid target, uint32 pathId)
{
    KiteAssignment assignment;
    assignment.kiterGuid = kiter;
    assignment.targetGuid = target;
    assignment.pathId = pathId;
    assignment.currentWaypointIndex = 0;
    assignment.isActive = true;

    _assignments.push_back(assignment);

    TC_LOG_DEBUG("playerbots.raid", "KitingManager::AssignKiter - Kiter assigned to path %u", pathId);
}

void KitingManager::UnassignKiter(ObjectGuid kiter)
{
    auto it = std::remove_if(_assignments.begin(), _assignments.end(),
        [kiter](const KiteAssignment& a) { return a.kiterGuid == kiter; });
    _assignments.erase(it, _assignments.end());
}

void KitingManager::SwapKiter(ObjectGuid oldKiter, ObjectGuid newKiter)
{
    KiteAssignment* assignment = FindAssignment(oldKiter);
    if (assignment)
    {
        assignment->kiterGuid = newKiter;
        TC_LOG_DEBUG("playerbots.raid", "KitingManager::SwapKiter - Kiter swapped");
    }
}

ObjectGuid KitingManager::GetKiterForTarget(ObjectGuid target) const
{
    for (const auto& assignment : _assignments)
    {
        if (assignment.targetGuid == target && assignment.isActive)
            return assignment.kiterGuid;
    }
    return ObjectGuid();
}

bool KitingManager::IsKiting(ObjectGuid player) const
{
    return FindAssignment(player) != nullptr;
}

std::tuple<float, float, float> KitingManager::GetNextWaypoint(ObjectGuid kiter) const
{
    const KiteAssignment* assignment = FindAssignment(kiter);
    if (!assignment)
        return std::make_tuple(0.0f, 0.0f, 0.0f);

    const KitePath* path = GetPath(assignment->pathId);
    if (!path || path->waypoints.empty())
        return std::make_tuple(0.0f, 0.0f, 0.0f);

    uint32 nextIndex = (assignment->currentWaypointIndex + 1) % path->waypoints.size();
    if (!path->isLoop && nextIndex < assignment->currentWaypointIndex)
        nextIndex = assignment->currentWaypointIndex;

    const KiteWaypoint& wp = path->waypoints[nextIndex];
    return std::make_tuple(wp.x, wp.y, wp.z);
}

std::tuple<float, float, float> KitingManager::GetCurrentWaypoint(ObjectGuid kiter) const
{
    const KiteAssignment* assignment = FindAssignment(kiter);
    if (!assignment)
        return std::make_tuple(0.0f, 0.0f, 0.0f);

    const KitePath* path = GetPath(assignment->pathId);
    if (!path || path->waypoints.empty())
        return std::make_tuple(0.0f, 0.0f, 0.0f);

    const KiteWaypoint& wp = path->waypoints[assignment->currentWaypointIndex];
    return std::make_tuple(wp.x, wp.y, wp.z);
}

void KitingManager::AdvanceWaypoint(ObjectGuid kiter)
{
    KiteAssignment* assignment = FindAssignment(kiter);
    if (!assignment)
        return;

    const KitePath* path = GetPath(assignment->pathId);
    if (!path || path->waypoints.empty())
        return;

    if (path->isLoop)
    {
        assignment->currentWaypointIndex = (assignment->currentWaypointIndex + 1) % path->waypoints.size();
    }
    else if (assignment->currentWaypointIndex + 1 < path->waypoints.size())
    {
        assignment->currentWaypointIndex++;
    }
}

uint32 KitingManager::GetCurrentWaypointIndex(ObjectGuid kiter) const
{
    const KiteAssignment* assignment = FindAssignment(kiter);
    return assignment ? assignment->currentWaypointIndex : 0;
}

float KitingManager::GetRecommendedDistance(ObjectGuid kiter) const
{
    const KiteAssignment* assignment = FindAssignment(kiter);
    if (!assignment)
        return _defaultDistance;

    const KitePath* path = GetPath(assignment->pathId);
    return path ? path->safeDistance : _defaultDistance;
}

float KitingManager::GetCurrentDistance(ObjectGuid kiter) const
{
    const KiteAssignment* assignment = FindAssignment(kiter);
    if (!assignment)
        return 0.0f;

    Player* kiterPlayer = _coordinator->GetPlayer(kiter);
    Player* targetPlayer = _coordinator->GetPlayer(assignment->targetGuid);

    if (!kiterPlayer || !targetPlayer)
        return 0.0f;

    return kiterPlayer->GetDistance(targetPlayer);
}

bool KitingManager::IsTooClose(ObjectGuid kiter) const
{
    return GetCurrentDistance(kiter) < _tooCloseThreshold;
}

bool KitingManager::IsTooFar(ObjectGuid kiter) const
{
    return GetCurrentDistance(kiter) > _tooFarThreshold;
}

void KitingManager::OnKiterDied(ObjectGuid kiter)
{
    TC_LOG_DEBUG("playerbots.raid", "KitingManager::OnKiterDied - Kiter died, finding replacement!");

    KiteAssignment* assignment = FindAssignment(kiter);
    if (!assignment)
        return;

    ObjectGuid emergency = FindEmergencyKiter(assignment->targetGuid);
    if (!emergency.IsEmpty())
    {
        SwapKiter(kiter, emergency);
    }
    else
    {
        assignment->isActive = false;
    }
}

void KitingManager::OnDeathEvent(const CombatEvent& event)
{
    if (IsKiting(event.source))
    {
        OnKiterDied(event.source);
    }
}

ObjectGuid KitingManager::FindEmergencyKiter(ObjectGuid target) const
{
    for (ObjectGuid guid : _coordinator->GetDPS())
    {
        if (IsKiting(guid))
            continue;

        Player* player = _coordinator->GetPlayer(guid);
        if (player && player->IsAlive())
        {
            // Prefer mobile classes for kiting
            uint8 pClass = player->GetClass();
            if (pClass == 3 || pClass == 11 || pClass == 8) // Hunter, Druid, Mage
                return guid;
        }
    }

    // Any DPS if no mobile class found
    for (ObjectGuid guid : _coordinator->GetDPS())
    {
        if (IsKiting(guid))
            continue;

        Player* player = _coordinator->GetPlayer(guid);
        if (player && player->IsAlive())
            return guid;
    }

    return ObjectGuid();
}

KiteAssignment* KitingManager::FindAssignment(ObjectGuid kiter)
{
    for (auto& assignment : _assignments)
    {
        if (assignment.kiterGuid == kiter)
            return &assignment;
    }
    return nullptr;
}

const KiteAssignment* KitingManager::FindAssignment(ObjectGuid kiter) const
{
    for (const auto& assignment : _assignments)
    {
        if (assignment.kiterGuid == kiter)
            return &assignment;
    }
    return nullptr;
}

} // namespace Playerbot
