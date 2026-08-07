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

#ifndef Garrison_h__
#define Garrison_h__

#include "Define.h"
#include "DatabaseEnvFwd.h"
#include "GarrisonPackets.h"
#include "Optional.h"
#include "QueensConservatory.h"
#include "QuaternionData.h"
#include "SharedDefines.h"
#include <algorithm>
#include <unordered_map>

class GameObject;
class Map;
class Player;
struct GarrBuildingEntry;
struct GarrItemLevelUpgradeDataEntry;
struct GarrMissionEntry;
struct GarrSiteLevelEntry;

enum GarrisonType : int32
{
    GARRISON_TYPE_GARRISON      = 2,
    GARRISON_TYPE_CLASS_ORDER   = 3,
    GARRISON_TYPE_WAR_CAMPAIGN  = 9,
    GARRISON_TYPE_COVENANT      = 111
};

// Known GarrSite ids. GarrSite.db2 (GarrSiteID -> GarrTypeID) is not shipped in 12.0.x, so the ids the core creates
// garrisons with are named here; GetGarrisonTypeFromSiteId (Garrison.cpp) maps them back to a GarrisonType.
enum GarrisonSiteId : uint32
{
    GARR_SITE_COVENANT_SANCTUM  = 296   // Shadowlands covenant sanctum (GarrSiteLevel 837/838/839 -> maps 2222/2162/2236)
};

// GarrTalentTree.FeatureTypeIndex. For the covenant sanctum (GarrTypeID 111) this says which sanctum feature a
// tree belongs to, and FeatureSubtypeIndex is then the CovenantID. Every non-covenant garrison type publishes
// index 0 only, so these are safe to test on any tree.
enum GarrTalentTreeFeatureType : uint8
{
    GARR_TALENT_FEATURE_ABILITIES           = 0,    // covenant class + signature abilities (trees 393/396/397/395)
    GARR_TALENT_FEATURE_ANIMA_CONDUCTOR     = 1,    // trees 312/314/311/313
    GARR_TALENT_FEATURE_TRANSPORT_NETWORK   = 2,    // trees 308/309/307/310
    GARR_TALENT_FEATURE_COMMAND_TABLE       = 3,    // trees 316/317/315/318
    GARR_TALENT_FEATURE_RESERVOIR           = 4,    // trees 327/326/328/329
    GARR_TALENT_FEATURE_UNIQUE              = 5,    // trees 320/324/319/321
    GARR_TALENT_FEATURE_SOULBIND            = 6,    // the 12 soulbind trees
    GARR_TALENT_FEATURE_CHANNEL_ANIMA       = 7     // trees 345/348/346/347
};

enum GarrisonFactionIndex
{
    GARRISON_FACTION_INDEX_HORDE    = 0,
    GARRISON_FACTION_INDEX_ALLIANCE = 1
};

enum GarrisonBuildingFlags
{
    GARRISON_BUILDING_FLAG_NEEDS_PLAN   = 0x1
};

enum GarrisonFollowerFlags
{
    GARRISON_FOLLOWER_FLAG_UNIQUE   = 0x1
};

enum GarrisonFollowerType
{
    FOLLOWER_TYPE_GARRISON      = 1,
    FOLLOWER_TYPE_SHIPYARD      = 2,
    FOLLOWER_TYPE_CLASS_ORDER   = 4,
    FOLLOWER_TYPE_WAR_CAMPAIGN  = 22,
    FOLLOWER_TYPE_COVENANT      = 123
};

enum GarrisonAbilityFlags
{
    GARRISON_ABILITY_FLAG_TRAIT                         = 0x0001,
    GARRISON_ABILITY_CANNOT_ROLL                        = 0x0002,
    GARRISON_ABILITY_HORDE_ONLY                         = 0x0004,
    GARRISON_ABILITY_ALLIANCE_ONLY                      = 0x0008,
    GARRISON_ABILITY_FLAG_CANNOT_REMOVE                 = 0x0010,
    GARRISON_ABILITY_FLAG_EXCLUSIVE                     = 0x0020,
    GARRISON_ABILITY_FLAG_SINGLE_MISSION_DURATION       = 0x0040,
    GARRISON_ABILITY_FLAG_ACTIVE_ONLY_ON_ZONE_SUPPORT   = 0x0080,
    GARRISON_ABILITY_FLAG_APPLY_TO_FIRST_MISSION        = 0x0100,
    GARRISON_ABILITY_FLAG_IS_SPECIALIZATION             = 0x0200,
    GARRISON_ABILITY_FLAG_IS_EMPTY_SLOT                 = 0x0400
};

enum GarrisonError
{
    GARRISON_SUCCESS                                            = 0,
    GARRISON_ERROR_NO_GARRISON                                  = 1,
    GARRISON_ERROR_GARRISON_EXISTS                              = 2,
    GARRISON_ERROR_GARRISON_SAME_TYPE_EXISTS                    = 3,
    GARRISON_ERROR_INVALID_GARRISON                             = 4,
    GARRISON_ERROR_INVALID_GARRISON_LEVEL                       = 5,
    GARRISON_ERROR_GARRISON_LEVEL_UNCHANGED                     = 6,
    GARRISON_ERROR_NOT_IN_GARRISON                              = 7,
    GARRISON_ERROR_NO_BUILDING                                  = 8,
    GARRISON_ERROR_BUILDING_EXISTS                              = 9,
    GARRISON_ERROR_INVALID_PLOT_INSTANCEID                      = 10,
    GARRISON_ERROR_INVALID_BUILDINGID                           = 11,
    GARRISON_ERROR_INVALID_UPGRADE_LEVEL                        = 12,
    GARRISON_ERROR_UPGRADE_LEVEL_EXCEEDS_GARRISON_LEVEL         = 13,
    GARRISON_ERROR_PLOTS_NOT_FULL                               = 14,
    GARRISON_ERROR_INVALID_SITE_ID                              = 15,
    GARRISON_ERROR_INVALID_PLOT_BUILDING                        = 16,
    GARRISON_ERROR_INVALID_FACTION                              = 17,
    GARRISON_ERROR_INVALID_SPECIALIZATION                       = 18,
    GARRISON_ERROR_SPECIALIZATION_EXISTS                        = 19,
    GARRISON_ERROR_SPECIALIZATION_ON_COOLDOWN                   = 20,
    GARRISON_ERROR_BLUEPRINT_EXISTS                             = 21,
    GARRISON_ERROR_REQUIRES_BLUEPRINT                           = 22,
    GARRISON_ERROR_INVALID_DOODAD_SET_ID                        = 23,
    GARRISON_ERROR_BUILDING_TYPE_EXISTS                         = 24,
    GARRISON_ERROR_BUILDING_NOT_ACTIVE                          = 25,
    GARRISON_ERROR_CONSTRUCTION_COMPLETE                        = 26,
    GARRISON_ERROR_FOLLOWER_EXISTS                              = 27,
    GARRISON_ERROR_INVALID_FOLLOWER                             = 28,
    GARRISON_ERROR_FOLLOWER_ALREADY_ON_MISSION                  = 29,
    GARRISON_ERROR_FOLLOWER_IN_BUILDING                         = 30,
    GARRISON_ERROR_FOLLOWER_INVALID_FOR_BUILDING                = 31,
    GARRISON_ERROR_INVALID_FOLLOWER_LEVEL                       = 32,
    GARRISON_ERROR_MISSION_EXISTS                               = 33,
    GARRISON_ERROR_INVALID_MISSION                              = 34,
    GARRISON_ERROR_INVALID_MISSION_TIME                         = 35,
    GARRISON_ERROR_INVALID_MISSION_REWARD_INDEX                 = 36,
    GARRISON_ERROR_MISSION_NOT_OFFERED                          = 37,
    GARRISON_ERROR_ALREADY_ON_MISSION                           = 38,
    GARRISON_ERROR_MISSION_SIZE_INVALID                         = 39,
    GARRISON_ERROR_FOLLOWER_SOFT_CAP_EXCEEDED                   = 40,
    GARRISON_ERROR_NOT_ON_MISSION                               = 41,
    GARRISON_ERROR_ALREADY_COMPLETED_MISSION                    = 42,
    GARRISON_ERROR_MISSION_NOT_COMPLETE                         = 43,
    GARRISON_ERROR_MISSION_REWARDS_PENDING                      = 44,
    GARRISON_ERROR_MISSION_EXPIRED                              = 45,
    GARRISON_ERROR_NOT_ENOUGH_CURRENCY                          = 46,
    GARRISON_ERROR_NOT_ENOUGH_GOLD                              = 47,
    GARRISON_ERROR_BUILDING_MISSING                             = 48,
    GARRISON_ERROR_NO_ARCHITECT                                 = 49,
    GARRISON_ERROR_ARCHITECT_NOT_AVAILABLE                      = 50,
    GARRISON_ERROR_NO_MISSION_NPC                               = 51,
    GARRISON_ERROR_MISSION_NPC_NOT_AVAILABLE                    = 52,
    GARRISON_ERROR_INTERNAL_ERROR                               = 53,
    GARRISON_ERROR_INVALID_STATIC_TABLE_VALUE                   = 54,
    GARRISON_ERROR_INVALID_ITEM_LEVEL                           = 55,
    GARRISON_ERROR_INVALID_AVAILABLE_RECRUIT                    = 56,
    GARRISON_ERROR_FOLLOWER_ALREADY_RECRUITED                   = 57,
    GARRISON_ERROR_RECRUITMENT_GENERATION_IN_PROGRESS           = 58,
    GARRISON_ERROR_RECRUITMENT_ON_COOLDOWN                      = 59,
    GARRISON_ERROR_RECRUIT_BLOCKED_BY_GENERATION                = 60,
    GARRISON_ERROR_RECRUITMENT_NPC_NOT_AVAILABLE                = 61,
    GARRISON_ERROR_INVALID_FOLLOWER_QUALITY                     = 62,
    GARRISON_ERROR_PROXY_NOT_OK                                 = 63,
    GARRISON_ERROR_RECALL_PORTAL_USED_LESS_THAN_24_HOURS_AGO    = 64,
    GARRISON_ERROR_ON_REMOVE_BUILDING_SPELL_FAILED              = 65,
    GARRISON_ERROR_OPERATION_NOT_SUPPORTED                      = 66,
    GARRISON_ERROR_FOLLOWER_FATIGUED                            = 67,
    GARRISON_ERROR_UPGRADE_CONDITION_FAILED                     = 68,
    GARRISON_ERROR_FOLLOWER_INACTIVE                            = 69,
    GARRISON_ERROR_FOLLOWER_ACTIVE                              = 70,
    GARRISON_ERROR_FOLLOWER_ACTIVATION_UNAVAILABLE              = 71,
    GARRISON_ERROR_FOLLOWER_TYPE_MISMATCH                       = 72,
    GARRISON_ERROR_INVALID_GARRISON_TYPE                        = 73,
    GARRISON_ERROR_MISSION_START_CONDITION_FAILED               = 74,
    GARRISON_ERROR_INVALID_FOLLOWER_ABILITY                     = 75,
    GARRISON_ERROR_INVALID_MISSION_BONUS_ABILITY                = 76,
    GARRISON_ERROR_HIGHER_BUILDING_TYPE_EXISTS                  = 77,
    GARRISON_ERROR_AT_FOLLOWER_HARD_CAP                         = 78,
    GARRISON_ERROR_FOLLOWER_CANNOT_GAIN_XP                      = 79,
    GARRISON_ERROR_NO_OP                                        = 80,
    GARRISON_ERROR_AT_CLASS_SPEC_CAP                            = 81,
    GARRISON_ERROR_MISSION_REQUIRES_100_TO_START                = 82,
    GARRISON_ERROR_MISSION_MISSING_REQUIRED_FOLLOWER            = 83,
    GARRISON_ERROR_INVALID_TALENT                               = 84,
    GARRISON_ERROR_ALREADY_RESEARCHING_TALENT                   = 85,
    GARRISON_ERROR_FAILED_CONDITION                             = 86,
    GARRISON_ERROR_INVALID_TIER                                 = 87,
    GARRISON_ERROR_INVALID_CLASS                                = 88
};

enum GarrisonFollowerStatus
{
    FOLLOWER_STATUS_FAVORITE    = 0x01,
    FOLLOWER_STATUS_EXHAUSTED   = 0x02,
    FOLLOWER_STATUS_INACTIVE    = 0x04,
    FOLLOWER_STATUS_TROOP       = 0x08,
    FOLLOWER_STATUS_NO_XP_GAIN  = 0x10
};

// Charge count minted onto a recruited class-hall/order-hall troop. CharShipment carries no charge field;
// Legion troops are created with 3 charges (each mission use spends one).
enum : uint32 { GARRISON_TROOP_DEFAULT_DURABILITY = 3 };

class TC_GAME_API Garrison
{
public:
    struct Building
    {
        bool CanActivate() const;

        ObjectGuid Guid;
        std::unordered_set<ObjectGuid> Spawns;
        Optional<WorldPackets::Garrison::GarrisonBuildingInfo> PacketInfo;
    };

    struct Plot
    {
        GameObject* CreateGameObject(Map* map, GarrisonFactionIndex faction);
        void DeleteGameObject(Map* map);
        void ClearBuildingInfo(GarrisonType garrisonType, Player* owner);
        void SetBuildingInfo(WorldPackets::Garrison::GarrisonBuildingInfo const& buildingInfo, Player* owner);

        WorldPackets::Garrison::GarrisonPlotInfo PacketInfo;
        QuaternionData Rotation;
        uint32 EmptyGameObjectId = 0;
        uint32 GarrSiteLevelPlotInstId = 0;
        Building BuildingInfo;
    };

    struct Follower
    {
        uint32 GetItemLevel() const;
        bool HasAbility(uint32 garrAbilityId) const;

        WorldPackets::Garrison::GarrisonFollower PacketInfo;
    };

    struct Mission
    {
        WorldPackets::Garrison::GarrisonMission PacketInfo;
        std::vector<uint64> CurrentFollowerDBIDs;
        // Outcome is rolled once at CMSG_GARRISON_COMPLETE_MISSION and reused when the mission is
        // finalized (CMSG_GARRISON_MISSION_BONUS_ROLL on success / at complete-time on failure), so the
        // result shown to the player matches the result used to grant rewards. Runtime-only: a mission
        // caught mid-completion by a restart is re-rolled once at finalize (see FinalizeMission).
        bool ResultDetermined = false;
        bool Succeeded = false;
    };

    struct Shipment
    {
        uint64 DbID = 0;
        uint32 ShipmentRecID = 0;
        uint32 PlotInstanceID = 0;
        time_t CreationTime = 0;
        int32 Duration = 0;
        uint64 AssignedFollowerDBID = 0;

        bool IsReady() const;
    };

    // Bit values for Garrison::Talent::Flags. Exact wire bits unverified by sniff;
    // the Temporary bit is server-only and routed back into the wire Flags value.
    enum GarrisonTalentFlags : int32
    {
        GARRISON_TALENT_FLAG_NONE       = 0x0,
        // Legion class-hall one-shot/temporary talent. In a covenant sanctum (GarrType 111) this bit means
        // exactly one thing: an Anima Conductor channel bought with reservoir anima rather than permanently
        // reinforced with Channeled Anima. Garrison::LearnTalent refuses to set it on any other type-111 talent,
        // which is what lets World::DailyReset expire all of them with a single flag test.
        GARRISON_TALENT_FLAG_TEMPORARY  = 0x1,
    };

    struct Talent
    {
        uint32 GarrTalentID = 0;
        int32 Rank = 0;
        time_t ResearchStartTime = 0;
        int32 Flags = 0;
        int32 SoulbindConduitID = 0;
        int32 SoulbindConduitRank = 0;

        bool IsResearching() const;
        bool IsResearchComplete() const;
        bool IsTemporary() const { return (Flags & GARRISON_TALENT_FLAG_TEMPORARY) != 0; }
    };

    explicit Garrison(Player* owner);

    bool LoadFromDB(PreparedQueryResult garrison, PreparedQueryResult blueprints, PreparedQueryResult buildings,
        PreparedQueryResult followers, PreparedQueryResult abilities, PreparedQueryResult missions,
        PreparedQueryResult specializations, PreparedQueryResult shipments, PreparedQueryResult talents,
        PreparedQueryResult trophies, PreparedQueryResult archivedMissions);
    void SaveToDB(CharacterDatabaseTransaction trans);
    static void DeleteFromDB(ObjectGuid::LowType ownerGuid, GarrisonType garrType, CharacterDatabaseTransaction trans);
    static void DeleteFromDB(ObjectGuid::LowType ownerGuid, CharacterDatabaseTransaction trans);

    bool Create(uint32 garrSiteId);
    void Delete();
    void Upgrade();

    void Update(uint32 diff);
    void Enter() const;
    void Leave() const;

    static constexpr GarrisonFactionIndex GetFaction(Team team) { return team == HORDE ? GARRISON_FACTION_INDEX_HORDE : GARRISON_FACTION_INDEX_ALLIANCE; }
    GarrisonFactionIndex GetFaction() const;
    GarrisonType GetType() const { return _garrType; }
    GarrSiteLevelEntry const* GetSiteLevel() const { return _siteLevel; }

    // Plots
    std::vector<Plot*> GetPlots();
    Plot* GetPlot(uint32 garrPlotInstanceId);
    Plot const* GetPlot(uint32 garrPlotInstanceId) const;

    // Buildings
    void LearnBlueprint(uint32 garrBuildingId);
    void UnlearnBlueprint(uint32 garrBuildingId);
    bool HasBlueprint(uint32 garrBuildingId) const { return _knownBuildings.find(garrBuildingId) != _knownBuildings.end(); }
    void PlaceBuilding(uint32 garrPlotInstanceId, uint32 garrBuildingId);
    void CancelBuildingConstruction(uint32 garrPlotInstanceId);

    // WoD Shipyard. Unlike normal buildings it has no architect plot (no GarrBuildingPlotInst entry) and lives
    // on the naval map; we track only its tier. CreateShipyard builds/upgrades it (gated on garrison level 3).
    void CreateShipyard();
    bool HasShipyard() const { return _shipyardBuilding != 0; }
    uint32 GetShipyardBuildingId() const { return _shipyardBuilding; }
    // Whether missions/followers of the given GarrFollowerType are available to this garrison: the garrison's own
    // primary type always is; the shipyard (naval) type only once the shipyard is built. Gates naval mission offers.
    bool IsMissionFollowerTypeAvailable(int8 followerTypeId) const;
    // Whether the owner's active covenant may hold this follower. GarrFollower.CovenantID is 0 for followers that
    // belong to no covenant (every WoD/Legion/War-Campaign follower, plus 41 of the 138 Shadowlands companions) and
    // 1-4 for a covenant-exclusive companion, which only that covenant's sanctum may recruit.
    bool IsFollowerCovenantAllowed(GarrFollowerEntry const* followerEntry) const;
    // Full health of a follower at the given level, taken from its GarrAutoCombatant statline.
    // 0 when the follower publishes no statline (all WoD/Legion/War-Campaign followers).
    static int32 GetFollowerMaxHealth(GarrFollowerEntry const* followerEntry, uint32 followerLevel);
    // Build a ship (a GarrFollowerType-2 GarrFollower) at the shipyard. Validates the shipyard exists, the id is a
    // real ship, it is not already owned, and the ship soft-cap is not exceeded, then adds it as a follower.
    GarrisonError BuildShip(uint32 garrFollowerId);
    uint32 GetShipCount() const;
    static constexpr uint32 SHIPYARD_FOLLOWER_SOFT_CAP = 6;
    void ActivateBuilding(uint32 garrPlotInstanceId);
    void SwapBuildings(uint32 plotId1, uint32 plotId2);

    // Specializations
    void LearnSpecialization(uint32 garrSpecId);
    bool HasSpecialization(uint32 garrSpecId) const { return _knownSpecializations.find(garrSpecId) != _knownSpecializations.end(); }
    void SetBuildingSpecialization(uint32 garrPlotInstanceId, uint32 garrSpecId);

    // Followers
    void AddFollower(uint32 garrFollowerId);
    void AddTroop(uint32 garrFollowerId, uint32 durability);
    Follower const* GetFollower(uint64 dbId) const;
    Follower* GetFollower(uint64 dbId);
    Follower const* GetFollowerByEntry(uint32 garrFollowerId) const;
    void RemoveFollower(uint64 dbId);
    void SetFollowerFavorite(uint64 dbId, bool favorite);
    void SetFollowerInactive(uint64 dbId, bool inactive);
    void RenameFollower(uint64 dbId, std::string const& name);
    void AssignFollowerToBuilding(uint64 dbId, uint32 plotInstanceId);
    void RemoveFollowerFromBuilding(uint64 dbId);
    std::unordered_map<uint64, Follower> const& GetFollowerMap() const { return _followers; }
    template<typename Predicate>
    uint32 CountFollowers(Predicate&& predicate) const
    {
        uint32 count = 0;
        for (auto itr = _followers.begin(); itr != _followers.end(); ++itr)
            if (predicate(itr->second))
                ++count;

        return count;
    }

    // Missions
    // Target size of the offered-mission board. The board is topped up toward this cap over several
    // periodic ticks (see GenerateAvailableMissions); the command-table open handler uses it to detect a
    // full pool (nothing new to generate) and re-send the existing offers so the table is not empty.
    static constexpr uint32 MAX_AVAILABLE_MISSIONS = 15;

    void AddMission(uint32 garrMissionId);
    void SendOfferedMissions() const;
    // Whether the offered-mission board is already at MAX_AVAILABLE_MISSIONS (GenerateAvailableMissions
    // would add nothing, so no ADD_MISSION packets would otherwise reach the client on open).
    bool IsOfferPoolFull() const;
    Mission const* GetMission(uint64 dbId) const;
    Mission* GetMission(uint64 dbId);
    Mission const* GetMissionByRecID(uint32 missionRecID) const;
    Mission* GetMissionByRecID(uint32 missionRecID);
    GarrisonError StartMission(uint32 missionRecID, std::vector<uint64> const& followerDBIDs);
    GarrisonError CompleteMission(uint32 missionRecID);
    GarrisonError ClaimMissionReward(uint32 missionRecID);
    GarrisonError MissionBonusRoll(uint32 missionRecID);
    // Grants rewards (if the stored outcome succeeded) + follower XP (always) + frees followers + removes
    // the mission. Called from the opcodes the WoD client actually sends: BONUS_ROLL on success, and
    // COMPLETE on failure (the client sends no bonus roll for a failed mission).
    GarrisonError FinalizeMission(uint32 missionRecID, bool grantOvermax);
    bool RollMissionOutcome(Mission const& mission, uint32 missionRecID) const;
    void RemoveMission(uint32 missionRecID);
    void GenerateAvailableMissions();
    uint64 GenerateMissionDbId();
    int32 CalculateSuccessChance(uint32 missionRecID, std::vector<uint64> const& followerDBIDs) const;
    void PopulateMissionData(Mission& mission, GarrMissionEntry const* missionEntry) const;
    bool IsAutoCombatMission(Mission const& mission) const;
    void RemoveExpiredMissions();
    uint32 GetAndIncrementSessionMissionCount() { return _sessionMissionCount++; }
    void SendDeleteExpiredMissionsResult() const;
    // Read-only views used by the criteria/PlayerCondition modifier evaluators (ModifierTreeType 141/177/186/195).
    std::unordered_map<uint64 /*dbId*/, Mission> const& GetAllMissions() const { return _missions; }
    // GarrMission record ids of every mission this garrison has finalized (success or failure).
    std::vector<int32> const& GetArchivedMissions() const { return _archivedMissions; }
    bool HasCompletedMission(uint32 garrMissionRecID) const
    {
        return std::find(_archivedMissions.begin(), _archivedMissions.end(), int32(garrMissionRecID)) != _archivedMissions.end();
    }

    // Recruitment
    void SetRecruitmentPreferences(uint32 abilityId, uint32 traitId);
    void GenerateRecruits(uint32 faction);
    GarrisonError RecruitFollower(uint32 garrFollowerID);
    std::vector<WorldPackets::Garrison::GarrisonFollower> const& GetAvailableRecruits() const { return _availableRecruits; }

    // Follower healing
    void HealAllFollowers();
    void SendAllFollowerUpdates();

    // Spell-driven operations
    void FinishMission(uint32 garrMissionRecID);
    void FinishShipment(uint32 plotInstanceId);
    void SetFollowerQuality(uint64 dbId, uint32 quality);
    void SetFollowerLevel(uint64 dbId, uint32 level);
    void AddFollowerXP(uint64 dbId, uint32 xp);
    void LearnFollowerAbility(uint64 dbId, uint32 abilityId);
    void RandomizeFollowerAbilities(uint64 dbId);
    void EndBuildingConstruction(uint32 garrPlotInstanceId);
    void SetGarrisonCacheSize(uint32 size);

    // Garrison resource cache: the WoD cache GameObject accrues Garrison Resources (currency 824) over
    // time (1 per CACHE_RESOURCE_INTERVAL, up to _garrisonCacheSize) and is collected when the player
    // clicks it. GetPendingCacheResources reports what is currently banked; CollectGarrisonCache grants
    // it, advancing the timer by the whole intervals consumed so sub-interval progress is not lost.
    uint32 GetPendingCacheResources() const;
    uint32 CollectGarrisonCache();
    Follower* GetFollowerByGarrFollowerID(uint32 garrFollowerID);
    GarrisonError UpgradeFollowerItemLevel(uint64 dbId, int32 amount, int32 slot, GarrItemLevelUpgradeDataEntry const* upgradeData = nullptr);

    // Building-specific helpers
    GarrBuildingEntry const* GetActiveBuildingByType(uint32 buildingType) const;
    uint32 GetBonusFollowerSlots() const;

    // Shipments (work orders)
    GarrisonError CreateShipment(ObjectGuid npcGUID, uint32 count);
    GarrisonError CreateTroopShipment(ObjectGuid npcGUID, uint32 count); // order-hall/class-hall troop work order (plotless)
    void CompleteShipment(uint64 dbId);
    void CollectReadyShipments(uint32 plotInstanceId);
    void CollectReadyShipmentsForContainer(uint32 containerId); // plotless orders: picked up at the container's "standard" GO
    void UpdateOrderHallStandards(); // sync each plotless container's "standard" GO display to the owner's orders (working/ready/empty)
    void SendOpenShipmentUI(ObjectGuid npcGuid);
    // Swap each building's work-order crate GO display to the "filled" model (CharShipmentContainer
    // Small/Medium/Large DisplayInfoID by order count) while it holds orders, base model when empty.
    void UpdateWorkOrderCrates();
    std::vector<Shipment const*> GetShipmentsForPlot(uint32 plotInstanceId) const;
    std::vector<Shipment const*> GetAllShipments() const;
    void SendShipmentInfo(ObjectGuid npcGUID);
    void SendLandingPageShipments();
    uint32 GetBuildingTypeForPlot(uint32 plotInstanceId) const;
    uint32 FindPlotInstanceForNpc(ObjectGuid npcGUID) const;

    // Talent system
    uint32 LearnTalent(uint32 garrTalentID, bool isTemporary);
    uint32 ResearchTalent(uint32 garrTalentID);
    uint32 SocketTalent(uint32 garrTalentID, int32 soulbindConduitID, int32 soulbindConduitRank);
    // Wipes every talent in one tree (server-driven respec) and pushes the removal to the client.
    uint32 ResetTalentTree(uint32 garrTalentTreeID);
    Talent const* GetTalent(uint32 garrTalentID) const;
    std::unordered_map<uint32, Talent> const& GetAllTalents() const { return _talents; }
    void CompleteAllTalentResearch(bool sendUpdate = false);
    // (Re)apply GarrTalentRank.PerkSpellID for every rank this garrison has already completed. Called on login,
    // after the garrisons and the covenant/soulbind state are both loaded.
    void ApplyAllTalentPerks();
    // Covenant switching. Every covenant-scoped tree of the sanctum publishes its owner as
    // GarrTalentTree.FeatureSubtypeIndex (= Covenant.db2 id), so the researched talents of the four covenants are
    // already stored as disjoint sets and NOTHING here ever deletes a talent row - only the perks a row grants are
    // scoped to the covenant currently being served. Strips the PerkSpellID of every covenant-scoped tree that is
    // not the player's active covenant and (re)applies the active one's. Idempotent; safe with no covenant at all
    // (everything covenant-scoped is stripped). A no-op for every garrison type except the covenant sanctum.
    void RefreshCovenantTalentPerks();
    // Seat the 14 talents of a covenant's ability tree (GarrTalentTree.FeatureTypeIndex 0). All four ability trees
    // are authored cost 0 / gold 0 / duration 0 with no prerequisites, so LearnTalent puts each straight at rank 1
    // and the class filtering happens through GarrTalentRank.PerkPlayerConditionID - i.e. this is exactly what the
    // retail class + signature grant spells do, without hardcoding their ids. Already-known talents are skipped.
    void GrantCovenantAbilityTalents(uint32 covenantId);

    // Trophy system
    void AddTrophy(uint32 trophyID);
    void RemoveTrophy(uint32 trophyID);
    bool HasTrophy(uint32 trophyID) const { return _trophies.count(trophyID) > 0; }
    std::unordered_set<uint32> const& GetTrophies() const { return _trophies; }

    // Queen's Conservatory - the Night Fae unique sanctum feature (GarrTalentTree 319). Only meaningful on a
    // GARRISON_TYPE_COVENANT garrison owned by a Night Fae character; it reports zero plots for anything else.
    QueensConservatory& GetConservatory() { return _conservatory; }
    QueensConservatory const& GetConservatory() const { return _conservatory; }

    void BuildInfoPacket(WorldPackets::Garrison::GarrisonInfo& garrison) const;
    void SendRemoteInfo() const;
    void SendInfo() const;
    void SendBlueprintAndSpecializationData();
    void SendMapData(Player* receiver) const;
    void SendMissionStartConditionUpdate() const;
    void SendTroopQualityRefresh() const;

    void ResetFollowerActivationLimit() { _followerActivationsRemainingToday = 1; }

    // Anima Conductor channels bought with reservoir anima last until the daily reset; drop the lapsed ones and
    // tell the client. A no-op for every garrison type except the covenant sanctum. Called from Player::DailyReset.
    void ExpireTemporaryChannelAnima();

private:
    Map* FindMap() const;

    // GarrTalentRank.PerkSpellID plumbing. A rank index is the 0-based index into the talent's rank list; a talent
    // sitting at Rank N has completed rank indices [0, N).
    void ApplyTalentRankPerk(uint32 garrTalentID, int32 rankIndex);
    void RemoveTalentRankPerks(uint32 garrTalentID, int32 completedRanks);
    // Channel Anima destinations are gated by their GarrTalent.PlayerConditionID on the Anima Conductor tiers.
    // Returns true for every talent that is not a Channel Anima destination.
    bool IsChannelAnimaTalentAvailable(GarrTalentEntry const* talentEntry) const;
    // True for the six Anima Conductor destinations of a covenant (GarrTalentTree.FeatureTypeIndex 7).
    static bool IsChannelAnimaTalent(GarrTalentEntry const* talentEntry);
    // Charge one Channel Anima selection and take the previous temporary channel down. Returns a GARRISON_*
    // result; on success the caller may seat the talent.
    uint32 TakeChannelAnimaCost(GarrTalentEntry const* talentEntry, bool permanent);
    // Drop one channel, strip its perks, tell the client and delete its row.
    void RemoveChannelAnimaTalent(uint32 garrTalentID);
    // Covenant-scoped sanctum trees (GarrTalentTree.FeatureSubtypeIndex = CovenantID) may only be touched by a
    // member of that covenant. Returns true for every tree that is not covenant-scoped.
    bool IsTalentTreeOwnedByPlayerCovenant(GarrTalentTreeEntry const* treeEntry) const;
    void InitializePlots();
    GarrisonError CheckBuildingPlacement(uint32 garrPlotInstanceId, uint32 garrBuildingId) const;
    GarrisonError CheckBuildingRemoval(uint32 garrPlotInstanceId) const;
    Player* _owner;
    GarrisonType _garrType;
    GarrSiteLevelEntry const* _siteLevel;
    uint32 _followerActivationsRemainingToday;
    uint32 _updateTimer = 0;
    uint32 _garrisonCacheSize = 500;
    time_t _cacheLastUsed = 0; // last time the resource cache was collected (advances by whole intervals)
    uint32 _shipyardBuilding = 0; // WoD Shipyard tier: GarrBuilding 205/206/207 (L1/L2/L3), 0 = not built
    static constexpr uint32 GARRISON_UPDATE_INTERVAL = 60000; // 60 seconds
    static constexpr uint32 CACHE_RESOURCE_INTERVAL = 600;    // WoD rate: 1 Garrison Resource per 10 minutes
    static constexpr uint32 CURRENCY_GARRISON_RESOURCES = 824;
    // WoD Shipyard building tiers (GarrBuilding "Lunarfall/Frostwall Shipyard", BuildingType 9), verified in 12.0.7
    static constexpr uint32 GARRISON_SHIPYARD_BUILDING_L1 = 205;
    static constexpr uint32 GARRISON_SHIPYARD_BUILDING_L2 = 206;
    static constexpr uint32 GARRISON_SHIPYARD_BUILDING_L3 = 207;

    std::unordered_map<uint32 /*garrPlotInstanceId*/, Plot> _plots;
    std::unordered_set<uint32 /*garrBuildingId*/> _knownBuildings;
    std::unordered_set<uint32 /*garrSpecId*/> _knownSpecializations;
    std::unordered_map<uint64 /*dbId*/, Follower> _followers;
    std::unordered_set<uint32> _followerIds;
    std::unordered_map<uint64 /*dbId*/, Mission> _missions;
    uint64 _missionDbIdGenerator = 1;
    time_t _lastMissionGenerationTime = 0;
    uint32 _lastFinishedMissionCount = 0; // #17: re-sends garrison info when a mission's timer completes so the report refreshes
    std::unordered_set<uint32 /*missionRecID*/> _activeMissionRecIDs;
    uint32 _sessionMissionCount = 0;
    uint32 _missionsStartedToday = 0;
    uint32 _lastMissionStartDay = 0; // days since epoch, for daily reset detection
    std::vector<int32> _archivedMissions;

    // Recruitment
    std::vector<WorldPackets::Garrison::GarrisonFollower> _availableRecruits;
    uint32 _recruitmentPreferenceAbilityId = 0;
    uint32 _recruitmentPreferenceTraitId = 0;

    // Shipments
    std::unordered_map<uint64 /*dbId*/, Shipment> _shipments;
    std::unordered_map<uint32 /*containerId*/, uint8> _shownStandardContainers; // standards we've lit up, so they reset to base after collection
    std::unordered_map<uint32 /*containerId*/, ObjectGuid> _privateStandards;   // per-player private "standard" GOs showing THIS owner's order state
    std::unordered_map<uint32 /*garrTalentID*/, Talent> _talents;

    // Trophies
    std::unordered_set<uint32 /*trophyID*/> _trophies;

    // Night Fae unique sanctum feature; inert for every other garrison type.
    QueensConservatory _conservatory;

    // Temporary storage for BuildInfoPacket (mission copies with cleared inline rewards)
    mutable std::vector<WorldPackets::Garrison::GarrisonMission> _infoMissions;
};

#endif // Garrison_h__
