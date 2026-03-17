/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PersonalityProfile.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <numeric>

namespace Playerbot
{
namespace Humanization
{

// ============================================================================
// CONSTRUCTORS
// ============================================================================

PersonalityProfile::PersonalityProfile()
    : _type(PersonalityType::CASUAL)
    , _rng(std::random_device{}())
{
    InitializeFromType(PersonalityType::CASUAL);
}

PersonalityProfile::PersonalityProfile(PersonalityType type)
    : _type(type)
    , _rng(std::random_device{}())
{
    InitializeFromType(type);
}

PersonalityProfile::PersonalityProfile(PersonalityProfile const& other)
    : _type(other._type)
    , _name(other._name)
    , _description(other._description)
    , _traits(other._traits)
    , _preferences(other._preferences)
    , _rng(std::random_device{}())
{
}

PersonalityProfile& PersonalityProfile::operator=(PersonalityProfile const& other)
{
    if (this != &other)
    {
        _type = other._type;
        _name = other._name;
        _description = other._description;
        _traits = other._traits;
        _preferences = other._preferences;
    }
    return *this;
}

// ============================================================================
// ACTIVITY PREFERENCES
// ============================================================================

float PersonalityProfile::GetActivityWeight(ActivityType activity) const
{
    for (auto const& pref : _preferences)
    {
        if (pref.activity == activity)
            return pref.weight;
    }

    // Default weight based on category preferences in traits
    ActivityCategory category = GetActivityCategory(activity);
    switch (category)
    {
        case ActivityCategory::QUESTING:   return _traits.questingPreference;
        case ActivityCategory::GATHERING:  return _traits.gatheringPreference;
        case ActivityCategory::CRAFTING:   return _traits.craftingPreference;
        case ActivityCategory::DUNGEONS:   return _traits.dungeonPreference;
        case ActivityCategory::PVP:        return _traits.pvpPreference;
        case ActivityCategory::CITY_LIFE:  return _traits.cityLifePreference;
        case ActivityCategory::FARMING:    return _traits.farmingPreference;
        default:                           return 0.5f;
    }
}

float PersonalityProfile::GetActivityDurationMultiplier(ActivityType activity) const
{
    for (auto const& pref : _preferences)
    {
        if (pref.activity == activity)
            return pref.durationMultiplier;
    }
    return _traits.sessionDurationMultiplier;
}

void PersonalityProfile::SetActivityPreference(ActivityPreference const& pref)
{
    for (auto& existing : _preferences)
    {
        if (existing.activity == pref.activity)
        {
            existing = pref;
            return;
        }
    }
    _preferences.push_back(pref);
}

// ============================================================================
// WEIGHTED SELECTION
// ============================================================================

ActivityType PersonalityProfile::SelectWeightedActivity(std::vector<ActivityType> const& availableActivities) const
{
    if (availableActivities.empty())
        return ActivityType::NONE;

    if (availableActivities.size() == 1)
        return availableActivities[0];

    // Calculate total weight
    float totalWeight = 0.0f;
    std::vector<float> weights;
    weights.reserve(availableActivities.size());

    for (ActivityType activity : availableActivities)
    {
        float weight = GetActivityWeight(activity);
        weights.push_back(weight);
        totalWeight += weight;
    }

    if (totalWeight <= 0.0f)
    {
        // Equal probability if no weights
        std::uniform_int_distribution<size_t> dist(0, availableActivities.size() - 1);
        return availableActivities[dist(_rng)];
    }

    // Weighted random selection
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float roll = dist(_rng);

    float cumulative = 0.0f;
    for (size_t i = 0; i < availableActivities.size(); ++i)
    {
        cumulative += weights[i];
        if (roll <= cumulative)
            return availableActivities[i];
    }

    return availableActivities.back();
}

float PersonalityProfile::GetCategoryPreference(ActivityCategory category) const
{
    switch (category)
    {
        case ActivityCategory::QUESTING:   return _traits.questingPreference;
        case ActivityCategory::GATHERING:  return _traits.gatheringPreference;
        case ActivityCategory::CRAFTING:   return _traits.craftingPreference;
        case ActivityCategory::DUNGEONS:   return _traits.dungeonPreference;
        case ActivityCategory::PVP:        return _traits.pvpPreference;
        case ActivityCategory::CITY_LIFE:  return _traits.cityLifePreference;
        case ActivityCategory::FARMING:    return _traits.farmingPreference;
        case ActivityCategory::SOCIAL:     return _traits.sociability;
        case ActivityCategory::EXPLORATION: return _traits.exploration;
        default:                           return 0.5f;
    }
}

// ============================================================================
// TIME-BASED MODIFIERS
// ============================================================================

float PersonalityProfile::GetCurrentTimeMultiplier() const
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&time);
    return GetTimeMultiplier(localTime->tm_hour);
}

float PersonalityProfile::GetTimeMultiplier(uint32 hour) const
{
    if (hour >= 6 && hour < 12)
        return _traits.morningActivity;
    else if (hour >= 12 && hour < 18)
        return _traits.afternoonActivity;
    else if (hour >= 18 && hour < 24)
        return _traits.eveningActivity;
    else
        return _traits.nightActivity;
}

// ============================================================================
// SESSION DURATION
// ============================================================================

uint32 PersonalityProfile::CalculateSessionDuration(ActivityType activity, uint32 baseMinMs, uint32 baseMaxMs) const
{
    float activityMult = GetActivityDurationMultiplier(activity);
    float sessionMult = _traits.sessionDurationMultiplier;
    float timeMult = GetCurrentTimeMultiplier();

    float finalMult = activityMult * sessionMult * timeMult;

    uint32 minMs = static_cast<uint32>(baseMinMs * finalMult);
    uint32 maxMs = static_cast<uint32>(baseMaxMs * finalMult);

    if (minMs > maxMs)
        std::swap(minMs, maxMs);

    std::uniform_int_distribution<uint32> dist(minMs, maxMs);
    return dist(_rng);
}

uint32 PersonalityProfile::CalculateBreakDuration() const
{
    // Base break: 5-15 minutes
    uint32 baseMin = 300000;  // 5 min
    uint32 baseMax = 900000;  // 15 min

    uint32 minMs = static_cast<uint32>(baseMin * _traits.breakDurationMultiplier);
    uint32 maxMs = static_cast<uint32>(baseMax * _traits.breakDurationMultiplier);

    std::uniform_int_distribution<uint32> dist(minMs, maxMs);
    return dist(_rng);
}

bool PersonalityProfile::ShouldTakeBreak(uint32 currentSessionDurationMs) const
{
    // Minimum 30 minutes before considering a break
    if (currentSessionDurationMs < 1800000)
        return false;

    // Increase break chance over time
    float sessionHours = currentSessionDurationMs / 3600000.0f;
    float breakChance = _traits.breakFrequency * sessionHours;

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(_rng) < breakChance;
}

// ============================================================================
// RANDOMIZATION
// ============================================================================

void PersonalityProfile::ApplyRandomVariation(float variance)
{
    std::uniform_real_distribution<float> dist(-variance, variance);

    auto vary = [&](float& value, float min = 0.0f, float max = 1.0f)
    {
        value = std::clamp(value + dist(_rng), min, max);
    };

    vary(_traits.reactionSpeedMultiplier, 0.5f, 2.0f);
    vary(_traits.sessionDurationMultiplier, 0.5f, 2.0f);
    vary(_traits.breakFrequency);
    vary(_traits.breakDurationMultiplier, 0.5f, 2.0f);
    vary(_traits.aggressiveness);
    vary(_traits.efficiency);
    vary(_traits.sociability);
    vary(_traits.exploration);
    vary(_traits.riskTolerance);
    vary(_traits.questingPreference);
    vary(_traits.gatheringPreference);
    vary(_traits.craftingPreference);
    vary(_traits.dungeonPreference);
    vary(_traits.pvpPreference);
    vary(_traits.cityLifePreference);
    vary(_traits.farmingPreference);
    vary(_traits.emoteFrequency);
    vary(_traits.afkFrequency, 0.0f, 0.2f);
}

// ============================================================================
// STATIC FACTORY METHODS
// ============================================================================

PersonalityProfile PersonalityProfile::CreateProfile(PersonalityType type)
{
    return PersonalityProfile(type);
}

PersonalityProfile PersonalityProfile::CreateRandomProfile()
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(PersonalityType::MAX_PERSONALITY) - 1);

    PersonalityType type = static_cast<PersonalityType>(dist(rng));
    PersonalityProfile profile(type);
    profile.ApplyRandomVariation(0.2f);
    return profile;
}

std::string PersonalityProfile::GetTypeName(PersonalityType type)
{
    switch (type)
    {
        case PersonalityType::CASUAL:        return "Casual";
        case PersonalityType::HARDCORE:      return "Hardcore";
        case PersonalityType::SOCIAL:        return "Social";
        case PersonalityType::EXPLORER:      return "Explorer";
        case PersonalityType::FARMER:        return "Farmer";
        case PersonalityType::PVP_ORIENTED:  return "PvP Oriented";
        case PersonalityType::COMPLETIONIST: return "Completionist";
        case PersonalityType::SPEEDRUNNER:   return "Speedrunner";
        case PersonalityType::ROLEPLAYER:    return "Roleplayer";
        case PersonalityType::NEWCOMER:      return "Newcomer";
        default:                             return "Unknown";
    }
}

// ============================================================================
// PROFILE INITIALIZATION
// ============================================================================

void PersonalityProfile::InitializeFromType(PersonalityType type)
{
    _type = type;

    switch (type)
    {
        case PersonalityType::CASUAL:        InitializeCasual(); break;
        case PersonalityType::HARDCORE:      InitializeHardcore(); break;
        case PersonalityType::SOCIAL:        InitializeSocial(); break;
        case PersonalityType::EXPLORER:      InitializeExplorer(); break;
        case PersonalityType::FARMER:        InitializeFarmer(); break;
        case PersonalityType::PVP_ORIENTED:  InitializePvPOriented(); break;
        case PersonalityType::COMPLETIONIST: InitializeCompletionist(); break;
        case PersonalityType::SPEEDRUNNER:   InitializeSpeedrunner(); break;
        case PersonalityType::ROLEPLAYER:    InitializeRoleplayer(); break;
        case PersonalityType::NEWCOMER:      InitializeNewcomer(); break;
        default:                             InitializeCasual(); break;
    }
}

void PersonalityProfile::InitializeCasual()
{
    _name = "Casual";
    _description = "Relaxed player who enjoys varied activities with frequent breaks";

    _traits.reactionSpeedMultiplier = 1.0f;
    _traits.sessionDurationMultiplier = 0.8f;
    _traits.breakFrequency = 0.2f;
    _traits.breakDurationMultiplier = 1.2f;
    _traits.aggressiveness = 0.4f;
    _traits.efficiency = 0.5f;
    _traits.sociability = 0.6f;
    _traits.exploration = 0.6f;
    _traits.riskTolerance = 0.4f;
    _traits.questingPreference = 0.6f;
    _traits.gatheringPreference = 0.4f;
    _traits.craftingPreference = 0.3f;
    _traits.dungeonPreference = 0.4f;
    _traits.pvpPreference = 0.2f;
    _traits.cityLifePreference = 0.5f;
    _traits.farmingPreference = 0.3f;
    _traits.emoteFrequency = 0.15f;
    _traits.afkFrequency = 0.1f;
    _traits.morningActivity = 0.4f;
    _traits.afternoonActivity = 0.8f;
    _traits.eveningActivity = 1.0f;
    _traits.nightActivity = 0.2f;
}

void PersonalityProfile::InitializeHardcore()
{
    _name = "Hardcore";
    _description = "Efficient, goal-oriented player with long focused sessions";

    _traits.reactionSpeedMultiplier = 1.3f;
    _traits.sessionDurationMultiplier = 1.5f;
    _traits.breakFrequency = 0.05f;
    _traits.breakDurationMultiplier = 0.5f;
    _traits.aggressiveness = 0.7f;
    _traits.efficiency = 0.9f;
    _traits.sociability = 0.3f;
    _traits.exploration = 0.3f;
    _traits.riskTolerance = 0.6f;
    _traits.questingPreference = 0.7f;
    _traits.gatheringPreference = 0.3f;
    _traits.craftingPreference = 0.4f;
    _traits.dungeonPreference = 0.8f;
    _traits.pvpPreference = 0.5f;
    _traits.cityLifePreference = 0.2f;
    _traits.farmingPreference = 0.6f;
    _traits.emoteFrequency = 0.02f;
    _traits.afkFrequency = 0.02f;
    _traits.morningActivity = 0.6f;
    _traits.afternoonActivity = 1.0f;
    _traits.eveningActivity = 1.0f;
    _traits.nightActivity = 0.7f;
}

void PersonalityProfile::InitializeSocial()
{
    _name = "Social";
    _description = "Player who loves group content, chatting, and guild activities";

    _traits.reactionSpeedMultiplier = 0.9f;
    _traits.sessionDurationMultiplier = 1.0f;
    _traits.breakFrequency = 0.15f;
    _traits.breakDurationMultiplier = 1.0f;
    _traits.aggressiveness = 0.4f;
    _traits.efficiency = 0.4f;
    _traits.sociability = 0.95f;
    _traits.exploration = 0.5f;
    _traits.riskTolerance = 0.5f;
    _traits.questingPreference = 0.5f;
    _traits.gatheringPreference = 0.3f;
    _traits.craftingPreference = 0.4f;
    _traits.dungeonPreference = 0.7f;
    _traits.pvpPreference = 0.4f;
    _traits.cityLifePreference = 0.8f;
    _traits.farmingPreference = 0.2f;
    _traits.emoteFrequency = 0.25f;
    _traits.afkFrequency = 0.08f;
    _traits.morningActivity = 0.3f;
    _traits.afternoonActivity = 0.7f;
    _traits.eveningActivity = 1.0f;
    _traits.nightActivity = 0.5f;
}

void PersonalityProfile::InitializeExplorer()
{
    _name = "Explorer";
    _description = "Player who loves exploring new areas, finding hidden content, and achievements";

    _traits.reactionSpeedMultiplier = 0.8f;
    _traits.sessionDurationMultiplier = 1.2f;
    _traits.breakFrequency = 0.1f;
    _traits.breakDurationMultiplier = 1.0f;
    _traits.aggressiveness = 0.3f;
    _traits.efficiency = 0.3f;
    _traits.sociability = 0.4f;
    _traits.exploration = 0.95f;
    _traits.riskTolerance = 0.7f;
    _traits.questingPreference = 0.7f;
    _traits.gatheringPreference = 0.5f;
    _traits.craftingPreference = 0.3f;
    _traits.dungeonPreference = 0.4f;
    _traits.pvpPreference = 0.2f;
    _traits.cityLifePreference = 0.3f;
    _traits.farmingPreference = 0.2f;
    _traits.emoteFrequency = 0.1f;
    _traits.afkFrequency = 0.05f;
    _traits.morningActivity = 0.5f;
    _traits.afternoonActivity = 1.0f;
    _traits.eveningActivity = 0.8f;
    _traits.nightActivity = 0.4f;

    // Add exploration-specific preferences
    _preferences.push_back(ActivityPreference(ActivityType::ZONE_EXPLORATION, 0.9f, 1.5f));
    _preferences.push_back(ActivityPreference(ActivityType::ACHIEVEMENT_HUNTING, 0.8f, 1.3f));
}

void PersonalityProfile::InitializeFarmer()
{
    _name = "Farmer";
    _description = "Player focused on farming, gold making, and resource gathering";

    _traits.reactionSpeedMultiplier = 1.1f;
    _traits.sessionDurationMultiplier = 1.8f;
    _traits.breakFrequency = 0.08f;
    _traits.breakDurationMultiplier = 0.6f;
    _traits.aggressiveness = 0.5f;
    _traits.efficiency = 0.85f;
    _traits.sociability = 0.2f;
    _traits.exploration = 0.3f;
    _traits.riskTolerance = 0.4f;
    _traits.questingPreference = 0.3f;
    _traits.gatheringPreference = 0.9f;
    _traits.craftingPreference = 0.6f;
    _traits.dungeonPreference = 0.3f;
    _traits.pvpPreference = 0.1f;
    _traits.cityLifePreference = 0.4f;
    _traits.farmingPreference = 0.95f;
    _traits.emoteFrequency = 0.02f;
    _traits.afkFrequency = 0.03f;
    _traits.morningActivity = 0.7f;
    _traits.afternoonActivity = 1.0f;
    _traits.eveningActivity = 0.8f;
    _traits.nightActivity = 0.3f;

    // Add farming-specific preferences
    _preferences.push_back(ActivityPreference(ActivityType::MINING, 0.9f, 2.0f));
    _preferences.push_back(ActivityPreference(ActivityType::HERBALISM, 0.9f, 2.0f));
    _preferences.push_back(ActivityPreference(ActivityType::GOLD_FARMING, 0.85f, 1.8f));
    _preferences.push_back(ActivityPreference(ActivityType::AUCTION_POSTING, 0.7f, 1.0f));
}

void PersonalityProfile::InitializePvPOriented()
{
    _name = "PvP Oriented";
    _description = "Player who prefers PvP content including battlegrounds, arenas, and world PvP";

    _traits.reactionSpeedMultiplier = 1.4f;
    _traits.sessionDurationMultiplier = 1.0f;
    _traits.breakFrequency = 0.1f;
    _traits.breakDurationMultiplier = 0.8f;
    _traits.aggressiveness = 0.9f;
    _traits.efficiency = 0.7f;
    _traits.sociability = 0.5f;
    _traits.exploration = 0.3f;
    _traits.riskTolerance = 0.8f;
    _traits.questingPreference = 0.3f;
    _traits.gatheringPreference = 0.2f;
    _traits.craftingPreference = 0.3f;
    _traits.dungeonPreference = 0.4f;
    _traits.pvpPreference = 0.95f;
    _traits.cityLifePreference = 0.4f;
    _traits.farmingPreference = 0.3f;
    _traits.emoteFrequency = 0.1f;
    _traits.afkFrequency = 0.05f;
    _traits.morningActivity = 0.4f;
    _traits.afternoonActivity = 0.8f;
    _traits.eveningActivity = 1.0f;
    _traits.nightActivity = 0.6f;

    // Add PvP-specific preferences
    _preferences.push_back(ActivityPreference(ActivityType::BATTLEGROUND, 0.9f, 1.2f));
    _preferences.push_back(ActivityPreference(ActivityType::ARENA, 0.85f, 1.0f));
    _preferences.push_back(ActivityPreference(ActivityType::WORLD_PVP, 0.8f, 1.5f));
}

void PersonalityProfile::InitializeCompletionist()
{
    _name = "Completionist";
    _description = "Player who does everything - achievements, collections, all content";

    _traits.reactionSpeedMultiplier = 1.0f;
    _traits.sessionDurationMultiplier = 1.3f;
    _traits.breakFrequency = 0.1f;
    _traits.breakDurationMultiplier = 1.0f;
    _traits.aggressiveness = 0.5f;
    _traits.efficiency = 0.6f;
    _traits.sociability = 0.5f;
    _traits.exploration = 0.8f;
    _traits.riskTolerance = 0.5f;
    _traits.questingPreference = 0.8f;
    _traits.gatheringPreference = 0.6f;
    _traits.craftingPreference = 0.7f;
    _traits.dungeonPreference = 0.7f;
    _traits.pvpPreference = 0.5f;
    _traits.cityLifePreference = 0.5f;
    _traits.farmingPreference = 0.6f;
    _traits.emoteFrequency = 0.1f;
    _traits.afkFrequency = 0.05f;
    _traits.morningActivity = 0.5f;
    _traits.afternoonActivity = 1.0f;
    _traits.eveningActivity = 1.0f;
    _traits.nightActivity = 0.4f;

    // Add completionist-specific preferences
    _preferences.push_back(ActivityPreference(ActivityType::ACHIEVEMENT_HUNTING, 0.85f, 1.5f));
    _preferences.push_back(ActivityPreference(ActivityType::MOUNT_FARMING, 0.8f, 1.3f));
    _preferences.push_back(ActivityPreference(ActivityType::TRANSMOG_FARMING, 0.75f, 1.2f));
}

void PersonalityProfile::InitializeSpeedrunner()
{
    _name = "Speedrunner";
    _description = "Efficient dungeon runner focused on fast clears";

    _traits.reactionSpeedMultiplier = 1.5f;
    _traits.sessionDurationMultiplier = 1.0f;
    _traits.breakFrequency = 0.05f;
    _traits.breakDurationMultiplier = 0.4f;
    _traits.aggressiveness = 0.8f;
    _traits.efficiency = 0.95f;
    _traits.sociability = 0.4f;
    _traits.exploration = 0.1f;
    _traits.riskTolerance = 0.7f;
    _traits.questingPreference = 0.3f;
    _traits.gatheringPreference = 0.1f;
    _traits.craftingPreference = 0.2f;
    _traits.dungeonPreference = 0.95f;
    _traits.pvpPreference = 0.3f;
    _traits.cityLifePreference = 0.2f;
    _traits.farmingPreference = 0.3f;
    _traits.emoteFrequency = 0.01f;
    _traits.afkFrequency = 0.01f;
    _traits.morningActivity = 0.6f;
    _traits.afternoonActivity = 1.0f;
    _traits.eveningActivity = 1.0f;
    _traits.nightActivity = 0.5f;

    // Add speedrunner-specific preferences
    _preferences.push_back(ActivityPreference(ActivityType::DUNGEON_RUN, 0.95f, 0.8f)); // Shorter duration!
}

void PersonalityProfile::InitializeRoleplayer()
{
    _name = "Roleplayer";
    _description = "Player who enjoys emotes, staying in character, and immersive play";

    _traits.reactionSpeedMultiplier = 0.7f;
    _traits.sessionDurationMultiplier = 1.0f;
    _traits.breakFrequency = 0.15f;
    _traits.breakDurationMultiplier = 1.5f;
    _traits.aggressiveness = 0.3f;
    _traits.efficiency = 0.3f;
    _traits.sociability = 0.8f;
    _traits.exploration = 0.7f;
    _traits.riskTolerance = 0.4f;
    _traits.questingPreference = 0.6f;
    _traits.gatheringPreference = 0.4f;
    _traits.craftingPreference = 0.5f;
    _traits.dungeonPreference = 0.4f;
    _traits.pvpPreference = 0.2f;
    _traits.cityLifePreference = 0.7f;
    _traits.farmingPreference = 0.2f;
    _traits.emoteFrequency = 0.4f;
    _traits.afkFrequency = 0.1f;
    _traits.morningActivity = 0.3f;
    _traits.afternoonActivity = 0.6f;
    _traits.eveningActivity = 1.0f;
    _traits.nightActivity = 0.4f;

    // Add RP-specific preferences
    _preferences.push_back(ActivityPreference(ActivityType::EMOTING, 0.9f, 2.0f));
    _preferences.push_back(ActivityPreference(ActivityType::CITY_WANDERING, 0.8f, 1.5f));
    _preferences.push_back(ActivityPreference(ActivityType::INN_REST, 0.7f, 2.0f));
}

void PersonalityProfile::InitializeNewcomer()
{
    _name = "Newcomer";
    _description = "New player learning the game - slower reactions, frequent pauses";

    _traits.reactionSpeedMultiplier = 0.6f;
    _traits.sessionDurationMultiplier = 0.7f;
    _traits.breakFrequency = 0.2f;
    _traits.breakDurationMultiplier = 1.5f;
    _traits.aggressiveness = 0.3f;
    _traits.efficiency = 0.3f;
    _traits.sociability = 0.5f;
    _traits.exploration = 0.8f;
    _traits.riskTolerance = 0.2f;
    _traits.questingPreference = 0.8f;
    _traits.gatheringPreference = 0.3f;
    _traits.craftingPreference = 0.2f;
    _traits.dungeonPreference = 0.3f;
    _traits.pvpPreference = 0.1f;
    _traits.cityLifePreference = 0.5f;
    _traits.farmingPreference = 0.2f;
    _traits.emoteFrequency = 0.05f;
    _traits.afkFrequency = 0.15f;
    _traits.typoRate = 0.08f; // More typos!
    _traits.morningActivity = 0.5f;
    _traits.afternoonActivity = 0.8f;
    _traits.eveningActivity = 1.0f;
    _traits.nightActivity = 0.2f;

    // Newcomers explore and quest more
    _preferences.push_back(ActivityPreference(ActivityType::ZONE_EXPLORATION, 0.8f, 1.2f));
    _preferences.push_back(ActivityPreference(ActivityType::QUEST_OBJECTIVE, 0.85f, 1.3f));
}

} // namespace Humanization
} // namespace Playerbot
