/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RaidHealCoordinator.h"
#include "RaidCoordinator.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot {

// ============================================================================
// CONSTRUCTOR
// ============================================================================

RaidHealCoordinator::RaidHealCoordinator(RaidCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void RaidHealCoordinator::Initialize()
{
    Reset();

    // Initialize healer info from coordinator's healer list
    for (ObjectGuid healerGuid : _coordinator->GetHealers())
    {
        RaidHealerInfo info;
        info.guid = healerGuid;
        info.assignment = HealerAssignment::RAID_HEALING;
        _healers.push_back(info);
    }

    // Auto-assign if enabled
    if (_autoAssignEnabled)
    {
        AutoAssignHealers();
    }

    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::Initialize - Initialized with %zu healers",
        _healers.size());
}

void RaidHealCoordinator::Update(uint32 diff)
{
    // Update healer info
    UpdateHealerInfo();

    // Process external CD requests
    ProcessExternalRequests();

    // Update external cooldowns
    for (auto& pair : _externalCooldowns)
    {
        if (pair.second > diff)
            pair.second -= diff;
        else
            pair.second = 0;
    }

    // Rebalance if needed
    if (_autoAssignEnabled && NeedsRebalancing())
    {
        RebalanceAssignments();
    }
}

void RaidHealCoordinator::Reset()
{
    _healers.clear();
    _assignments.clear();
    _tankAssignments.clear();
    _groupAssignments.clear();
    _externalRequests.clear();
    _externalCooldowns.clear();
    _dispelHealers.clear();
}

// ============================================================================
// HEALER ASSIGNMENT
// ============================================================================

void RaidHealCoordinator::AssignHealerToTank(ObjectGuid healer, ObjectGuid tank)
{
    RaidHealerInfo* info = FindHealer(healer);
    if (!info)
        return;

    // Determine tank assignment (TANK_1 or TANK_2)
    bool isTank1 = (tank == _coordinator->GetMainTank());
    info->assignment = isTank1 ? HealerAssignment::TANK_1 : HealerAssignment::TANK_2;
    info->assignedTarget = tank;

    _assignments[healer] = info->assignment;
    _tankAssignments[healer] = tank;
    _groupAssignments.erase(healer);

    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::AssignHealerToTank - Healer assigned to tank");
}

void RaidHealCoordinator::AssignHealerToGroup(ObjectGuid healer, uint8 groupNum)
{
    RaidHealerInfo* info = FindHealer(healer);
    if (!info)
        return;

    info->assignment = static_cast<HealerAssignment>(10 + groupNum);  // GROUP_1 = 11, etc.
    info->groupAssignment = groupNum;

    _assignments[healer] = info->assignment;
    _groupAssignments[healer] = groupNum;
    _tankAssignments.erase(healer);

    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::AssignHealerToGroup - Healer assigned to group %u", groupNum);
}

void RaidHealCoordinator::AssignHealerToDispel(ObjectGuid healer)
{
    RaidHealerInfo* info = FindHealer(healer);
    if (!info)
        return;

    info->assignment = HealerAssignment::DISPEL_DUTY;

    _assignments[healer] = info->assignment;
    _dispelHealers.push_back(healer);

    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::AssignHealerToDispel - Healer assigned to dispel duty");
}

void RaidHealCoordinator::AssignHealerToRaid(ObjectGuid healer)
{
    RaidHealerInfo* info = FindHealer(healer);
    if (!info)
        return;

    info->assignment = HealerAssignment::RAID_HEALING;
    info->assignedTarget.Clear();
    info->groupAssignment = 0;

    _assignments[healer] = HealerAssignment::RAID_HEALING;
    _tankAssignments.erase(healer);
    _groupAssignments.erase(healer);
}

void RaidHealCoordinator::UnassignHealer(ObjectGuid healer)
{
    _assignments.erase(healer);
    _tankAssignments.erase(healer);
    _groupAssignments.erase(healer);

    auto dispelIt = std::remove(_dispelHealers.begin(), _dispelHealers.end(), healer);
    if (dispelIt != _dispelHealers.end())
        _dispelHealers.erase(dispelIt, _dispelHealers.end());

    RaidHealerInfo* info = FindHealer(healer);
    if (info)
    {
        info->assignment = HealerAssignment::RAID_HEALING;
    }
}

HealerAssignment RaidHealCoordinator::GetHealerAssignment(ObjectGuid healer) const
{
    auto it = _assignments.find(healer);
    if (it != _assignments.end())
        return it->second;

    return HealerAssignment::RAID_HEALING;
}

ObjectGuid RaidHealCoordinator::GetAssignedTarget(ObjectGuid healer) const
{
    auto it = _tankAssignments.find(healer);
    if (it != _tankAssignments.end())
        return it->second;

    return ObjectGuid();
}

std::vector<ObjectGuid> RaidHealCoordinator::GetHealersAssignedTo(ObjectGuid target) const
{
    std::vector<ObjectGuid> healers;

    for (const auto& pair : _tankAssignments)
    {
        if (pair.second == target)
            healers.push_back(pair.first);
    }

    return healers;
}

std::vector<ObjectGuid> RaidHealCoordinator::GetHealersAssignedToGroup(uint8 groupNum) const
{
    std::vector<ObjectGuid> healers;

    for (const auto& pair : _groupAssignments)
    {
        if (pair.second == groupNum)
            healers.push_back(pair.first);
    }

    return healers;
}

// ============================================================================
// HEALER INFO
// ============================================================================

RaidHealerInfo* RaidHealCoordinator::GetHealerInfo(ObjectGuid healer)
{
    return FindHealer(healer);
}

const RaidHealerInfo* RaidHealCoordinator::GetHealerInfo(ObjectGuid healer) const
{
    for (const auto& info : _healers)
    {
        if (info.guid == healer)
            return &info;
    }
    return nullptr;
}

// ============================================================================
// AUTO ASSIGNMENT
// ============================================================================

void RaidHealCoordinator::AutoAssignHealers()
{
    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::AutoAssignHealers - Auto-assigning healers");

    // First, assign healers to tanks
    ObjectGuid mainTank = _coordinator->GetMainTank();
    ObjectGuid offTank = _coordinator->GetOffTank();

    uint32 tankHealersAssigned = 0;

    for (auto& healerInfo : _healers)
    {
        if (tankHealersAssigned < _minTankHealers && !mainTank.IsEmpty())
        {
            AssignHealerToTank(healerInfo.guid, mainTank);
            tankHealersAssigned++;
        }
        else if (tankHealersAssigned < _minTankHealers * 2 && !offTank.IsEmpty())
        {
            AssignHealerToTank(healerInfo.guid, offTank);
            tankHealersAssigned++;
        }
        else
        {
            // Assign remaining to raid healing
            AssignHealerToRaid(healerInfo.guid);
        }
    }
}

void RaidHealCoordinator::RebalanceAssignments()
{
    // Re-run auto assignment if healers died or assignments are suboptimal
    AutoAssignHealers();
}

bool RaidHealCoordinator::NeedsRebalancing() const
{
    // Check if any assigned healers are dead
    for (const auto& pair : _tankAssignments)
    {
        Player* healer = _coordinator->GetPlayer(pair.first);
        if (!healer || !healer->IsAlive())
            return true;
    }

    return false;
}

// ============================================================================
// EXTERNAL COOLDOWNS
// ============================================================================

void RaidHealCoordinator::RequestExternalCooldown(ObjectGuid target, uint8 urgency)
{
    ExternalCDRequest request;
    request.targetGuid = target;
    request.urgency = urgency;
    request.requestTime = 0;
    request.fulfilled = false;

    _externalRequests.push_back(request);

    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::RequestExternalCooldown - Request urgency %u", urgency);
}

void RaidHealCoordinator::OnExternalCooldownUsed(ObjectGuid healer, ObjectGuid /*target*/, uint32 /*spellId*/)
{
    _externalCooldowns[healer] = _externalCDDuration;

    RaidHealerInfo* info = FindHealer(healer);
    if (info)
    {
        info->hasExternalAvailable = false;
        info->externalCooldown = _externalCDDuration;
    }
}

ObjectGuid RaidHealCoordinator::GetNextExternalProvider() const
{
    for (const auto& healer : _healers)
    {
        if (healer.hasExternalAvailable)
        {
            Player* player = _coordinator->GetPlayer(healer.guid);
            if (player && player->IsAlive())
                return healer.guid;
        }
    }

    return ObjectGuid();
}

bool RaidHealCoordinator::HasExternalAvailable() const
{
    return !GetNextExternalProvider().IsEmpty();
}

uint32 RaidHealCoordinator::GetExternalCooldownCount() const
{
    uint32 count = 0;
    for (const auto& healer : _healers)
    {
        if (healer.hasExternalAvailable)
            count++;
    }
    return count;
}

std::vector<ObjectGuid> RaidHealCoordinator::GetAvailableExternalProviders() const
{
    std::vector<ObjectGuid> providers;

    for (const auto& healer : _healers)
    {
        if (healer.hasExternalAvailable)
        {
            Player* player = _coordinator->GetPlayer(healer.guid);
            if (player && player->IsAlive())
                providers.push_back(healer.guid);
        }
    }

    return providers;
}

// ============================================================================
// DISPEL COORDINATION
// ============================================================================

void RaidHealCoordinator::RequestDispel(ObjectGuid target, uint32 /*auraId*/)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::RequestDispel - Dispel requested for target");

    // Find a dispel healer
    for (ObjectGuid healerGuid : _dispelHealers)
    {
        Player* healer = _coordinator->GetPlayer(healerGuid);
        if (healer && healer->IsAlive())
        {
            // Would trigger dispel action
            return;
        }
    }
}

void RaidHealCoordinator::OnDispelSucceeded(ObjectGuid /*healer*/, ObjectGuid /*target*/, uint32 /*auraId*/)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::OnDispelSucceeded - Dispel succeeded");
}

std::vector<ObjectGuid> RaidHealCoordinator::GetDispelHealers() const
{
    return _dispelHealers;
}

// ============================================================================
// MANA MANAGEMENT
// ============================================================================

float RaidHealCoordinator::GetAverageHealerMana() const
{
    if (_healers.empty())
        return 100.0f;

    float totalMana = 0.0f;
    uint32 count = 0;

    for (const auto& healer : _healers)
    {
        Player* player = _coordinator->GetPlayer(healer.guid);
        if (player && player->IsAlive())
        {
            totalMana += player->GetPowerPct(POWER_MANA);
            count++;
        }
    }

    return count > 0 ? totalMana / count : 0.0f;
}

float RaidHealCoordinator::GetLowestHealerMana() const
{
    float lowest = 100.0f;

    for (const auto& healer : _healers)
    {
        Player* player = _coordinator->GetPlayer(healer.guid);
        if (player && player->IsAlive())
        {
            float mana = player->GetPowerPct(POWER_MANA);
            if (mana < lowest)
                lowest = mana;
        }
    }

    return lowest;
}

ObjectGuid RaidHealCoordinator::GetLowestManaHealer() const
{
    ObjectGuid lowestGuid;
    float lowestMana = 100.0f;

    for (const auto& healer : _healers)
    {
        Player* player = _coordinator->GetPlayer(healer.guid);
        if (player && player->IsAlive())
        {
            float mana = player->GetPowerPct(POWER_MANA);
            if (mana < lowestMana)
            {
                lowestMana = mana;
                lowestGuid = healer.guid;
            }
        }
    }

    return lowestGuid;
}

bool RaidHealCoordinator::AreHealersLowOnMana() const
{
    return GetAverageHealerMana() < _lowManaThreshold;
}

void RaidHealCoordinator::SignalManaBreak()
{
    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::SignalManaBreak - Mana break requested");
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void RaidHealCoordinator::OnHealingEvent(const CombatEvent& /*event*/)
{
    // Track healing for assignment optimization
}

void RaidHealCoordinator::OnHealerDied(ObjectGuid healer)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidHealCoordinator::OnHealerDied - Healer died!");

    RaidHealerInfo* info = FindHealer(healer);
    if (info)
    {
        // Reassign this healer's duties
        if (_autoAssignEnabled)
        {
            RebalanceAssignments();
        }
    }
}

void RaidHealCoordinator::OnTankAssigned(ObjectGuid tank)
{
    // Ensure tank has a healer
    auto healers = GetHealersAssignedTo(tank);
    if (healers.empty() && _autoAssignEnabled)
    {
        ObjectGuid bestHealer = FindBestHealerForTank(tank);
        if (!bestHealer.IsEmpty())
        {
            AssignHealerToTank(bestHealer, tank);
        }
    }
}

void RaidHealCoordinator::OnTankDied(ObjectGuid tank)
{
    // Reassign healers that were on this tank
    for (auto& pair : _tankAssignments)
    {
        if (pair.second == tank)
        {
            // Temporarily assign to raid healing
            AssignHealerToRaid(pair.first);
        }
    }
}

// ============================================================================
// INTERNAL METHODS (PRIVATE)
// ============================================================================

void RaidHealCoordinator::UpdateHealerInfo()
{
    for (auto& healerInfo : _healers)
    {
        Player* player = _coordinator->GetPlayer(healerInfo.guid);
        if (!player)
            continue;

        healerInfo.manaPercent = player->GetPowerPct(POWER_MANA);

        // Update external cooldown availability
        if (healerInfo.externalCooldown > 0)
        {
            // Cooldown tracking
        }
        else
        {
            healerInfo.hasExternalAvailable = true;
        }
    }
}

void RaidHealCoordinator::ProcessExternalRequests()
{
    // Process pending external CD requests
    auto it = _externalRequests.begin();
    while (it != _externalRequests.end())
    {
        if (it->fulfilled || it->urgency < 5)
        {
            it = _externalRequests.erase(it);
            continue;
        }

        // Try to fulfill high urgency requests
        if (it->urgency >= 8)
        {
            ObjectGuid provider = GetNextExternalProvider();
            if (!provider.IsEmpty())
            {
                // Would trigger external usage
                it->fulfilled = true;
            }
        }

        ++it;
    }
}

RaidHealerInfo* RaidHealCoordinator::FindHealer(ObjectGuid guid)
{
    for (auto& healer : _healers)
    {
        if (healer.guid == guid)
            return &healer;
    }
    return nullptr;
}

ObjectGuid RaidHealCoordinator::FindBestHealerForTank(ObjectGuid /*tank*/) const
{
    // Find unassigned healer with best suitability
    for (const auto& healer : _healers)
    {
        if (_tankAssignments.find(healer.guid) == _tankAssignments.end())
        {
            Player* player = _coordinator->GetPlayer(healer.guid);
            if (player && player->IsAlive())
                return healer.guid;
        }
    }

    return ObjectGuid();
}

ObjectGuid RaidHealCoordinator::FindBestHealerForGroup(uint8 /*groupNum*/) const
{
    // Find unassigned healer with best suitability
    for (const auto& healer : _healers)
    {
        if (_groupAssignments.find(healer.guid) == _groupAssignments.end() &&
            _tankAssignments.find(healer.guid) == _tankAssignments.end())
        {
            Player* player = _coordinator->GetPlayer(healer.guid);
            if (player && player->IsAlive())
                return healer.guid;
        }
    }

    return ObjectGuid();
}

float RaidHealCoordinator::CalculateHealerSuitability(ObjectGuid healer, HealerAssignment /*assignment*/) const
{
    const RaidHealerInfo* info = GetHealerInfo(healer);
    if (!info)
        return 0.0f;

    float score = 50.0f;

    // Higher mana = better
    score += info->manaPercent * 0.3f;

    // Has external available
    if (info->hasExternalAvailable)
        score += 10.0f;

    return score;
}

} // namespace Playerbot
