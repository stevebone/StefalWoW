/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Zone Selector: Chooses appropriate zones for bot activities
 * based on bot level, current activity, and personality.
 */

#pragma once

#include "Define.h"
#include "ActivityType.h"
#include "Position.h"
#include <vector>
#include <string>

class Player;

namespace Playerbot
{
namespace Humanization
{

/// Information about a zone suitable for a particular activity
struct ZoneCandidate
{
    uint32 zoneId;
    uint32 areaId;
    std::string zoneName;
    uint32 minLevel;
    uint32 maxLevel;
    float weight;                   // Selection weight (higher = more likely)
    Position suggestedPosition;     // A good starting position in this zone
    bool isSafe;                    // No hostile mobs in immediate area
    bool hasFlightPath;             // Connected via flight path

    ZoneCandidate() : zoneId(0), areaId(0), minLevel(0), maxLevel(0),
                      weight(1.0f), isSafe(false), hasFlightPath(false) {}
};

/// Selects appropriate zones for bot activities.
class TC_GAME_API ZoneSelector
{
public:
    ZoneSelector() = default;
    ~ZoneSelector() = default;

    /// Select a zone appropriate for the given activity and bot level.
    ZoneCandidate SelectZoneForActivity(Player* bot, ActivityType activity) const;

    /// Get a list of zones appropriate for grinding/questing at the bot's level.
    std::vector<ZoneCandidate> GetLevelAppropriateZones(Player* bot) const;

    /// Get safe zones for resting (cities, inns, etc.)
    std::vector<ZoneCandidate> GetSafeZones(Player* bot) const;

    /// Check if the bot is already in an appropriate zone for the activity.
    bool IsInAppropriateZone(Player* bot, ActivityType activity) const;

    /// Get a nearby gathering route zone for the bot.
    ZoneCandidate SelectGatheringZone(Player* bot) const;

private:
    /// Check if a zone level range is appropriate for the bot.
    bool IsLevelAppropriate(uint32 botLevel, uint32 minLevel, uint32 maxLevel) const;

    /// Get the bot's current zone info.
    uint32 GetCurrentZoneId(Player* bot) const;

    /// Calculate travel weight (closer zones weighted higher).
    float CalculateTravelWeight(Player* bot, const ZoneCandidate& zone) const;
};

} // namespace Humanization
} // namespace Playerbot
