/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_PERKS_PROGRAM_MGR_H
#define TRINITYCORE_PERKS_PROGRAM_MGR_H

#include "Define.h"
#include "PerksProgramPacketsCommon.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PerksProgramVendorItemData
{
    WorldPackets::PerksProgram::PerksVendorItem PacketData;
};

struct PerksProgramRotationEntry
{
    uint32 RotationID = 0;
    uint32 MonthStart = 0;
    uint32 MonthEnd = 0;
    int32 VendorItemID = 0;
};

struct PerksProgramActivityLists
{
    std::vector<uint32> MainActivityIDs;
    std::vector<int32>  ThresholdActivityIDs;
};

class TC_GAME_API PerksProgramMgr
{
private:
    PerksProgramMgr();
    ~PerksProgramMgr();

public:
    PerksProgramMgr(PerksProgramMgr const&) = delete;
    PerksProgramMgr(PerksProgramMgr&&) = delete;
    PerksProgramMgr& operator=(PerksProgramMgr const&) = delete;
    PerksProgramMgr& operator=(PerksProgramMgr&&) = delete;

    static PerksProgramMgr* Instance();

    void LoadVendorItems();
    void LoadMonthlyRotation();
    void LoadActivityIntervals();
    void BuildCriteriaTreeMap();
    void LoadCurrentActivities();

    PerksProgramVendorItemData const* GetVendorItem(int32 vendorItemID) const;
    std::vector<PerksProgramVendorItemData const*> GetCurrentRotationItems() const;
    PerksProgramActivityLists GetCurrentMonthActivityLists() const;
    int32 GetThresholdActivityForCriteriaTree(uint32 criteriaTreeID) const;

    uint32 GetCurrentMonthStart() const { return _currentMonthStart; }
    uint32 GetCurrentMonthEnd() const { return _currentMonthEnd; }

private:
    struct ActivityIntervalEntry
    {
        uint32 IntervalID  = 0;
        uint32 MonthStart  = 0;
        uint32 MonthEnd    = 0;
        bool   IsThreshold = false;
    };

    std::unordered_map<int32, PerksProgramVendorItemData> _vendorItems;
    std::vector<PerksProgramRotationEntry> _rotationEntries;
    std::vector<ActivityIntervalEntry> _activityIntervals;
    std::unordered_map<uint32, int32> _criteriaTreeToThresholdActivity;
    uint32 _currentMonthStart = 0;
    uint32 _currentMonthEnd = 0;
    std::vector<uint32> _directMainActivities;
    std::vector<int32>  _directThresholdActivities;
};

#define sPerksProgramMgr PerksProgramMgr::Instance()

#endif // TRINITYCORE_PERKS_PROGRAM_MGR_H
