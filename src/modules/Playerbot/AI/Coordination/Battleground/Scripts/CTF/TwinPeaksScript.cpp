/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Twin Peaks Script Implementation
 * ~750 lines - Complete CTF coordination with phase-aware strategy
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TwinPeaksScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "BotMovementUtil.h"
#include "Player.h"
#include "Log.h"
#include <cmath>

namespace Playerbot::Coordination::Battleground
{

// Register the script
REGISTER_BG_SCRIPT(TwinPeaksScript, 726);  // TwinPeaks::MAP_ID

// ============================================================================
// LIFECYCLE METHODS
// ============================================================================

void TwinPeaksScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    CTFScriptBase::OnLoad(coordinator);

    // Cache objective data
    m_cachedObjectives = GetObjectiveData();

    // Register world state mappings
    RegisterScoreWorldState(TwinPeaks::WorldStates::ALLIANCE_FLAG_CAPTURES, true);
    RegisterScoreWorldState(TwinPeaks::WorldStates::HORDE_FLAG_CAPTURES, false);

    RegisterWorldStateMapping(TwinPeaks::WorldStates::ALLIANCE_FLAG_STATE,
        TwinPeaks::ObjectiveIds::ALLIANCE_FLAG, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(TwinPeaks::WorldStates::HORDE_FLAG_STATE,
        TwinPeaks::ObjectiveIds::HORDE_FLAG, BGObjectiveState::HORDE_CONTROLLED);

    TC_LOG_DEBUG("playerbots.bg.script",
        "TwinPeaksScript: Loaded with {} objectives, {} sniper positions, {} chokepoints",
        m_cachedObjectives.size(),
        TwinPeaks::GetSniperPositions().size(),
        TwinPeaks::GetMiddleChokepoints().size());
}

void TwinPeaksScript::OnMatchStart()
{
    CTFScriptBase::OnMatchStart();

    m_matchActive = true;
    m_matchStartTime = 0;
    m_matchElapsedTime = 0;
    m_currentPhase = TwinPeaksPhase::OPENING;
    m_allianceScore = 0;
    m_hordeScore = 0;
    m_allianceFlagState = TwinPeaks::WorldStates::FLAG_STATE_AT_BASE;
    m_hordeFlagState = TwinPeaks::WorldStates::FLAG_STATE_AT_BASE;
    m_phaseUpdateTimer = 0;

    TC_LOG_DEBUG("playerbots.bg.script",
        "TwinPeaksScript: Match started - OPENING phase begins");
}

void TwinPeaksScript::OnMatchEnd(bool victory)
{
    CTFScriptBase::OnMatchEnd(victory);

    m_matchActive = false;

    TC_LOG_DEBUG("playerbots.bg.script",
        "TwinPeaksScript: Match ended - Result: {}, Final Score: Alliance {} - Horde {}",
        victory ? "Victory" : "Defeat",
        m_allianceScore, m_hordeScore);
}

void TwinPeaksScript::OnUpdate(uint32 diff)
{
    CTFScriptBase::OnUpdate(diff);

    if (!m_matchActive)
        return;

    m_matchElapsedTime += diff;
    m_phaseUpdateTimer += diff;

    // Update phase periodically
    if (m_phaseUpdateTimer >= PHASE_UPDATE_INTERVAL)
    {
        m_phaseUpdateTimer = 0;
        uint32 timeRemaining = TwinPeaks::MAX_DURATION > m_matchElapsedTime ?
            TwinPeaks::MAX_DURATION - m_matchElapsedTime : 0;
        UpdatePhase(m_matchElapsedTime, timeRemaining);
    }
}

void TwinPeaksScript::OnEvent(const BGScriptEventData& event)
{
    CTFScriptBase::OnEvent(event);

    ProcessFlagEvent(event);

    // Enhanced event logging
    switch (event.eventType)
    {
        case BGScriptEvent::FLAG_PICKED_UP:
            TC_LOG_DEBUG("playerbots.bg.script",
                "TwinPeaks: {} flag picked up by {} at ({:.1f}, {:.1f}, {:.1f}) - Phase: {}",
                event.faction == ALLIANCE ? "Alliance" : "Horde",
                event.primaryGuid.ToString(),
                event.x, event.y, event.z,
                static_cast<int>(m_currentPhase.load()));
            break;

        case BGScriptEvent::FLAG_CAPTURED:
            TC_LOG_DEBUG("playerbots.bg.script",
                "TwinPeaks: Flag captured! New score - Alliance: {}, Horde: {} - Phase: {}",
                m_allianceScore, m_hordeScore,
                static_cast<int>(m_currentPhase.load()));
            break;

        case BGScriptEvent::FLAG_DROPPED:
            TC_LOG_DEBUG("playerbots.bg.script",
                "TwinPeaks: Flag dropped at ({:.1f}, {:.1f}, {:.1f}) - {} - Phase: {}",
                event.x, event.y, event.z,
                IsOnBridge(event.x, event.y) ? "ON BRIDGE (critical)" :
                    (IsInWater(event.x, event.y, event.z) ? "IN WATER" : "normal"),
                static_cast<int>(m_currentPhase.load()));
            break;

        case BGScriptEvent::FLAG_RETURNED:
            TC_LOG_DEBUG("playerbots.bg.script",
                "TwinPeaks: Flag returned to {} base - Phase: {}",
                event.faction == ALLIANCE ? "Alliance" : "Horde",
                static_cast<int>(m_currentPhase.load()));
            break;

        default:
            break;
    }
}

// ============================================================================
// RUNTIME BEHAVIOR - Dynamic Behavior Tree
// ============================================================================

bool TwinPeaksScript::ExecuteStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    // Refresh flag carrier state (throttled to 1s)
    RefreshFlagState(player);

    // =========================================================================
    // PRIORITY 1: Carrying flag -> run it home!
    // =========================================================================
    if (IsPlayerCarryingFlag(player))
    {
        TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 1: carrying flag, running home",
            player->GetName());
        RunFlagHome(player);
        return true;
    }

    // =========================================================================
    // PRIORITY 2: Dropped friendly flag nearby -> return it
    // =========================================================================
    if (ReturnDroppedFlag(player))
    {
        TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 2: returning dropped flag",
            player->GetName());
        return true;
    }

    // =========================================================================
    // PRIORITY 3: No flags in play -> phase-aware split with mid-field control
    // =========================================================================
    if (!m_cachedFriendlyFC && !m_cachedEnemyFC)
    {
        uint32 dutySlot = player->GetGUID().GetCounter() % 10;
        // Phase-aware: pickup / defend / mid-field thresholds
        uint32 pickupThreshold, defendThreshold; // pickup < defend < 10 (rest = mid-field)

        switch (m_currentPhase.load())
        {
            case TwinPeaksPhase::OPENING:
                pickupThreshold = 3; // 30% rush enemy flag
                defendThreshold = 7; // 40% defend, 30% mid-field
                break;
            case TwinPeaksPhase::MID_GAME:
                pickupThreshold = 4; // 40% pickup
                defendThreshold = 7; // 30% defend, 30% mid-field
                break;
            case TwinPeaksPhase::LATE_GAME:
                pickupThreshold = 3; // 30% pickup
                defendThreshold = 7; // 40% defend, 30% mid-field
                break;
            case TwinPeaksPhase::DESPERATE:
                pickupThreshold = 6; // 60% all-in rush
                defendThreshold = 8; // 20% defend, 20% mid-field
                break;
            default:
                pickupThreshold = 4;
                defendThreshold = 7;
                break;
        }

        if (dutySlot < pickupThreshold)
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 3: going to pick up enemy flag (phase={})",
                player->GetName(), static_cast<int>(m_currentPhase.load()));
            PickupEnemyFlag(player);
        }
        else if (dutySlot < defendThreshold)
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 3: defending flag room (phase={})",
                player->GetName(), static_cast<int>(m_currentPhase.load()));
            DefendOwnFlagRoom(player);
        }
        else
        {
            // Mid-field control: hold chokepoints and contest mid
            auto chokepoints = GetMiddleChokepoints();
            if (!chokepoints.empty())
            {
                uint32 cpIdx = player->GetGUID().GetCounter() % chokepoints.size();
                Playerbot::BotMovementUtil::MoveToPosition(player, chokepoints[cpIdx]);
                ::Player* nearEnemy = FindNearestEnemyPlayer(player, 20.0f);
                if (nearEnemy)
                    EngageTarget(player, nearEnemy);
            }
            else
            {
                DefendOwnFlagRoom(player);
            }
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 3: holding mid-field (phase={})",
                player->GetName(), static_cast<int>(m_currentPhase.load()));
        }
        return true;
    }

    // =========================================================================
    // PRIORITY 4: Both FCs exist -> phase-aware escort/defend/hunt split
    // =========================================================================
    if (m_cachedFriendlyFC && m_cachedEnemyFC)
    {
        uint32 dutySlot = player->GetGUID().GetCounter() % 10;
        // escort / defend / hunt thresholds
        uint32 escortThreshold, defendThreshold; // escort < defend < 10 (rest = hunt)

        switch (m_currentPhase.load())
        {
            case TwinPeaksPhase::OPENING:
                escortThreshold = 2; // 20% escort
                defendThreshold = 6; // 40% defend, 40% hunt
                break;
            case TwinPeaksPhase::MID_GAME:
                escortThreshold = 2; // 20% escort
                defendThreshold = 6; // 40% defend, 40% hunt
                break;
            case TwinPeaksPhase::LATE_GAME:
                // If we're ahead, escort more to protect cap opportunity
                if (GetScoreAdvantage(player->GetBGTeam()) > 0)
                {
                    escortThreshold = 4; // 40% escort
                    defendThreshold = 7; // 30% defend, 30% hunt
                }
                else
                {
                    escortThreshold = 2; // 20% escort
                    defendThreshold = 5; // 30% defend, 50% hunt
                }
                break;
            case TwinPeaksPhase::DESPERATE:
                escortThreshold = 1; // 10% escort
                defendThreshold = 3; // 20% defend, 70% hunt
                break;
            default:
                escortThreshold = 2;
                defendThreshold = 6;
                break;
        }

        if (dutySlot < escortThreshold)
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 4: escorting friendly FC {} (phase={})",
                player->GetName(), m_cachedFriendlyFC->GetName(), static_cast<int>(m_currentPhase.load()));
            EscortFriendlyFC(player, m_cachedFriendlyFC);
        }
        else if (dutySlot < defendThreshold)
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 4: defending flag room during standoff (phase={})",
                player->GetName(), static_cast<int>(m_currentPhase.load()));
            DefendOwnFlagRoom(player);
        }
        else
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 4: hunting enemy FC {} (phase={})",
                player->GetName(), m_cachedEnemyFC->GetName(), static_cast<int>(m_currentPhase.load()));
            HuntEnemyFC(player, m_cachedEnemyFC);
        }
        return true;
    }

    // =========================================================================
    // PRIORITY 5: Only friendly FC exists -> 60% escort / 40% defend
    // =========================================================================
    if (m_cachedFriendlyFC)
    {
        uint32 dutySlot = player->GetGUID().GetCounter() % 5;
        if (dutySlot < 3) // 60% escort
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 5: escorting friendly FC {}",
                player->GetName(), m_cachedFriendlyFC->GetName());
            EscortFriendlyFC(player, m_cachedFriendlyFC);
        }
        else // 40% defend home flag
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 5: defending flag room while FC runs",
                player->GetName());
            DefendOwnFlagRoom(player);
        }
        return true;
    }

    // =========================================================================
    // PRIORITY 6: Only enemy FC exists -> 60% hunt / 20% grab flag / 20% defend
    // =========================================================================
    if (m_cachedEnemyFC)
    {
        uint32 dutySlot = player->GetGUID().GetCounter() % 5;
        if (dutySlot < 3) // 60% hunt
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 6: hunting enemy FC {}",
                player->GetName(), m_cachedEnemyFC->GetName());
            HuntEnemyFC(player, m_cachedEnemyFC);
        }
        else if (dutySlot == 3) // 20% go grab enemy flag
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 6: picking up enemy flag to force standoff",
                player->GetName());
            PickupEnemyFlag(player);
        }
        else // 20% defend
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 6: defending flag room",
                player->GetName());
            DefendOwnFlagRoom(player);
        }
        return true;
    }

    // =========================================================================
    // PRIORITY 7: Fallback - shouldn't normally reach here
    // =========================================================================
    TC_LOG_DEBUG("playerbots.bg.script", "[TP] {} PRIORITY 7: fallback to flag room defense",
        player->GetName());
    DefendOwnFlagRoom(player);
    return true;
}

// ============================================================================
// DATA PROVIDERS
// ============================================================================

std::vector<BGObjectiveData> TwinPeaksScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    // Alliance Flag
    BGObjectiveData allianceFlag;
    allianceFlag.id = TwinPeaks::ObjectiveIds::ALLIANCE_FLAG;
    allianceFlag.type = ObjectiveType::FLAG;
    allianceFlag.name = "Alliance Flag";
    allianceFlag.x = TwinPeaks::ALLIANCE_FLAG_X;
    allianceFlag.y = TwinPeaks::ALLIANCE_FLAG_Y;
    allianceFlag.z = TwinPeaks::ALLIANCE_FLAG_Z;
    allianceFlag.orientation = TwinPeaks::ALLIANCE_FLAG_O;
    allianceFlag.strategicValue = 10;
    allianceFlag.captureTime = 0;
    allianceFlag.gameObjectEntry = TwinPeaks::GameObjects::ALLIANCE_FLAG;
    allianceFlag.allianceWorldState = TwinPeaks::WorldStates::ALLIANCE_FLAG_STATE;
    allianceFlag.distanceFromAllianceSpawn = 0.0f;
    allianceFlag.distanceFromHordeSpawn = TwinPeaks::Distances::FLAG_TO_FLAG;
    objectives.push_back(allianceFlag);

    // Horde Flag
    BGObjectiveData hordeFlag;
    hordeFlag.id = TwinPeaks::ObjectiveIds::HORDE_FLAG;
    hordeFlag.type = ObjectiveType::FLAG;
    hordeFlag.name = "Horde Flag";
    hordeFlag.x = TwinPeaks::HORDE_FLAG_X;
    hordeFlag.y = TwinPeaks::HORDE_FLAG_Y;
    hordeFlag.z = TwinPeaks::HORDE_FLAG_Z;
    hordeFlag.orientation = TwinPeaks::HORDE_FLAG_O;
    hordeFlag.strategicValue = 10;
    hordeFlag.captureTime = 0;
    hordeFlag.gameObjectEntry = TwinPeaks::GameObjects::HORDE_FLAG;
    hordeFlag.hordeWorldState = TwinPeaks::WorldStates::HORDE_FLAG_STATE;
    hordeFlag.distanceFromAllianceSpawn = TwinPeaks::Distances::FLAG_TO_FLAG;
    hordeFlag.distanceFromHordeSpawn = 0.0f;
    objectives.push_back(hordeFlag);

    // Connect flags
    objectives[0].connectedObjectives.push_back(TwinPeaks::ObjectiveIds::HORDE_FLAG);
    objectives[1].connectedObjectives.push_back(TwinPeaks::ObjectiveIds::ALLIANCE_FLAG);

    return objectives;
}

std::vector<BGPositionData> TwinPeaksScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;

    if (faction == ALLIANCE)
    {
        for (const auto& pos : TwinPeaks::ALLIANCE_SPAWNS)
        {
            BGPositionData spawn;
            spawn.name = "Alliance Spawn";
            spawn.x = pos.GetPositionX();
            spawn.y = pos.GetPositionY();
            spawn.z = pos.GetPositionZ();
            spawn.orientation = pos.GetOrientation();
            spawn.faction = ALLIANCE;
            spawn.posType = BGPositionData::PositionType::SPAWN_POINT;
            spawn.importance = 5;
            spawns.push_back(spawn);
        }
    }
    else
    {
        for (const auto& pos : TwinPeaks::HORDE_SPAWNS)
        {
            BGPositionData spawn;
            spawn.name = "Horde Spawn";
            spawn.x = pos.GetPositionX();
            spawn.y = pos.GetPositionY();
            spawn.z = pos.GetPositionZ();
            spawn.orientation = pos.GetOrientation();
            spawn.faction = HORDE;
            spawn.posType = BGPositionData::PositionType::SPAWN_POINT;
            spawn.importance = 5;
            spawns.push_back(spawn);
        }
    }

    return spawns;
}

std::vector<BGPositionData> TwinPeaksScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Flag room defenses
    auto allianceDefense = TwinPeaks::GetAllianceFlagRoomDefense();
    for (const auto& pos : allianceDefense)
    {
        BGPositionData p("Alliance Flag Defense", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::FLAG_ROOM, ALLIANCE, 8);
        positions.push_back(p);
    }

    auto hordeDefense = TwinPeaks::GetHordeFlagRoomDefense();
    for (const auto& pos : hordeDefense)
    {
        BGPositionData p("Horde Flag Defense", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::FLAG_ROOM, HORDE, 8);
        positions.push_back(p);
    }

    // Chokepoints
    auto chokepoints = GetChokepoints();
    for (const auto& p : chokepoints)
        positions.push_back(p);

    // Sniper positions
    auto sniperPositions = GetSniperPositions();
    for (const auto& p : sniperPositions)
        positions.push_back(p);

    // Buffs
    auto buffs = GetBuffPositions();
    for (const auto& p : buffs)
        positions.push_back(p);

    // Ambush positions (both factions)
    auto allianceAmbush = GetAmbushPositions(ALLIANCE);
    for (const auto& p : allianceAmbush)
        positions.push_back(p);

    auto hordeAmbush = GetAmbushPositions(HORDE);
    for (const auto& p : hordeAmbush)
        positions.push_back(p);

    // River crossings
    auto riverCrossings = TwinPeaks::GetRiverCrossingPoints();
    for (const auto& pos : riverCrossings)
    {
        BGPositionData p("River Crossing", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::STRATEGIC_POINT, 0, 6);
        p.description = "Water crossing - affects movement speed";
        positions.push_back(p);
    }

    return positions;
}

std::vector<BGPositionData> TwinPeaksScript::GetGraveyardPositions(uint32 faction) const
{
    std::vector<BGPositionData> graveyards;

    if (faction == 0 || faction == ALLIANCE)
    {
        Position allyGY = TwinPeaks::GetAllianceGraveyard();
        BGPositionData p("Alliance Graveyard", allyGY.GetPositionX(),
            allyGY.GetPositionY(), allyGY.GetPositionZ(), allyGY.GetOrientation(),
            BGPositionData::PositionType::GRAVEYARD, ALLIANCE, 5);
        graveyards.push_back(p);
    }

    if (faction == 0 || faction == HORDE)
    {
        Position hordeGY = TwinPeaks::GetHordeGraveyard();
        BGPositionData p("Horde Graveyard", hordeGY.GetPositionX(),
            hordeGY.GetPositionY(), hordeGY.GetPositionZ(), hordeGY.GetOrientation(),
            BGPositionData::PositionType::GRAVEYARD, HORDE, 5);
        graveyards.push_back(p);
    }

    return graveyards;
}

std::vector<BGWorldState> TwinPeaksScript::GetInitialWorldStates() const
{
    std::vector<BGWorldState> states;

    // Flag states
    states.push_back(BGWorldState(TwinPeaks::WorldStates::ALLIANCE_FLAG_STATE,
        "Alliance Flag State", BGWorldState::StateType::FLAG_STATE,
        TwinPeaks::WorldStates::FLAG_STATE_AT_BASE));
    states.push_back(BGWorldState(TwinPeaks::WorldStates::HORDE_FLAG_STATE,
        "Horde Flag State", BGWorldState::StateType::FLAG_STATE,
        TwinPeaks::WorldStates::FLAG_STATE_AT_BASE));

    // Scores
    states.push_back(BGWorldState(TwinPeaks::WorldStates::ALLIANCE_FLAG_CAPTURES,
        "Alliance Captures", BGWorldState::StateType::SCORE_ALLIANCE, 0));
    states.push_back(BGWorldState(TwinPeaks::WorldStates::HORDE_FLAG_CAPTURES,
        "Horde Captures", BGWorldState::StateType::SCORE_HORDE, 0));

    // Timer
    states.push_back(BGWorldState(TwinPeaks::WorldStates::TIME_REMAINING,
        "Time Remaining", BGWorldState::StateType::TIMER,
        static_cast<int32>(TwinPeaks::MAX_DURATION / 1000)));

    return states;
}

// ============================================================================
// WORLD STATE
// ============================================================================

bool TwinPeaksScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    if (TryInterpretFromCache(stateId, value, outObjectiveId, outState))
        return true;

    // Alliance flag state
    if (stateId == TwinPeaks::WorldStates::ALLIANCE_FLAG_STATE)
    {
        outObjectiveId = TwinPeaks::ObjectiveIds::ALLIANCE_FLAG;
        switch (value)
        {
            case TwinPeaks::WorldStates::FLAG_STATE_AT_BASE:
                outState = BGObjectiveState::ALLIANCE_CONTROLLED;
                return true;
            case TwinPeaks::WorldStates::FLAG_STATE_PICKED_UP:
                outState = BGObjectiveState::HORDE_CAPTURING;
                return true;
            case TwinPeaks::WorldStates::FLAG_STATE_DROPPED:
                outState = BGObjectiveState::NEUTRAL;
                return true;
        }
    }

    // Horde flag state
    if (stateId == TwinPeaks::WorldStates::HORDE_FLAG_STATE)
    {
        outObjectiveId = TwinPeaks::ObjectiveIds::HORDE_FLAG;
        switch (value)
        {
            case TwinPeaks::WorldStates::FLAG_STATE_AT_BASE:
                outState = BGObjectiveState::HORDE_CONTROLLED;
                return true;
            case TwinPeaks::WorldStates::FLAG_STATE_PICKED_UP:
                outState = BGObjectiveState::ALLIANCE_CAPTURING;
                return true;
            case TwinPeaks::WorldStates::FLAG_STATE_DROPPED:
                outState = BGObjectiveState::NEUTRAL;
                return true;
        }
    }

    return false;
}

void TwinPeaksScript::GetScoreFromWorldStates(const std::map<int32, int32>& states,
    uint32& allianceScore, uint32& hordeScore) const
{
    allianceScore = 0;
    hordeScore = 0;

    auto allyIt = states.find(TwinPeaks::WorldStates::ALLIANCE_FLAG_CAPTURES);
    if (allyIt != states.end())
        allianceScore = static_cast<uint32>(allyIt->second);

    auto hordeIt = states.find(TwinPeaks::WorldStates::HORDE_FLAG_CAPTURES);
    if (hordeIt != states.end())
        hordeScore = static_cast<uint32>(hordeIt->second);

    // Update cached scores
    const_cast<TwinPeaksScript*>(this)->m_allianceScore = allianceScore;
    const_cast<TwinPeaksScript*>(this)->m_hordeScore = hordeScore;
}

// ============================================================================
// STRATEGY AND ROLES
// ============================================================================

RoleDistribution TwinPeaksScript::GetRecommendedRoles(const StrategicDecision& decision,
    float scoreAdvantage, uint32 timeRemaining) const
{
    (void)decision;
    (void)timeRemaining;

    RoleDistribution roles;

    // Base distribution for CTF using BGRole enum
    switch (m_currentPhase.load())
    {
        case TwinPeaksPhase::OPENING:
            // Opening: heavy offense to grab flag first
            roles.SetRole(BGRole::NODE_DEFENDER, TwinPeaks::Strategy::MIN_FLAG_DEFENDERS, TwinPeaks::Strategy::MIN_FLAG_DEFENDERS + 1);
            roles.SetRole(BGRole::FLAG_ESCORT, 0, 1);
            roles.SetRole(BGRole::FLAG_HUNTER, TwinPeaks::Strategy::OPTIMAL_OFFENSE, TwinPeaks::Strategy::OPTIMAL_OFFENSE + 2);
            roles.SetRole(BGRole::ROAMER, 2, 3);
            roles.reasoning = "Opening: grab flag fast";
            break;

        case TwinPeaksPhase::MID_GAME:
            // Standard balanced distribution
            roles.SetRole(BGRole::NODE_DEFENDER, TwinPeaks::Strategy::OPTIMAL_FLAG_DEFENDERS, TwinPeaks::Strategy::OPTIMAL_FLAG_DEFENDERS + 1);
            roles.SetRole(BGRole::FLAG_ESCORT, TwinPeaks::Strategy::OPTIMAL_FC_ESCORT, TwinPeaks::Strategy::OPTIMAL_FC_ESCORT + 1);
            roles.SetRole(BGRole::FLAG_HUNTER, TwinPeaks::Strategy::MIN_OFFENSE, TwinPeaks::Strategy::MIN_OFFENSE + 2);
            roles.SetRole(BGRole::ROAMER, 1, 2);
            roles.reasoning = "Mid-game balanced";
            break;

        case TwinPeaksPhase::LATE_GAME:
            if (scoreAdvantage > 0)
            {
                // Winning: defensive stance
                roles.SetRole(BGRole::NODE_DEFENDER, TwinPeaks::Strategy::MAX_FLAG_DEFENDERS, TwinPeaks::Strategy::MAX_FLAG_DEFENDERS + 1);
                roles.SetRole(BGRole::FLAG_ESCORT, TwinPeaks::Strategy::MAX_FC_ESCORT, TwinPeaks::Strategy::MAX_FC_ESCORT + 1);
                roles.SetRole(BGRole::FLAG_HUNTER, 2, 3);
                roles.SetRole(BGRole::ROAMER, 0, 1);
                roles.reasoning = "Late-game defensive: protect lead";
            }
            else
            {
                // Losing: aggressive push
                roles.SetRole(BGRole::NODE_DEFENDER, TwinPeaks::Strategy::MIN_FLAG_DEFENDERS, TwinPeaks::Strategy::MIN_FLAG_DEFENDERS + 1);
                roles.SetRole(BGRole::FLAG_ESCORT, TwinPeaks::Strategy::MIN_FC_ESCORT, TwinPeaks::Strategy::MIN_FC_ESCORT + 1);
                roles.SetRole(BGRole::FLAG_HUNTER, TwinPeaks::Strategy::OPTIMAL_OFFENSE + 1, TwinPeaks::Strategy::OPTIMAL_OFFENSE + 3);
                roles.SetRole(BGRole::ROAMER, 1, 2);
                roles.reasoning = "Late-game aggressive: catch up";
            }
            break;

        case TwinPeaksPhase::DESPERATE:
            // All-in offense
            roles.SetRole(BGRole::NODE_DEFENDER, 1, 2);
            roles.SetRole(BGRole::FLAG_ESCORT, TwinPeaks::Strategy::MIN_FC_ESCORT, TwinPeaks::Strategy::MIN_FC_ESCORT + 1);
            roles.SetRole(BGRole::FLAG_HUNTER, 6, 8);
            roles.SetRole(BGRole::ROAMER, 0, 1);
            roles.reasoning = "Desperate: all-in offense";
            break;
    }

    return roles;
}

void TwinPeaksScript::AdjustStrategy(StrategicDecision& decision, float scoreAdvantage,
    uint32 controlledCount, uint32 totalObjectives, uint32 timeRemaining) const
{
    (void)controlledCount;   // Unused in CTF
    (void)totalObjectives;   // Unused in CTF

    // Determine faction based on defense allocation bias
    uint32 faction = (decision.defenseAllocation > decision.offenseAllocation) ? ALLIANCE : HORDE;

    // Apply phase-specific strategy
    switch (m_currentPhase.load())
    {
        case TwinPeaksPhase::OPENING:
            ApplyOpeningPhaseStrategy(decision, faction);
            break;

        case TwinPeaksPhase::MID_GAME:
            ApplyMidGameStrategy(decision, scoreAdvantage);
            break;

        case TwinPeaksPhase::LATE_GAME:
            ApplyLateGameStrategy(decision, scoreAdvantage, timeRemaining);
            break;

        case TwinPeaksPhase::DESPERATE:
            ApplyDesperateStrategy(decision);
            break;
    }
}

// ============================================================================
// POSITIONAL DATA PROVIDERS
// ============================================================================

std::vector<Position> TwinPeaksScript::GetObjectivePath(
    uint32 fromObjective, uint32 toObjective) const
{
    if (fromObjective == TwinPeaks::ObjectiveIds::ALLIANCE_FLAG &&
        toObjective == TwinPeaks::ObjectiveIds::HORDE_FLAG)
    {
        return TwinPeaks::GetAllianceFCKitePath();
    }
    else if (fromObjective == TwinPeaks::ObjectiveIds::HORDE_FLAG &&
             toObjective == TwinPeaks::ObjectiveIds::ALLIANCE_FLAG)
    {
        return TwinPeaks::GetHordeFCKitePath();
    }

    return {};
}

std::vector<BGPositionData> TwinPeaksScript::GetSniperPositions() const
{
    std::vector<BGPositionData> positions;
    auto sniperPos = TwinPeaks::GetSniperPositions();

    for (size_t i = 0; i < sniperPos.size(); ++i)
    {
        const auto& pos = sniperPos[i];
        // Determine faction based on position
        uint32 faction = (i < 4) ? ALLIANCE : (i < 6) ? HORDE : 0;

        BGPositionData p("Sniper Position", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SNIPER_POSITION, faction, 7);
        p.description = "Elevated advantage for ranged attackers";
        positions.push_back(p);
    }

    return positions;
}

std::vector<BGPositionData> TwinPeaksScript::GetAmbushPositions(uint32 faction) const
{
    std::vector<BGPositionData> positions;

    auto ambushPos = (faction == ALLIANCE) ?
        TwinPeaks::GetAllianceAmbushPositions() :
        TwinPeaks::GetHordeAmbushPositions();

    for (const auto& pos : ambushPos)
    {
        BGPositionData p("Ambush Position", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::STRATEGIC_POINT, faction, 8);
        p.description = "FC interception point";
        positions.push_back(p);
    }

    return positions;
}

std::vector<BGPositionData> TwinPeaksScript::GetChokepoints() const
{
    std::vector<BGPositionData> positions;
    auto chokepoints = TwinPeaks::GetMiddleChokepoints();

    for (size_t i = 0; i < chokepoints.size(); ++i)
    {
        const auto& pos = chokepoints[i];
        uint8 importance = (i < 5) ? 9 : 7;  // Bridge positions are more important

        BGPositionData p("Chokepoint", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, importance);

        if (i == 0)
            p.description = "Main bridge - critical chokepoint";
        else if (i < 5)
            p.description = "Bridge area";
        else
            p.description = "Flanking route";

        positions.push_back(p);
    }

    return positions;
}

std::vector<BGPositionData> TwinPeaksScript::GetBuffPositions() const
{
    std::vector<BGPositionData> positions;

    // Speed buffs (highest priority for FC)
    auto speedBuffs = TwinPeaks::GetSpeedBuffPositions();
    for (size_t i = 0; i < speedBuffs.size(); ++i)
    {
        uint32 faction = (i == 0) ? ALLIANCE : HORDE;
        BGPositionData p("Speed Buff", speedBuffs[i].GetPositionX(),
            speedBuffs[i].GetPositionY(), speedBuffs[i].GetPositionZ(),
            speedBuffs[i].GetOrientation(),
            BGPositionData::PositionType::BUFF_LOCATION, faction,
            TwinPeaks::Strategy::SPEED_BUFF_PRIORITY);
        p.description = "Critical for flag carriers";
        positions.push_back(p);
    }

    // Restore buffs
    auto restoreBuffs = TwinPeaks::GetRestoreBuffPositions();
    for (size_t i = 0; i < restoreBuffs.size(); ++i)
    {
        uint32 faction = (i == 0) ? ALLIANCE : HORDE;
        BGPositionData p("Restore Buff", restoreBuffs[i].GetPositionX(),
            restoreBuffs[i].GetPositionY(), restoreBuffs[i].GetPositionZ(),
            restoreBuffs[i].GetOrientation(),
            BGPositionData::PositionType::BUFF_LOCATION, faction,
            TwinPeaks::Strategy::RESTORE_BUFF_PRIORITY);
        positions.push_back(p);
    }

    // Berserk buffs
    auto berserkBuffs = TwinPeaks::GetBerserkBuffPositions();
    for (size_t i = 0; i < berserkBuffs.size(); ++i)
    {
        uint32 faction = (i == 0) ? ALLIANCE : HORDE;
        BGPositionData p("Berserk Buff", berserkBuffs[i].GetPositionX(),
            berserkBuffs[i].GetPositionY(), berserkBuffs[i].GetPositionZ(),
            berserkBuffs[i].GetOrientation(),
            BGPositionData::PositionType::BUFF_LOCATION, faction,
            TwinPeaks::Strategy::BERSERK_BUFF_PRIORITY);
        positions.push_back(p);
    }

    return positions;
}

// ============================================================================
// FC ESCORT AND ROUTING
// ============================================================================

std::vector<Position> TwinPeaksScript::GetFCEscortFormation() const
{
    return TwinPeaks::GetFCEscortFormation();
}

std::vector<Position> TwinPeaksScript::GetAbsoluteEscortPositions(const Position& fcPosition) const
{
    std::vector<Position> absolutePositions;
    auto formation = TwinPeaks::GetFCEscortFormation();

    float fcO = fcPosition.GetOrientation();
    float cosO = std::cos(fcO);
    float sinO = std::sin(fcO);

    for (const auto& offset : formation)
    {
        // Rotate offset by FC orientation
        float rotatedX = offset.GetPositionX() * cosO - offset.GetPositionY() * sinO;
        float rotatedY = offset.GetPositionX() * sinO + offset.GetPositionY() * cosO;

        Position absolutePos(
            fcPosition.GetPositionX() + rotatedX,
            fcPosition.GetPositionY() + rotatedY,
            fcPosition.GetPositionZ() + offset.GetPositionZ(),
            fcO + offset.GetOrientation()
        );
        absolutePositions.push_back(absolutePos);
    }

    return absolutePositions;
}

std::vector<Position> TwinPeaksScript::GetFCKitePath(uint32 faction, FCRouteType route) const
{
    if (faction == ALLIANCE)
    {
        switch (route)
        {
            case FCRouteType::DIRECT: return TwinPeaks::GetAllianceFCKitePathDirect();
            case FCRouteType::NORTH:  return TwinPeaks::GetAllianceFCKitePathNorth();
            case FCRouteType::SOUTH:  return TwinPeaks::GetAllianceFCKitePathSouth();
        }
    }
    else
    {
        switch (route)
        {
            case FCRouteType::DIRECT: return TwinPeaks::GetHordeFCKitePathDirect();
            case FCRouteType::NORTH:  return TwinPeaks::GetHordeFCKitePathNorth();
            case FCRouteType::SOUTH:  return TwinPeaks::GetHordeFCKitePathSouth();
        }
    }

    return {};
}

FCRouteType TwinPeaksScript::RecommendFCRoute(uint32 faction,
    const std::vector<Position>& enemyPositions) const
{
    float directRisk = EvaluateRouteRisk(FCRouteType::DIRECT, faction, enemyPositions);
    float northRisk = EvaluateRouteRisk(FCRouteType::NORTH, faction, enemyPositions);
    float southRisk = EvaluateRouteRisk(FCRouteType::SOUTH, faction, enemyPositions);

    // Choose lowest risk route
    if (directRisk <= northRisk && directRisk <= southRisk)
        return FCRouteType::DIRECT;
    else if (northRisk <= southRisk)
        return FCRouteType::NORTH;
    else
        return FCRouteType::SOUTH;
}

float TwinPeaksScript::EvaluateRouteRisk(FCRouteType route, uint32 faction,
    const std::vector<Position>& enemyPositions) const
{
    auto path = GetFCKitePath(faction, route);
    float totalRisk = 0.0f;

    // Base risk for route type
    switch (route)
    {
        case FCRouteType::DIRECT:
            totalRisk = 5.0f;  // Most contested
            break;
        case FCRouteType::NORTH:
            totalRisk = 3.0f;  // Moderate
            break;
        case FCRouteType::SOUTH:
            totalRisk = 4.0f;  // Slower but avoids main traffic
            break;
    }

    // Add risk based on enemy proximity to path
    for (const auto& pathPoint : path)
    {
        for (const auto& enemy : enemyPositions)
        {
            float dx = pathPoint.GetPositionX() - enemy.GetPositionX();
            float dy = pathPoint.GetPositionY() - enemy.GetPositionY();
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance < TwinPeaks::Strategy::AMBUSH_TRIGGER_DISTANCE)
            {
                // Enemy is near this path point - increase risk
                totalRisk += (TwinPeaks::Strategy::AMBUSH_TRIGGER_DISTANCE - distance) / 10.0f;
            }
        }
    }

    return totalRisk;
}

std::vector<Position> TwinPeaksScript::GetFCRouteWaypoints(uint32 faction,
    const std::vector<Position>& enemyPositions) const
{
    FCRouteType bestRoute = RecommendFCRoute(faction, enemyPositions);

    TC_LOG_DEBUG("playerbots.bg.script", "TwinPeaks FC route: selected {} for {}",
        bestRoute == FCRouteType::DIRECT ? "DIRECT" :
        bestRoute == FCRouteType::NORTH ? "NORTH" : "SOUTH",
        faction == ALLIANCE ? "Alliance" : "Horde");

    return GetFCKitePath(faction, bestRoute);
}

// ============================================================================
// PHASE AND STATE QUERIES
// ============================================================================

uint32 TwinPeaksScript::GetMatchRemainingTime() const
{
    if (m_matchElapsedTime >= TwinPeaks::MAX_DURATION)
        return 0;
    return TwinPeaks::MAX_DURATION - m_matchElapsedTime;
}

int32 TwinPeaksScript::GetScoreAdvantage(uint32 faction) const
{
    if (faction == ALLIANCE)
        return static_cast<int32>(m_allianceScore) - static_cast<int32>(m_hordeScore);
    else
        return static_cast<int32>(m_hordeScore) - static_cast<int32>(m_allianceScore);
}

bool TwinPeaksScript::IsFlagAtBase(uint32 faction) const
{
    int32 state = (faction == ALLIANCE) ? m_allianceFlagState : m_hordeFlagState;
    return state == TwinPeaks::WorldStates::FLAG_STATE_AT_BASE;
}

bool TwinPeaksScript::IsFlagCarried(uint32 faction) const
{
    int32 state = (faction == ALLIANCE) ? m_allianceFlagState : m_hordeFlagState;
    return state == TwinPeaks::WorldStates::FLAG_STATE_PICKED_UP;
}

bool TwinPeaksScript::IsFlagDropped(uint32 faction) const
{
    int32 state = (faction == ALLIANCE) ? m_allianceFlagState : m_hordeFlagState;
    return state == TwinPeaks::WorldStates::FLAG_STATE_DROPPED;
}

// ============================================================================
// TERRAIN QUERIES
// ============================================================================

bool TwinPeaksScript::IsInWater(float x, float y, float z) const
{
    return TwinPeaks::IsInWaterZone(x, y, z);
}

bool TwinPeaksScript::IsOnBridge(float x, float y) const
{
    return TwinPeaks::IsOnBridge(x, y);
}

bool TwinPeaksScript::IsInAllianceBase(float x, float y) const
{
    return TwinPeaks::IsInAllianceBase(x, y);
}

bool TwinPeaksScript::IsInHordeBase(float x, float y) const
{
    return TwinPeaks::IsInHordeBase(x, y);
}

float TwinPeaksScript::GetLocationDistance(uint8 fromLoc, uint8 toLoc) const
{
    if (fromLoc >= TwinPeaks::Distances::LOC_COUNT ||
        toLoc >= TwinPeaks::Distances::LOC_COUNT)
        return 0.0f;

    return TwinPeaks::Distances::MATRIX[fromLoc][toLoc];
}

// ============================================================================
// INTERNAL UPDATE METHODS
// ============================================================================

void TwinPeaksScript::UpdatePhase(uint32 timeElapsed, uint32 timeRemaining)
{
    TwinPeaksPhase newPhase = m_currentPhase;

    // Calculate score difference
    int32 scoreDiff = std::abs(static_cast<int32>(m_allianceScore) -
                               static_cast<int32>(m_hordeScore));

    // Check for desperate mode (behind by 2+)
    if (scoreDiff >= static_cast<int32>(TwinPeaks::Strategy::DESPERATE_SCORE_DIFF))
    {
        newPhase = TwinPeaksPhase::DESPERATE;
    }
    // Time-based phase progression
    else if (timeElapsed < TwinPeaks::Strategy::OPENING_PHASE)
    {
        newPhase = TwinPeaksPhase::OPENING;
    }
    else if (timeRemaining <= TwinPeaks::Strategy::LATE_GAME_THRESHOLD)
    {
        newPhase = TwinPeaksPhase::LATE_GAME;
    }
    else
    {
        newPhase = TwinPeaksPhase::MID_GAME;
    }

    if (newPhase != m_currentPhase)
    {
        TC_LOG_DEBUG("playerbots.bg.script",
            "TwinPeaks: Phase transition {} -> {} at {}ms elapsed, score: A{}-H{}",
            static_cast<int>(m_currentPhase.load()), static_cast<int>(newPhase),
            timeElapsed, m_allianceScore, m_hordeScore);
        m_currentPhase = newPhase;
    }
}

void TwinPeaksScript::UpdateFlagStates(const std::map<int32, int32>& worldStates)
{
    auto allyIt = worldStates.find(TwinPeaks::WorldStates::ALLIANCE_FLAG_STATE);
    if (allyIt != worldStates.end())
        m_allianceFlagState = allyIt->second;

    auto hordeIt = worldStates.find(TwinPeaks::WorldStates::HORDE_FLAG_STATE);
    if (hordeIt != worldStates.end())
        m_hordeFlagState = hordeIt->second;
}

void TwinPeaksScript::ProcessFlagEvent(const BGScriptEventData& event)
{
    switch (event.eventType)
    {
        case BGScriptEvent::FLAG_PICKED_UP:
            if (event.faction == ALLIANCE)
                m_allianceFlagState = TwinPeaks::WorldStates::FLAG_STATE_PICKED_UP;
            else
                m_hordeFlagState = TwinPeaks::WorldStates::FLAG_STATE_PICKED_UP;
            break;

        case BGScriptEvent::FLAG_DROPPED:
            if (event.faction == ALLIANCE)
            {
                m_allianceFlagState = TwinPeaks::WorldStates::FLAG_STATE_DROPPED;
                m_droppedAllianceFlagPos = Position(event.x, event.y, event.z, 0);
            }
            else
            {
                m_hordeFlagState = TwinPeaks::WorldStates::FLAG_STATE_DROPPED;
                m_droppedHordeFlagPos = Position(event.x, event.y, event.z, 0);
            }
            break;

        case BGScriptEvent::FLAG_RETURNED:
            if (event.faction == ALLIANCE)
                m_allianceFlagState = TwinPeaks::WorldStates::FLAG_STATE_AT_BASE;
            else
                m_hordeFlagState = TwinPeaks::WorldStates::FLAG_STATE_AT_BASE;
            break;

        case BGScriptEvent::FLAG_CAPTURED:
            // Update scores
            if (event.faction == ALLIANCE)
                m_allianceScore++;
            else
                m_hordeScore++;
            break;

        default:
            break;
    }
}

// ============================================================================
// INTERNAL STRATEGY HELPERS
// ============================================================================

void TwinPeaksScript::ApplyOpeningPhaseStrategy(StrategicDecision& decision, uint32 faction) const
{
    // Opening: aggressive flag grab
    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.attackObjectives.clear();
    decision.attackObjectives.push_back((faction == ALLIANCE) ?
        TwinPeaks::ObjectiveIds::HORDE_FLAG : TwinPeaks::ObjectiveIds::ALLIANCE_FLAG);
    decision.offenseAllocation = 70;
    decision.defenseAllocation = 30;
    decision.reasoning = "Opening: grab flag fast";
    decision.confidence = 0.9f;
}

void TwinPeaksScript::ApplyMidGameStrategy(StrategicDecision& decision, float scoreAdvantage) const
{
    if (scoreAdvantage > 0)
    {
        // Winning: balanced with slight defense
        decision.strategy = BGStrategy::BALANCED;
        decision.offenseAllocation = 40;
        decision.defenseAllocation = 60;
        decision.reasoning = "Mid-game: protect lead";
    }
    else if (scoreAdvantage < 0)
    {
        // Losing: more aggressive
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.offenseAllocation = 65;
        decision.defenseAllocation = 35;
        decision.reasoning = "Mid-game: catch up";
    }
    else
    {
        // Tied: fully balanced
        decision.strategy = BGStrategy::BALANCED;
        decision.offenseAllocation = 50;
        decision.defenseAllocation = 50;
        decision.reasoning = "Mid-game: balanced";
    }
    decision.confidence = 0.75f;
}

void TwinPeaksScript::ApplyLateGameStrategy(StrategicDecision& decision, float scoreAdvantage,
    uint32 timeRemaining) const
{
    (void)timeRemaining;

    if (scoreAdvantage > 0)
    {
        // Winning late game: turtle hard
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.offenseAllocation = 25;
        decision.defenseAllocation = 75;
        decision.reasoning = "Late-game: turtle for win";
    }
    else if (scoreAdvantage < 0)
    {
        // Losing late game: all-in offense
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.offenseAllocation = 80;
        decision.defenseAllocation = 20;
        decision.reasoning = "Late-game: all-in offense";
    }
    else
    {
        // Tied late game: aggressive to break deadlock
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.offenseAllocation = 60;
        decision.defenseAllocation = 40;
        decision.reasoning = "Late-game: break deadlock";
    }
    decision.confidence = 0.7f;
}

void TwinPeaksScript::ApplyDesperateStrategy(StrategicDecision& decision) const
{
    // Desperate: maximum aggression
    decision.strategy = BGStrategy::ALL_IN;
    decision.offenseAllocation = 90;
    decision.defenseAllocation = 10;
    decision.reasoning = "Desperate: all-in attack";
    decision.confidence = 0.5f;
}

} // namespace Playerbot::Coordination::Battleground
