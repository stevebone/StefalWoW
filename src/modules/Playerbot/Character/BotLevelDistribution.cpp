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

#include "BotLevelDistribution.h"
#include "Config/PlayerbotConfig.h"
#include "Log.h"
#include "DatabaseEnv.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace Playerbot
{

BotLevelDistribution* BotLevelDistribution::instance()
{
    static BotLevelDistribution instance;
    return &instance;
}

bool BotLevelDistribution::LoadConfig()
{
    TC_LOG_INFO("playerbot", "BotLevelDistribution: Loading configuration (tier-based)...");

    m_enabled = sPlayerbotConfig->GetBool("Playerbot.Population.Enabled", false);
    if (!m_enabled)
    {
        TC_LOG_INFO("playerbot", "BotLevelDistribution: System disabled in config");
        return false;
    }

    m_dynamicDistribution = sPlayerbotConfig->GetBool("Playerbot.Population.DynamicDistribution", false);
    m_realPlayerWeight = sPlayerbotConfig->GetFloat("Playerbot.Population.RealPlayerWeight", 1.0f);
    m_syncFactions = sPlayerbotConfig->GetBool("Playerbot.Population.SyncFactions", false);

    // Clear existing data
    m_allianceBrackets.clear();
    m_hordeBrackets.clear();

    // Initialize ZoneLevelHelper if not already done
    if (!sZoneLevelHelper.IsInitialized())
    {
        sZoneLevelHelper.Initialize();
    }

    // Build tiers from ZoneLevelHelper (ContentTuning DB2)
    BuildTiersFromZoneLevelHelper();

    // Validate configuration
    if (!ValidateConfig())
    {
        TC_LOG_ERROR("playerbot", "BotLevelDistribution: Configuration validation failed");
        m_loaded = false;
        return false;
    }

    m_loaded = true;
    TC_LOG_INFO("playerbot", "BotLevelDistribution: Loaded {} expansion tiers for both factions",
        NUM_TIERS);

    PrintDistributionReport();
    return true;
}

void BotLevelDistribution::BuildTiersFromZoneLevelHelper()
{
    // Get expansion tier configs from ZoneLevelHelper
    auto const& tierConfigs = sZoneLevelHelper.GetExpansionTiers();

    // Initialize both factions with the same tier structure
    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        ExpansionTierConfig const& config = tierConfigs[i];

        // Alliance tier
        m_allianceTiers[i].tier = config.tier;
        m_allianceTiers[i].minLevel = config.levels.minLevel;
        m_allianceTiers[i].maxLevel = config.levels.maxLevel;
        m_allianceTiers[i].targetPercentage = config.targetPercentage;
        m_allianceTiers[i].faction = TEAM_ALLIANCE;
        m_allianceTiers[i].SetCount(0);

        // Horde tier (same levels, different faction)
        m_hordeTiers[i].tier = config.tier;
        m_hordeTiers[i].minLevel = config.levels.minLevel;
        m_hordeTiers[i].maxLevel = config.levels.maxLevel;
        m_hordeTiers[i].targetPercentage = config.targetPercentage;
        m_hordeTiers[i].faction = TEAM_HORDE;
        m_hordeTiers[i].SetCount(0);

        // Add to vector-based storage for interface compatibility
        m_allianceBrackets.push_back(m_allianceTiers[i]);
        m_hordeBrackets.push_back(m_hordeTiers[i]);

        TC_LOG_DEBUG("playerbot", "BotLevelDistribution: Tier {} ({}): L{}-{}, {}%",
            i, config.name, config.levels.minLevel, config.levels.maxLevel,
            config.targetPercentage);
    }

    // Build level-to-tier lookup table
    m_levelToTierIndex.fill(0);  // Default to Starting tier
    for (uint32 level = 1; level <= 80; ++level)
    {
        ExpansionTier tier = sZoneLevelHelper.GetTierForLevel(static_cast<int16>(level));
        m_levelToTierIndex[level] = static_cast<size_t>(tier);
    }
}

bool BotLevelDistribution::ValidateConfig() const
{
    // Validate tier coverage
    if (m_allianceTiers.empty() || m_hordeTiers.empty())
    {
        TC_LOG_ERROR("playerbot", "BotLevelDistribution: No tiers initialized");
        return false;
    }

    // Validate percentage sums
    float allianceSum = 0.0f;
    float hordeSum = 0.0f;

    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        allianceSum += m_allianceTiers[i].targetPercentage;
        hordeSum += m_hordeTiers[i].targetPercentage;
    }

    if (::std::abs(allianceSum - 100.0f) > 1.0f)
    {
        TC_LOG_WARN("playerbot", "BotLevelDistribution: Alliance percentages sum to {:.2f}% (should be 100%)",
            allianceSum);
    }

    if (::std::abs(hordeSum - 100.0f) > 1.0f)
    {
        TC_LOG_WARN("playerbot", "BotLevelDistribution: Horde percentages sum to {:.2f}% (should be 100%)",
            hordeSum);
    }

    // Verify all levels 1-80 have tier coverage
    for (uint32 level = 1; level <= 80; ++level)
    {
        if (m_levelToTierIndex[level] >= NUM_TIERS)
        {
            TC_LOG_ERROR("playerbot", "BotLevelDistribution: Level {} has invalid tier index", level);
            return false;
        }
    }

    return true;
}

void BotLevelDistribution::ReloadConfig()
{
    TC_LOG_INFO("playerbot", "BotLevelDistribution: Reloading configuration...");

    // Refresh ZoneLevelHelper cache
    sZoneLevelHelper.RefreshCache();

    LoadConfig();
}

LevelBracket const* BotLevelDistribution::SelectBracket(TeamId faction) const
{
    if (!m_loaded)
        return nullptr;

    // Use weighted selection based on priority (deviation from target)
    return SelectBracketWeighted(faction);
}

LevelBracket const* BotLevelDistribution::SelectBracketWeighted(TeamId faction) const
{
    auto const& tiers = (faction == TEAM_ALLIANCE) ? m_allianceTiers : m_hordeTiers;

    // Calculate total bots for this faction
    uint32 totalBots = 0;
    for (size_t i = 0; i < NUM_TIERS; ++i)
        totalBots += tiers[i].GetCount();

    // If no bots yet, use target distribution
    if (totalBots == 0)
    {
        float random = static_cast<float>(rand()) / RAND_MAX * 100.0f;
        float cumulative = 0.0f;

        for (size_t i = 0; i < NUM_TIERS; ++i)
        {
            cumulative += tiers[i].targetPercentage;
            if (random <= cumulative)
                return &tiers[i];
        }

        return &tiers[NUM_TIERS - 1];
    }

    // Calculate priorities (negative deviation = needs more bots)
    ::std::array<float, NUM_TIERS> priorities;

    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        float priority = tiers[i].GetSelectionPriority(totalBots);
        // Boost priority for underpopulated tiers
        if (priority > 0.0f)
            priority *= 2.0f;  // Double weight for underpopulated
        else
            priority = 0.01f;  // Minimal chance for overpopulated

        priorities[i] = priority;
    }

    // Normalize priorities to probabilities
    float totalPriority = 0.0f;
    for (size_t i = 0; i < NUM_TIERS; ++i)
        totalPriority += priorities[i];

    if (totalPriority <= 0.0f)
        return &tiers[rand() % NUM_TIERS];

    // Select tier by weighted random
    float random = static_cast<float>(rand()) / RAND_MAX * totalPriority;
    float cumulative = 0.0f;

    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        cumulative += priorities[i];
        if (random <= cumulative)
            return &tiers[i];
    }

    return &tiers[NUM_TIERS - 1];
}

LevelBracket const* BotLevelDistribution::GetBracketForLevel(uint32 level, TeamId faction) const
{
    if (!m_loaded || level == 0 || level > 80)
        return nullptr;

    size_t tierIndex = m_levelToTierIndex[level];
    if (tierIndex >= NUM_TIERS)
        return nullptr;

    auto const& tiers = (faction == TEAM_ALLIANCE) ? m_allianceTiers : m_hordeTiers;
    return &tiers[tierIndex];
}

LevelBracket const* BotLevelDistribution::SelectTier(TeamId faction, ExpansionTier tier) const
{
    if (!m_loaded)
        return nullptr;

    size_t tierIndex = static_cast<size_t>(tier);
    if (tierIndex >= NUM_TIERS)
        return nullptr;

    auto const& tiers = (faction == TEAM_ALLIANCE) ? m_allianceTiers : m_hordeTiers;
    return &tiers[tierIndex];
}

LevelBracket const* BotLevelDistribution::GetBracketForTier(ExpansionTier tier, TeamId faction) const
{
    return SelectTier(faction, tier);
}

bool BotLevelDistribution::IsLevelValidForZone(uint32 level, uint32 zoneId) const
{
    return sZoneLevelHelper.IsLevelValidForZone(zoneId, static_cast<int16>(level));
}

uint32 BotLevelDistribution::GetRecommendedLevelForZone(uint32 zoneId) const
{
    return static_cast<uint32>(sZoneLevelHelper.GetRecommendedSpawnLevel(zoneId));
}

void BotLevelDistribution::IncrementBracket(uint32 level, TeamId faction)
{
    if (auto const* bracket = GetBracketForLevel(level, faction))
        bracket->IncrementCount();
}

void BotLevelDistribution::DecrementBracket(uint32 level, TeamId faction)
{
    if (auto const* bracket = GetBracketForLevel(level, faction))
        bracket->DecrementCount();
}

void BotLevelDistribution::RecalculateDistribution()
{
    TC_LOG_INFO("playerbot", "BotLevelDistribution: Recalculating distribution from active bot sessions...");

    // Phase 1: Reset all tier counters to 0
    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        m_allianceTiers[i].SetCount(0);
        m_hordeTiers[i].SetCount(0);
    }

    // Phase 2: Count bots by level and faction from database
    uint32 allianceTotal = 0;
    uint32 hordeTotal = 0;
    uint32 skippedNoPlayer = 0;
    uint32 skippedInvalidLevel = 0;

    // Query database for all bot characters with their levels and factions
    QueryResult result = CharacterDatabase.Query(
        "SELECT c.level, c.race FROM characters c "
        "JOIN account a ON c.account = a.id "
        "WHERE a.battlenet_account IN (SELECT id FROM battlenet_accounts WHERE email LIKE 'bot%@bot.bot') "
        "AND c.online = 1"
    );

    if (!result)
    {
        TC_LOG_DEBUG("playerbot", "BotLevelDistribution: No online bot characters found in database");

        DistributionStats stats = GetDistributionStats();
        TC_LOG_INFO("playerbot", "BotLevelDistribution: Recalculation complete - Alliance: {}, Horde: {}, Total: {}",
            stats.allianceBots, stats.hordeBots, stats.totalBots);
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint8 level = fields[0].GetUInt8();
        uint8 race = fields[1].GetUInt8();

        // Validate level range
        if (level == 0 || level > 80)
        {
            ++skippedInvalidLevel;
            continue;
        }

        // Determine faction from race
        TeamId faction;
        switch (race)
        {
            case 1: case 3: case 4: case 7: case 11: case 22:  // Classic Alliance
            case 25: case 29: case 30: case 32: case 34: case 37:  // Allied Alliance
            case 52: case 70: case 84: case 85:  // Dracthyr, Earthen
                faction = TEAM_ALLIANCE;
                break;
            case 2: case 5: case 6: case 8: case 9: case 10:  // Classic Horde
            case 26: case 27: case 28: case 31: case 35: case 36:  // Allied Horde
            case 53:  // Dracthyr Horde
                faction = TEAM_HORDE;
                break;
            default:
                faction = TEAM_ALLIANCE;
                TC_LOG_DEBUG("playerbot", "BotLevelDistribution: Unknown race {} defaulting to Alliance", race);
                break;
        }

        // Find and increment the appropriate tier
        LevelBracket const* bracket = GetBracketForLevel(level, faction);
        if (bracket)
        {
            bracket->IncrementCount();
            if (faction == TEAM_ALLIANCE)
                ++allianceTotal;
            else
                ++hordeTotal;
        }
        else
        {
            TC_LOG_DEBUG("playerbot", "BotLevelDistribution: No tier found for level {} faction {}",
                level, faction == TEAM_ALLIANCE ? "Alliance" : "Horde");
        }
    } while (result->NextRow());

    // Phase 3: Log summary
    TC_LOG_INFO("playerbot", "BotLevelDistribution: Recalculation complete - Alliance: {}, Horde: {}, Total: {}",
        allianceTotal, hordeTotal, allianceTotal + hordeTotal);

    if (skippedNoPlayer > 0)
    {
        TC_LOG_DEBUG("playerbot", "BotLevelDistribution: Skipped {} sessions with no player", skippedNoPlayer);
    }
    if (skippedInvalidLevel > 0)
    {
        TC_LOG_DEBUG("playerbot", "BotLevelDistribution: Skipped {} characters with invalid level", skippedInvalidLevel);
    }

    // Phase 4: Log distribution status
    DistributionStats stats = GetDistributionStats();
    TC_LOG_INFO("playerbot", "BotLevelDistribution: Distribution balance - {}/{} tiers within tolerance, avg deviation: {:.1f}%",
        stats.bracketsWithinTolerance, stats.totalBrackets, stats.averageDeviation * 100.0f);

    if (!stats.mostUnderpopulatedBracket.empty())
    {
        TC_LOG_INFO("playerbot", "BotLevelDistribution: Most underpopulated tier: {}", stats.mostUnderpopulatedBracket);
    }
    if (!stats.mostOverpopulatedBracket.empty())
    {
        TC_LOG_INFO("playerbot", "BotLevelDistribution: Most overpopulated tier: {}", stats.mostOverpopulatedBracket);
    }
}

DistributionStats BotLevelDistribution::GetDistributionStats() const
{
    DistributionStats stats{};
    stats.totalBrackets = NUM_TIERS * 2;  // 4 tiers per faction

    // Calculate Alliance stats
    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        uint32 count = m_allianceTiers[i].GetCount();
        stats.allianceBots += count;
        stats.totalBots += count;
    }

    // Calculate Horde stats
    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        uint32 count = m_hordeTiers[i].GetCount();
        stats.hordeBots += count;
        stats.totalBots += count;
    }

    // Calculate deviation statistics
    ::std::vector<float> deviations;
    float maxDeviation = 0.0f;
    float maxDevValue = 0.0f;
    ::std::string maxDevName;

    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        LevelBracket const& tier = m_allianceTiers[i];
        if (tier.IsWithinTolerance(stats.allianceBots))
            ++stats.bracketsWithinTolerance;

        float deviation = ::std::abs(tier.GetDeviation(stats.allianceBots));
        deviations.push_back(deviation);

        if (deviation > maxDeviation)
        {
            maxDeviation = deviation;
            maxDevValue = tier.GetDeviation(stats.allianceBots);
            maxDevName = "Alliance " + tier.GetTierName();
        }
    }

    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        LevelBracket const& tier = m_hordeTiers[i];
        if (tier.IsWithinTolerance(stats.hordeBots))
            ++stats.bracketsWithinTolerance;

        float deviation = ::std::abs(tier.GetDeviation(stats.hordeBots));
        deviations.push_back(deviation);

        if (deviation > maxDeviation)
        {
            maxDeviation = deviation;
            maxDevValue = tier.GetDeviation(stats.hordeBots);
            maxDevName = "Horde " + tier.GetTierName();
        }
    }

    stats.averageDeviation = deviations.empty() ? 0.0f :
        ::std::accumulate(deviations.begin(), deviations.end(), 0.0f) / deviations.size();
    stats.maxDeviation = maxDeviation;

    if (maxDevValue > 0.0f)
        stats.mostOverpopulatedBracket = maxDevName;
    else
        stats.mostUnderpopulatedBracket = maxDevName;

    return stats;
}

::std::vector<LevelBracket const*> BotLevelDistribution::GetUnderpopulatedBrackets(TeamId faction) const
{
    ::std::vector<LevelBracket const*> result;
    auto const& tiers = (faction == TEAM_ALLIANCE) ? m_allianceTiers : m_hordeTiers;

    uint32 totalBots = 0;
    for (size_t i = 0; i < NUM_TIERS; ++i)
        totalBots += tiers[i].GetCount();

    if (totalBots == 0)
        return result;

    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        if (!tiers[i].IsWithinTolerance(totalBots) && tiers[i].GetDeviation(totalBots) < 0.0f)
            result.push_back(&tiers[i]);
    }

    // Sort by priority (most underpopulated first)
    ::std::sort(result.begin(), result.end(),
        [totalBots](LevelBracket const* a, LevelBracket const* b) {
            return a->GetDeviation(totalBots) < b->GetDeviation(totalBots);
        });

    return result;
}

::std::vector<LevelBracket const*> BotLevelDistribution::GetOverpopulatedBrackets(TeamId faction) const
{
    ::std::vector<LevelBracket const*> result;
    auto const& tiers = (faction == TEAM_ALLIANCE) ? m_allianceTiers : m_hordeTiers;

    uint32 totalBots = 0;
    for (size_t i = 0; i < NUM_TIERS; ++i)
        totalBots += tiers[i].GetCount();

    if (totalBots == 0)
        return result;

    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        if (!tiers[i].IsWithinTolerance(totalBots) && tiers[i].GetDeviation(totalBots) > 0.0f)
            result.push_back(&tiers[i]);
    }

    // Sort by deviation (most overpopulated first)
    ::std::sort(result.begin(), result.end(),
        [totalBots](LevelBracket const* a, LevelBracket const* b) {
            return a->GetDeviation(totalBots) > b->GetDeviation(totalBots);
        });

    return result;
}

bool BotLevelDistribution::IsDistributionBalanced(TeamId faction) const
{
    auto const& tiers = (faction == TEAM_ALLIANCE) ? m_allianceTiers : m_hordeTiers;

    uint32 totalBots = 0;
    for (size_t i = 0; i < NUM_TIERS; ++i)
        totalBots += tiers[i].GetCount();

    if (totalBots == 0)
        return true;  // Empty is considered balanced

    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        if (!tiers[i].IsWithinTolerance(totalBots))
            return false;
    }

    return true;
}

void BotLevelDistribution::PrintDistributionReport() const
{
    TC_LOG_INFO("playerbot", "==============================================");
    TC_LOG_INFO("playerbot", "Bot Level Distribution (Expansion Tier-Based)");
    TC_LOG_INFO("playerbot", "==============================================");
    TC_LOG_INFO("playerbot", "Enabled: {}", m_enabled ? "YES" : "NO");
    TC_LOG_INFO("playerbot", "Tiers: {} (ContentTuning DB2-based)", NUM_TIERS);
    TC_LOG_INFO("playerbot", "Dynamic Distribution: {}", m_dynamicDistribution ? "YES" : "NO");
    TC_LOG_INFO("playerbot", "Tolerance: ±15%");
    TC_LOG_INFO("playerbot", "");

    // Alliance tiers
    TC_LOG_INFO("playerbot", "Alliance Tiers:");
    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        LevelBracket const& tier = m_allianceTiers[i];
        TC_LOG_INFO("playerbot", "  {} (L{}-{}): {:.1f}%",
            tier.GetTierName(), tier.minLevel, tier.maxLevel, tier.targetPercentage);
    }

    TC_LOG_INFO("playerbot", "");

    // Horde tiers
    TC_LOG_INFO("playerbot", "Horde Tiers:");
    for (size_t i = 0; i < NUM_TIERS; ++i)
    {
        LevelBracket const& tier = m_hordeTiers[i];
        TC_LOG_INFO("playerbot", "  {} (L{}-{}): {:.1f}%",
            tier.GetTierName(), tier.minLevel, tier.maxLevel, tier.targetPercentage);
    }

    TC_LOG_INFO("playerbot", "");
    TC_LOG_INFO("playerbot", "Zone Level Data: {} zones cached (via ContentTuning DB2)",
        sZoneLevelHelper.GetCachedZoneCount());
    TC_LOG_INFO("playerbot", "==============================================");
}

::std::string BotLevelDistribution::GetDistributionSummary() const
{
    DistributionStats stats = GetDistributionStats();

    ::std::ostringstream oss;
    oss << "Total Bots: " << stats.totalBots
        << " | Alliance: " << stats.allianceBots
        << " | Horde: " << stats.hordeBots
        << " | Balanced Tiers: " << stats.bracketsWithinTolerance << "/" << stats.totalBrackets
        << " | Avg Deviation: " << ::std::fixed << ::std::setprecision(1) << (stats.averageDeviation * 100.0f) << "%";

    return oss.str();
}

LevelBracket const* BotLevelDistribution::SelectByPriority(::std::vector<LevelBracket> const& brackets) const
{
    if (brackets.empty())
        return nullptr;

    // Calculate total bots
    uint32 totalBots = 0;
    for (auto const& bracket : brackets)
        totalBots += bracket.GetCount();

    // Find bracket with highest priority (most underpopulated)
    LevelBracket const* best = &brackets[0];
    float bestPriority = best->GetSelectionPriority(totalBots);

    for (size_t i = 1; i < brackets.size(); ++i)
    {
        float priority = brackets[i].GetSelectionPriority(totalBots);
        if (priority > bestPriority)
        {
            bestPriority = priority;
            best = &brackets[i];
        }
    }

    return best;
}

} // namespace Playerbot
