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

#include "GarrisonMgr.h"
#include "Containers.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "Garrison.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Random.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Timer.h"
#include "World.h"

GarrisonMgr::GarrisonMgr() = default;
GarrisonMgr::~GarrisonMgr() = default;

GarrisonMgr& GarrisonMgr::Instance()
{
    static GarrisonMgr instance;
    return instance;
}

void GarrisonMgr::Initialize()
{
    // Build site level index for O(1) lookups by (siteId, level)
    for (GarrSiteLevelEntry const* siteLevel : sGarrSiteLevelStore)
        _garrSiteLevelBySiteAndLevel[std::make_pair(siteLevel->GarrSiteID, siteLevel->GarrLevel)] = siteLevel;

    for (GarrSiteLevelPlotInstEntry const* siteLevelPlotInst : sGarrSiteLevelPlotInstStore)
        _garrisonPlotInstBySiteLevel[siteLevelPlotInst->GarrSiteLevelID].push_back(siteLevelPlotInst);

    for (GameObjectsEntry const* gameObject : sGameObjectsStore)
        if (gameObject->TypeID == GAMEOBJECT_TYPE_GARRISON_PLOT)
            _garrisonPlots[gameObject->OwnerID][gameObject->PropValue[0]] = gameObject;

    for (GarrPlotBuildingEntry const* plotBuilding : sGarrPlotBuildingStore)
        _garrisonBuildingsByPlot[plotBuilding->GarrPlotID].insert(plotBuilding->GarrBuildingID);

    for (GarrBuildingPlotInstEntry const* buildingPlotInst : sGarrBuildingPlotInstStore)
        _garrisonBuildingPlotInstances[std::make_pair(buildingPlotInst->GarrBuildingID, buildingPlotInst->GarrSiteLevelPlotInstID)] = buildingPlotInst->ID;

    for (GarrBuildingEntry const* building : sGarrBuildingStore)
        _garrisonBuildingsByType[building->BuildingType].push_back(building->ID);

    for (GarrFollowerXAbilityEntry const* followerAbility : sGarrFollowerXAbilityStore)
    {
        if (GarrAbilityEntry const* ability = sGarrAbilityStore.LookupEntry(followerAbility->GarrAbilityID))
        {
            // Load per-follower abilities for BOTH WoD garrison (type 1) and Legion class-order hall (type 4)
            // followers. Previously only FOLLOWER_TYPE_GARRISON was allowed, so every Order Hall champion's
            // counter/trait abilities (incl. their class spec, e.g. Marksmanship 365) were dropped here -> they
            // recruited with an empty ability list, countered no mission mechanics, and their spec "didn't count".
            if (ability->GarrFollowerTypeID != FOLLOWER_TYPE_GARRISON && ability->GarrFollowerTypeID != FOLLOWER_TYPE_CLASS_ORDER)
                continue;

            // The generic random-trait fill pool is per follower type - keep it WoD-only so garrison followers
            // never roll a class-order trait (and vice versa). Class-order champions get their fixed abilities from
            // GarrFollowerXAbility below, so they don't draw from a random pool.
            if (ability->GarrFollowerTypeID == FOLLOWER_TYPE_GARRISON
                && !(ability->Flags & GARRISON_ABILITY_CANNOT_ROLL) && ability->Flags & GARRISON_ABILITY_FLAG_TRAIT)
                _garrisonFollowerRandomTraits.insert(ability);

            if (followerAbility->FactionIndex < 2)
            {
                if (ability->Flags & GARRISON_ABILITY_FLAG_TRAIT)
                    _garrisonFollowerAbilities[followerAbility->FactionIndex][followerAbility->GarrFollowerID].Traits.insert(ability);
                else
                    _garrisonFollowerAbilities[followerAbility->FactionIndex][followerAbility->GarrFollowerID].Counters.insert(ability);
            }
        }
    }

    // Build follower level XP index
    for (GarrFollowerLevelXPEntry const* levelXP : sGarrFollowerLevelXPStore)
        _followerLevelXP[std::make_pair(levelXP->GarrFollowerTypeID, levelXP->FollowerLevel)] = levelXP;

    // Build follower quality index
    for (GarrFollowerQualityEntry const* quality : sGarrFollowerQualityStore)
        _followerQuality[std::make_pair(quality->GarrFollowerTypeID, quality->Quality)] = quality;

    // Build mission index by garrison type
    for (GarrMissionEntry const* mission : sGarrMissionStore)
        _missionsByGarrType[mission->GarrTypeID].push_back(mission);

    // Build mission encounter index
    for (GarrMissionXEncounterEntry const* missionEncounter : sGarrMissionXEncounterStore)
        _missionEncounters[missionEncounter->GarrMissionID].push_back(missionEncounter);

    // Build mission required followers index
    for (GarrMissionXFollowerEntry const* missionFollower : sGarrMissionXFollowerStore)
        _missionRequiredFollowers[missionFollower->GarrMissionID].push_back(missionFollower);

    // Build encounter mechanics index (GarrEncounterXMechanic -> GarrMechanic per encounter)
    for (GarrEncounterXMechanicEntry const* encounterMechanic : sGarrEncounterXMechanicStore)
    {
        if (GarrMechanicEntry const* mechanic = sGarrMechanicStore.LookupEntry(encounterMechanic->GarrMechanicID))
            _encounterMechanics[encounterMechanic->GarrEncounterID].push_back(mechanic);
    }

    // Build shipment container index (building type -> container)
    for (CharShipmentContainerEntry const* container : sCharShipmentContainerStore)
        _shipmentContainersByBuildingType[container->GarrBuildingType] = container;

    // Build shipment index (container -> shipments)
    for (CharShipmentEntry const* shipment : sCharShipmentStore)
        _shipmentsByContainer[shipment->ContainerID].push_back(shipment);

    // Build follower type index (garrison type -> primary follower type)
    // Each garrison type has one primary follower type (e.g., type 2 -> follower type 1, type 3 -> follower type 4)
    for (GarrFollowerTypeEntry const* followerType : sGarrFollowerTypeStore)
    {
        // Use first (non-shipyard) follower type per garrison type as primary
        // Shipyard followers (type 2) share garrTypeID 2 with regular followers (type 1)
        auto itr = _followerTypeByGarrType.find(followerType->GarrTypeID);
        if (itr == _followerTypeByGarrType.end() || followerType->ID < itr->second->ID)
            _followerTypeByGarrType[followerType->GarrTypeID] = followerType;
    }

    // Build talent tree index (garrison type -> talent trees)
    for (GarrTalentTreeEntry const* tree : sGarrTalentTreeStore)
        _talentTreesByGarrType[tree->GarrTypeID].push_back(tree);

    // Build talent index (talent tree -> talents)
    for (GarrTalentEntry const* talent : sGarrTalentStore)
        _talentsByTree[talent->GarrTalentTreeID].push_back(talent);

    // Build talent rank index (talent -> ranks, sorted by rank)
    for (GarrTalentRankEntry const* rank : sGarrTalentRankStore)
        _talentRanksByTalent[rank->GarrTalentID].push_back(rank);

    for (auto& [talentId, ranks] : _talentRanksByTalent)
        std::sort(ranks.begin(), ranks.end(), [](GarrTalentRankEntry const* a, GarrTalentRankEntry const* b) { return a->Rank < b->Rank; });

    // Index GarrAbilityEffect by its owning GarrAbility (same shape as _talentsByTree above). This is what lets
    // a GarrTalent.GarrAbilityID-carrying talent (e.g. the Command Table tiers: GarrAbility 1274 'Forward
    // Planning' effect 1844 AbilityAction 14 / GarrAbility 1273 'Strategic Genius' effect 1843 AbilityAction 17)
    // be dispatched data-driven instead of the store being loaded but never read.
    for (GarrAbilityEffectEntry const* effect : sGarrAbilityEffectStore)
        _abilityEffectsByAbility[effect->GarrAbilityID].push_back(effect);

    // Build talent research index (talent tree -> research entry via crossref)
    for (GarrTalTreeXGarrTalResearchEntry const* xref : sGarrTalTreeXGarrTalResearchStore)
    {
        if (GarrTalentResearchEntry const* research = sGarrTalentResearchStore.LookupEntry(xref->GarrTalentResearchID))
            _talentResearchByTree[xref->GarrTalentTreeID] = research;
    }

    // Auto-combat indices
    for (GarrAutoSpellEffectEntry const* effect : sGarrAutoSpellEffectStore)
        _autoSpellEffects[effect->GarrAutoSpellID].push_back(effect);

    for (GarrEncounterSetXEncounterEntry const* xref : sGarrEncounterSetXEncounterStore)
        _encounterSetEncounters[xref->GarrEncounterSetID].push_back(xref->GarrEncounterID);

    // The encounter -> statline link lives on GarrEncounter.AutoCombatantID; GarrAutoCombatant has
    // no back-reference to an encounter. 251 of 2626 encounters carry one and every one of them
    // belongs to a GarrTypeID 111 (Shadowlands Adventures) mission.
    for (GarrEncounterEntry const* encounter : sGarrEncounterStore)
        if (encounter->AutoCombatantID != 0)
            if (GarrAutoCombatantEntry const* combatant = sGarrAutoCombatantStore.LookupEntry(encounter->AutoCombatantID))
                _autoCombatantByEncounter[encounter->ID] = combatant;

    InitializeDbIdSequences();
    LoadPlotFinalizeGOInfo();
    LoadFollowerClassSpecAbilities();
    LoadMissionRewards();
    LoadOrderHallShipments();
    LoadOrderHallStandards();
    LoadConservatoryWildseeds();
    LoadConservatoryCatalysts();
    LoadConservatoryYields();
    LoadAbominationRecipes();
    LoadAscensionMemories();
    LoadEmberCourtGuests();
    LoadTransportNetworkSpells();
}

// Transport Network (trees 308/309/307/310). The 12 talents are category (c) in the covenant sanctum audit:
// every rank row publishes PerkSpellID 0 / GarrAbilityID 0 / Points 0, so the client data names NO effect at
// all - the talents' own descriptions name destinations, and matching standalone teleport/taxi spells exist in
// the build but nothing links them. That link is therefore authored content (garrison_transport_network), and
// every authored row is validated here exactly like a rank perk would be: the talent must exist and belong to
// a Transport Network tree, and the spell must exist.
void GarrisonMgr::LoadTransportNetworkSpells()
{
    _transportNetworkSpells.clear();

    QueryResult result = WorldDatabase.Query("SELECT garrTalentId, spellId FROM garrison_transport_network");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 transport network spells. DB table `garrison_transport_network` is empty - "
            "researching a Transport Network tier will grant nothing until it is authored.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 talentId = fields[0].GetUInt32();
        uint32 spellId = fields[1].GetUInt32();

        GarrTalentEntry const* talentEntry = sGarrTalentStore.LookupEntry(talentId);
        if (!talentEntry)
        {
            TC_LOG_ERROR("sql.sql", "Non-existing GarrTalent.db2 entry {} referenced in `garrison_transport_network` (spellId {}); skipped.", talentId, spellId);
            continue;
        }

        GarrTalentTreeEntry const* treeEntry = sGarrTalentTreeStore.LookupEntry(talentEntry->GarrTalentTreeID);
        if (!treeEntry || treeEntry->FeatureTypeIndex != GARR_TALENT_FEATURE_TRANSPORT_NETWORK)
        {
            TC_LOG_ERROR("sql.sql", "GarrTalent {} referenced in `garrison_transport_network` is not a Transport Network talent "
                "(tree {}, FeatureTypeIndex {}); skipped.", talentId, talentEntry->GarrTalentTreeID, treeEntry ? treeEntry->FeatureTypeIndex : -1);
            continue;
        }

        if (!sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing spell {} referenced in `garrison_transport_network` (garrTalentId {}); skipped.", spellId, talentId);
            continue;
        }

        _transportNetworkSpells[talentId].push_back(spellId);
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} transport network spell grants.", count);
}

std::vector<uint32> const* GarrisonMgr::GetTransportNetworkSpells(uint32 garrTalentId) const
{
    auto itr = _transportNetworkSpells.find(garrTalentId);
    return itr != _transportNetworkSpells.end() ? &itr->second : nullptr;
}

// Class-hall / order-hall (and any non-plot garrison) troop recruiters aren't garrison plot buildings, so the
// WoD building-type container index can't resolve them (all GarrTypeID 3 containers have GarrBuildingType 0 and
// collide). This world table maps a recruiter creature entry -> its CharShipmentContainer (whose CharShipment
// rows carry a GarrFollowerID = the recruited troop). See Garrison::CreateTroopShipment.
void GarrisonMgr::LoadOrderHallShipments()
{
    _orderHallContainerByNpc.clear();
    _orderHallGateByNpc.clear();
    _recruiterByContainer.clear();

    QueryResult result = WorldDatabase.Query("SELECT npcEntry, containerId, requiredTalentId, weeklyLimit FROM garrison_order_hall_shipment");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 order-hall troop recruiters. DB table `garrison_order_hall_shipment` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 npcEntry = fields[0].GetUInt32();
        uint32 containerId = fields[1].GetUInt32();
        uint32 requiredTalentId = fields[2].GetUInt32();
        uint32 weeklyLimit = fields[3].GetUInt32();

        CharShipmentContainerEntry const* container = sCharShipmentContainerStore.LookupEntry(containerId);
        if (!container)
        {
            TC_LOG_ERROR("sql.sql", "Non-existing CharShipmentContainer.db2 entry {} referenced in `garrison_order_hall_shipment` (npcEntry {}); skipped.", containerId, npcEntry);
            continue;
        }

        _orderHallContainerByNpc[npcEntry] = container;
        _recruiterByContainer[containerId] = npcEntry;
        if (requiredTalentId || weeklyLimit)
        {
            OrderHallShipmentGate& gate = _orderHallGateByNpc[npcEntry];
            gate.RequiredTalentId = requiredTalentId;
            gate.WeeklyLimit = weeklyLimit;
        }
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} order-hall troop recruiter -> container mappings.", count);

    // Map each CharShipmentContainer to its "standard" GameObject (GAMEOBJECT_TYPE_GARRISON_SHIPMENT), so finished
    // plotless orders can light up the standard's ready/working display (Garrison::UpdateOrderHallStandards).
    _orderHallStandardGoByContainer.clear();
    for (auto const& [goEntry, goTemplate] : sObjectMgr->GetGameObjectTemplates())
        if (goTemplate.type == GAMEOBJECT_TYPE_GARRISON_SHIPMENT && goTemplate.garrisonShipment.ShipmentContainer)
            _orderHallStandardGoByContainer.emplace(goTemplate.garrisonShipment.ShipmentContainer, goEntry);
}

CharShipmentContainerEntry const* GarrisonMgr::GetShipmentContainerForNpc(uint32 creatureEntry) const
{
    auto itr = _orderHallContainerByNpc.find(creatureEntry);
    return itr != _orderHallContainerByNpc.end() ? itr->second : nullptr;
}

uint32 GarrisonMgr::GetStandardGoForContainer(uint32 containerId) const
{
    auto itr = _orderHallStandardGoByContainer.find(containerId);
    return itr != _orderHallStandardGoByContainer.end() ? itr->second : 0;
}

// Queen's Conservatory wildseed kinds. The 12.0.7 client publishes the Conservatory's unlock ladder
// (GarrTalentTree 319 + GarrTalent 1086-1090 + their GarrTalentRank/GarrTalentCost rows) and the harvest
// reward (GameObject 350978 "Queen's Conservatory Cache" -> gameobject_loot_template 350978), but nothing
// anywhere describes what an individual wildseed costs, how long it takes to mature, or which catalyst
// combination changes the yield. Those are therefore authored here instead of guessed in code. An empty
// table is a valid state: QueensConservatory::PlantWildseed then answers CONSERVATORY_ERROR_NO_WILDSEED_DATA
// and nothing else about the sanctum changes.
void GarrisonMgr::LoadConservatoryWildseeds()
{
    _conservatoryWildseeds.clear();

    QueryResult result = WorldDatabase.Query("SELECT wildseedEntry, costCurrencyId, costCurrencyCount, costItemId, "
        "costItemCount, maturationSeconds, rewardGameObjectId, requiredTier FROM garrison_conservatory_wildseed");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 Queen's Conservatory wildseed kinds. DB table "
            "`garrison_conservatory_wildseed` is empty - planting stays disabled until it is authored.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        ConservatoryWildseedTemplate wildseed;
        wildseed.WildseedEntry      = fields[0].GetUInt32();
        wildseed.CostCurrencyId     = fields[1].GetUInt32();
        wildseed.CostCurrencyCount  = fields[2].GetUInt32();
        wildseed.CostItemId         = fields[3].GetUInt32();
        wildseed.CostItemCount      = fields[4].GetUInt32();
        wildseed.MaturationSeconds  = fields[5].GetUInt32();
        wildseed.RewardGameObjectId = fields[6].GetUInt32();
        wildseed.RequiredTier       = fields[7].GetUInt8();

        if (!wildseed.WildseedEntry)
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_conservatory_wildseed` has a row with wildseedEntry 0; skipped.");
            continue;
        }

        if (wildseed.CostCurrencyId && !sCurrencyTypesStore.LookupEntry(wildseed.CostCurrencyId))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing CurrencyTypes.db2 entry {} referenced in "
                "`garrison_conservatory_wildseed` (wildseedEntry {}); cost cleared.", wildseed.CostCurrencyId, wildseed.WildseedEntry);
            wildseed.CostCurrencyId = 0;
            wildseed.CostCurrencyCount = 0;
        }

        if (wildseed.CostItemId && !sObjectMgr->GetItemTemplate(wildseed.CostItemId))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing item {} referenced in `garrison_conservatory_wildseed` "
                "(wildseedEntry {}); cost cleared.", wildseed.CostItemId, wildseed.WildseedEntry);
            wildseed.CostItemId = 0;
            wildseed.CostItemCount = 0;
        }

        if (!wildseed.RewardGameObjectId)
            wildseed.RewardGameObjectId = CONSERVATORY_DEFAULT_REWARD_GO;

        if (!sObjectMgr->GetGameObjectTemplate(wildseed.RewardGameObjectId))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing gameobject_template {} referenced as rewardGameObjectId in "
                "`garrison_conservatory_wildseed` (wildseedEntry {}); row skipped.", wildseed.RewardGameObjectId, wildseed.WildseedEntry);
            continue;
        }

        if (!wildseed.MaturationSeconds)
            TC_LOG_ERROR("sql.sql", "Table `garrison_conservatory_wildseed` row {} has maturationSeconds 0; "
                "it will be refused at plant time.", wildseed.WildseedEntry);

        if (!wildseed.RequiredTier || wildseed.RequiredTier > CONSERVATORY_MAX_TIERS)
            wildseed.RequiredTier = 1;

        _conservatoryWildseeds[wildseed.WildseedEntry] = wildseed;
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} Queen's Conservatory wildseed kinds.", count);
}

ConservatoryWildseedTemplate const* GarrisonMgr::GetConservatoryWildseed(uint32 wildseedEntry) const
{
    auto itr = _conservatoryWildseeds.find(wildseedEntry);
    return itr != _conservatoryWildseeds.end() ? &itr->second : nullptr;
}

// Queen's Conservatory catalysts. Unlike the wildseed kinds, WHAT a catalyst does is published by the client:
// the three items 176921 Temporal Leaves / 176922 Wild Nightbloom / 176832 Wildseed Root Grain share the Use
// spell 323169 "Infuse Catalyst" and each one's own description states its effect verbatim (quoted in
// QueensConservatory.h and in world migration 2026_08_07_63). What is NOT published is the effect's magnitude
// in server terms and the loot table each combination pays out, so the whole set is a world table and no item
// id is compiled in. An empty table is valid - AttachCatalyst then refuses with CONSERVATORY_ERROR_NO_CATALYST_DATA
// instead of recording a link that changes nothing, which is precisely the silent no-op this table replaces.
void GarrisonMgr::LoadConservatoryCatalysts()
{
    _conservatoryCatalysts.clear();

    QueryResult result = WorldDatabase.Query("SELECT catalystItemId, effectType, effectValue, maxPerPlot, spellId "
        "FROM garrison_conservatory_catalyst");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 Queen's Conservatory catalysts. DB table "
            "`garrison_conservatory_catalyst` is empty - linking catalysts stays disabled until it is authored.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        ConservatoryCatalystTemplate catalyst;
        catalyst.CatalystItemId = fields[0].GetUInt32();
        catalyst.EffectType     = ConservatoryCatalystEffect(fields[1].GetUInt8());
        catalyst.EffectValue    = fields[2].GetInt32();
        catalyst.MaxPerPlot     = fields[3].GetUInt8();
        catalyst.SpellId        = fields[4].GetUInt32();

        if (!catalyst.CatalystItemId)
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_conservatory_catalyst` has a row with catalystItemId 0; skipped.");
            continue;
        }

        // The item has to be a real one or the link could never be paid for.
        if (!sObjectMgr->GetItemTemplate(catalyst.CatalystItemId))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing item {} referenced as catalystItemId in "
                "`garrison_conservatory_catalyst`; row skipped.", catalyst.CatalystItemId);
            continue;
        }

        if (catalyst.EffectType == CONSERVATORY_CATALYST_EFFECT_NONE || catalyst.EffectType >= CONSERVATORY_CATALYST_EFFECT_MAX)
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_conservatory_catalyst` row {} has effectType {}, which is not "
                "one of 1 TIME_DELTA / 2 YIELD_QUALITY / 3 YIELD_QUANTITY; row skipped rather than linked as a "
                "catalyst that would do nothing.", catalyst.CatalystItemId, uint32(catalyst.EffectType));
            continue;
        }

        // A TIME_DELTA of zero would move no clock, i.e. exactly the no-op this table exists to prevent.
        if (catalyst.EffectType == CONSERVATORY_CATALYST_EFFECT_TIME_DELTA && !catalyst.EffectValue)
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_conservatory_catalyst` row {} is TIME_DELTA with effectValue 0 "
                "and would change nothing; row skipped.", catalyst.CatalystItemId);
            continue;
        }

        if (catalyst.SpellId && !sSpellMgr->GetSpellInfo(catalyst.SpellId, DIFFICULTY_NONE))
            TC_LOG_ERROR("sql.sql", "Table `garrison_conservatory_catalyst` row {} references spell {}, which is not "
                "in the spell store. The column is provenance only, so the row is kept.",
                catalyst.CatalystItemId, catalyst.SpellId);

        if (!catalyst.MaxPerPlot || catalyst.MaxPerPlot > CONSERVATORY_MAX_CATALYSTS)
            catalyst.MaxPerPlot = CONSERVATORY_MAX_CATALYSTS;

        _conservatoryCatalysts[catalyst.CatalystItemId] = catalyst;
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} Queen's Conservatory catalysts.", count);
}

ConservatoryCatalystTemplate const* GarrisonMgr::GetConservatoryCatalyst(uint32 catalystItemId) const
{
    auto itr = _conservatoryCatalysts.find(catalystItemId);
    return itr != _conservatoryCatalysts.end() ? &itr->second : nullptr;
}

// Which gameobject_loot_template a harvest rolls for a given catalyst combination. `gameobject_loot_template`
// 350978 - the "Queen's Conservatory Cache" chest - flattens every outcome into one table, so rolling it means
// every harvest can produce every satchel tier and size no matter what was linked to the pod. This table is
// what makes the catalysts matter. A combination with no row is a REFUSAL, never a fallback to another table;
// see QueensConservatory::ResolveHarvestLootId.
void GarrisonMgr::LoadConservatoryYields()
{
    _conservatoryYields.clear();

    QueryResult result = WorldDatabase.Query("SELECT spiritItemId, rootGrainCount, nightbloomCount, lootId "
        "FROM garrison_conservatory_yield");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 Queen's Conservatory yield rows. DB table "
            "`garrison_conservatory_yield` is empty - harvests fall back to the wildseed's reward chest.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        ConservatoryYieldKey key;
        key.SpiritItemId    = fields[0].GetUInt32();
        key.RootGrainCount  = fields[1].GetUInt8();
        key.NightbloomCount = fields[2].GetUInt8();
        uint32 const lootId = fields[3].GetUInt32();

        if (!lootId)
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_conservatory_yield` row ({}, {}, {}) has lootId 0; skipped.",
                key.SpiritItemId, uint32(key.RootGrainCount), uint32(key.NightbloomCount));
            continue;
        }

        if (key.RootGrainCount > CONSERVATORY_MAX_CATALYSTS || key.NightbloomCount > CONSERVATORY_MAX_CATALYSTS
            || key.RootGrainCount + key.NightbloomCount > CONSERVATORY_MAX_CATALYSTS)
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_conservatory_yield` row ({}, {}, {}) needs more catalyst links "
                "than any wildseed pod has ({}); skipped.", key.SpiritItemId, uint32(key.RootGrainCount),
                uint32(key.NightbloomCount), uint32(CONSERVATORY_MAX_CATALYSTS));
            continue;
        }

        if (key.SpiritItemId && !_conservatoryWildseeds.count(key.SpiritItemId))
            TC_LOG_ERROR("sql.sql", "Table `garrison_conservatory_yield` row ({}, {}, {}) names a spiritItemId with "
                "no `garrison_conservatory_wildseed` row; it can never be selected.", key.SpiritItemId,
                uint32(key.RootGrainCount), uint32(key.NightbloomCount));

        _conservatoryYields[key] = lootId;
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} Queen's Conservatory yield rows.", count);
}

uint32 GarrisonMgr::GetConservatoryYieldLootId(uint32 spiritItemId, uint8 rootGrainCount, uint8 nightbloomCount) const
{
    // A spirit-specific row wins over the wildcard, so a single spirit can be re-pointed without restating the
    // whole grid.
    if (spiritItemId)
    {
        auto itr = _conservatoryYields.find({ spiritItemId, rootGrainCount, nightbloomCount });
        if (itr != _conservatoryYields.end())
            return itr->second;
    }

    auto itr = _conservatoryYields.find({ 0, rootGrainCount, nightbloomCount });
    return itr != _conservatoryYields.end() ? itr->second : 0;
}

// Abomination Factory (Necrolord unique sanctum feature, GarrTalentTree 321). Two of the three things this needs
// come straight out of the client:
//   * the recipe set  - the 66 SkillLineAbility rows of SkillLine 2787 "Abominable Stitching";
//   * which of them build a construct - exactly the ones whose spell carries SPELL_EFFECT_KILL_CREDIT (the
//     "Construct Body: X" spells credit creature 167076 / 167581). That is 15 of the 66 and nothing else in the
//     skill, so the stable roster never needs a hardcoded id list.
// The third - which researched tier of tree 321 unlocks each recipe - is published nowhere: every one of the 66
// rows has MinSkillLineRank 1, TradeSkillCategory groups them by kind rather than rank, and no PlayerCondition in
// the build mentions talents 1096-1100. That mapping is therefore authored in `garrison_abomination_recipe`. An
// empty table is a valid state: the skill line is still granted and ranked, but no recipe is taught.
void GarrisonMgr::LoadAbominationRecipes()
{
    _abominationStitchingSpells.clear();
    _abominationConstructSpells.clear();
    _abominationRecipes.clear();

    if (std::vector<SkillLineAbilityEntry const*> const* abilities = sDB2Manager.GetSkillLineAbilitiesBySkill(SKILL_ABOMINABLE_STITCHING))
    {
        for (SkillLineAbilityEntry const* ability : *abilities)
        {
            _abominationStitchingSpells.insert(ability->Spell);

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(ability->Spell, DIFFICULTY_NONE);
            if (!spellInfo)
                continue;

            for (SpellEffectInfo const& effect : spellInfo->GetEffects())
            {
                if (effect.IsEffect(SPELL_EFFECT_KILL_CREDIT))
                {
                    _abominationConstructSpells.insert(ability->Spell);
                    break;
                }
            }
        }
    }

    TC_LOG_INFO("server.loading", ">> Abominable Stitching (SkillLine {}): {} client recipe(s), {} of them construct bodies.",
        uint32(SKILL_ABOMINABLE_STITCHING), uint32(_abominationStitchingSpells.size()), uint32(_abominationConstructSpells.size()));

    QueryResult result = WorldDatabase.Query("SELECT spellId, requiredRank FROM garrison_abomination_recipe");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 Abomination Factory recipe unlocks. DB table "
            "`garrison_abomination_recipe` is empty - no stitching recipe is taught until it is authored.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        AbominationRecipeTemplate recipe;
        recipe.RecipeSpellId = fields[0].GetUInt32();
        recipe.RequiredRank  = fields[1].GetUInt8();

        // The recipe list is not authorable - a row may only gate a spell the client already publishes as an
        // Abominable Stitching recipe. This keeps content from inventing a stitching recipe that does not exist.
        if (!_abominationStitchingSpells.count(recipe.RecipeSpellId))
        {
            TC_LOG_ERROR("sql.sql", "Spell {} referenced in `garrison_abomination_recipe` is not a SkillLineAbility "
                "of SkillLine {} (Abominable Stitching); row skipped.", recipe.RecipeSpellId, uint32(SKILL_ABOMINABLE_STITCHING));
            continue;
        }

        if (!recipe.RequiredRank || recipe.RequiredRank > ABOMINATION_FACTORY_MAX_RANK)
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_abomination_recipe` row for spell {} has requiredRank {} "
                "outside 1-{}; row skipped.", recipe.RecipeSpellId, uint32(recipe.RequiredRank), uint32(ABOMINATION_FACTORY_MAX_RANK));
            continue;
        }

        _abominationRecipes[recipe.RecipeSpellId] = recipe;
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} Abomination Factory recipe unlocks.", count);
}

bool GarrisonMgr::IsAbominationStitchingRecipe(uint32 spellId) const
{
    return _abominationStitchingSpells.count(spellId) != 0;
}

bool GarrisonMgr::IsAbominationConstructRecipe(uint32 spellId) const
{
    return _abominationConstructSpells.count(spellId) != 0;
}

uint8 GarrisonMgr::GetAbominationRecipeRank(uint32 spellId) const
{
    auto itr = _abominationRecipes.find(spellId);
    return itr != _abominationRecipes.end() ? itr->second.RequiredRank : uint8(0);
}

// Path of Ascension (Kyrian unique sanctum feature, GarrTalentTree 320).
//
// Almost the whole feature is published by the 12.0.7.68275 client and is read straight out of it - the five
// talents 1091-1095 and their costs, the four trial difficulties 168-171 ("Path of Ascension: Courage /
// Loyalty / Wisdom / Humility"), scenario 1803 on map 2375, and the ten memory quests already sitting in
// `integ_world` under QuestSortID -595 with their kill-credit objectives and reward items. What NO row in the
// build states is which memory is one of the SIX the first tier captures versus the FOUR the second adds, and
// which memories are the "some" that gain a second trial at tier 2 versus "the rest" at tier 3. That mapping
// is content, so it is authored here instead of being invented in C++; an empty table is a valid state and
// simply leaves PathOfAscension::CaptureMemory answering ASCENSION_ERROR_NO_MEMORY_DATA.
void GarrisonMgr::LoadAscensionMemories()
{
    _ascensionMemories.clear();
    _ascensionArenaAuthored = false;

    // Does this world DB actually instantiate the Ascension Coliseum? Scenario 1803 and map 2375 are real
    // client data, but a scenario only runs if `scenarios` names it for the map AND the map has something in
    // it. Both halves are checked so a half-authored arena cannot report itself as playable.
    if (QueryResult arena = WorldDatabase.PQuery("SELECT "
        "(SELECT COUNT(*) FROM scenarios WHERE map = {}), "
        "(SELECT COUNT(*) FROM creature WHERE map = {})", uint32(ASCENSION_MAP_ID), uint32(ASCENSION_MAP_ID)))
    {
        Field* fields = arena->Fetch();
        _ascensionArenaAuthored = fields[0].GetUInt64() > 0 && fields[1].GetUInt64() > 0;
    }

    if (!_ascensionArenaAuthored)
        TC_LOG_INFO("server.loading", ">> Path of Ascension: the Ascension Coliseum (scenario {} on map {}) is not "
            "authored in this world DB - no `scenarios` row and/or no spawns. Trials will be refused rather than "
            "started.", uint32(ASCENSION_SCENARIO_ID), uint32(ASCENSION_MAP_ID));

    QueryResult result = WorldDatabase.Query("SELECT memoryId, creatureId, captureQuestId, requiredTier, "
        "courageTier, loyaltyTier, wisdomTier, humilityTier FROM garrison_ascension_memory");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 Path of Ascension memories. DB table `garrison_ascension_memory` "
            "is empty - no memory can be captured until the roster is authored.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        AscensionMemoryTemplate memory;
        memory.MemoryId       = fields[0].GetUInt32();
        memory.CreatureId     = fields[1].GetUInt32();
        memory.CaptureQuestId = fields[2].GetUInt32();
        memory.RequiredTier   = fields[3].GetUInt8();
        memory.CourageTier    = fields[4].GetUInt8();
        memory.LoyaltyTier    = fields[5].GetUInt8();
        memory.WisdomTier     = fields[6].GetUInt8();
        memory.HumilityTier   = fields[7].GetUInt8();

        // A memory must name a creature that exists: CompleteTrial credits it, and that credit is what makes
        // the memory's own quest pay out. Without it a completion would silently reward nothing.
        if (!sObjectMgr->GetCreatureTemplate(memory.CreatureId))
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_ascension_memory` row {} names creatureId {} which has no "
                "creature_template; row skipped.", memory.MemoryId, memory.CreatureId);
            continue;
        }

        if (memory.CaptureQuestId && !sObjectMgr->GetQuestTemplate(memory.CaptureQuestId))
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_ascension_memory` row {} names captureQuestId {} which does "
                "not exist; row skipped.", memory.MemoryId, memory.CaptureQuestId);
            continue;
        }

        if (!memory.RequiredTier || memory.RequiredTier > ASCENSION_MAX_TIERS)
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_ascension_memory` row {} has requiredTier {} outside 1-{}; "
                "row skipped.", memory.MemoryId, uint32(memory.RequiredTier), uint32(ASCENSION_MAX_TIERS));
            continue;
        }

        bool badTrialTier = false;
        for (uint8 trial = ASCENSION_TRIAL_COURAGE; trial <= ASCENSION_TRIAL_MAX; ++trial)
        {
            uint8 const trialTier = memory.GetTrialTier(AscensionTrial(trial));
            if (trialTier > ASCENSION_MAX_TIERS)
            {
                TC_LOG_ERROR("sql.sql", "Table `garrison_ascension_memory` row {} opens {} at researched tier {}, "
                    "outside 0-{}; row skipped.", memory.MemoryId, PathOfAscension::GetTrialName(AscensionTrial(trial)),
                    uint32(trialTier), uint32(ASCENSION_MAX_TIERS));
                badTrialTier = true;
                break;
            }

            // A trial cannot open before the memory can be captured - that combination could never be entered.
            if (trialTier && trialTier < memory.RequiredTier)
            {
                TC_LOG_ERROR("sql.sql", "Table `garrison_ascension_memory` row {} opens {} at tier {} but the memory "
                    "is only capturable at tier {}; row skipped.", memory.MemoryId,
                    PathOfAscension::GetTrialName(AscensionTrial(trial)), uint32(trialTier), uint32(memory.RequiredTier));
                badTrialTier = true;
                break;
            }
        }

        if (badTrialTier)
            continue;

        _ascensionMemories[memory.MemoryId] = memory;
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} Path of Ascension memories.", count);
}

AscensionMemoryTemplate const* GarrisonMgr::GetAscensionMemory(uint32 memoryId) const
{
    auto itr = _ascensionMemories.find(memoryId);
    return itr != _ascensionMemories.end() ? &itr->second : nullptr;
}

// The Ember Court (Venthyr unique sanctum feature, GarrTalentTree 324).
//
// Almost the whole feature is published by the 12.0.7.68275 client and is read straight out of it - the five
// talents 1111-1115 with their guest-slot wording, scenario 1791 and its four steps, AreaTable 13329
// "SinfallScenario", the sixteen guests (CriteriaTree 87983 cross-checked against the sixteen "RSVP: <Guest>"
// quests in `integ_world`), and the five attribute axes (CriteriaTree 88024-88033 + UiWidgetVisualization
// 1435-1440), the five-rung mood ladder (SpellName 327199/327200/327201/327781/327202 + the "Mood: <Rung>"
// strings in UiWidgetStringSource), and every guest's LIKES (that guest's ItemSparse mood-icon item, whose
// Description_lang is a literal "Likes: <poles>" list). The single thing NO row in the build states is what
// each guest DISLIKES - only "Likes:" strings exist - so that, and only that, is authored here. An empty
// table is a perfectly normal state: a guest simply has no dislike, and nothing is inferred from its likes.
void GarrisonMgr::LoadEmberCourtGuests()
{
    _emberCourtGuests.clear();
    _emberCourtVenueAuthored = false;

    // Does this world DB actually instantiate the Ember Court? A scenario only runs if `scenarios` names it
    // for the map AND the venue has something in it. Both halves are checked so a half-authored Court cannot
    // report itself as playable.
    if (QueryResult venue = WorldDatabase.PQuery("SELECT "
        "(SELECT COUNT(*) FROM scenarios WHERE map = {}), "
        "(SELECT COUNT(*) FROM creature WHERE areaId = {})", uint32(EMBER_COURT_MAP_ID), uint32(EMBER_COURT_AREA_ID)))
    {
        Field* fields = venue->Fetch();
        _emberCourtVenueAuthored = fields[0].GetUInt64() > 0 && fields[1].GetUInt64() > 0;
    }

    if (!_emberCourtVenueAuthored)
        TC_LOG_INFO("server.loading", ">> The Ember Court: the venue (scenario {} in area {} on map {}) is not "
            "authored in this world DB - no `scenarios` row and/or no spawns. Courts will be refused rather "
            "than started.", uint32(EMBER_COURT_SCENARIO_ID), uint32(EMBER_COURT_AREA_ID), uint32(EMBER_COURT_MAP_ID));

    QueryResult result = WorldDatabase.Query("SELECT guestIndex, dislikedAttribute, dislikedPole "
        "FROM garrison_ember_court_guest");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 Ember Court guest dislikes. DB table "
            "`garrison_ember_court_guest` is empty - the 12.0.7 build publishes no dislikes, so this is the "
            "expected state; every guest's LIKES are client data and are already loaded.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        EmberCourtGuestTemplate guest;
        guest.GuestIndex        = fields[0].GetUInt8();
        guest.DislikedAttribute = fields[1].GetUInt8();
        guest.DislikedPole      = fields[2].GetUInt8();

        // The roster is fixed at sixteen by the client (CriteriaTree 87983 has exactly sixteen children); an
        // index outside it names nobody.
        if (!EmberCourt::GetGuestInfo(guest.GuestIndex))
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_ember_court_guest` has guestIndex {} but the client roster "
                "(CriteriaTree 87983) only has {} guests, 0-{}; row skipped.", uint32(guest.GuestIndex),
                uint32(EMBER_COURT_GUEST_COUNT), uint32(EMBER_COURT_GUEST_COUNT - 1));
            continue;
        }

        struct { uint8 attribute; uint8 pole; char const* what; } const axes[1] =
        {
            { guest.DislikedAttribute, guest.DislikedPole, "disliked" }
        };

        bool badAttribute = false;
        for (auto const& [attribute, pole, what] : axes)
        {
            if (attribute > EMBER_COURT_ATTRIBUTE_MAX)
            {
                TC_LOG_ERROR("sql.sql", "Table `garrison_ember_court_guest` row {} has {}Attribute {} outside "
                    "0-{}; row skipped.", uint32(guest.GuestIndex), what, uint32(attribute),
                    uint32(EMBER_COURT_ATTRIBUTE_MAX));
                badAttribute = true;
                break;
            }

            if (pole > EMBER_COURT_POLE_HIGH)
            {
                TC_LOG_ERROR("sql.sql", "Table `garrison_ember_court_guest` row {} has {}Pole {} outside 0-2 "
                    "(0 none, 1 low, 2 high); row skipped.", uint32(guest.GuestIndex), what, uint32(pole));
                badAttribute = true;
                break;
            }

            // An axis without an end, or an end without an axis, cannot be evaluated either way.
            if ((attribute == EMBER_COURT_ATTRIBUTE_NONE) != (pole == EMBER_COURT_POLE_NONE))
            {
                TC_LOG_ERROR("sql.sql", "Table `garrison_ember_court_guest` row {} sets {}Attribute {} with "
                    "{}Pole {} - an attribute and a pole must both be set or both be 0; row skipped.",
                    uint32(guest.GuestIndex), what, uint32(attribute), what, uint32(pole));
                badAttribute = true;
                break;
            }
        }

        if (badAttribute)
            continue;

        // A guest cannot dislike the very pole its own client-published "Likes:" list names.
        if (guest.DislikedAttribute != EMBER_COURT_ATTRIBUTE_NONE
            && EmberCourt::IsAttributeLiked(guest.GuestIndex, EmberCourtAttribute(guest.DislikedAttribute),
                EmberCourtAttributePole(guest.DislikedPole)))
        {
            TC_LOG_ERROR("sql.sql", "Table `garrison_ember_court_guest` row {} makes the guest dislike {} {}, "
                "but the client's own 'Likes:' list for them says they like it; row skipped.",
                uint32(guest.GuestIndex),
                EmberCourt::GetAttributePoleName(EmberCourtAttribute(guest.DislikedAttribute), EmberCourtAttributePole(guest.DislikedPole)),
                EmberCourt::GetAttributeName(EmberCourtAttribute(guest.DislikedAttribute)));
            continue;
        }

        _emberCourtGuests[guest.GuestIndex] = guest;
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} Ember Court guest dislikes.", count);
}

EmberCourtGuestTemplate const* GarrisonMgr::GetEmberCourtGuest(uint8 guestIndex) const
{
    auto itr = _emberCourtGuests.find(guestIndex);
    return itr != _emberCourtGuests.end() ? &itr->second : nullptr;
}

void GarrisonMgr::LoadOrderHallStandards()
{
    _orderHallStandardByContainer.clear();

    QueryResult result = WorldDatabase.Query("SELECT containerId, goEntry, map, posX, posY, posZ, orientation FROM garrison_order_hall_standard");
    if (!result)
        return;

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        OrderHallStandard& standard = _orderHallStandardByContainer[fields[0].GetUInt32()];
        standard.GoEntry = fields[1].GetUInt32();
        standard.MapId = fields[2].GetUInt32();
        standard.Pos.Relocate(fields[3].GetFloat(), fields[4].GetFloat(), fields[5].GetFloat(), fields[6].GetFloat());
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} order-hall standard spawn points.", count);
}

GarrisonMgr::OrderHallStandard const* GarrisonMgr::GetOrderHallStandard(uint32 containerId) const
{
    auto itr = _orderHallStandardByContainer.find(containerId);
    return itr != _orderHallStandardByContainer.end() ? &itr->second : nullptr;
}

uint32 GarrisonMgr::GetRecruiterForContainer(uint32 containerId) const
{
    auto itr = _recruiterByContainer.find(containerId);
    return itr != _recruiterByContainer.end() ? itr->second : 0;
}

GarrisonMgr::OrderHallShipmentGate const* GarrisonMgr::GetOrderHallShipmentGate(uint32 creatureEntry) const
{
    auto itr = _orderHallGateByNpc.find(creatureEntry);
    return itr != _orderHallGateByNpc.end() ? &itr->second : nullptr;
}

GarrSiteLevelEntry const* GarrisonMgr::GetGarrSiteLevelEntry(uint32 garrSiteId, uint32 level) const
{
    auto itr = _garrSiteLevelBySiteAndLevel.find(std::make_pair(garrSiteId, level));
    if (itr != _garrSiteLevelBySiteAndLevel.end())
        return itr->second;

    return nullptr;
}

std::vector<GarrSiteLevelPlotInstEntry const*> const* GarrisonMgr::GetGarrPlotInstForSiteLevel(uint32 garrSiteLevelId) const
{
    auto itr = _garrisonPlotInstBySiteLevel.find(garrSiteLevelId);
    if (itr != _garrisonPlotInstBySiteLevel.end())
        return &itr->second;

    return nullptr;
}

GameObjectsEntry const* GarrisonMgr::GetPlotGameObject(uint32 mapId, uint32 garrPlotInstanceId) const
{
    auto mapItr = _garrisonPlots.find(mapId);
    if (mapItr != _garrisonPlots.end())
    {
        auto plotItr = mapItr->second.find(garrPlotInstanceId);
        if (plotItr != mapItr->second.end())
            return plotItr->second;
    }

    return nullptr;
}

bool GarrisonMgr::IsPlotMatchingBuilding(uint32 garrPlotId, uint32 garrBuildingId) const
{
    auto plotItr = _garrisonBuildingsByPlot.find(garrPlotId);
    if (plotItr != _garrisonBuildingsByPlot.end())
        return plotItr->second.count(garrBuildingId) > 0;

    return false;
}

uint32 GarrisonMgr::GetGarrBuildingPlotInst(uint32 garrBuildingId, uint32 garrSiteLevelPlotInstId) const
{
    auto itr = _garrisonBuildingPlotInstances.find(std::make_pair(garrBuildingId, garrSiteLevelPlotInstId));
    if (itr != _garrisonBuildingPlotInstances.end())
        return itr->second;

    return 0;
}

uint32 GarrisonMgr::GetPreviousLevelBuildingId(uint32 buildingType, uint32 currentLevel) const
{
    auto itr = _garrisonBuildingsByType.find(buildingType);
    if (itr != _garrisonBuildingsByType.end())
        for (uint32 buildingId : itr->second)
            if (sGarrBuildingStore.AssertEntry(buildingId)->UpgradeLevel == currentLevel - 1)
                return buildingId;

    return 0;
}

FinalizeGarrisonPlotGOInfo const* GarrisonMgr::GetPlotFinalizeGOInfo(uint32 garrPlotInstanceID) const
{
    auto itr = _finalizePlotGOInfo.find(garrPlotInstanceID);
    if (itr != _finalizePlotGOInfo.end())
        return &itr->second;

    return nullptr;
}

uint64 GarrisonMgr::GenerateFollowerDbId()
{
    if (_followerDbIdGenerator >= std::numeric_limits<uint64>::max())
    {
        TC_LOG_ERROR("misc", "Garrison follower db id overflow! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }

    return _followerDbIdGenerator++;
}

uint32 const AbilitiesForQuality[][2] =
{
    // Counters, Traits
    { 0, 0 },
    { 1, 0 },
    { 1, 1 },   // Uncommon
    { 1, 2 },   // Rare
    { 2, 3 },   // Epic
    { 2, 3 }    // Legendary
};

std::list<GarrAbilityEntry const*> GarrisonMgr::RollFollowerAbilities(uint32 garrFollowerId, GarrFollowerEntry const* follower, uint32 quality, uint32 faction, bool initial) const
{
    ASSERT(faction < 2);

    bool hasForcedExclusiveTrait = false;
    std::list<GarrAbilityEntry const*> result;
    uint32 slots[2] = { AbilitiesForQuality[quality][0], AbilitiesForQuality[quality][1] };

    GarrAbilities const* abilities = nullptr;
    auto itr = _garrisonFollowerAbilities[faction].find(garrFollowerId);
    if (itr != _garrisonFollowerAbilities[faction].end())
        abilities = &itr->second;

    std::list<GarrAbilityEntry const*> abilityList, forcedAbilities, traitList, forcedTraits;
    if (abilities)
    {
        for (GarrAbilityEntry const* ability : abilities->Counters)
        {
            if (ability->Flags & GARRISON_ABILITY_HORDE_ONLY && faction != GARRISON_FACTION_INDEX_HORDE)
                continue;
            else if (ability->Flags & GARRISON_ABILITY_ALLIANCE_ONLY && faction != GARRISON_FACTION_INDEX_ALLIANCE)
                continue;

            if (ability->Flags & GARRISON_ABILITY_FLAG_CANNOT_REMOVE)
                forcedAbilities.push_back(ability);
            else
                abilityList.push_back(ability);
        }

        for (GarrAbilityEntry const* ability : abilities->Traits)
        {
            if (ability->Flags & GARRISON_ABILITY_HORDE_ONLY && faction != GARRISON_FACTION_INDEX_HORDE)
                continue;
            else if (ability->Flags & GARRISON_ABILITY_ALLIANCE_ONLY && faction != GARRISON_FACTION_INDEX_ALLIANCE)
                continue;

            if (ability->Flags & GARRISON_ABILITY_FLAG_CANNOT_REMOVE)
                forcedTraits.push_back(ability);
            else
                traitList.push_back(ability);
        }
    }

    Trinity::Containers::RandomResize(abilityList, std::max<int32>(0, slots[0] - forcedAbilities.size()));
    Trinity::Containers::RandomResize(traitList, std::max<int32>(0, slots[1] - forcedTraits.size()));

    // Add abilities specified in GarrFollowerXAbility.db2 before generic classspec ones on follower creation
    if (initial)
    {
        forcedAbilities.splice(forcedAbilities.end(), abilityList);
        forcedTraits.splice(forcedTraits.end(), traitList);
    }

    forcedAbilities.sort();
    abilityList.sort();
    forcedTraits.sort();
    traitList.sort();

    // check if we have a trait from exclusive category
    for (GarrAbilityEntry const* ability : forcedTraits)
    {
        if (ability->Flags & GARRISON_ABILITY_FLAG_EXCLUSIVE)
        {
            hasForcedExclusiveTrait = true;
            break;
        }
    }

    if (slots[0] > forcedAbilities.size() + abilityList.size())
    {
        std::list<GarrAbilityEntry const*> classSpecAbilities = GetClassSpecAbilities(follower, faction);
        std::list<GarrAbilityEntry const*> classSpecAbilitiesTemp, classSpecAbilitiesTemp2;
        classSpecAbilitiesTemp2.swap(abilityList);
        std::set_difference(classSpecAbilities.begin(), classSpecAbilities.end(), forcedAbilities.begin(), forcedAbilities.end(), std::back_inserter(classSpecAbilitiesTemp));
        std::set_union(classSpecAbilitiesTemp.begin(), classSpecAbilitiesTemp.end(), classSpecAbilitiesTemp2.begin(), classSpecAbilitiesTemp2.end(), std::back_inserter(abilityList));

        Trinity::Containers::RandomResize(abilityList, std::max<int32>(0, slots[0] - forcedAbilities.size()));
    }

    if (slots[1] > forcedTraits.size() + traitList.size())
    {
        std::list<GarrAbilityEntry const*> genericTraits, genericTraitsTemp;
        for (GarrAbilityEntry const* ability : _garrisonFollowerRandomTraits)
        {
            if (ability->Flags & GARRISON_ABILITY_HORDE_ONLY && faction != GARRISON_FACTION_INDEX_HORDE)
                continue;
            else if (ability->Flags & GARRISON_ABILITY_ALLIANCE_ONLY && faction != GARRISON_FACTION_INDEX_ALLIANCE)
                continue;

            // forced exclusive trait exists, skip other ones entirely
            if (hasForcedExclusiveTrait && ability->Flags & GARRISON_ABILITY_FLAG_EXCLUSIVE)
                continue;

            genericTraitsTemp.push_back(ability);
        }

        std::set_difference(genericTraitsTemp.begin(), genericTraitsTemp.end(), forcedTraits.begin(), forcedTraits.end(), std::back_inserter(genericTraits));
        genericTraits.splice(genericTraits.begin(), traitList);
        // "split" the list into two parts [nonexclusive, exclusive] to make selection later easier
        genericTraits.sort([](GarrAbilityEntry const* a1, GarrAbilityEntry const* a2)
        {
            uint32 e1 = a1->Flags & GARRISON_ABILITY_FLAG_EXCLUSIVE;
            uint32 e2 = a2->Flags & GARRISON_ABILITY_FLAG_EXCLUSIVE;
            if (e1 != e2)
                return e1 < e2;

            return a1->ID < a2->ID;
        });
        genericTraits.unique();

        std::size_t firstExclusive = 0, total = genericTraits.size();
        for (auto genericTraitItr = genericTraits.begin(); genericTraitItr != genericTraits.end(); ++genericTraitItr, ++firstExclusive)
            if ((*genericTraitItr)->Flags & GARRISON_ABILITY_FLAG_EXCLUSIVE)
                break;

        while (traitList.size() < size_t(std::max<int32>(0, slots[1] - forcedTraits.size())) && total)
        {
            auto genericTraitItr = genericTraits.begin();
            std::advance(genericTraitItr, urand(0, total-- - 1));
            if ((*genericTraitItr)->Flags & GARRISON_ABILITY_FLAG_EXCLUSIVE)
                total = firstExclusive; // selected exclusive trait - no other can be selected now
            else
                --firstExclusive;

            traitList.push_back(*genericTraitItr);
            genericTraits.erase(genericTraitItr);
        }
    }

    result.splice(result.end(), forcedAbilities);
    result.splice(result.end(), abilityList);
    result.splice(result.end(), forcedTraits);
    result.splice(result.end(), traitList);

    return result;
}

std::list<GarrAbilityEntry const*> GarrisonMgr::GetClassSpecAbilities(GarrFollowerEntry const* follower, uint32 faction) const
{
    std::list<GarrAbilityEntry const*> abilities;
    uint32 classSpecId;
    switch (faction)
    {
        case GARRISON_FACTION_INDEX_HORDE:
            classSpecId = follower->HordeGarrClassSpecID;
            break;
        case GARRISON_FACTION_INDEX_ALLIANCE:
            classSpecId = follower->AllianceGarrClassSpecID;
            break;
        default:
            return abilities;
    }

    if (!sGarrClassSpecStore.LookupEntry(classSpecId))
        return abilities;

    auto itr = _garrisonFollowerClassSpecAbilities.find(classSpecId);
    if (itr != _garrisonFollowerClassSpecAbilities.end())
        abilities = itr->second;

    return abilities;
}

GarrFollowerTypeEntry const* GarrisonMgr::GetFollowerTypeForGarrType(int8 garrTypeID) const
{
    auto itr = _followerTypeByGarrType.find(garrTypeID);
    if (itr != _followerTypeByGarrType.end())
        return itr->second;

    return nullptr;
}

uint8 GarrisonMgr::GetPrimaryFollowerType(int8 garrTypeID) const
{
    GarrFollowerTypeEntry const* entry = GetFollowerTypeForGarrType(garrTypeID);
    if (entry)
        return entry->ID;

    // Fallback to WoD follower type
    return FOLLOWER_TYPE_GARRISON;
}

GarrFollowerLevelXPEntry const* GarrisonMgr::GetFollowerLevelXP(uint8 garrFollowerTypeID, int8 followerLevel) const
{
    auto itr = _followerLevelXP.find(std::make_pair(garrFollowerTypeID, followerLevel));
    if (itr != _followerLevelXP.end())
        return itr->second;

    return nullptr;
}

GarrFollowerQualityEntry const* GarrisonMgr::GetFollowerQuality(uint16 garrFollowerTypeID, int8 quality) const
{
    auto itr = _followerQuality.find(std::make_pair(garrFollowerTypeID, quality));
    if (itr != _followerQuality.end())
        return itr->second;

    return nullptr;
}

uint32 GarrisonMgr::GetFollowerZoneSupportSpell(uint32 garrFollowerID, uint32 factionIndex) const
{
    GarrFollSupportSpellEntry const* best = nullptr;
    for (GarrFollSupportSpellEntry const* entry : sGarrFollSupportSpellStore)
    {
        if (uint32(entry->GarrFollowerID) != garrFollowerID)
            continue;
        if (!best || entry->OrderIndex < best->OrderIndex)
            best = entry;
    }

    if (!best)
        return 0;

    return factionIndex == GARRISON_FACTION_INDEX_ALLIANCE ? uint32(best->AllianceSpellID) : uint32(best->HordeSpellID);
}

std::vector<GarrMissionEntry const*> const* GarrisonMgr::GetMissionsByGarrType(int8 garrTypeID) const
{
    auto itr = _missionsByGarrType.find(garrTypeID);
    if (itr != _missionsByGarrType.end())
        return &itr->second;

    return nullptr;
}

std::vector<GarrMissionXEncounterEntry const*> const* GarrisonMgr::GetMissionEncounters(uint32 garrMissionID) const
{
    auto itr = _missionEncounters.find(garrMissionID);
    if (itr != _missionEncounters.end())
        return &itr->second;

    return nullptr;
}

std::vector<GarrMechanicEntry const*> const* GarrisonMgr::GetEncounterMechanics(uint32 garrEncounterID) const
{
    auto itr = _encounterMechanics.find(garrEncounterID);
    if (itr != _encounterMechanics.end())
        return &itr->second;

    return nullptr;
}

std::vector<GarrMissionXFollowerEntry const*> const* GarrisonMgr::GetMissionRequiredFollowers(uint32 garrMissionID) const
{
    auto itr = _missionRequiredFollowers.find(garrMissionID);
    if (itr != _missionRequiredFollowers.end())
        return &itr->second;

    return nullptr;
}

GarrMechanicTypeEntry const* GarrisonMgr::GetMechanicType(int32 garrMechanicTypeID) const
{
    return sGarrMechanicTypeStore.LookupEntry(garrMechanicTypeID);
}

bool GarrisonMgr::DoesAbilityCounterMechanic(GarrAbilityEntry const* ability, GarrMechanicTypeEntry const* mechanicType) const
{
    if (!ability || !mechanicType)
        return false;

    // An ability counters a mechanic if they share the same GarrAbilityCategoryID
    return ability->GarrAbilityCategoryID == mechanicType->GarrAbilityCategoryID
        && ability->GarrAbilityCategoryID != 0;
}

CharShipmentContainerEntry const* GarrisonMgr::GetShipmentContainerForBuilding(uint8 garrBuildingType, uint8 factionIndex) const
{
    auto itr = _shipmentContainersByBuildingType.find(garrBuildingType);
    if (itr == _shipmentContainersByBuildingType.end())
        return nullptr;

    CharShipmentContainerEntry const* container = itr->second;
    // WoD shipment containers come in an Alliance/Horde pair (CharShipmentContainer.Faction, CrossFactionID).
    // The by-building-type index keeps only one of the pair, so pick the one matching the garrison's faction —
    // sending the wrong-faction container crashes the client when it opens the work-order UI. Faction: 0=Horde,
    // 1=Alliance, matching GarrisonFactionIndex.
    if (container->Faction != int8(factionIndex) && container->CrossFactionID)
        if (CharShipmentContainerEntry const* cross = sCharShipmentContainerStore.LookupEntry(container->CrossFactionID))
            container = cross;

    return container;
}

std::vector<CharShipmentEntry const*> const* GarrisonMgr::GetShipmentsForContainer(uint32 containerID) const
{
    auto itr = _shipmentsByContainer.find(containerID);
    if (itr != _shipmentsByContainer.end())
        return &itr->second;

    return nullptr;
}

uint64 GarrisonMgr::GenerateShipmentDbId()
{
    if (_shipmentDbIdGenerator >= std::numeric_limits<uint64>::max())
    {
        TC_LOG_ERROR("misc", "Garrison shipment db id overflow! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }

    return _shipmentDbIdGenerator++;
}

uint64 GarrisonMgr::GenerateMissionDbId()
{
    if (_missionDbIdGenerator >= std::numeric_limits<uint64>::max())
    {
        TC_LOG_ERROR("misc", "Garrison mission db id overflow! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }

    return _missionDbIdGenerator++;
}

std::vector<GarrTalentTreeEntry const*> const* GarrisonMgr::GetTalentTreesForGarrType(int8 garrTypeID) const
{
    auto itr = _talentTreesByGarrType.find(garrTypeID);
    if (itr != _talentTreesByGarrType.end())
        return &itr->second;

    return nullptr;
}

std::vector<GarrTalentEntry const*> const* GarrisonMgr::GetTalentsForTree(uint32 garrTalentTreeID) const
{
    auto itr = _talentsByTree.find(garrTalentTreeID);
    if (itr != _talentsByTree.end())
        return &itr->second;

    return nullptr;
}

std::vector<GarrTalentRankEntry const*> const* GarrisonMgr::GetTalentRanksForTalent(uint32 garrTalentID) const
{
    auto itr = _talentRanksByTalent.find(garrTalentID);
    if (itr != _talentRanksByTalent.end())
        return &itr->second;

    return nullptr;
}

GarrTalentResearchEntry const* GarrisonMgr::GetTalentResearchForTree(uint32 garrTalentTreeID) const
{
    auto itr = _talentResearchByTree.find(garrTalentTreeID);
    if (itr != _talentResearchByTree.end())
        return itr->second;

    return nullptr;
}

std::vector<GarrAbilityEffectEntry const*> const* GarrisonMgr::GetGarrAbilityEffects(uint32 garrAbilityID) const
{
    auto itr = _abilityEffectsByAbility.find(garrAbilityID);
    if (itr != _abilityEffectsByAbility.end())
        return &itr->second;

    return nullptr;
}

void GarrisonMgr::InitializeDbIdSequences()
{
    if (QueryResult result = CharacterDatabase.Query("SELECT MAX(dbId) FROM character_garrison_followers"))
        _followerDbIdGenerator = (*result)[0].GetUInt64() + 1;

    if (QueryResult result = CharacterDatabase.Query("SELECT MAX(dbId) FROM character_garrison_shipments"))
        _shipmentDbIdGenerator = (*result)[0].GetUInt64() + 1;

    // Global across ALL garrison types (WoD, order hall, war campaign, covenant): character_garrison_missions.dbId is
    // a per-character PK, so a per-Garrison counter collided (war-campaign garrison reused the WoD garrison's ids).
    if (QueryResult result = CharacterDatabase.Query("SELECT MAX(dbId) FROM character_garrison_missions"))
        _missionDbIdGenerator = (*result)[0].GetUInt64() + 1;
}

void GarrisonMgr::LoadPlotFinalizeGOInfo()
{
    //                                                                0                  1       2       3       4       5               6
    QueryResult result = WorldDatabase.Query("SELECT garrPlotInstanceId, hordeGameObjectId, hordeX, hordeY, hordeZ, hordeO, hordeAnimKitId, "
    //                      7          8          9         10         11                 12
        "allianceGameObjectId, allianceX, allianceY, allianceZ, allianceO, allianceAnimKitId FROM garrison_plot_finalize_info");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 garrison follower class spec abilities. DB table `garrison_plot_finalize_info` is empty.");
        return;
    }

    uint32 msTime = getMSTime();
    do
    {
        Field* fields = result->Fetch();
        uint32 garrPlotInstanceId = fields[0].GetUInt32();
        uint32 hordeGameObjectId = fields[1].GetUInt32();
        uint32 allianceGameObjectId = fields[7].GetUInt32();
        uint16 hordeAnimKitId = fields[6].GetUInt16();
        uint16 allianceAnimKitId = fields[12].GetUInt16();

        if (!sGarrPlotInstanceStore.LookupEntry(garrPlotInstanceId))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing GarrPlotInstance.db2 entry {} was referenced in `garrison_plot_finalize_info`.", garrPlotInstanceId);
            continue;
        }

        GameObjectTemplate const* goTemplate = sObjectMgr->GetGameObjectTemplate(hordeGameObjectId);
        if (!goTemplate)
        {
            TC_LOG_ERROR("sql.sql", "Non-existing gameobject_template entry {} was referenced in `garrison_plot_finalize_info`.`hordeGameObjectId` for garrPlotInstanceId {}.",
                hordeGameObjectId, garrPlotInstanceId);
            continue;
        }

        if (goTemplate->type != GAMEOBJECT_TYPE_GOOBER)
        {
            TC_LOG_ERROR("sql.sql", "Invalid gameobject type {} (entry {}) was referenced in `garrison_plot_finalize_info`.`hordeGameObjectId` for garrPlotInstanceId {}.",
                goTemplate->type, hordeGameObjectId, garrPlotInstanceId);
            continue;
        }

        goTemplate = sObjectMgr->GetGameObjectTemplate(allianceGameObjectId);
        if (!goTemplate)
        {
            TC_LOG_ERROR("sql.sql", "Non-existing gameobject_template entry {} was referenced in `garrison_plot_finalize_info`.`allianceGameObjectId` for garrPlotInstanceId {}.",
                allianceGameObjectId, garrPlotInstanceId);
            continue;
        }

        if (goTemplate->type != GAMEOBJECT_TYPE_GOOBER)
        {
            TC_LOG_ERROR("sql.sql", "Invalid gameobject type {} (entry {}) was referenced in `garrison_plot_finalize_info`.`allianceGameObjectId` for garrPlotInstanceId {}.",
                goTemplate->type, allianceGameObjectId, garrPlotInstanceId);
            continue;
        }

        if (hordeAnimKitId && !sAnimKitStore.LookupEntry(hordeAnimKitId))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing AnimKit.dbc entry {} was referenced in `garrison_plot_finalize_info`.`hordeAnimKitId` for garrPlotInstanceId {}.",
                hordeAnimKitId, garrPlotInstanceId);
            continue;
        }

        if (allianceAnimKitId && !sAnimKitStore.LookupEntry(allianceAnimKitId))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing AnimKit.dbc entry {} was referenced in `garrison_plot_finalize_info`.`allianceAnimKitId` for garrPlotInstanceId {}.",
                allianceAnimKitId, garrPlotInstanceId);
            continue;
        }

        FinalizeGarrisonPlotGOInfo& info = _finalizePlotGOInfo[garrPlotInstanceId];
        info.FactionInfo[GARRISON_FACTION_INDEX_HORDE].GameObjectId = hordeGameObjectId;
        info.FactionInfo[GARRISON_FACTION_INDEX_HORDE].Pos.Relocate(fields[2].GetFloat(), fields[3].GetFloat(), fields[4].GetFloat(), fields[5].GetFloat());
        info.FactionInfo[GARRISON_FACTION_INDEX_HORDE].AnimKitId = hordeAnimKitId;

        info.FactionInfo[GARRISON_FACTION_INDEX_ALLIANCE].GameObjectId = allianceGameObjectId;
        info.FactionInfo[GARRISON_FACTION_INDEX_ALLIANCE].Pos.Relocate(fields[8].GetFloat(), fields[9].GetFloat(), fields[10].GetFloat(), fields[11].GetFloat());
        info.FactionInfo[GARRISON_FACTION_INDEX_ALLIANCE].AnimKitId = allianceAnimKitId;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} garrison plot finalize entries in {}.", uint32(_finalizePlotGOInfo.size()), GetMSTimeDiffToNow(msTime));
}

void GarrisonMgr::LoadFollowerClassSpecAbilities()
{
    QueryResult result = WorldDatabase.Query("SELECT classSpecId, abilityId FROM garrison_follower_class_spec_abilities");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 garrison follower class spec abilities. DB table `garrison_follower_class_spec_abilities` is empty.");
        return;
    }

    uint32 msTime = getMSTime();
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 classSpecId = fields[0].GetUInt32();
        uint32 abilityId = fields[1].GetUInt32();

        if (!sGarrClassSpecStore.LookupEntry(classSpecId))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing GarrClassSpec.db2 entry {} was referenced in `garrison_follower_class_spec_abilities` by row ({}, {}).", classSpecId, classSpecId, abilityId);
            continue;
        }

        GarrAbilityEntry const* ability = sGarrAbilityStore.LookupEntry(abilityId);
        if (!ability)
        {
            TC_LOG_ERROR("sql.sql", "Non-existing GarrAbility.db2 entry {} was referenced in `garrison_follower_class_spec_abilities` by row ({}, {}).", abilityId, classSpecId, abilityId);
            continue;
        }

        _garrisonFollowerClassSpecAbilities[classSpecId].push_back(ability);
        ++count;

    } while (result->NextRow());

    for (auto& pair : _garrisonFollowerClassSpecAbilities)
        pair.second.sort();

    TC_LOG_INFO("server.loading", ">> Loaded {} garrison follower class spec abilities in {}.", count, GetMSTimeDiffToNow(msTime));
}

void GarrisonMgr::LoadMissionRewards()
{
    _missionRewards.clear();

    QueryResult result = WorldDatabase.Query("SELECT GarrMissionId, RewardType, ItemId, ItemQuantity, CurrencyId, CurrencyQuantity, Gold, FollowerXP FROM garrison_mission_reward");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 garrison mission rewards. DB table `garrison_mission_reward` is empty (missions fall back to the per-GarrType resource formula).");
        return;
    }

    uint32 msTime = getMSTime();
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 garrMissionId = fields[0].GetUInt32();

        if (!sGarrMissionStore.LookupEntry(garrMissionId))
        {
            TC_LOG_ERROR("sql.sql", "Non-existing GarrMission.db2 entry {} was referenced in `garrison_mission_reward`; skipped.", garrMissionId);
            continue;
        }

        GarrisonMissionRewardEntry reward;
        reward.RewardType       = fields[1].GetUInt8();
        reward.ItemId           = fields[2].GetUInt32();
        reward.ItemQuantity     = fields[3].GetUInt32();
        reward.CurrencyId       = fields[4].GetUInt32();
        reward.CurrencyQuantity = fields[5].GetUInt32();
        reward.Gold             = fields[6].GetUInt32();
        reward.FollowerXP       = fields[7].GetUInt32();

        _missionRewards[garrMissionId].push_back(reward);
        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded {} garrison mission rewards for {} missions in {}.", count, _missionRewards.size(), GetMSTimeDiffToNow(msTime));
}

std::vector<GarrisonMissionRewardEntry> const* GarrisonMgr::GetMissionRewards(uint32 garrMissionID) const
{
    auto itr = _missionRewards.find(garrMissionID);
    return itr != _missionRewards.end() ? &itr->second : nullptr;
}

uint32 GarrisonMgr::GetMissionRewardCurrency(GarrMissionEntry const* mission) const
{
    if (!mission)
        return 0;

    // Reward currency is the garrison TYPE's resource currency, NOT the mission's cost currency: some Legion
    // order-hall missions carry a stray MissionCostCurrencyTypesID (824) which previously leaked WoD Garrison
    // Resources into order-hall rewards. WoD is the only type with two resources - land = Garrison Resources
    // (824), shipyard = Oil (1101) - distinguished by the cost currency.
    switch (mission->GarrTypeID)
    {
        case 2:   return mission->MissionCostCurrencyTypesID == 1101 ? 1101 : 824;  // WoD garrison / shipyard (Oil)
        case 3:   return 1220;  // Legion Order Resources
        case 9:   return 1560;  // BfA War Resources
        case 111: return 1813;  // Shadowlands Reservoir Anima
        default:  return mission->MissionCostCurrencyTypesID;  // unknown type: best-effort
    }
}

uint32 GarrisonMgr::ComputeBaseResourceReward(GarrMissionEntry const* mission) const
{
    if (!mission)
        return 0;

    // Retail base resource rewards scale with a mission's investment (cost) and length (duration) and are
    // net-positive over cost. Cost>0 "sink" missions pay back a multiple of their cost; cost==0 "faucet"
    // missions pay a duration-scaled flat amount. Constants tuned to the live per-GarrType cost/duration
    // medians and wiki reward amounts (see garrison_mission_success_formula memory / reward research).
    float k = 0.0f, h = 0.0f, f = 0.0f;
    uint32 minReward = 0, maxReward = 0;

    uint32 currency = GetMissionRewardCurrency(mission);
    switch (mission->GarrTypeID)
    {
        case 2:
            if (currency == 1101) { k = 1.2f; h = 3.0f; f = 4.0f; minReward = 4;  maxReward = 60;  }  // WoD shipyard (Oil)
            else                  { k = 1.5f; h = 5.0f; f = 5.0f; minReward = 5;  maxReward = 150; }  // WoD garrison (Garrison Resources)
            break;
        case 3:   k = 1.4f; h = 8.0f; f = 10.0f; minReward = 10; maxReward = 200; break;  // Legion Order Resources
        case 9:   k = 1.5f; h = 6.0f; f = 8.0f;  minReward = 8;  maxReward = 150; break;  // BfA War Resources
        case 111: k = 1.6f; h = 4.0f; f = 5.0f;  minReward = 5;  maxReward = 120; break;  // SL Reservoir Anima
        default:  return 0;
    }

    float base;
    if (mission->MissionCost > 0)
        base = float(mission->MissionCost) * k;
    else
        base = (float(mission->MissionDuration) / 3600.0f) * h + f;

    return std::clamp(uint32(base + 0.5f), minReward, maxReward);
}

GarrAutoCombatantEntry const* GarrisonMgr::GetAutoCombatant(uint32 garrAutoCombatantID) const
{
    return sGarrAutoCombatantStore.LookupEntry(garrAutoCombatantID);
}

GarrAutoCombatantEntry const* GarrisonMgr::GetAutoCombatantForEncounter(uint32 garrEncounterID) const
{
    auto itr = _autoCombatantByEncounter.find(garrEncounterID);
    if (itr != _autoCombatantByEncounter.end())
        return itr->second;

    return nullptr;
}

std::vector<GarrAutoSpellEffectEntry const*> const* GarrisonMgr::GetAutoSpellEffects(uint32 garrAutoSpellID) const
{
    auto itr = _autoSpellEffects.find(garrAutoSpellID);
    if (itr != _autoSpellEffects.end())
        return &itr->second;

    return nullptr;
}

std::vector<uint32> const* GarrisonMgr::GetEncounterSetEncounters(int32 garrEncounterSetID) const
{
    auto itr = _encounterSetEncounters.find(garrEncounterSetID);
    if (itr != _encounterSetEncounters.end())
        return &itr->second;

    return nullptr;
}
