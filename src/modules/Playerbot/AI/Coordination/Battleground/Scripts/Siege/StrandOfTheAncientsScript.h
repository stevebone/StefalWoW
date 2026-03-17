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

#ifndef PLAYERBOT_AI_COORDINATION_BG_SIEGE_STRANDOFTHEANCIENTSSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_SIEGE_STRANDOFTHEANCIENTSSCRIPT_H

#include "SiegeScriptBase.h"
#include "StrandOfTheAncientsData.h"
#include <array>
#include <atomic>

namespace Playerbot::Coordination::Battleground
{

/**
 * @class StrandOfTheAncientsScript
 * @brief Enterprise-grade Strand of the Ancients battleground script
 *
 * Strand of the Ancients (SOTA) is a unique assault/defense battleground:
 * - Round-Based: 2 rounds, teams swap attack/defense roles
 * - Gate Progression: 6 gates from beach to relic chamber
 *   - Tier 1 (Outer): Green Jade (left), Blue Sapphire (right)
 *   - Tier 2 (Middle): Red Sun (left), Purple Amethyst (right)
 *   - Tier 3 (Inner): Yellow Moon
 *   - Tier 4 (Final): Chamber of Ancient Relics
 * - Vehicles: Demolishers for attacking gates, Turrets for defense
 * - Graveyards: Captured as attackers progress (Beach, West, East, South)
 * - Victory: Capture Titan Relic OR defend until time expires
 *
 * Key strategies:
 * - Attackers: Split or focus path, demolisher escort, infantry support
 * - Defenders: Gate defense, demolisher kill squads, turret coverage
 * - Round 2: Beat opponent's time (attacker) or outlast their time (defender)
 *
 * This script provides:
 * - 48 gate defense positions (8 per gate)
 * - 10 chokepoints
 * - 8 sniper positions
 * - 8 ambush positions
 * - 12 turret positions
 * - 20 relic room positions (10 attack, 10 defense)
 * - Full demolisher routing (left and right paths)
 * - Phase-aware strategy (beach assault, outer gates, middle gates, etc.)
 * - Round management with time comparison
 *
 * Map ID: 607
 */
class StrandOfTheAncientsScript : public SiegeScriptBase
{
public:
    StrandOfTheAncientsScript() = default;
    ~StrandOfTheAncientsScript() = default;

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return StrandOfTheAncients::MAP_ID; }
    std::string GetName() const override { return StrandOfTheAncients::BG_NAME; }
    BGType GetBGType() const override { return BGType::STRAND_OF_THE_ANCIENTS; }
    uint32 GetMaxScore() const override { return 0; }  // Time-based win
    uint32 GetMaxDuration() const override { return StrandOfTheAncients::MAX_DURATION; }
    uint8 GetTeamSize() const override { return StrandOfTheAncients::TEAM_SIZE; }
    bool HasVehicles() const override { return true; }
    bool HasRounds() const { return true; }

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
    std::vector<BGVehicleData> GetVehicleData() const;
    std::vector<BGWorldState> GetInitialWorldStates() const override;

    // ========================================================================
    // WORLD STATE
    // ========================================================================

    bool InterpretWorldState(int32 stateId, int32 value,
        uint32& outObjectiveId, BGObjectiveState& outState) const override;

    void GetScoreFromWorldStates(const std::map<int32, int32>& states,
        uint32& allianceScore, uint32& hordeScore) const override;

    // ========================================================================
    // STRATEGY & ROLE DISTRIBUTION
    // ========================================================================

    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision,
        float scoreAdvantage, uint32 timeRemaining) const override;

    void AdjustStrategy(StrategicDecision& decision,
        float scoreAdvantage, uint32 controlledCount,
        uint32 totalObjectives, uint32 timeRemaining) const override;

    // ========================================================================
    // SIEGE ABSTRACT IMPLEMENTATIONS
    // ========================================================================

    uint32 GetBossEntry(uint32 faction) const override;
    Position GetBossPosition(uint32 faction) const override;
    bool CanAttackBoss(uint32 faction) const override;

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // SOTA-SPECIFIC METHODS
    // ========================================================================

    /// Check if we are the attacking team this round
    bool IsAttacker() const { return m_isAttacker; }

    /// Check if we are the defending team this round
    bool IsDefender() const { return !m_isAttacker; }

    /// Get current round number (1 or 2)
    uint32 GetCurrentRound() const { return m_currentRound; }

    /// Check if a specific gate is destroyed
    bool IsGateDestroyed(uint32 gateId) const;

    /// Check if the final gate (Chamber) is destroyed
    bool IsAncientGateDestroyed() const;

    /// Get count of destroyed gates
    uint32 GetDestroyedGateCount() const;

    /// Get recommended attack path based on current state
    StrandOfTheAncients::AttackPath GetRecommendedPath() const;

    /// Get list of gates that can currently be attacked
    std::vector<uint32> GetNextTargetGates() const;

    /// Check if a specific gate can be attacked (dependencies met)
    bool CanAttackGate(uint32 gateId) const;

    /// Get the current gate tier we're fighting at
    uint8 GetCurrentGateTier() const;

    /// Get highest priority gate to attack
    uint32 GetPriorityGate() const;

    /// Get highest priority gate to defend
    uint32 GetPriorityDefenseGate() const;

    // ========================================================================
    // POSITION PROVIDERS
    // ========================================================================

    /// Get defense positions for a specific gate
    std::vector<Position> GetGateDefensePositions(uint32 gateId) const;

    /// Get turret positions for current tier
    std::vector<Position> GetTurretPositions() const;

    /// Get demolisher route for a path
    std::vector<Position> GetDemolisherRoute(StrandOfTheAncients::AttackPath path) const;

    /// Get escort formation positions around a demolisher
    std::vector<Position> GetDemolisherEscortFormation(const Position& demoPos) const;

    /// Get relic attack positions
    std::vector<Position> GetRelicAttackPositions() const;

    /// Get relic defense positions
    std::vector<Position> GetRelicDefensePositions() const;

    /// Get ambush positions for defenders
    std::vector<Position> GetAmbushPositions() const;

    /// Get chokepoint positions
    std::vector<Position> GetChokepoints() const;

    /// Get sniper positions
    std::vector<Position> GetSniperPositions() const;

protected:
    // ========================================================================
    // SIEGE ABSTRACT IMPLEMENTATIONS
    // ========================================================================

    std::vector<BGObjectiveData> GetTowerData() const override;
    std::vector<BGObjectiveData> GetGraveyardData() const override;
    std::vector<BGObjectiveData> GetGateData() const override;

    uint32 GetStartingReinforcements() const override { return 0; }  // No reinforcements in SOTA
    uint32 GetReinforcementLossPerDeath() const override { return 0; }
    uint32 GetReinforcementLossPerTower() const override { return 0; }

    // ========================================================================
    // PHASE MANAGEMENT
    // ========================================================================

    /// Game phase enumeration for SOTA
    enum class SOTAPhase
    {
        PREP,            // Preparation phase (60 seconds)
        BEACH_ASSAULT,   // Landing and initial push
        OUTER_GATES,     // Attacking Green Jade / Blue Sapphire
        MIDDLE_GATES,    // Attacking Red Sun / Purple Amethyst
        INNER_GATE,      // Attacking Yellow Moon
        ANCIENT_GATE,    // Final push to Chamber
        RELIC_CAPTURE,   // Capturing Titan Relic
        DEFENSE,         // General defense (if defending)
        DESPERATE        // Low time remaining
    };

    /// Determine current game phase
    SOTAPhase GetCurrentPhase() const;

    /// Get phase name for logging
    const char* GetPhaseName(SOTAPhase phase) const;

    /// Apply phase-specific strategy
    void ApplyPhaseStrategy(StrategicDecision& decision, SOTAPhase phase) const;

    /// Apply beach assault strategy (attackers)
    void ApplyBeachAssaultStrategy(StrategicDecision& decision) const;

    /// Apply outer gates strategy
    void ApplyOuterGatesStrategy(StrategicDecision& decision) const;

    /// Apply middle gates strategy
    void ApplyMiddleGatesStrategy(StrategicDecision& decision) const;

    /// Apply inner gate strategy
    void ApplyInnerGateStrategy(StrategicDecision& decision) const;

    /// Apply ancient gate strategy
    void ApplyAncientGateStrategy(StrategicDecision& decision) const;

    /// Apply relic capture strategy
    void ApplyRelicCaptureStrategy(StrategicDecision& decision) const;

    /// Apply general defense strategy
    void ApplyDefenseStrategy(StrategicDecision& decision) const;

    /// Apply desperate strategy (low time)
    void ApplyDesperateStrategy(StrategicDecision& decision) const;

private:
    // ========================================================================
    // EVENT HANDLERS
    // ========================================================================

    void OnGateDestroyed(uint32 gateId);
    void OnGraveyardCaptured(uint32 graveyardId);
    void OnRoundStarted(uint32 roundNumber);
    void OnRoundEnded(uint32 roundTime);
    void OnRelicCapture();

    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /// Update gate state tracking
    void UpdateGateStates();

    /// Determine which path to focus on
    void EvaluateAttackPath();

    /// Check if we should focus on one path
    bool ShouldFocusPath() const;

    /// Get time target for round 2
    uint32 GetRound2TimeTarget() const;

    /// Check if on track to beat round 1 time
    bool IsAheadOfPace() const;

    // ========================================================================
    // STATE TRACKING
    // ========================================================================

    // Thread-safety: OnUpdate/OnEvent writes (main thread), ExecuteStrategy reads (worker thread)
    std::atomic<uint32> m_matchStartTime{0};
    std::atomic<uint32> m_roundStartTime{0};
    std::atomic<uint32> m_lastStrategyUpdate{0};
    std::atomic<uint32> m_lastGateCheck{0};

    std::atomic<bool> m_isAttacker{false};
    std::atomic<uint32> m_currentRound{1};
    std::atomic<uint32> m_round1Time{0};  // Time attacker took in round 1 (milliseconds)
    std::atomic<bool> m_round1Victory{false};  // Did attackers win in round 1?

    // Gate tracking (small arrays — single writer, stale reads are benign)
    std::array<bool, StrandOfTheAncients::Gates::COUNT> m_gateDestroyed;
    std::array<uint32, StrandOfTheAncients::Gates::COUNT> m_gateHealth;

    // Graveyard tracking
    std::array<bool, StrandOfTheAncients::Graveyards::COUNT> m_graveyardCaptured;

    // Attack path state (written on main thread during EvaluateAttackPath, read by workers)
    // Note: AttackPath enum is small enough that torn reads won't cause logical errors
    StrandOfTheAncients::AttackPath m_currentPath = StrandOfTheAncients::AttackPath::SPLIT;
    std::atomic<bool> m_pathDecided{false};

    // Relic state
    std::atomic<bool> m_relicCaptured{false};
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_SIEGE_STRANDOFTHEANCIENTSSCRIPT_H
