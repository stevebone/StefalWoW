/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * AFK SIMULATOR
 *
 * Phase 3: Humanization Core - Task 12
 *
 * Simulates human-like AFK (Away From Keyboard) behavior.
 * Real players take breaks - bots should too for realism.
 *
 * Features:
 * - Personality-based AFK frequency and duration
 * - Natural break patterns (bio breaks, snack breaks, phone checks)
 * - Time-of-day awareness (more AFK late at night)
 * - Session-based break scheduling
 * - Idle behaviors during AFK (emotes, movement adjustments)
 * - Auto-resume with natural delay
 */

#pragma once

#include "Define.h"
#include "../Core/ActivityType.h"
#include "../Core/PersonalityProfile.h"
#include "../../AI/BehaviorManager.h"
#include <chrono>
#include <random>
#include <functional>
#include <vector>
#include <atomic>

class Player;

namespace Playerbot
{

class BotAI;

namespace Humanization
{

/**
 * @brief Types of AFK behavior
 */
enum class AFKType : uint8
{
    NONE = 0,
    SHORT,              // 30 sec - 2 min (quick phone check)
    MEDIUM,             // 2-5 min (bathroom, getting drink)
    LONG,               // 5-15 min (extended break)
    BIO_BREAK,          // 3-10 min (bathroom break)
    SNACK_BREAK,        // 2-8 min (getting food/drink)
    PHONE_CHECK,        // 30 sec - 2 min (checking phone)
    DISTRACTION,        // 1-3 min (looked away briefly)
    SESSION_END,        // 15-30 min (taking a break before logging)

    MAX_TYPE
};

/**
 * @brief AFK behavior flags
 */
enum class AFKBehaviorFlags : uint8
{
    NONE            = 0x00,
    SIT_DOWN        = 0x01,     // Sit down during AFK
    EMOTE_RANDOMLY  = 0x02,     // Random emotes during AFK
    SLIGHT_MOVEMENT = 0x04,     // Minor position adjustments
    MOUNT_DISMOUNT  = 0x08,     // May dismount if mounted
    USE_CHAT_AFK    = 0x10,     // Set /afk status
    FIND_SAFE_SPOT  = 0x20,     // Move to safer location first

    DEFAULT = SIT_DOWN | SLIGHT_MOVEMENT
};

// Enable bitwise operations for AFKBehaviorFlags
inline AFKBehaviorFlags operator|(AFKBehaviorFlags a, AFKBehaviorFlags b)
{
    return static_cast<AFKBehaviorFlags>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

inline AFKBehaviorFlags operator&(AFKBehaviorFlags a, AFKBehaviorFlags b)
{
    return static_cast<AFKBehaviorFlags>(static_cast<uint8>(a) & static_cast<uint8>(b));
}

inline bool HasFlag(AFKBehaviorFlags flags, AFKBehaviorFlags flag)
{
    return (static_cast<uint8>(flags) & static_cast<uint8>(flag)) != 0;
}

/**
 * @brief AFK session data
 */
struct AFKSession
{
    AFKType type{AFKType::NONE};
    uint32 startTimeMs{0};              // Game time when AFK started
    uint32 plannedDurationMs{0};        // How long we plan to be AFK
    uint32 actualDurationMs{0};         // Actual elapsed time
    AFKBehaviorFlags behaviors{AFKBehaviorFlags::DEFAULT};
    bool isActive{false};
    bool wasInterrupted{false};
    std::string reason;                  // For logging/debugging

    void Reset()
    {
        type = AFKType::NONE;
        startTimeMs = 0;
        plannedDurationMs = 0;
        actualDurationMs = 0;
        behaviors = AFKBehaviorFlags::DEFAULT;
        isActive = false;
        wasInterrupted = false;
        reason.clear();
    }
};

/**
 * @brief AFK timing configuration
 */
struct AFKTimingConfig
{
    // Duration ranges per type (in milliseconds)
    uint32 shortMinMs{30000};           // 30 sec
    uint32 shortMaxMs{120000};          // 2 min
    uint32 mediumMinMs{120000};         // 2 min
    uint32 mediumMaxMs{300000};         // 5 min
    uint32 longMinMs{300000};           // 5 min
    uint32 longMaxMs{900000};           // 15 min
    uint32 bioBreakMinMs{180000};       // 3 min
    uint32 bioBreakMaxMs{600000};       // 10 min
    uint32 snackBreakMinMs{120000};     // 2 min
    uint32 snackBreakMaxMs{480000};     // 8 min
    uint32 phoneCheckMinMs{30000};      // 30 sec
    uint32 phoneCheckMaxMs{120000};     // 2 min
    uint32 distractionMinMs{60000};     // 1 min
    uint32 distractionMaxMs{180000};    // 3 min
    uint32 sessionEndMinMs{900000};     // 15 min
    uint32 sessionEndMaxMs{1800000};    // 30 min

    // Behavior timing
    uint32 emoteIntervalMinMs{30000};   // Min time between emotes
    uint32 emoteIntervalMaxMs{120000};  // Max time between emotes
    uint32 movementIntervalMinMs{60000};// Min time between slight movements
    uint32 movementIntervalMaxMs{180000};// Max time between slight movements
};

/**
 * @brief Statistics for AFK sessions
 */
struct AFKStatistics
{
    std::atomic<uint32> totalAFKCount{0};
    std::atomic<uint32> shortAFKCount{0};
    std::atomic<uint32> mediumAFKCount{0};
    std::atomic<uint32> longAFKCount{0};
    std::atomic<uint32> bioBreakCount{0};
    std::atomic<uint64> totalAFKTimeMs{0};
    std::atomic<uint32> interruptedCount{0};

    void Reset()
    {
        totalAFKCount = 0;
        shortAFKCount = 0;
        mediumAFKCount = 0;
        longAFKCount = 0;
        bioBreakCount = 0;
        totalAFKTimeMs = 0;
        interruptedCount = 0;
    }
};

/**
 * @brief Simulates human-like AFK behavior
 */
class TC_GAME_API AFKSimulator : public BehaviorManager
{
public:
    /**
     * @brief Construct AFK simulator for a bot
     * @param bot The player bot
     * @param ai The bot AI
     */
    AFKSimulator(Player* bot, BotAI* ai);
    ~AFKSimulator();

    // Non-copyable
    AFKSimulator(AFKSimulator const&) = delete;
    AFKSimulator& operator=(AFKSimulator const&) = delete;

    // ========================================================================
    // LIFECYCLE (BehaviorManager overrides)
    // ========================================================================

    bool OnInitialize();
    void OnShutdown();
    void OnUpdate(uint32 elapsed);

    // ========================================================================
    // AFK STATE
    // ========================================================================

    /**
     * @brief Check if currently AFK
     */
    bool IsAFK() const { return _currentSession.isActive; }

    /**
     * @brief Get current AFK type
     */
    AFKType GetAFKType() const { return _currentSession.type; }

    /**
     * @brief Get current AFK session data
     */
    AFKSession const& GetCurrentSession() const { return _currentSession; }

    /**
     * @brief Get remaining AFK time
     * @return Remaining time in milliseconds, 0 if not AFK
     */
    uint32 GetRemainingAFKTime() const;

    /**
     * @brief Get AFK progress (0.0 - 1.0)
     */
    float GetAFKProgress() const;

    // ========================================================================
    // AFK CONTROL
    // ========================================================================

    /**
     * @brief Start AFK with specific type
     * @param type Type of AFK
     * @param reason Optional reason for logging
     * @return true if AFK started successfully
     */
    bool StartAFK(AFKType type, std::string const& reason = "");

    /**
     * @brief Start AFK with custom duration
     * @param durationMs Duration in milliseconds
     * @param type Type of AFK (affects behavior)
     * @param reason Optional reason
     * @return true if started
     */
    bool StartAFKWithDuration(uint32 durationMs, AFKType type = AFKType::SHORT,
        std::string const& reason = "");

    /**
     * @brief End AFK immediately
     * @param wasInterrupted True if interrupted externally
     */
    void EndAFK(bool wasInterrupted = false);

    /**
     * @brief Force end AFK (for critical situations like combat)
     */
    void ForceEndAFK();

    // ========================================================================
    // AFK DECISIONS
    // ========================================================================

    /**
     * @brief Check if should go AFK now
     * Based on: session duration, personality, randomness
     */
    bool ShouldGoAFK() const;

    /**
     * @brief Get recommended AFK type
     * Based on: time since last break, session length, personality
     */
    AFKType GetRecommendedAFKType() const;

    /**
     * @brief Check if can go AFK now
     * Considers: combat, group activity, important tasks
     */
    bool CanGoAFK() const;

    /**
     * @brief Attempt to trigger AFK if appropriate
     * @return true if AFK was triggered
     */
    bool TryTriggerAFK();

    // ========================================================================
    // PERSONALITY
    // ========================================================================

    /**
     * @brief Set personality profile for AFK behavior
     * @param personality The personality profile
     */
    void SetPersonality(PersonalityProfile const* personality);

    /**
     * @brief Get personality modifier for AFK frequency
     * @return Modifier (>1 = more AFK, <1 = less AFK)
     */
    float GetPersonalityAFKModifier() const;

    // ========================================================================
    // TIMING CONFIGURATION
    // ========================================================================

    /**
     * @brief Get timing configuration
     */
    AFKTimingConfig const& GetTimingConfig() const { return _timingConfig; }

    /**
     * @brief Set timing configuration
     */
    void SetTimingConfig(AFKTimingConfig const& config) { _timingConfig = config; }

    /**
     * @brief Get duration range for AFK type
     * @param type The AFK type
     * @param outMinMs Output: minimum duration
     * @param outMaxMs Output: maximum duration
     */
    void GetDurationRange(AFKType type, uint32& outMinMs, uint32& outMaxMs) const;

    // ========================================================================
    // CALLBACKS
    // ========================================================================

    using AFKCallback = std::function<void(AFKType type, bool started)>;

    /**
     * @brief Register callback for AFK state changes
     */
    void OnAFKStateChange(AFKCallback callback);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Get AFK statistics
     */
    AFKStatistics const& GetStatistics() const { return _statistics; }

    /**
     * @brief Reset statistics
     */
    void ResetStatistics() { _statistics.Reset(); }

    // ========================================================================
    // SESSION TRACKING
    // ========================================================================

    /**
     * @brief Get time since last AFK
     * @return Time in milliseconds
     */
    uint32 GetTimeSinceLastAFK() const;

    /**
     * @brief Get total session time (since Initialize)
     */
    uint32 GetTotalSessionTime() const;

    /**
     * @brief Get time in current activity before AFK
     */
    uint32 GetCurrentActivityDuration() const;

private:
    // ========================================================================
    // INTERNAL UPDATE METHODS
    // ========================================================================

    /**
     * @brief Update AFK state
     */
    void UpdateAFKState(uint32 elapsed);

    /**
     * @brief Check for auto-AFK trigger
     */
    void CheckAutoAFK();

    /**
     * @brief Handle AFK behaviors (emotes, movement)
     */
    void HandleAFKBehaviors(uint32 elapsed);

    /**
     * @brief Check if AFK should end
     */
    bool ShouldEndAFK() const;

    // ========================================================================
    // BEHAVIOR METHODS
    // ========================================================================

    /**
     * @brief Perform sit down behavior
     */
    void DoSitDown();

    /**
     * @brief Perform stand up behavior
     */
    void DoStandUp();

    /**
     * @brief Perform random AFK emote
     */
    void DoRandomEmote();

    /**
     * @brief Perform slight movement adjustment
     */
    void DoSlightMovement();

    /**
     * @brief Dismount if mounted
     */
    void DoDismount();

    /**
     * @brief Set chat AFK status
     */
    void SetChatAFKStatus(bool afk);

    // ========================================================================
    // DURATION CALCULATION
    // ========================================================================

    /**
     * @brief Calculate random AFK duration for type
     */
    uint32 CalculateAFKDuration(AFKType type) const;

    /**
     * @brief Apply personality modifiers to duration
     */
    uint32 ApplyPersonalityModifiers(uint32 baseDurationMs) const;

    /**
     * @brief Apply time-of-day modifiers
     */
    float GetTimeOfDayModifier() const;

    // ========================================================================
    // PROBABILITY METHODS
    // ========================================================================

    /**
     * @brief Get base AFK probability
     */
    float GetBaseAFKProbability() const;

    /**
     * @brief Get session-based AFK probability modifier
     */
    float GetSessionModifier() const;

    /**
     * @brief Select weighted AFK type
     */
    AFKType SelectWeightedAFKType() const;

    // ========================================================================
    // NOTIFICATION
    // ========================================================================

    /**
     * @brief Notify callbacks of state change
     */
    void NotifyStateChange(bool started);

    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /**
     * @brief Get current game time in milliseconds
     */
    uint32 GetCurrentTimeMs() const;

    /**
     * @brief Get random number in range
     */
    uint32 RandomInRange(uint32 min, uint32 max) const;

    /**
     * @brief Get random float 0-1
     */
    float RandomFloat() const;

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Session state
    AFKSession _currentSession;
    uint32 _lastAFKEndTime{0};          // When last AFK ended
    uint32 _sessionStartTime{0};        // When session started
    uint32 _lastActivityStartTime{0};   // When current activity started

    // Behavior timing
    uint32 _lastEmoteTime{0};
    uint32 _lastMovementTime{0};
    uint32 _nextEmoteTime{0};
    uint32 _nextMovementTime{0};

    // Configuration
    AFKTimingConfig _timingConfig;
    PersonalityProfile const* _personality{nullptr};
    bool _autoAFKEnabled{true};

    // Callbacks
    std::vector<AFKCallback> _callbacks;

    // Statistics
    AFKStatistics _statistics;

    // Random number generation
    mutable std::mt19937 _rng;

    // Configurable probabilities
    float _baseAFKCheckInterval{60000.0f};  // Check every 60 seconds
    float _baseAFKProbability{0.02f};       // 2% base chance per check

    // AFK type weights (relative probabilities)
    static constexpr float WEIGHT_SHORT = 0.30f;
    static constexpr float WEIGHT_MEDIUM = 0.25f;
    static constexpr float WEIGHT_LONG = 0.10f;
    static constexpr float WEIGHT_BIO = 0.15f;
    static constexpr float WEIGHT_SNACK = 0.10f;
    static constexpr float WEIGHT_PHONE = 0.08f;
    static constexpr float WEIGHT_DISTRACTION = 0.02f;

    // Emotes to use during AFK
    static const std::vector<uint32> AFK_EMOTES;
};

/**
 * @brief Get human-readable name for AFK type
 */
inline std::string GetAFKTypeName(AFKType type)
{
    switch (type)
    {
        case AFKType::NONE:         return "None";
        case AFKType::SHORT:        return "Short Break";
        case AFKType::MEDIUM:       return "Medium Break";
        case AFKType::LONG:         return "Long Break";
        case AFKType::BIO_BREAK:    return "Bio Break";
        case AFKType::SNACK_BREAK:  return "Snack Break";
        case AFKType::PHONE_CHECK:  return "Phone Check";
        case AFKType::DISTRACTION:  return "Distraction";
        case AFKType::SESSION_END:  return "Session End";
        default:                    return "Unknown";
    }
}

} // namespace Humanization
} // namespace Playerbot
