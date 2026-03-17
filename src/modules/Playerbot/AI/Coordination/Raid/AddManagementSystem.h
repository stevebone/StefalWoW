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

class AddManagementSystem
{
public:
    AddManagementSystem(RaidCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // Add Tracking
    void OnAddSpawned(ObjectGuid guid, uint32 creatureId);
    void OnAddDied(ObjectGuid guid);
    void OnDeathEvent(const CombatEvent& event);
    void OnDamageEvent(const CombatEvent& event);

    // Priority Management
    void SetAddPriority(ObjectGuid guid, AddPriority priority);
    AddPriority GetAddPriority(ObjectGuid guid) const;
    ObjectGuid GetHighestPriorityAdd() const;
    ::std::vector<ObjectGuid> GetAddsByPriority() const;

    // Assignment
    void AssignTankToAdd(ObjectGuid add, ObjectGuid tank);
    void AssignDPSToAdd(ObjectGuid add, ObjectGuid dps);
    void UnassignFromAdd(ObjectGuid player);
    ObjectGuid GetAssignedTank(ObjectGuid add) const;
    ::std::vector<ObjectGuid> GetAssignedDPS(ObjectGuid add) const;

    // Coordination
    void CallSwitchToAdd(ObjectGuid add);
    void DistributeDPSToAdds();
    void ConcentrateDPSOnAdd(ObjectGuid add);

    // Query
    const RaidAdd* GetAddInfo(ObjectGuid guid) const;
    ::std::vector<RaidAdd> GetAllAdds() const;
    uint32 GetActiveAddCount() const;
    bool HasActiveAdds() const;

private:
    RaidCoordinator* _coordinator;

    ::std::vector<RaidAdd> _adds;
    ::std::map<ObjectGuid, ObjectGuid> _tankAssignments;
    ::std::map<ObjectGuid, ::std::vector<ObjectGuid>> _dpsAssignments;

    RaidAdd* FindAdd(ObjectGuid guid);
    void CleanupDeadAdds();
    void AutoAssignTanks();
    void AutoDistributeDPS();
};

} // namespace Playerbot
