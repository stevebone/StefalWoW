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

#ifndef PLAYERBOT_AI_COORDINATION_BG_CTF_CTFSCRIPTBASE_H
#define PLAYERBOT_AI_COORDINATION_BG_CTF_CTFSCRIPTBASE_H

#include "BGScriptBase.h"
#include "Common.h"
#include <atomic>
#include <shared_mutex>

namespace Playerbot::Coordination::Battleground
{

/**
 * @brief Base class for Capture-The-Flag battlegrounds
 *
 * Provides common CTF mechanics for Warsong Gulch, Twin Peaks, and
 * any future CTF battlegrounds.
 *
 * Key CTF Mechanics:
 * - Flag pickup, carry, drop, capture, return
 * - Focused Assault / Brutal Assault debuffs (10+ minutes)
 * - Escort formations around flag carrier
 * - Flag standoff detection and handling
 */
class CTFScriptBase : public BGScriptBase
{
public:
    CTFScriptBase() = default;
    virtual ~CTFScriptBase() = default;

    // ========================================================================
    // CTF specific
    // ========================================================================

    virtual bool IsCTF() const override { return true; }

    // Default CTF values
    virtual uint32 GetMaxScore() const override { return 3; }
    virtual uint32 GetMaxDuration() const override { return 25 * MINUTE * IN_MILLISECONDS; }
    virtual uint8 GetTeamSize() const override { return 10; }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;
    void OnUpdate(uint32 diff) override;

    // ========================================================================
    // CTF-SPECIFIC IMPLEMENTATIONS
    // ========================================================================

    std::vector<Position> GetEscortFormation(
        const Position& fcPos, uint8 escortCount) const;

    std::vector<Position> GetFlagRoomPositions(uint32 faction) const;

    uint32 GetFlagDebuffSpellId(uint8 stackCount) const;

    // ========================================================================
    // STRATEGY - CTF overrides
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
    // EVENT HANDLING
    // ========================================================================

    void OnEvent(const BGScriptEventData& event) override;
    void OnMatchStart() override;

protected:
    // ========================================================================
    // ABSTRACT - MUST BE IMPLEMENTED BY DERIVED CLASSES
    // ========================================================================

    /**
     * @brief Get the alliance flag position
     */
    virtual Position GetAllianceFlagPosition() const = 0;

    /**
     * @brief Get the horde flag position
     */
    virtual Position GetHordeFlagPosition() const = 0;

    /**
     * @brief Get flag room defensive positions for alliance
     */
    virtual std::vector<Position> GetAllianceFlagRoomDefense() const = 0;

    /**
     * @brief Get flag room defensive positions for horde
     */
    virtual std::vector<Position> GetHordeFlagRoomDefense() const = 0;

    /**
     * @brief Get middle map chokepoint positions
     */
    virtual std::vector<Position> GetMiddleChokepoints() const = 0;

    /**
     * @brief Get speed buff positions
     */
    virtual std::vector<Position> GetSpeedBuffPositions() const = 0;

    /**
     * @brief Get restoration buff positions (health/mana)
     */
    virtual std::vector<Position> GetRestoreBuffPositions() const = 0;

    /**
     * @brief Get berserk buff positions
     */
    virtual std::vector<Position> GetBerserkBuffPositions() const = 0;

    // ========================================================================
    // CTF HELPERS
    // ========================================================================

    /**
     * @brief Check if there's a flag standoff (both flags taken)
     */
    bool IsStandoff() const;

    /**
     * @brief Get the match score difference (positive = we're winning)
     */
    int32 GetScoreDifference() const;

    /**
     * @brief Check if the match is in overtime (brutal assault active)
     */
    bool IsOvertime() const { return m_isOvertime; }

    /**
     * @brief Get time since flag was picked up (for debuff tracking)
     */
    uint32 GetFlagHoldTime(bool isFriendly) const;

    /**
     * @brief Calculate debuff stacks based on time
     */
    uint8 CalculateDebuffStacks(uint32 holdTime) const;

    /**
     * @brief Get recommended escort count based on situation
     */
    uint8 GetRecommendedEscortCount(bool weHaveFlag, bool theyHaveFlag,
        uint32 timeRemaining, int32 scoreDiff) const;

    /**
     * @brief Get recommended hunter count for EFC
     */
    uint8 GetRecommendedHunterCount(bool weHaveFlag, bool theyHaveFlag,
        uint32 timeRemaining, int32 scoreDiff) const;

    /**
     * @brief Determine if FC should run or hide
     */
    enum class FCTactic { RUN_HOME, KITE_MIDDLE, HIDE_BASE, AGGRESSIVE_PUSH };
    FCTactic GetFCTactic(bool standoff, uint8 debuffStacks,
        uint8 escortCount, uint32 timeRemaining) const;

    /**
     * @brief Calculate escort formation positions
     * @param center FC position
     * @param heading Direction FC is facing
     * @param count Number of escorts
     * @param radius Formation radius
     */
    std::vector<Position> CalculateEscortRing(
        const Position& center, float heading,
        uint8 count, float radius) const;

    // ========================================================================
    // RUNTIME BEHAVIOR METHODS (for ExecuteStrategy)
    // ========================================================================

    /**
     * @brief Refresh flag carrier state from aura scanning
     * Throttled to once per second. Updates m_cachedFriendlyFC and m_cachedEnemyFC.
     */
    void RefreshFlagState(::Player* bot);

    /**
     * @brief Check if player is carrying a flag (alliance or horde)
     */
    static bool IsPlayerCarryingFlag(::Player* player);

    /**
     * @brief Get FC route waypoints for flag running.
     * Default returns empty (straight-line). Override in derived scripts
     * to provide route evasion (DIRECT/NORTH/SOUTH).
     * @param faction The FC's faction
     * @param enemyPositions Known enemy positions for risk evaluation
     * @return Route waypoints, empty means straight-line to capture point
     */
    virtual std::vector<Position> GetFCRouteWaypoints(uint32 faction,
        const std::vector<Position>& enemyPositions) const { return {}; }

    /**
     * @brief Run the carried flag home to the capture point
     * Handles movement to our flag room + interaction with flag stand GO.
     * Uses route evasion when available (via GetFCRouteWaypoints).
     * Attacks enemies en route but NEVER stops moving.
     * @return true if behavior was executed
     */
    bool RunFlagHome(::Player* bot);

    /**
     * @brief Go to enemy flag location and pick it up
     * Uses phase-ignoring GO search for flag stand.
     * @return true if behavior was executed
     */
    bool PickupEnemyFlag(::Player* bot);

    /**
     * @brief Chase and attack the enemy flag carrier
     * @return true if behavior was executed
     */
    bool HuntEnemyFC(::Player* bot, ::Player* enemyFC);

    /**
     * @brief Escort the friendly flag carrier in formation
     * Attacks enemies threatening the FC.
     * @return true if behavior was executed
     */
    bool EscortFriendlyFC(::Player* bot, ::Player* friendlyFC);

    /**
     * @brief Defend own flag room: patrol, engage enemies, return dropped flags
     * @return true if behavior was executed
     */
    bool DefendOwnFlagRoom(::Player* bot);

    /**
     * @brief Find and return a dropped friendly flag (GAMEOBJECT_TYPE_FLAGDROP)
     * @return true if a dropped flag was found and bot is interacting/moving to it
     */
    bool ReturnDroppedFlag(::Player* bot);

    // ========================================================================
    // CTF STATE
    // ========================================================================

    // Cached flag carriers (refreshed by RefreshFlagState)
    // NOTE: These raw pointers are protected by m_flagStateMutex.
    // Always acquire a shared_lock before reading, unique_lock before writing.
    ::Player* m_cachedFriendlyFC = nullptr;
    ::Player* m_cachedEnemyFC = nullptr;
    std::atomic<uint32> m_lastFlagStateRefresh{0};
    mutable std::shared_mutex m_flagStateMutex;
    static constexpr uint32 FLAG_STATE_REFRESH_INTERVAL = 1000; // 1 second

    // Flag states (tracked from events — written on main, read on workers)
    std::atomic<bool> m_allianceFlagTaken{false};
    std::atomic<bool> m_hordeFlagTaken{false};
    ObjectGuid m_allianceFC;
    ObjectGuid m_hordeFC;
    std::atomic<uint32> m_allianceFlagPickupTime{0};
    std::atomic<uint32> m_hordeFlagPickupTime{0};

    // Score tracking
    std::atomic<uint32> m_allianceCaptures{0};
    std::atomic<uint32> m_hordeCaptures{0};

    // Overtime tracking
    std::atomic<bool> m_isOvertime{false};
    std::atomic<uint32> m_overtimeStartTime{0};

    // Performance metrics
    std::atomic<uint32> m_successfulCaptures{0};
    std::atomic<uint32> m_failedCaptures{0};  // Dropped before cap
    std::atomic<uint32> m_flagReturns{0};

    // FC route tracking - stores selected route waypoints and progress per FC
    struct FCRouteState
    {
        std::vector<Position> waypoints;
        uint32 currentWaypointIndex = 0;
        uint32 routeSelectedTime = 0;
    };
    mutable std::map<ObjectGuid, FCRouteState> m_fcRouteStates;
    mutable std::shared_mutex m_fcRouteStateMutex;

private:
    // Internal update timers
    uint32 m_debuffCheckTimer = 0;
    static constexpr uint32 DEBUFF_CHECK_INTERVAL = 5000;

    // Debuff thresholds (in milliseconds of holding)
    static constexpr uint32 FOCUSED_ASSAULT_START = 10 * MINUTE * IN_MILLISECONDS;
    static constexpr uint32 BRUTAL_ASSAULT_START = 15 * MINUTE * IN_MILLISECONDS;
};

// ============================================================================
// CTF SPELL IDS
// ============================================================================

namespace CTFSpells
{
    constexpr uint32 ALLIANCE_FLAG_CARRIED = 23333;  // Alliance Flag aura
    constexpr uint32 HORDE_FLAG_CARRIED = 23335;     // Horde Flag aura

    constexpr uint32 FOCUSED_ASSAULT = 46392;        // Stacking debuff
    constexpr uint32 BRUTAL_ASSAULT = 46393;         // Stronger stacking debuff

    constexpr uint32 SPEED_BUFF = 23451;             // Speed boost
    constexpr uint32 RESTORE_BUFF = 23493;           // Health/Mana restore
    constexpr uint32 BERSERK_BUFF = 23505;           // Damage boost

    constexpr uint32 FLAG_DROP = 23384;              // Flag drop spell
    constexpr uint32 FLAG_RETURN = 23385;            // Flag return spell
}

// ============================================================================
// CTF CONSTANTS
// ============================================================================

namespace CTFConstants
{
    constexpr uint32 FLAG_RESPAWN_TIME = 23000;      // 23 seconds
    constexpr float FLAG_DROP_RADIUS = 5.0f;         // Flag drops within this radius
    constexpr uint32 FLAG_DROP_TIMEOUT = 10000;      // 10 seconds before flag returns

    constexpr float ESCORT_RING_RADIUS = 8.0f;       // Escort formation radius
    constexpr float ESCORT_HEALER_OFFSET = 12.0f;    // Healers slightly further

    constexpr uint8 MIN_ESCORTS = 2;
    constexpr uint8 MAX_ESCORTS = 6;
    constexpr uint8 MIN_HUNTERS = 1;
    constexpr uint8 MAX_HUNTERS = 5;
}

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_CTF_CTFSCRIPTBASE_H
