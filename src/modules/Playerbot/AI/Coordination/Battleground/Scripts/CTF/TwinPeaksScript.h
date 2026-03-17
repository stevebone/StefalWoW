/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Twin Peaks Script Header
 * ~250 lines - Complete interface for CTF coordination
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_CTF_TWINPEAKSSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_CTF_TWINPEAKSSCRIPT_H

#include "CTFScriptBase.h"
#include "TwinPeaksData.h"
#include <atomic>
#include <unordered_set>

namespace Playerbot::Coordination::Battleground
{

// ============================================================================
// GAME PHASE ENUMERATION
// ============================================================================

enum class TwinPeaksPhase
{
    OPENING,     // First 90 seconds - flag grab race, establish positions
    MID_GAME,    // Standard play - coordinated offense/defense
    LATE_GAME,   // Score pressure - adjust strategy based on score
    DESPERATE    // Behind by 2+ caps - all-in offense
};

// ============================================================================
// FC ROUTE TYPES
// ============================================================================

enum class FCRouteType
{
    DIRECT,      // Bridge route - fastest, most contested
    NORTH,       // North flank - avoids bridge
    SOUTH        // Waterfall route - scenic, slower
};

/**
 * @brief Twin Peaks battleground script
 *
 * Implements CTF mechanics for Twin Peaks:
 * - 10v10 capture the flag
 * - First to 3 captures wins
 * - 25 minute time limit
 * - Different terrain from WSG (river crossing, elevation changes)
 * - Phase-aware strategy with escort formations
 *
 * Map ID: 726
 */
class TwinPeaksScript : public CTFScriptBase
{
public:
    TwinPeaksScript() = default;
    ~TwinPeaksScript() = default;

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return TwinPeaks::MAP_ID; }
    std::string GetName() const override { return TwinPeaks::BG_NAME; }
    BGType GetBGType() const override { return BGType::TWIN_PEAKS; }
    uint32 GetMaxScore() const override { return TwinPeaks::MAX_SCORE; }
    uint32 GetMaxDuration() const override { return TwinPeaks::MAX_DURATION; }
    uint8 GetTeamSize() const override { return TwinPeaks::TEAM_SIZE; }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;
    void OnMatchStart() override;
    void OnMatchEnd(bool victory) override;
    void OnUpdate(uint32 diff) override;
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
     * @brief Dynamic behavior tree for Twin Peaks bot strategy
     * Phase-aware CTF: evaluates game phase, flag state, and score each tick.
     * @param player The bot player to execute strategy for
     * @return true if the script handled the player's behavior
     */
    bool ExecuteStrategy(::Player* player) override;

    /**
     * @brief Get FC route waypoints using 3-route risk evaluation
     * Evaluates DIRECT, NORTH, and SOUTH routes based on enemy positions.
     * @return Waypoints for the lowest-risk route
     */
    std::vector<Position> GetFCRouteWaypoints(uint32 faction,
        const std::vector<Position>& enemyPositions) const override;

    // ========================================================================
    // STRATEGY AND ROLES
    // ========================================================================

    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision,
        float scoreAdvantage, uint32 timeRemaining) const override;

    void AdjustStrategy(StrategicDecision& decision, float scoreAdvantage,
        uint32 controlledCount, uint32 totalObjectives,
        uint32 timeRemaining) const override;

    // ========================================================================
    // POSITIONAL DATA PROVIDERS
    // ========================================================================

    std::vector<Position> GetObjectivePath(
        uint32 fromObjective, uint32 toObjective) const override;

    // Sniper/overlook positions
    std::vector<BGPositionData> GetSniperPositions() const;

    // Ambush positions (faction-specific)
    std::vector<BGPositionData> GetAmbushPositions(uint32 faction) const;

    // Chokepoints
    std::vector<BGPositionData> GetChokepoints() const;

    // All buff positions
    std::vector<BGPositionData> GetBuffPositions() const;

    // ========================================================================
    // FC ESCORT AND ROUTING
    // ========================================================================

    // Get escort formation positions relative to FC
    std::vector<Position> GetFCEscortFormation() const;

    // Get absolute escort positions around a specific FC location
    std::vector<Position> GetAbsoluteEscortPositions(const Position& fcPosition) const;

    // Get FC kite path for specific route
    std::vector<Position> GetFCKitePath(uint32 faction, FCRouteType route) const;

    // Recommend best route based on enemy positions
    FCRouteType RecommendFCRoute(uint32 faction, const std::vector<Position>& enemyPositions) const;

    // ========================================================================
    // PHASE AND STATE QUERIES
    // ========================================================================

    TwinPeaksPhase GetCurrentPhase() const { return m_currentPhase; }
    uint32 GetMatchElapsedTime() const { return m_matchElapsedTime; }
    uint32 GetMatchRemainingTime() const;

    // Score queries
    uint32 GetAllianceScore() const { return m_allianceScore; }
    uint32 GetHordeScore() const { return m_hordeScore; }
    int32 GetScoreAdvantage(uint32 faction) const;

    // Flag state queries
    bool IsFlagAtBase(uint32 faction) const;
    bool IsFlagCarried(uint32 faction) const;
    bool IsFlagDropped(uint32 faction) const;

    // ========================================================================
    // TERRAIN QUERIES
    // ========================================================================

    bool IsInWater(float x, float y, float z) const;
    bool IsOnBridge(float x, float y) const;
    bool IsInAllianceBase(float x, float y) const;
    bool IsInHordeBase(float x, float y) const;

    // Get distance between two key locations
    float GetLocationDistance(uint8 fromLoc, uint8 toLoc) const;

protected:
    // ========================================================================
    // CTF ABSTRACT IMPLEMENTATIONS
    // ========================================================================

    Position GetAllianceFlagPosition() const override
    {
        return TwinPeaks::GetAllianceFlagPos();
    }

    Position GetHordeFlagPosition() const override
    {
        return TwinPeaks::GetHordeFlagPos();
    }

    std::vector<Position> GetAllianceFlagRoomDefense() const override
    {
        return TwinPeaks::GetAllianceFlagRoomDefense();
    }

    std::vector<Position> GetHordeFlagRoomDefense() const override
    {
        return TwinPeaks::GetHordeFlagRoomDefense();
    }

    std::vector<Position> GetMiddleChokepoints() const override
    {
        return TwinPeaks::GetMiddleChokepoints();
    }

    std::vector<Position> GetSpeedBuffPositions() const override
    {
        return TwinPeaks::GetSpeedBuffPositions();
    }

    std::vector<Position> GetRestoreBuffPositions() const override
    {
        return TwinPeaks::GetRestoreBuffPositions();
    }

    std::vector<Position> GetBerserkBuffPositions() const override
    {
        return TwinPeaks::GetBerserkBuffPositions();
    }

private:
    // ========================================================================
    // INTERNAL UPDATE METHODS
    // ========================================================================

    void UpdatePhase(uint32 timeElapsed, uint32 timeRemaining);
    void UpdateFlagStates(const std::map<int32, int32>& worldStates);
    void ProcessFlagEvent(const BGScriptEventData& event);

    // ========================================================================
    // INTERNAL STRATEGY HELPERS
    // ========================================================================

    void ApplyOpeningPhaseStrategy(StrategicDecision& decision, uint32 faction) const;
    void ApplyMidGameStrategy(StrategicDecision& decision, float scoreAdvantage) const;
    void ApplyLateGameStrategy(StrategicDecision& decision, float scoreAdvantage,
        uint32 timeRemaining) const;
    void ApplyDesperateStrategy(StrategicDecision& decision) const;

    // Route evaluation
    float EvaluateRouteRisk(FCRouteType route, uint32 faction,
        const std::vector<Position>& enemyPositions) const;

    // ========================================================================
    // STATE TRACKING
    // ========================================================================

    // Match state
    std::atomic<TwinPeaksPhase> m_currentPhase{TwinPeaksPhase::OPENING};
    uint32 m_matchElapsedTime = 0;
    uint32 m_matchStartTime = 0;
    bool m_matchActive = false;

    // Score tracking
    uint32 m_allianceScore = 0;
    uint32 m_hordeScore = 0;

    // Flag state tracking
    int32 m_allianceFlagState = TwinPeaks::WorldStates::FLAG_STATE_AT_BASE;
    int32 m_hordeFlagState = TwinPeaks::WorldStates::FLAG_STATE_AT_BASE;

    // Flag positions when dropped
    Position m_droppedAllianceFlagPos;
    Position m_droppedHordeFlagPos;

    // Update timers
    uint32 m_phaseUpdateTimer = 0;
    static constexpr uint32 PHASE_UPDATE_INTERVAL = 1000;  // 1 second
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_CTF_TWINPEAKSSCRIPT_H
