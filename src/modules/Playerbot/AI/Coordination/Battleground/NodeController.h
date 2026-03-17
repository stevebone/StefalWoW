/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "BGState.h"
#include <vector>
#include <map>

namespace Playerbot {

class BattlegroundCoordinator;

/**
 * @struct NodeAssignment
 * @brief Assignment of players to a node
 */
struct NodeAssignment
{
    uint32 nodeId;
    ::std::vector<ObjectGuid> defenders;
    ::std::vector<ObjectGuid> attackers;
    uint32 targetDefenderCount;
    uint32 targetAttackerCount;

    NodeAssignment()
        : nodeId(0), targetDefenderCount(0), targetAttackerCount(0) {}
};

/**
 * @struct NodeThreatInfo
 * @brief Threat assessment for a node
 */
struct NodeThreatInfo
{
    uint32 nodeId;
    uint32 nearbyEnemies;
    uint32 nearbyAllies;
    float enemyStrength;
    float allyStrength;
    bool isUnderAttack;
    bool isBeingLost;
    uint32 timeUntilLost;

    NodeThreatInfo()
        : nodeId(0), nearbyEnemies(0), nearbyAllies(0),
          enemyStrength(0), allyStrength(0),
          isUnderAttack(false), isBeingLost(false), timeUntilLost(0) {}
};

/**
 * @class NodeController
 * @brief Manages control point nodes in battlegrounds
 *
 * Handles:
 * - Node defense coordination
 * - Node attack coordination
 * - Threat assessment
 * - Reinforcement routing
 * - Capture priority
 */
class NodeController
{
public:
    NodeController(BattlegroundCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // NODE ACCESS
    // ========================================================================

    ::std::vector<uint32> GetAllNodeIds() const;
    ::std::vector<uint32> GetFriendlyNodeIds() const;
    ::std::vector<uint32> GetEnemyNodeIds() const;
    ::std::vector<uint32> GetNeutralNodeIds() const;
    ::std::vector<uint32> GetContestedNodeIds() const;

    // ========================================================================
    // NODE ASSIGNMENT
    // ========================================================================

    void AssignDefender(uint32 nodeId, ObjectGuid player);
    void AssignAttacker(uint32 nodeId, ObjectGuid player);
    void UnassignFromNode(ObjectGuid player);
    NodeAssignment* GetNodeAssignment(uint32 nodeId);
    const NodeAssignment* GetNodeAssignment(uint32 nodeId) const;
    uint32 GetPlayerAssignment(ObjectGuid player) const;
    bool IsAssignedTo(ObjectGuid player, uint32 nodeId) const;

    // ========================================================================
    // DEFENSE MANAGEMENT
    // ========================================================================

    void SetDefenderTarget(uint32 nodeId, uint32 count);
    uint32 GetDefenderTarget(uint32 nodeId) const;
    uint32 GetDefenderCount(uint32 nodeId) const;
    bool NeedsMoreDefenders(uint32 nodeId) const;
    bool HasTooManyDefenders(uint32 nodeId) const;
    ObjectGuid GetBestDefenderCandidate(uint32 nodeId) const;

    // ========================================================================
    // ATTACK MANAGEMENT
    // ========================================================================

    void SetAttackerTarget(uint32 nodeId, uint32 count);
    uint32 GetAttackerTarget(uint32 nodeId) const;
    uint32 GetAttackerCount(uint32 nodeId) const;
    bool NeedsMoreAttackers(uint32 nodeId) const;
    ObjectGuid GetBestAttackerCandidate(uint32 nodeId) const;

    // ========================================================================
    // THREAT ASSESSMENT
    // ========================================================================

    NodeThreatInfo AssessNodeThreat(uint32 nodeId) const;
    ::std::vector<NodeThreatInfo> AssessAllNodeThreats() const;
    bool IsNodeUnderAttack(uint32 nodeId) const;
    bool IsNodeSafe(uint32 nodeId) const;
    uint32 GetMostThreatenedNode() const;
    uint32 GetLeastDefendedNode() const;

    // ========================================================================
    // REINFORCEMENT
    // ========================================================================

    void RequestReinforcements(uint32 nodeId, uint32 count);
    void CancelReinforcementRequest(uint32 nodeId);
    bool HasPendingReinforcementRequest(uint32 nodeId) const;
    ::std::vector<uint32> GetNodesPendingReinforcement() const;
    ObjectGuid GetClosestAvailableReinforcement(uint32 nodeId) const;

    // ========================================================================
    // CAPTURE COORDINATION
    // ========================================================================

    bool ShouldCaptureNode(uint32 nodeId) const;
    bool IsSafeToCapture(uint32 nodeId) const;
    uint32 GetCaptureProgress(uint32 nodeId) const;
    uint32 GetTimeUntilCapture(uint32 nodeId) const;
    ObjectGuid GetCapturingPlayer(uint32 nodeId) const;

    // ========================================================================
    // OPTIMAL NODE SELECTION
    // ========================================================================

    uint32 GetBestNodeToAttack() const;
    uint32 GetBestNodeToDefend() const;
    uint32 GetBestNodeToReinforce() const;
    float ScoreNodeForAttack(uint32 nodeId) const;
    float ScoreNodeForDefense(uint32 nodeId) const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    uint32 GetTotalFriendlyNodes() const;
    uint32 GetTotalEnemyNodes() const;
    float GetNodeControlRatio() const;

private:
    BattlegroundCoordinator* _coordinator;

    // ========================================================================
    // ASSIGNMENTS
    // ========================================================================

    ::std::map<uint32, NodeAssignment> _nodeAssignments;
    ::std::map<ObjectGuid, uint32> _playerToNode;

    // ========================================================================
    // REINFORCEMENT REQUESTS
    // ========================================================================

    struct ReinforcementRequest
    {
        uint32 nodeId;
        uint32 count;
        uint32 requestTime;
    };
    ::std::vector<ReinforcementRequest> _reinforcementRequests;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    uint32 _defaultDefenderCount = 1;
    uint32 _contestedDefenderCount = 2;
    uint32 _attackGroupSize = 3;
    float _threatDetectionRadius = 50.0f;

    // ========================================================================
    // THREAT CALCULATION
    // ========================================================================

    float CalculateEnemyStrength(uint32 nodeId) const;
    float CalculateAllyStrength(uint32 nodeId) const;
    uint32 CountNearbyEnemies(uint32 nodeId) const;
    uint32 CountNearbyAllies(uint32 nodeId) const;

    // ========================================================================
    // SCORING
    // ========================================================================

    float ScoreStrategicImportance(uint32 nodeId) const;
    float ScoreDefensibility(uint32 nodeId) const;
    float ScoreContestability(uint32 nodeId) const;
    float ScoreResourceValue(uint32 nodeId) const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    void UpdateAssignments();
    void ProcessReinforcementRequests();
    float GetDistanceToNode(ObjectGuid player, uint32 nodeId) const;
    bool IsPlayerAvailable(ObjectGuid player) const;
};

} // namespace Playerbot
