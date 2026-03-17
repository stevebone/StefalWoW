/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "HumanizationConfig.h"
#include "Config/ConfigManager.h"
#include "Log.h"

namespace Playerbot
{
namespace Humanization
{

HumanizationConfig& HumanizationConfig::Instance()
{
    static HumanizationConfig instance;
    return instance;
}

void HumanizationConfig::Load()
{
    // CRITICAL FIX: Use exclusive lock only during config load/reload
    // This is a rare operation (server startup or admin reload command)
    std::unique_lock<std::shared_mutex> lock(_mutex);

    TC_LOG_INFO("module.playerbot.humanization", "Loading Humanization configuration...");

    auto* config = ConfigManager::instance();

    // ========================================================================
    // GENERAL SETTINGS
    // ========================================================================
    _enabled = config->GetBool("Humanization.Enabled", true);
    _debugLevel = config->GetUInt("Humanization.DebugLevel", 0);
    _applyToExisting = config->GetBool("Humanization.ApplyToExisting", true);

    // ========================================================================
    // SESSION CONFIGURATION
    // ========================================================================
    _sessionConfig.minDurationMs = config->GetUInt("Humanization.Session.MinDuration", 1800000);
    _sessionConfig.maxDurationMs = config->GetUInt("Humanization.Session.MaxDuration", 5400000);
    _sessionConfig.extendChancePercent = config->GetUInt("Humanization.Session.ExtendChance", 20);
    _sessionConfig.maxExtensions = config->GetUInt("Humanization.Session.MaxExtensions", 2);

    // ========================================================================
    // BREAK CONFIGURATION
    // ========================================================================
    _breakConfig.shortBreakMinMs = config->GetUInt("Humanization.Break.Short.Min", 60000);
    _breakConfig.shortBreakMaxMs = config->GetUInt("Humanization.Break.Short.Max", 300000);
    _breakConfig.longBreakMinMs = config->GetUInt("Humanization.Break.Long.Min", 300000);
    _breakConfig.longBreakMaxMs = config->GetUInt("Humanization.Break.Long.Max", 900000);
    _breakConfig.bioBreakMinMs = config->GetUInt("Humanization.Break.Bio.Min", 120000);
    _breakConfig.bioBreakMaxMs = config->GetUInt("Humanization.Break.Bio.Max", 600000);
    _breakConfig.longBreakChancePercent = config->GetUInt("Humanization.Break.LongChance", 10);

    // ========================================================================
    // AFK CONFIGURATION
    // ========================================================================
    _afkConfig.enabled = config->GetBool("Humanization.AFK.Enabled", true);
    _afkConfig.chancePerHourPercent = config->GetUInt("Humanization.AFK.ChancePerHour", 5);
    _afkConfig.shortAFKMinMs = config->GetUInt("Humanization.AFK.Short.Min", 60000);
    _afkConfig.shortAFKMaxMs = config->GetUInt("Humanization.AFK.Short.Max", 300000);
    _afkConfig.mediumAFKMinMs = config->GetUInt("Humanization.AFK.Medium.Min", 300000);
    _afkConfig.mediumAFKMaxMs = config->GetUInt("Humanization.AFK.Medium.Max", 900000);
    _afkConfig.longAFKMinMs = config->GetUInt("Humanization.AFK.Long.Min", 900000);
    _afkConfig.longAFKMaxMs = config->GetUInt("Humanization.AFK.Long.Max", 1800000);

    // ========================================================================
    // GATHERING CONFIGURATION
    // ========================================================================
    _gatheringMinDurationMs = config->GetUInt("Humanization.Gathering.MinDuration", 1800000);
    _gatheringMaxDurationMs = config->GetUInt("Humanization.Gathering.MaxDuration", 3600000);
    _gatherUntilBagsFull = config->GetBool("Humanization.Gathering.UntilBagsFull", true);

    // ========================================================================
    // CITY LIFE CONFIGURATION
    // ========================================================================
    _cityLifeMinDurationMs = config->GetUInt("Humanization.CityLife.MinDuration", 600000);
    _cityLifeMaxDurationMs = config->GetUInt("Humanization.CityLife.MaxDuration", 1800000);
    _enableAuctionBrowsing = config->GetBool("Humanization.CityLife.AuctionBrowsing", true);
    _enableInnResting = config->GetBool("Humanization.CityLife.InnResting", true);

    // ========================================================================
    // FISHING CONFIGURATION
    // ========================================================================
    _fishingMinDurationMs = config->GetUInt("Humanization.Fishing.MinDuration", 1800000);
    _fishingMaxDurationMs = config->GetUInt("Humanization.Fishing.MaxDuration", 3600000);

    // ========================================================================
    // PERSONALITY CONFIGURATION
    // ========================================================================
    _assignRandomPersonalities = config->GetBool("Humanization.Personality.Random", true);
    _personalityVariance = config->GetFloat("Humanization.Personality.Variance", 0.15f);

    // ========================================================================
    // EMOTE CONFIGURATION
    // ========================================================================
    _enableIdleEmotes = config->GetBool("Humanization.Emotes.Enabled", true);
    _emoteFrequency = config->GetFloat("Humanization.Emotes.Frequency", 0.1f);

    // ========================================================================
    // TIME OF DAY CONFIGURATION
    // ========================================================================
    _enableTimeOfDay = config->GetBool("Humanization.TimeOfDay.Enabled", true);

    // Load hourly multipliers if configured
    for (uint32 hour = 0; hour < 24; ++hour)
    {
        std::string key = "Humanization.TimeOfDay.Hour" + std::to_string(hour);
        _hourlyMultipliers[hour] = config->GetFloat(key, _hourlyMultipliers[hour]);
    }

    // ========================================================================
    // ACTIVITY-SPECIFIC CONFIGURATION
    // ========================================================================

    // Initialize activity configs with defaults
    for (uint8 i = 0; i < static_cast<uint8>(ActivityCategory::MAX_CATEGORY); ++i)
    {
        ActivityCategory cat = static_cast<ActivityCategory>(i);
        ActivityConfig actConfig;

        std::string catName = GetCategoryName(cat);

        std::string enabledKey = "Humanization.Activity." + catName + ".Enabled";
        std::string minKey = "Humanization.Activity." + catName + ".MinDuration";
        std::string maxKey = "Humanization.Activity." + catName + ".MaxDuration";
        std::string weightKey = "Humanization.Activity." + catName + ".Weight";

        actConfig.enabled = config->GetBool(enabledKey, true);
        actConfig.minDurationMs = config->GetUInt(minKey, 1800000);
        actConfig.maxDurationMs = config->GetUInt(maxKey, 3600000);
        actConfig.probabilityWeight = config->GetFloat(weightKey, 1.0f);

        _activityConfigs[cat] = actConfig;
    }

    _loaded = true;

    TC_LOG_INFO("module.playerbot.humanization",
        "Humanization configuration loaded: enabled={}, sessions={}-{}ms, "
        "gathering={}-{}ms, citylife={}-{}ms",
        _enabled ? "true" : "false",
        _sessionConfig.minDurationMs, _sessionConfig.maxDurationMs,
        _gatheringMinDurationMs, _gatheringMaxDurationMs,
        _cityLifeMinDurationMs, _cityLifeMaxDurationMs);
}

void HumanizationConfig::Reload()
{
    TC_LOG_INFO("module.playerbot.humanization", "Reloading Humanization configuration...");
    Load();
}

ActivityConfig HumanizationConfig::GetActivityConfig(ActivityCategory category) const
{
    // CRITICAL FIX: Use shared_lock for read operations
    // Allows multiple concurrent readers (100+ bot updates) without blocking
    std::shared_lock<std::shared_mutex> lock(_mutex);

    auto it = _activityConfigs.find(category);
    if (it != _activityConfigs.end())
        return it->second;

    // Return default
    ActivityConfig defaultConfig;
    return defaultConfig;
}

bool HumanizationConfig::IsActivityEnabled(ActivityCategory category) const
{
    return GetActivityConfig(category).enabled;
}

uint32 HumanizationConfig::GetActivityMinDuration(ActivityCategory category) const
{
    return GetActivityConfig(category).minDurationMs;
}

uint32 HumanizationConfig::GetActivityMaxDuration(ActivityCategory category) const
{
    return GetActivityConfig(category).maxDurationMs;
}

float HumanizationConfig::GetHourlyActivityMultiplier(uint32 hour) const
{
    if (hour >= 24)
        hour = hour % 24;

    // CRITICAL FIX: Lock-free read for hourly multipliers
    // _hourlyMultipliers is a std::array<float, 24> that is:
    // 1. Set once during Load() under exclusive lock
    // 2. Never modified during runtime
    // 3. std::array elements are POD types (float) with atomic-like read semantics on x86/x64
    //
    // This eliminates a major contention point - every bot's HumanizationManager::Update()
    // calls GetTimeOfDayMultiplier() which was acquiring mutex lock every time.
    // With 100+ concurrent bots, this was causing severe lock contention.
    return _hourlyMultipliers[hour];
}

} // namespace Humanization
} // namespace Playerbot
