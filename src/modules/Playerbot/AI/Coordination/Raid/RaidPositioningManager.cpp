/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#include "RaidPositioningManager.h"
#include "RaidCoordinator.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot {

RaidPositioningManager::RaidPositioningManager(RaidCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void RaidPositioningManager::Initialize()
{
    Reset();
    TC_LOG_DEBUG("playerbots.raid", "RaidPositioningManager::Initialize - Initialized");
}

void RaidPositioningManager::Update(uint32 /*diff*/)
{
    UpdatePositionCompliance();
}

void RaidPositioningManager::Reset()
{
    _assignments.clear();
    _namedPositions.clear();
    _isSpread = false;
    _isStacked = false;
}

void RaidPositioningManager::AssignPosition(ObjectGuid player, float x, float y, float z)
{
    PositionAssignment assignment;
    assignment.playerGuid = player;
    assignment.x = x;
    assignment.y = y;
    assignment.z = z;
    assignment.allowedDeviation = _positionTolerance;

    _assignments[player] = assignment;

    TC_LOG_DEBUG("playerbots.raid", "RaidPositioningManager::AssignPosition - Position assigned");
}

void RaidPositioningManager::AssignPosition(ObjectGuid player, const ::std::string& positionName, float x, float y, float z)
{
    PositionAssignment assignment;
    assignment.playerGuid = player;
    assignment.positionName = positionName;
    assignment.x = x;
    assignment.y = y;
    assignment.z = z;
    assignment.allowedDeviation = _positionTolerance;

    _assignments[player] = assignment;
}

void RaidPositioningManager::ClearPosition(ObjectGuid player)
{
    _assignments.erase(player);
}

const PositionAssignment* RaidPositioningManager::GetPositionAssignment(ObjectGuid player) const
{
    auto it = _assignments.find(player);
    return it != _assignments.end() ? &it->second : nullptr;
}

void RaidPositioningManager::CallSpread(float distance)
{
    _isSpread = true;
    _isStacked = false;
    _spreadDistance = distance;

    TC_LOG_DEBUG("playerbots.raid", "RaidPositioningManager::CallSpread - Spread %.1f yards", distance);
}

void RaidPositioningManager::CallStack(float x, float y, float z)
{
    _isStacked = true;
    _isSpread = false;
    _stackX = x;
    _stackY = y;
    _stackZ = z;

    // Assign all members to stack point
    for (ObjectGuid guid : _coordinator->GetAllMembers())
    {
        AssignPosition(guid, "Stack Point", x, y, z);
    }

    TC_LOG_DEBUG("playerbots.raid", "RaidPositioningManager::CallStack - Stack at (%.1f, %.1f, %.1f)", x, y, z);
}

void RaidPositioningManager::CallMoveToPosition(const ::std::string& positionName)
{
    float x, y, z;
    if (GetNamedPosition(positionName, x, y, z))
    {
        for (ObjectGuid guid : _coordinator->GetAllMembers())
        {
            AssignPosition(guid, positionName, x, y, z);
        }
        TC_LOG_DEBUG("playerbots.raid", "RaidPositioningManager::CallMoveToPosition - Move to %s", positionName.c_str());
    }
}

void RaidPositioningManager::CallLooseSpread(float minDistance)
{
    _isSpread = true;
    _isStacked = false;
    _spreadDistance = minDistance;

    TC_LOG_DEBUG("playerbots.raid", "RaidPositioningManager::CallLooseSpread - Loose spread %.1f yards", minDistance);
}

void RaidPositioningManager::RegisterNamedPosition(const ::std::string& name, float x, float y, float z)
{
    _namedPositions[name] = std::make_tuple(x, y, z);
    TC_LOG_DEBUG("playerbots.raid", "RaidPositioningManager::RegisterNamedPosition - Registered %s", name.c_str());
}

void RaidPositioningManager::UnregisterNamedPosition(const ::std::string& name)
{
    _namedPositions.erase(name);
}

bool RaidPositioningManager::GetNamedPosition(const ::std::string& name, float& x, float& y, float& z) const
{
    auto it = _namedPositions.find(name);
    if (it == _namedPositions.end())
        return false;

    x = std::get<0>(it->second);
    y = std::get<1>(it->second);
    z = std::get<2>(it->second);
    return true;
}

std::vector<std::string> RaidPositioningManager::GetAllNamedPositions() const
{
    std::vector<std::string> names;
    names.reserve(_namedPositions.size());

    for (const auto& pair : _namedPositions)
    {
        names.push_back(pair.first);
    }

    return names;
}

bool RaidPositioningManager::IsPlayerInPosition(ObjectGuid player) const
{
    return GetDistanceFromPosition(player) <= _positionTolerance;
}

float RaidPositioningManager::GetDistanceFromPosition(ObjectGuid player) const
{
    const PositionAssignment* assignment = GetPositionAssignment(player);
    if (!assignment)
        return 0.0f;

    Player* p = _coordinator->GetPlayer(player);
    if (!p)
        return std::numeric_limits<float>::max();

    return p->GetDistance(assignment->x, assignment->y, assignment->z);
}

std::vector<ObjectGuid> RaidPositioningManager::GetPlayersOutOfPosition() const
{
    std::vector<ObjectGuid> outOfPosition;

    for (const auto& pair : _assignments)
    {
        if (!IsPlayerInPosition(pair.first))
        {
            outOfPosition.push_back(pair.first);
        }
    }

    return outOfPosition;
}

float RaidPositioningManager::GetOverallPositionCompliance() const
{
    if (_assignments.empty())
        return 100.0f;

    uint32 inPosition = 0;
    for (const auto& pair : _assignments)
    {
        if (IsPlayerInPosition(pair.first))
            inPosition++;
    }

    return static_cast<float>(inPosition) / _assignments.size() * 100.0f;
}

void RaidPositioningManager::UpdatePositionCompliance()
{
    // Check for players that have reached their positions
    // This could trigger events or state changes
}

} // namespace Playerbot
