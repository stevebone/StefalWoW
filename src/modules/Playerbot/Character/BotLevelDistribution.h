/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Define.h"
#include "SharedDefines.h"
#include "ZoneLevelHelper.h"
#include <atomic>
#include <array>
#include <vector>
#include <map>
#include <string>
#include <random>

namespace Playerbot
{

/**
 * Thread-Safe Expansion Tier Bracket
 *
 * Represents an expansion tier with target distribution percentage.
 * Uses atomic counter for lock-free concurrent access.
 * Memory order relaxed is sufficient due to ±15% tolerance.
 *
 * Modern WoW 11.x uses expansion-based tiers:
 * - Starting (1-10): Exile's Reach / racial zones
 * - ChromieTime (10-60): All scaled content
 * - Dragonflight (60-70): Dragon Isles
 * - TheWarWithin (70-80): Khaz Algar
 */
struct LevelBracket
{
    ExpansionTier tier;
    uint32 minLevel;
    uint32 maxLevel;
    float targetPercentage;
    TeamId faction;

    // Thread-safe counter (relaxed memory order)
    mutable ::std::atomic<uint32> currentCount{0};

    // Copy constructor - manually copy atomic value
    LevelBracket(LevelBracket const& other)
        : tier(other.tier)
        , minLevel(other.minLevel)
        , maxLevel(other.maxLevel)
        , targetPercentage(other.targetPercentage)
        , faction(other.faction)
        , currentCount(other.currentCount.load(::std::memory_order_relaxed))
    {
    }

    // Default constructor
    LevelBracket() : tier(ExpansionTier::Starting), minLevel(1), maxLevel(10),
                     targetPercentage(0.0f), faction(TEAM_NEUTRAL) {}

    // Copy assignment - manually copy atomic value
    LevelBracket& operator=(LevelBracket const& other)
    {
        if (this != &other)
        {
            tier = other.tier;
            minLevel = other.minLevel;
            maxLevel = other.maxLevel;
            targetPercentage = other.targetPercentage;
            faction = other.faction;
            currentCount.store(other.currentCount.load(::std::memory_order_relaxed), ::std::memory_order_relaxed);
        }
        return *this;
    }

    // Thread-safe operations
    void IncrementCount() const
    {
        currentCount.fetch_add(1, ::std::memory_order_relaxed);
    }

    void DecrementCount() const
    {
        if (currentCount.load(::std::memory_order_relaxed) > 0)
            currentCount.fetch_sub(1, ::std::memory_order_relaxed);
    }

    uint32 GetCount() const
    {
        return currentCount.load(::std::memory_order_relaxed);
    }

    void SetCount(uint32 count) const
    {
        currentCount.store(count, ::std::memory_order_relaxed);
    }

    // Tier-based behavior flags
    bool IsStartingTier() const { return tier == ExpansionTier::Starting; }
    bool IsChromieTimeTier() const { return tier == ExpansionTier::ChromieTime; }
    bool IsDragonflightTier() const { return tier == ExpansionTier::Dragonflight; }
    bool IsWarWithinTier() const { return tier == ExpansionTier::TheWarWithin; }

    // Legacy behavior flags
    bool IsNaturalLeveling() const
    {
        return minLevel <= 4;  // Levels 1-4 level naturally
    }

    bool SupportsDualSpec() const
    {
        return minLevel >= 10;  // WoW 12.0: Dual-spec unlocks at level 10
    }

    bool IsEndgame() const
    {
        return maxLevel == 80;
    }

    // Get random level within bracket
    uint32 GetRandomLevel() const
    {
        if (minLevel == maxLevel)
            return minLevel;

        thread_local ::std::mt19937 generator(::std::random_device{}());
        ::std::uniform_int_distribution<uint32> distribution(minLevel, maxLevel);
        return distribution(generator);
    }

    // Calculate target count based on total bots
    uint32 GetTargetCount(uint32 totalBots) const
    {
        return static_cast<uint32>(totalBots * targetPercentage / 100.0f);
    }

    // Check if bracket is within tolerance (±15%)
    bool IsWithinTolerance(uint32 totalBots) const
    {
        uint32 target = GetTargetCount(totalBots);
        uint32 current = GetCount();

        float lowerBound = target * 0.85f;
        float upperBound = target * 1.15f;

        return current >= lowerBound && current <= upperBound;
    }

    // Get deviation from target (-1.0 = empty, 0.0 = perfect, +1.0 = double target)
    float GetDeviation(uint32 totalBots) const
    {
        uint32 target = GetTargetCount(totalBots);
        if (target == 0)
            return 0.0f;

        uint32 current = GetCount();
        return (static_cast<float>(current) - static_cast<float>(target)) / static_cast<float>(target);
    }

    // Priority for selection (negative deviation = needs more bots)
    float GetSelectionPriority(uint32 totalBots) const
    {
        return -GetDeviation(totalBots);  // Negative deviation = higher priority
    }

    // Get tier name for logging
    ::std::string GetTierName() const
    {
        switch (tier)
        {
            case ExpansionTier::Starting: return "Starting";
            case ExpansionTier::ChromieTime: return "ChromieTime";
            case ExpansionTier::Dragonflight: return "Dragonflight";
            case ExpansionTier::TheWarWithin: return "TheWarWithin";
            default: return "Unknown";
        }
    }
};

/**
 * Distribution Statistics
 */
struct DistributionStats
{
    uint32 totalBots;
    uint32 allianceBots;
    uint32 hordeBots;
    uint32 bracketsWithinTolerance;
    uint32 totalBrackets;
    float averageDeviation;
    float maxDeviation;
    ::std::string mostUnderpopulatedBracket;
    ::std::string mostOverpopulatedBracket;
};

/**
 * Bot Level Distribution System
 *
 * Purpose: Automated world population with level-appropriate bots
 *
 * Features:
 * - Expansion tier-based distribution (4 tiers: Starting, Chromie, DF, TWW)
 * - Level ranges derived from ContentTuning DB2 via ZoneLevelHelper
 * - Thread-safe atomic counters (lock-free reads)
 * - Distribution tolerance checking (±15%)
 * - Weighted bracket selection based on deviation
 * - Separate Alliance/Horde distributions
 * - Natural leveling for levels 1-4 (no instant gear)
 * - Instant level-up + gear for levels 5+
 *
 * Thread Safety:
 * - Config data is immutable after LoadConfig()
 * - Atomic counters for current bot counts
 * - No locks required for bracket selection
 * - Relaxed memory ordering (tolerance allows eventual consistency)
 *
 * Performance:
 * - O(1) tier selection (4 fixed tiers)
 * - Lock-free counter updates
 * - Minimal contention
 *
 * ContentTuning Integration:
 * - Uses ZoneLevelHelper to get zone level requirements
 * - Expansion tiers: Starting(1-10), Chromie(10-60), DF(60-70), TWW(70-80)
 * - Target percentages configurable via Playerbot.Population.Tier.*.Pct
 */
class TC_GAME_API BotLevelDistribution final
{
public:
    static BotLevelDistribution* instance();

    // Initialization
    bool LoadConfig();
    void ReloadConfig();

    // Bracket selection
    LevelBracket const* SelectBracket(TeamId faction) const;
    LevelBracket const* SelectBracketWeighted(TeamId faction) const;
    LevelBracket const* GetBracketForLevel(uint32 level, TeamId faction) const;

    // Tier-based selection (new API)
    LevelBracket const* SelectTier(TeamId faction, ExpansionTier tier) const;
    LevelBracket const* GetBracketForTier(ExpansionTier tier, TeamId faction) const;

    // Distribution analysis
    DistributionStats GetDistributionStats() const;
    ::std::vector<LevelBracket const*> GetUnderpopulatedBrackets(TeamId faction) const;
    ::std::vector<LevelBracket const*> GetOverpopulatedBrackets(TeamId faction) const;
    bool IsDistributionBalanced(TeamId faction) const;

    // Counter updates
    void IncrementBracket(uint32 level, TeamId faction);
    void DecrementBracket(uint32 level, TeamId faction);
    void RecalculateDistribution();

    // Configuration queries
    uint32 GetNumBrackets() const { return NUM_TIERS; }
    float GetTolerancePercent() const { return 15.0f; }  // ±15% tolerance
    bool IsEnabled() const { return m_enabled; }
    bool IsDynamicDistribution() const { return m_dynamicDistribution; }

    // Zone-level integration
    bool IsLevelValidForZone(uint32 level, uint32 zoneId) const;
    uint32 GetRecommendedLevelForZone(uint32 zoneId) const;

    // Debugging
    void PrintDistributionReport() const;
    ::std::string GetDistributionSummary() const;

    // Number of expansion tiers
    static constexpr uint32 NUM_TIERS = static_cast<uint32>(ExpansionTier::Max);

private:
    BotLevelDistribution() = default;
    ~BotLevelDistribution() = default;
    BotLevelDistribution(BotLevelDistribution const&) = delete;
    BotLevelDistribution& operator=(BotLevelDistribution const&) = delete;

    // Tier-based initialization
    void InitializeTiers();
    void BuildTiersFromZoneLevelHelper();
    bool ValidateConfig() const;

    // Selection helpers
    LevelBracket const* SelectByPriority(::std::vector<LevelBracket> const& brackets) const;

private:
    // Configuration (immutable after load)
    bool m_enabled = false;
    bool m_dynamicDistribution = false;
    float m_realPlayerWeight = 1.0f;
    bool m_syncFactions = false;

    // Tier-based bracket storage (4 tiers per faction)
    ::std::array<LevelBracket, NUM_TIERS> m_allianceTiers;
    ::std::array<LevelBracket, NUM_TIERS> m_hordeTiers;

    // Legacy vector-based access for interface compatibility
    ::std::vector<LevelBracket> m_allianceBrackets;
    ::std::vector<LevelBracket> m_hordeBrackets;

    // Fast lookup map: level -> tier index
    ::std::array<size_t, 81> m_levelToTierIndex;  // levels 0-80

    // RNG (thread-local for thread safety)
    mutable ::std::mt19937 m_generator{::std::random_device{}()};

    // Status
    bool m_loaded = false;
};

} // namespace Playerbot

#define sBotLevelDistribution Playerbot::BotLevelDistribution::instance()
