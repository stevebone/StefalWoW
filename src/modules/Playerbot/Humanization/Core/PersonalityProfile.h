/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * PERSONALITY PROFILE
 *
 * Phase 3: Humanization Core
 *
 * Defines personality profiles that affect how bots behave.
 * Different profiles create different play styles and behaviors.
 */

#pragma once

#include "Define.h"
#include "ActivityType.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <random>

namespace Playerbot
{
namespace Humanization
{

/**
 * @brief Pre-defined personality types
 */
enum class PersonalityType : uint8
{
    CASUAL = 0,         // Relaxed, varied activities, frequent breaks
    HARDCORE,           // Efficient, goal-oriented, long sessions
    SOCIAL,             // Prefers group content, chatting, guilds
    EXPLORER,           // Loves exploring, achievements, lore
    FARMER,             // Focuses on farming, gold making
    PVP_ORIENTED,       // Prefers PvP content
    COMPLETIONIST,      // Does everything, achievements, collections
    SPEEDRUNNER,        // Efficient dungeon runs
    ROLEPLAYER,         // Heavy emote usage, stays in character
    NEWCOMER,           // New player behavior, slower, learning

    MAX_PERSONALITY
};

/**
 * @brief Activity preference weight
 */
struct ActivityPreference
{
    ActivityType activity;
    float weight;               // 0.0-1.0, higher = more likely
    float durationMultiplier;   // Multiplier for session duration

    ActivityPreference()
        : activity(ActivityType::NONE)
        , weight(0.5f)
        , durationMultiplier(1.0f)
    {}

    ActivityPreference(ActivityType act, float w, float durMult = 1.0f)
        : activity(act)
        , weight(w)
        , durationMultiplier(durMult)
    {}
};

/**
 * @brief Personality traits that affect behavior
 */
struct PersonalityTraits
{
    // Timing traits
    float reactionSpeedMultiplier = 1.0f;   // How fast bot reacts (0.5-2.0)
    float sessionDurationMultiplier = 1.0f; // How long sessions last
    float breakFrequency = 0.1f;            // How often to take breaks (0.0-1.0)
    float breakDurationMultiplier = 1.0f;   // How long breaks last

    // Behavior traits
    float aggressiveness = 0.5f;            // Combat style (0=defensive, 1=aggressive)
    float efficiency = 0.5f;                // How optimized is gameplay (0-1)
    float sociability = 0.5f;               // Likelihood to engage in social activities
    float exploration = 0.5f;               // Tendency to explore
    float riskTolerance = 0.5f;             // Willingness to take risks

    // Activity preferences
    float questingPreference = 0.5f;
    float gatheringPreference = 0.5f;
    float craftingPreference = 0.5f;
    float dungeonPreference = 0.5f;
    float pvpPreference = 0.5f;
    float cityLifePreference = 0.5f;
    float farmingPreference = 0.5f;

    // Emote and interaction
    float emoteFrequency = 0.1f;            // How often to emote
    float afkFrequency = 0.05f;             // How often to go AFK
    float typoRate = 0.02f;                 // Chance of typing errors (for chat)

    // Time-of-day preferences (multipliers for activity during each period)
    float morningActivity = 0.5f;           // 6am-12pm
    float afternoonActivity = 1.0f;         // 12pm-6pm
    float eveningActivity = 1.0f;           // 6pm-12am
    float nightActivity = 0.3f;             // 12am-6am
};

/**
 * @brief Full personality profile for a bot
 */
class TC_GAME_API PersonalityProfile
{
public:
    /**
     * @brief Construct a default personality profile
     */
    PersonalityProfile();

    /**
     * @brief Construct a profile from a personality type
     * @param type The personality type to use
     */
    explicit PersonalityProfile(PersonalityType type);

    /**
     * @brief Copy constructor
     */
    PersonalityProfile(PersonalityProfile const& other);

    /**
     * @brief Assignment operator
     */
    PersonalityProfile& operator=(PersonalityProfile const& other);

    ~PersonalityProfile() = default;

    // ========================================================================
    // IDENTITY
    // ========================================================================

    /**
     * @brief Get the personality type
     */
    PersonalityType GetType() const { return _type; }

    /**
     * @brief Get profile name
     */
    std::string const& GetName() const { return _name; }

    /**
     * @brief Set profile name
     */
    void SetName(std::string const& name) { _name = name; }

    /**
     * @brief Get profile description
     */
    std::string const& GetDescription() const { return _description; }

    // ========================================================================
    // TRAITS
    // ========================================================================

    /**
     * @brief Get personality traits
     */
    PersonalityTraits const& GetTraits() const { return _traits; }

    /**
     * @brief Get mutable reference to traits for modification
     */
    PersonalityTraits& GetTraitsMutable() { return _traits; }

    /**
     * @brief Set personality traits
     */
    void SetTraits(PersonalityTraits const& traits) { _traits = traits; }

    // ========================================================================
    // ACTIVITY PREFERENCES
    // ========================================================================

    /**
     * @brief Get activity preference weight
     * @param activity The activity to check
     * @return Weight (0.0-1.0)
     */
    float GetActivityWeight(ActivityType activity) const;

    /**
     * @brief Get activity duration multiplier
     * @param activity The activity to check
     * @return Duration multiplier
     */
    float GetActivityDurationMultiplier(ActivityType activity) const;

    /**
     * @brief Set activity preference
     * @param pref The preference to set
     */
    void SetActivityPreference(ActivityPreference const& pref);

    /**
     * @brief Get all activity preferences
     */
    std::vector<ActivityPreference> const& GetAllPreferences() const { return _preferences; }

    // ========================================================================
    // WEIGHTED SELECTION
    // ========================================================================

    /**
     * @brief Select a random activity based on preferences
     * @param availableActivities List of activities currently available
     * @return Selected activity type
     */
    ActivityType SelectWeightedActivity(std::vector<ActivityType> const& availableActivities) const;

    /**
     * @brief Get category preference (derived from activity preferences)
     * @param category The category to check
     * @return Preference weight (0.0-1.0)
     */
    float GetCategoryPreference(ActivityCategory category) const;

    // ========================================================================
    // TIME-BASED MODIFIERS
    // ========================================================================

    /**
     * @brief Get activity multiplier for current time of day
     * @return Multiplier based on personality's time preferences
     */
    float GetCurrentTimeMultiplier() const;

    /**
     * @brief Get activity multiplier for a specific hour
     * @param hour Hour of day (0-23)
     * @return Multiplier
     */
    float GetTimeMultiplier(uint32 hour) const;

    // ========================================================================
    // SESSION DURATION
    // ========================================================================

    /**
     * @brief Calculate session duration for an activity
     * @param activity The activity
     * @param baseMinMs Base minimum duration
     * @param baseMaxMs Base maximum duration
     * @return Calculated duration in milliseconds
     */
    uint32 CalculateSessionDuration(ActivityType activity, uint32 baseMinMs, uint32 baseMaxMs) const;

    /**
     * @brief Calculate break duration
     * @return Break duration in milliseconds
     */
    uint32 CalculateBreakDuration() const;

    /**
     * @brief Should take a break now?
     * @param currentSessionDurationMs How long current session has been
     * @return True if should take break
     */
    bool ShouldTakeBreak(uint32 currentSessionDurationMs) const;

    // ========================================================================
    // RANDOMIZATION
    // ========================================================================

    /**
     * @brief Apply randomization to create variation
     * @param variance Amount of variance (0.0-1.0)
     */
    void ApplyRandomVariation(float variance = 0.1f);

    // ========================================================================
    // STATIC FACTORY METHODS
    // ========================================================================

    /**
     * @brief Create a profile for a specific personality type
     */
    static PersonalityProfile CreateProfile(PersonalityType type);

    /**
     * @brief Create a random profile
     */
    static PersonalityProfile CreateRandomProfile();

    /**
     * @brief Get name for a personality type
     */
    static std::string GetTypeName(PersonalityType type);

private:
    /**
     * @brief Initialize profile based on type
     */
    void InitializeFromType(PersonalityType type);

    /**
     * @brief Initialize casual profile
     */
    void InitializeCasual();

    /**
     * @brief Initialize hardcore profile
     */
    void InitializeHardcore();

    /**
     * @brief Initialize social profile
     */
    void InitializeSocial();

    /**
     * @brief Initialize explorer profile
     */
    void InitializeExplorer();

    /**
     * @brief Initialize farmer profile
     */
    void InitializeFarmer();

    /**
     * @brief Initialize PvP-oriented profile
     */
    void InitializePvPOriented();

    /**
     * @brief Initialize completionist profile
     */
    void InitializeCompletionist();

    /**
     * @brief Initialize speedrunner profile
     */
    void InitializeSpeedrunner();

    /**
     * @brief Initialize roleplayer profile
     */
    void InitializeRoleplayer();

    /**
     * @brief Initialize newcomer profile
     */
    void InitializeNewcomer();

    // Data members
    PersonalityType _type{PersonalityType::CASUAL};
    std::string _name;
    std::string _description;
    PersonalityTraits _traits;
    std::vector<ActivityPreference> _preferences;

    // Random generator for weighted selection
    mutable std::mt19937 _rng;
};

} // namespace Humanization
} // namespace Playerbot
