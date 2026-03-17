/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * RPG Activity Scheduler: Generates personality-driven daily schedules
 * for autonomous bot behavior. Each bot gets a unique schedule based
 * on its PersonalityProfile.
 */

#pragma once

#include "Define.h"
#include "ActivityType.h"
#include <vector>
#include <string>

namespace Playerbot
{
namespace Humanization
{

class PersonalityProfile;

/// Time slot in a daily schedule
struct ScheduleSlot
{
    ActivityType activity;
    float durationMinutes;     // How long to spend on this activity
    float weight;              // Priority weight (higher = more likely to be chosen)
    bool mandatory;            // Must do this activity (e.g., rest at night)

    ScheduleSlot(ActivityType act, float dur, float w = 1.0f, bool req = false)
        : activity(act), durationMinutes(dur), weight(w), mandatory(req) {}
};

/// Time period of the in-game day
enum class TimePeriod : uint8
{
    EARLY_MORNING = 0,  // 4:00 - 8:00
    MORNING       = 1,  // 8:00 - 12:00
    AFTERNOON     = 2,  // 12:00 - 16:00
    EVENING       = 3,  // 16:00 - 20:00
    NIGHT         = 4,  // 20:00 - 0:00
    LATE_NIGHT    = 5,  // 0:00 - 4:00
};

/// A daily schedule: list of activities per time period
struct DailySchedule
{
    std::vector<ScheduleSlot> earlyMorning;
    std::vector<ScheduleSlot> morning;
    std::vector<ScheduleSlot> afternoon;
    std::vector<ScheduleSlot> evening;
    std::vector<ScheduleSlot> night;
    std::vector<ScheduleSlot> lateNight;

    const std::vector<ScheduleSlot>& GetSlotsForPeriod(TimePeriod period) const;
};

/// Generates personality-driven daily schedules for bots.
class TC_GAME_API RPGActivityScheduler
{
public:
    RPGActivityScheduler() = default;
    ~RPGActivityScheduler() = default;

    /// Generate a daily schedule based on personality.
    DailySchedule GenerateSchedule(const PersonalityProfile& personality) const;

    /// Get the current time period based on in-game time.
    static TimePeriod GetCurrentTimePeriod();

    /// Get a random activity from the schedule for the current time.
    ActivityType SelectActivityForCurrentTime(const DailySchedule& schedule) const;

    /// Get the name of a time period.
    static const char* GetTimePeriodName(TimePeriod period);

private:
    /// Generate schedule slots for a time period based on personality preferences.
    std::vector<ScheduleSlot> GeneratePeriodSlots(
        const PersonalityProfile& personality, TimePeriod period) const;

    /// Get personality-based activity weights for a time period.
    void GetPeriodWeights(const PersonalityProfile& personality,
                          TimePeriod period,
                          float& combatWeight, float& questWeight,
                          float& gatherWeight, float& socialWeight,
                          float& exploreWeight, float& restWeight) const;
};

} // namespace Humanization
} // namespace Playerbot
