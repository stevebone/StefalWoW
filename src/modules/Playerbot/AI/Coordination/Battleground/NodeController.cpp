/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "NodeController.h"
#include "BattlegroundCoordinator.h"
#include "Player.h"
#include "Group.h"
#include "Log.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"

namespace Playerbot {

// ============================================================================
// CONSTRUCTOR
// ============================================================================

NodeController::NodeController(BattlegroundCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void NodeController::Initialize()
{
    Reset();

    TC_LOG_DEBUG("playerbots.bg", "NodeController::Initialize - Initialized for node-based battleground");
}

void NodeController::Update(uint32 diff)
{
    // Update assignments
    UpdateAssignments();

    // Process reinforcement requests
    ProcessReinforcementRequests();
}

void NodeController::Reset()
{
    _nodeAssignments.clear();
    _playerToNode.clear();
    _reinforcementRequests.clear();

    TC_LOG_DEBUG("playerbots.bg", "NodeController::Reset - Reset all node state");
}

// ============================================================================
// NODE ACCESS
// ============================================================================

std::vector<uint32> NodeController::GetAllNodeIds() const
{
    std::vector<uint32> nodeIds;

    for (const auto& objective : _coordinator->GetObjectives())
    {
        if (objective.type == ObjectiveType::CONTROL_POINT ||
            objective.type == ObjectiveType::CAPTURABLE)
        {
            nodeIds.push_back(objective.objectiveId);
        }
    }

    return nodeIds;
}

std::vector<uint32> NodeController::GetFriendlyNodeIds() const
{
    std::vector<uint32> nodeIds;

    for (const auto& objective : _coordinator->GetObjectives())
    {
        if ((objective.type == ObjectiveType::CONTROL_POINT ||
             objective.type == ObjectiveType::CAPTURABLE) &&
            objective.state == BGObjectiveState::CONTROLLED_FRIENDLY)
        {
            nodeIds.push_back(objective.objectiveId);
        }
    }

    return nodeIds;
}

std::vector<uint32> NodeController::GetEnemyNodeIds() const
{
    std::vector<uint32> nodeIds;

    for (const auto& objective : _coordinator->GetObjectives())
    {
        if ((objective.type == ObjectiveType::CONTROL_POINT ||
             objective.type == ObjectiveType::CAPTURABLE) &&
            objective.state == BGObjectiveState::CONTROLLED_ENEMY)
        {
            nodeIds.push_back(objective.objectiveId);
        }
    }

    return nodeIds;
}

std::vector<uint32> NodeController::GetNeutralNodeIds() const
{
    std::vector<uint32> nodeIds;

    for (const auto& objective : _coordinator->GetObjectives())
    {
        if ((objective.type == ObjectiveType::CONTROL_POINT ||
             objective.type == ObjectiveType::CAPTURABLE) &&
            objective.state == BGObjectiveState::NEUTRAL)
        {
            nodeIds.push_back(objective.objectiveId);
        }
    }

    return nodeIds;
}

std::vector<uint32> NodeController::GetContestedNodeIds() const
{
    std::vector<uint32> nodeIds;

    for (const auto& objective : _coordinator->GetObjectives())
    {
        if ((objective.type == ObjectiveType::CONTROL_POINT ||
             objective.type == ObjectiveType::CAPTURABLE) &&
            objective.state == BGObjectiveState::CONTESTED)
        {
            nodeIds.push_back(objective.objectiveId);
        }
    }

    return nodeIds;
}

// ============================================================================
// NODE ASSIGNMENT
// ============================================================================

void NodeController::AssignDefender(uint32 nodeId, ObjectGuid player)
{
    // Create assignment if doesn't exist
    if (_nodeAssignments.find(nodeId) == _nodeAssignments.end())
    {
        NodeAssignment assignment;
        assignment.nodeId = nodeId;
        _nodeAssignments[nodeId] = assignment;
    }

    NodeAssignment& assignment = _nodeAssignments[nodeId];

    // Check if already defending this node
    if (std::find(assignment.defenders.begin(), assignment.defenders.end(), player) != assignment.defenders.end())
        return;

    // Remove from previous assignment
    UnassignFromNode(player);

    // Add to defenders
    assignment.defenders.push_back(player);
    _playerToNode[player] = nodeId;

    TC_LOG_DEBUG("playerbots.bg", "NodeController::AssignDefender - Assigned defender to node %u, total: %zu",
        nodeId, assignment.defenders.size());
}

void NodeController::AssignAttacker(uint32 nodeId, ObjectGuid player)
{
    // Create assignment if doesn't exist
    if (_nodeAssignments.find(nodeId) == _nodeAssignments.end())
    {
        NodeAssignment assignment;
        assignment.nodeId = nodeId;
        _nodeAssignments[nodeId] = assignment;
    }

    NodeAssignment& assignment = _nodeAssignments[nodeId];

    // Check if already attacking this node
    if (std::find(assignment.attackers.begin(), assignment.attackers.end(), player) != assignment.attackers.end())
        return;

    // Remove from previous assignment
    UnassignFromNode(player);

    // Add to attackers
    assignment.attackers.push_back(player);
    _playerToNode[player] = nodeId;

    TC_LOG_DEBUG("playerbots.bg", "NodeController::AssignAttacker - Assigned attacker to node %u, total: %zu",
        nodeId, assignment.attackers.size());
}

void NodeController::UnassignFromNode(ObjectGuid player)
{
    auto it = _playerToNode.find(player);
    if (it == _playerToNode.end())
        return;

    uint32 nodeId = it->second;
    _playerToNode.erase(it);

    auto nodeIt = _nodeAssignments.find(nodeId);
    if (nodeIt == _nodeAssignments.end())
        return;

    NodeAssignment& assignment = nodeIt->second;

    // Remove from defenders
    auto defIt = std::remove(assignment.defenders.begin(), assignment.defenders.end(), player);
    if (defIt != assignment.defenders.end())
        assignment.defenders.erase(defIt, assignment.defenders.end());

    // Remove from attackers
    auto atkIt = std::remove(assignment.attackers.begin(), assignment.attackers.end(), player);
    if (atkIt != assignment.attackers.end())
        assignment.attackers.erase(atkIt, assignment.attackers.end());
}

NodeAssignment* NodeController::GetNodeAssignment(uint32 nodeId)
{
    auto it = _nodeAssignments.find(nodeId);
    if (it == _nodeAssignments.end())
        return nullptr;

    return &it->second;
}

const NodeAssignment* NodeController::GetNodeAssignment(uint32 nodeId) const
{
    auto it = _nodeAssignments.find(nodeId);
    if (it == _nodeAssignments.end())
        return nullptr;

    return &it->second;
}

uint32 NodeController::GetPlayerAssignment(ObjectGuid player) const
{
    auto it = _playerToNode.find(player);
    if (it == _playerToNode.end())
        return 0;

    return it->second;
}

bool NodeController::IsAssignedTo(ObjectGuid player, uint32 nodeId) const
{
    auto it = _playerToNode.find(player);
    return it != _playerToNode.end() && it->second == nodeId;
}

// ============================================================================
// DEFENSE MANAGEMENT
// ============================================================================

void NodeController::SetDefenderTarget(uint32 nodeId, uint32 count)
{
    if (_nodeAssignments.find(nodeId) == _nodeAssignments.end())
    {
        NodeAssignment assignment;
        assignment.nodeId = nodeId;
        _nodeAssignments[nodeId] = assignment;
    }

    _nodeAssignments[nodeId].targetDefenderCount = count;
}

uint32 NodeController::GetDefenderTarget(uint32 nodeId) const
{
    auto it = _nodeAssignments.find(nodeId);
    if (it == _nodeAssignments.end())
        return _defaultDefenderCount;

    return it->second.targetDefenderCount;
}

uint32 NodeController::GetDefenderCount(uint32 nodeId) const
{
    auto it = _nodeAssignments.find(nodeId);
    if (it == _nodeAssignments.end())
        return 0;

    return static_cast<uint32>(it->second.defenders.size());
}

bool NodeController::NeedsMoreDefenders(uint32 nodeId) const
{
    uint32 current = GetDefenderCount(nodeId);
    uint32 target = GetDefenderTarget(nodeId);

    // Contested nodes need more defenders
    auto contested = GetContestedNodeIds();
    if (std::find(contested.begin(), contested.end(), nodeId) != contested.end())
        target = std::max(target, _contestedDefenderCount);

    return current < target;
}

bool NodeController::HasTooManyDefenders(uint32 nodeId) const
{
    uint32 current = GetDefenderCount(nodeId);
    uint32 target = GetDefenderTarget(nodeId);

    // Allow 1 extra defender as buffer
    return current > target + 1;
}

ObjectGuid NodeController::GetBestDefenderCandidate(uint32 nodeId) const
{
    ObjectGuid bestCandidate;
    float bestScore = 0.0f;

    for (ObjectGuid guid : _coordinator->GetFriendlyPlayers())
    {
        if (!IsPlayerAvailable(guid))
            continue;

        // Skip if already assigned somewhere
        if (_playerToNode.find(guid) != _playerToNode.end())
            continue;

        float distance = GetDistanceToNode(guid, nodeId);
        float distanceScore = std::max(0.0f, 100.0f - distance);

        // Prefer tankier classes for defense
        Player* player = _coordinator->GetPlayer(guid);
        float classScore = 50.0f;
        if (player)
        {
            uint8 pClass = player->GetClass();
            // Paladin, Warrior, DK, DH preferred for defense
            if (pClass == 2 || pClass == 1 || pClass == 6 || pClass == 12)
                classScore = 80.0f;
        }

        float score = distanceScore * 0.7f + classScore * 0.3f;

        if (score > bestScore)
        {
            bestScore = score;
            bestCandidate = guid;
        }
    }

    return bestCandidate;
}

// ============================================================================
// ATTACK MANAGEMENT
// ============================================================================

void NodeController::SetAttackerTarget(uint32 nodeId, uint32 count)
{
    if (_nodeAssignments.find(nodeId) == _nodeAssignments.end())
    {
        NodeAssignment assignment;
        assignment.nodeId = nodeId;
        _nodeAssignments[nodeId] = assignment;
    }

    _nodeAssignments[nodeId].targetAttackerCount = count;
}

uint32 NodeController::GetAttackerTarget(uint32 nodeId) const
{
    auto it = _nodeAssignments.find(nodeId);
    if (it == _nodeAssignments.end())
        return _attackGroupSize;

    return it->second.targetAttackerCount;
}

uint32 NodeController::GetAttackerCount(uint32 nodeId) const
{
    auto it = _nodeAssignments.find(nodeId);
    if (it == _nodeAssignments.end())
        return 0;

    return static_cast<uint32>(it->second.attackers.size());
}

bool NodeController::NeedsMoreAttackers(uint32 nodeId) const
{
    return GetAttackerCount(nodeId) < GetAttackerTarget(nodeId);
}

ObjectGuid NodeController::GetBestAttackerCandidate(uint32 nodeId) const
{
    ObjectGuid bestCandidate;
    float bestScore = 0.0f;

    for (ObjectGuid guid : _coordinator->GetFriendlyPlayers())
    {
        if (!IsPlayerAvailable(guid))
            continue;

        // Skip if already assigned somewhere
        if (_playerToNode.find(guid) != _playerToNode.end())
            continue;

        float distance = GetDistanceToNode(guid, nodeId);
        float distanceScore = std::max(0.0f, 100.0f - distance);

        // Prefer DPS classes for attack
        Player* player = _coordinator->GetPlayer(guid);
        float classScore = 50.0f;
        if (player)
        {
            uint8 pClass = player->GetClass();
            // Rogue, Warrior, DK, DH, Hunter, Mage, Warlock preferred for attack
            if (pClass == 4 || pClass == 1 || pClass == 6 || pClass == 12 ||
                pClass == 3 || pClass == 8 || pClass == 9)
            {
                classScore = 80.0f;
            }
        }

        float score = distanceScore * 0.6f + classScore * 0.4f;

        if (score > bestScore)
        {
            bestScore = score;
            bestCandidate = guid;
        }
    }

    return bestCandidate;
}

// ============================================================================
// THREAT ASSESSMENT
// ============================================================================

NodeThreatInfo NodeController::AssessNodeThreat(uint32 nodeId) const
{
    NodeThreatInfo threat;
    threat.nodeId = nodeId;

    // Count nearby enemies and allies
    threat.nearbyEnemies = CountNearbyEnemies(nodeId);
    threat.nearbyAllies = CountNearbyAllies(nodeId);

    // Calculate strength
    threat.enemyStrength = CalculateEnemyStrength(nodeId);
    threat.allyStrength = CalculateAllyStrength(nodeId);

    // Determine if under attack
    threat.isUnderAttack = (threat.nearbyEnemies > 0 && threat.enemyStrength > threat.allyStrength * 0.5f);

    // Check if being lost
    const BGObjective* objective = _coordinator->GetObjective(nodeId);
    if (objective && objective->state == BGObjectiveState::CONTROLLED_FRIENDLY)
    {
        if (threat.isUnderAttack && threat.enemyStrength > threat.allyStrength)
        {
            threat.isBeingLost = true;
            // Estimate time until lost based on strength differential
            float differential = threat.enemyStrength - threat.allyStrength;
            threat.timeUntilLost = static_cast<uint32>(60000.0f / (differential + 1.0f));
        }
    }

    return threat;
}

std::vector<NodeThreatInfo> NodeController::AssessAllNodeThreats() const
{
    std::vector<NodeThreatInfo> threats;

    for (uint32 nodeId : GetFriendlyNodeIds())
    {
        threats.push_back(AssessNodeThreat(nodeId));
    }

    // Sort by threat level (most threatened first)
    std::sort(threats.begin(), threats.end(),
        [](const NodeThreatInfo& a, const NodeThreatInfo& b)
        {
            // Nodes being lost are highest priority
            if (a.isBeingLost != b.isBeingLost)
                return a.isBeingLost;

            // Then by enemy presence
            if (a.isUnderAttack != b.isUnderAttack)
                return a.isUnderAttack;

            // Then by enemy strength
            return a.enemyStrength > b.enemyStrength;
        });

    return threats;
}

bool NodeController::IsNodeUnderAttack(uint32 nodeId) const
{
    NodeThreatInfo threat = AssessNodeThreat(nodeId);
    return threat.isUnderAttack;
}

bool NodeController::IsNodeSafe(uint32 nodeId) const
{
    NodeThreatInfo threat = AssessNodeThreat(nodeId);
    return threat.nearbyEnemies == 0;
}

uint32 NodeController::GetMostThreatenedNode() const
{
    auto threats = AssessAllNodeThreats();
    if (threats.empty())
        return 0;

    // First threatened node (already sorted by threat)
    for (const auto& threat : threats)
    {
        if (threat.isBeingLost || threat.isUnderAttack)
            return threat.nodeId;
    }

    return 0;
}

uint32 NodeController::GetLeastDefendedNode() const
{
    uint32 leastDefended = 0;
    uint32 minDefenders = std::numeric_limits<uint32>::max();

    for (uint32 nodeId : GetFriendlyNodeIds())
    {
        uint32 defenders = GetDefenderCount(nodeId);
        if (defenders < minDefenders)
        {
            minDefenders = defenders;
            leastDefended = nodeId;
        }
    }

    return leastDefended;
}

// ============================================================================
// REINFORCEMENT
// ============================================================================

void NodeController::RequestReinforcements(uint32 nodeId, uint32 count)
{
    // Check if already have pending request
    for (auto& request : _reinforcementRequests)
    {
        if (request.nodeId == nodeId)
        {
            request.count = std::max(request.count, count);
            return;
        }
    }

    ReinforcementRequest request;
    request.nodeId = nodeId;
    request.count = count;
    request.requestTime = 0;

    _reinforcementRequests.push_back(request);

    TC_LOG_DEBUG("playerbots.bg", "NodeController::RequestReinforcements - Requested %u reinforcements for node %u",
        count, nodeId);

    // Broadcast defensive alert so group members activate defensives for the threatened node
    auto friendlyPlayers = _coordinator->GetFriendlyPlayers();
    if (!friendlyPlayers.empty())
    {
        Player* leader = _coordinator->GetPlayer(friendlyPlayers.front());
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            BotMessage msg = BotMessage::CommandUseDefensives(leader->GetGUID(), groupGuid);
            sBotMessageBus->Publish(msg);
        }
    }
}

void NodeController::CancelReinforcementRequest(uint32 nodeId)
{
    auto it = std::remove_if(_reinforcementRequests.begin(), _reinforcementRequests.end(),
        [nodeId](const ReinforcementRequest& r) { return r.nodeId == nodeId; });

    if (it != _reinforcementRequests.end())
    {
        _reinforcementRequests.erase(it, _reinforcementRequests.end());
        TC_LOG_DEBUG("playerbots.bg", "NodeController::CancelReinforcementRequest - Cancelled request for node %u",
            nodeId);
    }
}

bool NodeController::HasPendingReinforcementRequest(uint32 nodeId) const
{
    for (const auto& request : _reinforcementRequests)
    {
        if (request.nodeId == nodeId)
            return true;
    }
    return false;
}

std::vector<uint32> NodeController::GetNodesPendingReinforcement() const
{
    std::vector<uint32> nodes;
    nodes.reserve(_reinforcementRequests.size());

    for (const auto& request : _reinforcementRequests)
        nodes.push_back(request.nodeId);

    return nodes;
}

ObjectGuid NodeController::GetClosestAvailableReinforcement(uint32 nodeId) const
{
    ObjectGuid closest;
    float closestDistance = std::numeric_limits<float>::max();

    for (ObjectGuid guid : _coordinator->GetFriendlyPlayers())
    {
        if (!IsPlayerAvailable(guid))
            continue;

        // Skip if assigned to a more critical node
        auto assignIt = _playerToNode.find(guid);
        if (assignIt != _playerToNode.end())
        {
            // Check if current assignment is under attack
            if (IsNodeUnderAttack(assignIt->second))
                continue;
        }

        float distance = GetDistanceToNode(guid, nodeId);
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closest = guid;
        }
    }

    return closest;
}

// ============================================================================
// CAPTURE COORDINATION
// ============================================================================

bool NodeController::ShouldCaptureNode(uint32 nodeId) const
{
    return IsSafeToCapture(nodeId);
}

bool NodeController::IsSafeToCapture(uint32 nodeId) const
{
    // Check for nearby enemies
    uint32 enemies = CountNearbyEnemies(nodeId);
    if (enemies > 0)
        return false;

    // Check ally presence
    uint32 allies = CountNearbyAllies(nodeId);
    return allies > 0;
}

uint32 NodeController::GetCaptureProgress(uint32 nodeId) const
{
    const BGObjective* objective = _coordinator->GetObjective(nodeId);
    if (!objective)
        return 0;

    return objective->captureProgress;
}

uint32 NodeController::GetTimeUntilCapture(uint32 nodeId) const
{
    uint32 progress = GetCaptureProgress(nodeId);
    if (progress >= 100)
        return 0;

    // Assume ~1% per second baseline
    return (100 - progress) * 1000;
}

ObjectGuid NodeController::GetCapturingPlayer(uint32 nodeId) const
{
    // Find first player at node who can capture
    const NodeAssignment* assignment = GetNodeAssignment(nodeId);
    if (!assignment)
        return ObjectGuid();

    // Return first attacker at the node
    if (!assignment->attackers.empty())
        return assignment->attackers.front();

    return ObjectGuid();
}

// ============================================================================
// OPTIMAL NODE SELECTION
// ============================================================================

uint32 NodeController::GetBestNodeToAttack() const
{
    uint32 bestNode = 0;
    float bestScore = 0.0f;

    for (uint32 nodeId : GetEnemyNodeIds())
    {
        float score = ScoreNodeForAttack(nodeId);
        if (score > bestScore)
        {
            bestScore = score;
            bestNode = nodeId;
        }
    }

    // Also consider neutral nodes
    for (uint32 nodeId : GetNeutralNodeIds())
    {
        float score = ScoreNodeForAttack(nodeId);
        if (score > bestScore)
        {
            bestScore = score;
            bestNode = nodeId;
        }
    }

    return bestNode;
}

uint32 NodeController::GetBestNodeToDefend() const
{
    uint32 bestNode = 0;
    float bestScore = 0.0f;

    for (uint32 nodeId : GetFriendlyNodeIds())
    {
        float score = ScoreNodeForDefense(nodeId);
        if (score > bestScore)
        {
            bestScore = score;
            bestNode = nodeId;
        }
    }

    return bestNode;
}

uint32 NodeController::GetBestNodeToReinforce() const
{
    // Prioritize nodes being lost
    uint32 threatened = GetMostThreatenedNode();
    if (threatened != 0)
        return threatened;

    // Then nodes needing defenders
    return GetLeastDefendedNode();
}

float NodeController::ScoreNodeForAttack(uint32 nodeId) const
{
    float score = 0.0f;

    // Strategic importance
    score += ScoreStrategicImportance(nodeId) * 0.3f;

    // Contestability (weak defense)
    score += ScoreContestability(nodeId) * 0.3f;

    // Resource value
    score += ScoreResourceValue(nodeId) * 0.2f;

    // Current attacker presence
    uint32 attackers = GetAttackerCount(nodeId);
    if (attackers > 0)
        score += 20.0f; // Bonus for reinforcing existing attack

    return score;
}

float NodeController::ScoreNodeForDefense(uint32 nodeId) const
{
    float score = 0.0f;

    // Strategic importance
    score += ScoreStrategicImportance(nodeId) * 0.3f;

    // Defensibility
    score += ScoreDefensibility(nodeId) * 0.2f;

    // Threat level
    NodeThreatInfo threat = AssessNodeThreat(nodeId);
    if (threat.isBeingLost)
        score += 50.0f;
    else if (threat.isUnderAttack)
        score += 30.0f;

    // Current defender count
    if (GetDefenderCount(nodeId) == 0)
        score += 20.0f; // High priority for undefended nodes

    return score;
}

// ============================================================================
// STATISTICS
// ============================================================================

uint32 NodeController::GetTotalFriendlyNodes() const
{
    return static_cast<uint32>(GetFriendlyNodeIds().size());
}

uint32 NodeController::GetTotalEnemyNodes() const
{
    return static_cast<uint32>(GetEnemyNodeIds().size());
}

float NodeController::GetNodeControlRatio() const
{
    uint32 friendly = GetTotalFriendlyNodes();
    uint32 enemy = GetTotalEnemyNodes();
    uint32 total = friendly + enemy + static_cast<uint32>(GetNeutralNodeIds().size());

    if (total == 0)
        return 0.5f;

    return static_cast<float>(friendly) / static_cast<float>(total);
}

// ============================================================================
// THREAT CALCULATION (PRIVATE)
// ============================================================================

float NodeController::CalculateEnemyStrength(uint32 nodeId) const
{
    float strength = 0.0f;

    const BGObjective* objective = _coordinator->GetObjective(nodeId);
    if (!objective)
        return strength;

    for (ObjectGuid guid : _coordinator->GetEnemyPlayers())
    {
        Player* player = _coordinator->GetPlayer(guid);
        if (!player || !player->IsAlive())
            continue;

        float distance = player->GetDistance(objective->position.x, objective->position.y, objective->position.z);
        if (distance <= _threatDetectionRadius)
        {
            // Base strength 1.0 per player
            float playerStrength = 1.0f;

            // Adjust by health
            playerStrength *= player->GetHealthPct() / 100.0f;

            // Closer = more dangerous
            playerStrength *= 1.0f + (1.0f - distance / _threatDetectionRadius);

            strength += playerStrength;
        }
    }

    return strength;
}

float NodeController::CalculateAllyStrength(uint32 nodeId) const
{
    float strength = 0.0f;

    const BGObjective* objective = _coordinator->GetObjective(nodeId);
    if (!objective)
        return strength;

    for (ObjectGuid guid : _coordinator->GetFriendlyPlayers())
    {
        Player* player = _coordinator->GetPlayer(guid);
        if (!player || !player->IsAlive())
            continue;

        float distance = player->GetDistance(objective->position.x, objective->position.y, objective->position.z);
        if (distance <= _threatDetectionRadius)
        {
            float playerStrength = 1.0f;
            playerStrength *= player->GetHealthPct() / 100.0f;
            playerStrength *= 1.0f + (1.0f - distance / _threatDetectionRadius);
            strength += playerStrength;
        }
    }

    return strength;
}

uint32 NodeController::CountNearbyEnemies(uint32 nodeId) const
{
    uint32 count = 0;

    const BGObjective* objective = _coordinator->GetObjective(nodeId);
    if (!objective)
        return count;

    for (ObjectGuid guid : _coordinator->GetEnemyPlayers())
    {
        Player* player = _coordinator->GetPlayer(guid);
        if (!player || !player->IsAlive())
            continue;

        float distance = player->GetDistance(objective->position.x, objective->position.y, objective->position.z);
        if (distance <= _threatDetectionRadius)
            count++;
    }

    return count;
}

uint32 NodeController::CountNearbyAllies(uint32 nodeId) const
{
    uint32 count = 0;

    const BGObjective* objective = _coordinator->GetObjective(nodeId);
    if (!objective)
        return count;

    for (ObjectGuid guid : _coordinator->GetFriendlyPlayers())
    {
        Player* player = _coordinator->GetPlayer(guid);
        if (!player || !player->IsAlive())
            continue;

        float distance = player->GetDistance(objective->position.x, objective->position.y, objective->position.z);
        if (distance <= _threatDetectionRadius)
            count++;
    }

    return count;
}

// ============================================================================
// SCORING (PRIVATE)
// ============================================================================

float NodeController::ScoreStrategicImportance(uint32 nodeId) const
{
    const BGObjective* objective = _coordinator->GetObjective(nodeId);
    if (!objective)
        return 50.0f;

    // Use objective priority as basis
    return static_cast<float>(static_cast<uint8>(objective->currentPriority)) * 10.0f;
}

float NodeController::ScoreDefensibility(uint32 nodeId) const
{
    // Would be based on map geometry
    // Higher score = easier to defend
    return 50.0f; // Placeholder
}

float NodeController::ScoreContestability(uint32 nodeId) const
{
    // Low enemy presence = easy to contest
    float enemyStrength = CalculateEnemyStrength(nodeId);
    return std::max(0.0f, 100.0f - enemyStrength * 30.0f);
}

float NodeController::ScoreResourceValue(uint32 nodeId) const
{
    const BGObjective* objective = _coordinator->GetObjective(nodeId);
    if (!objective)
        return 50.0f;

    return objective->resourceValue;
}

// ============================================================================
// UTILITY (PRIVATE)
// ============================================================================

void NodeController::UpdateAssignments()
{
    // Clean up invalid assignments (dead players, etc.)
    std::vector<ObjectGuid> toRemove;

    for (const auto& pair : _playerToNode)
    {
        Player* player = _coordinator->GetPlayer(pair.first);
        if (!player || !player->IsAlive())
            toRemove.push_back(pair.first);
    }

    for (ObjectGuid guid : toRemove)
        UnassignFromNode(guid);
}

void NodeController::ProcessReinforcementRequests()
{
    // Process each request
    auto it = _reinforcementRequests.begin();
    while (it != _reinforcementRequests.end())
    {
        ReinforcementRequest& request = *it;

        // Check current defender count
        uint32 defenders = GetDefenderCount(request.nodeId);
        if (defenders >= request.count)
        {
            // Request fulfilled
            it = _reinforcementRequests.erase(it);
            continue;
        }

        // Try to find reinforcement
        ObjectGuid reinforcement = GetClosestAvailableReinforcement(request.nodeId);
        if (!reinforcement.IsEmpty())
        {
            AssignDefender(request.nodeId, reinforcement);
        }

        ++it;
    }
}

float NodeController::GetDistanceToNode(ObjectGuid player, uint32 nodeId) const
{
    Player* p = _coordinator->GetPlayer(player);
    if (!p)
        return std::numeric_limits<float>::max();

    const BGObjective* objective = _coordinator->GetObjective(nodeId);
    if (!objective)
        return std::numeric_limits<float>::max();

    return p->GetDistance(objective->position.x, objective->position.y, objective->position.z);
}

bool NodeController::IsPlayerAvailable(ObjectGuid player) const
{
    Player* p = _coordinator->GetPlayer(player);
    return p && p->IsAlive();
}

} // namespace Playerbot
