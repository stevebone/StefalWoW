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

#include "PerksProgramMgr.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "GameTime.h"
#include "Log.h"
#include "Timer.h"
#include <algorithm>

PerksProgramMgr::PerksProgramMgr() { }
PerksProgramMgr::~PerksProgramMgr() { }

PerksProgramMgr* PerksProgramMgr::Instance()
{
    static PerksProgramMgr instance;
    return &instance;
}

void PerksProgramMgr::LoadVendorItems()
{
    uint32 oldMSTime = getMSTime();

    _vendorItems.clear();

    QueryResult result = WorldDatabase.Query("SELECT VendorItemID, MountID, BattlePetSpeciesID, TransmogSetID, "
        "ItemModifiedAppearanceID, TransmogIllusionID, ToyID, WarbandSceneID, Price, OriginalPrice, "
        "AvailableUntil, Disabled, DoesNotExpire FROM perks_vendor_items");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 perks vendor items. DB table `perks_vendor_items` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        PerksProgramVendorItemData data;
        data.PacketData.VendorItemID              = fields[0].GetInt32();
        data.PacketData.MountID                   = fields[1].GetInt32();
        data.PacketData.BattlePetSpeciesID        = fields[2].GetInt32();
        data.PacketData.TransmogSetID             = fields[3].GetInt32();
        data.PacketData.ItemModifiedAppearanceID  = fields[4].GetInt32();
        data.PacketData.TransmogIllusionID        = fields[5].GetInt32();
        data.PacketData.ToyID                     = fields[6].GetInt32();
        data.PacketData.WarbandSceneID            = fields[7].GetInt32();
        data.PacketData.Price                     = fields[8].GetInt32();
        data.PacketData.OriginalPrice             = fields[9].GetInt32();
        data.PacketData.AvailableUntil            = time_t(fields[10].GetUInt32());
        data.PacketData.Disabled                  = fields[11].GetUInt8() != 0;
        data.PacketData.DoesNotExpire             = fields[12].GetUInt8() != 0;

        _vendorItems[data.PacketData.VendorItemID] = data;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} perks vendor items in {} ms.", uint32(_vendorItems.size()), GetMSTimeDiffToNow(oldMSTime));
}

void PerksProgramMgr::LoadMonthlyRotation()
{
    uint32 oldMSTime = getMSTime();

    _rotationEntries.clear();
    _currentMonthStart = 0;
    _currentMonthEnd = 0;

    QueryResult result = WorldDatabase.Query("SELECT rotation_id, month_start, month_end, vendor_item_id FROM perks_monthly_rotation ORDER BY month_start DESC");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 perks monthly rotation entries. DB table `perks_monthly_rotation` is empty.");
        return;
    }

    uint32 now = uint32(GameTime::GetGameTime());

    do
    {
        Field* fields = result->Fetch();

        PerksProgramRotationEntry entry;
        entry.RotationID   = fields[0].GetUInt32();
        entry.MonthStart   = fields[1].GetUInt32();
        entry.MonthEnd     = fields[2].GetUInt32();
        entry.VendorItemID = fields[3].GetInt32();

        _rotationEntries.push_back(entry);

        if (now >= entry.MonthStart && now < entry.MonthEnd)
        {
            if (_currentMonthStart == 0 || entry.MonthStart > _currentMonthStart)
                _currentMonthStart = entry.MonthStart;
            if (_currentMonthEnd == 0 || entry.MonthEnd > _currentMonthEnd)
                _currentMonthEnd = entry.MonthEnd;
        }
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} perks monthly rotation entries in {} ms.", uint32(_rotationEntries.size()), GetMSTimeDiffToNow(oldMSTime));

    if (_currentMonthStart > 0)
        TC_LOG_INFO("server.loading", ">> Current perks rotation: {} to {} ({} active items).",
            _currentMonthStart, _currentMonthEnd, uint32(GetCurrentRotationItems().size()));
}

PerksProgramVendorItemData const* PerksProgramMgr::GetVendorItem(int32 vendorItemID) const
{
    auto itr = _vendorItems.find(vendorItemID);
    return itr != _vendorItems.end() ? &itr->second : nullptr;
}

std::vector<PerksProgramVendorItemData const*> PerksProgramMgr::GetCurrentRotationItems() const
{
    std::vector<PerksProgramVendorItemData const*> items;
    uint32 now = uint32(GameTime::GetGameTime());

    for (PerksProgramRotationEntry const& entry : _rotationEntries)
    {
        if (now >= entry.MonthStart && now < entry.MonthEnd)
        {
            if (PerksProgramVendorItemData const* item = GetVendorItem(entry.VendorItemID))
                items.push_back(item);
        }
    }

    return items;
}

void PerksProgramMgr::LoadActivityIntervals()
{
    uint32 oldMSTime = getMSTime();

    _activityIntervals.clear();

    QueryResult result = WorldDatabase.Query(
        "SELECT interval_id, month_start, month_end, is_threshold "
        "FROM perks_activity_intervals");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 perks activity intervals. DB table `perks_activity_intervals` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        ActivityIntervalEntry entry;
        entry.IntervalID  = fields[0].GetUInt32();
        entry.MonthStart  = fields[1].GetUInt32();
        entry.MonthEnd    = fields[2].GetUInt32();
        entry.IsThreshold = fields[3].GetUInt8() != 0;
        _activityIntervals.push_back(entry);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} perks activity interval entries in {} ms.", uint32(_activityIntervals.size()), GetMSTimeDiffToNow(oldMSTime));
}

void PerksProgramMgr::LoadCurrentActivities()
{
    uint32 oldMSTime = getMSTime();

    _directMainActivities.clear();
    _directThresholdActivities.clear();

    QueryResult result = WorldDatabase.Query(
        "SELECT activity_id, is_threshold FROM perks_current_activities "
        "WHERE month_start <= UNIX_TIMESTAMP() AND month_end > UNIX_TIMESTAMP()");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 perks current activities. DB table `perks_current_activities` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        int32 activityID = fields[0].GetInt32();
        bool isThreshold = fields[1].GetUInt8() != 0;

        if (isThreshold)
            _directThresholdActivities.push_back(activityID);
        else
            _directMainActivities.push_back(uint32(activityID));
    } while (result->NextRow());

    std::sort(_directMainActivities.begin(), _directMainActivities.end());
    std::sort(_directThresholdActivities.begin(), _directThresholdActivities.end());

    TC_LOG_INFO("server.loading", ">> Loaded {} perks current activities ({} main, {} threshold) in {} ms.",
        uint32(_directMainActivities.size() + _directThresholdActivities.size()),
        uint32(_directMainActivities.size()),
        uint32(_directThresholdActivities.size()),
        GetMSTimeDiffToNow(oldMSTime));
}

PerksProgramActivityLists PerksProgramMgr::GetCurrentMonthActivityLists() const
{
    if (!_directMainActivities.empty() || !_directThresholdActivities.empty())
    {
        PerksProgramActivityLists result;
        result.MainActivityIDs = _directMainActivities;
        result.ThresholdActivityIDs = _directThresholdActivities;
        return result;
    }

    PerksProgramActivityLists result;
    uint32 now = uint32(GameTime::GetGameTime());

    std::unordered_set<uint32> mainIntervals;
    std::unordered_set<uint32> thresholdIntervals;

    for (ActivityIntervalEntry const& entry : _activityIntervals)
    {
        if (now >= entry.MonthStart && now < entry.MonthEnd)
        {
            if (entry.IsThreshold)
                thresholdIntervals.insert(entry.IntervalID);
            else
                mainIntervals.insert(entry.IntervalID);
        }
    }

    std::unordered_set<uint32> seenMain;
    std::unordered_set<int32> seenThreshold;

    for (PerksActivityXIntervalEntry const* xiv : sPerksActivityXIntervalStore)
    {
        if (mainIntervals.count(xiv->IntervalID) && seenMain.insert(uint32(xiv->PerksActivityID)).second)
            result.MainActivityIDs.push_back(uint32(xiv->PerksActivityID));
        else if (thresholdIntervals.count(xiv->IntervalID) && seenThreshold.insert(xiv->PerksActivityID).second)
            result.ThresholdActivityIDs.push_back(xiv->PerksActivityID);
    }

    std::sort(result.MainActivityIDs.begin(), result.MainActivityIDs.end());
    std::sort(result.ThresholdActivityIDs.begin(), result.ThresholdActivityIDs.end());

    return result;
}

void PerksProgramMgr::BuildCriteriaTreeMap()
{
    _criteriaTreeToThresholdActivity.clear();

    uint32 now = uint32(GameTime::GetGameTime());

    std::unordered_set<uint32> thresholdIntervals;
    for (ActivityIntervalEntry const& entry : _activityIntervals)
        if (entry.IsThreshold && now >= entry.MonthStart && now < entry.MonthEnd)
            thresholdIntervals.insert(entry.IntervalID);

    if (thresholdIntervals.empty())
        return;

    for (PerksActivityXIntervalEntry const* xiv : sPerksActivityXIntervalStore)
    {
        if (!thresholdIntervals.count(xiv->IntervalID))
            continue;

        if (PerksActivityEntry const* activity = sPerksActivityStore.LookupEntry(uint32(xiv->PerksActivityID)))
            if (activity->CriteriaTreeID != 0)
                _criteriaTreeToThresholdActivity[uint32(activity->CriteriaTreeID)] = xiv->PerksActivityID;
    }

    TC_LOG_INFO("server.loading", ">> Built perks criteria tree map: {} threshold activities mapped.", uint32(_criteriaTreeToThresholdActivity.size()));
}

int32 PerksProgramMgr::GetThresholdActivityForCriteriaTree(uint32 criteriaTreeID) const
{
    auto itr = _criteriaTreeToThresholdActivity.find(criteriaTreeID);
    return itr != _criteriaTreeToThresholdActivity.end() ? itr->second : 0;
}
