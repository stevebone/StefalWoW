/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#pragma once

#include "RaidState.h"
#include "Core/Events/CombatEvent.h"
#include <vector>
#include <map>

namespace Playerbot {

class RaidCoordinator;

struct KiteAssignment
{
    ObjectGuid kiterGuid;
    ObjectGuid targetGuid;
    uint32 pathId;
    uint32 currentWaypointIndex;
    bool isActive;
};

class KitingManager
{
public:
    KitingManager(RaidCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // Path Management
    void RegisterPath(const KitePath& path);
    void UnregisterPath(uint32 pathId);
    const KitePath* GetPath(uint32 pathId) const;
    ::std::vector<uint32> GetAllPathIds() const;

    // Kiter Assignment
    void AssignKiter(ObjectGuid kiter, ObjectGuid target, uint32 pathId);
    void UnassignKiter(ObjectGuid kiter);
    void SwapKiter(ObjectGuid oldKiter, ObjectGuid newKiter);
    ObjectGuid GetKiterForTarget(ObjectGuid target) const;
    bool IsKiting(ObjectGuid player) const;

    // Waypoint Navigation
    ::std::tuple<float, float, float> GetNextWaypoint(ObjectGuid kiter) const;
    ::std::tuple<float, float, float> GetCurrentWaypoint(ObjectGuid kiter) const;
    void AdvanceWaypoint(ObjectGuid kiter);
    uint32 GetCurrentWaypointIndex(ObjectGuid kiter) const;

    // Distance Management
    float GetRecommendedDistance(ObjectGuid kiter) const;
    float GetCurrentDistance(ObjectGuid kiter) const;
    bool IsTooClose(ObjectGuid kiter) const;
    bool IsTooFar(ObjectGuid kiter) const;

    // Emergency Handling
    void OnKiterDied(ObjectGuid kiter);
    void OnDeathEvent(const CombatEvent& event);
    ObjectGuid FindEmergencyKiter(ObjectGuid target) const;

private:
    RaidCoordinator* _coordinator;

    ::std::map<uint32, KitePath> _paths;
    ::std::vector<KiteAssignment> _assignments;
    uint32 _nextPathId = 1;
    float _defaultDistance = 15.0f;
    float _tooCloseThreshold = 10.0f;
    float _tooFarThreshold = 30.0f;

    KiteAssignment* FindAssignment(ObjectGuid kiter);
    const KiteAssignment* FindAssignment(ObjectGuid kiter) const;
};

} // namespace Playerbot
