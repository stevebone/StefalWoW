/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include <array>
#include <atomic>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <shared_mutex>

namespace Playerbot
{

/**
 * @brief Expansion tier for bot level distribution
 *
 * Modern WoW (11.x) uses dynamic level scaling with these tiers:
 * - Starting (1-10): Exile's Reach / racial starting zones
 * - Chromie (10-60): All Chromie Time content (selectable expansion)
 * - Dragonflight (60-70): Dragon Isles content
 * - TheWarWithin (70-80): Khaz Algar content
 */
enum class ExpansionTier : uint8
{
    Starting     = 0,  // Level 1-10 (Exile's Reach, starting zones)
    ChromieTime  = 1,  // Level 10-60 (BfA, Legion, WoD, MoP, Cata, WotLK, TBC, Vanilla)
    Dragonflight = 2,  // Level 60-70 (Dragon Isles)
    TheWarWithin = 3,  // Level 70-80 (Khaz Algar)
    Max          = 4
};

/**
 * @brief Level range for a zone or expansion tier
 */
struct LevelRange
{
    int16 minLevel = 0;
    int16 maxLevel = 0;
    int16 targetMin = 0;  // Preferred spawn level min
    int16 targetMax = 0;  // Preferred spawn level max

    bool IsValid() const { return minLevel > 0 && maxLevel >= minLevel; }
    bool ContainsLevel(int16 level) const { return level >= minLevel && level <= maxLevel; }
    int16 ClampLevel(int16 level) const;

    // For weighted distribution
    int16 GetMidpoint() const { return (minLevel + maxLevel) / 2; }
    int16 GetRange() const { return maxLevel - minLevel + 1; }
};

/**
 * @brief Expansion tier configuration for bot distribution
 */
struct ExpansionTierConfig
{
    ExpansionTier tier;
    LevelRange levels;
    float targetPercentage;     // Desired % of bots in this tier
    std::string name;           // Display name
    int32 expansionId;          // WoW expansion ID (-1 for multi-expansion like Chromie)

    ExpansionTierConfig()
        : tier(ExpansionTier::Starting), targetPercentage(0.0f), expansionId(-1) {}

    ExpansionTierConfig(ExpansionTier t, int16 min, int16 max, float pct,
                        std::string n, int32 expId = -1)
        : tier(t), targetPercentage(pct), name(std::move(n)), expansionId(expId)
    {
        levels.minLevel = min;
        levels.maxLevel = max;
        levels.targetMin = min;
        levels.targetMax = max;
    }
};

/**
 * @brief Zone information with ContentTuning data
 */
struct ZoneInfo
{
    uint32 zoneId;
    uint32 areaId;
    uint32 contentTuningId;
    LevelRange levels;
    std::string zoneName;
    int32 expansionId;
    uint16 continentId;
    bool isStartingZone;
    bool isChromieTimeZone;
    bool isDungeon;
    bool isRaid;

    ZoneInfo() : zoneId(0), areaId(0), contentTuningId(0), expansionId(-1),
                 continentId(0), isStartingZone(false), isChromieTimeZone(false),
                 isDungeon(false), isRaid(false) {}
};

/**
 * @brief Helper class for zone level requirements using ContentTuning DB2
 *
 * This class provides:
 * - Zone-to-level range lookups via TrinityCore's ContentTuning system
 * - Expansion tier definitions for bot level distribution
 * - Caching for performance optimization
 * - Bot-specific helper methods for spawning decisions
 *
 * Thread-safety: All methods are thread-safe for concurrent reads.
 * Cache is populated during Initialize() and refreshed via RefreshCache().
 */
class TC_GAME_API ZoneLevelHelper
{
public:
    static ZoneLevelHelper& Instance()
    {
        static ZoneLevelHelper instance;
        return instance;
    }

    /**
     * @brief Initialize zone level data from ContentTuning DB2
     * @return true if initialization successful
     */
    bool Initialize();

    /**
     * @brief Shutdown and clear cached data
     */
    void Shutdown();

    /**
     * @brief Refresh zone cache from ContentTuning DB2
     */
    void RefreshCache();

    // ========================================================================
    // Zone Level Queries
    // ========================================================================

    /**
     * @brief Get level range for a zone using ContentTuning
     * @param zoneId Zone ID from AreaTable
     * @return Level range or empty optional if zone not found
     */
    std::optional<LevelRange> GetZoneLevelRange(uint32 zoneId) const;

    /**
     * @brief Get level range for an area using ContentTuning
     * @param areaId Area ID from AreaTable
     * @return Level range or empty optional if area not found
     */
    std::optional<LevelRange> GetAreaLevelRange(uint32 areaId) const;

    /**
     * @brief Check if a level is valid for a zone
     * @param zoneId Zone ID
     * @param level Character level
     * @return true if level is within zone's ContentTuning range
     */
    bool IsLevelValidForZone(uint32 zoneId, int16 level) const;

    /**
     * @brief Get zone info with full ContentTuning data
     * @param zoneId Zone ID
     * @return Zone info or empty optional if not found
     */
    std::optional<ZoneInfo> GetZoneInfo(uint32 zoneId) const;

    // ========================================================================
    // Expansion Tier Queries
    // ========================================================================

    /**
     * @brief Get all expansion tier configurations
     * @return Array of tier configs for all expansion tiers
     */
    std::array<ExpansionTierConfig, static_cast<size_t>(ExpansionTier::Max)> const&
        GetExpansionTiers() const { return _expansionTiers; }

    /**
     * @brief Get expansion tier for a level
     * @param level Character level
     * @return Expansion tier containing this level
     */
    ExpansionTier GetTierForLevel(int16 level) const;

    /**
     * @brief Get expansion tier configuration
     * @param tier Expansion tier
     * @return Tier config
     */
    ExpansionTierConfig const& GetTierConfig(ExpansionTier tier) const;

    /**
     * @brief Get level range for an expansion tier
     * @param tier Expansion tier
     * @return Level range for the tier
     */
    LevelRange GetTierLevelRange(ExpansionTier tier) const;

    // ========================================================================
    // Bot Distribution Helpers
    // ========================================================================

    /**
     * @brief Select a random level within an expansion tier
     * @param tier Expansion tier
     * @return Random level within tier's range
     */
    int16 SelectRandomLevelInTier(ExpansionTier tier) const;

    /**
     * @brief Get zones suitable for a level
     * @param level Character level
     * @param maxCount Maximum zones to return
     * @return List of zone IDs suitable for the level
     */
    std::vector<uint32> GetZonesForLevel(int16 level, uint32 maxCount = 10) const;

    /**
     * @brief Check if zone is suitable for bot spawning
     * @param zoneId Zone ID
     * @return true if zone is suitable for bot spawning
     */
    bool IsZoneSuitableForBots(uint32 zoneId) const;

    /**
     * @brief Get recommended spawn level for a zone
     * @param zoneId Zone ID
     * @return Recommended level or 0 if zone not found
     */
    int16 GetRecommendedSpawnLevel(uint32 zoneId) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Get total number of cached zones
     */
    uint32 GetCachedZoneCount() const;

    /**
     * @brief Check if helper is initialized
     */
    bool IsInitialized() const { return _initialized.load(); }

private:
    ZoneLevelHelper() = default;
    ~ZoneLevelHelper() = default;

    ZoneLevelHelper(ZoneLevelHelper const&) = delete;
    ZoneLevelHelper& operator=(ZoneLevelHelper const&) = delete;

    // Internal initialization
    void InitializeExpansionTiers();
    void BuildZoneCache();
    void CategorizeZone(ZoneInfo& info, uint32 contentTuningId);

    // Data storage
    std::array<ExpansionTierConfig, static_cast<size_t>(ExpansionTier::Max)> _expansionTiers;
    std::unordered_map<uint32, ZoneInfo> _zoneCache;           // zoneId -> ZoneInfo
    std::unordered_map<uint32, uint32> _areaToZone;            // areaId -> zoneId
    std::vector<uint32> _zonesbyLevel[81];                     // level -> list of zoneIds

    // Thread safety
    mutable std::shared_mutex _cacheMutex;
    std::atomic<bool> _initialized{false};

    // Constants
    static constexpr int16 MAX_LEVEL = 80;
    static constexpr int16 STARTING_MAX_LEVEL = 10;
    static constexpr int16 CHROMIE_MIN_LEVEL = 10;
    static constexpr int16 CHROMIE_MAX_LEVEL = 60;
    static constexpr int16 DF_MIN_LEVEL = 60;
    static constexpr int16 DF_MAX_LEVEL = 70;
    static constexpr int16 TWW_MIN_LEVEL = 70;
    static constexpr int16 TWW_MAX_LEVEL = 80;
};

#define sZoneLevelHelper ZoneLevelHelper::Instance()

} // namespace Playerbot
