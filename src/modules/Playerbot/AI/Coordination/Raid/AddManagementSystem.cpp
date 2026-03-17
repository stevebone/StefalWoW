/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#include "AddManagementSystem.h"
#include "RaidCoordinator.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot {

AddManagementSystem::AddManagementSystem(RaidCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void AddManagementSystem::Initialize()
{
    Reset();
    TC_LOG_DEBUG("playerbots.raid", "AddManagementSystem::Initialize - Initialized");
}

void AddManagementSystem::Update(uint32 /*diff*/)
{
    CleanupDeadAdds();
}

void AddManagementSystem::Reset()
{
    _adds.clear();
    _tankAssignments.clear();
    _dpsAssignments.clear();
}

void AddManagementSystem::OnAddSpawned(ObjectGuid guid, uint32 creatureId)
{
    RaidAdd add;
    add.guid = guid;
    add.creatureId = creatureId;
    add.priority = AddPriority::NORMAL;
    add.requiresTank = true;
    add.healthPercent = 100.0f;
    add.spawnTime = 0;

    _adds.push_back(add);

    TC_LOG_DEBUG("playerbots.raid", "AddManagementSystem::OnAddSpawned - Add spawned: creature %u", creatureId);

    // Auto-assign tank if available
    AutoAssignTanks();
}

void AddManagementSystem::OnAddDied(ObjectGuid guid)
{
    auto it = std::remove_if(_adds.begin(), _adds.end(),
        [guid](const RaidAdd& add) { return add.guid == guid; });
    _adds.erase(it, _adds.end());

    _tankAssignments.erase(guid);
    _dpsAssignments.erase(guid);

    TC_LOG_DEBUG("playerbots.raid", "AddManagementSystem::OnAddDied - Add died");
}

void AddManagementSystem::OnDeathEvent(const CombatEvent& event)
{
    OnAddDied(event.source);
}

void AddManagementSystem::OnDamageEvent(const CombatEvent& /*event*/)
{
    // Track damage for priority updates
}

void AddManagementSystem::SetAddPriority(ObjectGuid guid, AddPriority priority)
{
    RaidAdd* add = FindAdd(guid);
    if (add)
    {
        add->priority = priority;
    }
}

AddPriority AddManagementSystem::GetAddPriority(ObjectGuid guid) const
{
    const RaidAdd* add = GetAddInfo(guid);
    return add ? add->priority : AddPriority::NORMAL;
}

ObjectGuid AddManagementSystem::GetHighestPriorityAdd() const
{
    ObjectGuid highest;
    AddPriority highestPriority = AddPriority::LOW;

    for (const auto& add : _adds)
    {
        if (add.priority > highestPriority && add.priority != AddPriority::IGNORE)
        {
            highestPriority = add.priority;
            highest = add.guid;
        }
    }

    return highest;
}

std::vector<ObjectGuid> AddManagementSystem::GetAddsByPriority() const
{
    std::vector<ObjectGuid> sorted;
    sorted.reserve(_adds.size());

    // Copy to sortable vector
    std::vector<const RaidAdd*> addPtrs;
    for (const auto& add : _adds)
    {
        if (add.priority != AddPriority::IGNORE)
            addPtrs.push_back(&add);
    }

    // Sort by priority (descending)
    std::sort(addPtrs.begin(), addPtrs.end(),
        [](const RaidAdd* a, const RaidAdd* b) {
            return static_cast<uint8>(a->priority) > static_cast<uint8>(b->priority);
        });

    for (const auto* add : addPtrs)
    {
        sorted.push_back(add->guid);
    }

    return sorted;
}

void AddManagementSystem::AssignTankToAdd(ObjectGuid add, ObjectGuid tank)
{
    RaidAdd* addInfo = FindAdd(add);
    if (addInfo)
    {
        addInfo->assignedTank = tank;
        _tankAssignments[add] = tank;
    }
}

void AddManagementSystem::AssignDPSToAdd(ObjectGuid add, ObjectGuid dps)
{
    RaidAdd* addInfo = FindAdd(add);
    if (addInfo)
    {
        addInfo->assignedDPS.push_back(dps);
        _dpsAssignments[add].push_back(dps);
    }
}

void AddManagementSystem::UnassignFromAdd(ObjectGuid player)
{
    for (auto& add : _adds)
    {
        if (add.assignedTank == player)
            add.assignedTank.Clear();

        auto& dps = add.assignedDPS;
        dps.erase(std::remove(dps.begin(), dps.end(), player), dps.end());
    }

    for (auto& pair : _dpsAssignments)
    {
        auto& dps = pair.second;
        dps.erase(std::remove(dps.begin(), dps.end(), player), dps.end());
    }
}

ObjectGuid AddManagementSystem::GetAssignedTank(ObjectGuid add) const
{
    auto it = _tankAssignments.find(add);
    return it != _tankAssignments.end() ? it->second : ObjectGuid();
}

std::vector<ObjectGuid> AddManagementSystem::GetAssignedDPS(ObjectGuid add) const
{
    auto it = _dpsAssignments.find(add);
    return it != _dpsAssignments.end() ? it->second : std::vector<ObjectGuid>();
}

void AddManagementSystem::CallSwitchToAdd(ObjectGuid add)
{
    RaidAdd* addInfo = FindAdd(add);
    if (addInfo)
    {
        addInfo->isActiveTarget = true;
        addInfo->priority = AddPriority::CRITICAL;
    }

    // Mark other adds as not active
    for (auto& other : _adds)
    {
        if (other.guid != add)
            other.isActiveTarget = false;
    }

    TC_LOG_DEBUG("playerbots.raid", "AddManagementSystem::CallSwitchToAdd - Switching to add");
}

void AddManagementSystem::DistributeDPSToAdds()
{
    AutoDistributeDPS();
}

void AddManagementSystem::ConcentrateDPSOnAdd(ObjectGuid add)
{
    // Unassign all DPS from other adds
    for (auto& other : _adds)
    {
        if (other.guid != add)
            other.assignedDPS.clear();
    }
    _dpsAssignments.clear();

    // Assign all DPS to this add
    RaidAdd* addInfo = FindAdd(add);
    if (addInfo)
    {
        for (ObjectGuid dpsGuid : _coordinator->GetDPS())
        {
            Player* player = _coordinator->GetPlayer(dpsGuid);
            if (player && player->IsAlive())
            {
                addInfo->assignedDPS.push_back(dpsGuid);
            }
        }
        _dpsAssignments[add] = addInfo->assignedDPS;
    }

    TC_LOG_DEBUG("playerbots.raid", "AddManagementSystem::ConcentrateDPSOnAdd - All DPS on single add");
}

const RaidAdd* AddManagementSystem::GetAddInfo(ObjectGuid guid) const
{
    for (const auto& add : _adds)
    {
        if (add.guid == guid)
            return &add;
    }
    return nullptr;
}

std::vector<RaidAdd> AddManagementSystem::GetAllAdds() const
{
    return _adds;
}

uint32 AddManagementSystem::GetActiveAddCount() const
{
    return static_cast<uint32>(_adds.size());
}

bool AddManagementSystem::HasActiveAdds() const
{
    return !_adds.empty();
}

RaidAdd* AddManagementSystem::FindAdd(ObjectGuid guid)
{
    for (auto& add : _adds)
    {
        if (add.guid == guid)
            return &add;
    }
    return nullptr;
}

void AddManagementSystem::CleanupDeadAdds()
{
    // Remove adds that are no longer valid
    auto it = std::remove_if(_adds.begin(), _adds.end(),
        [](const RaidAdd& add) { return add.healthPercent <= 0.0f; });
    _adds.erase(it, _adds.end());
}

void AddManagementSystem::AutoAssignTanks()
{
    const auto& tanks = _coordinator->GetTanks();
    if (tanks.size() < 2)
        return; // Need at least 2 tanks (one for boss)

    // Assign tanks to adds that need them
    uint32 tankIndex = 1; // Start from off-tank
    for (auto& add : _adds)
    {
        if (add.requiresTank && add.assignedTank.IsEmpty())
        {
            if (tankIndex < tanks.size())
            {
                Player* tank = _coordinator->GetPlayer(tanks[tankIndex]);
                if (tank && tank->IsAlive())
                {
                    AssignTankToAdd(add.guid, tanks[tankIndex]);
                    tankIndex++;
                }
            }
        }
    }
}

void AddManagementSystem::AutoDistributeDPS()
{
    if (_adds.empty())
        return;

    const auto& dps = _coordinator->GetDPS();
    if (dps.empty())
        return;

    // Clear existing assignments
    for (auto& add : _adds)
    {
        add.assignedDPS.clear();
    }
    _dpsAssignments.clear();

    // Distribute evenly
    uint32 addIndex = 0;
    for (ObjectGuid dpsGuid : dps)
    {
        Player* player = _coordinator->GetPlayer(dpsGuid);
        if (player && player->IsAlive())
        {
            _adds[addIndex].assignedDPS.push_back(dpsGuid);
            _dpsAssignments[_adds[addIndex].guid].push_back(dpsGuid);
            addIndex = (addIndex + 1) % _adds.size();
        }
    }
}

} // namespace Playerbot
