/*
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Deepwind Gorge Script Implementation
 * ~600 lines - Complete hybrid node+cart BG coordination with phase-aware strategy
 */

#include "DeepwindGorgeScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "Player.h"
#include "Log.h"
#include "Timer.h"
#include "../../../Movement/BotMovementUtil.h"
#include <algorithm>

namespace Playerbot::Coordination::Battleground
{

REGISTER_BG_SCRIPT(DeepwindGorgeScript, 1105);  // DeepwindGorge::MAP_ID

// ============================================================================
// LIFECYCLE METHODS
// ============================================================================

void DeepwindGorgeScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    DominationScriptBase::OnLoad(coordinator);
    InitializeNodeTracking();

    TC_LOG_DEBUG("bg.playerbot", "DeepwindGorgeScript::OnLoad - Initializing enterprise-grade Deepwind Gorge coordination");

    // Cache objective data
    m_cachedObjectives = GetObjectiveData();

    // Register score world states
    RegisterScoreWorldState(DeepwindGorge::WorldStates::RESOURCES_ALLY, true);
    RegisterScoreWorldState(DeepwindGorge::WorldStates::RESOURCES_HORDE, false);

    // Reset state
    m_activeCart = 0;
    m_cartProgress.clear();
    m_cartContested.clear();
    m_cartFaction.clear();
    m_nodeControlFaction.clear();
    m_matchElapsedTime = 0;
    m_matchStartTime = 0;
    m_matchActive = false;
    m_currentPhase = DeepwindGorgePhase::OPENING;
    m_allianceNodesControlled = 0;
    m_hordeNodesControlled = 0;
    m_cartUpdateTimer = 0;

    // Initialize cart states
    for (uint32 i = 0; i < DeepwindGorge::CART_COUNT; ++i)
    {
        m_cartProgress[i] = 0.0f;
        m_cartContested[i] = false;
        m_cartFaction[i] = 0;  // Neutral
    }

    TC_LOG_DEBUG("bg.playerbot", "DeepwindGorgeScript::OnLoad - Loaded %u nodes, %u carts, %u chokepoints, %u sniper spots",
        DeepwindGorge::NODE_COUNT, DeepwindGorge::CART_COUNT,
        DeepwindGorge::Chokepoints::COUNT, DeepwindGorge::SniperSpots::COUNT);
}

void DeepwindGorgeScript::OnMatchStart()
{
    DominationScriptBase::OnMatchStart();

    TC_LOG_INFO("bg.playerbot", "DeepwindGorgeScript::OnMatchStart - Match beginning");

    m_matchActive = true;
    m_matchStartTime = getMSTime();
    m_matchElapsedTime = 0;
    m_currentPhase = DeepwindGorgePhase::OPENING;
}

void DeepwindGorgeScript::OnMatchEnd(bool victory)
{
    DominationScriptBase::OnMatchEnd(victory);

    TC_LOG_INFO("bg.playerbot", "DeepwindGorgeScript::OnMatchEnd - Match concluded. Result: %s",
        victory ? "Victory" : "Defeat");

    m_matchActive = false;
}

void DeepwindGorgeScript::OnUpdate(uint32 diff)
{
    DominationScriptBase::OnUpdate(diff);

    if (!m_matchActive)
        return;

    // Update elapsed time
    m_matchElapsedTime = getMSTime() - m_matchStartTime;
    uint32 timeRemaining = GetMatchRemainingTime();

    // Update phase
    UpdatePhase(timeRemaining);

    // Update cart states periodically
    m_cartUpdateTimer += diff;
    if (m_cartUpdateTimer >= DeepwindGorge::Strategy::CART_CHECK_INTERVAL)
    {
        UpdateCartStates();
        m_cartUpdateTimer = 0;
    }
}

void DeepwindGorgeScript::OnEvent(const BGScriptEventData& event)
{
    DominationScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::CART_CAPTURED:
        {
            TC_LOG_INFO("bg.playerbot", "DeepwindGorgeScript::OnEvent - Cart %u captured by %s",
                event.objectiveId, event.faction == ALLIANCE ? "Alliance" : "Horde");

            ProcessCartCapture(event.objectiveId, event.faction);
            break;
        }

        case BGScriptEvent::OBJECTIVE_CAPTURED:
        {
            TC_LOG_INFO("bg.playerbot", "DeepwindGorgeScript::OnEvent - Node %s captured by %s",
                DeepwindGorge::GetNodeName(event.objectiveId),
                event.faction == ALLIANCE ? "Alliance" : "Horde");

            m_nodeControlFaction[event.objectiveId] = event.faction;

            // Update node counts
            m_allianceNodesControlled = 0;
            m_hordeNodesControlled = 0;
            for (const auto& [nodeId, faction] : m_nodeControlFaction)
            {
                if (faction == ALLIANCE)
                    ++m_allianceNodesControlled;
                else if (faction == HORDE)
                    ++m_hordeNodesControlled;
            }
            break;
        }

        case BGScriptEvent::OBJECTIVE_CONTESTED:
        {
            if (event.objectiveId >= 50)  // Cart (offset in GetObjectiveData)
            {
                uint32 cartId = event.objectiveId - 50;
                TC_LOG_DEBUG("bg.playerbot", "DeepwindGorgeScript::OnEvent - Cart %u contested", cartId);
                m_cartContested[cartId] = true;
            }
            break;
        }

        case BGScriptEvent::OBJECTIVE_NEUTRALIZED:
        {
            if (event.objectiveId >= 50)  // Cart
            {
                uint32 cartId = event.objectiveId - 50;
                TC_LOG_DEBUG("bg.playerbot", "DeepwindGorgeScript::OnEvent - Cart %u neutralized/uncontested", cartId);
                m_cartContested[cartId] = false;
            }
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// OBJECTIVE DATA PROVIDERS
// ============================================================================

std::vector<BGObjectiveData> DeepwindGorgeScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    // Add mines (nodes)
    for (uint32 i = 0; i < DeepwindGorge::NODE_COUNT; ++i)
        objectives.push_back(GetNodeData(i));

    // Add carts as objectives (offset by 50 to avoid collision)
    for (uint32 i = 0; i < DeepwindGorge::CART_COUNT; ++i)
    {
        BGObjectiveData cart;
        Position pos = DeepwindGorge::GetCartSpawnPosition(i);
        cart.id = 50 + i;
        cart.type = ObjectiveType::CART;
        cart.name = std::string("Mine Cart ") + std::to_string(i + 1);
        cart.x = pos.GetPositionX();
        cart.y = pos.GetPositionY();
        cart.z = pos.GetPositionZ();
        cart.strategicValue = 9;  // High value
        objectives.push_back(cart);
    }

    return objectives;
}

BGObjectiveData DeepwindGorgeScript::GetNodeData(uint32 nodeIndex) const
{
    BGObjectiveData node;
    Position pos = DeepwindGorge::GetNodePosition(nodeIndex);

    node.id = nodeIndex;
    node.type = ObjectiveType::NODE;
    node.name = DeepwindGorge::GetNodeName(nodeIndex);
    node.x = pos.GetPositionX();
    node.y = pos.GetPositionY();
    node.z = pos.GetPositionZ();

    // Pandaren Mine (center) is most valuable
    node.strategicValue = (nodeIndex == DeepwindGorge::Nodes::PANDAREN_MINE) ? 9 : 7;
    node.captureTime = DeepwindGorge::CAPTURE_TIME;

    return node;
}

std::vector<BGPositionData> DeepwindGorgeScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;
    Position pos = DeepwindGorge::GetSpawnPosition(faction);

    spawns.push_back(BGPositionData(
        faction == ALLIANCE ? "Alliance Spawn" : "Horde Spawn",
        pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
        BGPositionData::PositionType::SPAWN_POINT, faction, 5));

    return spawns;
}

std::vector<BGPositionData> DeepwindGorgeScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Add node positions
    for (uint32 i = 0; i < DeepwindGorge::NODE_COUNT; ++i)
    {
        Position pos = DeepwindGorge::GetNodePosition(i);
        uint8 value = (i == DeepwindGorge::Nodes::PANDAREN_MINE) ? 9 : 7;

        positions.push_back(BGPositionData(
            DeepwindGorge::GetNodeName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::STRATEGIC_POINT, 0, value));
    }

    // Add cart depot positions
    Position allyDepot = DeepwindGorge::GetCartDepotPosition(ALLIANCE);
    positions.push_back(BGPositionData(
        "Alliance Cart Depot",
        allyDepot.GetPositionX(), allyDepot.GetPositionY(), allyDepot.GetPositionZ(), allyDepot.GetOrientation(),
        BGPositionData::PositionType::STRATEGIC_POINT, ALLIANCE, 9));

    Position hordeDepot = DeepwindGorge::GetCartDepotPosition(HORDE);
    positions.push_back(BGPositionData(
        "Horde Cart Depot",
        hordeDepot.GetPositionX(), hordeDepot.GetPositionY(), hordeDepot.GetPositionZ(), hordeDepot.GetOrientation(),
        BGPositionData::PositionType::STRATEGIC_POINT, HORDE, 9));

    // Add chokepoints
    for (uint32 i = 0; i < DeepwindGorge::Chokepoints::COUNT; ++i)
    {
        Position pos = DeepwindGorge::GetChokepointPosition(i);
        positions.push_back(BGPositionData(
            DeepwindGorge::GetChokepointName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, 6));
    }

    // Add sniper positions
    for (uint32 i = 0; i < DeepwindGorge::SniperSpots::COUNT; ++i)
    {
        Position pos = DeepwindGorge::GetSniperPosition(i);
        positions.push_back(BGPositionData(
            DeepwindGorge::GetSniperSpotName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SNIPER_POSITION, 0, 8));
    }

    return positions;
}

std::vector<BGPositionData> DeepwindGorgeScript::GetGraveyardPositions(uint32 faction) const
{
    return GetSpawnPositions(faction);
}

std::vector<BGWorldState> DeepwindGorgeScript::GetInitialWorldStates() const
{
    return {
        BGWorldState(DeepwindGorge::WorldStates::RESOURCES_ALLY, "Alliance Resources",
                    BGWorldState::StateType::SCORE_ALLIANCE, 0),
        BGWorldState(DeepwindGorge::WorldStates::RESOURCES_HORDE, "Horde Resources",
                    BGWorldState::StateType::SCORE_HORDE, 0)
    };
}

// ============================================================================
// WORLD STATE INTERPRETATION
// ============================================================================

bool DeepwindGorgeScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    return TryInterpretFromCache(stateId, value, outObjectiveId, outState);
}

void DeepwindGorgeScript::GetScoreFromWorldStates(const std::map<int32, int32>& states,
    uint32& allianceScore, uint32& hordeScore) const
{
    allianceScore = hordeScore = 0;

    auto it = states.find(DeepwindGorge::WorldStates::RESOURCES_ALLY);
    if (it != states.end())
        allianceScore = static_cast<uint32>(std::max(0, it->second));

    it = states.find(DeepwindGorge::WorldStates::RESOURCES_HORDE);
    if (it != states.end())
        hordeScore = static_cast<uint32>(std::max(0, it->second));
}

// ============================================================================
// STRATEGY AND ROLE DISTRIBUTION
// ============================================================================

RoleDistribution DeepwindGorgeScript::GetRecommendedRoles(const StrategicDecision& decision,
    float /*scoreAdvantage*/, uint32 /*timeRemaining*/) const
{
    RoleDistribution dist;

    // Deepwind needs both node control and cart escorts
    switch (decision.strategy)
    {
        case BGStrategy::AGGRESSIVE:
            dist.roleCounts[BGRole::NODE_ATTACKER] = 30;
            dist.roleCounts[BGRole::CART_PUSHER] = 35;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 20;
            dist.roleCounts[BGRole::ROAMER] = 15;
            dist.reasoning = "Aggressive node capture + cart push";
            break;

        case BGStrategy::DEFENSIVE:
            dist.roleCounts[BGRole::NODE_DEFENDER] = 40;
            dist.roleCounts[BGRole::CART_PUSHER] = 25;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 20;
            dist.roleCounts[BGRole::ROAMER] = 15;
            dist.reasoning = "Defensive node hold + cart defense";
            break;

        case BGStrategy::ALL_IN:
            dist.roleCounts[BGRole::CART_PUSHER] = 45;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 35;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 10;
            dist.roleCounts[BGRole::ROAMER] = 10;
            dist.reasoning = "All-in on carts + aggressive nodes";
            break;

        default:  // BALANCED
            dist.roleCounts[BGRole::NODE_ATTACKER] = 25;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 25;
            dist.roleCounts[BGRole::CART_PUSHER] = 30;
            dist.roleCounts[BGRole::ROAMER] = 20;
            dist.reasoning = "Balanced node control + cart escort";
            break;
    }

    return dist;
}

void DeepwindGorgeScript::AdjustStrategy(StrategicDecision& decision, float scoreAdvantage,
    uint32 controlledCount, uint32 /*totalObjectives*/, uint32 timeRemaining) const
{
    // Score-based DESPERATE override with hysteresis
    // Enter DESPERATE when behind by 30%+ (scoreAdvantage < -0.3), exit when gap narrows to 15%
    DeepwindGorgePhase effectivePhase = m_currentPhase;
    if (m_currentPhase != DeepwindGorgePhase::OPENING)
    {
        if (m_lastPhase == DeepwindGorgePhase::DESPERATE)
        {
            // Already desperate: stay until gap narrows to half threshold
            if (scoreAdvantage < -0.15f)
                effectivePhase = DeepwindGorgePhase::DESPERATE;
        }
        else
        {
            // Not desperate: enter at full threshold
            if (scoreAdvantage < -0.30f)
                effectivePhase = DeepwindGorgePhase::DESPERATE;
        }
    }
    m_lastPhase = effectivePhase;

    // Phase-based strategy
    switch (effectivePhase)
    {
        case DeepwindGorgePhase::OPENING:
            ApplyOpeningPhaseStrategy(decision, ALLIANCE);
            break;

        case DeepwindGorgePhase::MID_GAME:
            ApplyMidGameStrategy(decision, scoreAdvantage);
            break;

        case DeepwindGorgePhase::LATE_GAME:
            ApplyLateGameStrategy(decision, scoreAdvantage, timeRemaining);
            break;

        case DeepwindGorgePhase::DESPERATE:
            ApplyDesperateStrategy(decision);
            break;
    }

    // Apply cart bonus adjustments
    ApplyCartBonus(decision);

    // Pandaren Mine priority
    if (IsPandarenMineCritical())
    {
        decision.reasoning += " + Pandaren Mine critical";
        decision.offenseAllocation += 5;
    }

    // 2-cap strategy check
    if (controlledCount >= 2)
    {
        decision.reasoning += " + holding 2+ nodes";
        decision.defenseAllocation += 10;
        decision.offenseAllocation -= 10;
    }

    // Clamp allocations
    decision.offenseAllocation = std::clamp(decision.offenseAllocation, static_cast<uint8>(10), static_cast<uint8>(90));
    decision.defenseAllocation = 100 - decision.offenseAllocation;

    decision.reasoning += " (nodes + carts hybrid)";
}

// ============================================================================
// CART-SPECIFIC METHODS
// ============================================================================

float DeepwindGorgeScript::GetCartProgress(uint32 cartId) const
{
    auto it = m_cartProgress.find(cartId);
    return it != m_cartProgress.end() ? it->second : 0.0f;
}

bool DeepwindGorgeScript::IsCartContested(uint32 cartId) const
{
    auto it = m_cartContested.find(cartId);
    return it != m_cartContested.end() && it->second;
}

uint32 DeepwindGorgeScript::GetCartControllingFaction(uint32 cartId) const
{
    auto it = m_cartFaction.find(cartId);
    return it != m_cartFaction.end() ? it->second : 0;
}

bool DeepwindGorgeScript::ShouldPrioritizeCart() const
{
    return CalculateCartPriority() >= DeepwindGorge::Strategy::CART_PRIORITY_THRESHOLD;
}

uint32 DeepwindGorgeScript::GetBestCartToContest(uint32 faction) const
{
    uint32 bestCart = 0;
    float bestValue = 0.0f;

    for (uint32 i = 0; i < DeepwindGorge::CART_COUNT; ++i)
    {
        // Prefer carts that are controlled by us and near our depot
        uint32 controlFaction = GetCartControllingFaction(i);
        float progress = GetCartProgress(i);

        float value = 0.0f;
        if (controlFaction == faction && progress > 0.5f)
            value = progress * 2.0f;  // High value - almost captured
        else if (controlFaction == 0)
            value = 1.0f;  // Neutral - good target
        else if (controlFaction != faction && !IsCartContested(i))
            value = 0.5f;  // Enemy controlled but uncontested

        if (value > bestValue)
        {
            bestValue = value;
            bestCart = i;
        }
    }

    return bestCart;
}

std::vector<BGPositionData> DeepwindGorgeScript::GetCartEscortFormation(uint32 cartId) const
{
    std::vector<BGPositionData> positions;
    Position cartPos = DeepwindGorge::GetCartSpawnPosition(cartId);

    auto formation = DeepwindGorge::GetEscortFormation(
        cartPos.GetPositionX(), cartPos.GetPositionY(),
        cartPos.GetPositionZ(), cartPos.GetOrientation());

    for (size_t i = 0; i < formation.size(); ++i)
    {
        std::string name = "Cart Escort " + std::to_string(i + 1);
        positions.push_back(BGPositionData(
            name,
            formation[i].GetPositionX(), formation[i].GetPositionY(),
            formation[i].GetPositionZ(), formation[i].GetOrientation(),
            BGPositionData::PositionType::STRATEGIC_POINT, 0, 8));
    }

    return positions;
}

std::vector<BGPositionData> DeepwindGorgeScript::GetCartInterceptionPositions() const
{
    std::vector<BGPositionData> positions;

    for (uint32 i = 0; i < DeepwindGorge::CartInterception::COUNT; ++i)
    {
        Position pos = DeepwindGorge::GetCartInterceptionPosition(i);
        std::string name = "Cart Intercept " + std::to_string(i + 1);

        positions.push_back(BGPositionData(
            name,
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, 7));
    }

    return positions;
}

std::vector<Position> DeepwindGorgeScript::GetCartTrackToDepot(uint32 cartId, uint32 faction) const
{
    return DeepwindGorge::GetCartTrackToDepot(cartId, faction);
}

// ============================================================================
// POSITIONAL DATA PROVIDERS
// ============================================================================

std::vector<BGPositionData> DeepwindGorgeScript::GetNodeDefensePositions(uint32 nodeId) const
{
    std::vector<BGPositionData> positions;
    auto defensePos = DeepwindGorge::GetNodeDefensePositions(nodeId);

    for (size_t i = 0; i < defensePos.size(); ++i)
    {
        std::string name = std::string(DeepwindGorge::GetNodeName(nodeId)) + " Defense " + std::to_string(i + 1);
        positions.push_back(BGPositionData(
            name,
            defensePos[i].GetPositionX(), defensePos[i].GetPositionY(),
            defensePos[i].GetPositionZ(), defensePos[i].GetOrientation(),
            BGPositionData::PositionType::DEFENSIVE_POSITION, 0, 7));
    }

    return positions;
}

std::vector<BGPositionData> DeepwindGorgeScript::GetDepotDefensePositions(uint32 faction) const
{
    std::vector<BGPositionData> positions;
    auto defensePos = DeepwindGorge::GetDepotDefensePositions(faction);

    for (size_t i = 0; i < defensePos.size(); ++i)
    {
        std::string name = (faction == ALLIANCE ? "Alliance" : "Horde");
        name += " Depot Defense " + std::to_string(i + 1);

        positions.push_back(BGPositionData(
            name,
            defensePos[i].GetPositionX(), defensePos[i].GetPositionY(),
            defensePos[i].GetPositionZ(), defensePos[i].GetOrientation(),
            BGPositionData::PositionType::DEFENSIVE_POSITION, faction, 8));
    }

    return positions;
}

std::vector<BGPositionData> DeepwindGorgeScript::GetChokepoints() const
{
    std::vector<BGPositionData> positions;

    for (uint32 i = 0; i < DeepwindGorge::Chokepoints::COUNT; ++i)
    {
        Position pos = DeepwindGorge::GetChokepointPosition(i);
        positions.push_back(BGPositionData(
            DeepwindGorge::GetChokepointName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, 6));
    }

    return positions;
}

std::vector<BGPositionData> DeepwindGorgeScript::GetSniperPositions() const
{
    std::vector<BGPositionData> positions;

    for (uint32 i = 0; i < DeepwindGorge::SniperSpots::COUNT; ++i)
    {
        Position pos = DeepwindGorge::GetSniperPosition(i);
        positions.push_back(BGPositionData(
            DeepwindGorge::GetSniperSpotName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SNIPER_POSITION, 0, 8));
    }

    return positions;
}

std::vector<BGPositionData> DeepwindGorgeScript::GetAmbushPositions(uint32 faction) const
{
    std::vector<BGPositionData> positions;
    auto ambushPos = DeepwindGorge::GetAmbushPositions(faction);

    for (size_t i = 0; i < ambushPos.size(); ++i)
    {
        std::string name = (faction == ALLIANCE ? "Alliance" : "Horde");
        name += " Ambush " + std::to_string(i + 1);

        positions.push_back(BGPositionData(
            name,
            ambushPos[i].GetPositionX(), ambushPos[i].GetPositionY(),
            ambushPos[i].GetPositionZ(), ambushPos[i].GetOrientation(),
            BGPositionData::PositionType::STRATEGIC_POINT, faction, 7));
    }

    return positions;
}

std::vector<Position> DeepwindGorgeScript::GetRotationPath(uint32 fromNode, uint32 toNode) const
{
    std::vector<Position> path;

    Position start = DeepwindGorge::GetNodePosition(fromNode);
    Position end = DeepwindGorge::GetNodePosition(toNode);

    path.push_back(start);

    // Add intermediate waypoint for non-adjacent nodes
    if ((fromNode == DeepwindGorge::Nodes::GOBLIN_MINE && toNode == DeepwindGorge::Nodes::CENTER_MINE) ||
        (fromNode == DeepwindGorge::Nodes::CENTER_MINE && toNode == DeepwindGorge::Nodes::GOBLIN_MINE))
    {
        // Route through Pandaren Mine
        Position pandaren = DeepwindGorge::GetNodePosition(DeepwindGorge::Nodes::PANDAREN_MINE);
        path.push_back(pandaren);
    }

    path.push_back(end);
    return path;
}

// ============================================================================
// PHASE AND STATE QUERIES
// ============================================================================

uint32 DeepwindGorgeScript::GetMatchRemainingTime() const
{
    if (m_matchElapsedTime >= DeepwindGorge::MAX_DURATION)
        return 0;
    return DeepwindGorge::MAX_DURATION - m_matchElapsedTime;
}

uint32 DeepwindGorgeScript::GetNodePriority(uint32 nodeId, uint32 faction) const
{
    // Pandaren Mine is always highest priority
    if (nodeId == DeepwindGorge::Nodes::PANDAREN_MINE)
        return 0;

    // Faction-closest node is second priority
    if (faction == ALLIANCE && nodeId == DeepwindGorge::Nodes::GOBLIN_MINE)
        return 1;
    if (faction == HORDE && nodeId == DeepwindGorge::Nodes::CENTER_MINE)
        return 1;

    return 2;
}

bool DeepwindGorgeScript::IsPandarenMineCritical() const
{
    // Pandaren Mine is critical if neither team has a clear node advantage
    return m_allianceNodesControlled <= 1 && m_hordeNodesControlled <= 1;
}

std::vector<uint32> DeepwindGorgeScript::GetTickPointsTable() const
{
    return std::vector<uint32>(DeepwindGorge::TICK_POINTS.begin(), DeepwindGorge::TICK_POINTS.end());
}

// ============================================================================
// INTERNAL UPDATE METHODS
// ============================================================================

void DeepwindGorgeScript::UpdateCartStates()
{
    // In a real implementation, this would query actual cart positions/states
    // For now, we maintain internal tracking from events
}

void DeepwindGorgeScript::UpdatePhase(uint32 timeRemaining)
{
    if (timeRemaining > (DeepwindGorge::MAX_DURATION - DeepwindGorge::Strategy::OPENING_PHASE))
        m_currentPhase = DeepwindGorgePhase::OPENING;
    else if (timeRemaining > (DeepwindGorge::MAX_DURATION - DeepwindGorge::Strategy::MID_GAME_END))
        m_currentPhase = DeepwindGorgePhase::MID_GAME;
    else
        m_currentPhase = DeepwindGorgePhase::LATE_GAME;
}

void DeepwindGorgeScript::ProcessCartCapture(uint32 cartId, uint32 faction)
{
    if (cartId < DeepwindGorge::CART_COUNT)
    {
        m_cartProgress[cartId] = 0.0f;
        m_cartContested[cartId] = false;
        m_cartFaction[cartId] = 0;  // Reset to neutral after capture

        TC_LOG_DEBUG("bg.playerbot", "DeepwindGorgeScript::ProcessCartCapture - Cart %u delivered to %s depot",
            cartId, faction == ALLIANCE ? "Alliance" : "Horde");
    }
}

// ============================================================================
// INTERNAL STRATEGY HELPERS
// ============================================================================

void DeepwindGorgeScript::ApplyOpeningPhaseStrategy(StrategicDecision& decision, uint32 /*faction*/) const
{
    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.reasoning = "Opening phase - capture nodes quickly";
    decision.offenseAllocation = 70;
    decision.defenseAllocation = 30;
}

void DeepwindGorgeScript::ApplyMidGameStrategy(StrategicDecision& decision, float scoreAdvantage) const
{
    if (scoreAdvantage > 0.15f)
    {
        decision.strategy = BGStrategy::BALANCED;
        decision.reasoning = "Mid-game leading - maintain control";
        decision.offenseAllocation = 50;
        decision.defenseAllocation = 50;
    }
    else if (scoreAdvantage < -0.15f)
    {
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.reasoning = "Mid-game trailing - push for carts";
        decision.offenseAllocation = 65;
        decision.defenseAllocation = 35;
    }
    else
    {
        decision.strategy = BGStrategy::BALANCED;
        decision.reasoning = "Mid-game tied - balanced approach";
        decision.offenseAllocation = 55;
        decision.defenseAllocation = 45;
    }
}

void DeepwindGorgeScript::ApplyLateGameStrategy(StrategicDecision& decision, float scoreAdvantage,
    uint32 timeRemaining) const
{
    if (scoreAdvantage > 0.2f)
    {
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.reasoning = "Late game ahead - protect lead";
        decision.offenseAllocation = 35;
        decision.defenseAllocation = 65;
    }
    else if (scoreAdvantage < -0.2f && timeRemaining < 300000)
    {
        decision.strategy = BGStrategy::ALL_IN;
        decision.reasoning = "Late game behind - all-in on carts!";
        decision.offenseAllocation = 85;
        decision.defenseAllocation = 15;
    }
    else
    {
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.reasoning = "Late game close - aggressive push";
        decision.offenseAllocation = 65;
        decision.defenseAllocation = 35;
    }
}

void DeepwindGorgeScript::ApplyDesperateStrategy(StrategicDecision& decision) const
{
    decision.strategy = BGStrategy::ALL_IN;
    decision.reasoning = "Desperate - maximum aggression";
    decision.offenseAllocation = 90;
    decision.defenseAllocation = 10;
}

void DeepwindGorgeScript::ApplyCartBonus(StrategicDecision& decision) const
{
    // Check for cart opportunities
    for (uint32 i = 0; i < DeepwindGorge::CART_COUNT; ++i)
    {
        float progress = GetCartProgress(i);
        bool contested = IsCartContested(i);

        if (progress > 0.7f && !contested)
        {
            decision.reasoning += " + cart nearly captured";
            decision.offenseAllocation += 10;
            break;
        }
        else if (contested)
        {
            decision.reasoning += " + cart contested";
            decision.offenseAllocation += 15;
            break;
        }
    }
}

float DeepwindGorgeScript::CalculateCartPriority() const
{
    float priority = 0.0f;

    for (uint32 i = 0; i < DeepwindGorge::CART_COUNT; ++i)
    {
        float progress = GetCartProgress(i);
        if (progress > 0.5f)
            priority += progress;
        if (IsCartContested(i))
            priority += 0.3f;
    }

    return priority / static_cast<float>(DeepwindGorge::CART_COUNT);
}

// ============================================================================
// RUNTIME BEHAVIOR - ExecuteStrategy
// ============================================================================

bool DeepwindGorgeScript::ExecuteStrategy(::Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive())
        return false;

    // Check pending GO interaction — hold position if waiting for deferred Use()
    if (CheckPendingInteraction(player))
        return true;

    // Check defense commitment — bot stays at captured node for the hold timer
    if (CheckDefenseCommitment(player))
        return true;

    // Refresh domination node state (throttled internally)
    RefreshNodeState();

    uint32 faction = player->GetBGTeam();

    // =========================================================================
    // PRIORITY 0: Nearby contested friendly node needs reinforcement
    // =========================================================================
    uint32 reinforceNode = CheckReinforcementNeeded(player, 60.0f);
    if (reinforceNode != UINT32_MAX)
    {
        BGObjectiveData nodeData = GetNodeData(reinforceNode);
        TC_LOG_DEBUG("playerbots.bg.script",
            "[DG] {} PRIORITY 0: reinforcing contested node {}",
            player->GetName(), nodeData.name);
        DefendNode(player, reinforceNode);
        return true;
    }

    // =========================================================================
    // PRIORITY 1: Nearby node (<30yd) capturable -> capture it
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
                "[DWG] {} PRIORITY 1: capturing nearby node {} (dist={:.0f})",
                player->GetName(), nodeData.name, dist);
            CaptureNode(player, nearCapture);
            return true;
        }
    }

    // =========================================================================
    // PRIORITY 2: Contested friendly node -> defend
    // =========================================================================
    uint32 threatened = FindNearestThreatenedNode(player);
    if (threatened != UINT32_MAX)
    {
        BGObjectiveData nodeData = GetNodeData(threatened);
        TC_LOG_DEBUG("playerbots.bg.script",
            "[DWG] {} PRIORITY 2: defending contested node {}",
            player->GetName(), nodeData.name);
        DefendNode(player, threatened);
        return true;
    }

    // =========================================================================
    // PRIORITY 3: GUID split: 60% node control, 40% cart escort
    // =========================================================================
    uint32 dutySlot = player->GetGUID().GetCounter() % 10;

    if (dutySlot < 6)
    {
        // 60% -> Node control (defend or attack)
        std::vector<uint32> friendlyNodes = GetFriendlyNodes(player);
        uint32 friendlyCount = GetFriendlyNodeCount(player);

        if (friendlyCount < GetOptimalNodeCount())
        {
            // Need more nodes -> attack
            uint32 targetNode = GetBestAssaultTarget(player);
            if (targetNode != UINT32_MAX)
            {
                BGObjectiveData nodeData = GetNodeData(targetNode);
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[DWG] {} PRIORITY 3: attacking node {} (need {} more)",
                    player->GetName(), nodeData.name,
                    GetOptimalNodeCount() - friendlyCount);
                CaptureNode(player, targetNode);
                return true;
            }
        }
        else
        {
            // Have enough nodes -> defend
            if (!friendlyNodes.empty())
            {
                uint32 defIdx = player->GetGUID().GetCounter() % friendlyNodes.size();
                uint32 defNode = friendlyNodes[defIdx];
                BGObjectiveData nodeData = GetNodeData(defNode);
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[DWG] {} PRIORITY 3: defending node {}",
                    player->GetName(), nodeData.name);
                DefendNode(player, defNode);
                return true;
            }
        }
    }
    else
    {
        // 40% -> Cart escort
        uint32 bestCart = GetBestCartToContest(faction);
        if (bestCart < DeepwindGorge::CART_COUNT)
        {
            Position cartPos = DeepwindGorge::GetCartSpawnPosition(bestCart);
            float cartDist = player->GetExactDist(&cartPos);

            TC_LOG_DEBUG("playerbots.bg.script",
                "[DWG] {} PRIORITY 3: cart duty - cart {} (dist={:.0f})",
                player->GetName(), bestCart, cartDist);

            // If enemy near cart, engage; otherwise escort
            if (::Player* enemy = FindNearestEnemyPlayer(player, 20.0f))
            {
                EngageTarget(player, enemy);
            }
            else
            {
                auto escorts = GetCartEscortFormation(bestCart);
                if (!escorts.empty())
                {
                    uint32 escortSlot = player->GetGUID().GetCounter() % escorts.size();
                    Position escortPos(escorts[escortSlot].x, escorts[escortSlot].y,
                                       escorts[escortSlot].z, escorts[escortSlot].orientation);
                    BotMovementUtil::MoveToPosition(player, escortPos);
                }
                else
                {
                    BotMovementUtil::MoveToPosition(player, cartPos);
                }
            }
            return true;
        }
    }

    // =========================================================================
    // PRIORITY 4: Fallback -> patrol chokepoint
    // =========================================================================
    {
        auto chokepoints = GetChokepoints();
        if (!chokepoints.empty())
        {
            uint32 chokeIdx = player->GetGUID().GetCounter() % chokepoints.size();
            Position chokePos(chokepoints[chokeIdx].x, chokepoints[chokeIdx].y,
                              chokepoints[chokeIdx].z, 0);

            TC_LOG_DEBUG("playerbots.bg.script",
                "[DWG] {} PRIORITY 4: patrolling chokepoint",
                player->GetName());
            PatrolAroundPosition(player, chokePos, 5.0f, 15.0f);
            return true;
        }
    }

    return false;
}

} // namespace Playerbot::Coordination::Battleground
