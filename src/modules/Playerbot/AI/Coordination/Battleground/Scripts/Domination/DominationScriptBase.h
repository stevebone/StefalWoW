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

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_DOMINATIONSCRIPTBASE_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_DOMINATIONSCRIPTBASE_H

#include "BGScriptBase.h"
#include <array>

namespace Playerbot::Coordination::Battleground
{

/**
 * @brief Base class for Domination (Node Control) battlegrounds
 *
 * Provides common domination mechanics for:
 * - Arathi Basin (5 nodes)
 * - Eye of the Storm (4 nodes + flag hybrid)
 * - Battle for Gilneas (3 nodes)
 * - Deepwind Gorge (3 nodes + carts)
 * - Seething Shore (dynamic spawning)
 *
 * Key Domination Mechanics:
 * - Node capture with progress bar
 * - Tick-based scoring (points per node count)
 * - Node state tracking (neutral, contested, controlled)
 * - Defense vs offense balance decisions
 */
class DominationScriptBase : public BGScriptBase
{
public:
    DominationScriptBase() = default;
    virtual ~DominationScriptBase() = default;

    // ========================================================================
    // Domination specific
    // ========================================================================

    bool IsDomination() const override { return true; }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;
    void OnUpdate(uint32 diff) override;

    // ========================================================================
    // STRATEGY - Domination overrides
    // ========================================================================

    RoleDistribution GetRecommendedRoles(
        const StrategicDecision& decision,
        float scoreAdvantage,
        uint32 timeRemaining) const override;

    void AdjustStrategy(StrategicDecision& decision,
        float scoreAdvantage, uint32 controlledCount,
        uint32 totalObjectives, uint32 timeRemaining) const override;

    uint8 GetObjectiveAttackPriority(uint32 objectiveId,
        BGObjectiveState state, uint32 faction) const override;

    uint8 GetObjectiveDefensePriority(uint32 objectiveId,
        BGObjectiveState state, uint32 faction) const override;

    float CalculateWinProbability(uint32 allianceScore, uint32 hordeScore,
        uint32 timeRemaining, uint32 objectivesControlled, uint32 faction) const override;

    // ========================================================================
    // DOMINATION-SPECIFIC IMPLEMENTATIONS
    // ========================================================================

    /**
     * @brief Get points per tick based on node count
     */
    uint32 GetTickPoints(uint32 nodeCount) const override;

    /**
     * @brief Get optimal node count for guaranteed win
     */
    uint32 GetOptimalNodeCount() const override;

    // ========================================================================
    // EVENT HANDLING
    // ========================================================================

    void OnEvent(const BGScriptEventData& event) override;
    void OnMatchStart() override;

protected:
    // ========================================================================
    // ABSTRACT - MUST BE IMPLEMENTED BY DERIVED CLASSES
    // ========================================================================

    /**
     * @brief Get total number of nodes in this BG
     */
    virtual uint32 GetNodeCount() const = 0;

    /**
     * @brief Get node data by index
     */
    virtual BGObjectiveData GetNodeData(uint32 nodeIndex) const = 0;

    /**
     * @brief Get tick points array for this BG
     * @return Array of points per tick indexed by controlled node count
     */
    virtual std::vector<uint32> GetTickPointsTable() const = 0;

    /**
     * @brief Get the time between resource ticks
     */
    virtual uint32 GetTickInterval() const { return 2000; }  // Default: 2 seconds

    /**
     * @brief Get default capture time for nodes
     */
    virtual uint32 GetDefaultCaptureTime() const { return 60000; }  // Default: 60 seconds

    // ========================================================================
    // DOMINATION HELPERS
    // ========================================================================

    /**
     * @brief Calculate resource rate based on controlled nodes
     */
    float CalculateResourceRate(uint32 controlledNodes) const;

    /**
     * @brief Estimate time to reach target score
     * @param currentScore Current score
     * @param targetScore Target score (usually max)
     * @param controlledNodes Nodes currently controlled
     */
    uint32 EstimateTimeToScore(uint32 currentScore, uint32 targetScore,
        uint32 controlledNodes) const;

    /**
     * @brief Calculate minimum nodes needed to win from current state
     */
    uint32 CalculateMinNodesNeeded(uint32 ourScore, uint32 theirScore,
        uint32 timeRemaining) const;

    /**
     * @brief Check if we can win with current node control
     */
    bool CanWinWithCurrentControl(uint32 ourScore, uint32 theirScore,
        uint32 ourNodes, uint32 theirNodes, uint32 timeRemaining) const;

    /**
     * @brief Get recommended defender count per node
     * @param nodeId Node to defend
     * @param threat Threat level (enemies nearby)
     */
    uint8 GetRecommendedDefenders(uint32 nodeId, uint8 threat) const;

    /**
     * @brief Get recommended attacker count for node
     * @param nodeId Node to attack
     * @param defenderCount Known defenders
     */
    uint8 GetRecommendedAttackers(uint32 nodeId, uint8 defenderCount) const;

    /**
     * @brief Find weakest enemy-controlled node
     */
    uint32 FindWeakestEnemyNode() const;

    /**
     * @brief Find most threatened friendly node
     */
    uint32 FindMostThreatenedFriendlyNode() const;

    /**
     * @brief Calculate node strategic value based on position
     */
    uint8 CalculateNodeStrategicValue(uint32 nodeId) const;

    /**
     * @brief Get nodes sorted by priority for attack
     */
    std::vector<uint32> GetAttackPriorityOrder() const;

    /**
     * @brief Get nodes sorted by priority for defense
     */
    std::vector<uint32> GetDefensePriorityOrder() const;

    // ========================================================================
    // DOMINATION STATE
    // ========================================================================

    // Node control tracking
    std::map<uint32, BGObjectiveState> m_nodeStates;
    std::map<uint32, float> m_nodeCaptureProgress;  // 0.0 to 1.0
    std::map<uint32, uint32> m_nodeLastContestTime;

    // Score tracking
    uint32 m_allianceScore = 0;
    uint32 m_hordeScore = 0;
    uint32 m_lastTickTime = 0;

    // Control counts
    uint32 m_allianceNodes = 0;
    uint32 m_hordeNodes = 0;
    uint32 m_contestedNodes = 0;
    uint32 m_neutralNodes = 0;

    // Strategic metrics
    float m_allianceResourceRate = 0.0f;
    float m_hordeResourceRate = 0.0f;
    uint32 m_projectedAllianceWinTime = 0;
    uint32 m_projectedHordeWinTime = 0;

    /**
     * @brief Initialize node tracking maps by calling GetNodeCount()/GetNodeData()
     *
     * This MUST be called from the derived class's OnLoad() AFTER calling
     * DominationScriptBase::OnLoad(). We deliberately avoid calling virtual
     * functions from within DominationScriptBase::OnLoad() because MSVC's
     * RelWithDebInfo vtable slot resolution can be fragile when the slot is
     * at the boundary between base and derived vtable regions.
     */
    void InitializeNodeTracking();

    // ========================================================================
    // RUNTIME BEHAVIOR METHODS (for ExecuteStrategy)
    // ========================================================================

    /**
     * @brief Refresh node ownership state from tracked m_nodeStates
     * Throttled to once per second. Updates count fields.
     */
    void RefreshNodeState();

    /**
     * @brief Move to a node and capture it (interact with banner/flag GO)
     * @param bot The bot player
     * @param nodeIndex Index into node data array
     * @return true if behavior was executed
     */
    bool CaptureNode(::Player* bot, uint32 nodeIndex);

    /**
     * @brief Defend a node: patrol nearby, engage enemies, recapture if contested
     * @param bot The bot player
     * @param nodeIndex Index into node data array
     * @return true if behavior was executed
     */
    bool DefendNode(::Player* bot, uint32 nodeIndex);

    /**
     * @brief Find the nearest capturable node (neutral or enemy-controlled)
     * @param bot The bot player
     * @return Node index, or UINT32_MAX if none found
     */
    uint32 FindNearestCapturableNode(::Player* bot) const;

    /**
     * @brief Find the nearest friendly node that is under attack (contested)
     * @param bot The bot player
     * @return Node index, or UINT32_MAX if none found
     */
    uint32 FindNearestThreatenedNode(::Player* bot) const;

    /**
     * @brief Get the best assault target node for the bot's faction
     * Considers strategic value, distance, and current defense
     * @param bot The bot player
     * @return Node index, or UINT32_MAX if none found
     */
    uint32 GetBestAssaultTarget(::Player* bot) const;

    /**
     * @brief Get number of friendly-controlled nodes for the given player's team
     */
    uint32 GetFriendlyNodeCount(::Player* bot) const;

    /**
     * @brief Get node indices controlled by the bot's faction
     */
    std::vector<uint32> GetFriendlyNodes(::Player* bot) const;

    /**
     * @brief Check if a node state is friendly to the given faction
     */
    static bool IsNodeFriendly(BGObjectiveState state, uint32 faction);

    /**
     * @brief Check if a node state is enemy-controlled for the given faction
     */
    static bool IsNodeEnemy(BGObjectiveState state, uint32 faction);

    /**
     * @brief Check if a node is contested (being captured by either side)
     */
    static bool IsNodeContested(BGObjectiveState state);

    uint32 m_lastNodeStateRefresh = 0;
    static constexpr uint32 NODE_STATE_REFRESH_INTERVAL = 1000;

    // ========================================================================
    // NODE DEFENSE COMMITMENT
    // ========================================================================

    /**
     * @brief After capturing a node, the bot commits to defending it for a period.
     * Prevents the human-unrealistic "capture and immediately leave" pattern.
     */
    struct DefenseCommitment
    {
        uint32 nodeIndex;      // Which node to defend
        uint32 commitTime;     // When commitment started (getMSTime)
        uint32 durationMs;     // How long to defend (scales by game state)
    };

    std::map<ObjectGuid, DefenseCommitment> m_nodeDefenseCommitments;

    /**
     * @brief Record that a bot should defend a node after capturing it
     * @param botGuid The bot's GUID
     * @param nodeIndex The captured node
     */
    void CommitToDefense(ObjectGuid botGuid, uint32 nodeIndex);

    /**
     * @brief Check if a bot has an active defense commitment
     * If so, execute DefendNode for the committed node and return true.
     * @param bot The bot player
     * @return true if the bot is under a defense commitment (behavior was handled)
     */
    bool CheckDefenseCommitment(::Player* bot);

    /**
     * @brief Get the defense commitment duration based on current game state
     * Opening: 10s, Mid-game: 20s, Just lost a node: 30s
     */
    uint32 GetDefenseCommitmentDuration() const;

    // ========================================================================
    // REINFORCEMENT ROUTING ("INC" RESPONSE)
    // ========================================================================

    /**
     * @brief Track when each node came under attack for reinforcement priority
     */
    std::map<uint32, uint32> m_nodeUnderAttackSince;

    /**
     * @brief Check if a nearby contested node needs reinforcement
     * Bots within divert range check for contested nodes with rising priority.
     * @param bot The bot player
     * @param maxDivertRange Max distance to divert from current path
     * @return Node index to reinforce, or UINT32_MAX if none
     */
    uint32 CheckReinforcementNeeded(::Player* bot, float maxDivertRange = 80.0f) const;

private:
    // Update timers
    uint32 m_nodeUpdateTimer = 0;
    uint32 m_strategyUpdateTimer = 0;

    static constexpr uint32 NODE_UPDATE_INTERVAL = 1000;      // Check nodes every second
    static constexpr uint32 STRATEGY_UPDATE_INTERVAL = 5000;  // Update strategy every 5 seconds

    // Internal helpers
    void UpdateNodeCounts();
    void UpdateProjectedWinTimes();
    void RecalculateResourceRates();
};

// ============================================================================
// DOMINATION CONSTANTS
// ============================================================================

namespace DominationConstants
{
    // Default defenders per node
    constexpr uint8 MIN_DEFENDERS = 1;
    constexpr uint8 NORMAL_DEFENDERS = 2;
    constexpr uint8 MAX_DEFENDERS = 4;

    // Attack thresholds
    constexpr uint8 MIN_ATTACKERS = 2;
    constexpr uint8 OVERWHELMING_FORCE = 5;

    // Priority thresholds
    constexpr uint8 CRITICAL_PRIORITY = 10;
    constexpr uint8 HIGH_PRIORITY = 8;
    constexpr uint8 NORMAL_PRIORITY = 5;
    constexpr uint8 LOW_PRIORITY = 3;

    // Score thresholds (as percentages)
    constexpr float COMFORTABLE_LEAD = 0.15f;      // 15% ahead
    constexpr float DANGEROUS_DEFICIT = -0.20f;    // 20% behind
    constexpr float CRITICAL_DEFICIT = -0.30f;     // 30% behind
}

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_DOMINATION_DOMINATIONSCRIPTBASE_H
