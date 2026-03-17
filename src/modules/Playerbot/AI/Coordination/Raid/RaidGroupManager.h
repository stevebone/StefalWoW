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

class RaidGroupManager
{
public:
    RaidGroupManager(RaidCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // Sub-group Access
    RaidSubGroup* GetSubGroup(uint8 groupNum);
    const RaidSubGroup* GetSubGroup(uint8 groupNum) const;
    uint8 GetPlayerGroup(ObjectGuid player) const;
    ::std::vector<ObjectGuid> GetGroupMembers(uint8 groupNum) const;

    // Assignment
    void AssignToSubGroup(ObjectGuid player, uint8 groupNum);
    void RemoveFromSubGroup(ObjectGuid player);
    void SwapPlayers(ObjectGuid player1, ObjectGuid player2);

    // Split Mechanics
    void SplitRaid(uint8 numGroups);
    void MergeRaid();
    bool IsSplit() const { return _isSplit; }
    uint8 GetSplitGroup(ObjectGuid player) const;

    // Optimization
    void OptimizeSubGroups();
    void BalanceTanksAndHealers();
    bool NeedsOptimization() const;

    // Statistics
    uint8 GetGroupCount() const { return static_cast<uint8>(_subGroups.size()); }
    uint32 GetTotalMembers() const;

private:
    RaidCoordinator* _coordinator;

    ::std::vector<RaidSubGroup> _subGroups;
    ::std::map<ObjectGuid, uint8> _playerGroups;
    bool _isSplit = false;
    uint8 _splitGroups = 1;

    void InitializeGroups();
    void UpdateGroupComposition();
};

} // namespace Playerbot
