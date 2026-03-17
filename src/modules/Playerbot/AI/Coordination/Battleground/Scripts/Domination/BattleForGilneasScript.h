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

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_BATTLEFORGILNEASSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_BATTLEFORGILNEASSCRIPT_H

#include "DominationScriptBase.h"
#include "BattleForGilneasData.h"

namespace Playerbot::Coordination::Battleground
{

/**
 * @class BattleForGilneasScript
 * @brief Enterprise-grade Battle for Gilneas battleground script
 *
 * Battle for Gilneas is a 10v10 domination battleground with 3 nodes:
 * - Lighthouse (Alliance-side, northwest)
 * - Waterworks (Center, critical)
 * - Mines (Horde-side, southeast)
 *
 * Key mechanics:
 * - 2-cap strategy is optimal (3 points/tick vs 10 for 3-cap)
 * - Waterworks is the critical center node
 * - First to 2000 resources wins
 *
 * This script provides:
 * - 30 node defense positions (10 per node)
 * - 8 chokepoint positions for ambushes
 * - 7 sniper/overlook positions
 * - Pre-calculated rotation paths
 * - Phase-aware strategy (opening, mid-game, late game)
 */
class BattleForGilneasScript : public DominationScriptBase
{
public:
    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return BattleForGilneas::MAP_ID; }
    std::string GetName() const override { return BattleForGilneas::BG_NAME; }
    BGType GetBGType() const override { return BGType::BATTLE_FOR_GILNEAS; }
    uint32 GetMaxScore() const override { return BattleForGilneas::MAX_SCORE; }
    uint32 GetMaxDuration() const override { return BattleForGilneas::MAX_DURATION; }
    uint8 GetTeamSize() const override { return BattleForGilneas::TEAM_SIZE; }
    uint32 GetOptimalNodeCount() const override { return BattleForGilneas::Strategy::OPTIMAL_NODE_COUNT; }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;
    void OnMatchStart() override;
    void OnMatchEnd(bool victory) override;
    void OnEvent(const BGScriptEventData& event) override;

    // ========================================================================
    // DATA PROVIDERS
    // ========================================================================

    std::vector<BGObjectiveData> GetObjectiveData() const override;
    std::vector<BGPositionData> GetSpawnPositions(uint32 faction) const override;
    std::vector<BGPositionData> GetStrategicPositions() const override;
    std::vector<BGPositionData> GetGraveyardPositions(uint32 faction) const override;
    std::vector<BGWorldState> GetInitialWorldStates() const override;

    // ========================================================================
    // WORLD STATE INTERPRETATION
    // ========================================================================

    bool InterpretWorldState(int32 stateId, int32 value, uint32& outObjectiveId, BGObjectiveState& outState) const override;
    void GetScoreFromWorldStates(const std::map<int32, int32>& states, uint32& allianceScore, uint32& hordeScore) const override;

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    /**
     * @brief Dynamic behavior tree for Battle for Gilneas bot strategy
     * Evaluates node state each tick and assigns capture/defend duties.
     * Uses 2-cap strategy with Waterworks priority.
     * @param player The bot player to execute strategy for
     * @return true if the script handled the player's behavior
     */
    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // STRATEGY & ROLE DISTRIBUTION
    // ========================================================================

    void AdjustStrategy(StrategicDecision& decision, float scoreAdvantage, uint32 controlledCount, uint32 totalObjectives, uint32 timeRemaining) const override;
    uint8 GetObjectiveAttackPriority(uint32 objectiveId, BGObjectiveState state, uint32 faction) const override;
    uint8 GetObjectiveDefensePriority(uint32 objectiveId, BGObjectiveState state, uint32 faction) const override;
    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision, float scoreAdvantage, uint32 timeRemaining) const override;

    // ========================================================================
    // ENTERPRISE-GRADE POSITIONING
    // ========================================================================

    /// Get pre-calculated rotation path between two nodes
    std::vector<Position> GetRotationPath(uint32 fromNode, uint32 toNode) const;

    /// Get faction-specific ambush positions for intercepting enemy rotations
    std::vector<Position> GetAmbushPositions(uint32 faction) const;

    /// Get pre-calculated distance between two nodes
    float GetNodeToNodeDistance(uint32 fromNode, uint32 toNode) const;

    /// Get the nearest node to a given position
    uint32 GetNearestNode(Position const& pos) const;

    /// Get the best assault target based on current game state
    uint32 GetBestAssaultTarget(uint32 faction) const;

    /// Get defense priority for a node (higher = more important to defend)
    uint32 GetDefensePriority(uint32 nodeId) const;

    /// Check if it's time to rotate based on strategy timing
    bool ShouldRotate() const;

    /// Get all chokepoint positions
    std::vector<Position> GetChokepoints() const;

    /// Get all sniper/overlook positions
    std::vector<Position> GetSniperPositions() const;

    /// Get all buff locations
    std::vector<Position> GetBuffPositions() const;

    // ========================================================================
    // BFG-SPECIFIC HELPERS
    // ========================================================================

    /// Get optimal 2-cap strategy targets for a faction
    std::vector<uint32> Get2CapStrategy(uint32 faction) const;

    /// Check if Waterworks is critical (always true in BFG)
    bool IsWaterworksCritical() const;

    /// Get the opening rush target for a faction
    uint32 GetOpeningRushTarget(uint32 faction) const;

    /// Get distance from spawn to a node
    float GetDistanceFromSpawn(uint32 nodeId, uint32 faction) const;

protected:
    // ========================================================================
    // BASE CLASS OVERRIDES
    // ========================================================================

    uint32 GetNodeCount() const override { return BattleForGilneas::NODE_COUNT; }
    BGObjectiveData GetNodeData(uint32 nodeIndex) const override;
    std::vector<uint32> GetTickPointsTable() const override;
    uint32 GetTickInterval() const override { return BattleForGilneas::TICK_INTERVAL; }
    uint32 GetDefaultCaptureTime() const override { return BattleForGilneas::CAPTURE_TIME; }

private:
    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /// Calculate 3D distance between two points
    float CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) const
    {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dz = z2 - z1;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /// Determine current game phase (opening, mid, late)
    enum class GamePhase { OPENING, MID_GAME, LATE_GAME, DESPERATE };
    GamePhase GetCurrentPhase() const;

    /// Get phase-specific strategy adjustments
    void ApplyPhaseStrategy(StrategicDecision& decision, GamePhase phase, float scoreAdvantage) const;

    /// Phase hysteresis: track last phase to prevent rapid oscillation
    mutable GamePhase m_lastPhase = GamePhase::OPENING;
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_DOMINATION_BATTLEFORGILNEASSCRIPT_H
