/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file PoolConfiguration.h
 * @brief Configuration structures for the Instance Bot Pool system
 *
 * This file defines all configurable parameters for:
 * - Pool sizes (per faction, per role, per bracket)
 * - Level bracket definitions and requirements
 * - Gear score requirements by content type
 * - Timing parameters (cooldowns, timeouts, warmup)
 * - JIT factory settings
 * - Orchestrator behavior
 *
 * All values can be overridden via playerbots.conf configuration file.
 *
 * Design Philosophy:
 * - Per-bracket pools for guaranteed content coverage
 * - Sensible defaults: 50 bots per faction per bracket = 800 total
 * - Full support for parallel BG + dungeon scenarios
 * - Performance-oriented settings for high-concurrency scenarios
 *
 * Per-Bracket Pool Architecture:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    PER-BRACKET POOL SYSTEM                              │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │                                                                         │
 * │  Bracket 0 (10-19):  Alliance[50] + Horde[50] = 100 bots               │
 * │  Bracket 1 (20-29):  Alliance[50] + Horde[50] = 100 bots               │
 * │  Bracket 2 (30-39):  Alliance[50] + Horde[50] = 100 bots               │
 * │  Bracket 3 (40-49):  Alliance[50] + Horde[50] = 100 bots               │
 * │  Bracket 4 (50-59):  Alliance[50] + Horde[50] = 100 bots               │
 * │  Bracket 5 (60-69):  Alliance[50] + Horde[50] = 100 bots               │
 * │  Bracket 6 (70-79):  Alliance[50] + Horde[50] = 100 bots               │
 * │  Bracket 7 (80+):    Alliance[50] + Horde[50] = 100 bots               │
 * │  ─────────────────────────────────────────────────────────────────────  │
 * │  TOTAL: 800 warm bots (configurable per bracket)                       │
 * │                                                                         │
 * │  Per Faction Per Bracket:                                               │
 * │  ├── Tanks:   10 (20%)                                                 │
 * │  ├── Healers: 15 (30%)                                                 │
 * │  └── DPS:     25 (50%)                                                 │
 * │                                                                         │
 * │  Supports: 2 dungeon groups + 40v40 AV per bracket simultaneously      │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 */

#pragma once

#include "Define.h"
#include "PoolSlotState.h"
#include <array>
#include <chrono>
#include <string>
#include <cstdint>

namespace Playerbot
{

// ============================================================================
// LEVEL BRACKET ENUMERATION
// ============================================================================

/**
 * @brief Level brackets matching WoW BG/content brackets
 *
 * 8 brackets covering levels 10-80+ with 10-level ranges.
 * These match standard WoW battleground brackets for proper matchmaking.
 */
enum class PoolBracket : uint8
{
    Bracket_10_19 = 0,   ///< Levels 10-19 (starting content)
    Bracket_20_29 = 1,   ///< Levels 20-29
    Bracket_30_39 = 2,   ///< Levels 30-39
    Bracket_40_49 = 3,   ///< Levels 40-49
    Bracket_50_59 = 4,   ///< Levels 50-59
    Bracket_60_69 = 5,   ///< Levels 60-69 (Dragonflight content)
    Bracket_70_79 = 6,   ///< Levels 70-79 (The War Within content)
    Bracket_80_Max = 7,  ///< Levels 80+ (max level content)

    Max = 8              ///< Number of brackets
};

/// Number of level brackets
static constexpr uint8 NUM_LEVEL_BRACKETS = static_cast<uint8>(PoolBracket::Max);

/**
 * @brief Convert level bracket enum to string
 */
inline char const* PoolBracketToString(PoolBracket bracket)
{
    switch (bracket)
    {
        case PoolBracket::Bracket_10_19: return "10-19";
        case PoolBracket::Bracket_20_29: return "20-29";
        case PoolBracket::Bracket_30_39: return "30-39";
        case PoolBracket::Bracket_40_49: return "40-49";
        case PoolBracket::Bracket_50_59: return "50-59";
        case PoolBracket::Bracket_60_69: return "60-69";
        case PoolBracket::Bracket_70_79: return "70-79";
        case PoolBracket::Bracket_80_Max: return "80+";
        default: return "Unknown";
    }
}

/**
 * @brief Get level bracket for a given character level
 * @param level Character level (1-80+)
 * @return Appropriate bracket enum value
 */
inline PoolBracket GetBracketForLevel(uint32 level)
{
    if (level < 10) return PoolBracket::Bracket_10_19;  // Sub-10 uses first bracket
    if (level < 20) return PoolBracket::Bracket_10_19;
    if (level < 30) return PoolBracket::Bracket_20_29;
    if (level < 40) return PoolBracket::Bracket_30_39;
    if (level < 50) return PoolBracket::Bracket_40_49;
    if (level < 60) return PoolBracket::Bracket_50_59;
    if (level < 70) return PoolBracket::Bracket_60_69;
    if (level < 80) return PoolBracket::Bracket_70_79;
    return PoolBracket::Bracket_80_Max;
}

/**
 * @brief Get level range for a bracket
 * @param bracket Level bracket enum
 * @param[out] minLevel Minimum level for bracket
 * @param[out] maxLevel Maximum level for bracket
 */
inline void GetBracketLevelRange(PoolBracket bracket, uint32& minLevel, uint32& maxLevel)
{
    switch (bracket)
    {
        case PoolBracket::Bracket_10_19: minLevel = 10; maxLevel = 19; break;
        case PoolBracket::Bracket_20_29: minLevel = 20; maxLevel = 29; break;
        case PoolBracket::Bracket_30_39: minLevel = 30; maxLevel = 39; break;
        case PoolBracket::Bracket_40_49: minLevel = 40; maxLevel = 49; break;
        case PoolBracket::Bracket_50_59: minLevel = 50; maxLevel = 59; break;
        case PoolBracket::Bracket_60_69: minLevel = 60; maxLevel = 69; break;
        case PoolBracket::Bracket_70_79: minLevel = 70; maxLevel = 79; break;
        case PoolBracket::Bracket_80_Max: minLevel = 80; maxLevel = 80; break;
        default: minLevel = 80; maxLevel = 80; break;
    }
}

/**
 * @brief Get midpoint level for a bracket (used for bot creation)
 * @param bracket Level bracket enum
 * @return Midpoint level for the bracket
 */
inline uint32 GetBracketMidpointLevel(PoolBracket bracket)
{
    uint32 minLevel, maxLevel;
    GetBracketLevelRange(bracket, minLevel, maxLevel);
    return (minLevel + maxLevel) / 2;
}

// ============================================================================
// PER-BRACKET ROLE DISTRIBUTION
// ============================================================================

/**
 * @brief Role distribution within a single bracket for one faction
 *
 * Defines how many tanks, healers, and DPS bots to create per faction
 * for a single level bracket. Default: 10 tanks, 15 healers, 25 DPS = 50 total
 *
 * This supports:
 * - 2 full dungeon groups (2 tanks, 2 healers, 6 DPS)
 * - 1 full Alterac Valley team (40 players with varied roles)
 * - Buffer for parallel content
 */
struct BracketRoleDistribution
{
    uint32 tanks = 10;      ///< Tank bots per faction per bracket (20%)
    uint32 healers = 15;    ///< Healer bots per faction per bracket (30%)
    uint32 dps = 25;        ///< DPS bots per faction per bracket (50%)

    /**
     * @brief Get total bots for this bracket/faction
     */
    uint32 GetTotal() const { return tanks + healers + dps; }

    /**
     * @brief Get role count by BotRole enum
     */
    uint32 GetRoleCount(BotRole role) const
    {
        switch (role)
        {
            case BotRole::Tank:   return tanks;
            case BotRole::Healer: return healers;
            case BotRole::DPS:    return dps;
            default:              return 0;
        }
    }

    /**
     * @brief Set role count by BotRole enum
     */
    void SetRoleCount(BotRole role, uint32 count)
    {
        switch (role)
        {
            case BotRole::Tank:   tanks = count; break;
            case BotRole::Healer: healers = count; break;
            case BotRole::DPS:    dps = count; break;
            default: break;
        }
    }

    /**
     * @brief Apply a multiplier to all roles
     */
    void ApplyMultiplier(float multiplier)
    {
        tanks = static_cast<uint32>(tanks * multiplier);
        healers = static_cast<uint32>(healers * multiplier);
        dps = static_cast<uint32>(dps * multiplier);
    }

    /**
     * @brief Validate role distribution (at least 1 of each)
     */
    bool IsValid() const
    {
        return tanks >= 1 && healers >= 1 && dps >= 1 && GetTotal() <= 200;
    }
};

// ============================================================================
// PER-BRACKET POOL CONFIGURATION
// ============================================================================

/**
 * @brief Configuration for a single level bracket's bot pool
 *
 * Each bracket has independent Alliance and Horde pools with
 * configurable role distribution. This enables full content
 * coverage at any level without relying on JIT.
 */
struct BracketPoolConfig
{
    PoolBracket bracket = PoolBracket::Bracket_80_Max;  ///< Which bracket this configures
    bool enabled = true;                                   ///< Whether this bracket pool is active

    BracketRoleDistribution alliance;  ///< Alliance role distribution
    BracketRoleDistribution horde;     ///< Horde role distribution

    /**
     * @brief Get total bots for this bracket (both factions)
     */
    uint32 GetTotal() const
    {
        return enabled ? (alliance.GetTotal() + horde.GetTotal()) : 0;
    }

    /**
     * @brief Get total bots for this bracket (alias for GetTotal)
     */
    uint32 GetTotalBots() const
    {
        return GetTotal();
    }

    /**
     * @brief Get total for one faction
     */
    uint32 GetFactionTotal(Faction faction) const
    {
        if (!enabled) return 0;
        return (faction == Faction::Alliance) ? alliance.GetTotal() : horde.GetTotal();
    }

    /**
     * @brief Get role count for faction
     */
    uint32 GetRoleCount(Faction faction, BotRole role) const
    {
        if (!enabled) return 0;
        return (faction == Faction::Alliance)
            ? alliance.GetRoleCount(role)
            : horde.GetRoleCount(role);
    }

    /**
     * @brief Get role distribution for faction
     */
    BracketRoleDistribution& GetDistribution(Faction faction)
    {
        return (faction == Faction::Alliance) ? alliance : horde;
    }

    BracketRoleDistribution const& GetDistribution(Faction faction) const
    {
        return (faction == Faction::Alliance) ? alliance : horde;
    }

    /**
     * @brief Apply uniform scaling to this bracket
     */
    void ApplyScaling(float scale)
    {
        alliance.ApplyMultiplier(scale);
        horde.ApplyMultiplier(scale);
    }

    /**
     * @brief Validate bracket configuration
     */
    bool IsValid() const
    {
        return alliance.IsValid() && horde.IsValid();
    }
};

// ============================================================================
// POOL SIZE CONFIGURATION (LEGACY + NEW)
// ============================================================================

/**
 * @brief Configuration for pool sizes per faction, role, AND bracket
 *
 * NEW ARCHITECTURE: Per-bracket pools with explicit sizing.
 *
 * Default configuration (50 bots per faction per bracket):
 * - 8 brackets × 2 factions × 50 bots = 800 total warm bots
 * - Per faction per bracket: 10 tanks, 15 healers, 25 DPS
 *
 * This ensures:
 * - Instant 5-man dungeon fill at any level
 * - Full 40v40 BG support at any bracket
 * - Parallel content (dungeon + BG simultaneously)
 *
 * Legacy fields maintained for backward compatibility with existing configs.
 */
struct PoolSizeConfig
{
    // ========================================================================
    // PER-BRACKET POOL CONFIGURATION (NEW - PRIMARY)
    // ========================================================================

    /// Enable per-bracket pooling (if false, uses legacy flat distribution)
    bool usePerBracketPools = true;

    /// Per-bracket pool configurations (8 brackets)
    std::array<BracketPoolConfig, NUM_LEVEL_BRACKETS> bracketPools;

    /// Default bots per faction per bracket (used for initialization)
    uint32 defaultBotsPerFactionPerBracket = 50;

    // ========================================================================
    // LEGACY PER-FACTION POOL SIZES (BACKWARD COMPATIBILITY)
    // ========================================================================

    /// Alliance warm pool distribution (legacy - used if usePerBracketPools=false)
    uint32 allianceTanks = 20;          ///< Alliance tank bots
    uint32 allianceHealers = 30;        ///< Alliance healer bots
    uint32 allianceDPS = 50;            ///< Alliance DPS bots

    /// Horde warm pool distribution (legacy - used if usePerBracketPools=false)
    uint32 hordeTanks = 20;             ///< Horde tank bots
    uint32 hordeHealers = 30;           ///< Horde healer bots
    uint32 hordeDPS = 50;               ///< Horde DPS bots

    // ========================================================================
    // OVERFLOW / JIT LIMITS
    // ========================================================================

    /// Maximum bots that JIT factory can create on-demand
    uint32 maxOverflowBots = 500;

    /// Maximum bots created per second during overflow
    uint32 overflowCreationRate = 10;

    /// Maximum concurrent JIT creation operations
    uint32 maxConcurrentCreations = 10;

    // ========================================================================
    // CONSTRUCTOR
    // ========================================================================

    PoolSizeConfig()
    {
        InitializeDefaultBracketPools();
    }

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    /**
     * @brief Initialize all bracket pools with default values
     *
     * Default: 50 bots per faction per bracket
     * - 10 tanks (20%)
     * - 15 healers (30%)
     * - 25 DPS (50%)
     */
    void InitializeDefaultBracketPools()
    {
        for (uint8 i = 0; i < NUM_LEVEL_BRACKETS; ++i)
        {
            auto bracket = static_cast<PoolBracket>(i);
            bracketPools[i].bracket = bracket;
            bracketPools[i].enabled = true;

            // Default role distribution: 10 tanks, 15 healers, 25 DPS per faction
            bracketPools[i].alliance.tanks = 10;
            bracketPools[i].alliance.healers = 15;
            bracketPools[i].alliance.dps = 25;

            bracketPools[i].horde.tanks = 10;
            bracketPools[i].horde.healers = 15;
            bracketPools[i].horde.dps = 25;
        }
    }

    /**
     * @brief Apply a uniform multiplier to all brackets
     * @param multiplier Scale factor (e.g., 0.5 = half size, 2.0 = double)
     */
    void ApplyGlobalScaling(float multiplier)
    {
        for (auto& bracket : bracketPools)
        {
            bracket.ApplyScaling(multiplier);
        }
    }

    /**
     * @brief Set bots per faction per bracket uniformly
     * @param botsPerFaction Total bots per faction per bracket
     */
    void SetUniformBotsPerBracket(uint32 botsPerFaction)
    {
        // Role distribution: 20% tank, 30% healer, 50% DPS
        uint32 tanks = std::max(1u, botsPerFaction * 20 / 100);
        uint32 healers = std::max(1u, botsPerFaction * 30 / 100);
        uint32 dps = botsPerFaction - tanks - healers;

        for (auto& bracket : bracketPools)
        {
            bracket.alliance.tanks = tanks;
            bracket.alliance.healers = healers;
            bracket.alliance.dps = dps;

            bracket.horde.tanks = tanks;
            bracket.horde.healers = healers;
            bracket.horde.dps = dps;
        }
    }

    // ========================================================================
    // PER-BRACKET ACCESSORS
    // ========================================================================

    /**
     * @brief Get bracket pool configuration
     */
    BracketPoolConfig& GetBracketPool(PoolBracket bracket)
    {
        return bracketPools[static_cast<uint8>(bracket)];
    }

    BracketPoolConfig const& GetBracketPool(PoolBracket bracket) const
    {
        return bracketPools[static_cast<uint8>(bracket)];
    }

    /**
     * @brief Get bracket pool for a given level
     */
    BracketPoolConfig& GetBracketPoolForLevel(uint32 level)
    {
        return GetBracketPool(GetBracketForLevel(level));
    }

    BracketPoolConfig const& GetBracketPoolForLevel(uint32 level) const
    {
        return GetBracketPool(GetBracketForLevel(level));
    }

    /**
     * @brief Get role count for bracket/faction/role
     */
    uint32 GetBracketRoleCount(PoolBracket bracket, Faction faction, BotRole role) const
    {
        if (!usePerBracketPools)
            return GetRoleCount(faction, role) / NUM_LEVEL_BRACKETS;

        return GetBracketPool(bracket).GetRoleCount(faction, role);
    }

    /**
     * @brief Get total bots for a specific bracket
     */
    uint32 GetBracketTotal(PoolBracket bracket) const
    {
        return GetBracketPool(bracket).GetTotal();
    }

    /**
     * @brief Get total bots for bracket/faction
     */
    uint32 GetBracketFactionTotal(PoolBracket bracket, Faction faction) const
    {
        return GetBracketPool(bracket).GetFactionTotal(faction);
    }

    // ========================================================================
    // AGGREGATE ACCESSORS
    // ========================================================================

    /**
     * @brief Get total Alliance pool size (all brackets)
     */
    uint32 GetAllianceTotal() const
    {
        if (!usePerBracketPools)
            return allianceTanks + allianceHealers + allianceDPS;

        uint32 total = 0;
        for (auto const& bracket : bracketPools)
            total += bracket.GetFactionTotal(Faction::Alliance);
        return total;
    }

    /**
     * @brief Get total Horde pool size (all brackets)
     */
    uint32 GetHordeTotal() const
    {
        if (!usePerBracketPools)
            return hordeTanks + hordeHealers + hordeDPS;

        uint32 total = 0;
        for (auto const& bracket : bracketPools)
            total += bracket.GetFactionTotal(Faction::Horde);
        return total;
    }

    /**
     * @brief Get total warm pool size (both factions, all brackets)
     */
    uint32 GetTotalWarmPool() const
    {
        return GetAllianceTotal() + GetHordeTotal();
    }

    /**
     * @brief Get total bots across all brackets (alias for GetTotalWarmPool)
     */
    uint32 GetTotalBotsAcrossAllBrackets() const
    {
        return GetTotalWarmPool();
    }

    /**
     * @brief Get total tank count (both factions, all brackets)
     */
    uint32 GetTotalTanks() const
    {
        if (!usePerBracketPools)
            return allianceTanks + hordeTanks;

        uint32 total = 0;
        for (auto const& bracket : bracketPools)
        {
            total += bracket.alliance.tanks + bracket.horde.tanks;
        }
        return total;
    }

    /**
     * @brief Get total healer count (both factions, all brackets)
     */
    uint32 GetTotalHealers() const
    {
        if (!usePerBracketPools)
            return allianceHealers + hordeHealers;

        uint32 total = 0;
        for (auto const& bracket : bracketPools)
        {
            total += bracket.alliance.healers + bracket.horde.healers;
        }
        return total;
    }

    /**
     * @brief Get total DPS count (both factions, all brackets)
     */
    uint32 GetTotalDPS() const
    {
        if (!usePerBracketPools)
            return allianceDPS + hordeDPS;

        uint32 total = 0;
        for (auto const& bracket : bracketPools)
        {
            total += bracket.alliance.dps + bracket.horde.dps;
        }
        return total;
    }

    /**
     * @brief Get role count for specific faction (legacy - aggregates all brackets)
     */
    uint32 GetRoleCount(Faction faction, BotRole role) const
    {
        if (!usePerBracketPools)
        {
            if (faction == Faction::Alliance)
            {
                switch (role)
                {
                    case BotRole::Tank:   return allianceTanks;
                    case BotRole::Healer: return allianceHealers;
                    case BotRole::DPS:    return allianceDPS;
                    default:              return 0;
                }
            }
            else
            {
                switch (role)
                {
                    case BotRole::Tank:   return hordeTanks;
                    case BotRole::Healer: return hordeHealers;
                    case BotRole::DPS:    return hordeDPS;
                    default:              return 0;
                }
            }
        }

        // Aggregate across all brackets
        uint32 total = 0;
        for (auto const& bracket : bracketPools)
        {
            total += bracket.GetRoleCount(faction, role);
        }
        return total;
    }

    // ========================================================================
    // VALIDATION
    // ========================================================================

    /**
     * @brief Validate pool size configuration
     */
    bool Validate() const
    {
        if (usePerBracketPools)
        {
            for (auto const& bracket : bracketPools)
            {
                if (bracket.enabled && !bracket.IsValid())
                    return false;
            }
        }
        return GetTotalWarmPool() <= 5000;  // Sanity limit
    }

    /**
     * @brief Get number of enabled brackets
     */
    uint32 GetEnabledBracketCount() const
    {
        uint32 count = 0;
        for (auto const& bracket : bracketPools)
        {
            if (bracket.enabled)
                ++count;
        }
        return count;
    }
};

// ============================================================================
// LEVEL CONFIGURATION
// ============================================================================

/**
 * @brief Configuration for level bracket requirements and gear scores
 *
 * This struct now uses the PoolBracket enum and GetBracketForLevel() function
 * for bracket determination. The legacy 4-bracket system is maintained for
 * backward compatibility with existing expansion tier references.
 *
 * NEW: Per-bracket pools are configured in PoolSizeConfig.bracketPools[]
 */
struct PoolLevelConfig
{
    // ========================================================================
    // LEGACY LEVEL BRACKET RANGES (BACKWARD COMPATIBILITY)
    // ========================================================================

    /// Level ranges for legacy 4-bracket system (expansion tiers)
    static constexpr uint32 STARTING_MIN = 1;
    static constexpr uint32 STARTING_MAX = 10;
    static constexpr uint32 CHROMIE_MIN = 10;
    static constexpr uint32 CHROMIE_MAX = 60;
    static constexpr uint32 DF_MIN = 60;
    static constexpr uint32 DF_MAX = 70;
    static constexpr uint32 TWW_MIN = 70;
    static constexpr uint32 TWW_MAX = 80;

    // ========================================================================
    // GEAR SCORE REQUIREMENTS BY CONTENT
    // ========================================================================

    /// Minimum gear score for normal dungeons
    uint32 normalDungeonMinGS = 350;

    /// Minimum gear score for heroic dungeons
    uint32 heroicDungeonMinGS = 400;

    /// Minimum gear score for Mythic 0 dungeons
    uint32 mythic0DungeonMinGS = 450;

    /// Minimum gear score for Mythic+ dungeons
    uint32 mythicPlusDungeonMinGS = 480;

    /// Minimum gear score for normal raids
    uint32 normalRaidMinGS = 480;

    /// Minimum gear score for heroic raids
    uint32 heroicRaidMinGS = 510;

    /// Minimum gear score for mythic raids
    uint32 mythicRaidMinGS = 540;

    /// Minimum gear score for rated PvP
    uint32 ratedPvPMinGS = 500;

    // ========================================================================
    // GEAR SCORE BY BRACKET (NEW)
    // ========================================================================

    /**
     * @brief Get expected gear score for a level bracket
     * @param bracket Level bracket
     * @return Expected gear score for bots at this bracket
     */
    static uint32 GetExpectedGearScoreForBracket(PoolBracket bracket)
    {
        switch (bracket)
        {
            case PoolBracket::Bracket_10_19: return 15;
            case PoolBracket::Bracket_20_29: return 30;
            case PoolBracket::Bracket_30_39: return 50;
            case PoolBracket::Bracket_40_49: return 80;
            case PoolBracket::Bracket_50_59: return 120;
            case PoolBracket::Bracket_60_69: return 280;
            case PoolBracket::Bracket_70_79: return 380;
            case PoolBracket::Bracket_80_Max: return 500;
            default: return 500;
        }
    }

    /**
     * @brief Get minimum gear score for content at bracket
     * @param bracket Level bracket
     * @param contentType 0=normal dungeon, 1=heroic, 2=mythic, 3=raid
     * @return Minimum required gear score
     */
    uint32 GetMinGearScoreForContent(PoolBracket bracket, uint32 contentType) const
    {
        uint32 baseGS = GetExpectedGearScoreForBracket(bracket);

        switch (contentType)
        {
            case 0: return baseGS;                          // Normal dungeon
            case 1: return baseGS + 50;                     // Heroic dungeon
            case 2: return baseGS + 100;                    // Mythic dungeon
            case 3: return baseGS + 80;                     // Raid
            default: return baseGS;
        }
    }

    // ========================================================================
    // LEGACY HELPER METHODS (BACKWARD COMPATIBILITY)
    // ========================================================================

    /**
     * @brief Get legacy expansion bracket for level (0-3)
     * @param level Character level
     * @return Legacy bracket index (0=Starting, 1=Chromie, 2=DF, 3=TWW)
     * @deprecated Use GetBracketForLevel() for new 8-bracket system
     */
    static uint32 GetLegacyBracketForLevel(uint32 level)
    {
        if (level < STARTING_MAX)
            return 0;
        if (level < CHROMIE_MAX)
            return 1;
        if (level < DF_MAX)
            return 2;
        return 3;
    }

    /**
     * @brief Get level range for legacy bracket
     * @param bracket Legacy bracket index (0-3)
     * @param[out] minLevel Minimum level
     * @param[out] maxLevel Maximum level
     * @deprecated Use GetBracketLevelRange() for new 8-bracket system
     */
    static void GetLegacyLevelRange(uint32 bracket, uint32& minLevel, uint32& maxLevel)
    {
        switch (bracket)
        {
            case 0:  minLevel = STARTING_MIN; maxLevel = STARTING_MAX; break;
            case 1:  minLevel = CHROMIE_MIN;  maxLevel = CHROMIE_MAX;  break;
            case 2:  minLevel = DF_MIN;       maxLevel = DF_MAX;       break;
            default: minLevel = TWW_MIN;      maxLevel = TWW_MAX;      break;
        }
    }

    /**
     * @brief Convert legacy 4-bracket to new 8-bracket system
     * @param legacyBracket Legacy bracket (0-3)
     * @return Primary PoolBracket for that expansion tier
     */
    static PoolBracket LegacyToNewBracket(uint32 legacyBracket)
    {
        switch (legacyBracket)
        {
            case 0: return PoolBracket::Bracket_10_19;   // Starting
            case 1: return PoolBracket::Bracket_30_39;   // Chromie (midpoint)
            case 2: return PoolBracket::Bracket_60_69;   // Dragonflight
            case 3: return PoolBracket::Bracket_80_Max;  // TWW max
            default: return PoolBracket::Bracket_80_Max;
        }
    }

    /**
     * @brief Get all new brackets that fall within a legacy bracket
     * @param legacyBracket Legacy bracket (0-3)
     * @param[out] brackets Array to fill with matching PoolBracket values
     * @return Number of brackets filled
     */
    static uint32 GetNewBracketsForLegacy(uint32 legacyBracket, std::array<PoolBracket, 8>& brackets)
    {
        uint32 count = 0;
        switch (legacyBracket)
        {
            case 0:  // Starting (1-10) → only 10-19
                brackets[count++] = PoolBracket::Bracket_10_19;
                break;
            case 1:  // Chromie (10-60) → 10-19, 20-29, 30-39, 40-49, 50-59
                brackets[count++] = PoolBracket::Bracket_10_19;
                brackets[count++] = PoolBracket::Bracket_20_29;
                brackets[count++] = PoolBracket::Bracket_30_39;
                brackets[count++] = PoolBracket::Bracket_40_49;
                brackets[count++] = PoolBracket::Bracket_50_59;
                break;
            case 2:  // Dragonflight (60-70) → 60-69
                brackets[count++] = PoolBracket::Bracket_60_69;
                break;
            case 3:  // TWW (70-80) → 70-79, 80+
                brackets[count++] = PoolBracket::Bracket_70_79;
                brackets[count++] = PoolBracket::Bracket_80_Max;
                break;
        }
        return count;
    }
};

// ============================================================================
// TIMING CONFIGURATION
// ============================================================================

/**
 * @brief Configuration for timing-related parameters
 *
 * All timing values can be adjusted based on server performance
 * and player experience requirements.
 */
struct PoolTimingConfig
{
    // ========================================================================
    // COOLDOWNS
    // ========================================================================

    /// Cooldown between bot assignments (seconds)
    /// Prevents the same bot from being assigned repeatedly
    std::chrono::seconds cooldownDuration{300};  // 5 minutes

    /// Shorter cooldown for overflow/JIT bots (seconds)
    std::chrono::seconds overflowCooldownDuration{60};  // 1 minute

    // ========================================================================
    // TIMEOUTS
    // ========================================================================

    /// Maximum time for reservation to be fulfilled (ms)
    std::chrono::milliseconds reservationTimeout{60000};  // 1 minute

    /// Maximum time for bot warmup/login (ms)
    std::chrono::milliseconds warmupTimeout{30000};  // 30 seconds

    /// Maximum time for JIT bot creation (ms)
    std::chrono::milliseconds jitCreationTimeout{60000};  // 1 minute

    /// Time before recycled JIT bots are cleaned up (minutes)
    std::chrono::minutes recycleTimeout{5};

    // ========================================================================
    // UPDATE INTERVALS
    // ========================================================================

    /// Pool maintenance update interval (ms)
    uint32 updateIntervalMs = 1000;  // 1 second

    /// Database sync interval (ms)
    uint32 dbSyncIntervalMs = 60000;  // 1 minute

    /// Statistics calculation interval (ms)
    uint32 statsIntervalMs = 5000;  // 5 seconds

    /// Pool replenishment check interval (ms)
    uint32 replenishIntervalMs = 10000;  // 10 seconds
};

// ============================================================================
// BEHAVIOR CONFIGURATION
// ============================================================================

/**
 * @brief Configuration for pool behavior and automation
 */
struct PoolBehaviorConfig
{
    // ========================================================================
    // AUTOMATION
    // ========================================================================

    /// Automatically replenish pool when bots are assigned
    bool autoReplenish = true;

    /// Persist pool state to database
    bool persistToDatabase = true;

    /// Warm pool on server startup
    bool warmOnStartup = true;

    /// Use JIT factory when pool is insufficient
    bool enableJITFactory = true;

    /// Enable bot recycling for JIT bots
    bool enableRecycling = true;

    // ========================================================================
    // THRESHOLDS
    // ========================================================================

    /// Use JIT factory when pool drops below this percentage
    uint32 jitThresholdPct = 20;

    /// Maximum recycled bots to keep
    uint32 maxRecycledBots = 100;

    /// Minimum bots to keep ready per role (prevents exhaustion)
    uint32 minBotsPerRole = 5;

    // ========================================================================
    // ASSIGNMENT PREFERENCES
    // ========================================================================

    /// Prefer bots that haven't been used recently
    bool spreadAssignments = true;

    /// Prefer bots with higher gear scores
    bool preferHighGearScore = true;

    /// Prefer bots with higher success rates
    bool preferHighSuccessRate = true;
};

// ============================================================================
// LOGGING CONFIGURATION
// ============================================================================

/**
 * @brief Configuration for pool logging and debugging
 */
struct PoolLoggingConfig
{
    /// Log individual bot assignments
    bool logAssignments = true;

    /// Log pool state changes
    bool logPoolChanges = false;

    /// Log JIT factory operations
    bool logJITOperations = true;

    /// Log cooldown expirations
    bool logCooldowns = false;

    /// Log reservation operations
    bool logReservations = true;

    /// Log detailed statistics periodically
    bool logDetailedStats = false;

    /// Log level for pool operations (0=disabled, 1=error, 2=warn, 3=info, 4=debug)
    uint32 logLevel = 3;
};

// ============================================================================
// MASTER CONFIGURATION
// ============================================================================

/**
 * @brief Master configuration structure for Instance Bot Pool
 *
 * This structure aggregates all configuration sub-structures
 * and provides methods for loading from config file.
 */
struct TC_GAME_API InstanceBotPoolConfig
{
    // ========================================================================
    // ENABLE/DISABLE
    // ========================================================================

    /// Master enable switch for instance bot pool
    bool enabled = true;

    // ========================================================================
    // SUB-CONFIGURATIONS
    // ========================================================================

    PoolSizeConfig poolSize;
    PoolLevelConfig levelConfig;
    PoolTimingConfig timing;
    PoolBehaviorConfig behavior;
    PoolLoggingConfig logging;

    // ========================================================================
    // LOADING
    // ========================================================================

    /**
     * @brief Load configuration from config file
     *
     * Reads all Playerbot.Instance.Pool.* settings from playerbots.conf
     */
    void LoadFromConfig();

    /**
     * @brief Validate configuration values
     * @return true if all values are valid
     */
    bool Validate() const;

    /**
     * @brief Get default configuration
     * @return Default config instance
     */
    static InstanceBotPoolConfig GetDefault();

    /**
     * @brief Print configuration to log
     */
    void PrintToLog() const;
};

// ============================================================================
// JIT FACTORY CONFIGURATION
// ============================================================================

/**
 * @brief Configuration specific to JIT (Just-In-Time) Bot Factory
 *
 * The JIT factory creates bots on-demand when the warm pool is exhausted,
 * typically for large content like 40-man raids or 40v40 battlegrounds.
 */
struct JITFactoryConfig
{
    /// Enable JIT factory
    bool enabled = true;

    /// Maximum concurrent bot creations
    uint32 maxConcurrentCreations = 10;

    /// Maximum requests in queue
    uint32 maxQueuedRequests = 50;

    /// Time before recycled bots are deleted (minutes)
    uint32 recycleTimeoutMinutes = 5;

    /// Maximum recycled bots to keep
    uint32 maxRecycledBots = 100;

    // ========================================================================
    // SAFETY HARD CAP (2026-01-22 FIX)
    // ========================================================================
    // Absolute maximum total instance bots that can exist at any time.
    // This is a safety cap to prevent runaway bot explosion even if other
    // checks fail (e.g., if MarkAsInstanceBot somehow fails or bots aren't
    // properly released). If this cap is reached, no new JIT bots will be
    // created until existing ones log out.
    //
    // This is separate from warm pool size and exists purely as an emergency
    // safeguard. Set to 0 to disable this safeguard (not recommended).
    // ========================================================================
    uint32 maxTotalInstanceBots = 500;

    /// Use template cloning for fast creation
    bool useTemplateCloning = true;

    /// Pre-serialize bot templates for speed
    bool preSerializeTemplates = true;

    /// Log factory operations
    bool logOperations = true;

    // Priority settings (lower = higher priority)
    uint8 dungeonPriority = 1;
    uint8 arenaPriority = 2;
    uint8 raidPriority = 3;
    uint8 battlegroundPriority = 4;

    // Timeout settings (milliseconds)
    uint32 dungeonTimeoutMs = 30000;
    uint32 raidTimeoutMs = 60000;
    uint32 battlegroundTimeoutMs = 120000;
    uint32 arenaTimeoutMs = 15000;

    /**
     * @brief Load from config file
     */
    void LoadFromConfig();
};

// ============================================================================
// NOTE: InstanceOrchestratorConfig is defined in InstanceBotOrchestrator.h
// ============================================================================

// ============================================================================
// CONTENT REQUIREMENTS DEFAULTS
// ============================================================================

/**
 * @brief Default content requirements for instance types
 *
 * These defaults are used when content-specific requirements are not
 * defined in the database. They provide reasonable defaults for
 * standard WoW content.
 */
struct ContentRequirementDefaults
{
    // 5-man Dungeons
    static constexpr uint32 DUNGEON_TANKS = 1;
    static constexpr uint32 DUNGEON_HEALERS = 1;
    static constexpr uint32 DUNGEON_DPS = 3;
    static constexpr uint32 DUNGEON_TOTAL = 5;

    // 10-man Raids
    static constexpr uint32 RAID_10_TANKS = 2;
    static constexpr uint32 RAID_10_HEALERS = 3;
    static constexpr uint32 RAID_10_DPS = 5;
    static constexpr uint32 RAID_10_TOTAL = 10;

    // 25-man Raids
    static constexpr uint32 RAID_25_TANKS = 3;
    static constexpr uint32 RAID_25_HEALERS = 6;
    static constexpr uint32 RAID_25_DPS = 16;
    static constexpr uint32 RAID_25_TOTAL = 25;

    // 40-man Raids
    static constexpr uint32 RAID_40_TANKS = 4;
    static constexpr uint32 RAID_40_HEALERS = 10;
    static constexpr uint32 RAID_40_DPS = 26;
    static constexpr uint32 RAID_40_TOTAL = 40;

    // Battlegrounds
    static constexpr uint32 WSG_IOC_PER_FACTION = 10;      // Warsong Gulch, etc.
    static constexpr uint32 AB_PER_FACTION = 15;           // Arathi Basin
    static constexpr uint32 AV_IOC_PER_FACTION = 40;       // Alterac Valley, Isle of Conquest

    // Arenas
    static constexpr uint32 ARENA_2V2 = 2;
    static constexpr uint32 ARENA_3V3 = 3;
    static constexpr uint32 ARENA_5V5 = 5;
};

} // namespace Playerbot
