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

#include "IsleOfConquestScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "BotActionManager.h"
#include "BGState.h"
#include "Player.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "Timer.h"
#include "Log.h"
#include "../../../Movement/BotMovementUtil.h"

namespace Playerbot::Coordination::Battleground
{

// Register script for Isle of Conquest (Map ID: 628)
REGISTER_BG_SCRIPT(IsleOfConquestScript, 628);

// ============================================================================
// LIFECYCLE METHODS
// ============================================================================

void IsleOfConquestScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    SiegeScriptBase::OnLoad(coordinator);
    m_cachedObjectives = GetObjectiveData();

    // Register score world states for reinforcement tracking
    RegisterScoreWorldState(IsleOfConquest::WorldStates::REINF_ALLY, true);
    RegisterScoreWorldState(IsleOfConquest::WorldStates::REINF_HORDE, false);

    // Initialize node control states as neutral
    for (uint32 i = 0; i < IsleOfConquest::ObjectiveIds::NODE_COUNT; ++i)
    {
        m_nodeStates[i] = BGObjectiveState::NEUTRAL;
        m_nodeControl[i] = 0;  // 0 = neutral
    }

    // Initialize gate states as intact
    for (size_t i = 0; i < m_gateIntact.size(); ++i)
        m_gateIntact[i] = true;

    m_destroyedGates.clear();

    // Initialize reinforcements
    m_allianceReinforcements = IsleOfConquest::STARTING_REINFORCEMENTS;
    m_hordeReinforcements = IsleOfConquest::STARTING_REINFORCEMENTS;

    // Initialize boss status
    m_halfordAlive = true;
    m_agmarAlive = true;

    // Reset timing trackers
    m_matchStartTime = 0;
    m_lastStrategyUpdate = 0;
    m_lastNodeCheck = 0;
    m_lastVehicleCheck = 0;

    TC_LOG_DEBUG("playerbots.bg.script",
        "IsleOfConquestScript: Loaded - {} nodes, {} gates, 2 bosses, vehicle support enabled",
        IsleOfConquest::ObjectiveIds::NODE_COUNT, 6);
}

void IsleOfConquestScript::OnMatchStart()
{
    SiegeScriptBase::OnMatchStart();

    uint32 now = getMSTime();
    m_matchStartTime = now;
    m_lastStrategyUpdate = now;
    m_lastNodeCheck = now;
    m_lastVehicleCheck = now;

    // Reset all states for fresh match
    for (uint32 i = 0; i < IsleOfConquest::ObjectiveIds::NODE_COUNT; ++i)
    {
        m_nodeStates[i] = BGObjectiveState::NEUTRAL;
        m_nodeControl[i] = 0;
    }

    for (size_t i = 0; i < m_gateIntact.size(); ++i)
        m_gateIntact[i] = true;

    m_destroyedGates.clear();

    m_allianceReinforcements = IsleOfConquest::STARTING_REINFORCEMENTS;
    m_hordeReinforcements = IsleOfConquest::STARTING_REINFORCEMENTS;
    m_halfordAlive = true;
    m_agmarAlive = true;

    TC_LOG_INFO("playerbots.bg.script",
        "IsleOfConquestScript: Match started - {} vs {} reinforcements",
        m_allianceReinforcements, m_hordeReinforcements);
}

void IsleOfConquestScript::OnMatchEnd(bool victory)
{
    SiegeScriptBase::OnMatchEnd(victory);

    uint32 duration = getMSTime() - m_matchStartTime;
    uint32 minutes = duration / 60000;
    uint32 seconds = (duration % 60000) / 1000;

    TC_LOG_INFO("playerbots.bg.script",
        "IsleOfConquestScript: Match ended - {} | Duration: {}:{:02} | "
        "Alliance: {} reinforcements | Horde: {} reinforcements | "
        "Alliance Gates: {}/3 | Horde Gates: {}/3 | "
        "Halford: {} | Agmar: {}",
        victory ? "VICTORY" : "DEFEAT", minutes, seconds,
        m_allianceReinforcements, m_hordeReinforcements,
        GetIntactGateCount(ALLIANCE), GetIntactGateCount(HORDE),
        m_halfordAlive.load() ? "ALIVE" : "DEAD",
        m_agmarAlive.load() ? "ALIVE" : "DEAD");
}

void IsleOfConquestScript::OnUpdate(uint32 diff)
{
    SiegeScriptBase::OnUpdate(diff);

    uint32 now = getMSTime();

    // Update node states periodically
    if (now - m_lastNodeCheck >= IsleOfConquest::Strategy::NODE_CHECK_INTERVAL)
    {
        UpdateNodeStates();
        m_lastNodeCheck = now;
    }

    // Update gate states periodically
    if (now - m_lastStrategyUpdate >= IsleOfConquest::Strategy::STRATEGY_UPDATE_INTERVAL)
    {
        UpdateGateStates();
        m_lastStrategyUpdate = now;

        // Log phase for debugging
        IOCPhase currentPhase = GetCurrentPhase();
        TC_LOG_DEBUG("playerbots.bg.script",
            "IsleOfConquestScript: Phase={} | Ally Reinf={} | Horde Reinf={} | "
            "Workshop: A={} H={} | Hangar: A={} H={}",
            GetPhaseName(currentPhase),
            m_allianceReinforcements, m_hordeReinforcements,
            IsNodeControlled(IsleOfConquest::ObjectiveIds::WORKSHOP, ALLIANCE),
            IsNodeControlled(IsleOfConquest::ObjectiveIds::WORKSHOP, HORDE),
            IsNodeControlled(IsleOfConquest::ObjectiveIds::HANGAR, ALLIANCE),
            IsNodeControlled(IsleOfConquest::ObjectiveIds::HANGAR, HORDE));
    }

    // Update vehicle availability
    if (now - m_lastVehicleCheck >= IsleOfConquest::Strategy::VEHICLE_CHECK_INTERVAL)
    {
        UpdateVehicleStates();
        m_lastVehicleCheck = now;
    }

    // Resolve boss GUIDs on main thread (once)
    if (!m_bossGuidsResolved && m_coordinator)
    {
        auto bots = m_coordinator->GetAllBots();
        if (!bots.empty())
        {
            if (::Player* anyBot = ObjectAccessor::FindPlayer(bots.front().guid))
            {
                if (::Creature* halford = anyBot->FindNearestCreature(
                    IsleOfConquest::Bosses::HALFORD_ENTRY, 5000.0f, true))
                    m_halfordGuid = halford->GetGUID();

                if (::Creature* agmar = anyBot->FindNearestCreature(
                    IsleOfConquest::Bosses::AGMAR_ENTRY, 5000.0f, true))
                    m_agmarGuid = agmar->GetGUID();

                if (!m_halfordGuid.IsEmpty() || !m_agmarGuid.IsEmpty())
                {
                    m_bossGuidsResolved = true;
                    TC_LOG_DEBUG("playerbots.bg.script",
                        "IsleOfConquestScript: Boss GUIDs resolved - Halford={} Agmar={}",
                        m_halfordGuid.IsEmpty() ? "NOT FOUND" : "OK",
                        m_agmarGuid.IsEmpty() ? "NOT FOUND" : "OK");
                }
            }
        }
    }
}

void IsleOfConquestScript::OnEvent(const BGScriptEventData& event)
{
    SiegeScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::GATE_DESTROYED:
        {
            uint32 gateIndex = event.objectiveId - IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_FRONT;
            if (gateIndex < 6)
            {
                m_gateIntact[gateIndex] = false;
                m_destroyedGates.insert(event.objectiveId);

                bool isAlliance = event.objectiveId <= IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_EAST;
                TC_LOG_INFO("playerbots.bg.script",
                    "IsleOfConquestScript: {} gate destroyed (ID: {}) - {} gates remaining for {}",
                    IsleOfConquest::GetGateName(gateIndex),
                    event.objectiveId,
                    GetIntactGateCount(isAlliance ? ALLIANCE : HORDE),
                    isAlliance ? "Alliance" : "Horde");
            }
            break;
        }

        case BGScriptEvent::OBJECTIVE_CAPTURED:
        {
            if (event.objectiveId < IsleOfConquest::ObjectiveIds::NODE_COUNT)
            {
                m_nodeStates[event.objectiveId] = event.newState;

                // Update node control tracking
                if (event.newState == BGObjectiveState::ALLIANCE_CONTROLLED)
                    m_nodeControl[event.objectiveId] = 1;
                else if (event.newState == BGObjectiveState::HORDE_CONTROLLED)
                    m_nodeControl[event.objectiveId] = 2;
                else
                    m_nodeControl[event.objectiveId] = 0;

                TC_LOG_INFO("playerbots.bg.script",
                    "IsleOfConquestScript: {} captured by {} (ID: {})",
                    IsleOfConquest::GetNodeName(event.objectiveId),
                    event.newState == BGObjectiveState::ALLIANCE_CONTROLLED ? "Alliance" : "Horde",
                    event.objectiveId);
            }
            break;
        }

        case BGScriptEvent::BOSS_KILLED:
        {
            if (event.objectiveId == IsleOfConquest::ObjectiveIds::HALFORD)
            {
                m_halfordAlive = false;
                TC_LOG_INFO("playerbots.bg.script",
                    "IsleOfConquestScript: High Commander Halford KILLED - Horde wins!");
            }
            else if (event.objectiveId == IsleOfConquest::ObjectiveIds::AGMAR)
            {
                m_agmarAlive = false;
                TC_LOG_INFO("playerbots.bg.script",
                    "IsleOfConquestScript: Overlord Agmar KILLED - Alliance wins!");
            }
            break;
        }

        case BGScriptEvent::WORLD_STATE_CHANGED:
        {
            // Check if this is a reinforcement change
            if (event.stateId == IsleOfConquest::WorldStates::REINF_ALLY)
            {
                m_allianceReinforcements = event.stateValue;
            }
            else if (event.stateId == IsleOfConquest::WorldStates::REINF_HORDE)
            {
                m_hordeReinforcements = event.stateValue;
            }

            TC_LOG_DEBUG("playerbots.bg.script",
                "IsleOfConquestScript: World state {} changed to {} | Reinforcements - Alliance: {} | Horde: {}",
                event.stateId, event.stateValue,
                m_allianceReinforcements, m_hordeReinforcements);
            break;
        }

        case BGScriptEvent::CUSTOM_EVENT:
        {
            TC_LOG_DEBUG("playerbots.bg.script",
                "IsleOfConquestScript: Custom event received - ID: {}, Value: {}",
                event.objectiveId, static_cast<int>(event.newState));
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// DATA PROVIDERS
// ============================================================================

std::vector<BGObjectiveData> IsleOfConquestScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    // Nodes (5 capturable points)
    auto nodes = GetNodeData();
    objectives.insert(objectives.end(), nodes.begin(), nodes.end());

    // Gates (6 destructible gates)
    auto gates = GetGateData();
    objectives.insert(objectives.end(), gates.begin(), gates.end());

    // Bosses (2 kill targets)
    BGObjectiveData halford;
    halford.id = IsleOfConquest::ObjectiveIds::HALFORD;
    halford.type = ObjectiveType::BOSS;
    halford.name = "High Commander Halford Wyrmbane";
    halford.x = IsleOfConquest::Bosses::HALFORD_X;
    halford.y = IsleOfConquest::Bosses::HALFORD_Y;
    halford.z = IsleOfConquest::Bosses::HALFORD_Z;
    halford.strategicValue = 10;
    objectives.push_back(halford);

    BGObjectiveData agmar;
    agmar.id = IsleOfConquest::ObjectiveIds::AGMAR;
    agmar.type = ObjectiveType::BOSS;
    agmar.name = "Overlord Agmar";
    agmar.x = IsleOfConquest::Bosses::AGMAR_X;
    agmar.y = IsleOfConquest::Bosses::AGMAR_Y;
    agmar.z = IsleOfConquest::Bosses::AGMAR_Z;
    agmar.strategicValue = 10;
    objectives.push_back(agmar);

    return objectives;
}

std::vector<BGObjectiveData> IsleOfConquestScript::GetNodeData() const
{
    std::vector<BGObjectiveData> nodes;

    const char* nodeNames[] = { "Refinery", "Quarry", "Docks", "Hangar", "Workshop" };
    uint8 strategicValues[] = { 6, 6, 7, 8, 9 };  // Workshop > Hangar > Docks > Resources

    for (uint32 i = 0; i < IsleOfConquest::ObjectiveIds::NODE_COUNT; ++i)
    {
        const auto& posData = IsleOfConquest::NodePositions::POSITIONS[i];
        BGObjectiveData node;
        node.id = i;
        node.type = ObjectiveType::NODE;
        node.name = nodeNames[i];
        node.x = posData.x;
        node.y = posData.y;
        node.z = posData.z;
        node.strategicValue = strategicValues[i];
        node.captureTime = IsleOfConquest::Strategy::NODE_CAPTURE_TIME;
        nodes.push_back(node);
    }

    return nodes;
}

std::vector<BGObjectiveData> IsleOfConquestScript::GetTowerData() const
{
    // IOC doesn't have towers like AV
    return {};
}

std::vector<BGObjectiveData> IsleOfConquestScript::GetGraveyardData() const
{
    std::vector<BGObjectiveData> graveyards;

    // Each controlled node provides a graveyard
    // Plus faction base graveyards
    BGObjectiveData allyBase;
    allyBase.id = 200;
    allyBase.type = ObjectiveType::GRAVEYARD;
    allyBase.name = "Alliance Keep Graveyard";
    allyBase.x = 290.0f;
    allyBase.y = -820.0f;
    allyBase.z = 48.0f;
    allyBase.strategicValue = 5;
    graveyards.push_back(allyBase);

    BGObjectiveData hordeBase;
    hordeBase.id = 201;
    hordeBase.type = ObjectiveType::GRAVEYARD;
    hordeBase.name = "Horde Keep Graveyard";
    hordeBase.x = 1141.0f;
    hordeBase.y = -780.0f;
    hordeBase.z = 48.0f;
    hordeBase.strategicValue = 5;
    graveyards.push_back(hordeBase);

    return graveyards;
}

std::vector<BGObjectiveData> IsleOfConquestScript::GetGateData() const
{
    std::vector<BGObjectiveData> gates;

    const char* gateNames[] = {
        "Alliance Front Gate", "Alliance West Gate", "Alliance East Gate",
        "Horde Front Gate", "Horde West Gate", "Horde East Gate"
    };

    for (uint32 i = 0; i < 6; ++i)
    {
        const auto& posData = IsleOfConquest::GatePositions::POSITIONS[i];
        BGObjectiveData gate;
        gate.id = IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_FRONT + i;
        gate.type = ObjectiveType::GATE;
        gate.name = gateNames[i];
        gate.x = posData.x;
        gate.y = posData.y;
        gate.z = posData.z;
        gate.strategicValue = 8;
        gates.push_back(gate);
    }

    return gates;
}

std::vector<BGPositionData> IsleOfConquestScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;

    if (faction == ALLIANCE)
    {
        for (const auto& pos : IsleOfConquest::ALLIANCE_SPAWNS)
        {
            spawns.push_back(BGPositionData("Alliance Keep Spawn",
                pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
                BGPositionData::PositionType::SPAWN_POINT, ALLIANCE, 5));
        }
    }
    else
    {
        for (const auto& pos : IsleOfConquest::HORDE_SPAWNS)
        {
            spawns.push_back(BGPositionData("Horde Keep Spawn",
                pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
                BGPositionData::PositionType::SPAWN_POINT, HORDE, 5));
        }
    }

    return spawns;
}

std::vector<BGPositionData> IsleOfConquestScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Node positions as strategic points
    const char* nodeNames[] = { "Refinery", "Quarry", "Docks", "Hangar", "Workshop" };
    for (uint32 i = 0; i < IsleOfConquest::ObjectiveIds::NODE_COUNT; ++i)
    {
        const auto& posData = IsleOfConquest::NodePositions::POSITIONS[i];
        positions.push_back(BGPositionData(nodeNames[i],
            posData.x, posData.y, posData.z, 0,
            BGPositionData::PositionType::STRATEGIC_POINT, 0, posData.strategicValue));
    }

    // Chokepoints
    for (const auto& choke : IsleOfConquest::StrategicPositions::CHOKEPOINTS)
    {
        positions.push_back(BGPositionData(choke.name,
            choke.x, choke.y, choke.z, 0,
            BGPositionData::PositionType::CHOKEPOINT, 0, choke.strategicValue));
    }

    // Sniper positions
    for (const auto& sniper : IsleOfConquest::StrategicPositions::SNIPER_POSITIONS)
    {
        positions.push_back(BGPositionData(sniper.name,
            sniper.x, sniper.y, sniper.z, 0,
            BGPositionData::PositionType::SNIPER_POSITION, 0, sniper.strategicValue));
    }

    // Gate positions as defensive points
    const char* gateNames[] = {
        "Alliance Front Gate", "Alliance West Gate", "Alliance East Gate",
        "Horde Front Gate", "Horde West Gate", "Horde East Gate"
    };
    for (uint32 i = 0; i < 6; ++i)
    {
        const auto& posData = IsleOfConquest::GatePositions::POSITIONS[i];
        uint32 faction = (i < 3) ? ALLIANCE : HORDE;
        positions.push_back(BGPositionData(gateNames[i],
            posData.x, posData.y, posData.z, 0,
            BGPositionData::PositionType::DEFENSIVE_POSITION, faction, 8));
    }

    // Boss positions
    positions.push_back(BGPositionData("High Commander Halford",
        IsleOfConquest::Bosses::HALFORD_X, IsleOfConquest::Bosses::HALFORD_Y,
        IsleOfConquest::Bosses::HALFORD_Z, 0,
        BGPositionData::PositionType::STRATEGIC_POINT, ALLIANCE, 10));

    positions.push_back(BGPositionData("Overlord Agmar",
        IsleOfConquest::Bosses::AGMAR_X, IsleOfConquest::Bosses::AGMAR_Y,
        IsleOfConquest::Bosses::AGMAR_Z, 0,
        BGPositionData::PositionType::STRATEGIC_POINT, HORDE, 10));

    return positions;
}

std::vector<BGPositionData> IsleOfConquestScript::GetGraveyardPositions(uint32 faction) const
{
    std::vector<BGPositionData> positions;

    // Base graveyards
    if (faction == ALLIANCE)
    {
        positions.push_back(BGPositionData("Alliance Keep Graveyard",
            290.0f, -820.0f, 48.0f, 0,
            BGPositionData::PositionType::SPAWN_POINT, ALLIANCE, 5));
    }
    else
    {
        positions.push_back(BGPositionData("Horde Keep Graveyard",
            1141.0f, -780.0f, 48.0f, 0,
            BGPositionData::PositionType::SPAWN_POINT, HORDE, 5));
    }

    // Node graveyards based on control
    const char* nodeNames[] = { "Refinery", "Quarry", "Docks", "Hangar", "Workshop" };
    for (uint32 i = 0; i < IsleOfConquest::ObjectiveIds::NODE_COUNT; ++i)
    {
        if (IsNodeControlled(i, faction))
        {
            const auto& posData = IsleOfConquest::NodePositions::POSITIONS[i];
            positions.push_back(BGPositionData(std::string(nodeNames[i]) + " Graveyard",
                posData.x, posData.y, posData.z, 0,
                BGPositionData::PositionType::SPAWN_POINT, faction, 5));
        }
    }

    return positions;
}

std::vector<BGVehicleData> IsleOfConquestScript::GetVehicleData() const
{
    std::vector<BGVehicleData> vehicles;

    // Docks vehicles
    vehicles.push_back(BGVehicleData(
        IsleOfConquest::Vehicles::GLAIVE_THROWER, "Glaive Thrower", 100000, 1, true));
    vehicles.push_back(BGVehicleData(
        IsleOfConquest::Vehicles::CATAPULT, "Catapult", 75000, 1, true));

    // Workshop vehicles (siege)
    vehicles.push_back(BGVehicleData(
        IsleOfConquest::Vehicles::DEMOLISHER, "Demolisher", 150000, 2, true));
    vehicles.push_back(BGVehicleData(
        IsleOfConquest::Vehicles::SIEGE_ENGINE, "Siege Engine", 500000, 4, true));

    return vehicles;
}

std::vector<BGWorldState> IsleOfConquestScript::GetInitialWorldStates() const
{
    std::vector<BGWorldState> states;

    // Reinforcement states
    states.push_back(BGWorldState(IsleOfConquest::WorldStates::REINF_ALLY,
        "Alliance Reinforcements", BGWorldState::StateType::REINFORCEMENTS,
        IsleOfConquest::STARTING_REINFORCEMENTS));
    states.push_back(BGWorldState(IsleOfConquest::WorldStates::REINF_HORDE,
        "Horde Reinforcements", BGWorldState::StateType::REINFORCEMENTS,
        IsleOfConquest::STARTING_REINFORCEMENTS));

    // Node states
    states.push_back(BGWorldState(IsleOfConquest::WorldStates::REFINERY_NEUTRAL,
        "Refinery Neutral", BGWorldState::StateType::OBJECTIVE_STATE, 1));
    states.push_back(BGWorldState(IsleOfConquest::WorldStates::QUARRY_NEUTRAL,
        "Quarry Neutral", BGWorldState::StateType::OBJECTIVE_STATE, 1));
    states.push_back(BGWorldState(IsleOfConquest::WorldStates::DOCKS_NEUTRAL,
        "Docks Neutral", BGWorldState::StateType::OBJECTIVE_STATE, 1));
    states.push_back(BGWorldState(IsleOfConquest::WorldStates::HANGAR_NEUTRAL,
        "Hangar Neutral", BGWorldState::StateType::OBJECTIVE_STATE, 1));
    states.push_back(BGWorldState(IsleOfConquest::WorldStates::WORKSHOP_NEUTRAL,
        "Workshop Neutral", BGWorldState::StateType::OBJECTIVE_STATE, 1));

    return states;
}

// ============================================================================
// WORLD STATE INTERPRETATION
// ============================================================================

bool IsleOfConquestScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    // Refinery states
    if (stateId == IsleOfConquest::WorldStates::REFINERY_ALLY && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::REFINERY;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::REFINERY_HORDE && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::REFINERY;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::REFINERY_NEUTRAL && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::REFINERY;
        outState = BGObjectiveState::NEUTRAL;
        return true;
    }

    // Quarry states
    if (stateId == IsleOfConquest::WorldStates::QUARRY_ALLY && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::QUARRY;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::QUARRY_HORDE && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::QUARRY;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::QUARRY_NEUTRAL && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::QUARRY;
        outState = BGObjectiveState::NEUTRAL;
        return true;
    }

    // Docks states
    if (stateId == IsleOfConquest::WorldStates::DOCKS_ALLY && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::DOCKS;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::DOCKS_HORDE && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::DOCKS;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::DOCKS_NEUTRAL && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::DOCKS;
        outState = BGObjectiveState::NEUTRAL;
        return true;
    }

    // Hangar states
    if (stateId == IsleOfConquest::WorldStates::HANGAR_ALLY && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::HANGAR;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::HANGAR_HORDE && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::HANGAR;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::HANGAR_NEUTRAL && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::HANGAR;
        outState = BGObjectiveState::NEUTRAL;
        return true;
    }

    // Workshop states
    if (stateId == IsleOfConquest::WorldStates::WORKSHOP_ALLY && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::WORKSHOP;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::WORKSHOP_HORDE && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::WORKSHOP;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }
    if (stateId == IsleOfConquest::WorldStates::WORKSHOP_NEUTRAL && value)
    {
        outObjectiveId = IsleOfConquest::ObjectiveIds::WORKSHOP;
        outState = BGObjectiveState::NEUTRAL;
        return true;
    }

    return TryInterpretFromCache(stateId, value, outObjectiveId, outState);
}

void IsleOfConquestScript::GetScoreFromWorldStates(const std::map<int32, int32>& states,
    uint32& allianceScore, uint32& hordeScore) const
{
    allianceScore = IsleOfConquest::STARTING_REINFORCEMENTS;
    hordeScore = IsleOfConquest::STARTING_REINFORCEMENTS;

    auto allyIt = states.find(IsleOfConquest::WorldStates::REINF_ALLY);
    if (allyIt != states.end())
        allianceScore = static_cast<uint32>(std::max(0, allyIt->second));

    auto hordeIt = states.find(IsleOfConquest::WorldStates::REINF_HORDE);
    if (hordeIt != states.end())
        hordeScore = static_cast<uint32>(std::max(0, hordeIt->second));
}

// ============================================================================
// STRATEGY & ROLE DISTRIBUTION
// ============================================================================

RoleDistribution IsleOfConquestScript::GetRecommendedRoles(const StrategicDecision& decision,
    float scoreAdvantage, uint32 timeRemaining) const
{
    RoleDistribution dist;
    IOCPhase phase = GetCurrentPhase();

    switch (phase)
    {
        case IOCPhase::OPENING:
            // Rush nodes, especially Workshop and Hangar
            dist.SetRole(BGRole::NODE_ATTACKER, 50, 60);
            dist.SetRole(BGRole::NODE_DEFENDER, 20, 25);
            dist.SetRole(BGRole::ROAMER, 10, 15);
            dist.SetRole(BGRole::VEHICLE_DRIVER, 5, 10);
            dist.SetRole(BGRole::VEHICLE_GUNNER, 0, 5);
            dist.reasoning = "Opening phase - rush strategic nodes";
            break;

        case IOCPhase::NODE_CAPTURE:
            // Continue node pressure
            dist.SetRole(BGRole::NODE_ATTACKER, 40, 50);
            dist.SetRole(BGRole::NODE_DEFENDER, 25, 30);
            dist.SetRole(BGRole::VEHICLE_DRIVER, 10, 15);
            dist.SetRole(BGRole::VEHICLE_GUNNER, 5, 10);
            dist.SetRole(BGRole::ROAMER, 10, 15);
            dist.reasoning = "Node capture phase - secure key nodes";
            break;

        case IOCPhase::VEHICLE_SIEGE:
            // Heavy vehicle focus
            dist.SetRole(BGRole::VEHICLE_DRIVER, 20, 25);
            dist.SetRole(BGRole::VEHICLE_GUNNER, 15, 20);
            dist.SetRole(BGRole::NODE_DEFENDER, 25, 30);
            dist.SetRole(BGRole::NODE_ATTACKER, 15, 20);
            dist.SetRole(BGRole::ROAMER, 10, 15);
            dist.reasoning = "Vehicle siege phase - assault gates";
            break;

        case IOCPhase::GATE_ASSAULT:
            // Breaking into keep
            dist.SetRole(BGRole::VEHICLE_DRIVER, 15, 20);
            dist.SetRole(BGRole::VEHICLE_GUNNER, 10, 15);
            dist.SetRole(BGRole::BOSS_ASSAULT, 30, 35);
            dist.SetRole(BGRole::NODE_DEFENDER, 20, 25);
            dist.SetRole(BGRole::ROAMER, 10, 15);
            dist.reasoning = "Gate assault phase - breakthrough";
            break;

        case IOCPhase::BOSS_ASSAULT:
            // All-in boss kill
            dist.SetRole(BGRole::BOSS_ASSAULT, 50, 60);
            dist.SetRole(BGRole::NODE_DEFENDER, 15, 20);
            dist.SetRole(BGRole::VEHICLE_DRIVER, 10, 15);
            dist.SetRole(BGRole::VEHICLE_GUNNER, 5, 10);
            dist.SetRole(BGRole::ROAMER, 5, 10);
            dist.reasoning = "Boss assault phase - KILL BOSS!";
            break;

        case IOCPhase::DEFENSE:
            // Protect our keep
            dist.SetRole(BGRole::NODE_DEFENDER, 40, 50);
            dist.SetRole(BGRole::BOSS_ASSAULT, 5, 10);
            dist.SetRole(BGRole::NODE_ATTACKER, 20, 25);
            dist.SetRole(BGRole::VEHICLE_DRIVER, 10, 15);
            dist.SetRole(BGRole::VEHICLE_GUNNER, 5, 10);
            dist.SetRole(BGRole::ROAMER, 10, 15);
            dist.reasoning = "Defense phase - protect our boss";
            break;

        case IOCPhase::DESPERATE:
            // Low reinforcements - all-in
            if (scoreAdvantage > 0)
            {
                // We're ahead - defend
                dist.SetRole(BGRole::NODE_DEFENDER, 50, 60);
                dist.SetRole(BGRole::ROAMER, 20, 25);
                dist.SetRole(BGRole::NODE_ATTACKER, 10, 15);
                dist.SetRole(BGRole::VEHICLE_DRIVER, 5, 10);
                dist.reasoning = "Desperate - stall to victory";
            }
            else
            {
                // We're behind - attack
                dist.SetRole(BGRole::BOSS_ASSAULT, 45, 55);
                dist.SetRole(BGRole::VEHICLE_DRIVER, 15, 20);
                dist.SetRole(BGRole::VEHICLE_GUNNER, 10, 15);
                dist.SetRole(BGRole::NODE_ATTACKER, 10, 15);
                dist.reasoning = "Desperate - all-in boss rush";
            }
            break;

        default:
            // Balanced default
            dist.SetRole(BGRole::NODE_ATTACKER, 25, 30);
            dist.SetRole(BGRole::NODE_DEFENDER, 25, 30);
            dist.SetRole(BGRole::VEHICLE_DRIVER, 12, 15);
            dist.SetRole(BGRole::VEHICLE_GUNNER, 8, 10);
            dist.SetRole(BGRole::BOSS_ASSAULT, 15, 20);
            dist.SetRole(BGRole::ROAMER, 10, 15);
            dist.reasoning = "Balanced siege approach";
            break;
    }

    return dist;
}

void IsleOfConquestScript::AdjustStrategy(StrategicDecision& decision, float scoreAdvantage,
    uint32 controlledCount, uint32 totalObjectives, uint32 timeRemaining) const
{
    SiegeScriptBase::AdjustStrategy(decision, scoreAdvantage, controlledCount, totalObjectives, timeRemaining);

    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 targetFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;

    // Apply phase-specific strategy adjustments
    IOCPhase phase = GetCurrentPhase();
    ApplyPhaseStrategy(decision, phase, faction);

    // Additional adjustments based on game state
    if (CanAccessKeep(targetFaction))
    {
        decision.reasoning += " + gate destroyed";

        if (IsWorkshopControlled(faction))
        {
            decision.strategy = BGStrategy::ALL_IN;
            decision.reasoning = "Gate down + siege engines - RUSH BOSS!";
            decision.offenseAllocation = 80;
            decision.defenseAllocation = 20;
            decision.attackObjectives.push_back(targetFaction == ALLIANCE ?
                IsleOfConquest::ObjectiveIds::HALFORD : IsleOfConquest::ObjectiveIds::AGMAR);
        }
        else
        {
            decision.offenseAllocation = std::min(static_cast<uint8>(decision.offenseAllocation + 15), static_cast<uint8>(85));
        }
    }

    // Hangar control provides gunship advantage
    if (IsHangarControlled(faction))
    {
        decision.reasoning += " + gunship access";
        if (ShouldUseParachuteAssault())
        {
            decision.reasoning += " - parachute assault viable";
        }
    }

    // Workshop control provides siege vehicles
    if (IsWorkshopControlled(faction) && ShouldPrioritizeVehicles())
    {
        decision.reasoning += " + siege vehicles ready";
    }
}

// ============================================================================
// PHASE MANAGEMENT
// ============================================================================

IsleOfConquestScript::IOCPhase IsleOfConquestScript::GetCurrentPhase() const
{
    uint32 elapsed = getMSTime() - m_matchStartTime;
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 targetFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;

    // Check desperate conditions first
    uint32 ourReinf = (faction == ALLIANCE) ? m_allianceReinforcements : m_hordeReinforcements;
    uint32 theirReinf = (faction == ALLIANCE) ? m_hordeReinforcements : m_allianceReinforcements;

    if (ourReinf < IsleOfConquest::Strategy::DESPERATE_THRESHOLD ||
        theirReinf < IsleOfConquest::Strategy::DESPERATE_THRESHOLD)
    {
        return IOCPhase::DESPERATE;
    }

    // Check if enemy is in our keep
    if (CanAccessKeep(faction))
    {
        return IOCPhase::DEFENSE;
    }

    // Check if we can attack boss
    if (CanAccessKeep(targetFaction) && IsBossViable(targetFaction))
    {
        return IOCPhase::BOSS_ASSAULT;
    }

    // Check if we're breaking gates
    if (CanAccessKeep(targetFaction))
    {
        return IOCPhase::GATE_ASSAULT;
    }

    // Check if we have vehicles ready for siege
    if (IsWorkshopControlled(faction) && elapsed >= IsleOfConquest::Strategy::VEHICLE_SIEGE_START)
    {
        return IOCPhase::VEHICLE_SIEGE;
    }

    // Opening phase
    if (elapsed < IsleOfConquest::Strategy::OPENING_PHASE_DURATION)
    {
        return IOCPhase::OPENING;
    }

    // Default to node capture
    return IOCPhase::NODE_CAPTURE;
}

const char* IsleOfConquestScript::GetPhaseName(IOCPhase phase) const
{
    switch (phase)
    {
        case IOCPhase::OPENING:       return "OPENING";
        case IOCPhase::NODE_CAPTURE:  return "NODE_CAPTURE";
        case IOCPhase::VEHICLE_SIEGE: return "VEHICLE_SIEGE";
        case IOCPhase::GATE_ASSAULT:  return "GATE_ASSAULT";
        case IOCPhase::BOSS_ASSAULT:  return "BOSS_ASSAULT";
        case IOCPhase::DEFENSE:       return "DEFENSE";
        case IOCPhase::DESPERATE:     return "DESPERATE";
        default:                      return "UNKNOWN";
    }
}

void IsleOfConquestScript::ApplyPhaseStrategy(StrategicDecision& decision, IOCPhase phase, uint32 faction) const
{
    switch (phase)
    {
        case IOCPhase::OPENING:
            ApplyOpeningStrategy(decision, faction);
            break;
        case IOCPhase::NODE_CAPTURE:
            ApplyNodeCaptureStrategy(decision, faction);
            break;
        case IOCPhase::VEHICLE_SIEGE:
            ApplyVehicleSiegeStrategy(decision, faction);
            break;
        case IOCPhase::GATE_ASSAULT:
            ApplyGateAssaultStrategy(decision, faction);
            break;
        case IOCPhase::BOSS_ASSAULT:
            ApplyBossAssaultStrategy(decision, faction);
            break;
        case IOCPhase::DEFENSE:
            ApplyDefensiveStrategy(decision, faction);
            break;
        case IOCPhase::DESPERATE:
            ApplyDesperateStrategy(decision, faction);
            break;
    }
}

void IsleOfConquestScript::ApplyOpeningStrategy(StrategicDecision& decision, uint32 faction) const
{
    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.offenseAllocation = 70;
    decision.defenseAllocation = 30;
    decision.reasoning = "Opening rush - capture Workshop/Hangar";

    // Priority targets: Workshop > Hangar > Docks
    auto priorities = GetNodePriorityOrder(faction);
    for (uint32 nodeId : priorities)
    {
        if (!IsNodeControlled(nodeId, faction))
        {
            decision.attackObjectives.push_back(nodeId);
        }
    }
}

void IsleOfConquestScript::ApplyNodeCaptureStrategy(StrategicDecision& decision, uint32 faction) const
{
    decision.strategy = BGStrategy::BALANCED;
    decision.offenseAllocation = 55;
    decision.defenseAllocation = 45;
    decision.reasoning = "Node capture - secure strategic positions";

    // Defend controlled nodes
    for (uint32 i = 0; i < IsleOfConquest::ObjectiveIds::NODE_COUNT; ++i)
    {
        if (IsNodeControlled(i, faction))
            decision.defendObjectives.push_back(i);
        else
            decision.attackObjectives.push_back(i);
    }
}

void IsleOfConquestScript::ApplyVehicleSiegeStrategy(StrategicDecision& decision, uint32 faction) const
{
    uint32 targetFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;

    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.offenseAllocation = 65;
    decision.defenseAllocation = 35;
    decision.reasoning = "Vehicle siege - assault enemy gates";

    // Add gate targets
    auto gatePriority = GetGatePriorityOrder(targetFaction);
    for (uint32 gateId : gatePriority)
    {
        if (!IsGateDestroyed(gateId))
        {
            decision.attackObjectives.push_back(gateId);
            break;  // Focus on one gate at a time
        }
    }

    // Defend Workshop for vehicle production
    if (IsWorkshopControlled(faction))
        decision.defendObjectives.push_back(IsleOfConquest::ObjectiveIds::WORKSHOP);
}

void IsleOfConquestScript::ApplyGateAssaultStrategy(StrategicDecision& decision, uint32 faction) const
{
    uint32 targetFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;

    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.offenseAllocation = 75;
    decision.defenseAllocation = 25;
    decision.reasoning = "Gate assault - break into enemy keep";

    // Target remaining gates
    auto gatePriority = GetGatePriorityOrder(targetFaction);
    for (uint32 gateId : gatePriority)
    {
        if (!IsGateDestroyed(gateId))
            decision.attackObjectives.push_back(gateId);
    }
}

void IsleOfConquestScript::ApplyBossAssaultStrategy(StrategicDecision& decision, uint32 faction) const
{
    uint32 targetFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;

    decision.strategy = BGStrategy::ALL_IN;
    decision.offenseAllocation = 85;
    decision.defenseAllocation = 15;
    decision.reasoning = "Boss assault - KILL THE BOSS!";

    // Target boss
    decision.attackObjectives.push_back(targetFaction == ALLIANCE ?
        IsleOfConquest::ObjectiveIds::HALFORD : IsleOfConquest::ObjectiveIds::AGMAR);
}

void IsleOfConquestScript::ApplyDefensiveStrategy(StrategicDecision& decision, uint32 faction) const
{
    decision.strategy = BGStrategy::DEFENSIVE;
    decision.offenseAllocation = 30;
    decision.defenseAllocation = 70;
    decision.reasoning = "Defense - protect our boss";

    // Defend our boss
    decision.defendObjectives.push_back(faction == ALLIANCE ?
        IsleOfConquest::ObjectiveIds::HALFORD : IsleOfConquest::ObjectiveIds::AGMAR);

    // Defend our gates
    uint32 startGate = (faction == ALLIANCE) ?
        IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_FRONT :
        IsleOfConquest::ObjectiveIds::GATE_HORDE_FRONT;
    for (uint32 i = 0; i < 3; ++i)
    {
        if (!IsGateDestroyed(startGate + i))
            decision.defendObjectives.push_back(startGate + i);
    }
}

void IsleOfConquestScript::ApplyDesperateStrategy(StrategicDecision& decision, uint32 faction) const
{
    uint32 ourReinf = (faction == ALLIANCE) ? m_allianceReinforcements : m_hordeReinforcements;
    uint32 theirReinf = (faction == ALLIANCE) ? m_hordeReinforcements : m_allianceReinforcements;
    uint32 targetFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;

    if (ourReinf > theirReinf)
    {
        // We're ahead - turtle
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.offenseAllocation = 20;
        decision.defenseAllocation = 80;
        decision.reasoning = "Desperate - stalling for victory";
    }
    else
    {
        // We're behind - all-in
        decision.strategy = BGStrategy::ALL_IN;
        decision.offenseAllocation = 90;
        decision.defenseAllocation = 10;
        decision.reasoning = "Desperate - all-in boss rush";

        decision.attackObjectives.push_back(targetFaction == ALLIANCE ?
            IsleOfConquest::ObjectiveIds::HALFORD : IsleOfConquest::ObjectiveIds::AGMAR);
    }
}

// ============================================================================
// IOC-SPECIFIC METHODS
// ============================================================================

bool IsleOfConquestScript::IsGateDestroyed(uint32 gateId) const
{
    return m_destroyedGates.find(gateId) != m_destroyedGates.end();
}

uint32 IsleOfConquestScript::GetIntactGateCount(uint32 faction) const
{
    uint32 count = 0;
    uint32 startGate = (faction == ALLIANCE) ?
        IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_FRONT :
        IsleOfConquest::ObjectiveIds::GATE_HORDE_FRONT;

    for (uint32 i = 0; i < 3; ++i)
    {
        if (!IsGateDestroyed(startGate + i))
            ++count;
    }

    return count;
}

bool IsleOfConquestScript::CanAccessKeep(uint32 targetFaction) const
{
    uint32 startGate = (targetFaction == ALLIANCE) ?
        IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_FRONT :
        IsleOfConquest::ObjectiveIds::GATE_HORDE_FRONT;

    for (uint32 i = 0; i < 3; ++i)
    {
        if (IsGateDestroyed(startGate + i))
            return true;
    }

    return false;
}

bool IsleOfConquestScript::IsNodeControlled(uint32 nodeId, uint32 faction) const
{
    auto it = m_nodeStates.find(nodeId);
    if (it == m_nodeStates.end())
        return false;

    return (faction == ALLIANCE && it->second == BGObjectiveState::ALLIANCE_CONTROLLED) ||
           (faction == HORDE && it->second == BGObjectiveState::HORDE_CONTROLLED);
}

std::vector<uint32> IsleOfConquestScript::GetAvailableVehicles(uint32 faction) const
{
    std::vector<uint32> vehicles;

    // Docks provides Glaive Throwers and Catapults
    if (IsNodeControlled(IsleOfConquest::ObjectiveIds::DOCKS, faction))
    {
        vehicles.push_back(IsleOfConquest::Vehicles::GLAIVE_THROWER);
        vehicles.push_back(IsleOfConquest::Vehicles::CATAPULT);
    }

    // Workshop provides Demolishers and Siege Engines
    if (IsNodeControlled(IsleOfConquest::ObjectiveIds::WORKSHOP, faction))
    {
        vehicles.push_back(IsleOfConquest::Vehicles::DEMOLISHER);
        vehicles.push_back(IsleOfConquest::Vehicles::SIEGE_ENGINE);
    }

    return vehicles;
}

uint32 IsleOfConquestScript::GetReinforcements(uint32 faction) const
{
    return (faction == ALLIANCE) ? m_allianceReinforcements : m_hordeReinforcements;
}

bool IsleOfConquestScript::IsWorkshopControlled(uint32 faction) const
{
    return IsNodeControlled(IsleOfConquest::ObjectiveIds::WORKSHOP, faction);
}

bool IsleOfConquestScript::IsHangarControlled(uint32 faction) const
{
    return IsNodeControlled(IsleOfConquest::ObjectiveIds::HANGAR, faction);
}

bool IsleOfConquestScript::IsBossViable(uint32 targetFaction) const
{
    if (!CanAccessKeep(targetFaction))
        return false;

    // Boss is viable if gate is down and boss is alive
    if (targetFaction == ALLIANCE)
        return m_halfordAlive;
    else
        return m_agmarAlive;
}

std::vector<uint32> IsleOfConquestScript::GetNodePriorityOrder(uint32 attackingFaction) const
{
    // Workshop > Hangar > Docks > Quarry > Refinery
    // But adjust based on proximity to faction spawn
    std::vector<uint32> priority;

    if (attackingFaction == ALLIANCE)
    {
        // Alliance rushes Workshop first, then Hangar
        priority = {
            IsleOfConquest::ObjectiveIds::WORKSHOP,
            IsleOfConquest::ObjectiveIds::HANGAR,
            IsleOfConquest::ObjectiveIds::DOCKS,
            IsleOfConquest::ObjectiveIds::QUARRY,
            IsleOfConquest::ObjectiveIds::REFINERY
        };
    }
    else
    {
        // Horde rushes Workshop first, then Hangar
        priority = {
            IsleOfConquest::ObjectiveIds::WORKSHOP,
            IsleOfConquest::ObjectiveIds::HANGAR,
            IsleOfConquest::ObjectiveIds::DOCKS,
            IsleOfConquest::ObjectiveIds::REFINERY,
            IsleOfConquest::ObjectiveIds::QUARRY
        };
    }

    return priority;
}

std::vector<uint32> IsleOfConquestScript::GetGatePriorityOrder(uint32 targetFaction) const
{
    std::vector<uint32> priority;

    // Front gate is usually the primary target
    if (targetFaction == ALLIANCE)
    {
        priority = {
            IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_FRONT,
            IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_WEST,
            IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_EAST
        };
    }
    else
    {
        priority = {
            IsleOfConquest::ObjectiveIds::GATE_HORDE_FRONT,
            IsleOfConquest::ObjectiveIds::GATE_HORDE_WEST,
            IsleOfConquest::ObjectiveIds::GATE_HORDE_EAST
        };
    }

    return priority;
}

// ============================================================================
// ENTERPRISE-GRADE POSITIONING
// ============================================================================

std::vector<Position> IsleOfConquestScript::GetNodeDefensePositions(uint32 nodeId) const
{
    std::vector<Position> positions;

    if (nodeId >= IsleOfConquest::ObjectiveIds::NODE_COUNT)
        return positions;

    const auto& defensePositions = IsleOfConquest::NodeDefense::POSITIONS[nodeId];
    for (const auto& pos : defensePositions)
    {
        positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));
    }

    return positions;
}

std::vector<Position> IsleOfConquestScript::GetGateApproachPositions(uint32 gateId) const
{
    std::vector<Position> positions;

    uint32 gateIndex = gateId - IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_FRONT;
    if (gateIndex >= 6)
        return positions;

    const auto& approachPositions = IsleOfConquest::GateApproach::POSITIONS[gateIndex];
    for (const auto& pos : approachPositions)
    {
        positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));
    }

    return positions;
}

std::vector<Position> IsleOfConquestScript::GetChokepoints() const
{
    std::vector<Position> positions;

    for (const auto& choke : IsleOfConquest::StrategicPositions::CHOKEPOINTS)
    {
        positions.push_back(Position(choke.x, choke.y, choke.z, 0));
    }

    return positions;
}

std::vector<Position> IsleOfConquestScript::GetSniperPositions() const
{
    std::vector<Position> positions;

    for (const auto& sniper : IsleOfConquest::StrategicPositions::SNIPER_POSITIONS)
    {
        positions.push_back(Position(sniper.x, sniper.y, sniper.z, 0));
    }

    return positions;
}

std::vector<Position> IsleOfConquestScript::GetAmbushPositions(uint32 faction) const
{
    std::vector<Position> positions;

    const auto& ambushPositions = (faction == ALLIANCE) ?
        IsleOfConquest::StrategicPositions::ALLIANCE_AMBUSH :
        IsleOfConquest::StrategicPositions::HORDE_AMBUSH;

    for (const auto& ambush : ambushPositions)
    {
        positions.push_back(Position(ambush.x, ambush.y, ambush.z, 0));
    }

    return positions;
}

std::vector<Position> IsleOfConquestScript::GetBossRaidPositions(uint32 targetFaction) const
{
    std::vector<Position> positions;

    const auto& bossPositions = (targetFaction == ALLIANCE) ?
        IsleOfConquest::BossRoom::HALFORD_ROOM :
        IsleOfConquest::BossRoom::AGMAR_ROOM;

    for (const auto& pos : bossPositions)
    {
        positions.push_back(Position(pos.x, pos.y, pos.z, pos.o));
    }

    return positions;
}

std::vector<Position> IsleOfConquestScript::GetVehicleStagingPositions(uint32 faction) const
{
    std::vector<Position> positions;

    const auto& stagingPositions = (faction == ALLIANCE) ?
        IsleOfConquest::StrategicPositions::ALLIANCE_VEHICLE_STAGING :
        IsleOfConquest::StrategicPositions::HORDE_VEHICLE_STAGING;

    for (const auto& pos : stagingPositions)
    {
        positions.push_back(Position(pos.x, pos.y, pos.z, 0));
    }

    return positions;
}

std::vector<Position> IsleOfConquestScript::GetSiegeRoute(uint32 attackingFaction, uint32 targetGate) const
{
    std::vector<Position> route;

    uint32 gateIndex = targetGate - IsleOfConquest::ObjectiveIds::GATE_ALLIANCE_FRONT;
    if (gateIndex >= 6)
        return route;

    // Get siege route from data
    const auto& siegeRouteData = IsleOfConquest::GetSiegeRoute(attackingFaction, targetGate);
    for (const auto& pos : siegeRouteData)
    {
        route.push_back(Position(pos.x, pos.y, pos.z, 0));
    }

    return route;
}

std::vector<Position> IsleOfConquestScript::GetParachuteDropPositions(uint32 targetFaction) const
{
    std::vector<Position> positions;

    // Parachute drop positions inside enemy keep
    const auto& dropPositions = (targetFaction == ALLIANCE) ?
        IsleOfConquest::StrategicPositions::ALLIANCE_PARACHUTE_DROP :
        IsleOfConquest::StrategicPositions::HORDE_PARACHUTE_DROP;

    for (const auto& pos : dropPositions)
    {
        positions.push_back(Position(pos.x, pos.y, pos.z, 0));
    }

    return positions;
}

// ============================================================================
// SIEGE ABSTRACT IMPLEMENTATIONS
// ============================================================================

uint32 IsleOfConquestScript::GetBossEntry(uint32 faction) const
{
    return (faction == ALLIANCE) ?
        IsleOfConquest::HIGH_COMMANDER_HALFORD :
        IsleOfConquest::OVERLORD_AGMAR;
}

Position IsleOfConquestScript::GetBossPosition(uint32 faction) const
{
    if (faction == ALLIANCE)
    {
        return Position(
            IsleOfConquest::Bosses::HALFORD_X,
            IsleOfConquest::Bosses::HALFORD_Y,
            IsleOfConquest::Bosses::HALFORD_Z,
            0);
    }
    else
    {
        return Position(
            IsleOfConquest::Bosses::AGMAR_X,
            IsleOfConquest::Bosses::AGMAR_Y,
            IsleOfConquest::Bosses::AGMAR_Z,
            0);
    }
}

bool IsleOfConquestScript::CanAttackBoss(uint32 targetFaction) const
{
    return CanAccessKeep(targetFaction);
}

// ============================================================================
// HELPER METHODS
// ============================================================================

void IsleOfConquestScript::UpdateNodeStates()
{
    // Node states are updated via events, but we can validate here
    TC_LOG_DEBUG("playerbots.bg.script",
        "IsleOfConquestScript: Node states - Refinery={} Quarry={} Docks={} Hangar={} Workshop={}",
        m_nodeControl[0], m_nodeControl[1], m_nodeControl[2], m_nodeControl[3], m_nodeControl[4]);
}

void IsleOfConquestScript::UpdateGateStates()
{
    TC_LOG_DEBUG("playerbots.bg.script",
        "IsleOfConquestScript: Gate states - AF={} AW={} AE={} HF={} HW={} HE={}",
        m_gateIntact[0], m_gateIntact[1], m_gateIntact[2],
        m_gateIntact[3], m_gateIntact[4], m_gateIntact[5]);
}

void IsleOfConquestScript::UpdateVehicleStates()
{
    // Update vehicle availability based on node control
    m_vehicleAvailability.clear();

    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;

    if (IsNodeControlled(IsleOfConquest::ObjectiveIds::DOCKS, faction))
    {
        m_vehicleAvailability[IsleOfConquest::Vehicles::GLAIVE_THROWER] = 2;
        m_vehicleAvailability[IsleOfConquest::Vehicles::CATAPULT] = 2;
    }

    if (IsNodeControlled(IsleOfConquest::ObjectiveIds::WORKSHOP, faction))
    {
        m_vehicleAvailability[IsleOfConquest::Vehicles::DEMOLISHER] = 4;
        m_vehicleAvailability[IsleOfConquest::Vehicles::SIEGE_ENGINE] = 2;
    }
}

bool IsleOfConquestScript::ShouldPrioritizeVehicles() const
{
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;

    // Prioritize vehicles if we control Workshop and have vehicles ready
    if (!IsWorkshopControlled(faction))
        return false;

    auto it = m_vehicleAvailability.find(IsleOfConquest::Vehicles::DEMOLISHER);
    return it != m_vehicleAvailability.end() && it->second >= IsleOfConquest::Strategy::MIN_SIEGE_VEHICLES;
}

bool IsleOfConquestScript::ShouldUseParachuteAssault() const
{
    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 targetFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;

    // Use parachute assault if we control Hangar and have limited gate access
    if (!IsHangarControlled(faction))
        return false;

    // If we can already access keep through gates, no need for parachute
    if (CanAccessKeep(targetFaction))
        return false;

    // Parachute assault is viable
    return true;
}

uint32 IsleOfConquestScript::GetBestGateTarget(uint32 attackingFaction) const
{
    uint32 targetFaction = (attackingFaction == ALLIANCE) ? HORDE : ALLIANCE;
    auto priority = GetGatePriorityOrder(targetFaction);

    for (uint32 gateId : priority)
    {
        if (!IsGateDestroyed(gateId))
            return gateId;
    }

    return 0;  // All gates destroyed
}

// ============================================================================
// RUNTIME BEHAVIOR - ExecuteStrategy
// ============================================================================

bool IsleOfConquestScript::ExecuteStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    // Skip if already in a vehicle - vehicle AI handles actions
    if (player->GetVehicle())
        return true;

    uint32 faction = player->GetBGTeam();
    uint32 targetFaction = (faction == ALLIANCE) ? HORDE : ALLIANCE;
    IOCPhase currentPhase = GetCurrentPhase();
    uint32 dutySlot = player->GetGUID().GetCounter() % 10;

    // =========================================================================
    // PRIORITY 1: Enemy nearby -> engage
    // =========================================================================
    if (::Player* enemy = FindNearestEnemyPlayer(player, 20.0f))
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "[IOC] {} P1: engaging enemy {} (dist={:.0f})",
            player->GetName(), enemy->GetName(),
            player->GetExactDist(enemy));
        EngageTarget(player, enemy);
        return true;
    }

    // =========================================================================
    // PRIORITY 2: Nearby capturable node -> capture it
    // =========================================================================
    for (uint32 i = 0; i < IsleOfConquest::ObjectiveIds::NODE_COUNT; ++i)
    {
        if (IsNodeControlled(i, faction))
            continue;

        const auto& posData = IsleOfConquest::NodePositions::POSITIONS[i];
        Position nodePos(posData.x, posData.y, posData.z, 0);
        float dist = player->GetExactDist(&nodePos);

        if (dist < 30.0f)
        {
            TC_LOG_DEBUG("playerbots.bg.script",
                "[IOC] {} P2: capturing node {} (dist={:.0f})",
                player->GetName(), IsleOfConquest::GetNodeName(i), dist);

            if (dist < 8.0f)
                TryInteractWithGameObject(player, 29 /*GAMEOBJECT_TYPE_CAPTURE_POINT*/, 10.0f);
            else
                Playerbot::BotMovementUtil::MoveToPosition(player, nodePos);

            return true;
        }
    }

    // =========================================================================
    // PRIORITY 3: Phase-based duty
    // =========================================================================
    switch (currentPhase)
    {
        case IOCPhase::OPENING:
        case IOCPhase::NODE_CAPTURE:
        {
            // Split between capturing nodes based on priority order
            auto priorities = GetNodePriorityOrder(faction);
            for (uint32 nodeId : priorities)
            {
                if (!IsNodeControlled(nodeId, faction))
                {
                    const auto& posData = IsleOfConquest::NodePositions::POSITIONS[nodeId];
                    Position nodePos(posData.x, posData.y, posData.z, 0);

                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[IOC] {} P3 (NODE_CAPTURE): moving to {}",
                        player->GetName(), IsleOfConquest::GetNodeName(nodeId));
                    Playerbot::BotMovementUtil::MoveToPosition(player, nodePos);
                    return true;
                }
            }

            // All nodes controlled -> defend Workshop or Hangar
            if (IsWorkshopControlled(faction))
            {
                auto defPos = GetNodeDefensePositions(IsleOfConquest::ObjectiveIds::WORKSHOP);
                if (!defPos.empty())
                {
                    uint32 posIdx = player->GetGUID().GetCounter() % defPos.size();
                    PatrolAroundPosition(player, defPos[posIdx], 3.0f, 10.0f);
                    return true;
                }
            }
            break;
        }

        case IOCPhase::VEHICLE_SIEGE:
        {
            // Slots 0-2 (30%): try to board siege vehicles
            if (dutySlot < 3)
            {
                // Try Siege Engine first (most powerful), then Demolisher, then Glaive/Catapult
                uint32 siegeEntry = (faction == ALLIANCE) ?
                    IsleOfConquest::Vehicles::SIEGE_ENGINE_A :
                    IsleOfConquest::Vehicles::SIEGE_ENGINE_H;

                if (IsWorkshopControlled(faction))
                {
                    if (TryBoardNearbyVehicle(player, siegeEntry, 50.0f) ||
                        TryBoardNearbyVehicle(player, IsleOfConquest::Vehicles::DEMOLISHER, 50.0f))
                    {
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[IOC] {} P3 (VEHICLE_SIEGE): boarding Workshop vehicle",
                            player->GetName());
                        return true;
                    }
                }

                if (IsNodeControlled(IsleOfConquest::ObjectiveIds::DOCKS, faction))
                {
                    if (TryBoardNearbyVehicle(player, IsleOfConquest::Vehicles::GLAIVE_THROWER, 50.0f) ||
                        TryBoardNearbyVehicle(player, IsleOfConquest::Vehicles::CATAPULT, 50.0f))
                    {
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[IOC] {} P3 (VEHICLE_SIEGE): boarding Docks vehicle",
                            player->GetName());
                        return true;
                    }
                }

                // No vehicle available - move to vehicle staging area
                auto stagingPos = GetVehicleStagingPositions(faction);
                if (!stagingPos.empty())
                {
                    uint32 posIdx = player->GetGUID().GetCounter() % stagingPos.size();
                    Playerbot::BotMovementUtil::MoveToPosition(player, stagingPos[posIdx]);
                    return true;
                }
            }

            // Slots 3-4 (20%): parachute assault if Hangar controlled + gates intact
            if (dutySlot >= 3 && dutySlot < 5 && ShouldUseParachuteAssault())
            {
                auto dropPositions = GetParachuteDropPositions(targetFaction);
                if (!dropPositions.empty())
                {
                    uint32 posIdx = player->GetGUID().GetCounter() % dropPositions.size();
                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[IOC] {} P3 (VEHICLE_SIEGE): parachute assault into enemy keep!",
                        player->GetName());
                    Playerbot::BotMovementUtil::MoveToPosition(player, dropPositions[posIdx]);
                    return true;
                }
            }

            // Slots 5-9 (50%): infantry escort/assault toward gates
            {
                uint32 targetGate = GetBestGateTarget(faction);
                if (targetGate != 0)
                {
                    auto approachPos = GetGateApproachPositions(targetGate);
                    if (!approachPos.empty())
                    {
                        uint32 posIdx = player->GetGUID().GetCounter() % approachPos.size();
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[IOC] {} P3 (VEHICLE_SIEGE): infantry assault gate {}",
                            player->GetName(), targetGate);
                        Playerbot::BotMovementUtil::MoveToPosition(player, approachPos[posIdx]);
                        return true;
                    }
                }
            }
            break;
        }

        case IOCPhase::GATE_ASSAULT:
        {
            // Slots 0-2 (30%): still try vehicles for remaining gates
            if (dutySlot < 3)
            {
                uint32 siegeEntry = (faction == ALLIANCE) ?
                    IsleOfConquest::Vehicles::SIEGE_ENGINE_A :
                    IsleOfConquest::Vehicles::SIEGE_ENGINE_H;

                if (IsWorkshopControlled(faction))
                {
                    if (TryBoardNearbyVehicle(player, siegeEntry, 50.0f) ||
                        TryBoardNearbyVehicle(player, IsleOfConquest::Vehicles::DEMOLISHER, 50.0f))
                    {
                        return true;
                    }
                }
            }

            // Rush enemy gates
            auto gatePriority = GetGatePriorityOrder(targetFaction);
            for (uint32 gateId : gatePriority)
            {
                if (!IsGateDestroyed(gateId))
                {
                    auto approachPos = GetGateApproachPositions(gateId);
                    if (!approachPos.empty())
                    {
                        uint32 posIdx = player->GetGUID().GetCounter() % approachPos.size();
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[IOC] {} P3 (GATE_ASSAULT): approaching gate {}",
                            player->GetName(), gateId);
                        Playerbot::BotMovementUtil::MoveToPosition(player, approachPos[posIdx]);
                        return true;
                    }
                }
            }
            break;
        }

        case IOCPhase::BOSS_ASSAULT:
        {
            // 90% rush enemy boss
            if (dutySlot < 9)
            {
                // Engage enemy players near boss first
                if (::Player* enemy = FindNearestEnemyPlayer(player, 30.0f))
                {
                    EngageTarget(player, enemy);
                }
                else
                {
                    // No enemy players - attack the boss NPC
                    QueueBossAttack(player, targetFaction);
                }

                // Move to boss raid positions
                auto raidPositions = GetBossRaidPositions(targetFaction);
                if (!raidPositions.empty())
                {
                    uint32 posIdx = player->GetGUID().GetCounter() % raidPositions.size();
                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[IOC] {} P3 (BOSS_ASSAULT): rushing enemy boss!",
                        player->GetName());
                    PatrolAroundPosition(player, raidPositions[posIdx], 1.0f, 5.0f);
                    return true;
                }

                // Fallback: move to boss position directly
                Position bossPos = GetBossPosition(targetFaction);
                Playerbot::BotMovementUtil::MoveToPosition(player, bossPos);
                return true;
            }
            else
            {
                // 10% defend our nodes
                auto priorities = GetNodePriorityOrder(faction);
                for (uint32 nodeId : priorities)
                {
                    if (IsNodeControlled(nodeId, faction))
                    {
                        auto defPos = GetNodeDefensePositions(nodeId);
                        if (!defPos.empty())
                        {
                            uint32 posIdx = player->GetGUID().GetCounter() % defPos.size();
                            PatrolAroundPosition(player, defPos[posIdx], 3.0f, 10.0f);
                            return true;
                        }
                    }
                }
            }
            break;
        }

        case IOCPhase::DEFENSE:
        {
            // Protect our boss - engage any enemy first
            if (::Player* enemy = FindNearestEnemyPlayer(player, 30.0f))
            {
                EngageTarget(player, enemy);
                return true;
            }

            // Patrol boss room
            auto raidPos = GetBossRaidPositions(faction);
            if (!raidPos.empty())
            {
                uint32 posIdx = player->GetGUID().GetCounter() % raidPos.size();
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[IOC] {} P3 (DEFENSE): defending our boss",
                    player->GetName());
                PatrolAroundPosition(player, raidPos[posIdx], 3.0f, 10.0f);
                return true;
            }
            Position bossPos = GetBossPosition(faction);
            PatrolAroundPosition(player, bossPos, 5.0f, 15.0f);
            return true;
        }

        case IOCPhase::DESPERATE:
        {
            // All-in: rush enemy boss if behind, defend if ahead
            uint32 ourReinf = GetReinforcements(faction);
            uint32 theirReinf = GetReinforcements(targetFaction);

            if (ourReinf < theirReinf)
            {
                // Behind -> boss rush with NPC attack
                if (::Player* enemy = FindNearestEnemyPlayer(player, 30.0f))
                {
                    EngageTarget(player, enemy);
                }
                else
                {
                    QueueBossAttack(player, targetFaction);
                }

                Position bossPos = GetBossPosition(targetFaction);
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[IOC] {} P3 (DESPERATE): all-in boss rush!",
                    player->GetName());
                Playerbot::BotMovementUtil::MoveToPosition(player, bossPos);
                return true;
            }
            else
            {
                // Ahead -> defend
                if (::Player* enemy = FindNearestEnemyPlayer(player, 30.0f))
                {
                    EngageTarget(player, enemy);
                    return true;
                }
                Position bossPos = GetBossPosition(faction);
                PatrolAroundPosition(player, bossPos, 5.0f, 15.0f);
                return true;
            }
        }

        default:
            break;
    }

    // =========================================================================
    // PRIORITY 4: Fallback -> patrol
    // =========================================================================
    {
        auto chokepoints = GetChokepoints();
        if (!chokepoints.empty())
        {
            uint32 chokeIdx = player->GetGUID().GetCounter() % chokepoints.size();
            TC_LOG_DEBUG("playerbots.bg.script",
                "[IOC] {} P4: patrolling chokepoint",
                player->GetName());
            PatrolAroundPosition(player, chokepoints[chokeIdx], 5.0f, 15.0f);
            return true;
        }
    }

    return false;
}

void IsleOfConquestScript::QueueBossAttack(::Player* bot, uint32 targetFaction)
{
    ObjectGuid bossGuid = (targetFaction == ALLIANCE) ? m_halfordGuid : m_agmarGuid;
    if (bossGuid.IsEmpty())
        return;

    // Don't re-queue if already attacking this boss
    if (bot->GetVictim() && bot->GetVictim()->GetGUID() == bossGuid)
        return;

    sBotActionMgr->QueueAction(Playerbot::BotAction::AttackTarget(
        bot->GetGUID(), bossGuid, getMSTime()));
}

} // namespace Playerbot::Coordination::Battleground
