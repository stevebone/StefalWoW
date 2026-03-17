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

#ifndef PLAYERBOT_AI_COORDINATION_BG_EPIC_ASHRANSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_EPIC_ASHRANSCRIPT_H

#include "../BGScriptBase.h"
#include "AshranData.h"
#include <atomic>
#include <shared_mutex>

namespace Playerbot::Coordination::Battleground
{

/**
 * @class AshranScript
 * @brief Enterprise-grade Ashran epic battleground script
 *
 * Ashran is a persistent PvP zone featuring:
 * - Road of Glory: Linear push-pull control point warfare (10-75 vs 10-75)
 * - 3 Control Points: Stormshield Stronghold, Crossroads, Warspear Stronghold
 * - 8 Side Events: Provide buffs, resources, and tactical advantages
 * - Faction Leaders: High Warlord Volrath (Horde) and Grand Marshal Tremblade (Alliance)
 * - Victory Condition: Kill enemy faction leader
 *
 * Key strategies:
 * - Control the Crossroads for central advantage
 * - Participate in high-value events (Seat of Omen, Ring of Conquest)
 * - Push road when holding 2+ control points
 * - Coordinate boss assault with 20+ players
 * - Ambush enemy pushes at chokepoints
 *
 * This script provides:
 * - 32+ control point defense positions
 * - 12 road chokepoints
 * - 8 sniper/overlook positions
 * - Boss approach routes with raid positioning
 * - Event participation logic
 * - Phase-aware strategy (opening, road push, event focus, boss assault)
 */
class AshranScript : public BGScriptBase
{
public:
    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return Ashran::MAP_ID; }
    std::string GetName() const override { return Ashran::BG_NAME; }
    BGType GetBGType() const override { return BGType::ASHRAN; }
    uint32 GetMaxScore() const override { return 0; }  // No score limit - kill boss to win
    uint32 GetMaxDuration() const override { return Ashran::MAX_DURATION; }
    uint8 GetTeamSize() const override { return Ashran::MAX_TEAM_SIZE; }
    bool IsDomination() const override { return false; }  // Epic battleground

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
    // WORLD STATE INTERPRETATION
    // ========================================================================

    bool InterpretWorldState(int32 stateId, int32 value, uint32& outObjectiveId, BGObjectiveState& outState) const override;
    void GetScoreFromWorldStates(const std::map<int32, int32>& states, uint32& allianceScore, uint32& hordeScore) const override;

    // ========================================================================
    // STRATEGY & ROLE DISTRIBUTION
    // ========================================================================

    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision, float scoreAdvantage, uint32 timeRemaining) const override;
    void AdjustStrategy(StrategicDecision& decision, float scoreAdvantage, uint32 controlledCount, uint32 totalObjectives, uint32 timeRemaining) const override;

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // ASHRAN-SPECIFIC METHODS
    // ========================================================================

    /// Get road progress for a faction (0.0 = at own base, 1.0 = at enemy base)
    float GetRoadProgress(uint32 faction) const;

    /// Check if a side event is currently active
    bool IsEventActive() const { return m_activeEvent < Ashran::Events::EVENT_COUNT; }

    /// Get the currently active event ID
    uint32 GetActiveEvent() const { return m_activeEvent; }

    /// Get position for a specific event
    Position GetEventPosition(uint32 eventId) const;

    /// Determine if faction should participate in an event
    bool ShouldParticipateInEvent(uint32 eventId) const;

    /// Check if enemy base is vulnerable (can push to boss)
    bool IsEnemyBaseVulnerable(uint32 attackingFaction) const;

    /// Get the priority value for an event (higher = more important)
    uint8 GetEventPriority(uint32 eventId) const;

    /// Check if we control the crossroads
    bool ControlsCrossroads(uint32 faction) const;

    /// Get number of control points held by faction
    uint32 GetControlledPointCount(uint32 faction) const;

    // ========================================================================
    // ENTERPRISE-GRADE POSITIONING
    // ========================================================================

    /// Get chokepoint positions along the Road of Glory
    std::vector<Position> GetChokepoints() const;

    /// Get sniper/overlook positions
    std::vector<Position> GetSniperPositions() const;

    /// Get ambush positions for a faction
    std::vector<Position> GetAmbushPositions(uint32 faction) const;

    /// Get defense positions for a control point
    std::vector<Position> GetControlPointDefensePositions(uint32 pointId) const;

    /// Get positions for a specific event
    std::vector<Position> GetEventPositions(uint32 eventId) const;

    /// Get boss approach route for a faction
    std::vector<Position> GetBossApproachRoute(uint32 faction) const;

    /// Get raid positions for boss assault
    std::vector<Position> GetBossRaidPositions(uint32 faction) const;

    /// Get Road of Glory waypoints
    std::vector<Position> GetRoadWaypoints() const;

    /// Calculate distance between two positions
    float CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) const;

protected:
    // ========================================================================
    // PHASE MANAGEMENT
    // ========================================================================

    /// Game phase enumeration for strategy adjustment
    enum class AshranPhase
    {
        OPENING,        // First 2 minutes - initial push to crossroads
        ROAD_PUSH,      // Normal gameplay - control road
        EVENT_FOCUS,    // High-priority event active
        BOSS_ASSAULT,   // Push to kill enemy boss
        DEFENSE         // Enemy threatening our base
    };

    /// Determine current game phase
    AshranPhase GetCurrentPhase() const;

    /// Get string name for phase (for logging)
    const char* GetPhaseName(AshranPhase phase) const;

    /// Apply phase-specific strategy adjustments
    void ApplyPhaseStrategy(StrategicDecision& decision, AshranPhase phase, float scoreAdvantage) const;

    /// Apply opening phase strategy
    void ApplyOpeningPhaseStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply road push phase strategy
    void ApplyRoadPushStrategy(StrategicDecision& decision, uint32 faction, float ourProgress, float enemyProgress) const;

    /// Apply event focus strategy
    void ApplyEventFocusStrategy(StrategicDecision& decision, uint32 faction, uint32 eventId) const;

    /// Apply boss assault strategy
    void ApplyBossAssaultStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply defensive strategy
    void ApplyDefensiveStrategy(StrategicDecision& decision, uint32 faction, float enemyProgress) const;

private:
    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /// Get objective data for a control point
    BGObjectiveData GetControlPointData(uint32 pointId) const;

    /// Get objective data for an event
    BGObjectiveData GetEventObjectiveData(uint32 eventId) const;

    /// Update road progress tracking
    void UpdateRoadProgress();

    /// Handle event-related updates
    void UpdateEventStatus();

    /// Queue boss NPC attack via BotActionMgr (deferred to main thread)
    void QueueBossAttack(::Player* bot, uint32 targetFaction);

    // ========================================================================
    // STATE TRACKING
    // ========================================================================

    // Thread-safety: OnUpdate/OnEvent writes (main thread), ExecuteStrategy reads (worker thread)
    // C++20 std::atomic<float> is supported by MSVC and is lock-free for IEEE754
    std::atomic<float> m_allianceProgress{0.0f};   // Road position (0 = Alliance base, 1 = Horde base)
    std::atomic<float> m_hordeProgress{0.0f};      // Road position (0 = Horde base, 1 = Alliance base)
    std::atomic<uint32> m_activeEvent{UINT32_MAX}; // Current active side event (UINT32_MAX = none)
    std::atomic<uint32> m_eventTimer{0};           // Time remaining for current event
    std::atomic<uint32> m_matchStartTime{0};       // Timestamp of match start
    std::atomic<uint32> m_lastRoadUpdate{0};       // Last road progress update time
    std::atomic<uint32> m_lastStrategyUpdate{0};   // Last strategy evaluation time
    std::map<uint32, BGObjectiveState> m_controlStates;  // Control point states
    mutable std::shared_mutex m_controlStateMutex;
    std::atomic<bool> m_trembladeAlive{true};      // Alliance leader status
    std::atomic<bool> m_volrathAlive{true};        // Horde leader status

    // Cached boss GUIDs (resolved on main thread in OnUpdate)
    ObjectGuid m_trembladeGuid;
    ObjectGuid m_volrathGuid;
    std::atomic<bool> m_bossGuidsResolved{false};
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_EPIC_ASHRANSCRIPT_H
