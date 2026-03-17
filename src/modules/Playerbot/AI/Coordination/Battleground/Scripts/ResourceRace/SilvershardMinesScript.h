/*
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Silvershard Mines Script Header
 * ~200 lines - Complete interface for mine cart coordination
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_RESOURCERACE_SILVERSHARDMINESSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_RESOURCERACE_SILVERSHARDMINESSCRIPT_H

#include "ResourceRaceScriptBase.h"
#include "SilvershardMinesData.h"
#include <atomic>
#include <unordered_set>

namespace Playerbot::Coordination::Battleground
{

// ============================================================================
// GAME PHASE ENUMERATION
// ============================================================================

enum class SilvershardMinesPhase
{
    OPENING,     // First 60 seconds - cart claim race
    MID_GAME,    // Standard cart control
    LATE_GAME,   // Score pressure - focus high-value carts
    DESPERATE    // Behind significantly - all-in interception
};

/**
 * @brief Silvershard Mines battleground script
 *
 * Implements resource race mechanics:
 * - 10v10 mine cart escort
 * - First to 1600 points wins
 * - 25 minute time limit
 * - 3 carts on different tracks with intersections
 * - Phase-aware strategy with escort formations
 *
 * Map ID: 727
 */
class SilvershardMinesScript : public ResourceRaceScriptBase
{
public:
    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return SilvershardMines::MAP_ID; }
    std::string GetName() const override { return SilvershardMines::BG_NAME; }
    BGType GetBGType() const override { return BGType::SILVERSHARD_MINES; }
    uint32 GetMaxScore() const override { return SilvershardMines::MAX_SCORE; }
    uint32 GetMaxDuration() const override { return SilvershardMines::MAX_DURATION; }
    uint8 GetTeamSize() const override { return SilvershardMines::TEAM_SIZE; }

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
    bool InterpretWorldState(int32 stateId, int32 value, uint32& outObjectiveId, BGObjectiveState& outState) const override;
    void GetScoreFromWorldStates(const std::map<int32, int32>& states, uint32& allianceScore, uint32& hordeScore) const override;

    // ========================================================================
    // STRATEGY AND ROLES
    // ========================================================================

    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision,
        float scoreAdvantage, uint32 timeRemaining) const override;

    void AdjustStrategy(StrategicDecision& decision, float scoreAdvantage,
        uint32 controlledCount, uint32 totalObjectives,
        uint32 timeRemaining) const override;

    // ========================================================================
    // RESOURCERACE BASE IMPLEMENTATIONS
    // ========================================================================

    uint32 GetCartCount() const override { return SilvershardMines::CART_COUNT; }
    Position GetCartPosition(uint32 cartId) const override;
    float GetCartProgress(uint32 cartId) const override;
    uint32 GetCartController(uint32 cartId) const override;
    bool IsCartContested(uint32 cartId) const override;
    uint32 GetPointsPerCapture() const override { return SilvershardMines::POINTS_PER_CAPTURE; }

    // Track info
    uint32 GetTrackCount() const override { return SilvershardMines::Tracks::TRACK_COUNT; }
    std::vector<Position> GetTrackWaypoints(uint32 trackId) const override;
    uint32 GetCartOnTrack(uint32 trackId) const override;

    // Intersection handling
    bool HasIntersections() const override { return true; }
    std::vector<uint32> GetIntersectionIds() const override;
    uint32 GetIntersectionDecisionTime(uint32 intersectionId) const override;

    // ========================================================================
    // POSITIONAL DATA PROVIDERS
    // ========================================================================

    std::vector<BGPositionData> GetDepotDefensePositions(uint32 faction) const;
    std::vector<BGPositionData> GetIntersectionPositions(uint32 intersectionId) const;
    std::vector<BGPositionData> GetChokepoints() const;
    std::vector<BGPositionData> GetSniperPositions() const;
    std::vector<BGPositionData> GetAmbushPositions(uint32 faction) const;

    // ========================================================================
    // CART ESCORT
    // ========================================================================

    std::vector<Position> GetCartEscortFormation() const;
    std::vector<Position> GetAbsoluteEscortPositions(const Position& cartPosition, float cartOrientation) const;

    // ========================================================================
    // SSM-SPECIFIC QUERIES
    // ========================================================================

    uint32 GetTrackForCart(uint32 cartId) const;
    bool IsCartNearIntersection(uint32 cartId) const;
    uint32 GetEstimatedCaptureTime(uint32 cartId) const;
    float GetCartPriority(uint32 cartId) const;
    uint32 GetMostValuableCart(uint32 faction) const;

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // PHASE AND STATE QUERIES
    // ========================================================================

    SilvershardMinesPhase GetCurrentPhase() const { return m_currentPhase; }
    uint32 GetMatchElapsedTime() const { return m_matchElapsedTime; }
    uint32 GetMatchRemainingTime() const;
    int32 GetScoreAdvantage(uint32 faction) const;

private:
    // ========================================================================
    // INTERNAL UPDATE METHODS
    // ========================================================================

    void UpdatePhase(uint32 timeElapsed, uint32 timeRemaining);
    void UpdateCartPositions();
    void ProcessCartEvent(const BGScriptEventData& event);

    // ========================================================================
    // INTERNAL STRATEGY HELPERS
    // ========================================================================

    void ApplyOpeningPhaseStrategy(StrategicDecision& decision, uint32 faction) const;
    void ApplyMidGameStrategy(StrategicDecision& decision, float scoreAdvantage) const;
    void ApplyLateGameStrategy(StrategicDecision& decision, float scoreAdvantage,
        uint32 timeRemaining) const;
    void ApplyDesperateStrategy(StrategicDecision& decision) const;

    // ========================================================================
    // STATE TRACKING
    // ========================================================================

    struct SSMCartState
    {
        uint32 trackId = 0;
        float trackProgress = 0.0f;
        bool atIntersection = false;
        uint32 intersectionId = 0;
        uint32 controller = 0;
        bool contested = false;
    };

    std::map<uint32, SSMCartState> m_ssmCartStates;
    std::map<uint32, Position> m_cartPositions;

    // Match state
    std::atomic<SilvershardMinesPhase> m_currentPhase{SilvershardMinesPhase::OPENING};
    uint32 m_matchElapsedTime = 0;
    uint32 m_matchStartTime = 0;
    bool m_matchActive = false;

    // Score tracking
    uint32 m_allianceScore = 0;
    uint32 m_hordeScore = 0;

    // Update timers
    uint32 m_phaseUpdateTimer = 0;
    uint32 m_cartUpdateTimer = 0;
    static constexpr uint32 PHASE_UPDATE_INTERVAL = 1000;
    static constexpr uint32 CART_UPDATE_INTERVAL = 500;
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_RESOURCERACE_SILVERSHARDMINESSCRIPT_H
