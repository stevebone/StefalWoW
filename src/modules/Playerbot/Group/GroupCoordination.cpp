/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GroupCoordination.h"
#include "GameTime.h"
#include "Player.h"
#include "Group.h"
#include "Unit.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include <algorithm>

namespace Playerbot
{

GroupCoordination::GroupCoordination(uint32 groupId)
    : _groupId(groupId)
    , _currentPhase(EncounterPhase::PREPARATION)
    , _overallThreat(ThreatLevel::NONE)
    , _maintainFormationDuringMove(true)
{
    _formationCenter = Position();
    _currentDestination = Position();
    _combatStartTime = ::std::chrono::steady_clock::now();
    _metrics.Reset();

    TC_LOG_DEBUG("playerbot", "GroupCoordination: Created coordination for group %u", groupId);
}

void GroupCoordination::ExecuteCommand(CoordinationCommand command, const ::std::vector<uint32>& targets)
{
    ::std::lock_guard lock(_commandMutex);

    CoordinationCommandData commandData(command, targets, 0, 100);

    if (ValidateCommand(commandData))
    {
        ExecuteCommandInternal(commandData);
        _metrics.commandsExecuted.fetch_add(1);
    }

    TC_LOG_DEBUG("playerbot", "GroupCoordination: Executed command %u for group %u",
                 static_cast<uint8>(command), _groupId);
}

void GroupCoordination::IssueCommand(uint32 memberGuid, CoordinationCommand command, const ::std::vector<uint32>& targets)
{
    ::std::lock_guard lock(_commandMutex);

    if (_commandQueue.size() >= MAX_COMMAND_QUEUE_SIZE)
    {
        TC_LOG_WARN("playerbot", "GroupCoordination: Command queue full for group %u", _groupId);
        return;
    }

    CoordinationCommandData commandData(command, targets, memberGuid);
    _commandQueue.push(commandData);
    _metrics.commandsIssued.fetch_add(1);
}

void GroupCoordination::BroadcastCommand(CoordinationCommand command, const ::std::vector<uint32>& targets)
{
    // Execute command for all group members
    if (Group* group = sGroupMgr->GetGroupByGUID(_groupId))
    {
        for (GroupReference const& itr : group->GetMembers())
        {
            if (Player* member = itr.GetSource())
            {
                IssueCommand(member->GetGUID().GetCounter(), command, targets);
            }
        }
    }
}

void GroupCoordination::SetPrimaryTarget(ObjectGuid targetGuid, uint32 priority)
{
    ::std::lock_guard lock(_targetMutex);

    _primaryTarget = targetGuid;

    // Add or update target in coordination system
    auto it = _targets.find(targetGuid);
    if (it != _targets.end())
    {
        it->second.priority = priority;
    }
    else
    {
        _targets.emplace(targetGuid, CoordinationTarget(targetGuid, priority, ThreatLevel::HIGH));
    }

    TC_LOG_DEBUG("playerbot", "GroupCoordination: Set primary target for group %u", _groupId);
}

void GroupCoordination::AddSecondaryTarget(ObjectGuid targetGuid, uint32 priority)
{
    ::std::lock_guard lock(_targetMutex);

    _targets.emplace(targetGuid, CoordinationTarget(targetGuid, priority, ThreatLevel::MEDIUM));
}

void GroupCoordination::RemoveTarget(ObjectGuid targetGuid)
{
    ::std::lock_guard lock(_targetMutex);

    _targets.erase(targetGuid);

    if (_primaryTarget == targetGuid)
    {
        _primaryTarget = ObjectGuid::Empty;

        // Find new primary target with highest priority
        uint32 highestPriority = 0;
        for (const auto& [guid, target] : _targets)
        {
            if (target.priority > highestPriority)
            {
                highestPriority = target.priority;
                _primaryTarget = guid;
            }
        }
    }
}

ObjectGuid GroupCoordination::GetPrimaryTarget() const
{
    ::std::lock_guard lock(_targetMutex);
    return _primaryTarget;
}

::std::vector<ObjectGuid> GroupCoordination::GetTargetPriorityList() const
{
    ::std::lock_guard lock(_targetMutex);

    ::std::vector<::std::pair<ObjectGuid, uint32>> targetPairs;
    for (const auto& [guid, target] : _targets)
    {
        targetPairs.emplace_back(guid, target.priority);
    }

    // Sort by priority (highest first)
    ::std::sort(targetPairs.begin(), targetPairs.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    ::std::vector<ObjectGuid> priorityList;
    for (const auto& pair : targetPairs)
    {
        priorityList.push_back(pair.first);
    }

    return priorityList;
}

void GroupCoordination::SetFormation(const ::std::vector<FormationSlot>& formation)
{
    ::std::lock_guard lock(_formationMutex);
    _formation = formation;
}

void GroupCoordination::UpdateFormation(const Position& leaderPosition)
{
    ::std::lock_guard lock(_formationMutex);
    _formationCenter = leaderPosition;
    UpdateFormationPositions();
}

Position GroupCoordination::GetFormationPosition(uint32 memberGuid) const
{
    ::std::lock_guard lock(_formationMutex);

    for (const auto& slot : _formation)
    {
        if (slot.memberGuid == memberGuid)
        {
            // Transform relative position to world position
            return Position(_formationCenter.GetPositionX() + slot.relativePosition.GetPositionX(),
                           _formationCenter.GetPositionY() + slot.relativePosition.GetPositionY(),
                           _formationCenter.GetPositionZ() + slot.relativePosition.GetPositionZ());
        }
    }

    return _formationCenter; // Default to formation center
}

bool GroupCoordination::IsInFormation(uint32 memberGuid, float tolerance) const
{
    Position assignedPos = GetFormationPosition(memberGuid);

    if (Player* player = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(memberGuid)))
    {
        float distance = assignedPos.GetExactDist(player->GetPosition());
        return distance <= tolerance;
    }

    return false;
}

void GroupCoordination::MoveToPosition(const Position& destination, bool maintainFormation)
{
    ::std::lock_guard lock(_movementMutex);

    _currentDestination = destination;
    _maintainFormationDuringMove = maintainFormation;

    // Create movement waypoint
    MovementWaypoint waypoint(destination, 0.0f, true, "Group movement destination");

    // Clear current path and add new destination
    while (!_movementPath.empty())
        _movementPath.pop();

    _movementPath.push(waypoint);
}

void GroupCoordination::FollowLeader(uint32 leaderGuid, float distance)
{
    if (Player* leader = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(leaderGuid)))
    {
        Position leaderPos = leader->GetPosition();
        UpdateFormation(leaderPos);
    }
}

Position GroupCoordination::GetNextWaypoint() const
{
    ::std::lock_guard lock(_movementMutex);

    if (!_movementPath.empty())
        return _movementPath.front().position;

    return Position();
}

bool GroupCoordination::HasReachedDestination() const
{
    ::std::lock_guard lock(_movementMutex);

    if (_movementPath.empty())
        return true;

    const Position& destination = _movementPath.front().position;

    // Check if formation center is close to destination
    float distance = _formationCenter.GetExactDist(destination);
    return distance <= WAYPOINT_REACH_DISTANCE;
}

void GroupCoordination::InitiateCombat(Unit* target)
{
    if (!target)
        return;

    _inCombat.store(true);
    _combatStartTime = ::std::chrono::steady_clock::now();
    _currentPhase = EncounterPhase::ENGAGE;

    SetPrimaryTarget(target->GetGUID(), 150);
    BroadcastCommand(CoordinationCommand::ATTACK_TARGET, {target->GetGUID().GetCounter()});

    TC_LOG_DEBUG("playerbot", "GroupCoordination: Initiated combat for group %u", _groupId);
}

void GroupCoordination::UpdateCombatCoordination()
{
    if (!_inCombat.load())
        return;

    UpdateTargetAssessment();
    UpdateCombatTactics();
    HandleTankThreatManagement();
    HandleHealerPriorities();
    HandleDPSTargeting();
    HandleSupportActions();
}

void GroupCoordination::EndCombat()
{
    _inCombat.store(false);
    _currentPhase = EncounterPhase::RECOVERY;
    _overallThreat = ThreatLevel::NONE;

    // Clear combat targets
    {
        ::std::lock_guard lock(_targetMutex);
        _targets.clear();
        _primaryTarget = ObjectGuid::Empty;
    }

    BroadcastCommand(CoordinationCommand::DEFENSIVE_MODE);

    TC_LOG_DEBUG("playerbot", "GroupCoordination: Ended combat for group %u", _groupId);
}

void GroupCoordination::SetEncounterPhase(EncounterPhase phase)
{
    _currentPhase = phase;

    switch (phase)
    {
        case EncounterPhase::BURN:
            BroadcastCommand(CoordinationCommand::USE_COOLDOWNS);
            BroadcastCommand(CoordinationCommand::BURN_PHASE);
            break;
        case EncounterPhase::RECOVERY:
            BroadcastCommand(CoordinationCommand::DEFENSIVE_MODE);
            break;
        case EncounterPhase::TRANSITION:
            BroadcastCommand(CoordinationCommand::SAVE_COOLDOWNS);
            break;
        default:
            break;
    }
}

void GroupCoordination::HandleEmergencySituation(ThreatLevel level)
{
    _overallThreat = level;

    switch (level)
    {
        case ThreatLevel::CRITICAL:
            BroadcastCommand(CoordinationCommand::USE_COOLDOWNS);
            BroadcastCommand(CoordinationCommand::SPREAD_OUT);
            break;
        case ThreatLevel::HIGH:
            BroadcastCommand(CoordinationCommand::DEFENSIVE_MODE);
            break;
        case ThreatLevel::MEDIUM:
            // Normal coordination continues
            break;
        default:
            break;
    }
}

void GroupCoordination::Update(uint32 /*diff*/)
{
    if (!_isActive.load())
        return;

    ProcessCommandQueue();
    UpdateTargetAssessment();
    UpdateFormationPositions();
    UpdateMovementProgress();

    if (_inCombat.load())
    {
        UpdateCombatCoordination();
    }

    UpdateMetrics();
}

void GroupCoordination::UpdateMetrics()
{
    // Calculate response time
    auto now = ::std::chrono::steady_clock::now();
    auto timeSinceUpdate = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _metrics.lastUpdate);
    _metrics.responseTime.store(timeSinceUpdate.count());

    // Update formation compliance
    float compliance = AssessFormationCompliance();
    _metrics.formationCompliance.store(compliance);

    _metrics.lastUpdate = now;
}

void GroupCoordination::ProcessCommandQueue()
{
    ::std::lock_guard lock(_commandMutex);

    while (!_commandQueue.empty())
    {
        CoordinationCommandData command = _commandQueue.top();
        _commandQueue.pop();

        if (ValidateCommand(command))
        {
            ExecuteCommandInternal(command);
            _metrics.commandsExecuted.fetch_add(1);
        }
    }
}

void GroupCoordination::UpdateTargetAssessment()
{
    ::std::lock_guard lock(_targetMutex);

    // Update target information including TTK estimation
    for (auto& [guid, target] : _targets)
    {
        if (Unit* unit = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(_primaryTarget), guid))
        {
            target.lastKnownPosition = unit->GetPosition();
            target.lastSeen = GameTime::GetGameTimeMS();

            // Estimate TTK from group DPS on this target
            if (unit->IsAlive())
            {
                uint64 health = unit->GetHealth();
                // Count how many group members are attacking this target
                uint32 attackerCount = static_cast<uint32>(target.assignedMembers.size());
                if (attackerCount == 0)
                    attackerCount = 1;

                // Rough per-attacker DPS estimate based on target level
                float estimatedGroupDPS = static_cast<float>(attackerCount) * static_cast<float>(unit->GetLevel()) * 15.0f;
                target.estimatedTimeToKill = (estimatedGroupDPS > 0.0f)
                    ? static_cast<float>(health) / estimatedGroupDPS
                    : 0.0f;
            }
            else
            {
                target.estimatedTimeToKill = 0.0f;
            }
        }
    }

    // Remove stale targets
    auto it = _targets.begin();
    while (it != _targets.end())
    {
        if (GameTime::GetGameTimeMS() - it->second.lastSeen > 30000) // 30 seconds
        {
            it = _targets.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void GroupCoordination::UpdateFormationPositions()
{
    // Formation positions are updated when formation center changes
    // This is handled in UpdateFormation method
}

void GroupCoordination::UpdateMovementProgress()
{
    ::std::lock_guard lock(_movementMutex);

    if (!_movementPath.empty() && HasReachedDestination())
    {
        _movementPath.pop();

        if (!_movementPath.empty())
        {
            // Move to next waypoint
            const Position& nextPos = _movementPath.front().position;
            UpdateFormation(nextPos);
        }
    }
}

void GroupCoordination::UpdateCombatTactics()
{
    // Adapt tactics based on current phase and threat level
    switch (_currentPhase)
    {
        case EncounterPhase::ENGAGE:
            OptimizeTargetAssignments();
            break;
        case EncounterPhase::BURN:
            BroadcastCommand(CoordinationCommand::FOCUS_FIRE);
            break;
        case EncounterPhase::TRANSITION:
            BroadcastCommand(CoordinationCommand::SAVE_COOLDOWNS);
            break;
        default:
            break;
    }
}

float GroupCoordination::AssessFormationCompliance()
{
    ::std::lock_guard lock(_formationMutex);

    if (_formation.empty())
        return 1.0f;

    uint32 membersInPosition = 0;

    for (const auto& slot : _formation)
    {
        if (IsInFormation(slot.memberGuid, FORMATION_TOLERANCE))
            membersInPosition++;
    }

    return static_cast<float>(membersInPosition) / _formation.size();
}

void GroupCoordination::OptimizeTargetAssignments()
{
    ::std::lock_guard lock(_targetMutex);

    // Assign targets based on priority and member capabilities
    auto priorityList = GetTargetPriorityList();

    if (!priorityList.empty())
    {
        // Focus fire on primary target
        BroadcastCommand(CoordinationCommand::FOCUS_FIRE, {priorityList[0].GetCounter()});
    }
}

bool GroupCoordination::ValidateCommand(const CoordinationCommandData& command)
{
    // Check command timeout
    if (GameTime::GetGameTimeMS() - command.timestamp > COMMAND_TIMEOUT)
        return false;

    // Basic validation - can be extended with more sophisticated checks
    return true;
}

void GroupCoordination::ExecuteCommandInternal(const CoordinationCommandData& command)
{
    switch (command.command)
    {
        case CoordinationCommand::ATTACK_TARGET:
            if (!command.targets.empty())
            {
                ObjectGuid targetGuid = ObjectGuid::Create<HighGuid::Unit>(command.targets[0]);
                SetPrimaryTarget(targetGuid, 100);
            }
            break;

        case CoordinationCommand::FOCUS_FIRE:
            // All DPS focus on primary target
    if (!command.targets.empty())
            {
                ObjectGuid targetGuid = ObjectGuid::Create<HighGuid::Unit>(command.targets[0]);
                SetPrimaryTarget(targetGuid, 150);
            }
            break;

        case CoordinationCommand::SPREAD_OUT:
            // Increase formation spacing
            {
                ::std::lock_guard lock(_formationMutex);
                for (auto& slot : _formation)
                {
                    slot.maxDistance *= 1.5f; // Increase spacing
                }
            }
            break;

        case CoordinationCommand::STACK_UP:
            // Decrease formation spacing
            {
                ::std::lock_guard lock(_formationMutex);
                for (auto& slot : _formation)
                {
                    slot.maxDistance *= 0.7f; // Decrease spacing
                }
            }
            break;

        case CoordinationCommand::RETREAT:
            // Move away from current position
            {
                Position retreatPos = _formationCenter;
                retreatPos.SetOrientation(retreatPos.GetOrientation() + static_cast<float>(M_PI)); // Turn around
                retreatPos.m_positionX += 20.0f * ::std::cos(retreatPos.GetOrientation());
                retreatPos.m_positionY += 20.0f * ::std::sin(retreatPos.GetOrientation());
                MoveToPosition(retreatPos, true);
            }
            break;

        default:
            // Command handled by individual AI systems
            break;
    }
}

void GroupCoordination::HandleTankThreatManagement()
{
    Group* group = sGroupMgr->GetGroupByGUID(_groupId);
    if (!group)
        return;

    // Identify all tanks in the group
    ::std::vector<Player*> tanks;
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            ChrSpecialization spec = member->GetPrimarySpecialization();
            uint32 specId = static_cast<uint32>(spec);

            // Check if tank spec
    if (specId == 66 ||   // Protection Paladin
                specId == 73 ||   // Protection Warrior
                specId == 104 ||  // Guardian Druid
                specId == 250 ||  // Blood Death Knight
                specId == 268 ||  // Brewmaster Monk
                specId == 581)    // Vengeance Demon Hunter
            {
                tanks.push_back(member);
            }
        }
    }

    if (tanks.empty())
        return;

    // Get primary target
    Unit* primaryTarget = nullptr;
    if (!_primaryTarget.IsEmpty())
        primaryTarget = ObjectAccessor::GetUnit(*tanks[0], _primaryTarget);

    if (!primaryTarget || !primaryTarget->IsAlive())
        return;

    // Analyze threat levels for each tank
    ::std::unordered_map<Player*, float> tankThreat;
    Player* currentTank = nullptr;
    float highestThreat = 0.0f;

    for (Player* tank : tanks)
    {
        float threat = primaryTarget->GetThreatManager().GetThreat(tank);
        tankThreat[tank] = threat;

        if (threat > highestThreat)
        {
            highestThreat = threat;
            currentTank = tank;
        }
    }

    // Implement taunt rotation for multi-tank encounters
    if (tanks.size() >= 2)
    {
        // Check if current tank needs help (low health or high stacks of debuff)
    if (currentTank && currentTank->GetHealthPct() < 40.0f)
        {
            // Find best backup tank
            Player* backupTank = nullptr;
            for (Player* tank : tanks)
            {
                if (tank != currentTank && tank->GetHealthPct() > 60.0f && !tank->HasUnitState(UNIT_STATE_CASTING))
                {
                    backupTank = tank;
                    break;
                }
            }

            // Coordinate taunt swap
    if (backupTank)
            {
                TC_LOG_DEBUG("playerbot", "GroupCoordination: Coordinating tank swap from {} to {}",
                             currentTank->GetName(), backupTank->GetName());

                // Issue taunt command to backup tank
                ::std::vector<uint32> targets;
                targets.push_back(_primaryTarget.GetCounter());
                IssueCommand(backupTank->GetGUID().GetCounter(), CoordinationCommand::ATTACK_TARGET, targets);

                // Tell current tank to use defensive cooldowns
                IssueCommand(currentTank->GetGUID().GetCounter(), CoordinationCommand::DEFENSIVE_MODE, {});
            }
        }

        // Balance threat among tanks to prepare for tank swaps
    for (Player* tank : tanks)
        {
            if (tank == currentTank)
                continue;

            // Maintain threat on backup tanks (should be second in threat)
            float backupThreatTarget = highestThreat * 0.7f; // 70% of main tank threat
    if (tankThreat[tank] < backupThreatTarget * 0.5f) // If below 35% of main tank threat
            {
                // Tell backup tank to build threat
                ::std::vector<uint32> targets;
                targets.push_back(_primaryTarget.GetCounter());
                IssueCommand(tank->GetGUID().GetCounter(), CoordinationCommand::ATTACK_TARGET, targets);
            }
        }
    }

    // Monitor threat transfer and ensure smooth handoffs
    HandleThreatRedirection(currentTank ? currentTank->GetGUID().GetCounter() : 0, 0);

    TC_LOG_DEBUG("playerbot", "GroupCoordination: Tank threat management updated for group {}", _groupId);
}

void GroupCoordination::HandleHealerPriorities()
{
    Group* group = sGroupMgr->GetGroupByGUID(_groupId);
    if (!group)
        return;

    // Identify all healers in the group
    ::std::vector<Player*> healers;
    ::std::vector<Player*> groupMembers;

    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            groupMembers.push_back(member);

            ChrSpecialization spec = member->GetPrimarySpecialization();
            uint32 specId = static_cast<uint32>(spec);

            // Check if healer spec
    if (specId == 65 ||    // Holy Paladin
                specId == 256 ||   // Discipline Priest
                specId == 257 ||   // Holy Priest
                specId == 264 ||   // Restoration Shaman
                specId == 270 ||   // Mistweaver Monk
                specId == 105 ||   // Restoration Druid
                specId == 1468)    // Preservation Evoker
            {
                healers.push_back(member);
            }
        }
    }

    if (healers.empty() || groupMembers.empty())
        return;

    // Build priority healing list based on health and role
    struct HealingTarget
    {
        Player* player;
        float healthPct;
        uint32 priority;
        bool hasDebuff;
        bool isTank;

        bool operator<(const HealingTarget& other) const
        {
            // Lower health = higher priority (higher numeric value)
    if (priority != other.priority)
                return priority > other.priority;
            return healthPct < other.healthPct;
        }
    };

    ::std::vector<HealingTarget> healingTargets;

    for (Player* member : groupMembers)
    {
        if (!member || !member->IsAlive())
            continue;

        HealingTarget target;
        target.player = member;
        target.healthPct = member->GetHealthPct();
        target.hasDebuff = false; // Debuff checking handled by ClassAI dispel rotation
        target.priority = 100;

        // Determine role-based priority
        ChrSpecialization spec = member->GetPrimarySpecialization();
        uint32 specId = static_cast<uint32>(spec);

        // Tanks get highest priority
    if (specId == 66 || specId == 73 || specId == 104 || specId == 250 || specId == 268 || specId == 581)
        {
            target.isTank = true;
            target.priority += 300;
        }
        // Healers get second priority
        else if (specId == 65 || specId == 256 || specId == 257 || specId == 264 || specId == 270 || specId == 105 || specId == 1468)
        {
            target.isTank = false;
            target.priority += 150;
        }
        // DPS get normal priority
        else
        {
            target.isTank = false;
            target.priority += 50;
        }

        // Health-based priority boost
    if (target.healthPct < 20.0f)
            target.priority += 1000; // Critical
        else if (target.healthPct < 40.0f)
            target.priority += 500; // High
        else if (target.healthPct < 60.0f)
            target.priority += 200; // Medium
        else if (target.healthPct < 80.0f)
            target.priority += 100; // Low

        healingTargets.push_back(target);
    }

    // Sort by priority (highest first)
    ::std::sort(healingTargets.begin(), healingTargets.end());

    // Assign healing targets to healers
    if (healers.size() == 1)
    {
        // Single healer: Focus on highest priority targets
        Player* healer = healers[0];
        for (const HealingTarget& target : healingTargets)
        {
            if (target.healthPct < 100.0f)
            {
                TC_LOG_DEBUG("playerbot", "GroupCoordination: Healer {} assigned to heal {} (priority: {}, HP: {:.1f}%)",
                             healer->GetName(), target.player->GetName(), target.priority, target.healthPct);

                // Only assign the top 2 priority targets to prevent spam
                static uint32 assignCount = 0;
                if (++assignCount >= 2)
                    break;
            }
        }
    }
    else
    {
        // Multiple healers: Distribute healing assignments
        uint32 healerIndex = 0;
        for (const HealingTarget& target : healingTargets)
        {
            if (target.healthPct < 85.0f) // Only assign if healing is needed
            {
                Player* assignedHealer = healers[healerIndex % healers.size()];
                TC_LOG_DEBUG("playerbot", "GroupCoordination: Healer {} assigned to heal {} (priority: {}, HP: {:.1f}%)",
                             assignedHealer->GetName(), target.player->GetName(), target.priority, target.healthPct);

                healerIndex++;

                // Distribute among all healers
    if (healerIndex >= healers.size() * 2) // Each healer gets max 2 assignments
                    break;
            }
        }
    }

    // Coordinate dispelling (remove debuffs)
    for (Player* healer : healers)
    {
        // Check if healer can dispel
        bool canDispelMagic = false;
        bool canDispelDisease = false;
        bool canDispelPoison = false;
        bool canDispelCurse = false;

        ChrSpecialization spec = healer->GetPrimarySpecialization();
        uint32 specId = static_cast<uint32>(spec);

        // Priest (Magic, Disease)
    if (specId == 256 || specId == 257)
        {
            canDispelMagic = true;
            canDispelDisease = true;
        }
        // Paladin (Magic, Poison, Disease)
        else if (specId == 65)
        {
            canDispelMagic = true;
            canDispelPoison = true;
            canDispelDisease = true;
        }
        // Shaman (Magic, Curse)
        else if (specId == 264)
        {
            canDispelMagic = true;
            canDispelCurse = true;
        }
        // Druid (Magic, Curse, Poison)
        else if (specId == 105)
        {
            canDispelMagic = true;
            canDispelCurse = true;
            canDispelPoison = true;
        }
        // Monk (Magic, Poison, Disease)
        else if (specId == 270)
        {
            canDispelMagic = true;
            canDispelPoison = true;
            canDispelDisease = true;
        }
        // Evoker (Magic, Poison, Curse, Disease, Bleed)
        else if (specId == 1468)
        {
            canDispelMagic = true;
            canDispelPoison = true;
            canDispelCurse = true;
            canDispelDisease = true;
        }

        // Find group members with dispellable debuffs
    if (canDispelMagic || canDispelDisease || canDispelPoison || canDispelCurse)
        {
            for (Player* member : groupMembers)
            {
                if (!member || !member->IsAlive())
                    continue;

                // Check for debuffs that can be dispelled
                // Note: Actual debuff checking would require iterating through auras
                // This is a simplified check
                TC_LOG_TRACE("playerbot", "GroupCoordination: Checking {} for dispellable debuffs", member->GetName());
            }
        }
    }

    // Coordinate mana management among healers
    for (Player* healer : healers)
    {
        float manaPct = healer->GetPowerPct(POWER_MANA);
        if (manaPct < 30.0f)
        {
            TC_LOG_DEBUG("playerbot", "GroupCoordination: Healer {} has low mana ({:.1f}%), requesting support",
                         healer->GetName(), manaPct);

            // Tell healer to use mana regeneration abilities
            IssueCommand(healer->GetGUID().GetCounter(), CoordinationCommand::SAVE_COOLDOWNS, {});
        }
    }

    TC_LOG_DEBUG("playerbot", "GroupCoordination: Healer priorities updated for group {}", _groupId);
}

void GroupCoordination::HandleDPSTargeting()
{
    Group* group = sGroupMgr->GetGroupByGUID(_groupId);
    if (!group)
        return;

    // Identify all DPS players in the group
    ::std::vector<Player*> dpsPlayers;
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            ChrSpecialization spec = member->GetPrimarySpecialization();
            uint32 specId = static_cast<uint32>(spec);

            // Skip tanks and healers, rest are DPS
            bool isTank = (specId == 66 || specId == 73 || specId == 104 || specId == 250 || specId == 268 || specId == 581);
            bool isHealer = (specId == 65 || specId == 256 || specId == 257 || specId == 264 || specId == 270 || specId == 105 || specId == 1468);

            if (!isTank && !isHealer)
            {
                dpsPlayers.push_back(member);
            }
        }
    }

    if (dpsPlayers.empty())
        return;

    // Get primary and secondary targets
    ::std::vector<ObjectGuid> targetPriority = GetTargetPriorityList();
    if (targetPriority.empty())
        return;

    // Assign DPS to primary target (focus fire)
    ObjectGuid primaryTarget = GetPrimaryTarget();
    if (!primaryTarget.IsEmpty())
    {
        // Issue focus fire command to all DPS
        ::std::vector<uint32> targets;
        targets.push_back(primaryTarget.GetCounter());
        for (Player* dps : dpsPlayers)
        {
            IssueCommand(dps->GetGUID().GetCounter(), CoordinationCommand::FOCUS_FIRE, targets);
            TC_LOG_DEBUG("playerbot", "GroupCoordination: DPS {} assigned to focus fire on primary target",
                         dps->GetName());
        }
    }

    // Coordinate interrupts among DPS
    if (!primaryTarget.IsEmpty())
    {
        Unit* target = ObjectAccessor::GetUnit(*dpsPlayers[0], primaryTarget);
        if (target && target->HasUnitState(UNIT_STATE_CASTING))
        {
            // Find first available DPS with interrupt capability
            Player* interrupter = nullptr;
            for (Player* dps : dpsPlayers)
            {
                // Check if DPS can interrupt (class-based)
                uint8 classId = dps->GetClass();
                bool canInterrupt = false;

                switch (classId)
                {
                    case CLASS_WARRIOR:     // Pummel
                    case CLASS_ROGUE:       // Kick
                    case CLASS_HUNTER:      // Counter Shot
                    case CLASS_SHAMAN:      // Wind Shear
                    case CLASS_MAGE:        // Counterspell
                    case CLASS_WARLOCK:     // Spell Lock (pet)
                    case CLASS_MONK:        // Spear Hand Strike
                    case CLASS_DEMON_HUNTER:// Disrupt
                    case CLASS_DEATH_KNIGHT:// Mind Freeze
                    case CLASS_EVOKER:      // Quell
                        canInterrupt = true;
                        break;
                    default:
                        canInterrupt = false;
                        break;
                }

                if (canInterrupt && !dps->HasUnitState(UNIT_STATE_CASTING))
                {
                    interrupter = dps;
                    break;
                }
            }

            if (interrupter)
            {
                ::std::vector<uint32> targets;
                targets.push_back(primaryTarget.GetCounter());
                IssueCommand(interrupter->GetGUID().GetCounter(), CoordinationCommand::INTERRUPT_CAST, targets);
                TC_LOG_DEBUG("playerbot", "GroupCoordination: Assigned {} to interrupt cast on primary target",
                             interrupter->GetName());
            }
        }
    }

    // Manage target switching for high-priority adds
    if (targetPriority.size() > 1)
    {
        // Check if secondary targets need attention
    for (size_t i = 1; i < targetPriority.size() && i < 4; ++i)
        {
            ObjectGuid secondaryGuid = targetPriority[i];
            Unit* secondaryTarget = ObjectAccessor::GetUnit(*dpsPlayers[0], secondaryGuid);

            if (!secondaryTarget || !secondaryTarget->IsAlive())
                continue;

            // Assign one DPS to handle secondary target if it's high threat
            auto it = _targets.find(secondaryGuid);
            if (it != _targets.end() && it->second.threatLevel >= ThreatLevel::HIGH)
            {
                // Find melee or ranged DPS closest to the target
                Player* assignedDPS = nullptr;
                float closestDistance = 100.0f;

                for (Player* dps : dpsPlayers)
                {
                    float distance = dps->GetDistance(secondaryTarget);
                    if (distance < closestDistance)
                    {
                        closestDistance = distance;
                        assignedDPS = dps;
                    }
                }

                if (assignedDPS)
                {
                    ::std::vector<uint32> targets;
                    targets.push_back(secondaryGuid.GetCounter());
                    IssueCommand(assignedDPS->GetGUID().GetCounter(), CoordinationCommand::ATTACK_TARGET, targets);
                    TC_LOG_DEBUG("playerbot", "GroupCoordination: DPS {} assigned to secondary target (threat: HIGH)",
                                 assignedDPS->GetName());
                }
            }
        }
    }

    // Coordinate cooldown usage during burn phase
    if (_currentPhase == EncounterPhase::BURN)
    {
        for (Player* dps : dpsPlayers)
        {
            IssueCommand(dps->GetGUID().GetCounter(), CoordinationCommand::USE_COOLDOWNS, {});
            TC_LOG_DEBUG("playerbot", "GroupCoordination: DPS {} instructed to use cooldowns for burn phase",
                         dps->GetName());
        }
    }

    TC_LOG_DEBUG("playerbot", "GroupCoordination: DPS targeting updated for group {}", _groupId);
}

void GroupCoordination::HandleSupportActions()
{
    Group* group = sGroupMgr->GetGroupByGUID(_groupId);
    if (!group)
        return;

    ::std::vector<Player*> groupMembers;
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
            groupMembers.push_back(member);
    }

    if (groupMembers.empty())
        return;

    // Coordinate raid-wide buffs
    struct BuffProvider
    {
        Player* player;
        uint8 classId;
        ::std::vector<uint32> providedBuffs;
    };

    ::std::vector<BuffProvider> buffProviders;

    for (Player* member : groupMembers)
    {
        BuffProvider provider;
        provider.player = member;
        provider.classId = member->GetClass();

        // Identify what buffs this player can provide
    switch (provider.classId)
        {
            case CLASS_WARRIOR:
                provider.providedBuffs.push_back(6673); // Battle Shout
                break;
            case CLASS_PALADIN:
                provider.providedBuffs.push_back(465); // Devotion Aura
                provider.providedBuffs.push_back(183435); // Retribution Aura
                break;
            case CLASS_HUNTER:
                provider.providedBuffs.push_back(13159); // Aspect of the Pack
                break;
            case CLASS_MAGE:
                provider.providedBuffs.push_back(1459); // Arcane Intellect
                break;
            case CLASS_PRIEST:
                provider.providedBuffs.push_back(21562); // Power Word: Fortitude
                break;
            case CLASS_WARLOCK:
                provider.providedBuffs.push_back(20707); // Soulstone
                break;
            case CLASS_SHAMAN:
                provider.providedBuffs.push_back(192077); // Wind Rush Totem
                break;
            case CLASS_MONK:
                provider.providedBuffs.push_back(116841); // Legacy of the White Tiger
                break;
            case CLASS_DRUID:
                provider.providedBuffs.push_back(1126); // Mark of the Wild
                break;
            case CLASS_DEMON_HUNTER:
                provider.providedBuffs.push_back(203981); // Chaos Brand
                break;
            case CLASS_DEATH_KNIGHT:
                provider.providedBuffs.push_back(57330); // Horn of Winter
                break;
            case CLASS_EVOKER:
                provider.providedBuffs.push_back(364342); // Blessing of the Bronze
                break;
            default:
                break;
        }

        if (!provider.providedBuffs.empty())
            buffProviders.push_back(provider);
    }

    // Apply missing buffs
    for (const BuffProvider& provider : buffProviders)
    {
        for (uint32 buffSpellId : provider.providedBuffs)
        {
            // Check if any group member is missing this buff
            bool needsBuff = false;
            for (Player* member : groupMembers)
            {
                if (!member->HasAura(buffSpellId))
                {
                    needsBuff = true;
                    break;
                }
            }

            if (needsBuff)
            {
                TC_LOG_DEBUG("playerbot", "GroupCoordination: Requesting {} to apply buff {}",
                             provider.player->GetName(), buffSpellId);
                // Note: Actual buff casting would be handled by individual bot AI
            }
        }
    }

    // Coordinate crowd control assignments
    ::std::vector<ObjectGuid> targetPriority = GetTargetPriorityList();
    if (targetPriority.size() > 2) // If multiple targets, coordinate CC
    {
        ::std::vector<Player*> ccCapablePlayers;
        for (Player* member : groupMembers)
        {
            uint8 classId = member->GetClass();
            bool canCC = (classId == CLASS_MAGE || classId == CLASS_HUNTER || classId == CLASS_ROGUE ||
                          classId == CLASS_WARLOCK || classId == CLASS_DRUID || classId == CLASS_SHAMAN ||
                          classId == CLASS_PRIEST || classId == CLASS_MONK);

            if (canCC)
                ccCapablePlayers.push_back(member);
        }

        // Assign CC to lowest priority targets
        size_t ccIndex = 0;
        for (size_t i = targetPriority.size() - 1; i >= 2 && ccIndex < ccCapablePlayers.size(); --i)
        {
            ObjectGuid ccTarget = targetPriority[i];
            Player* ccPlayer = ccCapablePlayers[ccIndex];

            ::std::vector<uint32> targets;
            targets.push_back(ccTarget.GetCounter());
            IssueCommand(ccPlayer->GetGUID().GetCounter(), CoordinationCommand::CROWD_CONTROL, targets);

            TC_LOG_DEBUG("playerbot", "GroupCoordination: Assigned {} to crowd control target {}",
                         ccPlayer->GetName(), ccTarget.ToString());

            ccIndex++;
        }
    }

    // Coordinate utility abilities for encounter mechanics
    if (_currentPhase == EncounterPhase::TRANSITION)
    {
        // During transitions, coordinate movement abilities
    for (Player* member : groupMembers)
        {
            TC_LOG_DEBUG("playerbot", "GroupCoordination: Preparing {} for encounter transition",
                         member->GetName());
        }
    }

    // Coordinate defensive cooldowns during high threat
    if (_overallThreat >= ThreatLevel::HIGH)
    {
        for (Player* member : groupMembers)
        {
            uint8 classId = member->GetClass();

            // Each class has raid-wide defensive cooldowns
            bool hasRaidCooldown = (classId == CLASS_PRIEST ||      // Divine Hymn, Power Word: Barrier
                                     classId == CLASS_PALADIN ||    // Aura Mastery, Divine Shield
                                     classId == CLASS_SHAMAN ||     // Spirit Link Totem, Healing Tide Totem
                                     classId == CLASS_MONK ||       // Revival
                                     classId == CLASS_DRUID ||      // Tranquility
                                     classId == CLASS_DEMON_HUNTER);// Darkness
    if (hasRaidCooldown && member->GetHealthPct() > 50.0f) // Only if player is healthy enough
            {
                IssueCommand(member->GetGUID().GetCounter(), CoordinationCommand::USE_COOLDOWNS, {});
                TC_LOG_DEBUG("playerbot", "GroupCoordination: Requesting {} to use raid defensive cooldown (threat: HIGH)",
                             member->GetName());
            }
        }
    }

    TC_LOG_DEBUG("playerbot", "GroupCoordination: Support actions updated for group {}", _groupId);
}

} // namespace Playerbot