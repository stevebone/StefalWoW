/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * FISHING SESSION MANAGER
 *
 * Phase 3: Humanization Core - Task 11
 *
 * Manages dedicated fishing sessions where bots fish as a "hobby"
 * for extended periods (30-60+ minutes) at a single spot.
 *
 * Features:
 * - Session-based fishing (30-60 min dedicated sessions)
 * - Personality-driven behavior (cast timing, patience)
 * - Human-like idle behaviors (sitting, emotes, watching water)
 * - Weather and time awareness
 * - Loot tracking and skill progression
 * - Natural breaks during long sessions
 */

#pragma once

#include "Define.h"
#include "../Core/ActivityType.h"
#include "../Core/PersonalityProfile.h"
#include "../../AI/BehaviorManager.h"
#include "Position.h"
#include <chrono>
#include <random>
#include <functional>
#include <vector>
#include <atomic>

class Player;
class GameObject;

namespace Playerbot
{

class BotAI;

namespace Humanization
{

/**
 * @brief Fishing spot data
 */
struct FishingSpot
{
    Position position;
    uint32 zoneId{0};
    uint32 areaId{0};
    uint16 minSkill{0};                 // Minimum fishing skill for this spot
    uint16 maxSkill{0};                 // Skill level where spot gives no more skillups
    std::string name;
    std::vector<uint32> possibleCatches; // Item IDs that can be caught here
    bool isLavaFishing{false};
    bool requiresSpecialLure{false};
    float waterLevel{0.0f};             // Water surface height

    bool IsValid() const { return position.IsPositionValid() && zoneId > 0; }
};

/**
 * @brief Fishing session state
 */
enum class FishingSessionState : uint8
{
    IDLE = 0,           // Not fishing
    TRAVELING,          // Moving to fishing spot
    SETTING_UP,         // Getting into position, possibly sitting
    CASTING,            // Casting the fishing line
    WAITING,            // Waiting for bite
    LOOTING,            // Caught something, looting
    REELING_MISS,       // Missed the catch (clicked too late/early)
    BREAK,              // Taking a short break
    WATCHING_WATER,     // Just watching water, not casting (humanization)
    EQUIPMENT_CHECK,    // Checking/applying lures
    INVENTORY_FULL,     // Can't fish, bags full
    ENDING              // Session ending, cleanup
};

/**
 * @brief Fishing session data
 */
struct FishingSession
{
    bool isActive{false};
    FishingSpot spot;
    FishingSessionState state{FishingSessionState::IDLE};

    // Timing
    uint32 startTimeMs{0};              // Game time when session started
    uint32 plannedDurationMs{0};        // Planned session duration
    uint32 elapsedTimeMs{0};            // Actual elapsed time

    // Progress
    uint32 castCount{0};                // Number of casts
    uint32 catchCount{0};               // Successful catches
    uint32 missCount{0};                // Missed catches
    uint32 skillGains{0};               // Skill points gained
    std::vector<uint32> caughtItems;    // Items caught (entry IDs)

    // State timing
    uint32 stateStartTimeMs{0};         // When current state started
    uint32 stateDurationMs{0};          // How long to stay in current state
    uint32 lastCastTimeMs{0};           // When last cast was made
    uint32 nextCastDelayMs{0};          // Delay before next cast

    // Breaks
    uint32 breaksTaken{0};
    uint32 lastBreakTimeMs{0};
    bool isOnBreak{false};

    // Humanization state
    bool isSitting{false};
    bool isWatchingWater{false};
    uint32 lastEmoteTimeMs{0};

    void Reset()
    {
        isActive = false;
        spot = FishingSpot{};
        state = FishingSessionState::IDLE;
        startTimeMs = 0;
        plannedDurationMs = 0;
        elapsedTimeMs = 0;
        castCount = 0;
        catchCount = 0;
        missCount = 0;
        skillGains = 0;
        caughtItems.clear();
        stateStartTimeMs = 0;
        stateDurationMs = 0;
        lastCastTimeMs = 0;
        nextCastDelayMs = 0;
        breaksTaken = 0;
        lastBreakTimeMs = 0;
        isOnBreak = false;
        isSitting = false;
        isWatchingWater = false;
        lastEmoteTimeMs = 0;
    }
};

/**
 * @brief Fishing session configuration
 */
struct FishingSessionConfig
{
    // Session duration (milliseconds)
    uint32 minDurationMs{1800000};      // 30 minutes
    uint32 maxDurationMs{3600000};      // 60 minutes

    // Cast timing
    uint32 minCastDelayMs{500};         // Min delay between casts
    uint32 maxCastDelayMs{3000};        // Max delay between casts
    uint32 bobberWaitMinMs{15000};      // Min time to wait for bite
    uint32 bobberWaitMaxMs{30000};      // Max time to wait for bite

    // Humanization
    float sittingChance{0.3f};          // Chance to sit while fishing
    float watchWaterChance{0.1f};       // Chance to just watch water (no cast)
    float emoteChance{0.05f};           // Chance to emote per cast
    float missChance{0.05f};            // Chance to "miss" the catch

    // Breaks
    uint32 breakIntervalMinMs{600000};  // Min time between breaks (10 min)
    uint32 breakIntervalMaxMs{1200000}; // Max time between breaks (20 min)
    uint32 breakDurationMinMs{30000};   // Min break duration (30 sec)
    uint32 breakDurationMaxMs{180000};  // Max break duration (3 min)

    // Inventory
    uint32 minFreeSlots{5};             // Min free slots to continue fishing
};

/**
 * @brief Fishing session statistics
 */
struct FishingStatistics
{
    std::atomic<uint32> totalSessions{0};
    std::atomic<uint32> totalCasts{0};
    std::atomic<uint32> totalCatches{0};
    std::atomic<uint32> totalMisses{0};
    std::atomic<uint32> totalSkillGains{0};
    std::atomic<uint64> totalTimeSpentMs{0};

    float GetCatchRate() const
    {
        uint32 casts = totalCasts.load();
        uint32 catches = totalCatches.load();
        return casts > 0 ? static_cast<float>(catches) / casts : 0.0f;
    }

    void Reset()
    {
        totalSessions = 0;
        totalCasts = 0;
        totalCatches = 0;
        totalMisses = 0;
        totalSkillGains = 0;
        totalTimeSpentMs = 0;
    }
};

/**
 * @brief Manages dedicated fishing sessions
 *
 * Unlike opportunistic fishing during travel, this creates focused
 * fishing sessions where the bot:
 * - Commits to fishing for 30-60+ minutes
 * - Stays at one spot
 * - Exhibits human-like behaviors (sitting, emotes, breaks)
 * - Continues until session ends or bags full
 */
class TC_GAME_API FishingSessionManager : public BehaviorManager
{
public:
    /**
     * @brief Construct fishing session manager for a bot
     * @param bot The player bot
     * @param ai The bot AI
     */
    FishingSessionManager(Player* bot, BotAI* ai);
    ~FishingSessionManager();

    // Non-copyable
    FishingSessionManager(FishingSessionManager const&) = delete;
    FishingSessionManager& operator=(FishingSessionManager const&) = delete;

    // ========================================================================
    // LIFECYCLE (BehaviorManager overrides)
    // ========================================================================

    bool OnInitialize();
    void OnShutdown();
    void OnUpdate(uint32 elapsed);

    // ========================================================================
    // SESSION MANAGEMENT
    // ========================================================================

    /**
     * @brief Start a fishing session
     * @param duration Target duration (0 = config default 30-60 min)
     * @param spot Specific spot (empty = auto-select)
     * @return true if session started
     */
    bool StartSession(uint32 durationMs = 0, FishingSpot const* spot = nullptr);

    /**
     * @brief End current fishing session
     * @param reason Reason for ending (for logging)
     */
    void EndSession(std::string const& reason = "");

    /**
     * @brief Check if in fishing session
     */
    bool IsInSession() const { return _session.isActive; }

    /**
     * @brief Get current session data
     */
    FishingSession const& GetSession() const { return _session; }

    /**
     * @brief Get remaining session time
     * @return Remaining time in milliseconds
     */
    uint32 GetRemainingTime() const;

    /**
     * @brief Get session progress (0.0 - 1.0)
     */
    float GetProgress() const;

    // ========================================================================
    // FISHING STATE
    // ========================================================================

    /**
     * @brief Get current fishing state
     */
    FishingSessionState GetState() const { return _session.state; }

    /**
     * @brief Check if currently casting/fishing
     */
    bool IsFishing() const;

    /**
     * @brief Check if on break
     */
    bool IsOnBreak() const { return _session.isOnBreak; }

    // ========================================================================
    // SPOT MANAGEMENT
    // ========================================================================

    /**
     * @brief Find best fishing spot near bot
     * @param maxDistance Maximum distance to search
     * @return Best spot or empty spot if none found
     */
    FishingSpot FindNearbySpot(float maxDistance = 100.0f) const;

    /**
     * @brief Get available fishing spots in zone
     * @param zoneId Zone to search (0 = current zone)
     * @return List of available spots
     */
    std::vector<FishingSpot> GetSpotsInZone(uint32 zoneId = 0) const;

    /**
     * @brief Check if spot is valid for bot's skill level
     * @param spot Spot to check
     * @return true if bot can fish there effectively
     */
    bool IsSpotAppropriate(FishingSpot const& spot) const;

    // ========================================================================
    // PERSONALITY
    // ========================================================================

    /**
     * @brief Set personality profile for fishing behavior
     * @param personality The personality profile
     */
    void SetPersonality(PersonalityProfile const* personality);

    /**
     * @brief Get personality modifier for session duration
     */
    float GetDurationModifier() const;

    /**
     * @brief Get personality modifier for patience (cast delay)
     */
    float GetPatienceModifier() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get configuration
     */
    FishingSessionConfig const& GetConfig() const { return _config; }

    /**
     * @brief Set configuration
     */
    void SetConfig(FishingSessionConfig const& config) { _config = config; }

    // ========================================================================
    // CALLBACKS
    // ========================================================================

    using FishingCallback = std::function<void(FishingSessionState state, bool started)>;
    using CatchCallback = std::function<void(uint32 itemId, bool success)>;

    /**
     * @brief Register callback for session state changes
     */
    void OnStateChange(FishingCallback callback);

    /**
     * @brief Register callback for catch events
     */
    void OnCatch(CatchCallback callback);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /**
     * @brief Get fishing statistics
     */
    FishingStatistics const& GetStatistics() const { return _statistics; }

    /**
     * @brief Reset statistics
     */
    void ResetStatistics() { _statistics.Reset(); }

    // ========================================================================
    // FISHING SKILL
    // ========================================================================

    /**
     * @brief Get bot's current fishing skill
     */
    uint16 GetFishingSkill() const;

    /**
     * @brief Check if bot has fishing skill
     */
    bool HasFishingSkill() const;

    /**
     * @brief Check if bot has fishing pole equipped
     */
    bool HasFishingPole() const;

    /**
     * @brief Check if bot can fish (skill + equipment)
     */
    bool CanFish() const;

private:
    // ========================================================================
    // STATE MACHINE
    // ========================================================================

    /**
     * @brief Transition to a new state
     */
    void TransitionTo(FishingSessionState newState);

    /**
     * @brief Update current state
     */
    void UpdateState(uint32 elapsed);

    // State handlers
    void HandleIdleState(uint32 elapsed);
    void HandleTravelingState(uint32 elapsed);
    void HandleSettingUpState(uint32 elapsed);
    void HandleCastingState(uint32 elapsed);
    void HandleWaitingState(uint32 elapsed);
    void HandleLootingState(uint32 elapsed);
    void HandleReelingMissState(uint32 elapsed);
    void HandleBreakState(uint32 elapsed);
    void HandleWatchingWaterState(uint32 elapsed);
    void HandleEquipmentCheckState(uint32 elapsed);
    void HandleInventoryFullState(uint32 elapsed);
    void HandleEndingState(uint32 elapsed);

    // ========================================================================
    // FISHING ACTIONS
    // ========================================================================

    /**
     * @brief Cast fishing line
     */
    bool CastFishingLine();

    /**
     * @brief Reel in the catch
     */
    bool ReelIn();

    /**
     * @brief Handle successful catch
     */
    void HandleCatch(uint32 itemId);

    /**
     * @brief Handle missed catch
     */
    void HandleMiss();

    /**
     * @brief Check fishing bobber
     */
    GameObject* FindFishingBobber() const;

    // ========================================================================
    // HUMANIZATION
    // ========================================================================

    /**
     * @brief Perform sitting behavior
     */
    void DoSit();

    /**
     * @brief Perform standing behavior
     */
    void DoStand();

    /**
     * @brief Perform random fishing emote
     */
    void DoRandomEmote();

    /**
     * @brief Check if should take break
     */
    bool ShouldTakeBreak() const;

    /**
     * @brief Start a break
     */
    void StartBreak();

    /**
     * @brief Check if should watch water (skip cast)
     */
    bool ShouldWatchWater() const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    /**
     * @brief Calculate random session duration
     */
    uint32 CalculateSessionDuration() const;

    /**
     * @brief Calculate cast delay
     */
    uint32 CalculateCastDelay() const;

    /**
     * @brief Calculate break duration
     */
    uint32 CalculateBreakDuration() const;

    /**
     * @brief Check if inventory has space
     */
    bool HasInventorySpace() const;

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

    /**
     * @brief Notify state change callbacks
     */
    void NotifyStateChange(bool started);

    /**
     * @brief Notify catch callbacks
     */
    void NotifyCatch(uint32 itemId, bool success);

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Session data
    FishingSession _session;

    // Configuration
    FishingSessionConfig _config;
    PersonalityProfile const* _personality{nullptr};

    // Callbacks
    std::vector<FishingCallback> _stateCallbacks;
    std::vector<CatchCallback> _catchCallbacks;

    // Statistics
    FishingStatistics _statistics;

    // Random number generation
    mutable std::mt19937 _rng;

    // Fishing emotes
    static const std::vector<uint32> FISHING_EMOTES;

    // Fishing spell ID
    static constexpr uint32 FISHING_SPELL = 131474;     // Fishing spell

    // Fishing skill ID
    static constexpr uint32 FISHING_SKILL = 356;        // Fishing skill ID
};

/**
 * @brief Get human-readable name for fishing state
 */
inline std::string GetFishingStateName(FishingSessionState state)
{
    switch (state)
    {
        case FishingSessionState::IDLE:             return "Idle";
        case FishingSessionState::TRAVELING:        return "Traveling";
        case FishingSessionState::SETTING_UP:       return "Setting Up";
        case FishingSessionState::CASTING:          return "Casting";
        case FishingSessionState::WAITING:          return "Waiting";
        case FishingSessionState::LOOTING:          return "Looting";
        case FishingSessionState::REELING_MISS:     return "Missed Catch";
        case FishingSessionState::BREAK:            return "Break";
        case FishingSessionState::WATCHING_WATER:   return "Watching Water";
        case FishingSessionState::EQUIPMENT_CHECK:  return "Equipment Check";
        case FishingSessionState::INVENTORY_FULL:   return "Inventory Full";
        case FishingSessionState::ENDING:           return "Ending";
        default:                                    return "Unknown";
    }
}

} // namespace Humanization
} // namespace Playerbot
