/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * BOT IDLE BEHAVIOR MANAGER
 *
 * Enterprise-grade idle behavior management for PlayerBot.
 * Creates natural, human-like idle behavior when bots are waiting.
 *
 * NEW: Leverages TrinityCore 12.0's MoveRandom() support for players
 * (commit 12743dd0e7) to create smooth wandering behavior.
 *
 * Features:
 * - Context-aware idle behavior (town, dungeon, combat wait)
 * - Natural wandering patterns using MoveRandom()
 * - Configurable behavior per context
 * - Performance-optimized (minimal CPU impact)
 * - Thread-safe design
 *
 * Idle Contexts:
 * - TOWN_IDLE: Wandering near mailbox/AH/bank
 * - GROUP_WAIT: Waiting for group members
 * - QUEST_WAIT: Waiting at quest location
 * - COMBAT_READY: Ready stance, minimal movement
 * - FISHING: Idle near water
 * - GUARD_PATROL: Random patrol within area
 *
 * Usage:
 *   BotIdleBehaviorManager* idleMgr = bot->GetBotIdleBehaviorManager();
 *   idleMgr->SetIdleContext(IdleContext::TOWN_IDLE);
 *   idleMgr->StartIdleBehavior();  // Starts natural wandering
 */

#pragma once

#include "Define.h"
#include "Position.h"
#include "Duration.h"
#include "Optional.h"
#include <atomic>
#include <chrono>

class Player;

namespace Playerbot
{

/**
 * Idle behavior context
 *
 * Determines the type of idle behavior based on the bot's current situation.
 * Each context has different parameters for natural behavior.
 */
enum class IdleContext : uint8
{
    NONE = 0,           // No idle behavior (combat, moving, etc.)
    TOWN_IDLE,          // Relaxed wandering in town areas
    GROUP_WAIT,         // Waiting for group members at meeting point
    QUEST_WAIT,         // Waiting at quest objective
    COMBAT_READY,       // Ready for combat, minimal movement
    FISHING,            // Idle near fishing spot
    GUARD_PATROL,       // Random patrol within defensive perimeter
    INSTANCE_WAIT,      // Waiting inside instance (limited movement)
    REST_AREA           // Resting at inn/city (can sit down)
};

/**
 * Idle behavior configuration for a specific context
 */
struct IdleBehaviorConfig
{
    float wanderRadius = 5.0f;              // Radius of wander area
    Milliseconds minWanderDuration{10000};  // Minimum time to wander
    Milliseconds maxWanderDuration{30000};  // Maximum time to wander
    Milliseconds minIdlePause{3000};        // Minimum pause between wanders
    Milliseconds maxIdlePause{8000};        // Maximum pause between wanders
    bool forceWalk = true;                  // Walk instead of run
    bool canSitDown = false;                // Can the bot sit down
    bool canEmote = true;                   // Can the bot use idle emotes
    float emoteChance = 0.1f;               // Chance to emote during idle (0.0-1.0)
};

/**
 * Idle behavior state
 */
enum class IdleBehaviorState : uint8
{
    INACTIVE,           // Idle behavior not active
    WANDERING,          // Currently wandering
    PAUSED,             // Paused between wanders
    SITTING,            // Sitting down (rest areas)
    EMOTING             // Playing an emote
};

/**
 * Bot Idle Behavior Manager
 *
 * Manages natural idle behavior for bots when they're not actively engaged.
 * Uses TrinityCore 12.0's MoveRandom() for smooth wandering.
 */
class TC_GAME_API BotIdleBehaviorManager
{
public:
    /**
     * Construct idle behavior manager for a bot
     *
     * @param bot The player bot this manager controls
     */
    explicit BotIdleBehaviorManager(Player* bot);

    ~BotIdleBehaviorManager() = default;

    // Non-copyable
    BotIdleBehaviorManager(BotIdleBehaviorManager const&) = delete;
    BotIdleBehaviorManager& operator=(BotIdleBehaviorManager const&) = delete;

    // ========================================================================
    // IDLE BEHAVIOR CONTROL
    // ========================================================================

    /**
     * Set the current idle context
     *
     * Changes the idle behavior parameters based on context.
     * Does not automatically start idle behavior.
     *
     * @param context The idle context to use
     */
    void SetIdleContext(IdleContext context);

    /**
     * Get the current idle context
     */
    IdleContext GetIdleContext() const { return _context; }

    /**
     * Start idle behavior
     *
     * Begins natural idle behavior based on current context.
     * Bot will wander around the current position.
     *
     * @param centerPos Optional center position (default: current position)
     * @return true if idle behavior started
     */
    bool StartIdleBehavior(Optional<Position> centerPos = {});

    /**
     * Stop idle behavior
     *
     * Stops wandering and returns to standing idle.
     */
    void StopIdleBehavior();

    /**
     * Check if idle behavior is active
     */
    bool IsIdleBehaviorActive() const;

    /**
     * Get current idle state
     */
    IdleBehaviorState GetState() const { return _state; }

    // ========================================================================
    // UPDATE (Called from BotAI::UpdateAI)
    // ========================================================================

    /**
     * Update idle behavior
     *
     * Should be called every BotAI update cycle.
     * Manages state transitions and initiates new wanders.
     *
     * @param diff Time since last update (milliseconds)
     */
    void Update(uint32 diff);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * Set custom configuration for a context
     *
     * @param context The context to configure
     * @param config The configuration to use
     */
    void SetContextConfig(IdleContext context, IdleBehaviorConfig const& config);

    /**
     * Get configuration for a context
     */
    IdleBehaviorConfig GetContextConfig(IdleContext context) const;

    /**
     * Enable/disable idle behavior globally
     */
    void SetEnabled(bool enabled) { _enabled = enabled; }

    /**
     * Check if idle behavior is enabled
     */
    bool IsEnabled() const { return _enabled; }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * Get total wanders performed
     */
    uint32 GetTotalWanders() const { return _totalWanders; }

    /**
     * Get total emotes performed
     */
    uint32 GetTotalEmotes() const { return _totalEmotes; }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * Initialize default configurations for all contexts
     */
    void InitializeDefaultConfigs();

    /**
     * Start a new wander period
     */
    bool StartWander();

    /**
     * Start a pause between wanders
     */
    void StartPause();

    /**
     * Perform an idle emote
     */
    void PerformIdleEmote();

    /**
     * Get random duration between min and max
     */
    Milliseconds GetRandomDuration(Milliseconds min, Milliseconds max);

    /**
     * Check if bot should continue idle behavior
     *
     * Returns false if bot is in combat, moving to target, etc.
     */
    bool ShouldContinueIdleBehavior() const;

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    Player* _bot;                                   // Owning bot

    // State
    IdleContext _context{IdleContext::NONE};
    IdleBehaviorState _state{IdleBehaviorState::INACTIVE};
    Position _centerPosition;                       // Center of wander area
    std::atomic<bool> _enabled{true};

    // Timing
    uint32 _currentStateDuration{0};                // Time in current state (ms)
    uint32 _targetStateDuration{0};                 // Target duration for current state

    // Statistics
    uint32 _totalWanders{0};
    uint32 _totalEmotes{0};

    // Configurations per context
    IdleBehaviorConfig _configs[9];                 // One per IdleContext value

    // Random number generation
    std::chrono::steady_clock::time_point _lastRandomSeed;
};

} // namespace Playerbot
