/*
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * Enterprise-Grade Deepwind Gorge Script Header
 * ~200 lines - Complete interface for hybrid node+cart BG coordination
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_DEEPWINDGORGESCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_DEEPWINDGORGESCRIPT_H

#include "DominationScriptBase.h"
#include "DeepwindGorgeData.h"
#include <atomic>
#include <unordered_set>

namespace Playerbot::Coordination::Battleground
{

// Game phase enumeration for phase-aware strategy
enum class DeepwindGorgePhase
{
    OPENING,     // First 90 seconds - capture nodes and position for carts
    MID_GAME,    // Middle period - balance nodes and carts
    LATE_GAME,   // Final push - score-focused decisions
    DESPERATE    // Behind significantly - all-in mode
};

class DeepwindGorgeScript : public DominationScriptBase
{
public:
    // ========================================================================
    // BASIC ACCESSORS (Override DominationScriptBase)
    // ========================================================================

    uint32 GetMapId() const override { return DeepwindGorge::MAP_ID; }
    std::string GetName() const override { return DeepwindGorge::BG_NAME; }
    BGType GetBGType() const override { return BGType::DEEPWIND_GORGE; }
    uint32 GetMaxScore() const override { return DeepwindGorge::MAX_SCORE; }
    uint32 GetMaxDuration() const override { return DeepwindGorge::MAX_DURATION; }
    uint8 GetTeamSize() const override { return DeepwindGorge::TEAM_SIZE; }
    uint32 GetOptimalNodeCount() const override { return DeepwindGorge::Strategy::OPTIMAL_NODE_COUNT; }

    // ========================================================================
    // LIFECYCLE METHODS
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;
    void OnMatchStart() override;
    void OnMatchEnd(bool victory) override;
    void OnUpdate(uint32 diff) override;
    void OnEvent(const BGScriptEventData& event) override;

    // ========================================================================
    // OBJECTIVE DATA PROVIDERS
    // ========================================================================

    std::vector<BGObjectiveData> GetObjectiveData() const override;
    std::vector<BGPositionData> GetSpawnPositions(uint32 faction) const override;
    std::vector<BGPositionData> GetStrategicPositions() const override;
    std::vector<BGPositionData> GetGraveyardPositions(uint32 faction) const override;
    std::vector<BGWorldState> GetInitialWorldStates() const override;

    // ========================================================================
    // WORLD STATE INTERPRETATION
    // ========================================================================

    bool InterpretWorldState(int32 stateId, int32 value, uint32& outObjectiveId,
                            BGObjectiveState& outState) const override;
    void GetScoreFromWorldStates(const std::map<int32, int32>& states,
                                uint32& allianceScore, uint32& hordeScore) const override;

    // ========================================================================
    // STRATEGY AND ROLE DISTRIBUTION
    // ========================================================================

    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision,
                                        float scoreAdvantage, uint32 timeRemaining) const override;
    void AdjustStrategy(StrategicDecision& decision, float scoreAdvantage,
                       uint32 controlledCount, uint32 totalObjectives,
                       uint32 timeRemaining) const override;

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // CART-SPECIFIC METHODS (Deepwind Gorge unique mechanics)
    // ========================================================================

    // Cart state queries
    bool IsCartActive(uint32 cartId) const { return m_activeCart == cartId; }
    uint32 GetActiveCart() const { return m_activeCart; }
    float GetCartProgress(uint32 cartId) const;
    bool IsCartContested(uint32 cartId) const;
    uint32 GetCartControllingFaction(uint32 cartId) const;

    // Cart strategic methods
    bool ShouldPrioritizeCart() const;
    uint32 GetBestCartToContest(uint32 faction) const;
    std::vector<BGPositionData> GetCartEscortFormation(uint32 cartId) const;
    std::vector<BGPositionData> GetCartInterceptionPositions() const;
    std::vector<Position> GetCartTrackToDepot(uint32 cartId, uint32 faction) const;

    // ========================================================================
    // POSITIONAL DATA PROVIDERS
    // ========================================================================

    // Node defense positions
    std::vector<BGPositionData> GetNodeDefensePositions(uint32 nodeId) const;

    // Cart depot defense
    std::vector<BGPositionData> GetDepotDefensePositions(uint32 faction) const;

    // Chokepoints
    std::vector<BGPositionData> GetChokepoints() const;

    // Sniper positions
    std::vector<BGPositionData> GetSniperPositions() const;

    // Ambush positions
    std::vector<BGPositionData> GetAmbushPositions(uint32 faction) const;

    // Rotation path between nodes
    std::vector<Position> GetRotationPath(uint32 fromNode, uint32 toNode) const;

    // ========================================================================
    // PHASE AND STATE QUERIES
    // ========================================================================

    DeepwindGorgePhase GetCurrentPhase() const { return m_currentPhase; }
    uint32 GetMatchElapsedTime() const { return m_matchElapsedTime; }
    uint32 GetMatchRemainingTime() const;

    // Node priority based on faction
    uint32 GetNodePriority(uint32 nodeId, uint32 faction) const;
    bool IsPandarenMineCritical() const;

protected:
    // ========================================================================
    // DOMINATION BASE OVERRIDES
    // ========================================================================

    uint32 GetNodeCount() const override { return DeepwindGorge::NODE_COUNT; }
    BGObjectiveData GetNodeData(uint32 nodeIndex) const override;
    std::vector<uint32> GetTickPointsTable() const override;
    uint32 GetTickInterval() const override { return DeepwindGorge::TICK_INTERVAL; }
    uint32 GetDefaultCaptureTime() const override { return DeepwindGorge::CAPTURE_TIME; }

private:
    // ========================================================================
    // INTERNAL UPDATE METHODS
    // ========================================================================

    void UpdateCartStates();
    void UpdatePhase(uint32 timeRemaining);
    void ProcessCartCapture(uint32 cartId, uint32 faction);

    // ========================================================================
    // INTERNAL STRATEGY HELPERS
    // ========================================================================

    void ApplyOpeningPhaseStrategy(StrategicDecision& decision, uint32 faction) const;
    void ApplyMidGameStrategy(StrategicDecision& decision, float scoreAdvantage) const;
    void ApplyLateGameStrategy(StrategicDecision& decision, float scoreAdvantage,
                               uint32 timeRemaining) const;
    void ApplyDesperateStrategy(StrategicDecision& decision) const;

    // Cart-specific strategy
    void ApplyCartBonus(StrategicDecision& decision) const;
    float CalculateCartPriority() const;

    // ========================================================================
    // STATE TRACKING
    // ========================================================================

    // Cart state
    uint32 m_activeCart = 0;
    std::map<uint32, float> m_cartProgress;      // cartId -> progress (0.0 - 1.0)
    std::map<uint32, bool> m_cartContested;      // cartId -> contested state
    std::map<uint32, uint32> m_cartFaction;      // cartId -> controlling faction

    // Match timing
    uint32 m_matchElapsedTime = 0;
    uint32 m_matchStartTime = 0;
    bool m_matchActive = false;

    // Phase tracking
    std::atomic<DeepwindGorgePhase> m_currentPhase{DeepwindGorgePhase::OPENING};
    mutable DeepwindGorgePhase m_lastPhase = DeepwindGorgePhase::OPENING;

    // Node control tracking
    std::map<uint32, uint32> m_nodeControlFaction;  // nodeId -> faction
    uint32 m_allianceNodesControlled = 0;
    uint32 m_hordeNodesControlled = 0;

    // Cart update timer
    uint32 m_cartUpdateTimer = 0;
};

} // namespace Playerbot::Coordination::Battleground

#endif
