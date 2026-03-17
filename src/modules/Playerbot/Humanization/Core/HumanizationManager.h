/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * HUMANIZATION MANAGER
 *
 * Phase 3: Humanization Core
 *
 * Main coordinator for the humanization system.
 * Per-bot manager that orchestrates all humanization features.
 */

#pragma once

#include "Define.h"
#include "SharedDefines.h"
#include "ActivityType.h"
#include "PersonalityProfile.h"
#include "HumanizationConfig.h"
#include "../Sessions/ActivitySessionManager.h"
#include "ObjectGuid.h"
#include <memory>
#include <atomic>

class Player;

namespace Playerbot
{

class BotAI;

namespace Humanization
{

// Forward declarations for sub-managers
class AFKSimulator;
class FishingSessionManager;

/**
 * @brief Humanization state
 */
enum class HumanizationState : uint8
{
    DISABLED = 0,       // Humanization disabled for this bot
    IDLE,               // Waiting for activity
    ACTIVE,             // Actively doing something
    TRANSITIONING,      // Transitioning between activities
    ON_BREAK,           // On break
    AFK                 // Simulating AFK
};

/**
 * @brief Humanization Manager
 *
 * Per-bot manager that coordinates humanization features:
 * - Activity session management
 * - Personality-based behavior
 * - Natural transitions between activities
 * - Break and AFK simulation
 * - Emote usage
 * - Time-of-day behavior variation
 *
 * **Integration with BotAI:**
 * - Created by GameSystemsManager
 * - Updated via BotAI::UpdateAI()
 * - Provides activity recommendations
 *
 * **Phase 3: 30th Manager in GameSystemsManager**
 */
class TC_GAME_API HumanizationManager
{
public:
    /**
     * @brief Construct humanization manager for a bot
     * @param bot The player bot this manager serves
     */
    explicit HumanizationManager(Player* bot);

    ~HumanizationManager();

    // Non-copyable
    HumanizationManager(HumanizationManager const&) = delete;
    HumanizationManager& operator=(HumanizationManager const&) = delete;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the humanization system
     */
    void Initialize();

    /**
     * @brief Update humanization state
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Is the manager initialized?
     */
    bool IsInitialized() const { return _initialized; }

    // ========================================================================
    // STATE
    // ========================================================================

    /**
     * @brief Get current humanization state
     */
    HumanizationState GetState() const { return _state; }

    /**
     * @brief Is humanization enabled for this bot?
     */
    bool IsEnabled() const { return _enabled; }

    /**
     * @brief Enable/disable humanization
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Is the bot on a break?
     */
    bool IsOnBreak() const;

    /**
     * @brief Is the bot AFK?
     */
    bool IsAFK() const { return _state == HumanizationState::AFK; }

    // ========================================================================
    // SESSION MANAGEMENT
    // ========================================================================

    /**
     * @brief Get the session manager
     */
    ActivitySessionManager* GetSessionManager() const { return _sessionManager.get(); }

    /**
     * @brief Get the AFK simulator (Phase 3 Task 12)
     */
    AFKSimulator* GetAFKSimulator() const { return _afkSimulator.get(); }

    /**
     * @brief Get the fishing session manager (Phase 3 Task 11)
     */
    FishingSessionManager* GetFishingSessionManager() const { return _fishingSessionManager.get(); }

    /**
     * @brief Get current activity type
     */
    ActivityType GetCurrentActivity() const;

    /**
     * @brief Get current activity category
     */
    ActivityCategory GetCurrentCategory() const;

    /**
     * @brief Start a specific activity session
     * @param activity Activity to start
     * @param durationMs Duration (0 = use default)
     * @return true if session started
     */
    bool StartActivity(ActivityType activity, uint32 durationMs = 0);

    /**
     * @brief Request transition to another activity
     * @param activity Target activity
     * @param immediate Force immediate transition
     * @return true if transition started/queued
     */
    bool RequestActivityTransition(ActivityType activity, bool immediate = false);

    /**
     * @brief Get recommended next activity
     * @return Recommended activity based on personality and context
     */
    ActivityType GetRecommendedActivity() const;

    // ========================================================================
    // PERSONALITY
    // ========================================================================

    /**
     * @brief Get the bot's personality profile
     */
    PersonalityProfile const& GetPersonality() const;

    /**
     * @brief Set the bot's personality
     */
    void SetPersonality(PersonalityProfile const& profile);

    /**
     * @brief Set personality type
     */
    void SetPersonalityType(PersonalityType type);

    /**
     * @brief Randomize personality with variation
     */
    void RandomizePersonality();

    // ========================================================================
    // BREAK MANAGEMENT
    // ========================================================================

    /**
     * @brief Start a break
     * @param durationMs Duration (0 = personality default)
     * @return true if break started
     */
    bool StartBreak(uint32 durationMs = 0);

    /**
     * @brief End current break
     */
    void EndBreak();

    /**
     * @brief Get remaining break time
     */
    uint32 GetRemainingBreakMs() const;

    /**
     * @brief Should the bot take a break?
     */
    bool ShouldTakeBreak() const;

    // ========================================================================
    // AFK SIMULATION
    // ========================================================================

    /**
     * @brief Start AFK simulation
     * @param durationMs Duration (0 = random based on config)
     * @return true if AFK started
     */
    bool StartAFK(uint32 durationMs = 0);

    /**
     * @brief End AFK state
     */
    void EndAFK();

    /**
     * @brief Get remaining AFK time
     */
    uint32 GetRemainingAFKMs() const;

    /**
     * @brief Should the bot go AFK?
     */
    bool ShouldGoAFK() const;

    // ========================================================================
    // SAFETY CHECKS
    // ========================================================================

    /**
     * @brief Is the bot in a group? (should not go AFK/break if grouped)
     */
    bool IsInGroup() const;

    /**
     * @brief Is the bot grouped with human players?
     */
    bool IsGroupedWithHumans() const;

    /**
     * @brief Is the bot in a safe location for AFK/break?
     * @return true if in inn, city, or other safe spot
     */
    bool IsInSafeLocation() const;

    /**
     * @brief Is the bot in combat or dangerous situation?
     */
    bool IsInDanger() const;

    /**
     * @brief Can the bot safely go AFK or take a break?
     * @return true if not in group, not in danger, and in safe location
     */
    bool CanSafelyGoIdle() const;

    /**
     * @brief Request moving to a safe location before going AFK
     * @return true if movement was initiated
     */
    bool RequestMoveToSafeLocation();

    // ========================================================================
    // EMOTES
    // ========================================================================

    /**
     * @brief Should the bot emote now?
     * @return true if should emote (based on personality)
     */
    bool ShouldEmote() const;

    /**
     * @brief Get a random idle emote
     * @return Emote to use
     */
    Emote GetRandomIdleEmote() const;

    /**
     * @brief Perform an idle emote
     */
    void PerformIdleEmote();

    // ========================================================================
    // TIME-OF-DAY
    // ========================================================================

    /**
     * @brief Get current activity multiplier based on time of day
     */
    float GetTimeOfDayMultiplier() const;

    /**
     * @brief Should bot be less active now? (late night, etc.)
     */
    bool IsLowActivityPeriod() const;

    // ========================================================================
    // METRICS
    // ========================================================================

    struct HumanizationMetrics
    {
        std::atomic<uint32> totalSessions{0};
        std::atomic<uint32> totalBreaks{0};
        std::atomic<uint32> totalAFKPeriods{0};
        std::atomic<uint32> totalEmotes{0};
        std::atomic<uint64> totalActiveTimeMs{0};
        std::atomic<uint64> totalBreakTimeMs{0};
        std::atomic<uint64> totalAFKTimeMs{0};

        void Reset()
        {
            totalSessions = 0;
            totalBreaks = 0;
            totalAFKPeriods = 0;
            totalEmotes = 0;
            totalActiveTimeMs = 0;
            totalBreakTimeMs = 0;
            totalAFKTimeMs = 0;
        }
    };

    HumanizationMetrics const& GetMetrics() const { return _metrics; }
    static HumanizationMetrics const& GetGlobalMetrics() { return _globalMetrics; }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Update state machine
     */
    void UpdateStateMachine(uint32 diff);

    /**
     * @brief Check for AFK trigger
     */
    void CheckAFKTrigger();

    /**
     * @brief Check for break trigger
     */
    void CheckBreakTrigger();

    /**
     * @brief Process idle state
     */
    void ProcessIdleState(uint32 diff);

    /**
     * @brief Process active state
     */
    void ProcessActiveState(uint32 diff);

    /**
     * @brief Transition to new state
     */
    void TransitionTo(HumanizationState newState);

    /**
     * @brief Get random AFK duration based on config
     */
    uint32 GetRandomAFKDuration() const;

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Bot reference
    Player* _bot;
    ObjectGuid _botGuid;

    // Session manager
    std::unique_ptr<ActivitySessionManager> _sessionManager;

    // Humanization sub-managers (Phase 3 Tasks 11-12)
    std::unique_ptr<AFKSimulator> _afkSimulator;
    std::unique_ptr<FishingSessionManager> _fishingSessionManager;

    // State
    HumanizationState _state{HumanizationState::DISABLED};
    bool _enabled{true};
    bool _initialized{false};

    // AFK tracking
    bool _isAFK{false};
    std::chrono::steady_clock::time_point _afkStartTime;
    uint32 _afkDurationMs{0};

    // Safe location movement tracking
    bool _movingToSafeLocation{false};
    bool _pendingAFK{false};         // True if we want to go AFK after reaching safe location
    bool _pendingBreak{false};       // True if we want to take a break after reaching safe location

    // Emote tracking
    uint32 _lastEmoteTime{0};
    uint32 _emoteCooldown{30000}; // 30 seconds between emotes

    // Update timing
    uint32 _updateTimer{0};
    uint32 _afkCheckTimer{0};
    static constexpr uint32 UPDATE_INTERVAL = 1000;      // 1 second
    static constexpr uint32 AFK_CHECK_INTERVAL = 60000;  // 1 minute

    // Per-bot metrics
    HumanizationMetrics _metrics;

    // Global metrics
    static HumanizationMetrics _globalMetrics;
};

} // namespace Humanization
} // namespace Playerbot
