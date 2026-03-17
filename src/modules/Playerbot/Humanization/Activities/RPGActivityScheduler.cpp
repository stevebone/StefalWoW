/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * RPG Activity Scheduler implementation.
 */

#include "RPGActivityScheduler.h"
#include "PersonalityProfile.h"
#include "GameTime.h"
#include <algorithm>
#include <random>
#include <ctime>

namespace Playerbot
{
namespace Humanization
{

const std::vector<ScheduleSlot>& DailySchedule::GetSlotsForPeriod(TimePeriod period) const
{
    switch (period)
    {
        case TimePeriod::EARLY_MORNING: return earlyMorning;
        case TimePeriod::MORNING:       return morning;
        case TimePeriod::AFTERNOON:     return afternoon;
        case TimePeriod::EVENING:       return evening;
        case TimePeriod::NIGHT:         return night;
        case TimePeriod::LATE_NIGHT:    return lateNight;
        default:                        return morning;
    }
}

TimePeriod RPGActivityScheduler::GetCurrentTimePeriod()
{
    // Get in-game time (WoW day is 24 hours, server tracks game time)
    time_t gameTime = GameTime::GetGameTime();
    struct tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &gameTime);
#else
    localtime_r(&gameTime, &timeInfo);
#endif

    int hour = timeInfo.tm_hour;

    if (hour >= 0 && hour < 4)   return TimePeriod::LATE_NIGHT;
    if (hour >= 4 && hour < 8)   return TimePeriod::EARLY_MORNING;
    if (hour >= 8 && hour < 12)  return TimePeriod::MORNING;
    if (hour >= 12 && hour < 16) return TimePeriod::AFTERNOON;
    if (hour >= 16 && hour < 20) return TimePeriod::EVENING;
    return TimePeriod::NIGHT;
}

const char* RPGActivityScheduler::GetTimePeriodName(TimePeriod period)
{
    switch (period)
    {
        case TimePeriod::EARLY_MORNING: return "EarlyMorning";
        case TimePeriod::MORNING:       return "Morning";
        case TimePeriod::AFTERNOON:     return "Afternoon";
        case TimePeriod::EVENING:       return "Evening";
        case TimePeriod::NIGHT:         return "Night";
        case TimePeriod::LATE_NIGHT:    return "LateNight";
        default:                        return "Unknown";
    }
}

DailySchedule RPGActivityScheduler::GenerateSchedule(const PersonalityProfile& personality) const
{
    DailySchedule schedule;

    schedule.earlyMorning = GeneratePeriodSlots(personality, TimePeriod::EARLY_MORNING);
    schedule.morning      = GeneratePeriodSlots(personality, TimePeriod::MORNING);
    schedule.afternoon    = GeneratePeriodSlots(personality, TimePeriod::AFTERNOON);
    schedule.evening      = GeneratePeriodSlots(personality, TimePeriod::EVENING);
    schedule.night        = GeneratePeriodSlots(personality, TimePeriod::NIGHT);
    schedule.lateNight    = GeneratePeriodSlots(personality, TimePeriod::LATE_NIGHT);

    return schedule;
}

std::vector<ScheduleSlot> RPGActivityScheduler::GeneratePeriodSlots(
    const PersonalityProfile& personality, TimePeriod period) const
{
    std::vector<ScheduleSlot> slots;

    float combatWeight, questWeight, gatherWeight, socialWeight, exploreWeight, restWeight;
    GetPeriodWeights(personality, period,
                     combatWeight, questWeight, gatherWeight,
                     socialWeight, exploreWeight, restWeight);

    // Always available activities
    if (combatWeight > 0.0f)
    {
        slots.emplace_back(ActivityType::SOLO_COMBAT, 30.0f, combatWeight);
        slots.emplace_back(ActivityType::DUNGEON_RUN, 45.0f, combatWeight * 0.5f);
    }

    if (questWeight > 0.0f)
    {
        slots.emplace_back(ActivityType::QUEST_OBJECTIVE, 40.0f, questWeight);
        slots.emplace_back(ActivityType::QUEST_PICKUP, 20.0f, questWeight * 0.6f);
        slots.emplace_back(ActivityType::QUEST_TRAVEL, 15.0f, questWeight * 0.4f);
    }

    if (gatherWeight > 0.0f)
    {
        slots.emplace_back(ActivityType::MINING, 20.0f, gatherWeight);
        slots.emplace_back(ActivityType::HERBALISM, 20.0f, gatherWeight);
        slots.emplace_back(ActivityType::FISHING, 25.0f, gatherWeight * 0.7f);
        slots.emplace_back(ActivityType::SKINNING, 15.0f, gatherWeight * 0.5f);
    }

    if (socialWeight > 0.0f)
    {
        slots.emplace_back(ActivityType::AUCTION_BROWSING, 10.0f, socialWeight);
        slots.emplace_back(ActivityType::BANK_VISIT, 5.0f, socialWeight * 0.5f);
        slots.emplace_back(ActivityType::VENDOR_VISIT, 5.0f, socialWeight * 0.5f);
        slots.emplace_back(ActivityType::CITY_WANDERING, 15.0f, socialWeight * 0.8f);
        slots.emplace_back(ActivityType::EMOTING, 5.0f, socialWeight * 0.3f);
    }

    if (exploreWeight > 0.0f)
    {
        slots.emplace_back(ActivityType::ZONE_EXPLORATION, 30.0f, exploreWeight);
        slots.emplace_back(ActivityType::ACHIEVEMENT_HUNTING, 20.0f, exploreWeight * 0.6f);
    }

    if (restWeight > 0.0f)
    {
        slots.emplace_back(ActivityType::INN_REST, 15.0f, restWeight);
        slots.emplace_back(ActivityType::AFK_SHORT, 5.0f, restWeight * 0.4f);
    }

    // Late night / early morning: add mandatory rest
    if (period == TimePeriod::LATE_NIGHT || period == TimePeriod::EARLY_MORNING)
    {
        slots.emplace_back(ActivityType::INN_REST, 30.0f, 2.0f, true);
    }

    return slots;
}

void RPGActivityScheduler::GetPeriodWeights(const PersonalityProfile& personality,
                                             TimePeriod period,
                                             float& combatWeight, float& questWeight,
                                             float& gatherWeight, float& socialWeight,
                                             float& exploreWeight, float& restWeight) const
{
    const auto& traits = personality.GetTraits();

    // Base weights from personality traits
    float aggressiveness = traits.aggressiveness;
    float sociability = traits.sociability;
    float exploration = traits.exploration;

    // Base activity preferences from personality
    combatWeight  = traits.questingPreference * aggressiveness;
    questWeight   = traits.questingPreference;
    gatherWeight  = traits.gatheringPreference;
    socialWeight  = traits.cityLifePreference * sociability;
    exploreWeight = exploration * 0.5f;
    restWeight    = 0.2f;

    // Time-of-day modifiers
    float timeMultiplier = 1.0f;
    switch (period)
    {
        case TimePeriod::EARLY_MORNING:
            timeMultiplier = traits.morningActivity * 0.5f;
            restWeight *= 2.0f;
            combatWeight *= 0.3f;
            socialWeight *= 0.2f;
            break;
        case TimePeriod::MORNING:
            timeMultiplier = traits.morningActivity;
            questWeight *= 1.5f;
            gatherWeight *= 1.3f;
            break;
        case TimePeriod::AFTERNOON:
            timeMultiplier = traits.afternoonActivity;
            combatWeight *= 1.3f;
            questWeight *= 1.2f;
            break;
        case TimePeriod::EVENING:
            timeMultiplier = traits.eveningActivity;
            socialWeight *= 2.0f;
            combatWeight *= 1.5f;
            break;
        case TimePeriod::NIGHT:
            timeMultiplier = traits.nightActivity;
            socialWeight *= 1.5f;
            restWeight *= 1.5f;
            gatherWeight *= 0.5f;
            break;
        case TimePeriod::LATE_NIGHT:
            timeMultiplier = traits.nightActivity * 0.3f;
            restWeight *= 3.0f;
            combatWeight *= 0.1f;
            questWeight *= 0.1f;
            gatherWeight *= 0.1f;
            socialWeight *= 0.1f;
            break;
    }

    // Apply time multiplier to all non-rest weights
    combatWeight *= timeMultiplier;
    questWeight *= timeMultiplier;
    gatherWeight *= timeMultiplier;
    socialWeight *= timeMultiplier;
    exploreWeight *= timeMultiplier;
}

ActivityType RPGActivityScheduler::SelectActivityForCurrentTime(const DailySchedule& schedule) const
{
    TimePeriod currentPeriod = GetCurrentTimePeriod();
    const auto& slots = schedule.GetSlotsForPeriod(currentPeriod);

    if (slots.empty())
        return ActivityType::STANDING_IDLE;

    // Check mandatory slots first
    for (const auto& slot : slots)
    {
        if (slot.mandatory)
            return slot.activity;
    }

    // Weighted random selection
    float totalWeight = 0.0f;
    for (const auto& slot : slots)
        totalWeight += slot.weight;

    if (totalWeight <= 0.0f)
        return ActivityType::STANDING_IDLE;

    // Generate random value
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float roll = dist(rng);

    float accumulated = 0.0f;
    for (const auto& slot : slots)
    {
        accumulated += slot.weight;
        if (roll <= accumulated)
            return slot.activity;
    }

    return slots.back().activity;
}

} // namespace Humanization
} // namespace Playerbot
