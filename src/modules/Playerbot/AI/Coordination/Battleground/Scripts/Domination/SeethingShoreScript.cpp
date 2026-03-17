/*
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Seething Shore Script Implementation
 * ~650 lines - Complete dynamic node BG coordination with phase-aware strategy
 */

#include "SeethingShoreScript.h"
#include "BGScriptRegistry.h"
#include "BattlegroundCoordinator.h"
#include "Player.h"
#include "Log.h"
#include "Timer.h"
#include "../../../Movement/BotMovementUtil.h"
#include <algorithm>
#include <random>

namespace Playerbot::Coordination::Battleground
{

REGISTER_BG_SCRIPT(SeethingShoreScript, 1803);  // SeethingShore::MAP_ID

// ============================================================================
// LIFECYCLE METHODS
// ============================================================================

void SeethingShoreScript::OnLoad(BattlegroundCoordinator* coordinator)
{
    DominationScriptBase::OnLoad(coordinator);
    InitializeNodeTracking();

    TC_LOG_DEBUG("bg.playerbot", "SeethingShoreScript::OnLoad - Initializing enterprise-grade Seething Shore coordination");

    // Cache objective data
    m_cachedObjectives = GetObjectiveData();

    // Register score world states
    RegisterScoreWorldState(SeethingShore::WorldStates::AZERITE_ALLY, true);
    RegisterScoreWorldState(SeethingShore::WorldStates::AZERITE_HORDE, false);

    // Reset dynamic state
    m_activeNodes.clear();
    m_contestedNodeIds.clear();
    m_zoneCaptureTimestamps.clear();
    m_nextSpawnTimer = 0;
    m_nextNodeId = 0;
    m_nodeSpawnCooldown = 0;
    m_matchElapsedTime = 0;
    m_matchStartTime = 0;
    m_matchActive = false;
    m_currentPhase = SeethingShorePhase::OPENING;
    m_cachedFaction = 0;
    m_cachedPriorityZones.clear();

    TC_LOG_DEBUG("bg.playerbot", "SeethingShoreScript::OnLoad - Loaded %zu zone positions, %u chokepoints, %u sniper spots",
        SeethingShore::SpawnZones::ZONE_COUNT, SeethingShore::Chokepoints::COUNT, SeethingShore::SniperSpots::COUNT);
}

void SeethingShoreScript::OnMatchStart()
{
    DominationScriptBase::OnMatchStart();

    TC_LOG_INFO("bg.playerbot", "SeethingShoreScript::OnMatchStart - Match beginning, spawning initial nodes");

    m_matchActive = true;
    m_matchStartTime = getMSTime();
    m_matchElapsedTime = 0;
    m_currentPhase = SeethingShorePhase::OPENING;

    // Spawn initial nodes (3 active at start)
    while (m_activeNodes.size() < SeethingShore::MAX_ACTIVE_NODES)
    {
        SpawnNewNode();
    }

    TC_LOG_INFO("bg.playerbot", "SeethingShoreScript::OnMatchStart - Spawned %zu initial Azerite nodes",
        m_activeNodes.size());
}

void SeethingShoreScript::OnMatchEnd(bool victory)
{
    DominationScriptBase::OnMatchEnd(victory);

    TC_LOG_INFO("bg.playerbot", "SeethingShoreScript::OnMatchEnd - Match concluded. Result: %s",
        victory ? "Victory" : "Defeat");

    m_matchActive = false;
    m_activeNodes.clear();
    m_contestedNodeIds.clear();
}

void SeethingShoreScript::OnUpdate(uint32 diff)
{
    DominationScriptBase::OnUpdate(diff);

    if (!m_matchActive)
        return;

    // Update elapsed time
    m_matchElapsedTime = getMSTime() - m_matchStartTime;
    uint32 timeRemaining = GetMatchRemainingTime();

    // Update phase based on time
    UpdatePhase(timeRemaining);

    // Update contested status
    UpdateContestedStatus();

    // Update active nodes (ensure we have 3 at all times)
    UpdateActiveNodes();

    // Handle node spawn cooldown
    if (m_nodeSpawnCooldown > 0)
    {
        if (m_nodeSpawnCooldown <= diff)
            m_nodeSpawnCooldown = 0;
        else
            m_nodeSpawnCooldown -= diff;
    }
}

void SeethingShoreScript::OnEvent(const BGScriptEventData& event)
{
    DominationScriptBase::OnEvent(event);

    switch (event.eventType)
    {
        case BGScriptEvent::AZERITE_SPAWNED:
        {
            TC_LOG_DEBUG("bg.playerbot", "SeethingShoreScript::OnEvent - Azerite node spawned in zone %u",
                event.objectiveId);
            // Track as recently spawned for high-priority diversion
            m_recentlySpawnedNode.zoneId = event.objectiveId;
            m_recentlySpawnedNode.spawnTime = getMSTime();
            break;
        }

        case BGScriptEvent::OBJECTIVE_CAPTURED:
        {
            TC_LOG_INFO("bg.playerbot", "SeethingShoreScript::OnEvent - Node %u captured by %s",
                event.objectiveId, event.faction == ALLIANCE ? "Alliance" : "Horde");

            // Remove captured node and record capture time for respawn cooldown
            RemoveCapturedNode(event.objectiveId);
            break;
        }

        case BGScriptEvent::OBJECTIVE_CONTESTED:
        {
            TC_LOG_DEBUG("bg.playerbot", "SeethingShoreScript::OnEvent - Node %u contested", event.objectiveId);
            m_contestedNodeIds.insert(event.objectiveId);
            break;
        }

        case BGScriptEvent::OBJECTIVE_NEUTRALIZED:
        {
            TC_LOG_DEBUG("bg.playerbot", "SeethingShoreScript::OnEvent - Node %u neutralized/uncontested", event.objectiveId);
            m_contestedNodeIds.erase(event.objectiveId);
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// OBJECTIVE DATA PROVIDERS
// ============================================================================

std::vector<BGObjectiveData> SeethingShoreScript::GetObjectiveData() const
{
    std::vector<BGObjectiveData> objectives;

    // Add all potential zones as objectives (even if not currently active)
    for (uint32 i = 0; i < SeethingShore::SpawnZones::ZONE_COUNT; ++i)
        objectives.push_back(GetNodeData(i));

    return objectives;
}

BGObjectiveData SeethingShoreScript::GetNodeData(uint32 nodeIndex) const
{
    BGObjectiveData node;
    Position pos = SeethingShore::GetZoneCenter(nodeIndex);

    node.id = nodeIndex;
    node.type = ObjectiveType::NODE;
    node.name = SeethingShore::GetZoneName(nodeIndex);
    node.x = pos.GetPositionX();
    node.y = pos.GetPositionY();
    node.z = pos.GetPositionZ();

    // Base strategic value, elevated zones are slightly more valuable
    node.strategicValue = SeethingShore::IsElevatedZone(nodeIndex) ? 9 : 8;
    node.captureTime = SeethingShore::CAPTURE_TIME;

    return node;
}

std::vector<BGPositionData> SeethingShoreScript::GetSpawnPositions(uint32 faction) const
{
    std::vector<BGPositionData> spawns;
    Position pos = SeethingShore::GetSpawnPosition(faction);

    spawns.push_back(BGPositionData(
        faction == ALLIANCE ? "Alliance Spawn" : "Horde Spawn",
        pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
        BGPositionData::PositionType::SPAWN_POINT, faction, 5));

    return spawns;
}

std::vector<BGPositionData> SeethingShoreScript::GetStrategicPositions() const
{
    std::vector<BGPositionData> positions;

    // Add all zone centers as strategic points
    for (uint32 i = 0; i < SeethingShore::SpawnZones::ZONE_COUNT; ++i)
    {
        Position pos = SeethingShore::GetZoneCenter(i);
        uint8 value = SeethingShore::IsElevatedZone(i) ? 9 : 7;

        positions.push_back(BGPositionData(
            SeethingShore::GetZoneName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::STRATEGIC_POINT, 0, value));
    }

    // Add chokepoints
    for (uint32 i = 0; i < SeethingShore::Chokepoints::COUNT; ++i)
    {
        Position pos = SeethingShore::GetChokepointPosition(i);
        positions.push_back(BGPositionData(
            SeethingShore::GetChokepointName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, 6));
    }

    // Add sniper positions
    for (uint32 i = 0; i < SeethingShore::SniperSpots::COUNT; ++i)
    {
        Position pos = SeethingShore::GetSniperPosition(i);
        positions.push_back(BGPositionData(
            SeethingShore::GetSniperSpotName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SNIPER_POSITION, 0, 8));
    }

    return positions;
}

std::vector<BGPositionData> SeethingShoreScript::GetGraveyardPositions(uint32 faction) const
{
    // Seething Shore uses faction spawns as graveyards
    return GetSpawnPositions(faction);
}

std::vector<BGWorldState> SeethingShoreScript::GetInitialWorldStates() const
{
    return {
        BGWorldState(SeethingShore::WorldStates::AZERITE_ALLY, "Alliance Azerite",
                    BGWorldState::StateType::SCORE_ALLIANCE, 0),
        BGWorldState(SeethingShore::WorldStates::AZERITE_HORDE, "Horde Azerite",
                    BGWorldState::StateType::SCORE_HORDE, 0)
    };
}

// ============================================================================
// WORLD STATE INTERPRETATION
// ============================================================================

bool SeethingShoreScript::InterpretWorldState(int32 stateId, int32 value,
    uint32& outObjectiveId, BGObjectiveState& outState) const
{
    return TryInterpretFromCache(stateId, value, outObjectiveId, outState);
}

void SeethingShoreScript::GetScoreFromWorldStates(const std::map<int32, int32>& states,
    uint32& allianceScore, uint32& hordeScore) const
{
    allianceScore = hordeScore = 0;

    auto it = states.find(SeethingShore::WorldStates::AZERITE_ALLY);
    if (it != states.end())
        allianceScore = static_cast<uint32>(std::max(0, it->second));

    it = states.find(SeethingShore::WorldStates::AZERITE_HORDE);
    if (it != states.end())
        hordeScore = static_cast<uint32>(std::max(0, it->second));
}

// ============================================================================
// STRATEGY AND ROLE DISTRIBUTION
// ============================================================================

RoleDistribution SeethingShoreScript::GetRecommendedRoles(const StrategicDecision& decision,
    float /*scoreAdvantage*/, uint32 /*timeRemaining*/) const
{
    RoleDistribution dist;

    // Seething Shore requires highly mobile, aggressive groups
    // No static defenders since nodes disappear when captured

    switch (decision.strategy)
    {
        case BGStrategy::AGGRESSIVE:
            dist.roleCounts[BGRole::NODE_ATTACKER] = 55;
            dist.roleCounts[BGRole::ROAMER] = 30;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 15;
            dist.reasoning = "Aggressive node capture - maximize mobility";
            break;

        case BGStrategy::DEFENSIVE:
            // Even defensive play is relatively aggressive in SS
            dist.roleCounts[BGRole::NODE_DEFENDER] = 35;
            dist.roleCounts[BGRole::NODE_ATTACKER] = 40;
            dist.roleCounts[BGRole::ROAMER] = 25;
            dist.reasoning = "Defensive play - control spawns near our side";
            break;

        case BGStrategy::ALL_IN:
            dist.roleCounts[BGRole::NODE_ATTACKER] = 70;
            dist.roleCounts[BGRole::ROAMER] = 25;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 5;
            dist.reasoning = "All-in - full aggression on all nodes";
            break;

        default:  // BALANCED
            dist.roleCounts[BGRole::NODE_ATTACKER] = 45;
            dist.roleCounts[BGRole::ROAMER] = 35;
            dist.roleCounts[BGRole::NODE_DEFENDER] = 20;
            dist.reasoning = "Balanced dynamic capture";
            break;
    }

    return dist;
}

void SeethingShoreScript::AdjustStrategy(StrategicDecision& decision, float scoreAdvantage,
    uint32 /*controlledCount*/, uint32 /*totalObjectives*/, uint32 timeRemaining) const
{
    uint32 activeNodes = GetActiveNodeCount();

    // Score-based DESPERATE override with hysteresis
    // Enter DESPERATE when behind by 30%+ (scoreAdvantage < -0.3), exit when gap narrows to 15%
    SeethingShorePhase effectivePhase = m_currentPhase;
    if (m_currentPhase != SeethingShorePhase::OPENING)
    {
        if (m_lastPhase == SeethingShorePhase::DESPERATE)
        {
            // Already desperate: stay until gap narrows to half threshold
            if (scoreAdvantage < -0.15f)
                effectivePhase = SeethingShorePhase::DESPERATE;
        }
        else
        {
            // Not desperate: enter at full threshold
            if (scoreAdvantage < -0.30f)
                effectivePhase = SeethingShorePhase::DESPERATE;
        }
    }
    m_lastPhase = effectivePhase;

    // Phase-based strategy adjustment
    switch (effectivePhase)
    {
        case SeethingShorePhase::OPENING:
            ApplyOpeningPhaseStrategy(decision, ALLIANCE);  // Faction-agnostic for now
            break;

        case SeethingShorePhase::MID_GAME:
            ApplyMidGameStrategy(decision, scoreAdvantage);
            break;

        case SeethingShorePhase::LATE_GAME:
            ApplyLateGameStrategy(decision, scoreAdvantage, timeRemaining);
            break;

        case SeethingShorePhase::DESPERATE:
            ApplyDesperateStrategy(decision);
            break;
    }

    // Dynamic node adjustment
    if (activeNodes >= 3)
    {
        // Multiple nodes active - spread and capture
        if (ShouldSplitTeam())
        {
            decision.reasoning += " + split team for multiple nodes";
            decision.offenseAllocation += 10;
        }
    }

    // Contested node boost
    if (!m_contestedNodeIds.empty())
    {
        decision.reasoning += " + contested nodes - reinforce";
        decision.offenseAllocation += static_cast<uint32>(m_contestedNodeIds.size()) * 5;
    }

    // Clamp allocations
    decision.offenseAllocation = std::min(decision.offenseAllocation, static_cast<uint8>(100));
    decision.defenseAllocation = 100 - decision.offenseAllocation;

    decision.reasoning += " (dynamic spawning)";
}

// ============================================================================
// DYNAMIC NODE METHODS
// ============================================================================

bool SeethingShoreScript::IsNodeActive(uint32 nodeId) const
{
    for (const auto& node : m_activeNodes)
    {
        if (node.id == nodeId && node.active)
            return true;
    }
    return false;
}

bool SeethingShoreScript::IsZoneActive(uint32 zoneId) const
{
    for (const auto& node : m_activeNodes)
    {
        if (node.zoneId == zoneId && node.active)
            return true;
    }
    return false;
}

Position SeethingShoreScript::GetNearestActiveNode(float x, float y) const
{
    Position nearest(0, 0, 0, 0);
    float minDist = std::numeric_limits<float>::max();

    for (const auto& node : m_activeNodes)
    {
        if (!node.active)
            continue;

        float dist = SeethingShore::CalculateDistance(x, y,
            node.position.GetPositionX(), node.position.GetPositionY());

        if (dist < minDist)
        {
            minDist = dist;
            nearest = node.position;
        }
    }

    return nearest;
}

uint32 SeethingShoreScript::GetNearestActiveZone(float x, float y) const
{
    uint32 nearestZone = SeethingShore::SpawnZones::ZONE_COUNT;
    float minDist = std::numeric_limits<float>::max();

    for (const auto& node : m_activeNodes)
    {
        if (!node.active)
            continue;

        float dist = SeethingShore::CalculateDistance(x, y,
            node.position.GetPositionX(), node.position.GetPositionY());

        if (dist < minDist)
        {
            minDist = dist;
            nearestZone = node.zoneId;
        }
    }

    return nearestZone;
}

std::vector<uint32> SeethingShoreScript::GetActiveZoneIds() const
{
    std::vector<uint32> zones;
    for (const auto& node : m_activeNodes)
    {
        if (node.active)
            zones.push_back(node.zoneId);
    }
    return zones;
}

std::vector<BGPositionData> SeethingShoreScript::GetActiveZoneDefensePositions() const
{
    std::vector<BGPositionData> positions;

    for (const auto& node : m_activeNodes)
    {
        if (!node.active)
            continue;

        auto defensePos = SeethingShore::GetZoneDefensePositions(node.zoneId);
        for (uint8 i = 0; i < defensePos.size(); ++i)
        {
            std::string name = std::string(SeethingShore::GetZoneName(node.zoneId)) + " Defense " + std::to_string(i + 1);
            positions.push_back(BGPositionData(
                name,
                defensePos[i].GetPositionX(), defensePos[i].GetPositionY(),
                defensePos[i].GetPositionZ(), defensePos[i].GetOrientation(),
                BGPositionData::PositionType::DEFENSIVE_POSITION, 0, 7));
        }
    }

    return positions;
}

uint32 SeethingShoreScript::GetZonePriority(uint32 zoneId, uint32 faction) const
{
    return SeethingShore::GetZonePriorityRank(zoneId, faction);
}

float SeethingShoreScript::GetZoneStrategicValue(uint32 zoneId, bool isContested) const
{
    float baseValue = SeethingShore::IsElevatedZone(zoneId) ? 9.0f : 8.0f;

    if (isContested)
        baseValue *= SeethingShore::Strategy::CONTESTED_ZONE_VALUE;
    else
        baseValue *= SeethingShore::Strategy::UNCONTESTED_ZONE_VALUE;

    return baseValue;
}

std::vector<uint32> SeethingShoreScript::GetPrioritizedActiveZones(uint32 faction) const
{
    auto activeZones = GetActiveZoneIds();

    // Sort by priority for this faction
    std::sort(activeZones.begin(), activeZones.end(),
        [this, faction](uint32 a, uint32 b) {
            return GetZonePriority(a, faction) < GetZonePriority(b, faction);
        });

    return activeZones;
}

bool SeethingShoreScript::ShouldSplitTeam() const
{
    auto activeZones = GetActiveZoneIds();
    if (activeZones.size() < 2)
        return false;

    // Check if nodes are far apart enough to warrant splitting
    for (size_t i = 0; i < activeZones.size(); ++i)
    {
        for (size_t j = i + 1; j < activeZones.size(); ++j)
        {
            float dist = SeethingShore::GetZoneDistance(activeZones[i], activeZones[j]);
            if (dist >= SeethingShore::Strategy::SPLIT_DISTANCE_THRESHOLD)
                return true;
        }
    }

    return false;
}

uint32 SeethingShoreScript::GetRecommendedSplitCount() const
{
    uint32 activeCount = GetActiveNodeCount();
    if (activeCount <= 1)
        return 1;

    // Split into groups for each active node if team size allows
    uint32 maxGroups = SeethingShore::TEAM_SIZE / SeethingShore::Strategy::MIN_SPLIT_TEAM_SIZE;
    return std::min(activeCount, maxGroups);
}

// ============================================================================
// POSITIONAL DATA PROVIDERS
// ============================================================================

std::vector<BGPositionData> SeethingShoreScript::GetChokepoints() const
{
    std::vector<BGPositionData> positions;

    for (uint32 i = 0; i < SeethingShore::Chokepoints::COUNT; ++i)
    {
        Position pos = SeethingShore::GetChokepointPosition(i);
        positions.push_back(BGPositionData(
            SeethingShore::GetChokepointName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::CHOKEPOINT, 0, 6));
    }

    return positions;
}

std::vector<BGPositionData> SeethingShoreScript::GetSniperPositions() const
{
    std::vector<BGPositionData> positions;

    for (uint32 i = 0; i < SeethingShore::SniperSpots::COUNT; ++i)
    {
        Position pos = SeethingShore::GetSniperPosition(i);
        positions.push_back(BGPositionData(
            SeethingShore::GetSniperSpotName(i),
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::SNIPER_POSITION, 0, 8));
    }

    return positions;
}

std::vector<BGPositionData> SeethingShoreScript::GetBuffPositions() const
{
    std::vector<BGPositionData> positions;

    for (uint32 i = 0; i < SeethingShore::BuffSpots::COUNT; ++i)
    {
        Position pos = SeethingShore::GetBuffPosition(i);
        std::string name = "Buff Location " + std::to_string(i + 1);
        positions.push_back(BGPositionData(
            name,
            pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            BGPositionData::PositionType::BUFF_LOCATION, 0, 5));
    }

    return positions;
}

std::vector<BGPositionData> SeethingShoreScript::GetAmbushPositions(uint32 faction) const
{
    std::vector<BGPositionData> positions;
    auto ambushPos = SeethingShore::GetAmbushPositions(faction);

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

std::vector<Position> SeethingShoreScript::GetRotationPath(uint32 fromZone, uint32 toZone) const
{
    std::vector<Position> path;

    // Start position
    Position start = SeethingShore::GetZoneCenter(fromZone);
    path.push_back(start);

    // Check if zones are adjacent - if so, direct path
    auto adjacent = SeethingShore::GetAdjacentZones(fromZone);
    bool isAdjacent = std::find(adjacent.begin(), adjacent.end(), toZone) != adjacent.end();

    if (!isAdjacent)
    {
        // Need intermediate waypoint through center zone (7) or closest hub
        Position center = SeethingShore::GetZoneCenter(SeethingShore::SpawnZones::CENTER);
        path.push_back(center);
    }

    // End position
    Position end = SeethingShore::GetZoneCenter(toZone);
    path.push_back(end);

    return path;
}

std::vector<uint32> SeethingShoreScript::GetAdjacentZones(uint32 zoneId) const
{
    return SeethingShore::GetAdjacentZones(zoneId);
}

// ============================================================================
// PHASE AND STATE QUERIES
// ============================================================================

uint32 SeethingShoreScript::GetMatchRemainingTime() const
{
    if (m_matchElapsedTime >= SeethingShore::MAX_DURATION)
        return 0;
    return SeethingShore::MAX_DURATION - m_matchElapsedTime;
}

bool SeethingShoreScript::IsNodeContested(uint32 nodeId) const
{
    return m_contestedNodeIds.find(nodeId) != m_contestedNodeIds.end();
}

std::vector<uint32> SeethingShoreScript::GetContestedNodeIds() const
{
    return std::vector<uint32>(m_contestedNodeIds.begin(), m_contestedNodeIds.end());
}

std::vector<uint32> SeethingShoreScript::GetTickPointsTable() const
{
    // Seething Shore uses flat points per capture
    return { SeethingShore::AZERITE_PER_NODE };
}

// ============================================================================
// INTERNAL UPDATE METHODS
// ============================================================================

void SeethingShoreScript::UpdateActiveNodes()
{
    // Ensure we have MAX_ACTIVE_NODES active
    while (m_activeNodes.size() < SeethingShore::MAX_ACTIVE_NODES && m_nodeSpawnCooldown == 0)
    {
        SpawnNewNode();
        m_nodeSpawnCooldown = SeethingShore::NODE_RESPAWN_TIME;
    }
}

void SeethingShoreScript::SpawnNewNode()
{
    // Find available zones (not currently active and not recently captured)
    std::vector<uint32> availableZones;
    uint32 currentTime = getMSTime();

    for (uint32 i = 0; i < SeethingShore::SpawnZones::ZONE_COUNT; ++i)
    {
        // Check if zone is already active
        bool zoneUsed = false;
        for (const auto& node : m_activeNodes)
        {
            if (node.zoneId == i)
            {
                zoneUsed = true;
                break;
            }
        }

        // Check respawn cooldown
        auto captureIt = m_zoneCaptureTimestamps.find(i);
        if (captureIt != m_zoneCaptureTimestamps.end())
        {
            if (currentTime - captureIt->second < SeethingShore::NODE_RESPAWN_TIME)
                zoneUsed = true;
        }

        if (!zoneUsed)
            availableZones.push_back(i);
    }

    if (availableZones.empty())
    {
        TC_LOG_WARN("bg.playerbot", "SeethingShoreScript::SpawnNewNode - No available zones for spawning!");
        return;
    }

    // Random zone selection
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(availableZones.size()) - 1);
    uint32 zoneId = availableZones[dis(gen)];

    // Create node
    SeethingShore::AzeriteNode node;
    node.id = m_nextNodeId++;
    node.zoneId = zoneId;
    node.position = SeethingShore::GetZoneCenter(zoneId);
    node.active = true;
    node.spawnTime = currentTime;
    node.capturedByFaction = 0;
    node.contested = false;
    node.captureProgress = 0.0f;

    m_activeNodes.push_back(node);

    TC_LOG_DEBUG("bg.playerbot", "SeethingShoreScript::SpawnNewNode - Spawned node %u in zone %s",
        node.id, SeethingShore::GetZoneName(zoneId));
}

void SeethingShoreScript::RemoveCapturedNode(uint32 nodeId)
{
    for (auto it = m_activeNodes.begin(); it != m_activeNodes.end(); ++it)
    {
        if (it->id == nodeId)
        {
            // Record capture timestamp for respawn cooldown
            m_zoneCaptureTimestamps[it->zoneId] = getMSTime();
            m_activeNodes.erase(it);
            m_contestedNodeIds.erase(nodeId);

            TC_LOG_DEBUG("bg.playerbot", "SeethingShoreScript::RemoveCapturedNode - Removed node %u", nodeId);
            return;
        }
    }
}

void SeethingShoreScript::UpdatePhase(uint32 timeRemaining)
{
    if (timeRemaining > (SeethingShore::MAX_DURATION - SeethingShore::Strategy::OPENING_PHASE))
        m_currentPhase = SeethingShorePhase::OPENING;
    else if (timeRemaining > (SeethingShore::MAX_DURATION - SeethingShore::Strategy::MID_GAME_END))
        m_currentPhase = SeethingShorePhase::MID_GAME;
    else
        m_currentPhase = SeethingShorePhase::LATE_GAME;

    // Desperate phase overrides time-based phase when significantly behind
    // (This would require score tracking which is handled by the coordinator)
}

void SeethingShoreScript::UpdateContestedStatus()
{
    // Contested status is updated via events, but we clean up stale entries here
    // (In a real implementation, this would check player proximity to nodes)
}

// ============================================================================
// INTERNAL STRATEGY HELPERS
// ============================================================================

void SeethingShoreScript::ApplyOpeningPhaseStrategy(StrategicDecision& decision, uint32 /*faction*/) const
{
    decision.strategy = BGStrategy::AGGRESSIVE;
    decision.reasoning = "Opening phase - race to capture first nodes";
    decision.offenseAllocation = 75;
    decision.defenseAllocation = 25;
}

void SeethingShoreScript::ApplyMidGameStrategy(StrategicDecision& decision, float scoreAdvantage) const
{
    if (scoreAdvantage > 0.15f)
    {
        decision.strategy = BGStrategy::BALANCED;
        decision.reasoning = "Mid-game leading - maintain pressure";
        decision.offenseAllocation = 55;
        decision.defenseAllocation = 45;
    }
    else if (scoreAdvantage < -0.15f)
    {
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.reasoning = "Mid-game trailing - increase aggression";
        decision.offenseAllocation = 70;
        decision.defenseAllocation = 30;
    }
    else
    {
        decision.strategy = BGStrategy::BALANCED;
        decision.reasoning = "Mid-game tied - balanced approach";
        decision.offenseAllocation = 60;
        decision.defenseAllocation = 40;
    }
}

void SeethingShoreScript::ApplyLateGameStrategy(StrategicDecision& decision, float scoreAdvantage,
    uint32 timeRemaining) const
{
    if (scoreAdvantage > 0.2f)
    {
        // Winning - protect lead
        decision.strategy = BGStrategy::DEFENSIVE;
        decision.reasoning = "Late game ahead - protect lead";
        decision.offenseAllocation = 40;
        decision.defenseAllocation = 60;
    }
    else if (scoreAdvantage < -0.2f && timeRemaining < 180000)
    {
        // Losing with little time - desperate
        decision.strategy = BGStrategy::ALL_IN;
        decision.reasoning = "Late game behind - all-in!";
        decision.offenseAllocation = 90;
        decision.defenseAllocation = 10;
    }
    else
    {
        decision.strategy = BGStrategy::AGGRESSIVE;
        decision.reasoning = "Late game close - aggressive push";
        decision.offenseAllocation = 70;
        decision.defenseAllocation = 30;
    }
}

void SeethingShoreScript::ApplyDesperateStrategy(StrategicDecision& decision) const
{
    decision.strategy = BGStrategy::ALL_IN;
    decision.reasoning = "Desperate - maximum aggression";
    decision.offenseAllocation = 95;
    decision.defenseAllocation = 5;
}

std::vector<uint32> SeethingShoreScript::CalculateBestTargetZones(uint32 faction, uint32 count) const
{
    auto prioritized = GetPrioritizedActiveZones(faction);

    // Return up to 'count' highest priority zones
    if (prioritized.size() > count)
        prioritized.resize(count);

    return prioritized;
}

// ============================================================================
// RUNTIME BEHAVIOR - ExecuteStrategy
// ============================================================================

bool SeethingShoreScript::ExecuteStrategy(::Player* player)
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
            "[SS] {} PRIORITY 0: reinforcing contested node {}",
            player->GetName(), nodeData.name);
        DefendNode(player, reinforceNode);
        return true;
    }

    // =========================================================================
    // PRIORITY 1: Nearby active node (<30yd) capturable -> capture
    // =========================================================================
    for (const auto& node : m_activeNodes)
    {
        if (!node.active)
            continue;

        float dist = player->GetExactDist(&node.position);
        if (dist < 30.0f)
        {
            // Check if this node is capturable (not already captured by us)
            if (node.capturedByFaction != faction)
            {
                TC_LOG_DEBUG("playerbots.bg.script",
                    "[SS] {} PRIORITY 1: capturing active node in zone {} (dist={:.0f})",
                    player->GetName(), SeethingShore::GetZoneName(node.zoneId), dist);

                if (dist < 8.0f)
                    TryInteractWithGameObject(player, 29 /*GAMEOBJECT_TYPE_CAPTURE_POINT*/, 10.0f);
                else
                    BotMovementUtil::MoveToPosition(player, node.position);

                return true;
            }
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
            "[SS] {} PRIORITY 2: defending contested node {}",
            player->GetName(), nodeData.name);
        DefendNode(player, threatened);
        return true;
    }

    // =========================================================================
    // PRIORITY 2.5: Recently spawned node -> nearest 3-4 bots immediately divert
    // =========================================================================
    if (m_recentlySpawnedNode.spawnTime > 0 &&
        getMSTime() - m_recentlySpawnedNode.spawnTime < RECENT_SPAWN_PRIORITY_DURATION)
    {
        Position spawnPos = SeethingShore::GetZoneCenter(m_recentlySpawnedNode.zoneId);
        float distToSpawn = player->GetExactDist(&spawnPos);

        // Nearest 3-4 bots divert (approximated by distance threshold or GUID hash)
        // Use GUID hash mod 3 to ensure ~30% of team (3 of 10) divert
        uint32 spawnSlot = player->GetGUID().GetCounter() % 3;
        if (spawnSlot == 0 || distToSpawn < 50.0f)
        {
            TC_LOG_DEBUG("playerbots.bg.script",
                "[SS] {} PRIORITY 2.5: rushing newly spawned node in zone {} (dist={:.0f})",
                player->GetName(), SeethingShore::GetZoneName(m_recentlySpawnedNode.zoneId), distToSpawn);

            if (distToSpawn < 8.0f)
                TryInteractWithGameObject(player, 29 /*GAMEOBJECT_TYPE_CAPTURE_POINT*/, 10.0f);
            else
                BotMovementUtil::MoveToPosition(player, spawnPos);

            // Attack enemies near the spawn
            ::Player* enemy = FindNearestEnemyPlayer(player, 15.0f);
            if (enemy)
                EngageTarget(player, enemy);

            return true;
        }
    }

    // =========================================================================
    // PRIORITY 3: GUID split: 50% capture nearest active unclaimed node, 50% defend
    // =========================================================================
    uint32 dutySlot = player->GetGUID().GetCounter() % 10;
    auto activeZones = GetActiveZoneIds();

    if (dutySlot < 5 && !activeZones.empty())
    {
        // 50% -> capture nearest active unclaimed node
        Position nearestNode = GetNearestActiveNode(
            player->GetPositionX(), player->GetPositionY());

        if (nearestNode.GetPositionX() != 0 || nearestNode.GetPositionY() != 0)
        {
            float dist = player->GetExactDist(&nearestNode);

            TC_LOG_DEBUG("playerbots.bg.script",
                "[SS] {} PRIORITY 3: moving to capture active node (dist={:.0f})",
                player->GetName(), dist);

            if (::Player* enemy = FindNearestEnemyPlayer(player, 15.0f))
            {
                EngageTarget(player, enemy);
            }
            else if (dist < 8.0f)
            {
                TryInteractWithGameObject(player, 29 /*GAMEOBJECT_TYPE_CAPTURE_POINT*/, 10.0f);
            }
            else
            {
                BotMovementUtil::MoveToPosition(player, nearestNode);
            }
            return true;
        }
    }
    else
    {
        // 50% -> defend an active node or engage enemies
        if (::Player* enemy = FindNearestEnemyPlayer(player, 30.0f))
        {
            TC_LOG_DEBUG("playerbots.bg.script",
                "[SS] {} PRIORITY 3: defending - engaging enemy {} (dist={:.0f})",
                player->GetName(), enemy->GetName(),
                player->GetExactDist(enemy));
            EngageTarget(player, enemy);
            return true;
        }

        // Defend nearest active node
        if (!activeZones.empty())
        {
            uint32 defZone = activeZones[player->GetGUID().GetCounter() % activeZones.size()];
            Position zoneCenter = SeethingShore::GetZoneCenter(defZone);

            TC_LOG_DEBUG("playerbots.bg.script",
                "[SS] {} PRIORITY 3: defending zone {}",
                player->GetName(), SeethingShore::GetZoneName(defZone));
            PatrolAroundPosition(player, zoneCenter, 5.0f, 15.0f);
            return true;
        }
    }

    // =========================================================================
    // PRIORITY 4: Fallback -> patrol between active nodes
    // =========================================================================
    {
        if (!activeZones.empty())
        {
            // Pick a random active zone based on GUID to spread out
            uint32 targetZone = activeZones[player->GetGUID().GetCounter() % activeZones.size()];
            Position zoneCenter = SeethingShore::GetZoneCenter(targetZone);

            TC_LOG_DEBUG("playerbots.bg.script",
                "[SS] {} PRIORITY 4: patrolling between active nodes (zone {})",
                player->GetName(), SeethingShore::GetZoneName(targetZone));
            BotMovementUtil::MoveToPosition(player, zoneCenter);
            return true;
        }

        // No active nodes at all - patrol center of map
        auto chokepoints = GetChokepoints();
        if (!chokepoints.empty())
        {
            uint32 chokeIdx = player->GetGUID().GetCounter() % chokepoints.size();
            Position chokePos(chokepoints[chokeIdx].x, chokepoints[chokeIdx].y,
                              chokepoints[chokeIdx].z, 0);
            PatrolAroundPosition(player, chokePos, 5.0f, 15.0f);
            return true;
        }
    }

    return false;
}

} // namespace Playerbot::Coordination::Battleground
