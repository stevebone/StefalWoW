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

#include "StrandOfTheAncientsScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "Player.h"
#include "Timer.h"
#include "Log.h"
#include "../../../Movement/BotMovementUtil.h"

namespace Playerbot::Coordination::Battleground
{

REGISTER_BG_SCRIPT(StrandOfTheAncientsScript, 607);  // StrandOfTheAncients::MAP_ID

// ============================================================================
// LIFECYCLE
// ============================================================================

void StrandOfTheAncientsScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    SiegeScriptBase::OnLoad(coordinator);
    m_cachedObjectives = GetObjectiveData();

    // Initialize state
    m_isAttacker = false;
    m_currentRound = 1;
    m_round1Time = 0;
    m_round1Victory = false;
    m_relicCaptured = false;
    m_pathDecided = false;
    m_currentPath = StrandOfTheAncients::AttackPath::SPLIT;

    // Initialize arrays
    m_gateDestroyed.fill(false);
    m_gateHealth.fill(0);
    m_graveyardCaptured.fill(false);

    // Set initial gate health
    for (uint32 i = 0; i < StrandOfTheAncients::Gates::COUNT; ++i)
        m_gateHealth[i] = StrandOfTheAncients::Gates::GetGateHealth(i);

    TC_LOG_DEBUG("playerbots.bg.script",
        "StrandOfTheAncientsScript: Loaded (6 gates, 5 graveyards, round-based siege)");
}

void StrandOfTheAncientsScript::OnMatchStart()
{
    SiegeScriptBase::OnMatchStart();

    uint32 now = getMSTime();
    m_matchStartTime = now;
    m_roundStartTime = now;
    m_lastStrategyUpdate = now;
    m_lastGateCheck = now;

    TC_LOG_DEBUG("playerbots.bg.script",
        "StrandOfTheAncientsScript: Match started, Round 1, %s",
        m_isAttacker ? "ATTACKING" : "DEFENDING");
}

void StrandOfTheAncientsScript::OnMatchEnd(bool victory)
{
    SiegeScriptBase::OnMatchEnd(victory);

    uint32 duration = getMSTime() - m_matchStartTime;
    uint32 gatesDestroyed = GetDestroyedGateCount();

    TC_LOG_DEBUG("playerbots.bg.script",
        "StrandOfTheAncientsScript: Match ended (victory=%s, duration=%ums, gates=%u, relic=%s)",
        victory ? "true" : "false", duration, gatesDestroyed,
        m_relicCaptured ? "captured" : "defended");
}

void StrandOfTheAncientsScript::OnUpdate(uint32 diff)
{
    SiegeScriptBase::OnUpdate(diff);

    uint32 now = getMSTime();

    // Periodic gate state update
    if (now - m_lastGateCheck >= StrandOfTheAncients::Strategy::GATE_CHECK_INTERVAL)
    {
        m_lastGateCheck = now;
        UpdateGateStates();
    }

    // Periodic strategy evaluation
    if (now - m_lastStrategyUpdate >= StrandOfTheAncients::Strategy::STRATEGY_UPDATE_INTERVAL)
    {
        m_lastStrategyUpdate = now;

        if (m_isAttacker && !m_pathDecided)
            EvaluateAttackPath();

        SOTAPhase phase = GetCurrentPhase();
        TC_LOG_TRACE("playerbots.bg.script",
            "StrandOfTheAncientsScript: Phase=%s, Tier=%u, DestroyedGates=%u",
            GetPhaseName(phase), GetCurrentGateTier(), GetDestroyedGateCount());
    }
}

void StrandOfTheAncientsScript::OnEvent(const BGScriptEventData& event)
{
    SiegeScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::GATE_DESTROYED:
            if (event.objectiveId < StrandOfTheAncients::Gates::COUNT)
                OnGateDestroyed(event.objectiveId);
            break;

        case BGScriptEvent::OBJECTIVE_CAPTURED:
            // Graveyard captured (IDs 50-54)
            if (event.objectiveId >= 50 && event.objectiveId < 50 + StrandOfTheAncients::Graveyards::COUNT)
                OnGraveyardCaptured(event.objectiveId - 50);
            // Relic captured (ID 100)
            else if (event.objectiveId == 100)
                OnRelicCapture();
            break;

        case BGScriptEvent::ROUND_STARTED:
            OnRoundStarted(static_cast<uint32>(event.stateValue));
            break;

        case BGScriptEvent::ROUND_ENDED:
            OnRoundEnded(static_cast<uint32>(event.stateValue));
            break;

        case BGScriptEvent::WORLD_STATE_CHANGED:
            // Handle gate damage states
            if (event.stateId == StrandOfTheAncients::WorldStates::ATTACKER_TEAM)
                m_isAttacker = (event.stateValue == 1);  // 1 = Alliance attacking
            break;

        default:
            break;
    }
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void StrandOfTheAncientsScript::OnGateDestroyed(uint32 gateId)
{
    if (gateId >= StrandOfTheAncients::Gates::COUNT)
        return;

    m_gateDestroyed[gateId] = true;
    m_gateHealth[gateId] = 0;

    TC_LOG_DEBUG("playerbots.bg.script",
        "StrandOfTheAncientsScript: Gate %u (%s) destroyed, total=%u",
        gateId, StrandOfTheAncients::GetGateName(gateId), GetDestroyedGateCount());

    // Re-evaluate path when outer gate is destroyed
    if (gateId == StrandOfTheAncients::Gates::GREEN_JADE ||
        gateId == StrandOfTheAncients::Gates::BLUE_SAPPHIRE)
    {
        EvaluateAttackPath();
    }
}

void StrandOfTheAncientsScript::OnGraveyardCaptured(uint32 graveyardId)
{
    if (graveyardId >= StrandOfTheAncients::Graveyards::COUNT)
        return;

    m_graveyardCaptured[graveyardId] = true;

    TC_LOG_DEBUG("playerbots.bg.script",
        "StrandOfTheAncientsScript: Graveyard %u (%s) captured",
        graveyardId, StrandOfTheAncients::GetGraveyardName(graveyardId));
}

void StrandOfTheAncientsScript::OnRoundStarted(uint32 roundNumber)
{
    m_currentRound = roundNumber;
    m_roundStartTime = getMSTime();

    // Swap roles
    m_isAttacker = !m_isAttacker;

    // Reset state for new round
    m_gateDestroyed.fill(false);
    m_graveyardCaptured.fill(false);
    m_relicCaptured = false;
    m_pathDecided = false;
    m_currentPath = StrandOfTheAncients::AttackPath::SPLIT;

    // Reset gate health
    for (uint32 i = 0; i < StrandOfTheAncients::Gates::COUNT; ++i)
        m_gateHealth[i] = StrandOfTheAncients::Gates::GetGateHealth(i);

    TC_LOG_DEBUG("playerbots.bg.script",
        "StrandOfTheAncientsScript: Round %u started, %s",
        roundNumber, m_isAttacker ? "ATTACKING" : "DEFENDING");
}

void StrandOfTheAncientsScript::OnRoundEnded(uint32 roundTime)
{
    if (m_currentRound == 1)
    {
        m_round1Time = roundTime;
        m_round1Victory = m_relicCaptured.load();

        TC_LOG_DEBUG("playerbots.bg.script",
            "StrandOfTheAncientsScript: Round 1 ended, time=%ums, victory=%s",
            roundTime, m_round1Victory ? "yes" : "no");
    }
    else
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "StrandOfTheAncientsScript: Round 2 ended, time=%ums (R1 time=%ums)",
            roundTime, m_round1Time.load());
    }
}

void StrandOfTheAncientsScript::OnRelicCapture()
{
    m_relicCaptured = true;

    TC_LOG_DEBUG("playerbots.bg.script",
        "StrandOfTheAncientsScript: Titan Relic captured!");
}

// ============================================================================
// OBJECTIVE DATA
// ============================================================================

std::vector<BGObjectiveData> StrandOfTheAncientsScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    // Gates
    auto gates = GetGateData();
    objectives.insert(objectives.end(), gates.begin(), gates.end());

    // Graveyards
    auto graveyards = GetGraveyardData();
    objectives.insert(objectives.end(), graveyards.begin(), graveyards.end());

    // Titan Relic
    BGObjectiveData relic;
    relic.id = 100;
    relic.type = ObjectiveType::RELIC;
    relic.name = "Titan Relic";
    relic.x = StrandOfTheAncients::Relic::X;
    relic.y = StrandOfTheAncients::Relic::Y;
    relic.z = StrandOfTheAncients::Relic::Z;
    relic.strategicValue = 10;
    objectives.push_back(relic);

    return objectives;
}

std::vector<BGObjectiveData> StrandOfTheAncientsScript::GetGateData() const
{
    std::vector<BGObjectiveData> gates;

    for (uint32 i = 0; i < StrandOfTheAncients::Gates::COUNT; ++i)
    {
        BGObjectiveData gate;
        Position pos = StrandOfTheAncients::GetGatePosition(i);

        gate.id = i;
        gate.type = ObjectiveType::GATE;
        gate.name = StrandOfTheAncients::GetGateName(i);
        gate.x = pos.GetPositionX();
        gate.y = pos.GetPositionY();
        gate.z = pos.GetPositionZ();

        // Strategic value based on tier
        uint8 tier = StrandOfTheAncients::Gates::GetGateTier(i);
        switch (tier)
        {
            case 1: gate.strategicValue = StrandOfTheAncients::Strategy::OUTER_GATE_PRIORITY; break;
            case 2: gate.strategicValue = StrandOfTheAncients::Strategy::MIDDLE_GATE_PRIORITY; break;
            case 3: gate.strategicValue = StrandOfTheAncients::Strategy::INNER_GATE_PRIORITY; break;
            case 4: gate.strategicValue = StrandOfTheAncients::Strategy::ANCIENT_GATE_PRIORITY; break;
            default: gate.strategicValue = 5; break;
        }

        gates.push_back(gate);
    }

    return gates;
}

std::vector<BGObjectiveData> StrandOfTheAncientsScript::GetGraveyardData() const
{
    std::vector<BGObjectiveData> graveyards;

    for (uint32 i = 0; i < StrandOfTheAncients::Graveyards::COUNT; ++i)
    {
        BGObjectiveData gy;
        Position pos = StrandOfTheAncients::GetGraveyardPosition(i);

        gy.id = 50 + i;
        gy.type = ObjectiveType::GRAVEYARD;
        gy.name = StrandOfTheAncients::GetGraveyardName(i);
        gy.x = pos.GetPositionX();
        gy.y = pos.GetPositionY();
        gy.z = pos.GetPositionZ();
        gy.strategicValue = 6;
        gy.captureTime = 30000;

        graveyards.push_back(gy);
    }

    return graveyards;
}

std::vector<BGObjectiveData> StrandOfTheAncientsScript::GetTowerData() const
{
    // SOTA has turrets, not capturable towers
    return {};
}

// ============================================================================
// POSITION PROVIDERS
// ============================================================================

std::vector<BGPositionData> StrandOfTheAncientsScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;

    bool isAttacking = m_isAttacker;
    if (m_coordinator && faction != m_coordinator->GetFaction())
        isAttacking = !isAttacking;

    if (isAttacking)
    {
        // Attackers spawn at beach
        Position pos = StrandOfTheAncients::GetGraveyardPosition(StrandOfTheAncients::Graveyards::BEACH_GY);
        spawns.push_back(BGPositionData("Beach Spawn",
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SPAWN_POINT, faction, 5));
    }
    else
    {
        // Defenders spawn at relic
        Position pos = StrandOfTheAncients::GetGraveyardPosition(StrandOfTheAncients::Graveyards::DEFENDER_START_GY);
        spawns.push_back(BGPositionData("Defender Spawn",
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SPAWN_POINT, faction, 5));
    }

    return spawns;
}

std::vector<BGPositionData> StrandOfTheAncientsScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Gate positions
    for (uint32 i = 0; i < StrandOfTheAncients::Gates::COUNT; ++i)
    {
        Position pos = StrandOfTheAncients::GetGatePosition(i);
        positions.push_back(BGPositionData(StrandOfTheAncients::GetGateName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::DEFENSIVE_POSITION, 0, 8));
    }

    // Chokepoints
    for (const auto& pos : StrandOfTheAncients::Chokepoints::POSITIONS)
    {
        positions.push_back(BGPositionData(pos.name,
            pos.x, pos.y, pos.z, 0,
            BGPositionData::PositionType::CHOKEPOINT, 0, pos.strategicValue));
    }

    // Sniper positions
    for (const auto& pos : StrandOfTheAncients::SniperPositions::POSITIONS)
    {
        positions.push_back(BGPositionData(pos.name,
            pos.x, pos.y, pos.z, 0,
            BGPositionData::PositionType::SNIPER_POSITION, 0, pos.strategicValue));
    }

    // Demolisher spawns
    for (uint32 i = 0; i < StrandOfTheAncients::Vehicles::SPAWN_COUNT; ++i)
    {
        Position pos = StrandOfTheAncients::GetDemolisherSpawn(i);
        positions.push_back(BGPositionData("Demolisher Spawn",
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::VEHICLE_SPAWN, 0, 7));
    }

    // Titan Relic
    positions.push_back(BGPositionData("Titan Relic",
        StrandOfTheAncients::Relic::X, StrandOfTheAncients::Relic::Y, StrandOfTheAncients::Relic::Z, 0,
        BGPositionData::PositionType::STRATEGIC_POINT, 0, 10));

    return positions;
}

std::vector<BGPositionData> StrandOfTheAncientsScript::GetGraveyardPositions(uint32 /*faction*/) const
{
    std::vector<BGPositionData> graveyards;

    for (uint32 i = 0; i < StrandOfTheAncients::Graveyards::COUNT; ++i)
    {
        Position pos = StrandOfTheAncients::GetGraveyardPosition(i);
        graveyards.push_back(BGPositionData(StrandOfTheAncients::GetGraveyardName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::GRAVEYARD, 0, 6));
    }

    return graveyards;
}

std::vector<BGVehicleData> StrandOfTheAncientsScript::GetVehicleData() const
{
    std::vector<BGVehicleData> vehicles;
    vehicles.push_back(BGVehicleData(StrandOfTheAncients::Vehicles::DEMOLISHER_ENTRY,
        "Demolisher", StrandOfTheAncients::Vehicles::DEMOLISHER_HP, 2, true));
    return vehicles;
}

// ============================================================================
// POSITION HELPER METHODS
// ============================================================================

std::vector<Position> StrandOfTheAncientsScript::GetGateDefensePositions(uint32 gateId) const
{
    return StrandOfTheAncients::GetGateDefensePositions(gateId);
}

std::vector<Position> StrandOfTheAncientsScript::GetTurretPositions() const
{
    std::vector<Position> positions;
    uint8 tier = GetCurrentGateTier();

    if (tier <= 1)
    {
        for (const auto& pos : StrandOfTheAncients::Turrets::OUTER_GATE_TURRETS)
            positions.push_back(pos);
    }
    if (tier <= 2)
    {
        for (const auto& pos : StrandOfTheAncients::Turrets::MIDDLE_GATE_TURRETS)
            positions.push_back(pos);
    }
    if (tier <= 3)
    {
        for (const auto& pos : StrandOfTheAncients::Turrets::INNER_GATE_TURRETS)
            positions.push_back(pos);
    }
    if (tier <= 4)
    {
        for (const auto& pos : StrandOfTheAncients::Turrets::ANCIENT_GATE_TURRETS)
            positions.push_back(pos);
    }

    return positions;
}

std::vector<Position> StrandOfTheAncientsScript::GetDemolisherRoute(StrandOfTheAncients::AttackPath path) const
{
    return StrandOfTheAncients::GetDemolisherRoute(path);
}

std::vector<Position> StrandOfTheAncientsScript::GetDemolisherEscortFormation(const Position& demoPos) const
{
    return StrandOfTheAncients::GetEscortFormation(demoPos);
}

std::vector<Position> StrandOfTheAncientsScript::GetRelicAttackPositions() const
{
    return StrandOfTheAncients::GetRelicAttackPositions();
}

std::vector<Position> StrandOfTheAncientsScript::GetRelicDefensePositions() const
{
    return StrandOfTheAncients::GetRelicDefensePositions();
}

std::vector<Position> StrandOfTheAncientsScript::GetAmbushPositions() const
{
    return StrandOfTheAncients::GetAmbushPositions();
}

std::vector<Position> StrandOfTheAncientsScript::GetChokepoints() const
{
    return StrandOfTheAncients::GetChokepoints();
}

std::vector<Position> StrandOfTheAncientsScript::GetSniperPositions() const
{
    return StrandOfTheAncients::GetSniperPositions();
}

// ============================================================================
// WORLD STATE
// ============================================================================

std::vector<BGWorldState> StrandOfTheAncientsScript::GetInitialWorldStates() const
{
    std::vector<BGWorldState> states;

    states.push_back(BGWorldState(StrandOfTheAncients::WorldStates::ROUND_TIME,
        "Round Time", BGWorldState::StateType::TIMER, StrandOfTheAncients::MAX_DURATION));

    states.push_back(BGWorldState(StrandOfTheAncients::WorldStates::ATTACKER_TEAM,
        "Attacker Team", BGWorldState::StateType::CUSTOM, 1));

    states.push_back(BGWorldState(StrandOfTheAncients::WorldStates::GATE_DESTROYED_COUNT,
        "Gates Destroyed", BGWorldState::StateType::OBJECTIVE_STATE, 0));

    return states;
}

bool StrandOfTheAncientsScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    // Check gate destroyed states
    if (stateId == StrandOfTheAncients::WorldStates::GREEN_JADE_DESTROYED && value == 1)
    {
        outObjectiveId = StrandOfTheAncients::Gates::GREEN_JADE;
        outState = BGObjectiveState::DESTROYED;
        return true;
    }
    if (stateId == StrandOfTheAncients::WorldStates::BLUE_SAPPHIRE_DESTROYED && value == 1)
    {
        outObjectiveId = StrandOfTheAncients::Gates::BLUE_SAPPHIRE;
        outState = BGObjectiveState::DESTROYED;
        return true;
    }
    if (stateId == StrandOfTheAncients::WorldStates::RED_SUN_DESTROYED && value == 1)
    {
        outObjectiveId = StrandOfTheAncients::Gates::RED_SUN;
        outState = BGObjectiveState::DESTROYED;
        return true;
    }
    if (stateId == StrandOfTheAncients::WorldStates::PURPLE_AMETHYST_DESTROYED && value == 1)
    {
        outObjectiveId = StrandOfTheAncients::Gates::PURPLE_AMETHYST;
        outState = BGObjectiveState::DESTROYED;
        return true;
    }
    if (stateId == StrandOfTheAncients::WorldStates::YELLOW_MOON_DESTROYED && value == 1)
    {
        outObjectiveId = StrandOfTheAncients::Gates::YELLOW_MOON;
        outState = BGObjectiveState::DESTROYED;
        return true;
    }
    if (stateId == StrandOfTheAncients::WorldStates::ANCIENT_GATE_DESTROYED && value == 1)
    {
        outObjectiveId = StrandOfTheAncients::Gates::ANCIENT_GATE;
        outState = BGObjectiveState::DESTROYED;
        return true;
    }

    return TryInterpretFromCache(stateId, value, outObjectiveId, outState);
}

void StrandOfTheAncientsScript::GetScoreFromWorldStates(const std::map<int32, int32>& /*states*/,
    uint32& allianceScore, uint32& hordeScore) const
{
    // SOTA doesn't use traditional scoring
    // Score is based on gates destroyed and time
    allianceScore = hordeScore = 0;

    uint32 gatesDestroyed = GetDestroyedGateCount();
    uint32 score = gatesDestroyed * StrandOfTheAncients::GATE_DESTROY_BONUS;

    if (m_relicCaptured)
        score += StrandOfTheAncients::RELIC_CAPTURE_BONUS;

    if (m_isAttacker)
        allianceScore = score;  // Assuming alliance is attacker this round
    else
        hordeScore = score;
}

// ============================================================================
// STRATEGY & ROLE DISTRIBUTION
// ============================================================================

RoleDistribution StrandOfTheAncientsScript::GetRecommendedRoles(const StrategicDecision& /*decision*/,
    float /*scoreAdvantage*/, uint32 timeRemaining) const
{
    RoleDistribution dist;

    if (m_isAttacker)
    {
        // Attack formation
        bool desperate = timeRemaining < StrandOfTheAncients::Strategy::DESPERATE_TIME_THRESHOLD;

        if (desperate)
        {
            // All-out attack
            dist.SetRole(BGRole::VEHICLE_DRIVER, 30, 40);
            dist.SetRole(BGRole::VEHICLE_GUNNER, 15, 20);
            dist.SetRole(BGRole::NODE_ATTACKER, 35, 45);
            dist.SetRole(BGRole::HEALER_OFFENSE, 10, 15);
            dist.SetRole(BGRole::ROAMER, 5, 10);
            dist.reasoning = "Desperate attack - all in!";
        }
        else
        {
            // Balanced attack
            dist.SetRole(BGRole::VEHICLE_DRIVER, 20, 30);
            dist.SetRole(BGRole::VEHICLE_GUNNER, 10, 15);
            dist.SetRole(BGRole::NODE_ATTACKER, 30, 40);
            dist.SetRole(BGRole::HEALER_OFFENSE, 15, 20);
            dist.SetRole(BGRole::ROAMER, 10, 15);
            dist.reasoning = "Balanced attack formation";
        }
    }
    else
    {
        // Defense formation
        bool roundTwoDefense = (m_currentRound == 2 && m_round1Time > 0);

        if (roundTwoDefense && timeRemaining <= m_round1Time)
        {
            // Must hold - turtle up
            dist.SetRole(BGRole::NODE_DEFENDER, 50, 60);
            dist.SetRole(BGRole::TURRET_OPERATOR, 15, 25);
            dist.SetRole(BGRole::HEALER_DEFENSE, 20, 25);
            dist.SetRole(BGRole::ROAMER, 5, 10);
            dist.reasoning = "Turtle defense - must hold!";
        }
        else
        {
            // Standard defense
            dist.SetRole(BGRole::NODE_DEFENDER, 40, 50);
            dist.SetRole(BGRole::TURRET_OPERATOR, 15, 20);
            dist.SetRole(BGRole::HEALER_DEFENSE, 20, 25);
            dist.SetRole(BGRole::ROAMER, 15, 20);
            dist.reasoning = "Standard gate defense";
        }
    }

    return dist;
}

void StrandOfTheAncientsScript::AdjustStrategy(StrategicDecision& decision,
    float /*scoreAdvantage*/, uint32 /*controlledCount*/,
    uint32 /*totalObjectives*/, uint32 timeRemaining) const
{
    SOTAPhase phase = GetCurrentPhase();
    ApplyPhaseStrategy(decision, phase);

    // Override for desperate situations
    if (timeRemaining < StrandOfTheAncients::Strategy::DESPERATE_TIME_THRESHOLD)
    {
        if (m_isAttacker)
        {
            decision.strategy = BGStrategy::ALL_IN;
            decision.reasoning = "Running out of time - all in attack!";
            decision.offenseAllocation = 95;
            decision.defenseAllocation = 5;
        }
        else if (m_currentRound == 2 && m_round1Time > 0 && timeRemaining <= m_round1Time)
        {
            decision.strategy = BGStrategy::TURTLE;
            decision.reasoning = "Must outlast round 1 time!";
            decision.defenseAllocation = 95;
            decision.offenseAllocation = 5;
        }
    }
}

// ============================================================================
// PHASE MANAGEMENT
// ============================================================================

StrandOfTheAncientsScript::SOTAPhase StrandOfTheAncientsScript::GetCurrentPhase() const
{
    uint32 elapsed = getMSTime() - m_roundStartTime;

    // Prep phase
    if (elapsed < StrandOfTheAncients::PREP_TIME)
        return SOTAPhase::PREP;

    // Check for desperate time
    uint32 timeRemaining = (elapsed < StrandOfTheAncients::MAX_DURATION) ?
        (StrandOfTheAncients::MAX_DURATION - elapsed) : 0;
    if (timeRemaining < StrandOfTheAncients::Strategy::DESPERATE_TIME_THRESHOLD)
        return SOTAPhase::DESPERATE;

    // Defenders always in defense phase
    if (!m_isAttacker)
        return SOTAPhase::DEFENSE;

    // Check gate progression for attackers
    if (m_relicCaptured || IsAncientGateDestroyed())
        return SOTAPhase::RELIC_CAPTURE;

    if (IsGateDestroyed(StrandOfTheAncients::Gates::YELLOW_MOON))
        return SOTAPhase::ANCIENT_GATE;

    if (IsGateDestroyed(StrandOfTheAncients::Gates::RED_SUN) ||
        IsGateDestroyed(StrandOfTheAncients::Gates::PURPLE_AMETHYST))
        return SOTAPhase::INNER_GATE;

    if (IsGateDestroyed(StrandOfTheAncients::Gates::GREEN_JADE) ||
        IsGateDestroyed(StrandOfTheAncients::Gates::BLUE_SAPPHIRE))
        return SOTAPhase::MIDDLE_GATES;

    if (elapsed < StrandOfTheAncients::Strategy::OPENING_PHASE_DURATION)
        return SOTAPhase::BEACH_ASSAULT;

    return SOTAPhase::OUTER_GATES;
}

const char* StrandOfTheAncientsScript::GetPhaseName(SOTAPhase phase) const
{
    switch (phase)
    {
        case SOTAPhase::PREP: return "Preparation";
        case SOTAPhase::BEACH_ASSAULT: return "Beach Assault";
        case SOTAPhase::OUTER_GATES: return "Outer Gates";
        case SOTAPhase::MIDDLE_GATES: return "Middle Gates";
        case SOTAPhase::INNER_GATE: return "Inner Gate";
        case SOTAPhase::ANCIENT_GATE: return "Ancient Gate";
        case SOTAPhase::RELIC_CAPTURE: return "Relic Capture";
        case SOTAPhase::DEFENSE: return "Defense";
        case SOTAPhase::DESPERATE: return "Desperate";
        default: return "Unknown";
    }
}

void StrandOfTheAncientsScript::ApplyPhaseStrategy(StrategicDecision& decision, SOTAPhase phase) const
{
    switch (phase)
    {
        case SOTAPhase::PREP:
            decision.strategy = BGStrategy::BALANCED;
            decision.reasoning = "Preparation phase";
            decision.offenseAllocation = 50;
            decision.defenseAllocation = 50;
            break;

        case SOTAPhase::BEACH_ASSAULT:
            ApplyBeachAssaultStrategy(decision);
            break;

        case SOTAPhase::OUTER_GATES:
            ApplyOuterGatesStrategy(decision);
            break;

        case SOTAPhase::MIDDLE_GATES:
            ApplyMiddleGatesStrategy(decision);
            break;

        case SOTAPhase::INNER_GATE:
            ApplyInnerGateStrategy(decision);
            break;

        case SOTAPhase::ANCIENT_GATE:
            ApplyAncientGateStrategy(decision);
            break;

        case SOTAPhase::RELIC_CAPTURE:
            ApplyRelicCaptureStrategy(decision);
            break;

        case SOTAPhase::DEFENSE:
            ApplyDefenseStrategy(decision);
            break;

        case SOTAPhase::DESPERATE:
            ApplyDesperateStrategy(decision);
            break;
    }
}

void StrandOfTheAncientsScript::ApplyBeachAssaultStrategy(StrategicDecision& decision) const
{
    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.reasoning = "Beach assault - push with demos";
    decision.offenseAllocation = 80;
    decision.defenseAllocation = 20;
}

void StrandOfTheAncientsScript::ApplyOuterGatesStrategy(StrategicDecision& decision) const
{
    decision.strategy = BGStrategy::AGGRESSIVE;

    switch (m_currentPath)
    {
        case StrandOfTheAncients::AttackPath::LEFT:
            decision.reasoning = "Focus left path (Green Jade)";
            break;
        case StrandOfTheAncients::AttackPath::RIGHT:
            decision.reasoning = "Focus right path (Blue Sapphire)";
            break;
        default:
            decision.reasoning = "Split attack on outer gates";
            break;
    }

    decision.offenseAllocation = 75;
    decision.defenseAllocation = 25;
}

void StrandOfTheAncientsScript::ApplyMiddleGatesStrategy(StrategicDecision& decision) const
{
    decision.strategy = BGStrategy::AGGRESSIVE;

    switch (m_currentPath)
    {
        case StrandOfTheAncients::AttackPath::LEFT:
            decision.reasoning = "Push Red Sun gate";
            break;
        case StrandOfTheAncients::AttackPath::RIGHT:
            decision.reasoning = "Push Purple Amethyst gate";
            break;
        default:
            decision.reasoning = "Split attack on middle gates";
            break;
    }

    decision.offenseAllocation = 75;
    decision.defenseAllocation = 25;
}

void StrandOfTheAncientsScript::ApplyInnerGateStrategy(StrategicDecision& decision) const
{
    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.reasoning = "All forces on Yellow Moon gate";
    decision.offenseAllocation = 85;
    decision.defenseAllocation = 15;
}

void StrandOfTheAncientsScript::ApplyAncientGateStrategy(StrategicDecision& decision) const
{
    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.reasoning = "Final push - Chamber of Ancient Relics";
    decision.offenseAllocation = 90;
    decision.defenseAllocation = 10;
}

void StrandOfTheAncientsScript::ApplyRelicCaptureStrategy(StrategicDecision& decision) const
{
    decision.strategy = BGStrategy::ALL_IN;
    decision.reasoning = "Capture the Titan Relic!";
    decision.offenseAllocation = 95;
    decision.defenseAllocation = 5;
}

void StrandOfTheAncientsScript::ApplyDefenseStrategy(StrategicDecision& decision) const
{
    uint8 tier = GetCurrentGateTier();

    decision.strategy = BGStrategy::DEFENSIVE;
    decision.defenseAllocation = 80;
    decision.offenseAllocation = 20;

    switch (tier)
    {
        case 1:
            decision.reasoning = "Defend outer gates, kill demos";
            break;
        case 2:
            decision.reasoning = "Defend middle gates, turret coverage";
            break;
        case 3:
            decision.reasoning = "Defend Yellow Moon, fallback ready";
            break;
        case 4:
            decision.reasoning = "Last stand at Ancient Gate!";
            decision.defenseAllocation = 90;
            decision.offenseAllocation = 10;
            break;
        default:
            decision.reasoning = "General defense";
            break;
    }
}

void StrandOfTheAncientsScript::ApplyDesperateStrategy(StrategicDecision& decision) const
{
    if (m_isAttacker)
    {
        decision.strategy = BGStrategy::ALL_IN;
        decision.reasoning = "Desperate attack - no time left!";
        decision.offenseAllocation = 100;
        decision.defenseAllocation = 0;
    }
    else
    {
        decision.strategy = BGStrategy::TURTLE;
        decision.reasoning = "Desperate defense - hold the line!";
        decision.defenseAllocation = 100;
        decision.offenseAllocation = 0;
    }
}

// ============================================================================
// SOTA-SPECIFIC METHODS
// ============================================================================

bool StrandOfTheAncientsScript::IsGateDestroyed(uint32 gateId) const
{
    if (gateId >= StrandOfTheAncients::Gates::COUNT)
        return false;
    return m_gateDestroyed[gateId];
}

bool StrandOfTheAncientsScript::IsAncientGateDestroyed() const
{
    return IsGateDestroyed(StrandOfTheAncients::Gates::ANCIENT_GATE);
}

uint32 StrandOfTheAncientsScript::GetDestroyedGateCount() const
{
    uint32 count = 0;
    for (bool destroyed : m_gateDestroyed)
        if (destroyed) ++count;
    return count;
}

StrandOfTheAncients::AttackPath StrandOfTheAncientsScript::GetRecommendedPath() const
{
    return m_currentPath;
}

std::vector<uint32> StrandOfTheAncientsScript::GetNextTargetGates() const
{
    std::vector<uint32> targets;

    for (uint32 i = 0; i < StrandOfTheAncients::Gates::COUNT; ++i)
    {
        if (!IsGateDestroyed(i) && CanAttackGate(i))
            targets.push_back(i);
    }

    return targets;
}

bool StrandOfTheAncientsScript::CanAttackGate(uint32 gateId) const
{
    if (gateId >= StrandOfTheAncients::Gates::COUNT)
        return false;

    if (IsGateDestroyed(gateId))
        return false;

    auto deps = StrandOfTheAncients::GetGateDependencies(gateId);

    if (deps.empty())
        return true;

    // For Yellow Moon, need EITHER left OR right path
    if (gateId == StrandOfTheAncients::Gates::YELLOW_MOON)
    {
        return IsGateDestroyed(StrandOfTheAncients::Gates::RED_SUN) ||
               IsGateDestroyed(StrandOfTheAncients::Gates::PURPLE_AMETHYST);
    }

    // For other gates, check if all dependencies are destroyed
    for (uint32 dep : deps)
    {
        if (!IsGateDestroyed(dep))
            return false;
    }

    return true;
}

uint8 StrandOfTheAncientsScript::GetCurrentGateTier() const
{
    // Find lowest tier with undestroyed gate
    for (uint32 i = 0; i < StrandOfTheAncients::Gates::COUNT; ++i)
    {
        if (!IsGateDestroyed(i) && CanAttackGate(i))
            return StrandOfTheAncients::Gates::GetGateTier(i);
    }

    return 5;  // All gates destroyed
}

uint32 StrandOfTheAncientsScript::GetPriorityGate() const
{
    auto targets = GetNextTargetGates();
    if (targets.empty())
        return StrandOfTheAncients::Gates::COUNT;

    // If path is decided, prioritize that path
    if (m_pathDecided && m_currentPath != StrandOfTheAncients::AttackPath::SPLIT)
    {
        if (m_currentPath == StrandOfTheAncients::AttackPath::LEFT)
        {
            if (!IsGateDestroyed(StrandOfTheAncients::Gates::GREEN_JADE))
                return StrandOfTheAncients::Gates::GREEN_JADE;
            if (!IsGateDestroyed(StrandOfTheAncients::Gates::RED_SUN))
                return StrandOfTheAncients::Gates::RED_SUN;
        }
        else
        {
            if (!IsGateDestroyed(StrandOfTheAncients::Gates::BLUE_SAPPHIRE))
                return StrandOfTheAncients::Gates::BLUE_SAPPHIRE;
            if (!IsGateDestroyed(StrandOfTheAncients::Gates::PURPLE_AMETHYST))
                return StrandOfTheAncients::Gates::PURPLE_AMETHYST;
        }
    }

    // Return first available target
    return targets[0];
}

uint32 StrandOfTheAncientsScript::GetPriorityDefenseGate() const
{
    // Defend lowest tier undestroyed gate
    for (uint8 tier = 1; tier <= 4; ++tier)
    {
        for (uint32 i = 0; i < StrandOfTheAncients::Gates::COUNT; ++i)
        {
            if (!IsGateDestroyed(i) && StrandOfTheAncients::Gates::GetGateTier(i) == tier)
                return i;
        }
    }

    return StrandOfTheAncients::Gates::ANCIENT_GATE;
}

// ============================================================================
// SIEGE ABSTRACT IMPLEMENTATIONS
// ============================================================================

uint32 StrandOfTheAncientsScript::GetBossEntry(uint32 /*faction*/) const
{
    return StrandOfTheAncients::Relic::TITAN_RELIC_ENTRY;
}

Position StrandOfTheAncientsScript::GetBossPosition(uint32 /*faction*/) const
{
    return StrandOfTheAncients::GetRelicPosition();
}

bool StrandOfTheAncientsScript::CanAttackBoss(uint32 /*faction*/) const
{
    return IsAncientGateDestroyed();
}

// ============================================================================
// HELPER METHODS
// ============================================================================

void StrandOfTheAncientsScript::UpdateGateStates()
{
    // Gate states are typically updated via events
    // This method can be used for periodic verification
}

void StrandOfTheAncientsScript::EvaluateAttackPath()
{
    // If one outer gate is down, focus that path
    bool leftDown = IsGateDestroyed(StrandOfTheAncients::Gates::GREEN_JADE);
    bool rightDown = IsGateDestroyed(StrandOfTheAncients::Gates::BLUE_SAPPHIRE);

    if (leftDown && !rightDown)
    {
        m_currentPath = StrandOfTheAncients::AttackPath::LEFT;
        m_pathDecided = true;
        TC_LOG_DEBUG("playerbots.bg.script", "StrandOfTheAncientsScript: Decided on LEFT path");
    }
    else if (rightDown && !leftDown)
    {
        m_currentPath = StrandOfTheAncients::AttackPath::RIGHT;
        m_pathDecided = true;
        TC_LOG_DEBUG("playerbots.bg.script", "StrandOfTheAncientsScript: Decided on RIGHT path");
    }
    else if (!leftDown && !rightDown)
    {
        // No gates down yet - split attack
        m_currentPath = StrandOfTheAncients::AttackPath::SPLIT;
        m_pathDecided = false;
    }
    else
    {
        // Both down - continue with current or default to left
        if (!m_pathDecided)
            m_currentPath = StrandOfTheAncients::AttackPath::LEFT;
    }
}

bool StrandOfTheAncientsScript::ShouldFocusPath() const
{
    return m_pathDecided && m_currentPath != StrandOfTheAncients::AttackPath::SPLIT;
}

uint32 StrandOfTheAncientsScript::GetRound2TimeTarget() const
{
    return m_round1Time;
}

bool StrandOfTheAncientsScript::IsAheadOfPace() const
{
    if (m_currentRound != 2 || m_round1Time == 0)
        return true;

    uint32 elapsed = getMSTime() - m_roundStartTime;
    uint32 gatesDestroyed = GetDestroyedGateCount();

    // Simple heuristic: should have same gates destroyed in less time
    // This is a rough approximation
    float pace = static_cast<float>(gatesDestroyed) / static_cast<float>(elapsed);
    float round1Pace = static_cast<float>(StrandOfTheAncients::Gates::COUNT) / static_cast<float>(m_round1Time);

    return pace >= round1Pace;
}

// ============================================================================
// RUNTIME BEHAVIOR - ExecuteStrategy
// ============================================================================

bool StrandOfTheAncientsScript::ExecuteStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    // If already in a vehicle, don't execute normal strategy
    if (player->GetVehicle())
        return true;

    uint32 dutySlot = player->GetGUID().GetCounter() % 10;

    // =========================================================================
    // PRIORITY 1: Enemy nearby -> engage
    // =========================================================================
    if (::Player* enemy = FindNearestEnemyPlayer(player, 20.0f))
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "[SOTA] {} PRIORITY 1: engaging enemy {} (dist={:.0f})",
            player->GetName(), enemy->GetName(),
            player->GetExactDist(enemy));
        EngageTarget(player, enemy);
        return true;
    }

    if (m_isAttacker)
    {
        // =================================================================
        // ATTACKING - Duty split: 30% demolisher, 40% escort, 30% infantry
        // =================================================================

        // If the ancient gate is destroyed, ALL rush the relic
        if (IsAncientGateDestroyed())
        {
            auto relicPositions = GetRelicAttackPositions();
            if (!relicPositions.empty())
            {
                uint32 posIdx = player->GetGUID().GetCounter() % relicPositions.size();
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[SOTA] {} RELIC RUSH: rushing Titan Relic!",
                    player->GetName());
                BotMovementUtil::MoveToPosition(player, relicPositions[posIdx]);
                TryInteractWithGameObject(player, 10 /*GAMEOBJECT_TYPE_GOOBER*/, 10.0f);
                return true;
            }
        }

        // PRIORITY 2: Demolisher duty (30% of team, slots 0-2)
        if (dutySlot < 3)
        {
            // Try to board a nearby demolisher
            if (TryBoardNearbyVehicle(player, StrandOfTheAncients::Vehicles::DEMOLISHER_ENTRY, 30.0f))
            {
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[SOTA] {} PRIORITY 2 (ATK): boarding demolisher",
                    player->GetName());
                return true;
            }

            // No demolisher nearby — move toward demolisher spawn
            auto demoRoute = GetDemolisherRoute(m_currentPath);
            if (!demoRoute.empty())
            {
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[SOTA] {} PRIORITY 2 (ATK): moving to demolisher route",
                    player->GetName());
                BotMovementUtil::MoveToPosition(player, demoRoute.front());
                return true;
            }
        }

        // PRIORITY 3: Escort/Infantry duty (70% of team, slots 3-9)
        {
            uint32 priorityGate = GetPriorityGate();
            if (priorityGate < StrandOfTheAncients::Gates::COUNT)
            {
                Position gatePos = StrandOfTheAncients::GetGatePosition(priorityGate);
                float dist = player->GetExactDist(&gatePos);

                TC_LOG_DEBUG("playerbots.bg.script",
                    "[SOTA] {} PRIORITY 3 (ATK): moving to gate {} (dist={:.0f})",
                    player->GetName(), StrandOfTheAncients::GetGateName(priorityGate), dist);
                BotMovementUtil::MoveToPosition(player, gatePos);
                return true;
            }
        }
    }
    else
    {
        // =================================================================
        // DEFENDING - Duty split: 25% turrets, 75% gate defense
        // =================================================================

        // PRIORITY 2: Turret duty (25% of team, slots 0-2)
        if (dutySlot < 3)
        {
            // Try to board a nearby turret
            if (TryBoardNearbyVehicle(player, StrandOfTheAncients::Vehicles::TURRET_ENTRY, 30.0f))
            {
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[SOTA] {} PRIORITY 2 (DEF): boarding turret",
                    player->GetName());
                return true;
            }

            // No turret nearby — move toward turret position
            auto turretPositions = GetTurretPositions();
            if (!turretPositions.empty())
            {
                uint32 turretIdx = (player->GetGUID().GetCounter() / 3) % turretPositions.size();
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[SOTA] {} PRIORITY 2 (DEF): moving to turret position",
                    player->GetName());
                BotMovementUtil::MoveToPosition(player, turretPositions[turretIdx]);
                return true;
            }
        }

        // PRIORITY 3: Gate defense (75% of team, slots 3-9)
        {
            uint32 defenseGate = GetPriorityDefenseGate();
            if (defenseGate < StrandOfTheAncients::Gates::COUNT)
            {
                auto defPositions = GetGateDefensePositions(defenseGate);
                if (!defPositions.empty())
                {
                    uint32 posIdx = player->GetGUID().GetCounter() % defPositions.size();
                    Position defPos = defPositions[posIdx];
                    float dist = player->GetExactDist(&defPos);

                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[SOTA] {} PRIORITY 3 (DEF): defending gate {} (dist={:.0f})",
                        player->GetName(), StrandOfTheAncients::GetGateName(defenseGate), dist);

                    if (dist < 10.0f)
                        PatrolAroundPosition(player, defPos, 3.0f, 10.0f);
                    else
                        BotMovementUtil::MoveToPosition(player, defPos);
                    return true;
                }
            }
        }
    }

    // =========================================================================
    // PRIORITY 4: Fallback -> move to nearest gate position
    // =========================================================================
    {
        float bestDist = std::numeric_limits<float>::max();
        Position bestPos;
        bool found = false;

        for (uint32 i = 0; i < StrandOfTheAncients::Gates::COUNT; ++i)
        {
            if (IsGateDestroyed(i))
                continue;

            Position gatePos = StrandOfTheAncients::GetGatePosition(i);
            float dist = player->GetExactDist(&gatePos);
            if (dist < bestDist)
            {
                bestDist = dist;
                bestPos = gatePos;
                found = true;
            }
        }

        if (found)
        {
            TC_LOG_DEBUG("playerbots.bg.script",
                "[SOTA] {} PRIORITY 4: moving to nearest gate (dist={:.0f})",
                player->GetName(), bestDist);
            BotMovementUtil::MoveToPosition(player, bestPos);
            return true;
        }
    }

    return false;
}

} // namespace Playerbot::Coordination::Battleground
