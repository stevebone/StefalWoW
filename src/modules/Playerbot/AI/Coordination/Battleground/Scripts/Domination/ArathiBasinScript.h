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

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_ARATHIBASINSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_ARATHIBASINSCRIPT_H

#include "DominationScriptBase.h"
#include "ArathiBasinData.h"

namespace Playerbot::Coordination::Battleground
{

/**
 * @brief Arathi Basin battleground script
 *
 * Implements 5-node domination for Arathi Basin:
 * - Stables (Alliance side)
 * - Blacksmith (Center - critical)
 * - Farm (Horde side)
 * - Gold Mine (Horde side)
 * - Lumber Mill (Alliance side, high ground)
 *
 * Win condition: First to 1500 resources
 * Resource generation: Tick-based (2 second intervals)
 *
 * Map ID: 529
 */
class ArathiBasinScript : public DominationScriptBase
{
public:
    ArathiBasinScript() = default;
    ~ArathiBasinScript() = default;

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return ArathiBasin::MAP_ID; }
    std::string GetName() const override { return ArathiBasin::BG_NAME; }
    BGType GetBGType() const override { return BGType::ARATHI_BASIN; }
    uint32 GetMaxScore() const override { return ArathiBasin::MAX_SCORE; }
    uint32 GetMaxDuration() const override { return ArathiBasin::MAX_DURATION; }
    uint8 GetTeamSize() const override { return ArathiBasin::TEAM_SIZE; }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;

    // ========================================================================
    // DATA PROVIDERS
    // ========================================================================

    std::vector<BGObjectiveData> GetObjectiveData() const override;
    std::vector<BGPositionData> GetSpawnPositions(uint32 faction) const override;
    std::vector<BGPositionData> GetStrategicPositions() const override;
    std::vector<BGPositionData> GetGraveyardPositions(uint32 faction) const override;
    std::vector<BGWorldState> GetInitialWorldStates() const override;

    // ========================================================================
    // WORLD STATE
    // ========================================================================

    bool InterpretWorldState(int32 stateId, int32 value,
        uint32& outObjectiveId, BGObjectiveState& outState) const override;

    void GetScoreFromWorldStates(const std::map<int32, int32>& states,
        uint32& allianceScore, uint32& hordeScore) const override;

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    /**
     * @brief Dynamic behavior tree for Arathi Basin bot strategy
     * Evaluates node state each tick and assigns capture/defend duties.
     * Uses 3-cap strategy with Blacksmith priority.
     * @param player The bot player to execute strategy for
     * @return true if the script handled the player's behavior
     */
    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // STRATEGY - AB SPECIFIC
    // ========================================================================

    void AdjustStrategy(StrategicDecision& decision,
        float scoreAdvantage, uint32 controlledCount,
        uint32 totalObjectives, uint32 timeRemaining) const override;

    uint8 GetObjectiveAttackPriority(uint32 objectiveId,
        BGObjectiveState state, uint32 faction) const override;

    uint8 GetObjectiveDefensePriority(uint32 objectiveId,
        BGObjectiveState state, uint32 faction) const override;

    uint32 GetOptimalNodeCount() const override { return 3; }  // 3-cap is the classic AB strategy

protected:
    // ========================================================================
    // DOMINATION ABSTRACT IMPLEMENTATIONS
    // ========================================================================

    uint32 GetNodeCount() const override { return ArathiBasin::NODE_COUNT; }

    BGObjectiveData GetNodeData(uint32 nodeIndex) const override;

    std::vector<uint32> GetTickPointsTable() const override;

    uint32 GetTickInterval() const override { return ArathiBasin::TICK_INTERVAL; }

    uint32 GetDefaultCaptureTime() const override { return ArathiBasin::CAPTURE_TIME; }

    // ========================================================================
    // EVENTS
    // ========================================================================

    void OnEvent(const BGScriptEventData& event) override;
    void OnMatchStart() override;
    void OnMatchEnd(bool victory) override;

    // ========================================================================
    // ENTERPRISE-GRADE ROUTING AND POSITIONING
    // ========================================================================

    /**
     * @brief Get rotation path between two nodes
     * @param fromNode Source node ID
     * @param toNode Destination node ID
     * @return Vector of waypoints for the route
     */
    std::vector<Position> GetRotationPath(uint32 fromNode, uint32 toNode) const;

    /**
     * @brief Get ambush positions for intercepting enemy rotations
     * @param faction ALLIANCE or HORDE (positions vary by faction)
     * @return Vector of ambush positions
     */
    std::vector<Position> GetAmbushPositions(uint32 faction) const;

    /**
     * @brief Get pre-calculated distance between two nodes
     * @return Distance in yards
     */
    float GetNodeToNodeDistance(uint32 fromNode, uint32 toNode) const;

    /**
     * @brief Find nearest node to a position
     */
    uint32 GetNearestNode(Position const& pos) const;

    /**
     * @brief Determine best target for assault based on current state
     */
    uint32 GetBestAssaultTarget(uint32 faction) const;

    /**
     * @brief Get defense priority for a node
     */
    uint32 GetDefensePriority(uint32 nodeId) const;

    /**
     * @brief Check if it's time to rotate defenders
     */
    bool ShouldRotate() const;

    /**
     * @brief Get all chokepoint positions
     */
    std::vector<Position> GetChokepoints() const;

    /**
     * @brief Get all sniper/elevated positions
     */
    std::vector<Position> GetSniperPositions() const;

    /**
     * @brief Get all buff spawn positions
     */
    std::vector<Position> GetBuffPositions() const;

private:
    // ========================================================================
    // AB-SPECIFIC HELPERS
    // ========================================================================

    /**
     * @brief Get the "3-cap" strategy nodes for a faction
     * @param faction ALLIANCE or HORDE
     * @return Vector of 3 node IDs to prioritize
     */
    std::vector<uint32> Get3CapStrategy(uint32 faction) const;

    /**
     * @brief Check if Blacksmith control is critical for current state
     */
    bool IsBlacksmithCritical() const;

    /**
     * @brief Get opening rush target based on faction
     */
    uint32 GetOpeningRushTarget(uint32 faction) const;

    /**
     * @brief Calculate distance from spawn to node
     */
    float GetDistanceFromSpawn(uint32 nodeId, uint32 faction) const;
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_DOMINATION_ARATHIBASINSCRIPT_H
