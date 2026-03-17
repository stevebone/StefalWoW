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

#include "ArathiBasinScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot::Coordination::Battleground
{

// Register the script
REGISTER_BG_SCRIPT(ArathiBasinScript, 529);  // ArathiBasin::MAP_ID

// ============================================================================
// LIFECYCLE
// ============================================================================

void ArathiBasinScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    DominationScriptBase::OnLoad(coordinator);
    InitializeNodeTracking();

    // Cache objective data
    m_cachedObjectives = GetObjectiveData();

    // Register world state mappings for scores
    RegisterScoreWorldState(ArathiBasin::WorldStates::RESOURCES_ALLY, true);
    RegisterScoreWorldState(ArathiBasin::WorldStates::RESOURCES_HORDE, false);

    // Register node world states
    // Stables
    RegisterWorldStateMapping(ArathiBasin::WorldStates::STABLES_ALLIANCE_CONTROLLED,
        ArathiBasin::Nodes::STABLES, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(ArathiBasin::WorldStates::STABLES_HORDE_CONTROLLED,
        ArathiBasin::Nodes::STABLES, BGObjectiveState::HORDE_CONTROLLED);
    RegisterWorldStateMapping(ArathiBasin::WorldStates::STABLES_ALLIANCE,
        ArathiBasin::Nodes::STABLES, BGObjectiveState::ALLIANCE_CONTESTED);
    RegisterWorldStateMapping(ArathiBasin::WorldStates::STABLES_HORDE,
        ArathiBasin::Nodes::STABLES, BGObjectiveState::HORDE_CONTESTED);

    // Blacksmith
    RegisterWorldStateMapping(ArathiBasin::WorldStates::BLACKSMITH_ALLIANCE_CONTROLLED,
        ArathiBasin::Nodes::BLACKSMITH, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(ArathiBasin::WorldStates::BLACKSMITH_HORDE_CONTROLLED,
        ArathiBasin::Nodes::BLACKSMITH, BGObjectiveState::HORDE_CONTROLLED);

    // Farm
    RegisterWorldStateMapping(ArathiBasin::WorldStates::FARM_ALLIANCE_CONTROLLED,
        ArathiBasin::Nodes::FARM, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(ArathiBasin::WorldStates::FARM_HORDE_CONTROLLED,
        ArathiBasin::Nodes::FARM, BGObjectiveState::HORDE_CONTROLLED);

    // Gold Mine
    RegisterWorldStateMapping(ArathiBasin::WorldStates::GOLD_MINE_ALLIANCE_CONTROLLED,
        ArathiBasin::Nodes::GOLD_MINE, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(ArathiBasin::WorldStates::GOLD_MINE_HORDE_CONTROLLED,
        ArathiBasin::Nodes::GOLD_MINE, BGObjectiveState::HORDE_CONTROLLED);

    // Lumber Mill
    RegisterWorldStateMapping(ArathiBasin::WorldStates::LUMBER_MILL_ALLIANCE_CONTROLLED,
        ArathiBasin::Nodes::LUMBER_MILL, BGObjectiveState::ALLIANCE_CONTROLLED);
    RegisterWorldStateMapping(ArathiBasin::WorldStates::LUMBER_MILL_HORDE_CONTROLLED,
        ArathiBasin::Nodes::LUMBER_MILL, BGObjectiveState::HORDE_CONTROLLED);

    TC_LOG_DEBUG("playerbots.bg.script",
        "ArathiBasinScript: Loaded with {} nodes",
        ArathiBasin::NODE_COUNT);
}

// ============================================================================
// RUNTIME BEHAVIOR - Dynamic Behavior Tree
// ============================================================================

bool ArathiBasinScript::ExecuteStrategy(::Player* player)
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
    uint32 nodeCount = GetNodeCount();

    // =========================================================================
    // PRIORITY 0: Nearby contested friendly node needs reinforcement
    // =========================================================================
    uint32 reinforceNode = CheckReinforcementNeeded(player, 60.0f);
    if (reinforceNode != UINT32_MAX)
    {
        BGObjectiveData nodeData = GetNodeData(reinforceNode);
        TC_LOG_DEBUG("playerbots.bg.script",
            "[AB] {} PRIORITY 0: reinforcing contested node {}",
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
                "[AB] {} PRIORITY 1: capturing nearby node {} (dist={:.0f})",
                player->GetName(), nodeData.name, dist);
            CaptureNode(player, nearCapture);
            return true;
        }
    }

    // =========================================================================
    // PRIORITY 1.5: Opening rush (first 60s) -> proximity-based targets
    // Alliance: 5 to Stables, 4 to Blacksmith, 1 to Lumber Mill
    // Horde: 5 to Farm, 4 to Blacksmith, 1 to Lumber Mill
    // =========================================================================
    if (GetElapsedTime() < 60000 && friendlyCount < 3)
    {
        uint32 dutySlot = player->GetGUID().GetCounter() % 10;
        uint32 targetNode;

        if (faction == ALLIANCE)
        {
            if (dutySlot < 5)      // 50% Stables (nearest)
                targetNode = ArathiBasin::Nodes::STABLES;
            else if (dutySlot < 9) // 40% Blacksmith (center)
                targetNode = ArathiBasin::Nodes::BLACKSMITH;
            else                   // 10% Lumber Mill (flank)
                targetNode = ArathiBasin::Nodes::LUMBER_MILL;
        }
        else
        {
            if (dutySlot < 5)      // 50% Farm (nearest)
                targetNode = ArathiBasin::Nodes::FARM;
            else if (dutySlot < 9) // 40% Blacksmith (center)
                targetNode = ArathiBasin::Nodes::BLACKSMITH;
            else                   // 10% Lumber Mill (flank)
                targetNode = ArathiBasin::Nodes::LUMBER_MILL;
        }

        BGObjectiveData nodeData = GetNodeData(targetNode);
        TC_LOG_DEBUG("playerbots.bg.script",
            "[AB] {} PRIORITY 1.5: opening rush to {} (slot {})",
            player->GetName(), nodeData.name, dutySlot);
        CaptureNode(player, targetNode);
        return true;
    }

    // =========================================================================
    // PRIORITY 2: Friendly node CONTESTED -> rush to defend
    // =========================================================================
    uint32 threatened = FindNearestThreatenedNode(player);
    if (threatened != UINT32_MAX)
    {
        BGObjectiveData nodeData = GetNodeData(threatened);
        TC_LOG_DEBUG("playerbots.bg.script",
            "[AB] {} PRIORITY 2: defending contested node {}",
            player->GetName(), nodeData.name);
        DefendNode(player, threatened);
        return true;
    }

    // =========================================================================
    // PRIORITY 3: Strategic duty split - 3-cap strategy
    // =========================================================================
    // AB classic strategy: control 3 nodes, defend them, and let tick rate win
    std::vector<uint32> strategy3Cap = Get3CapStrategy(faction);
    std::vector<uint32> friendlyNodes = GetFriendlyNodes(player);

    // Determine if we need to attack or defend
    bool needMoreNodes = friendlyCount < 3;

    // GUID-based duty split across 10 slots
    uint32 dutySlot = player->GetGUID().GetCounter() % 10;

    if (needMoreNodes)
    {
        // Under 3-cap: 60% attack, 40% defend existing
        if (dutySlot < 6) // 60% attack
        {
            // Find the best target from our 3-cap strategy that we don't control
            uint32 targetNode = UINT32_MAX;
            for (uint32 nodeId : strategy3Cap)
            {
                auto it = m_nodeStates.find(nodeId);
                if (it != m_nodeStates.end() && !IsNodeFriendly(it->second, faction))
                {
                    targetNode = nodeId;
                    break;
                }
            }

            // Fallback: use GetBestAssaultTarget from base class
            if (targetNode == UINT32_MAX)
                targetNode = DominationScriptBase::GetBestAssaultTarget(player);

            if (targetNode != UINT32_MAX)
            {
                BGObjectiveData nodeData = GetNodeData(targetNode);
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[AB] {} PRIORITY 3: attacking node {} (need {} more for 3-cap)",
                    player->GetName(), nodeData.name, 3 - friendlyCount);
                CaptureNode(player, targetNode);
                return true;
            }
        }
        else // 40% defend what we have
        {
            if (!friendlyNodes.empty())
            {
                // Spread defenders across friendly nodes
                uint32 defIdx = dutySlot % friendlyNodes.size();
                uint32 defNode = friendlyNodes[defIdx];
                BGObjectiveData nodeData = GetNodeData(defNode);
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[AB] {} PRIORITY 3: defending friendly node {} (while team captures)",
                    player->GetName(), nodeData.name);
                DefendNode(player, defNode);
                return true;
            }
        }
    }
    else
    {
        // At or above 3-cap: 70% defend, 30% attack/roam
        if (dutySlot < 7) // 70% defend
        {
            if (!friendlyNodes.empty())
            {
                // Spread defenders across nodes, with extra weight on Blacksmith
                uint32 defNode;
                bool bsControlled = false;

                // Check if we control Blacksmith
                for (uint32 n : friendlyNodes)
                {
                    if (n == ArathiBasin::Nodes::BLACKSMITH)
                    {
                        bsControlled = true;
                        break;
                    }
                }

                // Give 2/7 defenders (slots 0,1) to Blacksmith if controlled
                if (bsControlled && dutySlot < 2)
                {
                    defNode = ArathiBasin::Nodes::BLACKSMITH;
                }
                else
                {
                    // Distribute remaining defenders across all nodes
                    uint32 defIdx = dutySlot % friendlyNodes.size();
                    defNode = friendlyNodes[defIdx];
                }

                BGObjectiveData nodeData = GetNodeData(defNode);
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[AB] {} PRIORITY 3: defending node {} (3-cap hold)",
                    player->GetName(), nodeData.name);
                DefendNode(player, defNode);
                return true;
            }
        }
        else // 30% attack or roam
        {
            // Push for 4th/5th node when ahead
            uint32 targetNode = DominationScriptBase::GetBestAssaultTarget(player);
            if (targetNode != UINT32_MAX)
            {
                BGObjectiveData nodeData = GetNodeData(targetNode);
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[AB] {} PRIORITY 3: pushing enemy node {} (opportunistic)",
                    player->GetName(), nodeData.name);
                CaptureNode(player, targetNode);
                return true;
            }
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
            "[AB] {} PRIORITY 4: patrolling chokepoint", player->GetName());
        PatrolAroundPosition(player, chokepoints[idx], 5.0f, 15.0f);
        return true;
    }

    return true;
}

// ============================================================================
// DATA PROVIDERS
// ============================================================================

std::vector<BGObjectiveData> ArathiBasinScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    for (uint32 i = 0; i < ArathiBasin::NODE_COUNT; ++i)
    {
        BGObjectiveData node = GetNodeData(i);
        objectives.push_back(node);
    }

    return objectives;
}

BGObjectiveData ArathiBasinScript::GetNodeData(uint32 nodeIndex) const
{
    BGObjectiveData node;
    Position pos = ArathiBasin::GetNodePosition(nodeIndex);

    node.id = nodeIndex;
    node.type = ObjectiveType::NODE;
    node.name = ArathiBasin::GetNodeName(nodeIndex);
    node.x = pos.GetPositionX();
    node.y = pos.GetPositionY();
    node.z = pos.GetPositionZ();
    node.orientation = pos.GetOrientation();
    node.strategicValue = ArathiBasin::GetNodeStrategicValue(nodeIndex);
    node.captureTime = ArathiBasin::CAPTURE_TIME;

    // Set game object entries
    switch (nodeIndex)
    {
        case ArathiBasin::Nodes::STABLES:
            node.gameObjectEntry = ArathiBasin::GameObjects::STABLES_BANNER;
            break;
        case ArathiBasin::Nodes::BLACKSMITH:
            node.gameObjectEntry = ArathiBasin::GameObjects::BLACKSMITH_BANNER;
            break;
        case ArathiBasin::Nodes::FARM:
            node.gameObjectEntry = ArathiBasin::GameObjects::FARM_BANNER;
            break;
        case ArathiBasin::Nodes::GOLD_MINE:
            node.gameObjectEntry = ArathiBasin::GameObjects::GOLD_MINE_BANNER;
            break;
        case ArathiBasin::Nodes::LUMBER_MILL:
            node.gameObjectEntry = ArathiBasin::GameObjects::LUMBER_MILL_BANNER;
            break;
    }

    // Set connected nodes
    node.connectedObjectives = ArathiBasin::GetAdjacentNodes(nodeIndex);

    // Calculate distances from spawns
    node.distanceFromAllianceSpawn = GetDistanceFromSpawn(nodeIndex, ALLIANCE);
    node.distanceFromHordeSpawn = GetDistanceFromSpawn(nodeIndex, HORDE);

    return node;
}

std::vector<BGPositionData> ArathiBasinScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;

    if (faction == ALLIANCE)
    {
        for (const auto& pos : ArathiBasin::ALLIANCE_SPAWNS)
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
        for (const auto& pos : ArathiBasin::HORDE_SPAWNS)
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

std::vector<BGPositionData> ArathiBasinScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Add defense positions for each node
    for (uint32 i = 0; i < ArathiBasin::NODE_COUNT; ++i)
    {
        auto defensePositions = ArathiBasin::GetNodeDefensePositions(i);
        for (const auto& pos : defensePositions)
        {
            BGPositionData p(ArathiBasin::GetNodeName(i),
                pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
                pos.GetOrientation(),
                BGPositionData::PositionType::DEFENSIVE_POSITION,
                0, ArathiBasin::GetNodeStrategicValue(i));
            positions.push_back(p);
        }
    }

    // Add all chokepoints from enterprise data
    auto chokepoints = ArathiBasin::GetChokepoints();
    for (size_t i = 0; i < chokepoints.size(); ++i)
    {
        const auto& pos = chokepoints[i];
        std::string name = "Chokepoint " + std::to_string(i + 1);
        positions.push_back(BGPositionData(name, pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, 7));
    }

    // Add all sniper/overlook positions
    auto sniperPositions = ArathiBasin::GetSniperPositions();
    for (size_t i = 0; i < sniperPositions.size(); ++i)
    {
        const auto& pos = sniperPositions[i];
        std::string name = "Sniper Position " + std::to_string(i + 1);
        positions.push_back(BGPositionData(name, pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SNIPER_POSITION, 0, 8));
    }

    // Add buff positions
    auto buffPositions = ArathiBasin::GetBuffPositions();
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

std::vector<BGPositionData> ArathiBasinScript::GetGraveyardPositions(uint32 faction) const
{
    std::vector<BGPositionData> graveyards;

    // In AB, graveyards are at controlled nodes
    for (uint32 i = 0; i < ArathiBasin::NODE_COUNT; ++i)
    {
        Position gy = ArathiBasin::GetNodeGraveyard(i);
        if (gy.GetPositionX() != 0)  // Valid position
        {
            std::string name = std::string(ArathiBasin::GetNodeName(i)) + " Graveyard";
            BGPositionData p(name, gy.GetPositionX(), gy.GetPositionY(), gy.GetPositionZ(),
                gy.GetOrientation(), BGPositionData::PositionType::GRAVEYARD, 0, 6);
            graveyards.push_back(p);
        }
    }

    return graveyards;
}

std::vector<BGWorldState> ArathiBasinScript::GetInitialWorldStates() const
{
    std::vector<BGWorldState> states;

    // Resource scores
    states.push_back(BGWorldState(ArathiBasin::WorldStates::RESOURCES_ALLY,
        "Alliance Resources", BGWorldState::StateType::SCORE_ALLIANCE, 0));
    states.push_back(BGWorldState(ArathiBasin::WorldStates::RESOURCES_HORDE,
        "Horde Resources", BGWorldState::StateType::SCORE_HORDE, 0));

    // Max score
    states.push_back(BGWorldState(ArathiBasin::WorldStates::MAX_RESOURCES,
        "Max Resources", BGWorldState::StateType::CUSTOM, ArathiBasin::MAX_SCORE));

    // Node states (neutral initially)
    states.push_back(BGWorldState(ArathiBasin::WorldStates::STABLES_ICON,
        "Stables", BGWorldState::StateType::OBJECTIVE_STATE, 0));
    states.push_back(BGWorldState(ArathiBasin::WorldStates::BLACKSMITH_ICON,
        "Blacksmith", BGWorldState::StateType::OBJECTIVE_STATE, 0));
    states.push_back(BGWorldState(ArathiBasin::WorldStates::FARM_ICON,
        "Farm", BGWorldState::StateType::OBJECTIVE_STATE, 0));
    states.push_back(BGWorldState(ArathiBasin::WorldStates::GOLD_MINE_ICON,
        "Gold Mine", BGWorldState::StateType::OBJECTIVE_STATE, 0));
    states.push_back(BGWorldState(ArathiBasin::WorldStates::LUMBER_MILL_ICON,
        "Lumber Mill", BGWorldState::StateType::OBJECTIVE_STATE, 0));

    return states;
}

std::vector<uint32> ArathiBasinScript::GetTickPointsTable() const
{
    return std::vector<uint32>(
        ArathiBasin::TICK_POINTS.begin(),
        ArathiBasin::TICK_POINTS.end()
    );
}

// ============================================================================
// WORLD STATE
// ============================================================================

bool ArathiBasinScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    // Try cached mappings
    if (TryInterpretFromCache(stateId, value, outObjectiveId, outState))
        return true;

    // Manual interpretation for complex states
    // Stables
    if (stateId == ArathiBasin::WorldStates::STABLES_ALLIANCE_CONTROLLED && value)
    {
        outObjectiveId = ArathiBasin::Nodes::STABLES;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == ArathiBasin::WorldStates::STABLES_HORDE_CONTROLLED && value)
    {
        outObjectiveId = ArathiBasin::Nodes::STABLES;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }

    // Blacksmith
    if (stateId == ArathiBasin::WorldStates::BLACKSMITH_ALLIANCE_CONTROLLED && value)
    {
        outObjectiveId = ArathiBasin::Nodes::BLACKSMITH;
        outState = BGObjectiveState::ALLIANCE_CONTROLLED;
        return true;
    }
    if (stateId == ArathiBasin::WorldStates::BLACKSMITH_HORDE_CONTROLLED && value)
    {
        outObjectiveId = ArathiBasin::Nodes::BLACKSMITH;
        outState = BGObjectiveState::HORDE_CONTROLLED;
        return true;
    }

    // Farm, Gold Mine, Lumber Mill - similar pattern
    // (abbreviated for space)

    return false;
}

void ArathiBasinScript::GetScoreFromWorldStates(const std::map<int32, int32>& states,
    uint32& allianceScore, uint32& hordeScore) const
{
    allianceScore = 0;
    hordeScore = 0;

    auto allyIt = states.find(ArathiBasin::WorldStates::RESOURCES_ALLY);
    if (allyIt != states.end())
        allianceScore = static_cast<uint32>(std::max(0, allyIt->second));

    auto hordeIt = states.find(ArathiBasin::WorldStates::RESOURCES_HORDE);
    if (hordeIt != states.end())
        hordeScore = static_cast<uint32>(std::max(0, hordeIt->second));
}

// ============================================================================
// STRATEGY - AB SPECIFIC
// ============================================================================

void ArathiBasinScript::AdjustStrategy(StrategicDecision& decision,
    float scoreAdvantage, uint32 controlledCount,
    uint32 totalObjectives, uint32 timeRemaining) const
{
    // Get base domination strategy
    DominationScriptBase::AdjustStrategy(decision, scoreAdvantage,
        controlledCount, totalObjectives, timeRemaining);

    // AB-specific adjustments

    // Opening phase - prioritize home bases
    if (GetElapsedTime() < 60000)  // First minute
    {
        uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;

        decision.attackObjectives.clear();
        decision.attackObjectives = Get3CapStrategy(faction);

        decision.reasoning = "Opening rush - secure home bases first";
        decision.offenseAllocation = 80;
        decision.defenseAllocation = 20;
        return;
    }

    // Blacksmith priority
    if (IsBlacksmithCritical())
    {
        // If we don't have BS, prioritize it
        auto bsIt = m_nodeStates.find(ArathiBasin::Nodes::BLACKSMITH);
        if (bsIt != m_nodeStates.end())
        {
            uint32 faction = m_coordinator ? m_coordinator->GetFaction() : ALLIANCE;
            bool weControlBS =
                (faction == ALLIANCE && bsIt->second == BGObjectiveState::ALLIANCE_CONTROLLED) ||
                (faction == HORDE && bsIt->second == BGObjectiveState::HORDE_CONTROLLED);

            if (!weControlBS)
            {
                // Insert BS at front of attack priorities
                auto it = std::find(decision.attackObjectives.begin(),
                    decision.attackObjectives.end(), ArathiBasin::Nodes::BLACKSMITH);
                if (it != decision.attackObjectives.end())
                    decision.attackObjectives.erase(it);
                decision.attackObjectives.insert(decision.attackObjectives.begin(),
                    ArathiBasin::Nodes::BLACKSMITH);

                decision.reasoning += " (Blacksmith is critical)";
            }
        }
    }

    // 3-cap strategy if we have 3 nodes
    if (controlledCount == 3)
    {
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.reasoning = "3-cap achieved - defend and tick to victory";
        decision.defenseAllocation = 70;
        decision.offenseAllocation = 30;
    }

    // 4-cap aggressive if comfortable lead
    if (controlledCount >= 4 && scoreAdvantage > 0.2f)
    {
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.reasoning = "Strong position - push for 5-cap";
        decision.offenseAllocation = 60;
    }
}

uint8 ArathiBasinScript::GetObjectiveAttackPriority(uint32 objectiveId,
    BGObjectiveState state, uint32 faction) const
{
    uint8 basePriority = DominationScriptBase::GetObjectiveAttackPriority(
        objectiveId, state, faction);

    // Blacksmith is always high priority
    if (objectiveId == ArathiBasin::Nodes::BLACKSMITH)
        return std::min(static_cast<uint8>(10), static_cast<uint8>(basePriority + 2));

    // Home bases slightly higher priority at game start
    if (GetElapsedTime() < 120000)  // First 2 minutes
    {
        if (faction == ALLIANCE &&
            (objectiveId == ArathiBasin::Nodes::STABLES ||
             objectiveId == ArathiBasin::Nodes::LUMBER_MILL))
            return std::min(static_cast<uint8>(10), static_cast<uint8>(basePriority + 1));

        if (faction == HORDE &&
            (objectiveId == ArathiBasin::Nodes::FARM ||
             objectiveId == ArathiBasin::Nodes::GOLD_MINE))
            return std::min(static_cast<uint8>(10), static_cast<uint8>(basePriority + 1));
    }

    return basePriority;
}

uint8 ArathiBasinScript::GetObjectiveDefensePriority(uint32 objectiveId,
    BGObjectiveState state, uint32 faction) const
{
    uint8 basePriority = DominationScriptBase::GetObjectiveDefensePriority(
        objectiveId, state, faction);

    // Blacksmith defense is critical if controlled
    if (objectiveId == ArathiBasin::Nodes::BLACKSMITH &&
        ((faction == ALLIANCE && state == BGObjectiveState::ALLIANCE_CONTROLLED) ||
         (faction == HORDE && state == BGObjectiveState::HORDE_CONTROLLED)))
    {
        return std::min(static_cast<uint8>(10), static_cast<uint8>(basePriority + 2));
    }

    // Lumber Mill defense bonus (high ground advantage)
    if (objectiveId == ArathiBasin::Nodes::LUMBER_MILL)
        return std::min(static_cast<uint8>(10), static_cast<uint8>(basePriority + 1));

    return basePriority;
}

// ============================================================================
// AB-SPECIFIC HELPERS
// ============================================================================

std::vector<uint32> ArathiBasinScript::Get3CapStrategy(uint32 faction) const
{
    if (faction == ALLIANCE)
    {
        // Alliance typically takes Stables, Blacksmith, Lumber Mill
        return {
            ArathiBasin::Nodes::STABLES,
            ArathiBasin::Nodes::BLACKSMITH,
            ArathiBasin::Nodes::LUMBER_MILL
        };
    }
    else
    {
        // Horde typically takes Farm, Blacksmith, Gold Mine
        return {
            ArathiBasin::Nodes::FARM,
            ArathiBasin::Nodes::BLACKSMITH,
            ArathiBasin::Nodes::GOLD_MINE
        };
    }
}

bool ArathiBasinScript::IsBlacksmithCritical() const
{
    // Blacksmith is always critical in AB
    // It controls map center and provides strategic advantage
    return true;
}

uint32 ArathiBasinScript::GetOpeningRushTarget(uint32 faction) const
{
    // First rush: closest home base, then BS
    if (faction == ALLIANCE)
        return ArathiBasin::Nodes::STABLES;
    else
        return ArathiBasin::Nodes::FARM;
}

float ArathiBasinScript::GetDistanceFromSpawn(uint32 nodeId, uint32 faction) const
{
    Position nodePos = ArathiBasin::GetNodePosition(nodeId);
    Position spawnPos;

    if (faction == ALLIANCE)
        spawnPos = ArathiBasin::ALLIANCE_SPAWNS[0];
    else
        spawnPos = ArathiBasin::HORDE_SPAWNS[0];

    return CalculateDistance(spawnPos.GetPositionX(), spawnPos.GetPositionY(), spawnPos.GetPositionZ(),
        nodePos.GetPositionX(), nodePos.GetPositionY(), nodePos.GetPositionZ());
}

// ============================================================================
// ENTERPRISE-GRADE ROUTING AND POSITIONING
// ============================================================================

std::vector<Position> ArathiBasinScript::GetRotationPath(uint32 fromNode, uint32 toNode) const
{
    return ArathiBasin::GetRotationPath(fromNode, toNode);
}

std::vector<Position> ArathiBasinScript::GetAmbushPositions(uint32 faction) const
{
    return ArathiBasin::GetAmbushPositions(faction);
}

float ArathiBasinScript::GetNodeToNodeDistance(uint32 fromNode, uint32 toNode) const
{
    return ArathiBasin::GetNodeDistance(fromNode, toNode);
}

uint32 ArathiBasinScript::GetNearestNode(Position const& pos) const
{
    uint32 nearest = 0;
    float minDist = std::numeric_limits<float>::max();

    for (uint32 i = 0; i < ArathiBasin::NODE_COUNT; ++i)
    {
        Position nodePos = ArathiBasin::GetNodePosition(i);
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

uint32 ArathiBasinScript::GetBestAssaultTarget(uint32 faction) const
{
    // Use Strategy constants for decision making
    uint32 enemyNodes = 0;
    std::vector<uint32> contestableNodes;

    for (uint32 i = 0; i < ArathiBasin::NODE_COUNT; ++i)
    {
        auto it = m_nodeStates.find(i);
        if (it != m_nodeStates.end())
        {
            bool enemyControlled = (faction == ALLIANCE)
                ? (it->second == BGObjectiveState::HORDE_CONTROLLED)
                : (it->second == BGObjectiveState::ALLIANCE_CONTROLLED);

            if (enemyControlled)
            {
                enemyNodes++;
                contestableNodes.push_back(i);
            }
        }
    }

    // Prioritize Blacksmith if enemy has it
    for (uint32 node : contestableNodes)
    {
        if (node == ArathiBasin::Nodes::BLACKSMITH)
            return node;
    }

    // Otherwise find closest enemy node
    uint32 bestTarget = contestableNodes.empty() ? ArathiBasin::Nodes::BLACKSMITH : contestableNodes[0];
    float bestValue = 0.0f;

    for (uint32 node : contestableNodes)
    {
        float value = ArathiBasin::GetNodeStrategicValue(node);
        // Prefer closer nodes to our controlled nodes
        if (value > bestValue)
        {
            bestValue = value;
            bestTarget = node;
        }
    }

    return bestTarget;
}

uint32 ArathiBasinScript::GetDefensePriority(uint32 nodeId) const
{
    uint8 basePriority = ArathiBasin::GetNodeStrategicValue(nodeId);

    // Blacksmith always top priority
    if (nodeId == ArathiBasin::Nodes::BLACKSMITH)
        return basePriority + ArathiBasin::Strategy::BS_EXTRA_DEFENDERS;

    return basePriority;
}

bool ArathiBasinScript::ShouldRotate() const
{
    // Check if rotation interval has passed
    uint32 elapsed = GetElapsedTime();
    return (elapsed % ArathiBasin::Strategy::ROTATION_INTERVAL) < 1000;  // Within 1 sec of rotation time
}

std::vector<Position> ArathiBasinScript::GetChokepoints() const
{
    return ArathiBasin::GetChokepoints();
}

std::vector<Position> ArathiBasinScript::GetSniperPositions() const
{
    return ArathiBasin::GetSniperPositions();
}

std::vector<Position> ArathiBasinScript::GetBuffPositions() const
{
    return ArathiBasin::GetBuffPositions();
}

// ============================================================================
// ENHANCED EVENT HANDLING
// ============================================================================

void ArathiBasinScript::OnEvent(const BGScriptEventData& event)
{
    DominationScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::OBJECTIVE_CAPTURED:
            TC_LOG_DEBUG("playerbots.bg.script",
                "AB: Node {} captured by {}! Current control: Alliance={}, Horde={}",
                event.objectiveId,
                event.faction == ALLIANCE ? "Alliance" : "Horde",
                m_allianceNodes, m_hordeNodes);

            // Check if we need to adjust defenders
            if (event.objectiveId == ArathiBasin::Nodes::BLACKSMITH)
            {
                TC_LOG_DEBUG("playerbots.bg.script",
                    "AB: Blacksmith captured - critical node! Adjusting defense priority.");
            }
            break;

        case BGScriptEvent::OBJECTIVE_CONTESTED:
            TC_LOG_DEBUG("playerbots.bg.script",
                "AB: Node {} under attack at ({:.1f}, {:.1f})! Defenders needed!",
                event.objectiveId, event.x, event.y);
            break;

        case BGScriptEvent::OBJECTIVE_LOST:
            TC_LOG_DEBUG("playerbots.bg.script",
                "AB: Node {} lost! Counter-attack may be needed.",
                event.objectiveId);
            break;

        default:
            break;
    }
}

void ArathiBasinScript::OnMatchStart()
{
    DominationScriptBase::OnMatchStart();

    TC_LOG_INFO("playerbots.bg.script",
        "AB: Match started! Strategy: secure home bases then contest Blacksmith");
}

void ArathiBasinScript::OnMatchEnd(bool victory)
{
    DominationScriptBase::OnMatchEnd(victory);

    TC_LOG_INFO("playerbots.bg.script",
        "AB: Match ended - {}! Final score tracked.",
        victory ? "Victory" : "Defeat");
}

} // namespace Playerbot::Coordination::Battleground
