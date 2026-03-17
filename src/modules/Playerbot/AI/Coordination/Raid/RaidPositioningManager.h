/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#pragma once

#include "RaidState.h"
#include <vector>
#include <map>

namespace Playerbot {

class RaidCoordinator;

class RaidPositioningManager
{
public:
    RaidPositioningManager(RaidCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // Position Assignment
    void AssignPosition(ObjectGuid player, float x, float y, float z);
    void AssignPosition(ObjectGuid player, const ::std::string& positionName, float x, float y, float z);
    void ClearPosition(ObjectGuid player);
    const PositionAssignment* GetPositionAssignment(ObjectGuid player) const;

    // Group Commands
    void CallSpread(float distance);
    void CallStack(float x, float y, float z);
    void CallMoveToPosition(const ::std::string& positionName);
    void CallLooseSpread(float minDistance);

    // Named Positions
    void RegisterNamedPosition(const ::std::string& name, float x, float y, float z);
    void UnregisterNamedPosition(const ::std::string& name);
    bool GetNamedPosition(const ::std::string& name, float& x, float& y, float& z) const;
    ::std::vector<::std::string> GetAllNamedPositions() const;

    // Compliance Checking
    bool IsPlayerInPosition(ObjectGuid player) const;
    float GetDistanceFromPosition(ObjectGuid player) const;
    ::std::vector<ObjectGuid> GetPlayersOutOfPosition() const;
    float GetOverallPositionCompliance() const;

    // Spread/Stack
    bool IsCurrentlySpread() const { return _isSpread; }
    bool IsCurrentlyStacked() const { return _isStacked; }
    float GetCurrentSpreadDistance() const { return _spreadDistance; }

private:
    RaidCoordinator* _coordinator;

    ::std::map<ObjectGuid, PositionAssignment> _assignments;
    ::std::map<::std::string, ::std::tuple<float, float, float>> _namedPositions;
    bool _isSpread = false;
    bool _isStacked = false;
    float _spreadDistance = 8.0f;
    float _stackX = 0.0f, _stackY = 0.0f, _stackZ = 0.0f;
    float _positionTolerance = 5.0f;

    void UpdatePositionCompliance();
};

} // namespace Playerbot
