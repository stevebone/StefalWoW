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

#include "WarsongGulchScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "BotMovementUtil.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot::Coordination::Battleground
{

// Register for both classic and remake maps
REGISTER_BG_SCRIPT(WarsongGulchScript, 489);   // WarsongGulch::MAP_ID
REGISTER_BG_SCRIPT(WarsongGulchScript, 2106);  // WarsongGulch::MAP_ID_REMAKE

// ============================================================================
// LIFECYCLE
// ============================================================================

void WarsongGulchScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    CTFScriptBase::OnLoad(coordinator);

    // Cache objective data
    m_cachedObjectives = GetObjectiveData();

    // Register world state mappings
    RegisterScoreWorldState(WarsongGulch::WorldStates::ALLIANCE_FLAG_CAPTURES, true);
    RegisterScoreWorldState(WarsongGulch::WorldStates::HORDE_FLAG_CAPTURES, false);

    // Register flag state mappings
    RegisterWorldStateMapping(WarsongGulch::WorldStates::ALLIANCE_FLAG_STATE,
        WarsongGulch::ObjectiveIds::ALLIANCE_FLAG, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(WarsongGulch::WorldStates::HORDE_FLAG_STATE,
        WarsongGulch::ObjectiveIds::HORDE_FLAG, BGObjectiveState::HORDE_CONTROLLED);

    TC_LOG_DEBUG("playerbots.bg.script",
        "WarsongGulchScript: Loaded with {} objectives",
        m_cachedObjectives.size());
}

// ============================================================================
// DATA PROVIDERS
// ============================================================================

std::vector<BGObjectiveData> WarsongGulchScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    // Alliance Flag
    BGObjectiveData allianceFlag;
    allianceFlag.id = WarsongGulch::ObjectiveIds::ALLIANCE_FLAG;
    allianceFlag.type = ObjectiveType::FLAG;
    allianceFlag.name = "Alliance Flag";
    allianceFlag.x = WarsongGulch::ALLIANCE_FLAG_X;
    allianceFlag.y = WarsongGulch::ALLIANCE_FLAG_Y;
    allianceFlag.z = WarsongGulch::ALLIANCE_FLAG_Z;
    allianceFlag.orientation = WarsongGulch::ALLIANCE_FLAG_O;
    allianceFlag.strategicValue = 10;  // Maximum importance
    allianceFlag.captureTime = 0;      // Instant pickup
    allianceFlag.gameObjectEntry = WarsongGulch::GameObjects::ALLIANCE_FLAG;
    allianceFlag.allianceWorldState = WarsongGulch::WorldStates::ALLIANCE_FLAG_STATE;
    allianceFlag.distanceFromAllianceSpawn = 0.0f;
    allianceFlag.distanceFromHordeSpawn = 650.0f;  // Approximate
    objectives.push_back(allianceFlag);

    // Horde Flag
    BGObjectiveData hordeFlag;
    hordeFlag.id = WarsongGulch::ObjectiveIds::HORDE_FLAG;
    hordeFlag.type = ObjectiveType::FLAG;
    hordeFlag.name = "Horde Flag";
    hordeFlag.x = WarsongGulch::HORDE_FLAG_X;
    hordeFlag.y = WarsongGulch::HORDE_FLAG_Y;
    hordeFlag.z = WarsongGulch::HORDE_FLAG_Z;
    hordeFlag.orientation = WarsongGulch::HORDE_FLAG_O;
    hordeFlag.strategicValue = 10;
    hordeFlag.captureTime = 0;
    hordeFlag.gameObjectEntry = WarsongGulch::GameObjects::HORDE_FLAG;
    hordeFlag.hordeWorldState = WarsongGulch::WorldStates::HORDE_FLAG_STATE;
    hordeFlag.distanceFromAllianceSpawn = 650.0f;
    hordeFlag.distanceFromHordeSpawn = 0.0f;
    objectives.push_back(hordeFlag);

    // Connect flags to each other
    objectives[0].connectedObjectives.push_back(WarsongGulch::ObjectiveIds::HORDE_FLAG);
    objectives[1].connectedObjectives.push_back(WarsongGulch::ObjectiveIds::ALLIANCE_FLAG);

    return objectives;
}

std::vector<BGPositionData> WarsongGulchScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;

    if (faction == ALLIANCE)
    {
        for (const auto& pos : WarsongGulch::ALLIANCE_SPAWNS)
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
        for (const auto& pos : WarsongGulch::HORDE_SPAWNS)
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

std::vector<BGPositionData> WarsongGulchScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Flag rooms
    auto allianceDefense = WarsongGulch::GetAllianceFlagRoomDefense();
    for (const auto& pos : allianceDefense)
    {
        BGPositionData p("Alliance Flag Defense", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::FLAG_ROOM, ALLIANCE, 8);
        positions.push_back(p);
    }

    auto hordeDefense = WarsongGulch::GetHordeFlagRoomDefense();
    for (const auto& pos : hordeDefense)
    {
        BGPositionData p("Horde Flag Defense", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::FLAG_ROOM, HORDE, 8);
        positions.push_back(p);
    }

    // Chokepoints
    auto chokepoints = WarsongGulch::GetMiddleChokepoints();
    for (const auto& pos : chokepoints)
    {
        BGPositionData p("Middle Chokepoint", pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, 6);
        positions.push_back(p);
    }

    // Speed buffs
    auto speedBuffs = WarsongGulch::GetSpeedBuffPositions();
    for (size_t i = 0; i < speedBuffs.size(); ++i)
    {
        uint32 faction = (i == 0) ? ALLIANCE : HORDE;
        BGPositionData p("Speed Buff", speedBuffs[i].GetPositionX(),
            speedBuffs[i].GetPositionY(), speedBuffs[i].GetPositionZ(),
            speedBuffs[i].GetOrientation(),
            BGPositionData::PositionType::BUFF_LOCATION, faction, 7);
        positions.push_back(p);
    }

    // Restore buffs
    auto restoreBuffs = WarsongGulch::GetRestoreBuffPositions();
    for (size_t i = 0; i < restoreBuffs.size(); ++i)
    {
        uint32 faction = (i == 0) ? ALLIANCE : HORDE;
        BGPositionData p("Restore Buff", restoreBuffs[i].GetPositionX(),
            restoreBuffs[i].GetPositionY(), restoreBuffs[i].GetPositionZ(),
            restoreBuffs[i].GetOrientation(),
            BGPositionData::PositionType::BUFF_LOCATION, faction, 5);
        positions.push_back(p);
    }

    // Berserk buffs
    auto berserkBuffs = WarsongGulch::GetBerserkBuffPositions();
    for (size_t i = 0; i < berserkBuffs.size(); ++i)
    {
        uint32 faction = (i == 0) ? ALLIANCE : HORDE;
        BGPositionData p("Berserk Buff", berserkBuffs[i].GetPositionX(),
            berserkBuffs[i].GetPositionY(), berserkBuffs[i].GetPositionZ(),
            berserkBuffs[i].GetOrientation(),
            BGPositionData::PositionType::BUFF_LOCATION, faction, 5);
        positions.push_back(p);
    }

    // Tunnel entrances
    BGPositionData allianceTunnel("Alliance Tunnel", WarsongGulch::ALLIANCE_TUNNEL_X,
        WarsongGulch::ALLIANCE_TUNNEL_Y, WarsongGulch::ALLIANCE_TUNNEL_Z, 0.0f,
        BGPositionData::PositionType::TUNNEL_ENTRANCE, ALLIANCE, 7);
    positions.push_back(allianceTunnel);

    BGPositionData hordeTunnel("Horde Tunnel", WarsongGulch::HORDE_TUNNEL_X,
        WarsongGulch::HORDE_TUNNEL_Y, WarsongGulch::HORDE_TUNNEL_Z, 0.0f,
        BGPositionData::PositionType::TUNNEL_ENTRANCE, HORDE, 7);
    positions.push_back(hordeTunnel);

    return positions;
}

std::vector<BGPositionData> WarsongGulchScript::GetGraveyardPositions(uint32 faction) const
{
    std::vector<BGPositionData> graveyards;

    if (faction == 0 || faction == ALLIANCE)
    {
        Position allyGY = WarsongGulch::GetAllianceGraveyard();
        BGPositionData p("Alliance Graveyard", allyGY.GetPositionX(),
            allyGY.GetPositionY(), allyGY.GetPositionZ(), allyGY.GetOrientation(),
            BGPositionData::PositionType::GRAVEYARD, ALLIANCE, 5);
        graveyards.push_back(p);
    }

    if (faction == 0 || faction == HORDE)
    {
        Position hordeGY = WarsongGulch::GetHordeGraveyard();
        BGPositionData p("Horde Graveyard", hordeGY.GetPositionX(),
            hordeGY.GetPositionY(), hordeGY.GetPositionZ(), hordeGY.GetOrientation(),
            BGPositionData::PositionType::GRAVEYARD, HORDE, 5);
        graveyards.push_back(p);
    }

    return graveyards;
}

std::vector<BGWorldState> WarsongGulchScript::GetInitialWorldStates() const
{
    std::vector<BGWorldState> states;

    // Flag states
    states.push_back(BGWorldState(WarsongGulch::WorldStates::ALLIANCE_FLAG_STATE,
        "Alliance Flag State", BGWorldState::StateType::FLAG_STATE,
        WarsongGulch::WorldStates::FLAG_STATE_AT_BASE));
    states.push_back(BGWorldState(WarsongGulch::WorldStates::HORDE_FLAG_STATE,
        "Horde Flag State", BGWorldState::StateType::FLAG_STATE,
        WarsongGulch::WorldStates::FLAG_STATE_AT_BASE));

    // Scores
    states.push_back(BGWorldState(WarsongGulch::WorldStates::ALLIANCE_FLAG_CAPTURES,
        "Alliance Captures", BGWorldState::StateType::SCORE_ALLIANCE, 0));
    states.push_back(BGWorldState(WarsongGulch::WorldStates::HORDE_FLAG_CAPTURES,
        "Horde Captures", BGWorldState::StateType::SCORE_HORDE, 0));

    // Timer
    states.push_back(BGWorldState(WarsongGulch::WorldStates::TIME_REMAINING,
        "Time Remaining", BGWorldState::StateType::TIMER,
        static_cast<int32>(WarsongGulch::MAX_DURATION / 1000)));

    return states;
}

// ============================================================================
// WORLD STATE
// ============================================================================

bool WarsongGulchScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    // Try cached mappings first
    if (TryInterpretFromCache(stateId, value, outObjectiveId, outState))
        return true;

    // Alliance flag state
    if (stateId == WarsongGulch::WorldStates::ALLIANCE_FLAG_STATE)
    {
        outObjectiveId = WarsongGulch::ObjectiveIds::ALLIANCE_FLAG;
        switch (value)
        {
            case WarsongGulch::WorldStates::FLAG_STATE_AT_BASE:
                outState = BGObjectiveState::ALLIANCE_CONTROLLED;
                return true;
            case WarsongGulch::WorldStates::FLAG_STATE_PICKED_UP:
                outState = BGObjectiveState::HORDE_CAPTURING;  // Horde picked up Alliance flag
                return true;
            case WarsongGulch::WorldStates::FLAG_STATE_DROPPED:
                outState = BGObjectiveState::NEUTRAL;  // Dropped = contestable
                return true;
        }
    }

    // Horde flag state
    if (stateId == WarsongGulch::WorldStates::HORDE_FLAG_STATE)
    {
        outObjectiveId = WarsongGulch::ObjectiveIds::HORDE_FLAG;
        switch (value)
        {
            case WarsongGulch::WorldStates::FLAG_STATE_AT_BASE:
                outState = BGObjectiveState::HORDE_CONTROLLED;
                return true;
            case WarsongGulch::WorldStates::FLAG_STATE_PICKED_UP:
                outState = BGObjectiveState::ALLIANCE_CAPTURING;  // Alliance picked up Horde flag
                return true;
            case WarsongGulch::WorldStates::FLAG_STATE_DROPPED:
                outState = BGObjectiveState::NEUTRAL;
                return true;
        }
    }

    return false;
}

void WarsongGulchScript::GetScoreFromWorldStates(const std::map<int32, int32>& states,
    uint32& allianceScore, uint32& hordeScore) const
{
    allianceScore = 0;
    hordeScore = 0;

    auto allyIt = states.find(WarsongGulch::WorldStates::ALLIANCE_FLAG_CAPTURES);
    if (allyIt != states.end())
        allianceScore = static_cast<uint32>(allyIt->second);

    auto hordeIt = states.find(WarsongGulch::WorldStates::HORDE_FLAG_CAPTURES);
    if (hordeIt != states.end())
        hordeScore = static_cast<uint32>(hordeIt->second);
}

// ============================================================================
// EVENTS
// ============================================================================

void WarsongGulchScript::OnEvent(const BGScriptEventData& event)
{
    CTFScriptBase::OnEvent(event);

    // WSG-specific event handling
    switch (event.eventType)
    {
        case BGScriptEvent::FLAG_PICKED_UP:
            TC_LOG_DEBUG("playerbots.bg.script",
                "WSG: {} flag picked up by {} at ({:.1f}, {:.1f}, {:.1f})",
                event.faction == ALLIANCE ? "Alliance" : "Horde",
                event.primaryGuid.ToString(),
                event.x, event.y, event.z);
            break;

        case BGScriptEvent::FLAG_CAPTURED:
            TC_LOG_DEBUG("playerbots.bg.script",
                "WSG: Flag captured! New score - Alliance: {}, Horde: {}",
                m_allianceCaptures.load(), m_hordeCaptures.load());
            break;

        case BGScriptEvent::FLAG_DROPPED:
            TC_LOG_DEBUG("playerbots.bg.script",
                "WSG: Flag dropped at ({:.1f}, {:.1f}, {:.1f}) - needs return!",
                event.x, event.y, event.z);
            break;

        default:
            break;
    }
}

// ============================================================================
// UTILITY
// ============================================================================

std::vector<Position> WarsongGulchScript::GetObjectivePath(
    uint32 fromObjective, uint32 toObjective) const
{
    // WSG only has two objectives (flags), so the path is the FC kite path
    if (fromObjective == WarsongGulch::ObjectiveIds::ALLIANCE_FLAG &&
        toObjective == WarsongGulch::ObjectiveIds::HORDE_FLAG)
    {
        return WarsongGulch::GetAllianceFCKitePath();
    }
    else if (fromObjective == WarsongGulch::ObjectiveIds::HORDE_FLAG &&
             toObjective == WarsongGulch::ObjectiveIds::ALLIANCE_FLAG)
    {
        return WarsongGulch::GetHordeFCKitePath();
    }

    return {};
}

std::vector<Position> WarsongGulchScript::GetFCRouteWaypoints(uint32 faction,
    const std::vector<Position>& /*enemyPositions*/) const
{
    // WSG uses a single pre-calculated kite path per faction
    if (faction == ALLIANCE)
        return WarsongGulch::GetAllianceFCKitePath();
    else
        return WarsongGulch::GetHordeFCKitePath();
}

std::vector<Position> WarsongGulchScript::GetFlagRunRoute(uint32 faction, bool useSpeedBuff) const
{
    std::vector<Position> route;

    if (faction == ALLIANCE)
    {
        // Alliance running Horde flag home
        route.push_back(WarsongGulch::GetHordeFlagPos());

        if (useSpeedBuff)
        {
            auto speedBuffs = WarsongGulch::GetSpeedBuffPositions();
            route.push_back(speedBuffs[1]);  // Horde side speed buff
        }

        // Through middle
        auto chokepoints = WarsongGulch::GetMiddleChokepoints();
        route.push_back(chokepoints[0]);  // Center

        if (useSpeedBuff)
        {
            auto speedBuffs = WarsongGulch::GetSpeedBuffPositions();
            route.push_back(speedBuffs[0]);  // Alliance side speed buff
        }

        route.push_back(WarsongGulch::GetAllianceFlagPos());
    }
    else
    {
        // Horde running Alliance flag home
        route.push_back(WarsongGulch::GetAllianceFlagPos());

        if (useSpeedBuff)
        {
            auto speedBuffs = WarsongGulch::GetSpeedBuffPositions();
            route.push_back(speedBuffs[0]);  // Alliance side speed buff
        }

        auto chokepoints = WarsongGulch::GetMiddleChokepoints();
        route.push_back(chokepoints[0]);

        if (useSpeedBuff)
        {
            auto speedBuffs = WarsongGulch::GetSpeedBuffPositions();
            route.push_back(speedBuffs[1]);  // Horde side speed buff
        }

        route.push_back(WarsongGulch::GetHordeFlagPos());
    }

    return route;
}

std::vector<Position> WarsongGulchScript::GetTunnelAmbushPositions(uint32 faction) const
{
    std::vector<Position> positions;

    if (faction == ALLIANCE)
    {
        // Ambush positions near Alliance tunnel
        positions.emplace_back(WarsongGulch::ALLIANCE_TUNNEL_X - 10.0f,
            WarsongGulch::ALLIANCE_TUNNEL_Y, WarsongGulch::ALLIANCE_TUNNEL_Z, 0.0f);
        positions.emplace_back(WarsongGulch::ALLIANCE_TUNNEL_X + 10.0f,
            WarsongGulch::ALLIANCE_TUNNEL_Y + 10.0f, WarsongGulch::ALLIANCE_TUNNEL_Z, 0.0f);
    }
    else
    {
        // Ambush positions near Horde tunnel
        positions.emplace_back(WarsongGulch::HORDE_TUNNEL_X + 10.0f,
            WarsongGulch::HORDE_TUNNEL_Y, WarsongGulch::HORDE_TUNNEL_Z, 0.0f);
        positions.emplace_back(WarsongGulch::HORDE_TUNNEL_X - 10.0f,
            WarsongGulch::HORDE_TUNNEL_Y - 10.0f, WarsongGulch::HORDE_TUNNEL_Z, 0.0f);
    }

    return positions;
}

// ============================================================================
// RUNTIME BEHAVIOR - DYNAMIC BEHAVIOR TREE
// ============================================================================

bool WarsongGulchScript::ExecuteStrategy(::Player* player)
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
        TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 1: carrying flag, running home",
            player->GetName());
        RunFlagHome(player);
        return true;
    }

    // =========================================================================
    // PRIORITY 2: Dropped friendly flag nearby -> return it
    // =========================================================================
    // Check if there's a dropped flag within 50yd that we should return
    if (ReturnDroppedFlag(player))
    {
        TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 2: returning dropped flag",
            player->GetName());
        return true;
    }

    // =========================================================================
    // PRIORITY 3: No flags in play -> opening split (30% attack / 40% defend / 30% mid)
    // =========================================================================
    if (!m_cachedFriendlyFC && !m_cachedEnemyFC)
    {
        uint32 dutySlot = player->GetGUID().GetCounter() % 10;
        if (dutySlot < 3) // 30% go pickup enemy flag
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 3: going to pick up enemy flag",
                player->GetName());
            PickupEnemyFlag(player);
            return true;
        }
        else if (dutySlot < 7) // 40% defend flag room
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 3: defending flag room",
                player->GetName());
            DefendOwnFlagRoom(player);
            return true;
        }
        else // 30% hold mid-field (chokepoints)
        {
            auto chokepoints = GetMiddleChokepoints();
            if (!chokepoints.empty())
            {
                uint32 cpIdx = player->GetGUID().GetCounter() % chokepoints.size();
                Playerbot::BotMovementUtil::MoveToPosition(player, chokepoints[cpIdx]);
                // Attack enemies at mid
                ::Player* nearEnemy = FindNearestEnemyPlayer(player, 20.0f);
                if (nearEnemy)
                    EngageTarget(player, nearEnemy);
            }
            else
            {
                DefendOwnFlagRoom(player);
            }
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 3: holding mid-field",
                player->GetName());
            return true;
        }
    }

    // =========================================================================
    // PRIORITY 4: Both FCs exist -> 20% escort / 40% defend / 40% hunt
    // =========================================================================
    if (m_cachedFriendlyFC && m_cachedEnemyFC)
    {
        uint32 dutySlot = player->GetGUID().GetCounter() % 5;
        if (dutySlot == 0) // 20% escort friendly FC
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 4: escorting friendly FC {}",
                player->GetName(), m_cachedFriendlyFC->GetName());
            EscortFriendlyFC(player, m_cachedFriendlyFC);
        }
        else if (dutySlot < 3) // 40% defend base
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 4: defending flag room during standoff",
                player->GetName());
            DefendOwnFlagRoom(player);
        }
        else // 40% hunt enemy FC
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 4: hunting enemy FC {}",
                player->GetName(), m_cachedEnemyFC->GetName());
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
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 5: escorting friendly FC {}",
                player->GetName(), m_cachedFriendlyFC->GetName());
            EscortFriendlyFC(player, m_cachedFriendlyFC);
        }
        else // 40% defend base
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 5: defending flag room while FC runs",
                player->GetName());
            DefendOwnFlagRoom(player);
        }
        return true;
    }

    // =========================================================================
    // PRIORITY 6: Only enemy FC exists -> 60% hunt / 40% defend
    // =========================================================================
    if (m_cachedEnemyFC)
    {
        uint32 dutySlot = player->GetGUID().GetCounter() % 5;
        if (dutySlot < 3) // 60% hunt
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 6: hunting enemy FC {}",
                player->GetName(), m_cachedEnemyFC->GetName());
            HuntEnemyFC(player, m_cachedEnemyFC);
        }
        else // 40% defend
        {
            TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 6: defending while hunting enemy FC",
                player->GetName());
            DefendOwnFlagRoom(player);
        }
        return true;
    }

    // =========================================================================
    // PRIORITY 7: Fallback - shouldn't normally reach here
    // =========================================================================
    TC_LOG_DEBUG("playerbots.bg.script", "[WSG] {} PRIORITY 7: idle fallback, defending",
        player->GetName());
    DefendOwnFlagRoom(player);
    return true;
}

} // namespace Playerbot::Coordination::Battleground
