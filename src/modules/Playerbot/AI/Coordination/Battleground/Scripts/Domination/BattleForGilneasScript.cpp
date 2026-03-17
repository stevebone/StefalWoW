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

#include "BattleForGilneasScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot::Coordination::Battleground
{

// Register the script
REGISTER_BG_SCRIPT(BattleForGilneasScript, 761);  // BattleForGilneas::MAP_ID

// ============================================================================
// LIFECYCLE
// ============================================================================

void BattleForGilneasScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    DominationScriptBase::OnLoad(coordinator);
    InitializeNodeTracking();

    // Cache objective data
    m_cachedObjectives = GetObjectiveData();

    // Register world state mappings for scores
    RegisterScoreWorldState(BattleForGilneas::WorldStates::RESOURCES_ALLY, true);
    RegisterScoreWorldState(BattleForGilneas::WorldStates::RESOURCES_HORDE, false);

    // Register node world states
    // Lighthouse
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::LIGHTHOUSE_ALLIANCE_CONTROLLED,
        BattleForGilneas::Nodes::LIGHTHOUSE, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::LIGHTHOUSE_HORDE_CONTROLLED,
        BattleForGilneas::Nodes::LIGHTHOUSE, BGObjectiveState::HORDE_CONTROLLED);
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::LIGHTHOUSE_ALLIANCE,
        BattleForGilneas::Nodes::LIGHTHOUSE, BGObjectiveState::ALLIANCE_CONTESTED);
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::LIGHTHOUSE_HORDE,
        BattleForGilneas::Nodes::LIGHTHOUSE, BGObjectiveState::HORDE_CONTESTED);

    // Waterworks
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::WATERWORKS_ALLIANCE_CONTROLLED,
        BattleForGilneas::Nodes::WATERWORKS, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::WATERWORKS_HORDE_CONTROLLED,
        BattleForGilneas::Nodes::WATERWORKS, BGObjectiveState::HORDE_CONTROLLED);
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::WATERWORKS_ALLIANCE,
        BattleForGilneas::Nodes::WATERWORKS, BGObjectiveState::ALLIANCE_CONTESTED);
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::WATERWORKS_HORDE,
        BattleForGilneas::Nodes::WATERWORKS, BGObjectiveState::HORDE_CONTESTED);

    // Mines
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::MINES_ALLIANCE_CONTROLLED,
        BattleForGilneas::Nodes::MINES, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::MINES_HORDE_CONTROLLED,
        BattleForGilneas::Nodes::MINES, BGObjectiveState::HORDE_CONTROLLED);
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::MINES_ALLIANCE,
        BattleForGilneas::Nodes::MINES, BGObjectiveState::ALLIANCE_CONTESTED);
    RegisterWorldStateMapping(BattleForGilneas::WorldStates::MINES_HORDE,
        BattleForGilneas::Nodes::MINES, BGObjectiveState::HORDE_CONTESTED);

    TC_LOG_DEBUG("playerbots.bg.script",
        "BattleForGilneasScript: Loaded with {} nodes, optimal 2-cap strategy",
        BattleForGilneas::NODE_COUNT);
}

void BattleForGilneasScript::OnMatchStart()
{
    DominationScriptBase::OnMatchStart();

    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    std::vector<uint32> targets = Get2CapStrategy(faction);

    TC_LOG_INFO("playerbots.bg.script",
        "BFG: Match started! Strategy: Rush {} then contest Waterworks",
        faction == ALLIANCE ? "Lighthouse" : "Mines");
}

void BattleForGilneasScript::OnMatchEnd(bool victory)
{
    DominationScriptBase::OnMatchEnd(victory);

    TC_LOG_INFO("playerbots.bg.script",
        "BFG: Match ended - {}! Final node control tracked.",
        victory ? "Victory" : "Defeat");
}

// ============================================================================
// RUNTIME BEHAVIOR - Dynamic Behavior Tree
// ============================================================================

bool BattleForGilneasScript::ExecuteStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    // Check pending GO interaction — hold position if waiting for deferred Use()
    if (CheckPendingInteraction(player))
        return true;

    // Check defense commitment — bot stays at captured node for the hold timer
    if (CheckDefenseCommitment(player))
        return true;

    // Refresh node ownership state (throttled to 1s)
    RefreshNodeState();

    uint32 faction = player->GetBGTeam();
    uint32 friendlyCount = GetFriendlyNodeCount(player);
    GamePhase phase = GetCurrentPhase();

    // =========================================================================
    // PRIORITY 0: Nearby contested friendly node needs reinforcement
    // =========================================================================
    uint32 reinforceNode = CheckReinforcementNeeded(player, 60.0f);
    if (reinforceNode != UINT32_MAX)
    {
        BGObjectiveData nodeData = GetNodeData(reinforceNode);
        TC_LOG_DEBUG("playerbots.bg.script",
            "[BFG] {} PRIORITY 0: reinforcing contested node {}",
            player->GetName(), nodeData.name);
        DefendNode(player, reinforceNode);
        return true;
    }

    // =========================================================================
    // PRIORITY 1: Uncontrolled node within 30yd -> capture it immediately
    // =========================================================================
    uint32 nearCapture = FindNearestCapturableNode(player);
    if (nearCapture != UINT32_MAX)
    {
        BGObjectiveData nodeData = GetNodeData(nearCapture);
        Position nodePos(nodeData.x, nodeData.y, nodeData.z, nodeData.orientation);
        float dist = player->GetExactDist(&nodePos);

        if (dist < 30.0f)
        {
            TC_LOG_DEBUG("playerbots.bg.script",
                "[BFG] {} PRIORITY 1: capturing nearby node {} (dist={:.0f})",
                player->GetName(), nodeData.name, dist);
            CaptureNode(player, nearCapture);
            return true;
        }
    }

    // =========================================================================
    // PRIORITY 2: Friendly node CONTESTED -> rush to defend
    // =========================================================================
    uint32 threatened = FindNearestThreatenedNode(player);
    if (threatened != UINT32_MAX)
    {
        BGObjectiveData nodeData = GetNodeData(threatened);
        TC_LOG_DEBUG("playerbots.bg.script",
            "[BFG] {} PRIORITY 2: defending contested node {}",
            player->GetName(), nodeData.name);
        DefendNode(player, threatened);
        return true;
    }

    // =========================================================================
    // PRIORITY 3: Phase-aware 2-cap strategy
    // =========================================================================
    std::vector<uint32> strategy2Cap = Get2CapStrategy(faction);
    std::vector<uint32> friendlyNodes = GetFriendlyNodes(player);

    // GUID-based duty split across 10 slots
    uint32 dutySlot = player->GetGUID().GetCounter() % 10;

    switch (phase)
    {
        case GamePhase::OPENING:
        {
            // Opening: 80% rush to capture strategy nodes, 20% roam
            if (dutySlot < 8)
            {
                // Find uncaptured strategy node
                uint32 targetNode = UINT32_MAX;
                for (uint32 nodeId : strategy2Cap)
                {
                    auto it = m_nodeStates.find(nodeId);
                    if (it != m_nodeStates.end() && !IsNodeFriendly(it->second, faction))
                    {
                        targetNode = nodeId;
                        break;
                    }
                }

                if (targetNode != UINT32_MAX)
                {
                    BGObjectiveData nodeData = GetNodeData(targetNode);
                    TC_LOG_DEBUG("playerbots.bg.script",
                        "[BFG] {} PRIORITY 3 (OPENING): rushing to capture {}",
                        player->GetName(), nodeData.name);
                    CaptureNode(player, targetNode);
                    return true;
                }
            }
            // 20% or no target: attack enemy nodes
            {
                uint32 targetNode = DominationScriptBase::GetBestAssaultTarget(player);
                if (targetNode != UINT32_MAX)
                {
                    CaptureNode(player, targetNode);
                    return true;
                }
            }
            break;
        }

        case GamePhase::DESPERATE:
        {
            // Desperate: 90% all-in on Waterworks, 10% defend what we have
            if (dutySlot < 9)
            {
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[BFG] {} PRIORITY 3 (DESPERATE): all-in on Waterworks",
                    player->GetName());
                CaptureNode(player, BattleForGilneas::Nodes::WATERWORKS);
                return true;
            }
            else if (!friendlyNodes.empty())
            {
                DefendNode(player, friendlyNodes[0]);
                return true;
            }
            break;
        }

        default: // MID_GAME, LATE_GAME
        {
            bool needMoreNodes = friendlyCount < 2;

            if (needMoreNodes)
            {
                // Under 2-cap: 60% attack, 40% defend existing
                if (dutySlot < 6)
                {
                    uint32 targetNode = UINT32_MAX;
                    for (uint32 nodeId : strategy2Cap)
                    {
                        auto it = m_nodeStates.find(nodeId);
                        if (it != m_nodeStates.end() && !IsNodeFriendly(it->second, faction))
                        {
                            targetNode = nodeId;
                            break;
                        }
                    }

                    if (targetNode == UINT32_MAX)
                        targetNode = DominationScriptBase::GetBestAssaultTarget(player);

                    if (targetNode != UINT32_MAX)
                    {
                        BGObjectiveData nodeData = GetNodeData(targetNode);
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[BFG] {} PRIORITY 3: attacking node {} (need 2-cap)",
                            player->GetName(), nodeData.name);
                        CaptureNode(player, targetNode);
                        return true;
                    }
                }
                else if (!friendlyNodes.empty())
                {
                    uint32 defIdx = dutySlot % friendlyNodes.size();
                    DefendNode(player, friendlyNodes[defIdx]);
                    return true;
                }
            }
            else
            {
                // At or above 2-cap: phase-aware defense ratio
                uint32 defenseSlots = (phase == GamePhase::LATE_GAME) ? 8 : 7; // 80% or 70% defend

                if (dutySlot < defenseSlots)
                {
                    if (!friendlyNodes.empty())
                    {
                        // Extra weight on Waterworks
                        uint32 defNode;
                        bool wwControlled = false;
                        for (uint32 n : friendlyNodes)
                        {
                            if (n == BattleForGilneas::Nodes::WATERWORKS)
                            {
                                wwControlled = true;
                                break;
                            }
                        }

                        // Give 3/7 or 3/8 defenders to Waterworks
                        if (wwControlled && dutySlot < 3)
                        {
                            defNode = BattleForGilneas::Nodes::WATERWORKS;
                        }
                        else
                        {
                            uint32 defIdx = dutySlot % friendlyNodes.size();
                            defNode = friendlyNodes[defIdx];
                        }

                        BGObjectiveData nodeData = GetNodeData(defNode);
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[BFG] {} PRIORITY 3: defending node {} (2-cap hold)",
                            player->GetName(), nodeData.name);
                        DefendNode(player, defNode);
                        return true;
                    }
                }
                else
                {
                    // Push for 3rd node
                    uint32 targetNode = DominationScriptBase::GetBestAssaultTarget(player);
                    if (targetNode != UINT32_MAX)
                    {
                        BGObjectiveData nodeData = GetNodeData(targetNode);
                        TC_LOG_DEBUG("playerbots.bg.script",
                            "[BFG] {} PRIORITY 3: pushing enemy node {} (opportunistic)",
                            player->GetName(), nodeData.name);
                        CaptureNode(player, targetNode);
                        return true;
                    }
                }
            }
            break;
        }
    }

    // =========================================================================
    // PRIORITY 4: No clear objective -> patrol nearest chokepoint
    // =========================================================================
    auto chokepoints = GetChokepoints();
    if (!chokepoints.empty())
    {
        uint32 idx = player->GetGUID().GetCounter() % chokepoints.size();
        TC_LOG_DEBUG("playerbots.bg.script",
            "[BFG] {} PRIORITY 4: patrolling chokepoint", player->GetName());
        PatrolAroundPosition(player, chokepoints[idx], 5.0f, 15.0f);
        return true;
    }

    return true;
}

// ============================================================================
// DATA PROVIDERS
// ============================================================================

std::vector<BGObjectiveData> BattleForGilneasScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    for (uint32 i = 0; i < BattleForGilneas::NODE_COUNT; ++i)
    {
        BGObjectiveData node = GetNodeData(i);
        objectives.push_back(node);
    }

    return objectives;
}

BGObjectiveData BattleForGilneasScript::GetNodeData(uint32 nodeIndex) const
{
    BGObjectiveData node;
    Position pos = BattleForGilneas::GetNodePosition(nodeIndex);

    node.id = nodeIndex;
    node.type = ObjectiveType::NODE;
    node.name = BattleForGilneas::GetNodeName(nodeIndex);
    node.x = pos.GetPositionX();
    node.y = pos.GetPositionY();
    node.z = pos.GetPositionZ();
    node.orientation = pos.GetOrientation();
    node.strategicValue = BattleForGilneas::GetNodeStrategicValue(nodeIndex);
    node.captureTime = BattleForGilneas::CAPTURE_TIME;

    // Set game object entries
    switch (nodeIndex)
    {
        case BattleForGilneas::Nodes::LIGHTHOUSE:
            node.gameObjectEntry = BattleForGilneas::GameObjects::LIGHTHOUSE_BANNER;
            break;
        case BattleForGilneas::Nodes::WATERWORKS:
            node.gameObjectEntry = BattleForGilneas::GameObjects::WATERWORKS_BANNER;
            break;
        case BattleForGilneas::Nodes::MINES:
            node.gameObjectEntry = BattleForGilneas::GameObjects::MINES_BANNER;
            break;
    }

    // Set connected nodes
    node.connectedObjectives = BattleForGilneas::GetAdjacentNodes(nodeIndex);

    // Calculate distances from spawns
    node.distanceFromAllianceSpawn = GetDistanceFromSpawn(nodeIndex, ALLIANCE);
    node.distanceFromHordeSpawn = GetDistanceFromSpawn(nodeIndex, HORDE);

    return node;
}

std::vector<BGPositionData> BattleForGilneasScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;

    if (faction == ALLIANCE)
    {
        for (const auto& pos : BattleForGilneas::ALLIANCE_SPAWNS)
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
        for (const auto& pos : BattleForGilneas::HORDE_SPAWNS)
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

std::vector<BGPositionData> BattleForGilneasScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Add defense positions for each node
    for (uint32 i = 0; i < BattleForGilneas::NODE_COUNT; ++i)
    {
        auto defensePositions = BattleForGilneas::GetNodeDefensePositions(i);
        for (const auto& pos : defensePositions)
        {
            BGPositionData p(BattleForGilneas::GetNodeName(i),
                pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
                pos.GetOrientation(),
                BGPositionData::PositionType::DEFENSIVE_POSITION,
                0, BattleForGilneas::GetNodeStrategicValue(i));
            positions.push_back(p);
        }
    }

    // Add all chokepoints from enterprise data
    auto chokepoints = BattleForGilneas::GetChokepoints();
    for (size_t i = 0; i < chokepoints.size(); ++i)
    {
        const auto& pos = chokepoints[i];
        std::string name = "Chokepoint " + std::to_string(i + 1);
        positions.push_back(BGPositionData(name, pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, 7));
    }

    // Add all sniper/overlook positions
    auto sniperPositions = BattleForGilneas::GetSniperPositions();
    for (size_t i = 0; i < sniperPositions.size(); ++i)
    {
        const auto& pos = sniperPositions[i];
        std::string name = "Sniper Position " + std::to_string(i + 1);
        positions.push_back(BGPositionData(name, pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SNIPER_POSITION, 0, 8));
    }

    // Add buff positions
    auto buffPositions = BattleForGilneas::GetBuffPositions();
    for (size_t i = 0; i < buffPositions.size(); ++i)
    {
        const auto& pos = buffPositions[i];
        std::string name = "Restoration Buff " + std::to_string(i + 1);
        positions.push_back(BGPositionData(name, pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::BUFF_LOCATION, 0, 5));
    }

    return positions;
}

std::vector<BGPositionData> BattleForGilneasScript::GetGraveyardPositions(uint32 faction) const
{
    std::vector<BGPositionData> graveyards;

    // In BFG, graveyards are at controlled nodes
    for (uint32 i = 0; i < BattleForGilneas::NODE_COUNT; ++i)
    {
        Position gy = BattleForGilneas::GetNodeGraveyard(i);
        if (gy.GetPositionX() != 0)  // Valid position
        {
            std::string name = std::string(BattleForGilneas::GetNodeName(i)) + " Graveyard";
            BGPositionData p(name, gy.GetPositionX(), gy.GetPositionY(), gy.GetPositionZ(),
                gy.GetOrientation(), BGPositionData::PositionType::GRAVEYARD, 0, 6);
            graveyards.push_back(p);
        }
    }

    return graveyards;
}

std::vector<BGWorldState> BattleForGilneasScript::GetInitialWorldStates() const
{
    std::vector<BGWorldState> states;

    // Resource scores
    states.push_back(BGWorldState(BattleForGilneas::WorldStates::RESOURCES_ALLY,
        "Alliance Resources", BGWorldState::StateType::SCORE_ALLIANCE, 0));
    states.push_back(BGWorldState(BattleForGilneas::WorldStates::RESOURCES_HORDE,
        "Horde Resources", BGWorldState::StateType::SCORE_HORDE, 0));

    // Max score
    states.push_back(BGWorldState(BattleForGilneas::WorldStates::MAX_RESOURCES,
        "Max Resources", BGWorldState::StateType::CUSTOM, BattleForGilneas::MAX_SCORE));

    // Node states (neutral initially)
    states.push_back(BGWorldState(BattleForGilneas::WorldStates::LIGHTHOUSE_ALLIANCE_CONTROLLED,
        "Lighthouse", BGWorldState::StateType::OBJECTIVE_STATE, 0));
    states.push_back(BGWorldState(BattleForGilneas::WorldStates::WATERWORKS_ALLIANCE_CONTROLLED,
        "Waterworks", BGWorldState::StateType::OBJECTIVE_STATE, 0));
    states.push_back(BGWorldState(BattleForGilneas::WorldStates::MINES_ALLIANCE_CONTROLLED,
        "Mines", BGWorldState::StateType::OBJECTIVE_STATE, 0));

    return states;
}

std::vector<uint32> BattleForGilneasScript::GetTickPointsTable() const
{
    return std::vector<uint32>(
        BattleForGilneas::TICK_POINTS.begin(),
        BattleForGilneas::TICK_POINTS.end()
    );
}

// ============================================================================
// WORLD STATE INTERPRETATION
// ============================================================================

bool BattleForGilneasScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    // Try cached mappings
    if (TryInterpretFromCache(stateId, value, outObjectiveId, outState))
        return true;

    // Manual interpretation for complex states
    // Lighthouse
    if (stateId == BattleForGilneas::WorldStates::LIGHTHOUSE_ALLIANCE_CONTROLLED && value)
    {
        outObjectiveId = BattleForGilneas::Nodes::LIGHTHOUSE;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == BattleForGilneas::WorldStates::LIGHTHOUSE_HORDE_CONTROLLED && value)
    {
        outObjectiveId = BattleForGilneas::Nodes::LIGHTHOUSE;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }

    // Waterworks
    if (stateId == BattleForGilneas::WorldStates::WATERWORKS_ALLIANCE_CONTROLLED && value)
    {
        outObjectiveId = BattleForGilneas::Nodes::WATERWORKS;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == BattleForGilneas::WorldStates::WATERWORKS_HORDE_CONTROLLED && value)
    {
        outObjectiveId = BattleForGilneas::Nodes::WATERWORKS;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }

    // Mines
    if (stateId == BattleForGilneas::WorldStates::MINES_ALLIANCE_CONTROLLED && value)
    {
        outObjectiveId = BattleForGilneas::Nodes::MINES;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == BattleForGilneas::WorldStates::MINES_HORDE_CONTROLLED && value)
    {
        outObjectiveId = BattleForGilneas::Nodes::MINES;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }

    return false;
}

void BattleForGilneasScript::GetScoreFromWorldStates(const std::map<int32, int32>& states,
    uint32& allianceScore, uint32& hordeScore) const
{
    allianceScore = 0;
    hordeScore = 0;

    auto allyIt = states.find(BattleForGilneas::WorldStates::RESOURCES_ALLY);
    if (allyIt != states.end())
        allianceScore = static_cast<uint32>(std::max(0, allyIt->second));

    auto hordeIt = states.find(BattleForGilneas::WorldStates::RESOURCES_HORDE);
    if (hordeIt != states.end())
        hordeScore = static_cast<uint32>(std::max(0, hordeIt->second));
}

// ============================================================================
// STRATEGY - BFG SPECIFIC
// ============================================================================

BattleForGilneasScript::GamePhase BattleForGilneasScript::GetCurrentPhase() const
{
    uint32 elapsed = GetElapsedTime();

    // Check score advantage for desperate phase
    uint32 allyScore = m_allianceScore;
    uint32 hordeScore = m_hordeScore;

    uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
    uint32 ourScore = (faction == ALLIANCE) ? allyScore : hordeScore;
    uint32 theirScore = (faction == ALLIANCE) ? hordeScore : allyScore;

    // Phase hysteresis: use different thresholds for entering vs exiting DESPERATE
    // Enter DESPERATE at full threshold. Exit DESPERATE only when gap closes to half threshold.
    // This prevents rapid phase flipping when score oscillates near the threshold.
    constexpr uint32 ENTER_THRESHOLD = BattleForGilneas::Strategy::DESPERATION_THRESHOLD;
    constexpr uint32 EXIT_THRESHOLD = BattleForGilneas::Strategy::DESPERATION_THRESHOLD / 2;

    if (elapsed > BattleForGilneas::Strategy::MID_GAME_START)
    {
        if (m_lastPhase == GamePhase::DESPERATE)
        {
            // Already desperate — only exit if gap narrows to EXIT_THRESHOLD
            if (ourScore + EXIT_THRESHOLD < theirScore)
            {
                m_lastPhase = GamePhase::DESPERATE;
                return GamePhase::DESPERATE;
            }
            // Gap narrowed enough to exit desperate
        }
        else
        {
            // Not desperate — enter if gap widens to ENTER_THRESHOLD
            if (ourScore + ENTER_THRESHOLD < theirScore)
            {
                m_lastPhase = GamePhase::DESPERATE;
                return GamePhase::DESPERATE;
            }
        }
    }

    GamePhase phase;
    if (elapsed < BattleForGilneas::Strategy::OPENING_PHASE_DURATION)
        phase = GamePhase::OPENING;
    else if (elapsed < BattleForGilneas::Strategy::LATE_GAME_START)
        phase = GamePhase::MID_GAME;
    else
        phase = GamePhase::LATE_GAME;

    m_lastPhase = phase;
    return phase;
}

void BattleForGilneasScript::ApplyPhaseStrategy(StrategicDecision& decision, GamePhase phase, float scoreAdvantage) const
{
    switch (phase)
    {
        case GamePhase::OPENING:
            decision.reasoning = "Opening phase - rush home base then contest Waterworks";
            decision.offenseAllocation = 80;
            decision.defenseAllocation = 20;
            break;

        case GamePhase::MID_GAME:
            if (scoreAdvantage > 0.15f)
            {
                decision.reasoning = "Mid-game (leading) - defend 2-cap and tick to victory";
                decision.strategy = BGStrategy::DEFENSIVE;
                decision.offenseAllocation = 30;
                decision.defenseAllocation = 70;
            }
            else if (scoreAdvantage < -0.15f)
            {
                decision.reasoning = "Mid-game (behind) - push to contest enemy nodes";
                decision.strategy = BGStrategy::AGGRESSIVE;
                decision.offenseAllocation = 65;
                decision.defenseAllocation = 35;
            }
            else
            {
                decision.reasoning = "Mid-game (even) - balanced 2-cap strategy";
                decision.strategy = BGStrategy::BALANCED;
                decision.offenseAllocation = 50;
                decision.defenseAllocation = 50;
            }
            break;

        case GamePhase::LATE_GAME:
            if (scoreAdvantage > 0.1f)
            {
                decision.reasoning = "Late game (winning) - turtle and defend";
                decision.strategy = BGStrategy::DEFENSIVE;
                decision.offenseAllocation = 20;
                decision.defenseAllocation = 80;
            }
            else
            {
                decision.reasoning = "Late game (close/behind) - aggressive push needed";
                decision.strategy = BGStrategy::AGGRESSIVE;
                decision.offenseAllocation = 70;
                decision.defenseAllocation = 30;
            }
            break;

        case GamePhase::DESPERATE:
            decision.reasoning = "DESPERATE - All in on Waterworks!";
            decision.strategy = BGStrategy::ALL_IN;
            decision.offenseAllocation = 90;
            decision.defenseAllocation = 10;
            decision.attackObjectives.clear();
            decision.attackObjectives.push_back(BattleForGilneas::Nodes::WATERWORKS);
            break;
    }
}

void BattleForGilneasScript::AdjustStrategy(StrategicDecision& decision,
    float scoreAdvantage, uint32 controlledCount,
    uint32 totalObjectives, uint32 timeRemaining) const
{
    // Get base domination strategy
    DominationScriptBase::AdjustStrategy(decision, scoreAdvantage,
        controlledCount, totalObjectives, timeRemaining);

    // Apply phase-specific strategy
    GamePhase phase = GetCurrentPhase();
    ApplyPhaseStrategy(decision, phase, scoreAdvantage);

    // BFG-specific: Waterworks priority
    if (IsWaterworksCritical())
    {
        // If we don't have WW, prioritize it
        auto wwIt = m_nodeStates.find(BattleForGilneas::Nodes::WATERWORKS);
        if (wwIt != m_nodeStates.end())
        {
            uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
            bool weControlWW =
                (faction == ALLIANCE && wwIt->second == BGObjectiveState::ALLIANCE_CONTROLLED) ||
                (faction == HORDE && wwIt->second == BGObjectiveState::HORDE_CONTROLLED);

            if (!weControlWW && phase != GamePhase::OPENING)
            {
                // Insert WW at front of attack priorities
                auto it = std::find(decision.attackObjectives.begin(),
                    decision.attackObjectives.end(), BattleForGilneas::Nodes::WATERWORKS);
                if (it != decision.attackObjectives.end())
                    decision.attackObjectives.erase(it);
                decision.attackObjectives.insert(decision.attackObjectives.begin(),
                    BattleForGilneas::Nodes::WATERWORKS);

                decision.reasoning += " (Waterworks is critical)";
            }
        }
    }

    // 2-cap strategy: if we have 2 nodes, defend them
    if (controlledCount == 2)
    {
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.reasoning = "2-cap achieved - defend and tick to victory";
        decision.defenseAllocation = 70;
        decision.offenseAllocation = 30;
    }

    // 3-cap: if we have all 3, super aggressive to end quickly
    if (controlledCount == 3)
    {
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.reasoning = "3-cap achieved - push to end quickly (10 pts/tick)";
        decision.offenseAllocation = 60;
        decision.defenseAllocation = 40;
    }
}

uint8 BattleForGilneasScript::GetObjectiveAttackPriority(uint32 objectiveId,
    BGObjectiveState state, uint32 faction) const
{
    uint8 basePriority = DominationScriptBase::GetObjectiveAttackPriority(
        objectiveId, state, faction);

    // Waterworks is always high priority
    if (objectiveId == BattleForGilneas::Nodes::WATERWORKS)
        return std::min(static_cast<uint8>(10), static_cast<uint8>(basePriority + 2));

    // Home bases higher priority at game start
    if (GetCurrentPhase() == GamePhase::OPENING)
    {
        if (faction == ALLIANCE && objectiveId == BattleForGilneas::Nodes::LIGHTHOUSE)
            return std::min(static_cast<uint8>(10), static_cast<uint8>(basePriority + 1));

        if (faction == HORDE && objectiveId == BattleForGilneas::Nodes::MINES)
            return std::min(static_cast<uint8>(10), static_cast<uint8>(basePriority + 1));
    }

    return basePriority;
}

uint8 BattleForGilneasScript::GetObjectiveDefensePriority(uint32 objectiveId,
    BGObjectiveState state, uint32 faction) const
{
    uint8 basePriority = DominationScriptBase::GetObjectiveDefensePriority(
        objectiveId, state, faction);

    // Waterworks defense is critical if controlled
    if (objectiveId == BattleForGilneas::Nodes::WATERWORKS &&
        ((faction == ALLIANCE && state == BGObjectiveState::ALLIANCE_CONTROLLED) ||
         (faction == HORDE && state == BGObjectiveState::HORDE_CONTROLLED)))
    {
        return std::min(static_cast<uint8>(10), static_cast<uint8>(basePriority + 2));
    }

    return basePriority;
}

RoleDistribution BattleForGilneasScript::GetRecommendedRoles(const StrategicDecision& decision,
    float scoreAdvantage, uint32 timeRemaining) const
{
    RoleDistribution dist;

    GamePhase phase = GetCurrentPhase();

    switch (decision.strategy)
    {
        case BGStrategy::AGGRESSIVE:
            dist.roleCounts[BGRole::NODE_ATTACKER] = 50;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 30;
            dist.roleCounts[BGRole::ROAMER] = 20;
            dist.reasoning = "Aggressive node capture";
            break;

        case BGStrategy::DEFENSIVE:
            dist.roleCounts[BGRole::NODE_DEFENDER] = 60;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 25;
            dist.roleCounts[BGRole::ROAMER] = 15;
            dist.reasoning = "Defensive 2-cap hold";
            break;

        case BGStrategy::ALL_IN:
            dist.roleCounts[BGRole::NODE_ATTACKER] = 80;
            dist.roleCounts[BGRole::ROAMER] = 20;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 0;
            dist.reasoning = "Desperate all-in attack";
            break;

        default:  // BALANCED
            dist.roleCounts[BGRole::NODE_ATTACKER] = 35;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 40;
            dist.roleCounts[BGRole::ROAMER] = 25;
            dist.reasoning = "Balanced 2-cap strategy";
            break;
    }

    // Adjust for phase
    if (phase == GamePhase::OPENING)
    {
        dist.reasoning = "Opening rush - capture home base";
        dist.roleCounts[BGRole::NODE_ATTACKER] = 70;
        dist.roleCounts[BGRole::NODE_DEFENDER] = 10;
        dist.roleCounts[BGRole::ROAMER] = 20;
    }

    return dist;
}

// ============================================================================
// BFG-SPECIFIC HELPERS
// ============================================================================

std::vector<uint32> BattleForGilneasScript::Get2CapStrategy(uint32 faction) const
{
    if (faction == ALLIANCE)
    {
        // Alliance typically takes Lighthouse + Waterworks
        return {
            BattleForGilneas::Nodes::LIGHTHOUSE,
            BattleForGilneas::Nodes::WATERWORKS
        };
    }
    else
    {
        // Horde typically takes Mines + Waterworks
        return {
            BattleForGilneas::Nodes::MINES,
            BattleForGilneas::Nodes::WATERWORKS
        };
    }
}

bool BattleForGilneasScript::IsWaterworksCritical() const
{
    // Waterworks is always critical in BFG
    // It controls map center and is the most contested node
    return true;
}

uint32 BattleForGilneasScript::GetOpeningRushTarget(uint32 faction) const
{
    // First rush: closest home base, then WW
    if (faction == ALLIANCE)
        return BattleForGilneas::Nodes::LIGHTHOUSE;
    else
        return BattleForGilneas::Nodes::MINES;
}

float BattleForGilneasScript::GetDistanceFromSpawn(uint32 nodeId, uint32 faction) const
{
    Position nodePos = BattleForGilneas::GetNodePosition(nodeId);
    Position spawnPos;

    if (faction == ALLIANCE)
        spawnPos = BattleForGilneas::ALLIANCE_SPAWNS[0];
    else
        spawnPos = BattleForGilneas::HORDE_SPAWNS[0];

    return CalculateDistance(spawnPos.GetPositionX(), spawnPos.GetPositionY(), spawnPos.GetPositionZ(),
        nodePos.GetPositionX(), nodePos.GetPositionY(), nodePos.GetPositionZ());
}

// ============================================================================
// ENTERPRISE-GRADE ROUTING AND POSITIONING
// ============================================================================

std::vector<Position> BattleForGilneasScript::GetRotationPath(uint32 fromNode, uint32 toNode) const
{
    return BattleForGilneas::GetRotationPath(fromNode, toNode);
}

std::vector<Position> BattleForGilneasScript::GetAmbushPositions(uint32 faction) const
{
    return BattleForGilneas::GetAmbushPositions(faction);
}

float BattleForGilneasScript::GetNodeToNodeDistance(uint32 fromNode, uint32 toNode) const
{
    return BattleForGilneas::GetNodeDistance(fromNode, toNode);
}

uint32 BattleForGilneasScript::GetNearestNode(Position const& pos) const
{
    uint32 nearest = 0;
    float minDist = std::numeric_limits<float>::max();

    for (uint32 i = 0; i < BattleForGilneas::NODE_COUNT; ++i)
    {
        Position nodePos = BattleForGilneas::GetNodePosition(i);
        float dist = CalculateDistance(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
            nodePos.GetPositionX(), nodePos.GetPositionY(), nodePos.GetPositionZ());
        if (dist < minDist)
        {
            minDist = dist;
            nearest = i;
        }
    }
    return nearest;
}

uint32 BattleForGilneasScript::GetBestAssaultTarget(uint32 faction) const
{
    std::vector<uint32> contestableNodes;

    for (uint32 i = 0; i < BattleForGilneas::NODE_COUNT; ++i)
    {
        auto it = m_nodeStates.find(i);
        if (it != m_nodeStates.end())
        {
            bool enemyControlled = (faction == ALLIANCE)
                ? (it->second == BGObjectiveState::HORDE_CONTROLLED)
                : (it->second == BGObjectiveState::ALLIANCE_CONTROLLED);

            if (enemyControlled)
                contestableNodes.push_back(i);
        }
    }

    // Prioritize Waterworks if enemy has it
    for (uint32 node : contestableNodes)
    {
        if (node == BattleForGilneas::Nodes::WATERWORKS)
            return node;
    }

    // Otherwise find highest value enemy node
    uint32 bestTarget = contestableNodes.empty() ? BattleForGilneas::Nodes::WATERWORKS : contestableNodes[0];
    uint8 bestValue = 0;

    for (uint32 node : contestableNodes)
    {
        uint8 value = BattleForGilneas::GetNodeStrategicValue(node);
        if (value > bestValue)
        {
            bestValue = value;
            bestTarget = node;
        }
    }

    return bestTarget;
}

uint32 BattleForGilneasScript::GetDefensePriority(uint32 nodeId) const
{
    uint8 basePriority = BattleForGilneas::GetNodeStrategicValue(nodeId);

    // Waterworks always top priority
    if (nodeId == BattleForGilneas::Nodes::WATERWORKS)
        return basePriority + BattleForGilneas::Strategy::WW_EXTRA_DEFENDERS;

    return basePriority;
}

bool BattleForGilneasScript::ShouldRotate() const
{
    // Check if rotation interval has passed
    uint32 elapsed = GetElapsedTime();
    return (elapsed % BattleForGilneas::Strategy::ROTATION_INTERVAL) < 1000;  // Within 1 sec of rotation time
}

std::vector<Position> BattleForGilneasScript::GetChokepoints() const
{
    return BattleForGilneas::GetChokepoints();
}

std::vector<Position> BattleForGilneasScript::GetSniperPositions() const
{
    return BattleForGilneas::GetSniperPositions();
}

std::vector<Position> BattleForGilneasScript::GetBuffPositions() const
{
    return BattleForGilneas::GetBuffPositions();
}

// ============================================================================
// ENHANCED EVENT HANDLING
// ============================================================================

void BattleForGilneasScript::OnEvent(const BGScriptEventData& event)
{
    DominationScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::OBJECTIVE_CAPTURED:
            TC_LOG_DEBUG("playerbots.bg.script",
                "BFG: Node {} captured by {}! Current control: Alliance={}, Horde={}",
                event.objectiveId,
                event.faction == ALLIANCE ? "Alliance" : "Horde",
                m_allianceNodes, m_hordeNodes);

            // Check if we need to adjust defenders
            if (event.objectiveId == BattleForGilneas::Nodes::WATERWORKS)
            {
                TC_LOG_DEBUG("playerbots.bg.script",
                    "BFG: Waterworks captured - critical node! Adjusting defense priority.");
            }
            break;

        case BGScriptEvent::OBJECTIVE_CONTESTED:
            TC_LOG_DEBUG("playerbots.bg.script",
                "BFG: Node {} under attack at ({:.1f}, {:.1f})! Defenders needed!",
                event.objectiveId, event.x, event.y);
            break;

        case BGScriptEvent::OBJECTIVE_LOST:
            TC_LOG_DEBUG("playerbots.bg.script",
                "BFG: Node {} lost! Counter-attack may be needed.",
                event.objectiveId);
            break;

        default:
            break;
    }
}

} // namespace Playerbot::Coordination::Battleground
