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

#ifndef PLAYERBOT_AI_COORDINATION_BG_DOMINATION_TEMPLEOFKOTMOGUSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_DOMINATION_TEMPLEOFKOTMOGUSCRIPT_H

#include "DominationScriptBase.h"
#include "TempleOfKotmoguData.h"
#include "BGPositionDiscovery.h"
#include <memory>

namespace Playerbot::Coordination::Battleground
{

/**
 * @class TempleOfKotmoguScript
 * @brief Enterprise-grade Temple of Kotmogu battleground script
 *
 * Temple of Kotmogu is a 10v10 orb-based battleground with unique mechanics:
 * - 4 orbs spawn at temple corners (Orange, Blue, Green, Purple)
 * - Players pick up and carry orbs to score points
 * - Center zone provides bonus points (15 pts/orb vs 3 pts/orb outside)
 * - Orb carriers take increasing damage over time
 * - First to 1500 points wins
 *
 * Key strategies:
 * - Grab orbs quickly at game start
 * - Move to center when team has 2+ orbs with escort
 * - Protect orb carriers (they're high-value targets)
 * - Kill enemy orb carriers before they reach center
 *
 * This script provides:
 * - 32 orb defense positions (8 per orb spawn)
 * - 12 center zone positions
 * - Pre-calculated orb carrier routes
 * - Dynamic escort formations
 * - Phase-aware strategy (opening, mid-game, late game)
 */
class TempleOfKotmoguScript : public DominationScriptBase
{
public:
    // Bring base class methods into scope to avoid hiding warnings
    using DominationScriptBase::GetEscortFormation;

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return TempleOfKotmogu::MAP_ID; }
    std::string GetName() const override { return TempleOfKotmogu::BG_NAME; }
    BGType GetBGType() const override { return BGType::TEMPLE_OF_KOTMOGU; }
    uint32 GetMaxScore() const override { return TempleOfKotmogu::MAX_SCORE; }
    uint32 GetMaxDuration() const override { return TempleOfKotmogu::MAX_DURATION; }
    uint8 GetTeamSize() const override { return TempleOfKotmogu::TEAM_SIZE; }
    uint32 GetOptimalNodeCount() const override { return 2; }  // 2 orbs is good

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
    // STRATEGY & ROLE DISTRIBUTION
    // ========================================================================

    void AdjustStrategy(StrategicDecision& decision, float scoreAdvantage, uint32 controlledCount, uint32 totalObjectives, uint32 timeRemaining) const override;
    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision, float scoreAdvantage, uint32 timeRemaining) const override;

    // ========================================================================
    // ORB-SPECIFIC METHODS
    // ========================================================================

    /// Check if an orb is currently held by any player
    bool IsOrbHeld(uint32 orbId) const { return m_orbHolders.find(orbId) != m_orbHolders.end(); }

    /// Get the player holding a specific orb
    ObjectGuid GetOrbHolder(uint32 orbId) const;

    /// Check if a specific player is holding any orb
    bool IsPlayerHoldingOrb(ObjectGuid guid) const;

    /// Get the orb ID held by a player (-1 if none)
    int32 GetPlayerOrbId(ObjectGuid guid) const;

    /// Get count of orbs held by a faction
    uint32 GetOrbsHeldByFaction(uint32 faction) const;

    /// Check if a position is within the center bonus zone
    bool IsInCenter(float x, float y) const;

    /// Get the pre-calculated route from an orb spawn to center
    std::vector<Position> GetOrbCarrierRoute(uint32 orbId) const;

    /// Get dynamic escort formation around an orb carrier
    std::vector<Position> GetEscortFormation(float carrierX, float carrierY, float carrierZ) const;

    /// Check if it's safe to push to center (have escort, not outnumbered)
    bool ShouldPushToCenter(uint32 faction) const;

    /// Get the priority order for grabbing orbs
    std::vector<uint32> GetOrbPriority(uint32 faction) const;

    // ========================================================================
    // RUNTIME BEHAVIOR (lighthouse pattern)
    // ========================================================================

    /// Execute TOK-specific strategy for a bot (overrides IBGScript)
    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // ENTERPRISE-GRADE POSITIONING
    // ========================================================================

    /// Get all chokepoint positions
    std::vector<Position> GetChokepoints() const;

    /// Get all sniper/overlook positions
    std::vector<Position> GetSniperPositions() const;

    /// Get all buff locations
    std::vector<Position> GetBuffPositions() const;

    /// Get ambush positions for a faction
    std::vector<Position> GetAmbushPositions(uint32 faction) const;

    /// Get center zone defense positions
    std::vector<Position> GetCenterDefensePositions() const;

    /// Get defense positions for an orb spawn
    std::vector<Position> GetOrbDefensePositions(uint32 orbId) const;

    /// Get distance between two orbs
    float GetOrbToOrbDistance(uint32 fromOrb, uint32 toOrb) const;

    /// Get distance from an orb to center
    float GetOrbToCenterDistance(uint32 orbId) const;

    /// Get orb position (uses dynamic discovery if available, falls back to hardcoded)
    Position GetDynamicOrbPosition(uint32 orbId) const;

protected:
    // ========================================================================
    // BASE CLASS OVERRIDES
    // ========================================================================

    uint32 GetNodeCount() const override { return TempleOfKotmogu::ORB_COUNT; }
    BGObjectiveData GetNodeData(uint32 nodeIndex) const override;
    std::vector<uint32> GetTickPointsTable() const override;
    uint32 GetTickInterval() const override { return TempleOfKotmogu::TICK_INTERVAL; }
    uint32 GetDefaultCaptureTime() const override { return 0; }  // Orbs are instant pickup

private:
    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    // ========================================================================
    // RUNTIME BEHAVIOR HELPERS
    // ========================================================================

    /// Pick up the nearest available orb
    bool PickupOrb(::Player* player);

    /// Defend nearest friendly orb carrier or patrol center
    bool DefendOrbCarrier(::Player* player);

    /// Hunt and attack enemy orb carriers
    bool HuntEnemyOrbCarrier(::Player* player);

    /// Escort nearest friendly orb carrier in formation
    bool EscortOrbCarrier(::Player* player);

    /// Move orb carrier toward center or hold position
    bool ExecuteOrbCarrierMovement(::Player* player);

    BGObjectiveData GetOrbData(uint32 orbId) const;

    /// Calculate 3D distance between two points
    float CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) const
    {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dz = z2 - z1;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /// Determine current game phase
    enum class GamePhase { OPENING, MID_GAME, LATE_GAME, DESPERATE };
    GamePhase GetCurrentPhase() const;

    /// Apply phase-specific strategy
    void ApplyPhaseStrategy(StrategicDecision& decision, GamePhase phase, float scoreAdvantage) const;

    /// Phase hysteresis: track last phase to prevent rapid oscillation
    mutable GamePhase m_lastPhase = GamePhase::OPENING;

    // ========================================================================
    // STATE TRACKING
    // ========================================================================

    std::map<uint32, ObjectGuid> m_orbHolders;  // orbId -> holder guid
    std::map<ObjectGuid, uint32> m_playerOrbs;  // player guid -> orbId
    uint32 m_allianceOrbsHeld = 0;
    uint32 m_hordeOrbsHeld = 0;

    /// orbId -> GUID of bot currently moving toward this orb for pickup
    std::map<uint32, ObjectGuid> m_orbTargeters;

    /// orbId -> GameTimeMS when claim expires (prevents dual pickup race condition)
    std::map<uint32, uint32> m_orbClaimedUntil;

    /// orbId -> GameTimeMS when search-failed cooldown expires
    /// When PickupOrb finds 0 GOs at an orb spawn, we mark that orbId as temporarily
    /// unavailable for 15 seconds. This prevents the infinite loop where bots keep
    /// re-targeting a "free" orb that has no physical GO (dynamic respawn timing).
    std::map<uint32, uint32> m_orbSearchFailed;

    /// Pending orb pickup: bot has queued Use() via BotActionMgr and must hold position
    /// at the orb until the main thread processes the deferred action. Without this,
    /// the bot moves away on the next worker tick and the GO's CastSpell range check
    /// fails silently.
    struct PendingPickup
    {
        uint32 orbId;
        Position orbPosition;
        uint32 queuedTime;     // GameTimeMS when Use() was queued
    };
    std::map<ObjectGuid, PendingPickup> m_pendingOrbPickup;

    /// Timestamp for throttling RefreshOrbState() to once per second
    uint32 m_lastOrbRefresh = 0;

    /// Scan all BG players for orb auras and rebuild m_orbHolders/m_playerOrbs
    void RefreshOrbState();

    // ========================================================================
    // DYNAMIC POSITION DISCOVERY
    // ========================================================================

    /**
     * @brief Dynamic position discovery system
     *
     * Discovers actual orb positions from game objects at runtime instead of
     * relying on hardcoded coordinates which may be wrong for the current map.
     */
    std::unique_ptr<BGPositionDiscovery> m_positionDiscovery;

    /**
     * @brief Cached orb positions (dynamically discovered or fallback to hardcoded)
     */
    std::array<Position, TempleOfKotmogu::ORB_COUNT> m_orbPositions;

    /**
     * @brief Whether dynamic discovery has been completed
     */
    bool m_positionsDiscovered = false;

    /**
     * @brief Initialize dynamic position discovery
     * @return true if dynamic discovery succeeded
     */
    bool InitializePositionDiscovery();

};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_DOMINATION_TEMPLEOFKOTMOGUSCRIPT_H
