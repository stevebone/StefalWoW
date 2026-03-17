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

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_EYEOFTHESTORMSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_EYEOFTHESTORMSCRIPT_H

#include "DominationScriptBase.h"
#include "EyeOfTheStormData.h"

namespace Playerbot::Coordination::Battleground
{

/**
 * @brief Eye of the Storm battleground script
 *
 * Hybrid CTF/Domination battleground:
 * - 4 capturable towers (nodes)
 * - Center flag that gives points based on nodes held
 * - First to 1500 wins
 *
 * Map ID: 566
 */
class EyeOfTheStormScript : public DominationScriptBase
{
public:
    EyeOfTheStormScript() = default;
    ~EyeOfTheStormScript() = default;

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return EyeOfTheStorm::MAP_ID; }
    std::string GetName() const override { return EyeOfTheStorm::BG_NAME; }
    BGType GetBGType() const override { return BGType::EYE_OF_THE_STORM; }
    uint32 GetMaxScore() const override { return EyeOfTheStorm::MAX_SCORE; }
    uint32 GetMaxDuration() const override { return EyeOfTheStorm::MAX_DURATION; }
    uint8 GetTeamSize() const override { return EyeOfTheStorm::TEAM_SIZE; }

    // Hybrid BG
    bool HasCentralObjective() const override { return true; }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;
    void OnUpdate(uint32 diff) override;

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
     * @brief Dynamic behavior tree for Eye of the Storm bot strategy
     * Hybrid CTF/Domination: nodes provide tick rate, flag provides burst points.
     * With 2+ nodes, some bots pursue flag; otherwise 100% node focus.
     * @param player The bot player to execute strategy for
     * @return true if the script handled the player's behavior
     */
    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // STRATEGY - EOTS SPECIFIC
    // ========================================================================

    RoleDistribution GetRecommendedRoles(
        const StrategicDecision& decision,
        float scoreAdvantage,
        uint32 timeRemaining) const override;

    void AdjustStrategy(StrategicDecision& decision,
        float scoreAdvantage, uint32 controlledCount,
        uint32 totalObjectives, uint32 timeRemaining) const override;

    uint32 GetOptimalNodeCount() const override { return 3; }

    // ========================================================================
    // EVENTS
    // ========================================================================

    void OnEvent(const BGScriptEventData& event) override;
    void OnMatchStart() override;
    void OnMatchEnd(bool victory) override;

    // ========================================================================
    // ENTERPRISE-GRADE FLAG RUNNING AND POSITIONING
    // ========================================================================

    /**
     * @brief Get optimal flag running route from center to a node
     * @param nodeId Target node for flag capture
     * @return Vector of waypoints for the route
     */
    std::vector<Position> GetFlagRouteToNode(uint32 nodeId) const;

    /**
     * @brief Get escort formation positions around flag carrier
     * @param fcPosition Flag carrier's current position
     * @param escortCount Number of escorts
     * @return Vector of escort positions
     */
    std::vector<Position> GetEscortFormation(Position const& fcPosition, uint8 escortCount) const;

    /**
     * @brief Get best node for flag capture based on faction and control
     * @param faction ALLIANCE or HORDE
     * @return Best node ID for flag capture
     */
    uint32 GetBestFlagCaptureNode(uint32 faction) const;

    /**
     * @brief Get distance from a node to center flag
     */
    float GetDistanceToCenter(uint32 nodeId) const;

    /**
     * @brief Get all bridge positions (critical chokepoints)
     */
    std::vector<Position> GetBridgePositions() const;

    /**
     * @brief Get all sniper/elevated positions
     */
    std::vector<Position> GetSniperPositions() const;

    /**
     * @brief Get center flag area defense positions
     */
    std::vector<Position> GetCenterFlagDefensePositions() const;

    /**
     * @brief Check if flag is worth pursuing with current node control
     */
    bool IsFlagWorthPursuing() const;

    /**
     * @brief Get recommended escort count based on node control
     */
    uint8 GetRecommendedEscortCount() const;

    /**
     * @brief Check if a node is strategically important (closer to center)
     */
    bool IsStrategicNode(uint32 nodeId) const;

    /**
     * @brief Get the node nearest to the center flag
     */
    uint32 GetNearestNodeToFlag() const;

    /**
     * @brief Get defenders needed for a specific node
     */
    uint32 GetDefendersNeeded(uint32 nodeId) const;

protected:
    // ========================================================================
    // DOMINATION ABSTRACT IMPLEMENTATIONS
    // ========================================================================

    uint32 GetNodeCount() const override { return EyeOfTheStorm::NODE_COUNT; }
    BGObjectiveData GetNodeData(uint32 nodeIndex) const override;
    std::vector<uint32> GetTickPointsTable() const override;
    uint32 GetTickInterval() const override { return EyeOfTheStorm::TICK_INTERVAL; }
    uint32 GetDefaultCaptureTime() const override { return EyeOfTheStorm::CAPTURE_TIME; }

private:
    // ========================================================================
    // EOTS-SPECIFIC HELPERS
    // ========================================================================

    /**
     * @brief Get flag capture points based on node control
     */
    uint32 GetFlagCapturePoints() const;

    /**
     * @brief Should we prioritize flag over nodes?
     */
    bool ShouldPrioritizeFlag() const;

    /**
     * @brief Get recommended flag runner based on situation
     */
    ObjectGuid GetBestFlagRunnerCandidate() const;

    // Flag state tracking
    bool m_flagAtCenter = true;
    ObjectGuid m_flagCarrier;
    uint32 m_flagPickupTime = 0;
    uint32 m_flagDropX = 0, m_flagDropY = 0, m_flagDropZ = 0;

    // Flag capture tracking
    uint32 m_allianceFlagCaptures = 0;
    uint32 m_hordeFlagCaptures = 0;
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_DOMINATION_EYEOFTHESTORMSCRIPT_H
