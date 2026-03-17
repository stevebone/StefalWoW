/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SiegeScriptBase.h"
#include "BattlegroundCoordinator.h"
#include "BGState.h"
#include "BotActionManager.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Creature.h"
#include "Vehicle.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot::Coordination::Battleground
{

// ============================================================================
// LIFECYCLE
// ============================================================================

void SiegeScriptBase::OnLoad(BattlegroundCoordinator* coordinator)
{
    BGScriptBase::OnLoad(coordinator);

    // Reset siege state
    m_destroyedGates.clear();
    m_towerStates.clear();
    m_destroyedTowers.clear();
    m_graveyardControl.clear();
    m_vehicleAssignments.clear();

    m_allianceReinforcements = GetStartingReinforcements();
    m_hordeReinforcements = GetStartingReinforcements();

    m_allianceBossEngaged = false;
    m_hordeBossEngaged = false;
    m_allianceBossHealthPct = 100.0f;
    m_hordeBossHealthPct = 100.0f;

    m_siegeUpdateTimer = 0;

    TC_LOG_DEBUG("playerbots.bg.script",
        "SiegeScriptBase: Initialized for {} (reinforcements: {})",
        GetName(), GetStartingReinforcements());
}

void SiegeScriptBase::OnUpdate(uint32 diff)
{
    BGScriptBase::OnUpdate(diff);

    if (!IsMatchActive())
        return;

    m_siegeUpdateTimer += diff;
    if (m_siegeUpdateTimer >= SIEGE_UPDATE_INTERVAL)
    {
        m_siegeUpdateTimer = 0;
        UpdateSiegeState();
    }

    // Refresh vehicle GUID cache on main thread
    if (HasVehicles())
    {
        m_vehicleCacheTimer += diff;
        if (m_vehicleCacheTimer >= VEHICLE_CACHE_INTERVAL)
        {
            m_vehicleCacheTimer = 0;
            UpdateVehicleCache();
        }
    }
}

// ============================================================================
// VEHICLE DATA
// ============================================================================

std::vector<BGVehicleData> SiegeScriptBase::GetVehicleData() const
{
    // Default implementation returns empty
    // Derived classes should override
    return {};
}

// ============================================================================
// STRATEGY - SIEGE OVERRIDES
// ============================================================================

RoleDistribution SiegeScriptBase::GetRecommendedRoles(
    const StrategicDecision& decision,
    float scoreAdvantage,
    uint32 timeRemaining) const
{
    RoleDistribution dist;
    uint8 teamSize = GetTeamSize();

    // Siege BGs need different roles
    switch (decision.strategy)
    {
        case BGStrategy::AGGRESSIVE:
        case BGStrategy::ALL_IN:
            // Heavy offense - push enemy base
            dist.SetRole(BGRole::NODE_ATTACKER, teamSize / 2, teamSize * 2 / 3);
            dist.SetRole(BGRole::GRAVEYARD_ASSAULT, 5, 10);
            dist.SetRole(BGRole::NODE_DEFENDER, teamSize / 8, teamSize / 4);
            dist.SetRole(BGRole::HEALER_OFFENSE, 4, 8);
            dist.SetRole(BGRole::HEALER_DEFENSE, 2, 4);
            dist.reasoning = "Aggressive siege - push forward";
            break;

        case BGStrategy::DEFENSIVE:
        case BGStrategy::TURTLE:
            // Protect our base
            dist.SetRole(BGRole::NODE_DEFENDER, teamSize / 2, teamSize * 2 / 3);
            dist.SetRole(BGRole::NODE_ATTACKER, teamSize / 8, teamSize / 4);
            dist.SetRole(BGRole::HEALER_DEFENSE, 4, 8);
            dist.SetRole(BGRole::HEALER_OFFENSE, 2, 4);
            dist.reasoning = "Defensive siege - protect base";
            break;

        default:  // BALANCED
            dist.SetRole(BGRole::NODE_ATTACKER, teamSize / 3, teamSize / 2);
            dist.SetRole(BGRole::NODE_DEFENDER, teamSize / 3, teamSize / 2);
            dist.SetRole(BGRole::GRAVEYARD_ASSAULT, 3, 6);
            dist.SetRole(BGRole::HEALER_OFFENSE, 3, 6);
            dist.SetRole(BGRole::HEALER_DEFENSE, 3, 6);
            dist.reasoning = "Balanced siege approach";
            break;
    }

    // Add roamers
    dist.SetRole(BGRole::ROAMER, 2, 5);

    return dist;
}

void SiegeScriptBase::AdjustStrategy(StrategicDecision& decision,
    float scoreAdvantage, uint32 controlledCount,
    uint32 totalObjectives, uint32 timeRemaining) const
{
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;

    // Check reinforcements
    float ourReinfPct = static_cast<float>(GetReinforcements(faction)) /
        GetStartingReinforcements();
    float theirReinfPct = static_cast<float>(GetReinforcements(
        faction == ALLIANCE ? HORDE : ALLIANCE)) / GetStartingReinforcements();

    // Critical reinforcement situations
    if (ourReinfPct < SiegeConstants::REINF_CRITICAL)
    {
        // We're about to lose on reinforcements - all out attack on boss
        decision.strategy = BGStrategy::ALL_IN;
        decision.reasoning = "Critical reinforcements - rush boss!";
        decision.offenseAllocation = 90;
        decision.defenseAllocation = 10;
        return;
    }

    if (theirReinfPct < SiegeConstants::REINF_CRITICAL)
    {
        // They're about to lose - defensive turtle
        decision.strategy = BGStrategy::TURTLE;
        decision.reasoning = "Enemy reinforcements critical - turtle and win";
        decision.defenseAllocation = 80;
        decision.offenseAllocation = 20;
        return;
    }

    // Check boss status
    bool canAttackTheirBoss = CanAttackBoss(faction == ALLIANCE ? HORDE : ALLIANCE);
    bool theyCanAttackOurBoss = CanAttackBoss(faction);

    float theirBossHP = faction == ALLIANCE ? m_hordeBossHealthPct : m_allianceBossHealthPct;
    float ourBossHP = faction == ALLIANCE ? m_allianceBossHealthPct : m_hordeBossHealthPct;

    // Boss rush conditions
    if (canAttackTheirBoss && theirBossHP < SiegeConstants::BOSS_RUSH_THRESHOLD * 100)
    {
        decision.strategy = BGStrategy::ALL_IN;
        decision.reasoning = "Enemy boss low - finish them!";
        decision.offenseAllocation = 85;
        return;
    }

    if (theyCanAttackOurBoss && ourBossHP < SiegeConstants::BOSS_RUSH_THRESHOLD * 100)
    {
        decision.strategy = BGStrategy::TURTLE;
        decision.reasoning = "Our boss under attack - defend!";
        decision.defenseAllocation = 85;
        return;
    }

    // Default siege strategy based on control
    if (controlledCount > totalObjectives / 2)
    {
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.reasoning = "Good objective control - push forward";
        decision.offenseAllocation = 60;
    }
    else
    {
        decision.strategy = BGStrategy::BALANCED;
        decision.reasoning = "Contest objectives while pushing";
        decision.offenseAllocation = 50;
        decision.defenseAllocation = 50;
    }
}

float SiegeScriptBase::CalculateWinProbability(uint32 allianceScore, uint32 hordeScore,
    uint32 timeRemaining, uint32 objectivesControlled, uint32 faction) const
{
    // In siege BGs, "score" is often reinforcements
    uint32 ourReinf = GetReinforcements(faction);
    uint32 theirReinf = GetReinforcements(faction == ALLIANCE ? HORDE : ALLIANCE);
    uint32 startReinf = GetStartingReinforcements();

    if (startReinf == 0)
        return 0.5f;

    // Reinforcement advantage
    float reinfAdvantage = static_cast<float>(ourReinf - theirReinf) / startReinf;

    // Boss health factor
    float ourBossHP = faction == ALLIANCE ? m_allianceBossHealthPct : m_hordeBossHealthPct;
    float theirBossHP = faction == ALLIANCE ? m_hordeBossHealthPct : m_allianceBossHealthPct;

    float bossAdvantage = (ourBossHP - theirBossHP) / 200.0f;  // -0.5 to 0.5

    // Objective control factor
    float controlFactor = (objectivesControlled > 0) ? 0.1f : -0.1f;

    // Combine factors
    float probability = 0.5f + reinfAdvantage * 0.3f + bossAdvantage + controlFactor;

    return std::clamp(probability, 0.05f, 0.95f);
}

// ============================================================================
// SIEGE-SPECIFIC IMPLEMENTATIONS
// ============================================================================

uint8 SiegeScriptBase::GetGatePriority(uint32 gateId) const
{
    // Default: all gates have normal priority
    // Derived classes should override for specific gate priorities
    if (IsGateDestroyed(gateId))
        return 0;

    return SiegeConstants::GATE_PRIORITY_OUTER;
}

bool SiegeScriptBase::ShouldUseVehicle(ObjectGuid botGuid, uint32 vehicleEntry) const
{
    // Default: always allow vehicle use
    // Derived classes can override for specific logic
    return true;
}

// ============================================================================
// EVENT HANDLING
// ============================================================================

void SiegeScriptBase::OnEvent(const BGScriptEventData& event)
{
    BGScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::GATE_DESTROYED:
            m_destroyedGates.insert(event.objectiveId);
            TC_LOG_DEBUG("playerbots.bg.script", "Siege: Gate {} destroyed",
                event.objectiveId);
            break;

        case BGScriptEvent::TOWER_DESTROYED:
            m_destroyedTowers.insert(event.objectiveId);
            // Reinforcement loss
            if (GetReinforcementLossPerTower() > 0)
            {
                if (event.faction == ALLIANCE)
                    m_hordeReinforcements -= GetReinforcementLossPerTower();
                else
                    m_allianceReinforcements -= GetReinforcementLossPerTower();
            }
            TC_LOG_DEBUG("playerbots.bg.script", "Siege: Tower {} destroyed",
                event.objectiveId);
            break;

        case BGScriptEvent::OBJECTIVE_CAPTURED:
            // Graveyard capture
            m_graveyardControl[event.objectiveId] = event.faction;
            break;

        case BGScriptEvent::BOSS_ENGAGED:
            if (event.faction == ALLIANCE)
                m_allianceBossEngaged = true;
            else
                m_hordeBossEngaged = true;
            TC_LOG_DEBUG("playerbots.bg.script", "Siege: {} boss engaged!",
                event.faction == ALLIANCE ? "Alliance" : "Horde");
            break;

        case BGScriptEvent::BOSS_KILLED:
            TC_LOG_DEBUG("playerbots.bg.script", "Siege: {} boss killed!",
                event.faction == ALLIANCE ? "Alliance" : "Horde");
            break;

        case BGScriptEvent::PLAYER_DIED:
            // Reinforcement loss
            if (GetReinforcementLossPerDeath() > 0)
            {
                if (event.faction == ALLIANCE)
                {
                    if (m_allianceReinforcements >= GetReinforcementLossPerDeath())
                        m_allianceReinforcements -= GetReinforcementLossPerDeath();
                    else
                        m_allianceReinforcements = 0;
                }
                else
                {
                    if (m_hordeReinforcements >= GetReinforcementLossPerDeath())
                        m_hordeReinforcements -= GetReinforcementLossPerDeath();
                    else
                        m_hordeReinforcements = 0;
                }
            }
            break;

        default:
            break;
    }
}

void SiegeScriptBase::OnMatchStart()
{
    BGScriptBase::OnMatchStart();

    m_destroyedGates.clear();
    m_destroyedTowers.clear();

    m_allianceReinforcements = GetStartingReinforcements();
    m_hordeReinforcements = GetStartingReinforcements();

    m_allianceBossEngaged = false;
    m_hordeBossEngaged = false;
    m_allianceBossHealthPct = 100.0f;
    m_hordeBossHealthPct = 100.0f;

    TC_LOG_DEBUG("playerbots.bg.script", "Siege: Match started with {} reinforcements",
        GetStartingReinforcements());
}

// ============================================================================
// SIEGE HELPERS
// ============================================================================

bool SiegeScriptBase::IsGateDestroyed(uint32 gateId) const
{
    return m_destroyedGates.find(gateId) != m_destroyedGates.end();
}

uint32 SiegeScriptBase::GetStandingTowerCount(uint32 faction) const
{
    uint32 count = 0;
    for (const auto& [towerId, state] : m_towerStates)
    {
        if (m_destroyedTowers.find(towerId) == m_destroyedTowers.end())
        {
            if ((faction == ALLIANCE && state == BGObjectiveState::ALLIANCE_CONTROLLED) ||
                (faction == HORDE && state == BGObjectiveState::HORDE_CONTROLLED))
            {
                ++count;
            }
        }
    }
    return count;
}

uint32 SiegeScriptBase::GetDestroyedTowerCount(uint32 faction) const
{
    // This would need to track which faction's towers were destroyed
    // Simplified: just return total destroyed
    return static_cast<uint32>(m_destroyedTowers.size());
}

uint32 SiegeScriptBase::GetReinforcements(uint32 faction) const
{
    return faction == ALLIANCE ? m_allianceReinforcements : m_hordeReinforcements;
}

bool SiegeScriptBase::CanAttackBoss(uint32 faction) const
{
    // Default: check if all gates to boss are down
    // Derived classes should override for specific logic
    return true;
}

uint32 SiegeScriptBase::GetRecommendedVehicle(ObjectGuid botGuid) const
{
    auto it = m_vehicleAssignments.find(botGuid);
    if (it != m_vehicleAssignments.end())
        return it->second;
    return 0;
}

bool SiegeScriptBase::ShouldRushBoss() const
{
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;

    // Rush if enemy boss is low
    float theirBossHP = faction == ALLIANCE ? m_hordeBossHealthPct : m_allianceBossHealthPct;
    if (theirBossHP < SiegeConstants::BOSS_RUSH_THRESHOLD * 100)
        return true;

    // Rush if our reinforcements are critical
    float ourReinfPct = static_cast<float>(GetReinforcements(faction)) /
        GetStartingReinforcements();
    if (ourReinfPct < SiegeConstants::REINF_DANGER)
        return true;

    return false;
}

std::vector<uint32> SiegeScriptBase::GetGateDestructionOrder(uint32 attackingFaction) const
{
    // Default: return gates in order of priority
    std::vector<std::pair<uint32, uint8>> gatePriorities;

    auto gates = GetGateData();
    for (const auto& gate : gates)
    {
        if (!IsGateDestroyed(gate.id))
        {
            gatePriorities.emplace_back(gate.id, GetGatePriority(gate.id));
        }
    }

    std::sort(gatePriorities.begin(), gatePriorities.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<uint32> result;
    for (const auto& [id, priority] : gatePriorities)
        result.push_back(id);

    return result;
}

std::vector<uint32> SiegeScriptBase::GetTowerDestructionPriority(uint32 attackingFaction) const
{
    // Default: prioritize towers that buff enemy boss
    std::vector<std::pair<uint32, uint8>> towerPriorities;

    auto towers = GetTowerData();
    for (const auto& tower : towers)
    {
        if (m_destroyedTowers.find(tower.id) == m_destroyedTowers.end())
        {
            towerPriorities.emplace_back(tower.id, tower.strategicValue);
        }
    }

    std::sort(towerPriorities.begin(), towerPriorities.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<uint32> result;
    for (const auto& [id, priority] : towerPriorities)
        result.push_back(id);

    return result;
}

uint32 SiegeScriptBase::CalculateEffectiveBossHP(uint32 faction) const
{
    // Default: standing towers increase effective HP
    uint32 standingTowers = GetStandingTowerCount(faction);
    float baseHP = 100.0f;  // Normalized

    // Each tower might add 5% effective HP (buff)
    float multiplier = 1.0f + (standingTowers * 0.05f);

    return static_cast<uint32>(baseHP * multiplier);
}

void SiegeScriptBase::UpdateSiegeState()
{
    // Periodic state updates
    // Could query actual boss HP from game state, etc.
}

// ============================================================================
// VEHICLE SYSTEM
// ============================================================================

void SiegeScriptBase::UpdateVehicleCache()
{
    if (!m_coordinator)
        return;

    // Get any bot player for grid context
    auto bots = m_coordinator->GetAllBots();
    if (bots.empty())
        return;

    ::Player* anyBot = ObjectAccessor::FindPlayer(bots.front().guid);
    if (!anyBot || !anyBot->IsInWorld())
        return;

    m_cachedVehicles.clear();

    // Scan for vehicle creatures in the BG using GetVehicleData() entries
    auto vehicleDataList = GetVehicleData();
    for (const auto& vdata : vehicleDataList)
    {
        // Find all creatures of this entry nearby
        std::list<::Creature*> creatures;
        anyBot->GetCreatureListWithEntryInGrid(creatures, vdata.entry, 5000.0f);

        for (::Creature* creature : creatures)
        {
            if (!creature || !creature->IsAlive())
                continue;

            ::Vehicle* vehicle = creature->GetVehicleKit();
            if (!vehicle)
                continue;

            CachedVehicle cached;
            cached.guid = creature->GetGUID();
            cached.entry = creature->GetEntry();
            cached.pos = creature->GetPosition();

            // Check if vehicle has any empty seats
            bool hasEmpty = false;
            for (auto const& [seatId, seat] : vehicle->Seats)
            {
                if (seat.Passenger.Guid.IsEmpty())
                {
                    hasEmpty = true;
                    break;
                }
            }
            cached.hasPassengers = !hasEmpty;

            m_cachedVehicles.push_back(cached);
        }
    }

    TC_LOG_TRACE("playerbots.bg.script",
        "SiegeScriptBase: Cached {} vehicles", m_cachedVehicles.size());
}

bool SiegeScriptBase::TryBoardNearbyVehicle(::Player* bot, uint32 vehicleEntry, float maxRange) const
{
    if (!bot || m_cachedVehicles.empty())
        return false;

    // Already in a vehicle?
    if (bot->GetVehicle())
        return false;

    float bestDist = maxRange;
    ObjectGuid bestGuid;

    for (const auto& cached : m_cachedVehicles)
    {
        if (cached.entry != vehicleEntry)
            continue;

        // Skip vehicles that already have passengers (full)
        if (cached.hasPassengers)
            continue;

        float dist = bot->GetExactDist(&cached.pos);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestGuid = cached.guid;
        }
    }

    if (bestGuid.IsEmpty())
        return false;

    // Queue enter vehicle action via BotActionMgr (thread-safe)
    sBotActionMgr->QueueAction(Playerbot::BotAction::EnterVehicle(
        bot->GetGUID(), bestGuid, -1, getMSTime()));

    TC_LOG_DEBUG("playerbots.bg.script",
        "[Siege] {} queued vehicle boarding (entry {}, dist={:.0f})",
        bot->GetName(), vehicleEntry, bestDist);
    return true;
}

} // namespace Playerbot::Coordination::Battleground
