/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "FormationManager.h"
#include "GameTime.h"
#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "Log.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectAccessor.h"
#include "Movement/MotionMaster.h"
#include "Movement/UnifiedMovementCoordinator.h"
#include "../../Movement/Arbiter/MovementPriorityMapper.h"
#include "../BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "UnitAI.h"
#include "DataStores/DBCEnums.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

FormationManager::FormationManager(Player* bot)
    : _bot(bot), _leader(nullptr), _isLeader(false), _inFormation(false),
      _currentFormation(MovementFormationType::NONE), _movementState(FormationMovementState::STATIONARY),
      _currentIntegrity(FormationIntegrity::PERFECT), _formationOrientation(0.0f),
      _isMovingToDestination(false), _updateInterval(DEFAULT_UPDATE_INTERVAL),
      _cohesionRadius(DEFAULT_COHESION_RADIUS), _formationSpacing(DEFAULT_FORMATION_SPACING),
      _reformationThreshold(DEFAULT_REFORMATION_THRESHOLD), _formationPriority(0),
      _adaptiveFormations(true), _emergencyScatter(false), _lastUpdate(0),
      _lastIntegrityCheck(0), _lastReformation(0)
{
    InitializeFormationConfigs();
    // CRITICAL: No logging with _bot->GetName() in constructor
    // Player's m_name can be corrupted during concurrent access, causing ACCESS_VIOLATION
}

bool FormationManager::JoinFormation(const ::std::vector<Player*>& groupMembers, MovementFormationType formation)
{
    // No lock needed - _members is per-bot instance data
    try
    {
        if (_inFormation)
        {
            TC_LOG_DEBUG("playerbot.formation", "Bot {} already in formation", _bot->GetName());
            return false;
        }

        _members.clear();
        _members.reserve(groupMembers.size());

        Player* groupLeader = nullptr;
        if (Group* group = _bot->GetGroup())
        {
            groupLeader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
        }

        if (!groupLeader && !groupMembers.empty())
        {
            groupLeader = groupMembers[0];
        }

        _leader = groupLeader;
        _isLeader = (_leader == _bot);
        for (Player* member : groupMembers)
        {
            if (!member || !member->IsInWorld())
                continue;

            FormationMember formationMember;
            formationMember.guid = member->GetGUID();
            formationMember.player = member;
            formationMember.role = DeterminePlayerRole(member);
            formationMember.currentPosition = member->GetPosition();
            formationMember.name = member->GetName();
            formationMember.lastPositionUpdate = GameTime::GetGameTimeMS();
            formationMember.movementSpeed = member->GetSpeed(MOVE_RUN);
            formationMember.formationSlot = static_cast<uint32>(_members.size());

            _members.push_back(formationMember);
        }

        _currentFormation = formation;
        _inFormation = true;

        if (_isLeader)
        {
            _formationCenter = _bot->GetPosition();
            _formationOrientation = _bot->GetOrientation();
        }
        else
        {
            _formationCenter = FormationUtils::CalculateFormationCenterFromMembers(groupMembers);
            _formationOrientation = _leader ? _leader->GetOrientation() : 0.0f;
        }
        AssignFormationPositions();
        _metrics.formationChanges++;

        TC_LOG_DEBUG("playerbot.formation", "Bot {} joined formation {} with {} members",
                   _bot->GetName(), static_cast<uint32>(formation), _members.size());

        return true;
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.formation", "Exception in JoinFormation for bot {}: {}", _bot->GetName(), e.what());
        return false;
    }
}

bool FormationManager::LeaveFormation()
{
    // No lock needed - _inFormation and _members are per-bot instance data
    if (!_inFormation)
        return false;

    _inFormation = false;
    _currentFormation = MovementFormationType::NONE;
    _movementState = FormationMovementState::STATIONARY;
    _members.clear();
    _leader = nullptr;
    _isLeader = false;
    TC_LOG_DEBUG("playerbot.formation", "Bot {} left formation", _bot->GetName());
    return true;
}

bool FormationManager::ChangeFormation(MovementFormationType newFormation)
{
    // No lock needed - _currentFormation is per-bot instance data
    if (!_inFormation || newFormation == _currentFormation)
        return false;

    if (!_isLeader)
    {
        TC_LOG_DEBUG("playerbot.formation", "Bot {} cannot change formation - not leader", _bot->GetName());
        return false;
    }

    auto startTime = ::std::chrono::steady_clock::now();

    _currentFormation = newFormation;
    _movementState = FormationMovementState::REFORMING;

    AssignFormationPositions();
    _metrics.formationChanges++;

    auto endTime = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    TrackPerformance(duration, "ChangeFormation");

    TC_LOG_DEBUG("playerbot.formation", "Bot {} changed formation to {}",
               _bot->GetName(), static_cast<uint32>(newFormation));

    return true;
}

void FormationManager::UpdateFormation(uint32 /*diff*/)
{
    // No lock needed - all formation data is per-bot instance
    if (!_inFormation)
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastUpdate < _updateInterval)
        return;

    _lastUpdate = currentTime;

    try
    {
        UpdateMemberPositions();

        if (currentTime - _lastIntegrityCheck >= INTEGRITY_CHECK_INTERVAL)
        {
            MonitorFormationIntegrity();
            _lastIntegrityCheck = currentTime;
        }
        if (_movementState == FormationMovementState::MOVING)
        {
            MaintainFormationDuringMovement();
        }
        if (RequiresReformation() && currentTime - _lastReformation >= MIN_REFORMATION_INTERVAL)
        {
            TriggerReformationIfNeeded();
            _lastReformation = currentTime;
        }

        if (_adaptiveFormations)
        {
            AdjustForTerrain();
            AdjustForGroupSize();
        }
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.formation", "Exception in UpdateFormation for bot {}: {}", _bot->GetName(), e.what());
    }
}

bool FormationManager::ExecuteFormationCommand(const FormationCommand& command)
{
    // No lock needed - command execution modifies per-bot instance data
    if (!_inFormation)
        return false;

    try
    {
        auto startTime = ::std::chrono::steady_clock::now();

        if (command.newFormation != MovementFormationType::NONE && command.newFormation != _currentFormation)
        {
            _currentFormation = command.newFormation;
            AssignFormationPositions();
        }

        _movementState = command.movementState;

        if (command.targetPosition.GetPositionX() != 0.0f || command.targetPosition.GetPositionY() != 0.0f || command.targetPosition.GetPositionZ() != 0.0f)
        {
            _targetDestination = command.targetPosition;
            _isMovingToDestination = true;
            _formationOrientation = command.targetOrientation;
        }

        CalculateMovementTargets();
        IssueMovementCommands();

        auto endTime = ::std::chrono::steady_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
        TrackPerformance(duration, "ExecuteFormationCommand");
        TC_LOG_DEBUG("playerbot.formation", "Bot {} executed formation command", _bot->GetName());
        return true;
    }
    catch (const ::std::exception& e)
    {
        TC_LOG_ERROR("playerbot.formation", "Exception executing formation command for bot {}: {}", _bot->GetName(), e.what());
        return false;
    }
}

bool FormationManager::MoveFormationToPosition(const Position& targetPos, float orientation)
{
    if (!_inFormation || !_isLeader)
        return false;
    FormationCommand command;
    command.targetPosition = targetPos;
    command.targetOrientation = orientation != 0.0f ? orientation : _formationOrientation;
    command.movementState = FormationMovementState::MOVING;
    command.maintainCohesion = true;
    command.reason = "Formation movement";
    return ExecuteFormationCommand(command);
}

Position FormationManager::CalculateFormationPosition(FormationRole role, uint32 memberIndex)
{
    Position leaderPos = _leader ? _leader->GetPosition() : _formationCenter;

    switch (_currentFormation)
    {
        case MovementFormationType::LINE:
            return CalculateLineFormation(leaderPos, _formationOrientation)[memberIndex % CalculateLineFormation(leaderPos, _formationOrientation).size()];

        case MovementFormationType::COLUMN:
            return CalculateColumnFormation(leaderPos, _formationOrientation)[memberIndex % CalculateColumnFormation(leaderPos, _formationOrientation).size()];

        case MovementFormationType::WEDGE:
            return CalculateWedgeFormation(leaderPos, _formationOrientation)[memberIndex % CalculateWedgeFormation(leaderPos, _formationOrientation).size()];

        case MovementFormationType::DIAMOND:
            return CalculateDiamondFormation(leaderPos, _formationOrientation)[memberIndex % CalculateDiamondFormation(leaderPos, _formationOrientation).size()];

        case MovementFormationType::CIRCLE:
            return CalculateCircleFormation(leaderPos)[memberIndex % CalculateCircleFormation(leaderPos).size()];

        case MovementFormationType::BOX:
            return CalculateBoxFormation(leaderPos, _formationOrientation)[memberIndex % CalculateBoxFormation(leaderPos, _formationOrientation).size()];

        case MovementFormationType::DUNGEON:
            return CalculateDungeonFormation(leaderPos, _formationOrientation)[memberIndex % CalculateDungeonFormation(leaderPos, _formationOrientation).size()];

        case MovementFormationType::RAID:
            return CalculateRaidFormation(leaderPos, _formationOrientation)[memberIndex % CalculateRaidFormation(leaderPos, _formationOrientation).size()];

        default:
            return CalculateRoleBasedPosition(role, leaderPos, _formationOrientation);
    }
}

::std::vector<Position> FormationManager::CalculateAllFormationPositions()
{
    ::std::vector<Position> positions;
    positions.reserve(_members.size());

    for (size_t i = 0; i < _members.size(); ++i)
    {
        Position pos = CalculateFormationPosition(_members[i].role, static_cast<uint32>(i));
        positions.push_back(pos);
    }

    return positions;
}

Position FormationManager::GetAssignedPosition() const
{
    // No lock needed - reading per-bot instance data (_members)
    for (const FormationMember& member : _members)
    {
        if (member.player == _bot)
        {
            return member.assignedPosition;
        }
    }

    return _bot->GetPosition();
}

bool FormationManager::IsInFormationPosition(float tolerance) const
{
    // No lock needed - GetAssignedPosition() reads per-bot instance data
    Position assignedPos = GetAssignedPosition();
    Position currentPos = _bot->GetPosition();

    return currentPos.GetExactDist(&assignedPos) <= tolerance;
}

FormationIntegrity FormationManager::AssessFormationIntegrity()
{
    // No lock needed - assessing per-bot instance data (_members, _formationSpacing)
    if (_members.empty())
        return FormationIntegrity::BROKEN;

    uint32 inPosition = 0;
    uint32 closeToPosition = 0;
    float totalDeviation = 0.0f;

    for (const FormationMember& member : _members)
    {
        if (member.isInPosition)
        {
            inPosition++;
        }
        else if (member.distanceFromAssigned <= _formationSpacing * 1.5f)
        {
            closeToPosition++;
        }

        totalDeviation += member.distanceFromAssigned;
    }

    float averageDeviation = totalDeviation / _members.size();
    float inPositionRatio = static_cast<float>(inPosition) / _members.size();
    float closePositionRatio = static_cast<float>(inPosition + closeToPosition) / _members.size();

    if (inPositionRatio >= 0.9f && averageDeviation <= _formationSpacing * 0.5f)
        return FormationIntegrity::PERFECT;
    else if (inPositionRatio >= 0.7f && averageDeviation <= _formationSpacing)
        return FormationIntegrity::GOOD;
    else if (closePositionRatio >= 0.6f && averageDeviation <= _formationSpacing * 1.5f)
        return FormationIntegrity::ACCEPTABLE;
    else if (closePositionRatio >= 0.3f)
        return FormationIntegrity::POOR;
    else
        return FormationIntegrity::BROKEN;
}

void FormationManager::CoordinateMovement(const Position& destination)
{
    if (!_inFormation)
        return;

    if (_isLeader)
    {
        MoveFormationToPosition(destination);
    }
    else
    {
        Position myAssignedPos = GetAssignedPosition();
        float spacingSq = _formationSpacing * _formationSpacing;
        if (_bot->GetExactDistSq(myAssignedPos) > spacingSq)
        {
            // PHASE 6B: Use Movement Arbiter with FORMATION priority (160)
            BotAI* botAI = dynamic_cast<BotAI*>(_bot->GetAI());
            if (botAI && botAI->GetUnifiedMovementCoordinator())
            {
                botAI->RequestPointMovement(
                    PlayerBotMovementPriority::FORMATION,
                    myAssignedPos,
                    "Formation position maintenance",
                    "FormationManager");
            }
            else
            {
                // FALLBACK: Use BotMovementController with validated pathfinding
                if (BotAI* ai = GetBotAI(_bot))
                {
                    if (!ai->MoveTo(myAssignedPos, true))
                    {
                        // Final fallback to legacy if validation fails
                        _bot->GetMotionMaster()->MovePoint(0, myAssignedPos.GetPositionX(), myAssignedPos.GetPositionY(), myAssignedPos.GetPositionZ());
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    _bot->GetMotionMaster()->MovePoint(0, myAssignedPos.GetPositionX(), myAssignedPos.GetPositionY(), myAssignedPos.GetPositionZ());
                }
            }
        }
    }
}

bool FormationManager::CanMoveWithoutBreakingFormation(const Position& newPos)
{
    // No lock needed - checking per-bot instance data (_formationCenter, _cohesionRadius)
    if (!_inFormation)
        return true;

    Position currentCenter = _formationCenter;
    float currentCohesion = CalculateCohesionLevel();

    float distanceFromCenter = newPos.GetExactDist(&currentCenter);

    return distanceFromCenter <= _cohesionRadius && currentCohesion >= 0.6f;
}

Position FormationManager::AdjustMovementForFormation(const Position& intendedPos)
{
    // No lock needed - adjusting based on per-bot instance data
    if (!_inFormation)
        return intendedPos;

    Position assignedPos = GetAssignedPosition();
    Position currentPos = _bot->GetPosition();

    float distanceToAssigned = currentPos.GetExactDist(&assignedPos);
    float distanceToIntended = currentPos.GetExactDist(&intendedPos);

    if (distanceToAssigned > _formationSpacing * 1.5f)
    {
        float blendFactor = ::std::min(1.0f, distanceToAssigned / _cohesionRadius);

        Position adjusted;
        adjusted.m_positionX = intendedPos.GetPositionX() * (1.0f - blendFactor) + assignedPos.GetPositionX() * blendFactor;
        adjusted.m_positionY = intendedPos.GetPositionY() * (1.0f - blendFactor) + assignedPos.GetPositionY() * blendFactor;
        adjusted.m_positionZ = intendedPos.GetPositionZ() * (1.0f - blendFactor) + assignedPos.GetPositionZ() * blendFactor;

        return adjusted;
    }

    return intendedPos;
}

void FormationManager::TransitionToCombatFormation(const ::std::vector<Unit*>& enemies)
{
    if (!_inFormation || !_isLeader)
        return;

    MovementFormationType combatFormation = FormationUtils::GetOptimalFormationForCombat(
        [this]() {
            ::std::vector<Player*> players;
            for (const auto& member : _members)
                if (member.player)
                    players.push_back(member.player);
            return players;
        }(),
        enemies
    );

    if (combatFormation != _currentFormation)
    {
        ChangeFormation(combatFormation);
        _movementState = FormationMovementState::COMBAT;
    }
}

::std::vector<Position> FormationManager::CalculateLineFormation(const Position& leaderPos, float orientation)
{
    ::std::vector<Position> positions;
    positions.reserve(_members.size());

    for (size_t i = 0; i < _members.size(); ++i)
    {
        float offset = (static_cast<float>(i) - static_cast<float>(_members.size() - 1) * 0.5f) * _formationSpacing;

        Position pos;
        pos.m_positionX = leaderPos.GetPositionX() + offset * ::std::cos(orientation + static_cast<float>(M_PI) / 2.0f);
        pos.m_positionY = leaderPos.GetPositionY() + offset * ::std::sin(orientation + static_cast<float>(M_PI) / 2.0f);
        pos.m_positionZ = leaderPos.GetPositionZ();

        positions.push_back(pos);
    }

    return positions;
}

::std::vector<Position> FormationManager::CalculateColumnFormation(const Position& leaderPos, float orientation)
{
    ::std::vector<Position> positions;
    positions.reserve(_members.size());

    for (size_t i = 0; i < _members.size(); ++i)
    {
        float distance = static_cast<float>(i) * _formationSpacing;

        Position pos;
        pos.m_positionX = leaderPos.GetPositionX() - distance * ::std::cos(orientation);
        pos.m_positionY = leaderPos.GetPositionY() - distance * ::std::sin(orientation);
        pos.m_positionZ = leaderPos.GetPositionZ();

        positions.push_back(pos);
    }

    return positions;
}

::std::vector<Position> FormationManager::CalculateWedgeFormation(const Position& leaderPos, float orientation)
{
    ::std::vector<Position> positions;
    positions.reserve(_members.size());

    positions.push_back(leaderPos);

    for (size_t i = 1; i < _members.size(); ++i)
    {
        float row = ::std::floor((::std::sqrt(8.0f * i + 1.0f) - 1.0f) / 2.0f);
        float posInRow = i - (row * (row + 1.0f) / 2.0f);
        bool isLeft = (static_cast<int>(posInRow) % 2) == 1;

        float angle = orientation + (isLeft ? -static_cast<float>(M_PI) / 6.0f : static_cast<float>(M_PI) / 6.0f);
        float distance = (row + 1.0f) * _formationSpacing;

        Position pos;
        pos.m_positionX = leaderPos.GetPositionX() + distance * ::std::cos(angle);
        pos.m_positionY = leaderPos.GetPositionY() + distance * ::std::sin(angle);
        pos.m_positionZ = leaderPos.GetPositionZ();

        positions.push_back(pos);
    }

    return positions;
}

::std::vector<Position> FormationManager::CalculateCircleFormation(const Position& leaderPos)
{
    ::std::vector<Position> positions;
    positions.reserve(_members.size());

    positions.push_back(leaderPos);

    if (_members.size() > 1)
    {
        float radius = _formationSpacing;
        float angleIncrement = 2.0f * static_cast<float>(M_PI) / (_members.size() - 1);

        for (size_t i = 1; i < _members.size(); ++i)
        {
            float angle = (i - 1) * angleIncrement;

            Position pos;
            pos.m_positionX = leaderPos.GetPositionX() + radius * ::std::cos(angle);
            pos.m_positionY = leaderPos.GetPositionY() + radius * ::std::sin(angle);
            pos.m_positionZ = leaderPos.GetPositionZ();

            positions.push_back(pos);
        }
    }

    return positions;
}

::std::vector<Position> FormationManager::CalculateDungeonFormation(const Position& leaderPos, float orientation)
{
    ::std::vector<Position> positions;
    positions.reserve(_members.size());

    ::std::vector<FormationRole> orderedRoles = {
        FormationRole::TANK,
        FormationRole::MELEE_DPS,
        FormationRole::RANGED_DPS,
        FormationRole::HEALER,
        FormationRole::SUPPORT
    };
    for (size_t i = 0; i < _members.size(); ++i)
    {
        FormationRole role = (i < orderedRoles.size()) ? orderedRoles[i] : FormationRole::SUPPORT;
        Position pos = CalculateRoleBasedPosition(role, leaderPos, orientation);
        positions.push_back(pos);
    }
    return positions;
}

Position FormationManager::CalculateRoleBasedPosition(FormationRole role, const Position& leaderPos, float orientation)
{
    Position pos = leaderPos;
    float distance = _formationSpacing;
    float angle = orientation;

    switch (role)
    {
        case FormationRole::TANK:
            distance = _formationSpacing * 0.5f;
            angle = orientation;
            break;

        case FormationRole::MELEE_DPS:
            distance = _formationSpacing;
            angle = orientation + static_cast<float>(M_PI) / 4.0f;
            break;

        case FormationRole::RANGED_DPS:
            distance = _formationSpacing * 1.5f;
            angle = orientation + static_cast<float>(M_PI);
            break;

        case FormationRole::HEALER:
            distance = _formationSpacing * 1.2f;
            angle = orientation + static_cast<float>(M_PI) + static_cast<float>(M_PI) / 3.0f;
            break;

        case FormationRole::SUPPORT:
            distance = _formationSpacing;
            angle = orientation + static_cast<float>(M_PI) / 2.0f;
            break;

        default:
            break;
    }

    pos.m_positionX += distance * ::std::cos(angle);
    pos.m_positionY += distance * ::std::sin(angle);

    return pos;
}
void FormationManager::AssignFormationPositions()
{
    ::std::vector<Position> positions = CalculateAllFormationPositions();

    for (size_t i = 0; i < _members.size() && i < positions.size(); ++i)
    {
        _members[i].assignedPosition = positions[i];
        _members[i].targetPosition = positions[i];
    }
}

void FormationManager::UpdateMemberPositions()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (FormationMember& member : _members)
    {
        if (!member.player || !member.player->IsInWorld())
            continue;

        member.currentPosition = member.player->GetPosition();
        member.distanceFromAssigned = member.currentPosition.GetExactDist(&member.assignedPosition);
        member.distanceFromLeader = _leader ? member.currentPosition.GetExactDist(_leader) : 0.0f;
        member.isInPosition = member.distanceFromAssigned <= _formationSpacing * 0.8f;
        member.isMoving = member.player->isMoving();
        member.lastPositionUpdate = currentTime;
    }
}

void FormationManager::MonitorFormationIntegrity()
{
    _currentIntegrity = AssessFormationIntegrity();

    float integrityValue = 0.0f;
    switch (_currentIntegrity)
    {
        case FormationIntegrity::PERFECT: integrityValue = 100.0f; break;
        case FormationIntegrity::GOOD: integrityValue = 80.0f; break;
        case FormationIntegrity::ACCEPTABLE: integrityValue = 60.0f; break;
        case FormationIntegrity::POOR: integrityValue = 40.0f; break;
        case FormationIntegrity::BROKEN: integrityValue = 20.0f; break;
    }

    _metrics.averageIntegrity = _metrics.averageIntegrity * 0.9f + integrityValue * 0.1f;
    if (integrityValue < _metrics.minIntegrity)
        _metrics.minIntegrity = integrityValue;

    if (_currentIntegrity <= FormationIntegrity::POOR)
    {
        _metrics.cohesionBreaks++;
    }
}

bool FormationManager::RequiresReformation()
{
    return _currentIntegrity <= FormationIntegrity::POOR ||
           CalculateCohesionLevel() < 0.5f;
}

void FormationManager::TriggerReformationIfNeeded()
{
    if (!RequiresReformation())
        return;

    AssignFormationPositions();
    _movementState = FormationMovementState::REFORMING;
    _metrics.reformationEvents++;

    TC_LOG_DEBUG("playerbot.formation", "Formation {} triggered reformation due to poor integrity",
               _bot->GetName());
}

float FormationManager::CalculateCohesionLevel()
{
    if (_members.empty())
        return 0.0f;

    float totalDistance = 0.0f;
    uint32 validMembers = 0;

    for (const FormationMember& member : _members)
    {
        if (member.player && member.player->IsInWorld())
        {
            totalDistance += member.distanceFromAssigned;
            validMembers++;
        }
    }

    if (validMembers == 0)
        return 0.0f;

    float averageDistance = totalDistance / validMembers;
    float cohesionLevel = 1.0f - (averageDistance / _cohesionRadius);

    return ::std::max(0.0f, ::std::min(1.0f, cohesionLevel));
}

FormationRole FormationManager::DeterminePlayerRole(Player* player)
{
    if (!player)
        return FormationRole::SUPPORT;

    uint8 playerClass = player->GetClass();
    ChrSpecialization spec = player->GetPrimarySpecialization();

    switch (playerClass)
    {
        case CLASS_WARRIOR:
            return (spec == ChrSpecialization::WarriorProtection) ? FormationRole::TANK : FormationRole::MELEE_DPS;

        case CLASS_PALADIN:
            if (spec == ChrSpecialization::PaladinProtection)
                return FormationRole::TANK;
            else if (spec == ChrSpecialization::PaladinHoly)
                return FormationRole::HEALER;
            else
                return FormationRole::MELEE_DPS;

        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return FormationRole::RANGED_DPS;

        case CLASS_ROGUE:
            return FormationRole::MELEE_DPS;

        case CLASS_DEATH_KNIGHT:
            return (spec == ChrSpecialization::DeathKnightBlood) ? FormationRole::TANK : FormationRole::MELEE_DPS;

        case CLASS_DEMON_HUNTER:
            return (spec == ChrSpecialization::DemonHunterVengeance) ? FormationRole::TANK : FormationRole::MELEE_DPS;

        case CLASS_PRIEST:
            return (spec == ChrSpecialization::PriestShadow) ? FormationRole::RANGED_DPS : FormationRole::HEALER;

        case CLASS_SHAMAN:
            if (spec == ChrSpecialization::ShamanRestoration)
                return FormationRole::HEALER;
            else if (spec == ChrSpecialization::ShamanElemental)
                return FormationRole::RANGED_DPS;
            else
                return FormationRole::MELEE_DPS;

        case CLASS_DRUID:
            if (spec == ChrSpecialization::DruidGuardian)
                return FormationRole::TANK;
            else if (spec == ChrSpecialization::DruidRestoration)
                return FormationRole::HEALER;
            else if (spec == ChrSpecialization::DruidBalance)
                return FormationRole::RANGED_DPS;
            else
                return FormationRole::MELEE_DPS;

        case CLASS_MONK:
            if (spec == ChrSpecialization::MonkBrewmaster)
                return FormationRole::TANK;
            else if (spec == ChrSpecialization::MonkMistweaver)
                return FormationRole::HEALER;
            else
                return FormationRole::MELEE_DPS;

        case CLASS_EVOKER:
            return (spec == ChrSpecialization::EvokerPreservation) ? FormationRole::HEALER : FormationRole::RANGED_DPS;

        default:
            return FormationRole::SUPPORT;
    }
}

void FormationManager::InitializeFormationConfigs()
{
    FormationConfig dungeonConfig;
    dungeonConfig.type = MovementFormationType::DUNGEON;
    dungeonConfig.baseSpacing = 4.0f;
    dungeonConfig.cohesionRadius = 12.0f;
    dungeonConfig.combatFormation = true;
    dungeonConfig.roleOrder = { FormationRole::TANK, FormationRole::MELEE_DPS, FormationRole::RANGED_DPS, FormationRole::HEALER };
    _formationConfigs[MovementFormationType::DUNGEON] = dungeonConfig;

    FormationConfig lineConfig;
    lineConfig.type = MovementFormationType::LINE;
    lineConfig.baseSpacing = 3.0f;
    lineConfig.cohesionRadius = 10.0f;
    lineConfig.maintainOrientation = true;
    _formationConfigs[MovementFormationType::LINE] = lineConfig;

    FormationConfig circleConfig;
    circleConfig.type = MovementFormationType::CIRCLE;
    circleConfig.baseSpacing = 5.0f;
    circleConfig.cohesionRadius = 8.0f;
    circleConfig.combatFormation = true;
    _formationConfigs[MovementFormationType::CIRCLE] = circleConfig;
}

void FormationManager::TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation)
{
    if (duration > _metrics.maxFormationTime)
        _metrics.maxFormationTime = duration;

    auto currentTime = ::std::chrono::steady_clock::now();
    auto timeSinceLastUpdate = ::std::chrono::duration_cast<::std::chrono::seconds>(currentTime - _metrics.lastUpdate);

    if (timeSinceLastUpdate.count() >= 1)
    {
        _metrics.averageFormationTime = ::std::chrono::microseconds(
            static_cast<uint64_t>(_metrics.averageFormationTime.count() * 0.9 + duration.count() * 0.1)
        );
        _metrics.lastUpdate = currentTime;
    }
}

// FormationUtils implementation
MovementFormationType FormationUtils::GetOptimalFormationForGroup(const ::std::vector<Player*>& members)
{
    if (members.size() <= 2)
        return MovementFormationType::COLUMN;
    else if (members.size() <= 5)
        return MovementFormationType::DUNGEON;
    else if (members.size() <= 10)
        return MovementFormationType::WEDGE;
    else
        return MovementFormationType::RAID;
}

MovementFormationType FormationUtils::GetOptimalFormationForCombat(const ::std::vector<Player*>& members, const ::std::vector<Unit*>& enemies)
{
    if (enemies.size() == 1)
        return MovementFormationType::CIRCLE;
    else if (enemies.size() <= 3)
        return MovementFormationType::LINE;
    else
        return MovementFormationType::BOX;
}

FormationRole FormationUtils::DetermineOptimalRole(Player* player)
{
    if (!player)
        return FormationRole::SUPPORT;

    uint8 playerClass = player->GetClass();
    switch (playerClass)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
            return FormationRole::TANK;

        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        case CLASS_EVOKER:
            return FormationRole::HEALER;

        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return FormationRole::RANGED_DPS;

        case CLASS_ROGUE:
            return FormationRole::MELEE_DPS;

        default:
            return FormationRole::SUPPORT;
    }
}

bool FormationUtils::IsFormationValid(const ::std::vector<Position>& positions, MovementFormationType formation)
{
    if (positions.empty())
        return false;

    for (size_t i = 1; i < positions.size(); ++i)
    {
        float distance = positions[0].GetExactDist(&positions[i]);
        if (distance > 30.0f)
            return false;
    }

    return true;
}

Position FormationUtils::CalculateFormationCenterFromMembers(const ::std::vector<Player*>& members)
{
    if (members.empty())
        return Position();

    float totalX = 0.0f, totalY = 0.0f, totalZ = 0.0f;

    for (Player* member : members)
    {
        if (member)
        {
            totalX += member->GetPositionX();
            totalY += member->GetPositionY();
            totalZ += member->GetPositionZ();
        }
    }

    Position center;
    center.m_positionX = totalX / members.size();
    center.m_positionY = totalY / members.size();
    center.m_positionZ = totalZ / members.size();

    return center;
}

// ============================================================================
// ADDITIONAL FORMATION PATTERNS (Ported from GroupFormationManager)
// ============================================================================

std::vector<Position> FormationManager::CalculateDiamondFormation(const Position& leaderPos, float orientation)
{
    std::vector<Position> positions;
    positions.reserve(_members.size());

    // Diamond formation: tank front, DPS sides, healer rear center
    // 4 cardinal points + fill interior
    if (_members.empty())
        return positions;

    float spacing = _formationSpacing * 2.0f;

    // Position 0: Leader at center
    positions.push_back(leaderPos);

    if (_members.size() == 1)
        return positions;

    // Position 1: Front (North) - tank position
    Position frontPos;
    frontPos.m_positionX = leaderPos.GetPositionX() + spacing * std::sin(orientation);
    frontPos.m_positionY = leaderPos.GetPositionY() + spacing * std::cos(orientation);
    frontPos.m_positionZ = leaderPos.GetPositionZ();
    positions.push_back(frontPos);

    if (_members.size() == 2)
        return positions;

    // Position 2: Rear (South) - healer position
    Position rearPos;
    rearPos.m_positionX = leaderPos.GetPositionX() - spacing * std::sin(orientation);
    rearPos.m_positionY = leaderPos.GetPositionY() - spacing * std::cos(orientation);
    rearPos.m_positionZ = leaderPos.GetPositionZ();
    positions.push_back(rearPos);

    if (_members.size() == 3)
        return positions;

    // Position 3: Left (West) - DPS position
    float leftAngle = orientation - static_cast<float>(M_PI) / 2.0f;
    Position leftPos;
    leftPos.m_positionX = leaderPos.GetPositionX() + spacing * std::sin(leftAngle);
    leftPos.m_positionY = leaderPos.GetPositionY() + spacing * std::cos(leftAngle);
    leftPos.m_positionZ = leaderPos.GetPositionZ();
    positions.push_back(leftPos);

    if (_members.size() == 4)
        return positions;

    // Position 4: Right (East) - DPS position
    float rightAngle = orientation + static_cast<float>(M_PI) / 2.0f;
    Position rightPos;
    rightPos.m_positionX = leaderPos.GetPositionX() + spacing * std::sin(rightAngle);
    rightPos.m_positionY = leaderPos.GetPositionY() + spacing * std::cos(rightAngle);
    rightPos.m_positionZ = leaderPos.GetPositionZ();
    positions.push_back(rightPos);

    // Fill interior diamond with remaining members
    float innerRadius = spacing * 0.75f;
    size_t remainingMembers = _members.size() - 5;

    for (size_t i = 0; i < remainingMembers; ++i)
    {
        float angle = orientation + (i / static_cast<float>(remainingMembers)) * 2.0f * static_cast<float>(M_PI);

        Position pos;
        pos.m_positionX = leaderPos.GetPositionX() + innerRadius * std::sin(angle);
        pos.m_positionY = leaderPos.GetPositionY() + innerRadius * std::cos(angle);
        pos.m_positionZ = leaderPos.GetPositionZ();
        positions.push_back(pos);
    }

    return positions;
}

std::vector<Position> FormationManager::CalculateBoxFormation(const Position& leaderPos, float orientation)
{
    std::vector<Position> positions;
    positions.reserve(_members.size());

    // Box formation (defensive square): healers center, tanks corners, DPS edges
    if (_members.empty())
        return positions;

    float halfSize = _formationSpacing * 2.0f;

    // Position 0: Leader at center
    positions.push_back(leaderPos);

    if (_members.size() == 1)
        return positions;

    // Calculate corner positions with orientation
    float corners[4][2] = {
        {-halfSize, halfSize},   // NW
        {halfSize, halfSize},    // NE
        {-halfSize, -halfSize},  // SW
        {halfSize, -halfSize}    // SE
    };

    // Add corners (tanks)
    for (size_t i = 1; i < _members.size() && i <= 4; ++i)
    {
        float offsetX = corners[i-1][0];
        float offsetY = corners[i-1][1];

        // Rotate by orientation
        float rotatedX = offsetX * std::cos(orientation) - offsetY * std::sin(orientation);
        float rotatedY = offsetX * std::sin(orientation) + offsetY * std::cos(orientation);

        Position pos;
        pos.m_positionX = leaderPos.GetPositionX() + rotatedX;
        pos.m_positionY = leaderPos.GetPositionY() + rotatedY;
        pos.m_positionZ = leaderPos.GetPositionZ();
        positions.push_back(pos);
    }

    if (_members.size() <= 5)
        return positions;

    // Add edge positions (DPS) - distributed along the 4 edges
    size_t remainingMembers = _members.size() - 5;
    size_t botsPerEdge = (remainingMembers + 3) / 4;  // Distribute evenly

    float edges[4][4] = {
        // Start X, Start Y, End X, End Y
        {-halfSize, halfSize, halfSize, halfSize},     // North edge
        {halfSize, halfSize, halfSize, -halfSize},     // East edge
        {halfSize, -halfSize, -halfSize, -halfSize},   // South edge
        {-halfSize, -halfSize, -halfSize, halfSize}    // West edge
    };

    size_t memberIdx = 0;
    for (size_t edge = 0; edge < 4 && memberIdx < remainingMembers; ++edge)
    {
        for (size_t i = 0; i < botsPerEdge && memberIdx < remainingMembers; ++i)
        {
            float t = (i + 1) / static_cast<float>(botsPerEdge + 1);
            float offsetX = edges[edge][0] + t * (edges[edge][2] - edges[edge][0]);
            float offsetY = edges[edge][1] + t * (edges[edge][3] - edges[edge][1]);

            // Rotate by orientation
            float rotatedX = offsetX * std::cos(orientation) - offsetY * std::sin(orientation);
            float rotatedY = offsetX * std::sin(orientation) + offsetY * std::cos(orientation);

            Position pos;
            pos.m_positionX = leaderPos.GetPositionX() + rotatedX;
            pos.m_positionY = leaderPos.GetPositionY() + rotatedY;
            pos.m_positionZ = leaderPos.GetPositionZ();
            positions.push_back(pos);
            ++memberIdx;
        }
    }

    return positions;
}

std::vector<Position> FormationManager::CalculateRaidFormation(const Position& leaderPos, float orientation)
{
    std::vector<Position> positions;
    positions.reserve(_members.size());

    // Raid formation: Similar to dungeon but with wider spacing and more structured rows
    // Uses 5-person groups arranged in a grid pattern
    if (_members.empty())
        return positions;

    float spacing = _formationSpacing * 1.5f;

    // Leader at front center
    positions.push_back(leaderPos);

    if (_members.size() == 1)
        return positions;

    // Arrange in groups of 5 (standard WoW raid group size)
    size_t remainingMembers = _members.size() - 1;
    size_t numGroups = (remainingMembers + 4) / 5;

    for (size_t group = 0; group < numGroups; ++group)
    {
        size_t membersInGroup = std::min(size_t(5), remainingMembers - (group * 5));

        for (size_t member = 0; member < membersInGroup; ++member)
        {
            // Calculate row and column position
            float row = static_cast<float>(group);
            float col = static_cast<float>(member) - 2.0f;  // Center around 0 (-2, -1, 0, 1, 2)

            float offsetX = col * spacing;
            float offsetY = -row * spacing;  // Negative = behind leader

            // Rotate by orientation
            float rotatedX = offsetX * std::cos(orientation) - offsetY * std::sin(orientation);
            float rotatedY = offsetX * std::sin(orientation) + offsetY * std::cos(orientation);

            Position pos;
            pos.m_positionX = leaderPos.GetPositionX() + rotatedX;
            pos.m_positionY = leaderPos.GetPositionY() + rotatedY;
            pos.m_positionZ = leaderPos.GetPositionZ();
            positions.push_back(pos);
        }
    }

    return positions;
}

void FormationManager::CalculateMovementTargets()
{
    if (!_isLeader || !_leader || _members.empty())
        return;

    // Calculate target positions for each member based on current formation
    Position leaderPos = _leader->GetPosition();
    float orientation = _formationOrientation;

    std::vector<Position> targetPositions;
    switch (_currentFormation)
    {
        case MovementFormationType::LINE:
            targetPositions = CalculateLineFormation(leaderPos, orientation);
            break;
        case MovementFormationType::WEDGE:
            targetPositions = CalculateWedgeFormation(leaderPos, orientation);
            break;
        case MovementFormationType::COLUMN:
            targetPositions = CalculateColumnFormation(leaderPos, orientation);
            break;
        case MovementFormationType::DIAMOND:
            targetPositions = CalculateDiamondFormation(leaderPos, orientation);
            break;
        case MovementFormationType::CIRCLE:
            targetPositions = CalculateCircleFormation(leaderPos);
            break;
        case MovementFormationType::SPREAD:
        case MovementFormationType::BOX:
            targetPositions = CalculateBoxFormation(leaderPos, orientation);
            break;
        case MovementFormationType::DUNGEON:
            targetPositions = CalculateDungeonFormation(leaderPos, orientation);
            break;
        case MovementFormationType::RAID:
            targetPositions = CalculateRaidFormation(leaderPos, orientation);
            break;
        default:
            return;
    }

    // Update member target positions
    for (size_t i = 0; i < _members.size() && i < targetPositions.size(); ++i)
    {
        _members[i].targetPosition = targetPositions[i];
    }
}

void FormationManager::IssueMovementCommands()
{
    if (!_bot || _members.empty())
        return;

    // Issue movement commands to members to reach their target positions
    for (auto& member : _members)
    {
        if (!member.player || !member.player->IsInWorld())
            continue;

        // Check if member needs to move to target position
        float distanceToTarget = member.player->GetExactDist(&member.targetPosition);
        if (distanceToTarget > _formationSpacing * 0.5f)
        {
            // Member is too far from target position, issue movement command
            if (BotAI* ai = GetBotAI(member.player))
            {
                if (!ai->MoveTo(member.targetPosition, true))
                {
                    // Fallback to legacy if validation fails
                    member.player->GetMotionMaster()->MovePoint(0, member.targetPosition);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                member.player->GetMotionMaster()->MovePoint(0, member.targetPosition);
            }
            member.isMoving = true;
            member.isInPosition = false;
        }
        else
        {
            member.isMoving = false;
            member.isInPosition = true;
        }
    }
}

void FormationManager::AdjustForTerrain()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    Map* map = _bot->GetMap();
    if (!map)
        return;

    // Adjust member positions based on terrain height and obstacles
    for (auto& member : _members)
    {
        if (!member.player || !member.player->IsInWorld())
            continue;

        // Get terrain height at target position
        float targetZ = map->GetHeight(_bot->GetPhaseShift(),
            member.targetPosition.GetPositionX(),
            member.targetPosition.GetPositionY(),
            member.targetPosition.GetPositionZ() + 2.0f,
            true,
            50.0f);

        // Adjust Z coordinate to terrain height
        if (targetZ > INVALID_HEIGHT)
        {
            member.targetPosition.m_positionZ = targetZ;
        }

        // Check for water and adjust if needed
        if (map->IsInWater(_bot->GetPhaseShift(),
            member.targetPosition.GetPositionX(),
            member.targetPosition.GetPositionY(),
            member.targetPosition.GetPositionZ()))
        {
            // If in water, try to find nearby land position
            float landZ = map->GetHeight(_bot->GetPhaseShift(),
                member.targetPosition.GetPositionX(),
                member.targetPosition.GetPositionY(),
                member.targetPosition.GetPositionZ() + 5.0f,
                false);

            if (landZ > INVALID_HEIGHT && landZ > member.targetPosition.GetPositionZ())
            {
                member.targetPosition.m_positionZ = landZ;
            }
        }
    }
}

void FormationManager::AdjustForGroupSize()
{
    if (_members.empty())
        return;

    // Adjust formation spacing based on group size
    uint32 memberCount = static_cast<uint32>(_members.size());

    if (memberCount <= 2)
    {
        // Small group: tighter formation
        _formationSpacing = DEFAULT_FORMATION_SPACING * 0.8f;
    }
    else if (memberCount <= 5)
    {
        // Normal dungeon group: standard spacing
        _formationSpacing = DEFAULT_FORMATION_SPACING;
    }
    else if (memberCount <= 10)
    {
        // Large group: wider spacing
        _formationSpacing = DEFAULT_FORMATION_SPACING * 1.2f;
    }
    else
    {
        // Raid group: much wider spacing
        _formationSpacing = DEFAULT_FORMATION_SPACING * 1.5f;
    }

    // Adjust cohesion radius proportionally
    _cohesionRadius = _formationSpacing * 3.0f;
}

void FormationManager::MaintainFormationDuringMovement()
{
    if (!_isLeader || !_leader || _members.empty())
        return;

    // Check if leader is moving
    if (!_leader->isMoving())
    {
        _movementState = FormationMovementState::STATIONARY;
        return;
    }

    _movementState = FormationMovementState::MOVING;

    // Recalculate formation orientation based on leader's movement direction
    float leaderOrientation = _leader->GetOrientation();
    if (std::abs(_formationOrientation - leaderOrientation) > 0.1f)
    {
        _formationOrientation = leaderOrientation;
        CalculateMovementTargets();
    }

    // Check formation integrity during movement
    uint32 membersOutOfPosition = 0;
    for (const auto& member : _members)
    {
        if (!member.player || !member.player->IsInWorld())
            continue;

        float distanceToLeader = member.player->GetExactDist(_leader);
        if (distanceToLeader > _cohesionRadius)
        {
            ++membersOutOfPosition;
        }
    }

    // If too many members are out of position, trigger reformation
    float outOfPositionRatio = static_cast<float>(membersOutOfPosition) / static_cast<float>(_members.size());
    if (outOfPositionRatio > _reformationThreshold)
    {
        _movementState = FormationMovementState::REFORMING;
        _currentIntegrity = FormationIntegrity::BROKEN;
        _lastReformation = GameTime::GetGameTimeMS();

        // Issue new movement commands to bring members back
        IssueMovementCommands();
    }
    else if (outOfPositionRatio > (_reformationThreshold * 0.5f))
    {
        _currentIntegrity = FormationIntegrity::ACCEPTABLE;
    }
    else
    {
        _currentIntegrity = FormationIntegrity::GOOD;
    }
}

void FormationManager::ActivateEmergencyScatter()
{
    if (!_isLeader || !_leader || _members.empty())
        return;

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::ActivateEmergencyScatter: Activating emergency scatter for leader {}",
                 _leader->GetName());

    // Set scatter mode
    _currentFormation = MovementFormationType::SPREAD;
    _emergencyScatter = true;
    _lastReformation = GameTime::GetGameTimeMS(); // Track when scatter started

    // Calculate scatter positions in all directions from leader
    Position leaderPos = _leader->GetPosition();
    float scatterDistance = _formationSpacing * 3.0f; // Scatter far from center

    for (size_t i = 0; i < _members.size(); ++i)
    {
        if (!_members[i].player)
            continue;

        // Calculate angle based on member index to spread evenly
        float angle = (2.0f * static_cast<float>(M_PI) * i) / _members.size();
        float x = leaderPos.GetPositionX() + scatterDistance * std::cos(angle);
        float y = leaderPos.GetPositionY() + scatterDistance * std::sin(angle);
        float z = leaderPos.GetPositionZ();

        // Adjust for terrain
        Map* map = _leader->GetMap();
        if (map)
        {
            float groundZ = map->GetHeight(_leader->GetPhaseShift(), x, y, z + 2.0f);
            if (groundZ > INVALID_HEIGHT)
                z = groundZ;
        }

        _members[i].targetPosition = Position(x, y, z);
        if (BotAI* ai = GetBotAI(_members[i].player))
        {
            if (!ai->MoveTo(_members[i].targetPosition, true))
            {
                // Fallback to legacy if validation fails
                _members[i].player->GetMotionMaster()->MovePoint(0, _members[i].targetPosition);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            _members[i].player->GetMotionMaster()->MovePoint(0, _members[i].targetPosition);
        }
        _members[i].isMoving = true;
        _members[i].isInPosition = false;
    }

    _currentIntegrity = FormationIntegrity::BROKEN;
}

void FormationManager::DeactivateEmergencyScatter()
{
    if (!_isLeader || !_leader)
        return;

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::DeactivateEmergencyScatter: Deactivating emergency scatter for leader {}",
                 _leader->GetName());

    _emergencyScatter = false;

    // Reform to previous formation type
    CalculateMovementTargets();
    IssueMovementCommands();
}

void FormationManager::HandleEmergencyRegroup(const Position& /* rallyPoint */)
{
    if (!_isLeader || !_leader || _members.empty())
        return;

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::HandleEmergencyRegroup: Emergency regroup for leader {}",
                 _leader->GetName());

    // Deactivate scatter if active
    if (_emergencyScatter)
        DeactivateEmergencyScatter();

    // Set tight formation
    float originalSpacing = _formationSpacing;
    _formationSpacing = DEFAULT_FORMATION_SPACING * 0.5f; // Tighter formation for regroup

    // Use column formation for easy regrouping
    MovementFormationType originalType = _currentFormation;
    _currentFormation = MovementFormationType::COLUMN;

    // Calculate tight positions around leader
    CalculateMovementTargets();

    // Issue urgent movement commands
    for (auto& member : _members)
    {
        if (!member.player || !member.player->IsAlive())
            continue;

        member.player->GetMotionMaster()->Clear();
        if (BotAI* ai = GetBotAI(member.player))
        {
            if (!ai->MoveTo(member.targetPosition, true))
            {
                // Fallback to legacy if validation fails
                member.player->GetMotionMaster()->MovePoint(0, member.targetPosition, true, {}, 7.0f); // Run speed
            }
        }
        else
        {
            // Non-bot player - use standard movement
            member.player->GetMotionMaster()->MovePoint(0, member.targetPosition, true, {}, 7.0f); // Run speed
        }
        member.isMoving = true;
        member.isInPosition = false;
    }

    // Restore original settings after movement
    _formationSpacing = originalSpacing;
    _currentFormation = originalType;
    _lastReformation = GameTime::GetGameTimeMS();
    _currentIntegrity = FormationIntegrity::BROKEN;
}

void FormationManager::HandleMemberDisconnection(Player* disconnectedMember)
{
    if (!disconnectedMember)
        return;

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::HandleMemberDisconnection: Removing {} from formation",
                 disconnectedMember->GetName());

    // Find and remove the member
    auto it = std::find_if(_members.begin(), _members.end(),
        [disconnectedMember](const FormationMember& member) {
            return member.player == disconnectedMember;
        });

    if (it != _members.end())
    {
        _members.erase(it);

        // Adjust formation for new group size
        if (!_members.empty())
        {
            AdjustForGroupSize();
            CalculateMovementTargets();
            IssueMovementCommands();
        }
        else
        {
            // No members left, clear formation
            _currentFormation = MovementFormationType::NONE;
            _currentIntegrity = FormationIntegrity::BROKEN;
        }
    }
}

bool FormationManager::SetFormationLeader(Player* leader)
{
    if (!leader)
        return false;

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::SetFormationLeader: Setting leader to {} for bot {}",
                 leader->GetName(), _bot ? _bot->GetName() : "null");

    _leader = leader;
    _isLeader = (_bot == leader);

    if (_inFormation && !_members.empty())
    {
        AssignFormationPositions();
        CalculateMovementTargets();
    }

    return true;
}

bool FormationManager::AdjustFormationForCombat(const ::std::vector<Unit*>& threats)
{
    if (!_bot || threats.empty())
        return false;

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::AdjustFormationForCombat: Adjusting for {} threats",
                 threats.size());

    TransitionToCombatFormation(threats);
    AdjustForThreatSpread(threats);

    return true;
}

::std::vector<Player*> FormationManager::GetOutOfPositionMembers(float tolerance)
{
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    ::std::vector<Player*> outOfPosition;

    for (const auto& member : _members)
    {
        if (!member.player || !member.player->IsAlive())
            continue;

        float distance = member.player->GetExactDist(&member.assignedPosition);
        if (distance > tolerance)
        {
            outOfPosition.push_back(member.player);
        }
    }

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::GetOutOfPositionMembers: {} members out of position (tolerance: {})",
                 outOfPosition.size(), tolerance);

    return outOfPosition;
}

void FormationManager::TransitionToTravelFormation()
{
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::TransitionToTravelFormation: Transitioning to travel formation");

    _currentFormation = MovementFormationType::COLUMN;
    _movementState = FormationMovementState::MOVING;
    _formationSpacing = DEFAULT_FORMATION_SPACING * 1.2f;

    AssignFormationPositions();
    CalculateMovementTargets();
    IssueMovementCommands();

    _metrics.formationChanges++;
}

void FormationManager::AdjustForThreatSpread(const ::std::vector<Unit*>& threats)
{
    if (threats.empty())
        return;

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::AdjustForThreatSpread: Adjusting for {} threats",
                 threats.size());

    Position threatCenter;
    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    uint32 validThreats = 0;

    for (const auto* threat : threats)
    {
        if (threat && threat->IsAlive())
        {
            sumX += threat->GetPositionX();
            sumY += threat->GetPositionY();
            sumZ += threat->GetPositionZ();
            validThreats++;
        }
    }

    if (validThreats == 0)
        return;

    threatCenter.Relocate(sumX / validThreats, sumY / validThreats, sumZ / validThreats);

    if (_leader)
    {
        _formationOrientation = _leader->GetAbsoluteAngle(&threatCenter);
    }

    if (validThreats > 3)
    {
        _formationSpacing = DEFAULT_FORMATION_SPACING * 1.5f;
    }

    AssignFormationPositions();
    CalculateMovementTargets();
}

void FormationManager::HandleFormationBreakage()
{
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::HandleFormationBreakage: Handling formation breakage");

    _currentIntegrity = FormationIntegrity::BROKEN;
    _metrics.cohesionBreaks++;

    if (_leader && _leader->IsAlive())
    {
        Position leaderPos = _leader->GetPosition();

        for (auto& member : _members)
        {
            if (!member.player || !member.player->IsAlive())
                continue;

            member.targetPosition = leaderPos;
            member.isInPosition = false;
            if (BotAI* ai = GetBotAI(member.player))
            {
                if (!ai->MoveTo(leaderPos, true))
                {
                    // Fallback to legacy if validation fails
                    member.player->GetMotionMaster()->MovePoint(0, leaderPos, true, {}, 7.0f);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                member.player->GetMotionMaster()->MovePoint(0, leaderPos, true, {}, 7.0f);
            }
            member.isMoving = true;
        }
    }

    _lastReformation = GameTime::GetGameTimeMS();
}

MovementFormationType FormationManager::DetermineOptimalFormation(const ::std::vector<Player*>& members)
{
    if (members.empty())
        return MovementFormationType::NONE;

    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    uint32 memberCount = static_cast<uint32>(members.size());

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::DetermineOptimalFormation: Determining for {} members",
                 memberCount);

    uint32 tanks = 0, healers = 0, melee = 0, ranged = 0;

    for (const auto* player : members)
    {
        if (!player)
            continue;

        FormationRole role = DeterminePlayerRole(const_cast<Player*>(player));
        switch (role)
        {
            case FormationRole::TANK: tanks++; break;
            case FormationRole::HEALER: healers++; break;
            case FormationRole::MELEE_DPS: melee++; break;
            case FormationRole::RANGED_DPS: ranged++; break;
            default: break;
        }
    }

    if (memberCount <= 5)
    {
        if (tanks >= 1 && healers >= 1)
            return MovementFormationType::DUNGEON;
        return MovementFormationType::WEDGE;
    }
    else if (memberCount <= 10)
    {
        if (ranged > melee)
            return MovementFormationType::LINE;
        return MovementFormationType::BOX;
    }
    else
    {
        return MovementFormationType::RAID;
    }
}

FormationConfig FormationManager::GetFormationConfig(MovementFormationType formation)
{
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    auto it = _formationConfigs.find(formation);
    if (it != _formationConfigs.end())
    {
        return it->second;
    }

    FormationConfig defaultConfig;
    defaultConfig.type = formation;
    defaultConfig.baseSpacing = DEFAULT_FORMATION_SPACING;
    defaultConfig.cohesionRadius = DEFAULT_COHESION_RADIUS;
    defaultConfig.reformationThreshold = DEFAULT_REFORMATION_THRESHOLD;
    return defaultConfig;
}

void FormationManager::SetFormationConfig(MovementFormationType formation, const FormationConfig& config)
{
    std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_mutex);

    TC_LOG_DEBUG("playerbot.formation", "FormationManager::SetFormationConfig: Setting config for formation type {}",
                 static_cast<uint8>(formation));

    _formationConfigs[formation] = config;

    if (_currentFormation == formation && _inFormation)
    {
        _formationSpacing = config.baseSpacing;
        _cohesionRadius = config.cohesionRadius;
        _reformationThreshold = config.reformationThreshold;

        AssignFormationPositions();
        CalculateMovementTargets();
    }
}

} // namespace Playerbot
