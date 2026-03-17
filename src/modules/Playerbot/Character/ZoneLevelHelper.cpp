/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ZoneLevelHelper.h"
#include "DB2Stores.h"
#include "Log.h"
#include "Random.h"
#include "World.h"
#include "Config/PlayerbotConfig.h"
#include <algorithm>
#include <numeric>  // For std::iota

namespace Playerbot
{

int16 LevelRange::ClampLevel(int16 level) const
{
    return std::clamp(level, minLevel, maxLevel);
}

bool ZoneLevelHelper::Initialize()
{
    if (_initialized.load())
        return true;

    TC_LOG_INFO("module.playerbot.zonelevel", "Initializing ZoneLevelHelper...");

    InitializeExpansionTiers();
    BuildZoneCache();

    _initialized.store(true);

    TC_LOG_INFO("module.playerbot.zonelevel",
        "ZoneLevelHelper initialized: {} zones cached across {} expansion tiers",
        GetCachedZoneCount(), static_cast<uint8>(ExpansionTier::Max));

    return true;
}

void ZoneLevelHelper::Shutdown()
{
    std::unique_lock lock(_cacheMutex);

    _zoneCache.clear();
    _areaToZone.clear();
    for (auto& vec : _zonesbyLevel)
        vec.clear();

    _initialized.store(false);

    TC_LOG_INFO("module.playerbot.zonelevel", "ZoneLevelHelper shutdown complete");
}

void ZoneLevelHelper::RefreshCache()
{
    TC_LOG_INFO("module.playerbot.zonelevel", "Refreshing zone level cache...");

    std::unique_lock lock(_cacheMutex);

    _zoneCache.clear();
    _areaToZone.clear();
    for (auto& vec : _zonesbyLevel)
        vec.clear();

    lock.unlock();

    BuildZoneCache();

    TC_LOG_INFO("module.playerbot.zonelevel",
        "Zone level cache refreshed: {} zones", GetCachedZoneCount());
}

void ZoneLevelHelper::InitializeExpansionTiers()
{
    // Load target percentages from config with sensible defaults
    // These determine what % of bots should be at each tier
    float startingPct = sPlayerbotConfig->GetFloat(
        "Playerbot.Population.Tier.Starting.Pct", 5.0f);
    float chromiePct = sPlayerbotConfig->GetFloat(
        "Playerbot.Population.Tier.ChromieTime.Pct", 40.0f);
    float dfPct = sPlayerbotConfig->GetFloat(
        "Playerbot.Population.Tier.Dragonflight.Pct", 25.0f);
    float twwPct = sPlayerbotConfig->GetFloat(
        "Playerbot.Population.Tier.TheWarWithin.Pct", 30.0f);

    // Normalize percentages to 100%
    float total = startingPct + chromiePct + dfPct + twwPct;
    if (total > 0.0f)
    {
        startingPct = (startingPct / total) * 100.0f;
        chromiePct = (chromiePct / total) * 100.0f;
        dfPct = (dfPct / total) * 100.0f;
        twwPct = (twwPct / total) * 100.0f;
    }

    _expansionTiers[static_cast<size_t>(ExpansionTier::Starting)] =
        ExpansionTierConfig(ExpansionTier::Starting,
                           1, STARTING_MAX_LEVEL,
                           startingPct, "Starting Zones", -1);

    _expansionTiers[static_cast<size_t>(ExpansionTier::ChromieTime)] =
        ExpansionTierConfig(ExpansionTier::ChromieTime,
                           CHROMIE_MIN_LEVEL, CHROMIE_MAX_LEVEL,
                           chromiePct, "Chromie Time", -1);

    _expansionTiers[static_cast<size_t>(ExpansionTier::Dragonflight)] =
        ExpansionTierConfig(ExpansionTier::Dragonflight,
                           DF_MIN_LEVEL, DF_MAX_LEVEL,
                           dfPct, "Dragonflight", 9);

    _expansionTiers[static_cast<size_t>(ExpansionTier::TheWarWithin)] =
        ExpansionTierConfig(ExpansionTier::TheWarWithin,
                           TWW_MIN_LEVEL, TWW_MAX_LEVEL,
                           twwPct, "The War Within", 10);

    TC_LOG_DEBUG("module.playerbot.zonelevel",
        "Expansion tiers initialized: Starting={:.1f}%, Chromie={:.1f}%, DF={:.1f}%, TWW={:.1f}%",
        startingPct, chromiePct, dfPct, twwPct);
}

void ZoneLevelHelper::BuildZoneCache()
{
    std::unique_lock lock(_cacheMutex);

    uint32 zonesProcessed = 0;
    uint32 zonesWithContentTuning = 0;

    // Empty redirect flags for ContentTuning queries
    std::span<uint32 const> emptyRedirectFlags{};

    // Iterate all areas in AreaTable
    for (AreaTableEntry const* areaEntry : sAreaTableStore)
    {
        if (!areaEntry)
            continue;

        // Get ContentTuning for this area
        ContentTuningEntry const* contentTuning =
            sDB2Manager.GetContentTuningForArea(areaEntry);

        if (!contentTuning)
            continue;

        // Get calculated level data
        Optional<ContentTuningLevels> levels =
            sDB2Manager.GetContentTuningData(contentTuning->ID, emptyRedirectFlags);

        if (!levels)
            continue;

        ++zonesWithContentTuning;

        // Build zone info
        ZoneInfo info;
        info.areaId = areaEntry->ID;
        info.contentTuningId = contentTuning->ID;
        info.expansionId = contentTuning->ExpansionID;
        info.continentId = areaEntry->ContinentID;

        // Determine zone ID (parent if this is a subzone)
        info.zoneId = areaEntry->ParentAreaID > 0 ?
                      areaEntry->ParentAreaID : areaEntry->ID;

        // Set level range from ContentTuning
        info.levels.minLevel = static_cast<int16>(levels->MinLevel);
        info.levels.maxLevel = static_cast<int16>(levels->MaxLevel);
        info.levels.targetMin = static_cast<int16>(levels->TargetLevelMin);
        info.levels.targetMax = static_cast<int16>(levels->TargetLevelMax);

        // Store zone name (AreaName is a LocalizedString with Str[] containing const char* in 12.0.7)
        if (areaEntry->AreaName.Str[DEFAULT_LOCALE] && areaEntry->AreaName.Str[DEFAULT_LOCALE][0] != '\0')
            info.zoneName = areaEntry->AreaName.Str[DEFAULT_LOCALE];

        // Categorize zone type
        CategorizeZone(info, contentTuning->ID);

        // Only store zones (not subzones) in main cache
        // Subzones map to their parent zone
        if (areaEntry->ParentAreaID == 0)
        {
            _zoneCache[areaEntry->ID] = info;

            // Add to level->zone lookup
            for (int16 lvl = info.levels.minLevel;
                 lvl <= info.levels.maxLevel && lvl <= MAX_LEVEL; ++lvl)
            {
                if (lvl > 0)
                    _zonesbyLevel[lvl].push_back(areaEntry->ID);
            }
        }
        else
        {
            // Map subzone area to parent zone
            _areaToZone[areaEntry->ID] = info.zoneId;
        }

        ++zonesProcessed;
    }

    // De-duplicate zone lists per level
    for (auto& vec : _zonesbyLevel)
    {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    }

    TC_LOG_DEBUG("module.playerbot.zonelevel",
        "Built zone cache: {} areas processed, {} with ContentTuning, {} zones cached",
        zonesProcessed, zonesWithContentTuning, _zoneCache.size());
}

void ZoneLevelHelper::CategorizeZone(ZoneInfo& info, uint32 contentTuningId)
{
    // Determine zone type based on ContentTuning and other factors
    // This is a simplified categorization - can be expanded

    // Starting zones are level 1-10
    info.isStartingZone = (info.levels.minLevel == 1 &&
                           info.levels.maxLevel <= STARTING_MAX_LEVEL);

    // Chromie Time zones are level 10-60 (most old content)
    info.isChromieTimeZone = (info.levels.minLevel >= CHROMIE_MIN_LEVEL &&
                              info.levels.maxLevel <= CHROMIE_MAX_LEVEL &&
                              info.expansionId < 9); // Before Dragonflight

    // Check for dungeon/raid labels in ContentTuning
    // Label 57 = Dungeon, Label 58 = Raid (approximate)
    info.isDungeon = sDB2Manager.HasContentTuningLabel(contentTuningId, 57);
    info.isRaid = sDB2Manager.HasContentTuningLabel(contentTuningId, 58);
}

std::optional<LevelRange> ZoneLevelHelper::GetZoneLevelRange(uint32 zoneId) const
{
    std::shared_lock lock(_cacheMutex);

    auto it = _zoneCache.find(zoneId);
    if (it != _zoneCache.end())
        return it->second.levels;

    return std::nullopt;
}

std::optional<LevelRange> ZoneLevelHelper::GetAreaLevelRange(uint32 areaId) const
{
    std::shared_lock lock(_cacheMutex);

    // First check if this is a zone itself
    auto zoneIt = _zoneCache.find(areaId);
    if (zoneIt != _zoneCache.end())
        return zoneIt->second.levels;

    // Check if this is a subzone mapping to a parent zone
    auto areaIt = _areaToZone.find(areaId);
    if (areaIt != _areaToZone.end())
    {
        auto parentIt = _zoneCache.find(areaIt->second);
        if (parentIt != _zoneCache.end())
            return parentIt->second.levels;
    }

    return std::nullopt;
}

bool ZoneLevelHelper::IsLevelValidForZone(uint32 zoneId, int16 level) const
{
    auto range = GetZoneLevelRange(zoneId);
    if (!range)
        return false;

    return range->ContainsLevel(level);
}

std::optional<ZoneInfo> ZoneLevelHelper::GetZoneInfo(uint32 zoneId) const
{
    std::shared_lock lock(_cacheMutex);

    auto it = _zoneCache.find(zoneId);
    if (it != _zoneCache.end())
        return it->second;

    return std::nullopt;
}

ExpansionTier ZoneLevelHelper::GetTierForLevel(int16 level) const
{
    if (level <= STARTING_MAX_LEVEL)
        return ExpansionTier::Starting;
    else if (level <= CHROMIE_MAX_LEVEL)
        return ExpansionTier::ChromieTime;
    else if (level <= DF_MAX_LEVEL)
        return ExpansionTier::Dragonflight;
    else
        return ExpansionTier::TheWarWithin;
}

ExpansionTierConfig const& ZoneLevelHelper::GetTierConfig(ExpansionTier tier) const
{
    return _expansionTiers[static_cast<size_t>(tier)];
}

LevelRange ZoneLevelHelper::GetTierLevelRange(ExpansionTier tier) const
{
    return _expansionTiers[static_cast<size_t>(tier)].levels;
}

int16 ZoneLevelHelper::SelectRandomLevelInTier(ExpansionTier tier) const
{
    ExpansionTierConfig const& config = GetTierConfig(tier);
    return static_cast<int16>(urand(config.levels.minLevel, config.levels.maxLevel));
}

std::vector<uint32> ZoneLevelHelper::GetZonesForLevel(int16 level, uint32 maxCount) const
{
    std::shared_lock lock(_cacheMutex);

    if (level < 1 || level > MAX_LEVEL)
        return {};

    std::vector<uint32> const& zones = _zonesbyLevel[level];

    if (zones.size() <= maxCount)
        return zones;

    // Return random subset
    std::vector<uint32> result;
    result.reserve(maxCount);

    std::vector<uint32> indices(zones.size());
    std::iota(indices.begin(), indices.end(), 0);

    // Fisher-Yates shuffle for first maxCount elements
    size_t loopMax = std::min(static_cast<size_t>(maxCount), indices.size());
    for (size_t i = 0; i < loopMax; ++i)
    {
        uint32 j = urand(static_cast<uint32>(i), static_cast<uint32>(indices.size() - 1));
        std::swap(indices[i], indices[j]);
        result.push_back(zones[indices[i]]);
    }

    return result;
}

bool ZoneLevelHelper::IsZoneSuitableForBots(uint32 zoneId) const
{
    std::shared_lock lock(_cacheMutex);

    auto it = _zoneCache.find(zoneId);
    if (it == _zoneCache.end())
        return false;

    ZoneInfo const& info = it->second;

    // Exclude dungeons and raids from open-world bot spawning
    if (info.isDungeon || info.isRaid)
        return false;

    // Zone must have valid level range
    if (!info.levels.IsValid())
        return false;

    return true;
}

int16 ZoneLevelHelper::GetRecommendedSpawnLevel(uint32 zoneId) const
{
    std::shared_lock lock(_cacheMutex);

    auto it = _zoneCache.find(zoneId);
    if (it == _zoneCache.end())
        return 0;

    ZoneInfo const& info = it->second;

    // Use target levels if available, otherwise midpoint of range
    if (info.levels.targetMin > 0 && info.levels.targetMax > 0)
    {
        return static_cast<int16>(
            urand(info.levels.targetMin, info.levels.targetMax));
    }

    return info.levels.GetMidpoint();
}

uint32 ZoneLevelHelper::GetCachedZoneCount() const
{
    std::shared_lock lock(_cacheMutex);
    return static_cast<uint32>(_zoneCache.size());
}

} // namespace Playerbot
