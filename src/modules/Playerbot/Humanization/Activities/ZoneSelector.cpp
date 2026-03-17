/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Zone Selector implementation.
 * Selects level-appropriate zones for bot activities.
 */

#include "ZoneSelector.h"
#include "Player.h"
#include "RestMgr.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <random>
#include <algorithm>

namespace Playerbot
{
namespace Humanization
{

ZoneCandidate ZoneSelector::SelectZoneForActivity(Player* bot, ActivityType activity) const
{
    if (!bot || !bot->IsInWorld())
        return {};

    // For city/social activities, prefer safe zones
    ActivityCategory category = GetActivityCategory(activity);
    if (category == ActivityCategory::CITY_LIFE || category == ActivityCategory::SOCIAL ||
        activity == ActivityType::INN_REST || activity == ActivityType::AFK_SHORT)
    {
        auto safeZones = GetSafeZones(bot);
        if (!safeZones.empty())
        {
            // Weighted random selection preferring closer zones
            float totalWeight = 0.0f;
            for (auto& zone : safeZones)
            {
                zone.weight *= CalculateTravelWeight(bot, zone);
                totalWeight += zone.weight;
            }

            if (totalWeight > 0.0f)
            {
                static thread_local std::mt19937 rng(std::random_device{}());
                std::uniform_real_distribution<float> dist(0.0f, totalWeight);
                float roll = dist(rng);

                float accumulated = 0.0f;
                for (const auto& zone : safeZones)
                {
                    accumulated += zone.weight;
                    if (roll <= accumulated)
                        return zone;
                }
                return safeZones.back();
            }
        }
    }

    // For combat/questing/gathering, prefer level-appropriate zones
    if (category == ActivityCategory::COMBAT || category == ActivityCategory::QUESTING ||
        category == ActivityCategory::GATHERING || category == ActivityCategory::FARMING)
    {
        auto zones = GetLevelAppropriateZones(bot);
        if (!zones.empty())
        {
            // Weighted random selection
            float totalWeight = 0.0f;
            for (auto& zone : zones)
            {
                zone.weight *= CalculateTravelWeight(bot, zone);
                totalWeight += zone.weight;
            }

            if (totalWeight > 0.0f)
            {
                static thread_local std::mt19937 rng(std::random_device{}());
                std::uniform_real_distribution<float> dist(0.0f, totalWeight);
                float roll = dist(rng);

                float accumulated = 0.0f;
                for (const auto& zone : zones)
                {
                    accumulated += zone.weight;
                    if (roll <= accumulated)
                        return zone;
                }
                return zones.back();
            }
        }
    }

    // Default: stay in current zone
    ZoneCandidate current;
    current.zoneId = GetCurrentZoneId(bot);
    current.suggestedPosition = bot->GetPosition();
    current.isSafe = !bot->IsInCombat();
    return current;
}

std::vector<ZoneCandidate> ZoneSelector::GetLevelAppropriateZones(Player* bot) const
{
    std::vector<ZoneCandidate> result;
    if (!bot)
        return result;

    uint32 botLevel = bot->GetLevel();

    // Current zone is always a candidate (already here, no travel needed)
    ZoneCandidate currentZone;
    currentZone.zoneId = GetCurrentZoneId(bot);
    currentZone.suggestedPosition = bot->GetPosition();
    currentZone.weight = 2.0f; // Prefer staying in current zone
    currentZone.minLevel = botLevel > 5 ? botLevel - 5 : 1;
    currentZone.maxLevel = botLevel + 3;
    result.push_back(currentZone);

    return result;
}

std::vector<ZoneCandidate> ZoneSelector::GetSafeZones(Player* bot) const
{
    std::vector<ZoneCandidate> result;
    if (!bot)
        return result;

    // If we're already in a city, it's the best safe zone
    if (bot->GetRestMgr().HasRestFlag(REST_FLAG_IN_CITY))
    {
        ZoneCandidate current;
        current.zoneId = GetCurrentZoneId(bot);
        current.suggestedPosition = bot->GetPosition();
        current.isSafe = true;
        current.weight = 3.0f; // Strongly prefer staying in current city
        result.push_back(current);
    }

    // The bot's hearthstone location is always a safe fallback
    ZoneCandidate hearth;
    hearth.zoneId = 0; // Will be resolved by travel system
    hearth.isSafe = true;
    hearth.weight = 1.0f;
    result.push_back(hearth);

    return result;
}

bool ZoneSelector::IsInAppropriateZone(Player* bot, ActivityType activity) const
{
    if (!bot)
        return false;

    ActivityCategory category = GetActivityCategory(activity);

    // City activities require being in a city
    if (category == ActivityCategory::CITY_LIFE || category == ActivityCategory::SOCIAL)
        return bot->GetRestMgr().HasRestFlag(REST_FLAG_IN_CITY);

    // Combat/questing - check if zone is level-appropriate
    if (category == ActivityCategory::COMBAT || category == ActivityCategory::QUESTING)
    {
        // For now, any zone where the bot is not in extreme danger is appropriate
        return true;
    }

    // Default: current zone is fine
    return true;
}

ZoneCandidate ZoneSelector::SelectGatheringZone(Player* bot) const
{
    // Gathering prefers current zone if level-appropriate
    return SelectZoneForActivity(bot, ActivityType::HERBALISM);
}

bool ZoneSelector::IsLevelAppropriate(uint32 botLevel, uint32 minLevel, uint32 maxLevel) const
{
    return botLevel >= minLevel && botLevel <= maxLevel + 3;
}

uint32 ZoneSelector::GetCurrentZoneId(Player* bot) const
{
    if (!bot || !bot->IsInWorld())
        return 0;
    return bot->GetZoneId();
}

float ZoneSelector::CalculateTravelWeight(Player* bot, const ZoneCandidate& zone) const
{
    if (!bot)
        return 1.0f;

    // If in the same zone, max weight
    if (GetCurrentZoneId(bot) == zone.zoneId)
        return 3.0f;

    // Check distance to suggested position (if valid)
    if (zone.suggestedPosition.IsPositionValid())
    {
        float dist = bot->GetDistance(zone.suggestedPosition);
        // Closer zones get higher weight (inverse distance, capped)
        if (dist < 100.0f) return 2.5f;
        if (dist < 500.0f) return 2.0f;
        if (dist < 1000.0f) return 1.5f;
        return 0.5f; // Far away, low weight
    }

    return 1.0f; // Default weight for unknown distance
}

} // namespace Humanization
} // namespace Playerbot
