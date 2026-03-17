/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * HUMANIZATION CONFIG
 *
 * Phase 3: Humanization Core
 *
 * Configuration settings for the humanization system.
 * Loaded from playerbot.conf at startup.
 */

#pragma once

#include "Define.h"
#include "ActivityType.h"
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <array>

namespace Playerbot
{
namespace Humanization
{

/**
 * @brief Session duration configuration
 */
struct SessionDurationConfig
{
    uint32 minDurationMs = 1800000;    // 30 minutes minimum
    uint32 maxDurationMs = 5400000;    // 90 minutes maximum
    uint32 extendChancePercent = 20;   // Chance to extend session
    uint32 maxExtensions = 2;          // Maximum number of extensions
};

/**
 * @brief Break configuration
 */
struct BreakConfig
{
    uint32 shortBreakMinMs = 60000;    // 1 minute minimum
    uint32 shortBreakMaxMs = 300000;   // 5 minutes maximum
    uint32 longBreakMinMs = 300000;    // 5 minutes minimum
    uint32 longBreakMaxMs = 900000;    // 15 minutes maximum
    uint32 bioBreakMinMs = 120000;     // 2 minutes minimum
    uint32 bioBreakMaxMs = 600000;     // 10 minutes maximum
    uint32 longBreakChancePercent = 10;// Chance of long break vs short
};

/**
 * @brief AFK simulation configuration
 */
struct AFKConfig
{
    bool enabled = true;
    uint32 chancePerHourPercent = 5;   // Chance of going AFK per hour
    uint32 shortAFKMinMs = 60000;      // 1 minute
    uint32 shortAFKMaxMs = 300000;     // 5 minutes
    uint32 mediumAFKMinMs = 300000;    // 5 minutes
    uint32 mediumAFKMaxMs = 900000;    // 15 minutes
    uint32 longAFKMinMs = 900000;      // 15 minutes
    uint32 longAFKMaxMs = 1800000;     // 30 minutes
};

/**
 * @brief Activity-specific configuration
 */
struct ActivityConfig
{
    bool enabled = true;
    uint32 minDurationMs = 1800000;
    uint32 maxDurationMs = 3600000;
    float probabilityWeight = 1.0f;
};

/**
 * @brief Humanization system configuration
 *
 * Singleton that holds all configuration loaded from playerbot.conf.
 * Thread-safe for read operations after initialization.
 */
class TC_GAME_API HumanizationConfig
{
public:
    /**
     * @brief Get singleton instance
     */
    static HumanizationConfig& Instance();

    /**
     * @brief Load configuration from playerbot.conf
     */
    void Load();

    /**
     * @brief Reload configuration (can be called at runtime)
     */
    void Reload();

    // ========================================================================
    // GENERAL SETTINGS
    // ========================================================================

    /**
     * @brief Is humanization system enabled?
     */
    bool IsEnabled() const { return _enabled; }

    /**
     * @brief Get debug logging level
     * @return 0 = off, 1 = basic, 2 = verbose
     */
    uint32 GetDebugLevel() const { return _debugLevel; }

    /**
     * @brief Should apply to existing bots on load?
     */
    bool ApplyToExistingBots() const { return _applyToExisting; }

    // ========================================================================
    // SESSION CONFIGURATION
    // ========================================================================

    /**
     * @brief Get session duration config
     */
    SessionDurationConfig const& GetSessionConfig() const { return _sessionConfig; }

    /**
     * @brief Get break configuration
     */
    BreakConfig const& GetBreakConfig() const { return _breakConfig; }

    /**
     * @brief Get AFK configuration
     */
    AFKConfig const& GetAFKConfig() const { return _afkConfig; }

    // ========================================================================
    // ACTIVITY CONFIGURATION
    // ========================================================================

    /**
     * @brief Get activity-specific configuration
     * @param category Activity category
     * @return Configuration for that category
     */
    ActivityConfig GetActivityConfig(ActivityCategory category) const;

    /**
     * @brief Is an activity category enabled?
     */
    bool IsActivityEnabled(ActivityCategory category) const;

    /**
     * @brief Get minimum duration for an activity
     */
    uint32 GetActivityMinDuration(ActivityCategory category) const;

    /**
     * @brief Get maximum duration for an activity
     */
    uint32 GetActivityMaxDuration(ActivityCategory category) const;

    // ========================================================================
    // GATHERING SESSIONS
    // ========================================================================

    /**
     * @brief Get gathering session minimum duration
     */
    uint32 GetGatheringMinDuration() const { return _gatheringMinDurationMs; }

    /**
     * @brief Get gathering session maximum duration
     */
    uint32 GetGatheringMaxDuration() const { return _gatheringMaxDurationMs; }

    /**
     * @brief Should bots continue gathering until bags full?
     */
    bool GatherUntilBagsFull() const { return _gatherUntilBagsFull; }

    // ========================================================================
    // CITY LIFE
    // ========================================================================

    /**
     * @brief Get city life session minimum duration
     */
    uint32 GetCityLifeMinDuration() const { return _cityLifeMinDurationMs; }

    /**
     * @brief Get city life session maximum duration
     */
    uint32 GetCityLifeMaxDuration() const { return _cityLifeMaxDurationMs; }

    /**
     * @brief Should bots visit auction house?
     */
    bool EnableAuctionBrowsing() const { return _enableAuctionBrowsing; }

    /**
     * @brief Should bots rest at inns?
     */
    bool EnableInnResting() const { return _enableInnResting; }

    // ========================================================================
    // FISHING
    // ========================================================================

    /**
     * @brief Get fishing session minimum duration
     */
    uint32 GetFishingMinDuration() const { return _fishingMinDurationMs; }

    /**
     * @brief Get fishing session maximum duration
     */
    uint32 GetFishingMaxDuration() const { return _fishingMaxDurationMs; }

    // ========================================================================
    // PERSONALITY
    // ========================================================================

    /**
     * @brief Should assign random personalities to new bots?
     */
    bool AssignRandomPersonalities() const { return _assignRandomPersonalities; }

    /**
     * @brief Get personality variation amount
     * @return Variance (0.0-1.0) to apply to personality profiles
     */
    float GetPersonalityVariance() const { return _personalityVariance; }

    // ========================================================================
    // EMOTES
    // ========================================================================

    /**
     * @brief Are idle emotes enabled?
     */
    bool EnableIdleEmotes() const { return _enableIdleEmotes; }

    /**
     * @brief Get base emote frequency
     * @return Chance per update cycle (0.0-1.0)
     */
    float GetEmoteFrequency() const { return _emoteFrequency; }

    // ========================================================================
    // TIME-OF-DAY
    // ========================================================================

    /**
     * @brief Should activity vary by time of day?
     */
    bool EnableTimeOfDayVariation() const { return _enableTimeOfDay; }

    /**
     * @brief Get activity multiplier for a specific hour
     */
    float GetHourlyActivityMultiplier(uint32 hour) const;

private:
    HumanizationConfig() = default;
    ~HumanizationConfig() = default;

    // Non-copyable
    HumanizationConfig(HumanizationConfig const&) = delete;
    HumanizationConfig& operator=(HumanizationConfig const&) = delete;

    /**
     * @brief Load a config value with default
     */
    template<typename T>
    T LoadValue(std::string const& key, T defaultValue) const;

    // General settings
    bool _enabled = true;
    uint32 _debugLevel = 0;
    bool _applyToExisting = true;

    // Session settings
    SessionDurationConfig _sessionConfig;
    BreakConfig _breakConfig;
    AFKConfig _afkConfig;

    // Activity settings
    std::unordered_map<ActivityCategory, ActivityConfig> _activityConfigs;

    // Gathering
    uint32 _gatheringMinDurationMs = 1800000;  // 30 min
    uint32 _gatheringMaxDurationMs = 3600000;  // 60 min
    bool _gatherUntilBagsFull = true;

    // City life
    uint32 _cityLifeMinDurationMs = 600000;    // 10 min
    uint32 _cityLifeMaxDurationMs = 1800000;   // 30 min
    bool _enableAuctionBrowsing = true;
    bool _enableInnResting = true;

    // Fishing
    uint32 _fishingMinDurationMs = 1800000;    // 30 min
    uint32 _fishingMaxDurationMs = 3600000;    // 60 min

    // Personality
    bool _assignRandomPersonalities = true;
    float _personalityVariance = 0.15f;

    // Emotes
    bool _enableIdleEmotes = true;
    float _emoteFrequency = 0.1f;

    // Time of day
    bool _enableTimeOfDay = true;
    // CRITICAL FIX: Use std::array for thread-safe direct access (no lock needed for reads)
    // These values are set once during Load() and never modified during runtime
    std::array<float, 24> _hourlyMultipliers = {{
        0.2f, 0.1f, 0.1f, 0.1f, 0.1f, 0.2f,  // 0-5 (night)
        0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f,  // 6-11 (morning)
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,  // 12-17 (afternoon)
        1.0f, 1.0f, 0.9f, 0.8f, 0.6f, 0.4f   // 18-23 (evening)
    }};

    // Thread safety - use shared_mutex for read-heavy access patterns
    // CRITICAL FIX: std::mutex caused severe contention with 100+ concurrent bots
    // std::shared_mutex allows multiple concurrent readers (bot updates) with only
    // exclusive locking during config reload (rare operation)
    mutable std::shared_mutex _mutex;
    bool _loaded = false;
};

/**
 * @brief Macro for easy config access
 */
#define sHumanizationConfig Playerbot::Humanization::HumanizationConfig::Instance()

} // namespace Humanization
} // namespace Playerbot
