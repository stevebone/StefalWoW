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

#ifndef GarrisonMgr_h__
#define GarrisonMgr_h__

#include "Define.h"
#include "Hash.h"
#include "Position.h"
#include "QueensConservatory.h"
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct CharShipmentEntry;
struct CharShipmentContainerEntry;
struct GameObjectsEntry;
struct GarrAbilityEntry;
struct GarrAutoCombatantEntry;
struct GarrAutoSpellEntry;
struct GarrAutoSpellEffectEntry;
struct GarrEncounterEntry;
struct GarrFollowerEntry;
struct GarrFollowerLevelXPEntry;
struct GarrFollowerQualityEntry;
struct GarrFollSupportSpellEntry;
struct GarrMechanicEntry;
struct GarrMissionXFollowerEntry;
struct GarrMechanicTypeEntry;
struct GarrMissionEntry;
struct GarrMissionXEncounterEntry;
struct GarrSiteLevelEntry;
struct GarrSiteLevelPlotInstEntry;
struct GarrFollowerTypeEntry;
struct GarrTalentEntry;
struct GarrTalentRankEntry;
struct GarrTalentResearchEntry;
struct GarrTalentTreeEntry;

struct FinalizeGarrisonPlotGOInfo
{
    struct
    {
        uint32 GameObjectId;
        Position Pos;
        uint16 AnimKitId;
    } FactionInfo[2];
};

struct GarrAbilities
{
    std::unordered_set<GarrAbilityEntry const*> Counters;
    std::unordered_set<GarrAbilityEntry const*> Traits;
};

// Authored base/overmax mission reward (world table garrison_mission_reward). Base garrison mission
// rewards (Garrison/Order/War Resources, Anima, gold, gear) are server-authoritative in retail - they
// are NOT derivable from client DB2 - so they are seeded here (from real sniff data) with a per-GarrType
// resource-currency formula fallback for unauthored missions. See GarrisonMgr::LoadMissionRewards.
struct GarrisonMissionRewardEntry
{
    uint8  RewardType;      // 0 = base reward (granted on success), 1 = overmax/bonus (granted on bonus roll)
    uint32 ItemId;
    uint32 ItemQuantity;
    uint32 CurrencyId;
    uint32 CurrencyQuantity;
    uint32 Gold;            // copper; emitted as reward.CurrencyID = 0
    uint32 FollowerXP;      // 0 = use DB2 BaseFollowerXP (already pushed separately)
};

class TC_GAME_API GarrisonMgr
{
public:
    GarrisonMgr();
    GarrisonMgr(GarrisonMgr const&) = delete;
    GarrisonMgr(GarrisonMgr&&) = delete;
    GarrisonMgr& operator=(GarrisonMgr const&) = delete;
    GarrisonMgr& operator=(GarrisonMgr&&) = delete;
    ~GarrisonMgr();

    static GarrisonMgr& Instance();

    void Initialize();

    GarrSiteLevelEntry const* GetGarrSiteLevelEntry(uint32 garrSiteId, uint32 level) const;
    std::vector<GarrSiteLevelPlotInstEntry const*> const* GetGarrPlotInstForSiteLevel(uint32 garrSiteLevelId) const;
    GameObjectsEntry const* GetPlotGameObject(uint32 mapId, uint32 garrPlotInstanceId) const;
    bool IsPlotMatchingBuilding(uint32 garrPlotId, uint32 garrBuildingId) const;
    uint32 GetGarrBuildingPlotInst(uint32 garrBuildingId, uint32 garrSiteLevelPlotInstId) const;
    uint32 GetPreviousLevelBuildingId(uint32 buildingType, uint32 currentLevel) const;
    FinalizeGarrisonPlotGOInfo const* GetPlotFinalizeGOInfo(uint32 garrPlotInstanceID) const;
    uint64 GenerateFollowerDbId();
    std::list<GarrAbilityEntry const*> RollFollowerAbilities(uint32 garrFollowerId, GarrFollowerEntry const* follower, uint32 quality, uint32 faction, bool initial) const;
    std::list<GarrAbilityEntry const*> GetClassSpecAbilities(GarrFollowerEntry const* follower, uint32 faction) const;

    // Follower type mapping
    GarrFollowerTypeEntry const* GetFollowerTypeForGarrType(int8 garrTypeID) const;
    uint8 GetPrimaryFollowerType(int8 garrTypeID) const;

    // Follower progression
    GarrFollowerLevelXPEntry const* GetFollowerLevelXP(uint8 garrFollowerTypeID, int8 followerLevel) const;
    GarrFollowerQualityEntry const* GetFollowerQuality(uint16 garrFollowerTypeID, int8 quality) const;

    // Returns the lowest-OrderIndex zone-support spell for a follower in the given faction
    // (GARRISON_FACTION_INDEX_HORDE / GARRISON_FACTION_INDEX_ALLIANCE). Returns 0 when none defined.
    uint32 GetFollowerZoneSupportSpell(uint32 garrFollowerID, uint32 factionIndex) const;

    // Shipment system accessors
    CharShipmentContainerEntry const* GetShipmentContainerForBuilding(uint8 garrBuildingType, uint8 factionIndex) const;
    CharShipmentContainerEntry const* GetShipmentContainerForNpc(uint32 creatureEntry) const;
    std::vector<CharShipmentEntry const*> const* GetShipmentsForContainer(uint32 containerID) const;
    uint64 GenerateShipmentDbId();
    uint64 GenerateMissionDbId();

    // Optional gate on an order-hall work order (e.g. the Hunter "Unseen Path" talent unlocks a
    // "Requisition a Seal of Broken Fate" order, capped at 3 per week).
    struct OrderHallShipmentGate
    {
        uint32 RequiredTalentId = 0;    // GarrTalent that must be researched to place this order (0 = none)
        uint32 WeeklyLimit      = 0;    // per-week cap on orders placed at this NPC (0 = unlimited)
    };
    OrderHallShipmentGate const* GetOrderHallShipmentGate(uint32 creatureEntry) const;

    // The "standard" GameObject (GAMEOBJECT_TYPE_GARRISON_SHIPMENT, e.g. "Training Troops") that displays and hands
    // out a container's finished orders. 0 = none.
    uint32 GetStandardGoForContainer(uint32 containerId) const;

    // Where a container's per-player standard spawns when its troops are ready (garrison_order_hall_standard).
    struct OrderHallStandard
    {
        uint32 GoEntry = 0;
        uint32 MapId = 0;
        Position Pos;
    };
    OrderHallStandard const* GetOrderHallStandard(uint32 containerId) const;
    // The recruiter creature that hosts a container's work order (reverse of garrison_order_hall_shipment) - the NPC
    // that shows the "working" clock while an order recruits. 0 = none.
    uint32 GetRecruiterForContainer(uint32 containerId) const;
    void LoadOrderHallStandards();

    // Queen's Conservatory (Night Fae unique sanctum feature, GarrTalentTree 319). The wildseed kinds - their
    // cost, maturation time and reward chest - have no representation in any 12.0.7 DB2, so they are authored
    // content in the world table `garrison_conservatory_wildseed` rather than constants in the core. With no
    // rows loaded the Conservatory engine simply refuses to plant; see QueensConservatory.h.
    void LoadConservatoryWildseeds();
    ConservatoryWildseedTemplate const* GetConservatoryWildseed(uint32 wildseedEntry) const;
    std::unordered_map<uint32, ConservatoryWildseedTemplate> const& GetConservatoryWildseeds() const { return _conservatoryWildseeds; }

    // The catalyst items a wildseed pod can be linked to (world table `garrison_conservatory_catalyst`) and
    // the loot table each catalyst combination pays out (`garrison_conservatory_yield`). The catalyst effects
    // ARE client-derived - each item's own description states them - but the item set and the loot tables are
    // content, so neither is compiled in. Empty tables are valid: AttachCatalyst then refuses, and a harvest
    // falls back to the wildseed's reward chest exactly as it did before catalysts existed.
    void LoadConservatoryCatalysts();
    ConservatoryCatalystTemplate const* GetConservatoryCatalyst(uint32 catalystItemId) const;
    std::unordered_map<uint32, ConservatoryCatalystTemplate> const& GetConservatoryCatalysts() const { return _conservatoryCatalysts; }

    void LoadConservatoryYields();
    // gameobject_loot_template entry for this catalyst set, preferring a spirit-specific row over the
    // spiritItemId 0 wildcard. 0 means "no row" - the caller must refuse, never fall back to another table.
    uint32 GetConservatoryYieldLootId(uint32 spiritItemId, uint8 rootGrainCount, uint8 nightbloomCount) const;
    std::map<ConservatoryYieldKey, uint32> const& GetConservatoryYields() const { return _conservatoryYields; }

    // Talent system accessors
    std::vector<GarrTalentTreeEntry const*> const* GetTalentTreesForGarrType(int8 garrTypeID) const;
    std::vector<GarrTalentEntry const*> const* GetTalentsForTree(uint32 garrTalentTreeID) const;
    std::vector<GarrTalentRankEntry const*> const* GetTalentRanksForTalent(uint32 garrTalentID) const;
    GarrTalentResearchEntry const* GetTalentResearchForTree(uint32 garrTalentTreeID) const;

    // Mission system accessors
    std::vector<GarrMissionEntry const*> const* GetMissionsByGarrType(int8 garrTypeID) const;
    std::vector<GarrMissionXEncounterEntry const*> const* GetMissionEncounters(uint32 garrMissionID) const;
    std::vector<GarrMechanicEntry const*> const* GetEncounterMechanics(uint32 garrEncounterID) const;
    std::vector<GarrMissionXFollowerEntry const*> const* GetMissionRequiredFollowers(uint32 garrMissionID) const;
    GarrMechanicTypeEntry const* GetMechanicType(int32 garrMechanicTypeID) const;
    bool DoesAbilityCounterMechanic(GarrAbilityEntry const* ability, GarrMechanicTypeEntry const* mechanicType) const;

    // Mission reward accessors (authored world table + per-GarrType formula fallback)
    std::vector<GarrisonMissionRewardEntry> const* GetMissionRewards(uint32 garrMissionID) const;
    uint32 GetMissionRewardCurrency(GarrMissionEntry const* mission) const;   // era resource currency (824/1101/1220/1560/1813)
    uint32 ComputeBaseResourceReward(GarrMissionEntry const* mission) const;  // cost/duration-scaled fallback amount

    // Auto-combat accessors
    GarrAutoCombatantEntry const* GetAutoCombatant(uint32 garrAutoCombatantID) const;
    GarrAutoCombatantEntry const* GetAutoCombatantForEncounter(uint32 garrEncounterID) const;
    std::vector<GarrAutoSpellEffectEntry const*> const* GetAutoSpellEffects(uint32 garrAutoSpellID) const;
    std::vector<uint32> const* GetEncounterSetEncounters(int32 garrEncounterSetID) const;

private:
    void InitializeDbIdSequences();
    void LoadPlotFinalizeGOInfo();
    void LoadFollowerClassSpecAbilities();
    void LoadMissionRewards();
    void LoadOrderHallShipments();

    std::unordered_map<std::pair<uint32 /*garrSiteId*/, uint32 /*level*/>, GarrSiteLevelEntry const*> _garrSiteLevelBySiteAndLevel;
    std::unordered_map<uint32 /*garrSiteId*/, std::vector<GarrSiteLevelPlotInstEntry const*>> _garrisonPlotInstBySiteLevel;
    std::unordered_map<uint32 /*mapId*/, std::unordered_map<uint32 /*garrPlotId*/, GameObjectsEntry const*>> _garrisonPlots;
    std::unordered_map<uint32 /*garrPlotId*/, std::unordered_set<uint32/*garrBuildingId*/>> _garrisonBuildingsByPlot;
    std::unordered_map<std::pair<uint32 /*garrBuildingId*/, uint32 /*garrSiteLevelPlotInstId*/>, uint32 /*garrBuildingPlotInstId*/> _garrisonBuildingPlotInstances;
    std::unordered_map<uint32 /*buildingType*/, std::vector<uint32>> _garrisonBuildingsByType;
    std::unordered_map<uint32 /*garrPlotInstanceId*/, FinalizeGarrisonPlotGOInfo> _finalizePlotGOInfo;
    std::unordered_map<uint32 /*garrFollowerId*/, GarrAbilities> _garrisonFollowerAbilities[2];
    std::unordered_map<uint32 /*classSpecId*/, std::list<GarrAbilityEntry const*>> _garrisonFollowerClassSpecAbilities;
    std::set<GarrAbilityEntry const*> _garrisonFollowerRandomTraits;

    // Follower type index (garrison type -> primary follower type entry)
    std::unordered_map<int8 /*garrTypeID*/, GarrFollowerTypeEntry const*> _followerTypeByGarrType;

    // Follower progression indices
    std::unordered_map<std::pair<uint8 /*garrFollowerTypeID*/, int8 /*level*/>, GarrFollowerLevelXPEntry const*> _followerLevelXP;
    std::unordered_map<std::pair<uint16 /*garrFollowerTypeID*/, int8 /*quality*/>, GarrFollowerQualityEntry const*> _followerQuality;

    // Mission system indices
    std::unordered_map<int8 /*garrTypeID*/, std::vector<GarrMissionEntry const*>> _missionsByGarrType;
    std::unordered_map<uint32 /*garrMissionID*/, std::vector<GarrMissionXEncounterEntry const*>> _missionEncounters;
    std::unordered_map<uint32 /*garrEncounterID*/, std::vector<GarrMechanicEntry const*>> _encounterMechanics;
    std::unordered_map<uint32 /*garrMissionID*/, std::vector<GarrMissionXFollowerEntry const*>> _missionRequiredFollowers;
    std::unordered_map<uint32 /*garrMissionID*/, std::vector<GarrisonMissionRewardEntry>> _missionRewards;

    // Talent system indices
    std::unordered_map<int8 /*garrTypeID*/, std::vector<GarrTalentTreeEntry const*>> _talentTreesByGarrType;
    std::unordered_map<uint32 /*garrTalentTreeID*/, std::vector<GarrTalentEntry const*>> _talentsByTree;
    std::unordered_map<uint32 /*garrTalentID*/, std::vector<GarrTalentRankEntry const*>> _talentRanksByTalent;
    std::unordered_map<uint32 /*garrTalentTreeID*/, GarrTalentResearchEntry const*> _talentResearchByTree;

    // Auto-combat indices
    std::unordered_map<uint32 /*garrAutoSpellID*/, std::vector<GarrAutoSpellEffectEntry const*>> _autoSpellEffects;
    std::unordered_map<int32 /*garrEncounterSetID*/, std::vector<uint32 /*garrEncounterID*/>> _encounterSetEncounters;
    std::unordered_map<uint32 /*garrEncounterID*/, GarrAutoCombatantEntry const*> _autoCombatantByEncounter;

    // Shipment system indices
    std::unordered_map<uint8 /*garrBuildingType*/, CharShipmentContainerEntry const*> _shipmentContainersByBuildingType;
    std::unordered_map<uint32 /*creatureEntry*/, CharShipmentContainerEntry const*> _orderHallContainerByNpc;
    std::unordered_map<uint32 /*creatureEntry*/, OrderHallShipmentGate> _orderHallGateByNpc;
    std::unordered_map<uint32 /*containerId*/, uint32 /*goEntry*/> _orderHallStandardGoByContainer;
    std::unordered_map<uint32 /*containerId*/, OrderHallStandard> _orderHallStandardByContainer;
    std::unordered_map<uint32 /*containerId*/, uint32 /*npcEntry*/> _recruiterByContainer;
    std::unordered_map<uint32 /*containerID*/, std::vector<CharShipmentEntry const*>> _shipmentsByContainer;

    // Queen's Conservatory authored wildseed kinds (world table `garrison_conservatory_wildseed`).
    std::unordered_map<uint32 /*wildseedEntry*/, ConservatoryWildseedTemplate> _conservatoryWildseeds;
    // Queen's Conservatory catalyst items (`garrison_conservatory_catalyst`) and the loot table each
    // catalyst combination rolls (`garrison_conservatory_yield`).
    std::unordered_map<uint32 /*catalystItemId*/, ConservatoryCatalystTemplate> _conservatoryCatalysts;
    std::map<ConservatoryYieldKey, uint32 /*lootId*/> _conservatoryYields;

    uint64 _followerDbIdGenerator = UI64LIT(1);
    uint64 _shipmentDbIdGenerator = UI64LIT(1);
    uint64 _missionDbIdGenerator = UI64LIT(1);
};

#define sGarrisonMgr GarrisonMgr::Instance()

#endif // GarrisonMgr_h__
