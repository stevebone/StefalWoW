/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

/**
 * ============================================================================
 * DUNGEON AUTONOMY MANAGER
 * ============================================================================
 *
 * Provides autonomous dungeon navigation and coordination for bot groups.
 * This manager bridges DungeonCoordinator (state management) with BotAI
 * (decision making) to enable bots to progress through dungeons without
 * constant player input.
 *
 * KEY FEATURES:
 * - Pause/Resume functionality (critical safeguard)
 * - Tank-driven pulling decisions
 * - Group movement coordination
 * - Integration with DungeonScript for encounter mechanics
 * - Configurable aggression levels
 *
 * USAGE:
 * - Called from BotAI::UpdateAI() when in dungeon
 * - Tank bots decide when to pull
 * - Other bots follow tank and engage accordingly
 * - Player can pause at any time with .bot dungeon pause
 *
 * ============================================================================
 */

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "Threading/LockHierarchy.h"
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>

class Player;
class Group;
class Creature;
class Map;

namespace Playerbot
{

// Forward declarations
class DungeonCoordinator;
class DungeonScript;
class BotAI;

/**
 * @brief Autonomy state for a group in dungeon
 */
enum class DungeonAutonomyState : uint8
{
    DISABLED = 0,       // Autonomy off - manual control only
    PAUSED = 1,         // Temporarily paused by player command
    ACTIVE = 2,         // Fully autonomous
    WAITING = 3,        // Waiting for conditions (mana, health, cooldowns)
    PULLING = 4,        // Tank is pulling
    COMBAT = 5,         // In combat
    RECOVERING = 6      // Post-combat recovery (drinking, rezzing)
};

/**
 * @brief Aggression level for autonomous pulling
 */
enum class DungeonAggressionLevel : uint8
{
    CONSERVATIVE = 0,   // Wait for full mana/health, single pulls only
    NORMAL = 1,         // Standard pulling, reasonable recovery time
    AGGRESSIVE = 2,     // Quick pulls, minimal recovery
    SPEED_RUN = 3       // Maximum speed, chain pulling
};

/**
 * @brief Configuration for dungeon autonomy
 */
struct DungeonAutonomyConfig
{
    DungeonAggressionLevel aggressionLevel = DungeonAggressionLevel::NORMAL;
    float minHealthToPull = 0.7f;           // 70% group health minimum
    float minManaToPull = 0.5f;             // 50% healer mana minimum
    uint32 recoveryTimeMs = 5000;           // 5 seconds between pulls
    uint32 maxPullSize = 1;                 // Max trash packs per pull
    bool allowChainPulling = false;         // Pull before current combat ends
    bool waitForSlowMembers = true;         // Wait for all members before pull
    float maxMemberDistance = 40.0f;        // Max distance from tank to pull
    bool autoMarkTargets = true;            // Auto-mark skull/X on pull
    bool respectCCAssignments = true;       // Honor CC from DungeonCoordinator
};

/**
 * @brief Per-group autonomy state
 */
struct GroupAutonomyState
{
    ObjectGuid groupLeaderGuid;
    DungeonAutonomyState state = DungeonAutonomyState::DISABLED;
    DungeonAutonomyConfig config;
    uint32 lastPullTime = 0;
    uint32 lastStateChangeTime = 0;
    uint32 currentPackId = 0;               // Current trash pack being pulled
    Position lastTankPosition;
    bool tankMovingToTarget = false;
    uint32 pausedByPlayerGuid = 0;          // Who paused (0 = not paused)
    std::string pauseReason;

    GroupAutonomyState() = default;
    explicit GroupAutonomyState(ObjectGuid leader) : groupLeaderGuid(leader) {}
};

/**
 * @brief Main autonomy manager (singleton)
 */
class TC_GAME_API DungeonAutonomyManager
{
public:
    static DungeonAutonomyManager* instance();

    // ========================================================================
    // PAUSE/RESUME CONTROLS (Critical Safeguard)
    // ========================================================================

    /**
     * @brief Pause autonomous dungeon behavior for a group
     * @param group The group to pause
     * @param pausedBy Player who issued pause command (for logging)
     * @param reason Optional reason for pause
     * @return true if paused successfully
     */
    bool PauseDungeonAutonomy(Group* group, Player* pausedBy, const std::string& reason = "");

    /**
     * @brief Resume autonomous dungeon behavior for a group
     * @param group The group to resume
     * @param resumedBy Player who issued resume command
     * @return true if resumed successfully
     */
    bool ResumeDungeonAutonomy(Group* group, Player* resumedBy);

    /**
     * @brief Toggle pause state
     * @param group The group to toggle
     * @param toggledBy Player who issued command
     * @return New state (true = paused, false = active)
     */
    bool ToggleDungeonPause(Group* group, Player* toggledBy);

    /**
     * @brief Check if group autonomy is paused
     */
    bool IsPaused(Group* group) const;

    /**
     * @brief Check if group has autonomy enabled (not disabled)
     */
    bool IsAutonomyEnabled(Group* group) const;

    // ========================================================================
    // AUTONOMY CONTROL
    // ========================================================================

    /**
     * @brief Enable autonomous dungeon navigation for a group
     * @param group The group to enable
     * @param config Configuration settings
     */
    void EnableAutonomy(Group* group, const DungeonAutonomyConfig& config = {});

    /**
     * @brief Disable autonomous dungeon navigation
     * @param group The group to disable
     */
    void DisableAutonomy(Group* group);

    /**
     * @brief Set aggression level
     */
    void SetAggressionLevel(Group* group, DungeonAggressionLevel level);

    /**
     * @brief Get current autonomy state for a group
     */
    DungeonAutonomyState GetAutonomyState(Group* group) const;

    /**
     * @brief Get configuration for a group
     */
    DungeonAutonomyConfig GetConfig(Group* group) const;

    /**
     * @brief Update configuration
     */
    void UpdateConfig(Group* group, const DungeonAutonomyConfig& config);

    // ========================================================================
    // MAIN UPDATE LOOP (Called from BotAI)
    // ========================================================================

    /**
     * @brief Main update for a bot in a dungeon
     *
     * This is called from BotAI::UpdateAI() for each bot when in a dungeon.
     * It coordinates with DungeonCoordinator and DungeonScript to make
     * autonomous navigation decisions.
     *
     * @param bot The bot being updated
     * @param ai The bot's AI instance
     * @param diff Time delta in milliseconds
     * @return true if autonomy handled this update (bot should skip normal AI)
     */
    bool UpdateBotInDungeon(Player* bot, BotAI* ai, uint32 diff);

    // ========================================================================
    // TANK-SPECIFIC DECISIONS
    // ========================================================================

    /**
     * @brief Check if tank should pull the next pack
     * @param tank The tank bot
     * @param group The group
     * @param coordinator The dungeon coordinator
     * @return true if tank should initiate pull
     */
    bool ShouldTankPull(Player* tank, Group* group, DungeonCoordinator* coordinator);

    /**
     * @brief Execute tank pull
     * @param tank The tank bot
     * @param target The pull target (creature or pack)
     */
    void ExecuteTankPull(Player* tank, Creature* target);

    /**
     * @brief Check if tank should move to next objective
     */
    bool ShouldTankAdvance(Player* tank, Group* group, DungeonCoordinator* coordinator);

    // ========================================================================
    // GROUP COORDINATION
    // ========================================================================

    /**
     * @brief Check if group is ready to pull
     * @param group The group
     * @return true if health/mana/positions are acceptable
     */
    bool IsGroupReadyToPull(Group* group) const;

    /**
     * @brief Check if all members are in range
     * @param group The group
     * @param position Reference position (usually tank)
     * @param maxDistance Maximum allowed distance
     */
    bool AreAllMembersInRange(Group* group, const Position& position, float maxDistance) const;

    /**
     * @brief Get average group health percentage
     */
    float GetGroupHealthPercent(Group* group) const;

    /**
     * @brief Get average healer mana percentage
     */
    float GetHealerManaPercent(Group* group) const;

    // ========================================================================
    // GLOBAL UPDATE & CLEANUP
    // ========================================================================

    /**
     * @brief Global update tick (called periodically)
     */
    void Update(uint32 diff);

    /**
     * @brief Cleanup state for disbanded group
     */
    void OnGroupDisbanded(Group* group);

    /**
     * @brief Cleanup state when leaving dungeon
     */
    void OnLeaveDungeon(Group* group);

    // ========================================================================
    // DUNGEON COORDINATOR INTEGRATION
    // ========================================================================

    /**
     * @brief Get or create DungeonCoordinator for a group
     */
    DungeonCoordinator* GetOrCreateCoordinator(Group* group);

    /**
     * @brief Get existing coordinator (may be null)
     */
    DungeonCoordinator* GetCoordinator(Group* group) const;

private:
    DungeonAutonomyManager();
    ~DungeonAutonomyManager();

    // Non-copyable
    DungeonAutonomyManager(const DungeonAutonomyManager&) = delete;
    DungeonAutonomyManager& operator=(const DungeonAutonomyManager&) = delete;

    // ========================================================================
    // INTERNAL STATE
    // ========================================================================

    // Group autonomy states (groupId -> state)
    std::unordered_map<uint32, GroupAutonomyState> _groupStates;

    // DungeonCoordinators per group (groupId -> coordinator)
    std::unordered_map<uint32, std::unique_ptr<DungeonCoordinator>> _coordinators;

    // Thread safety
    mutable OrderedRecursiveMutex<LockOrder::BEHAVIOR_MANAGER> _mutex;

    // Global update timer
    uint32 _updateTimer = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 500;

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    /**
     * @brief Get or create state for a group
     */
    GroupAutonomyState& GetOrCreateState(Group* group);

    /**
     * @brief Get state (const, may return empty)
     */
    const GroupAutonomyState* GetState(Group* group) const;

    /**
     * @brief Transition to new autonomy state
     */
    void TransitionState(Group* group, DungeonAutonomyState newState);

    /**
     * @brief Handle tank AI update
     */
    bool UpdateTankAI(Player* tank, BotAI* ai, Group* group, DungeonCoordinator* coordinator, uint32 diff);

    /**
     * @brief Handle healer AI update
     */
    bool UpdateHealerAI(Player* healer, BotAI* ai, Group* group, DungeonCoordinator* coordinator, uint32 diff);

    /**
     * @brief Handle DPS AI update
     */
    bool UpdateDpsAI(Player* dps, BotAI* ai, Group* group, DungeonCoordinator* coordinator, uint32 diff);

    /**
     * @brief Check if bot is tank role
     */
    bool IsTankRole(Player* player, Group* group) const;

    /**
     * @brief Check if bot is healer role
     */
    bool IsHealerRole(Player* player, Group* group) const;

    /**
     * @brief Get tank from group
     */
    Player* GetGroupTank(Group* group) const;

    /**
     * @brief Get next pull target from coordinator
     */
    Creature* GetNextPullTarget(Group* group, DungeonCoordinator* coordinator) const;

    /**
     * @brief Mark target for group
     */
    void MarkPullTarget(Creature* target);

    /**
     * @brief Log state transition
     */
    void LogStateTransition(Group* group, DungeonAutonomyState oldState, DungeonAutonomyState newState);
};

#define sDungeonAutonomyMgr DungeonAutonomyManager::instance()

} // namespace Playerbot
