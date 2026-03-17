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

#ifndef _LFGBOTSELECTOR_H
#define _LFGBOTSELECTOR_H

#include "Common.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>

class Player;

/**
 * @brief Selects appropriate bots for LFG queue population
 *
 * This selector finds available bots matching specific criteria:
 * - Role requirements (tank, healer, DPS)
 * - Level range
 * - Gear quality/item level
 * - Availability (not in group, queue, dungeon, etc.)
 * - Cooldown status (no deserter debuff)
 * - Geographic location (same continent/zone preferred)
 *
 * Uses intelligent prioritization to select the best bots for each role.
 *
 * NOTE: Provides both instance() singleton access (for state tracking)
 * and static utility methods (for stateless queries) to support Phase 7
 * per-bot architecture while maintaining system-wide bot discovery.
 */
class TC_GAME_API LFGBotSelector final
{
private:
    LFGBotSelector();
    ~LFGBotSelector();

public:
    // Singleton access (for state tracking)
    static LFGBotSelector* instance();

    // Delete copy/move constructors and assignment operators
    LFGBotSelector(LFGBotSelector const&) = delete;
    LFGBotSelector(LFGBotSelector&&) = delete;
    LFGBotSelector& operator=(LFGBotSelector const&) = delete;
    LFGBotSelector& operator=(LFGBotSelector&&) = delete;

    // ========================================================================
    // STATIC UTILITY METHODS (Phase 7 - System-wide bot discovery)
    // ========================================================================
    // These methods provide stateless bot discovery for LFG queue filling.
    // They delegate to the singleton instance for state tracking.

    /**
     * @brief Find available tank bots (static utility)
     *
     * Convenience method for LFG queue population. Finds best available tanks
     * matching criteria and optionally excludes bots already grouped with a human.
     *
     * @param minLevel Minimum level requirement
     * @param maxLevel Maximum level requirement
     * @param count Number of tanks needed
     * @param humanPlayer Optional human player to avoid same-group bots
     * @return Vector of tank bot Player pointers
     */
    static std::vector<Player*> FindAvailableTanks(
        uint8 minLevel,
        uint8 maxLevel,
        uint32 count,
        Player* humanPlayer = nullptr);

    /**
     * @brief Find available healer bots (static utility)
     */
    static std::vector<Player*> FindAvailableHealers(
        uint8 minLevel,
        uint8 maxLevel,
        uint32 count,
        Player* humanPlayer = nullptr);

    /**
     * @brief Find available DPS bots (static utility)
     */
    static std::vector<Player*> FindAvailableDPS(
        uint8 minLevel,
        uint8 maxLevel,
        uint32 count,
        Player* humanPlayer = nullptr);

    // ========================================================================
    // INSTANCE METHODS (Original singleton interface)
    // ========================================================================

    /**
     * @brief Find available tank bots within level range
     *
     * @param minLevel Minimum level requirement
     * @param maxLevel Maximum level requirement
     * @param count Number of tanks needed
     * @return Vector of tank bot Player pointers (may be fewer than requested)
     */
    std::vector<Player*> FindTanks(uint8 minLevel, uint8 maxLevel, uint32 count);

    /**
     * @brief Find available healer bots within level range
     *
     * @param minLevel Minimum level requirement
     * @param maxLevel Maximum level requirement
     * @param count Number of healers needed
     * @return Vector of healer bot Player pointers (may be fewer than requested)
     */
    std::vector<Player*> FindHealers(uint8 minLevel, uint8 maxLevel, uint32 count);

    /**
     * @brief Find available DPS bots within level range
     *
     * @param minLevel Minimum level requirement
     * @param maxLevel Maximum level requirement
     * @param count Number of DPS needed
     * @return Vector of DPS bot Player pointers (may be fewer than requested)
     */
    std::vector<Player*> FindDPS(uint8 minLevel, uint8 maxLevel, uint32 count);

    /**
     * @brief Check if a bot is available for LFG queueing
     *
     * Checks:
     * - Not already in a group
     * - Not already in LFG queue
     * - Not in a dungeon/raid/battleground
     * - No deserter debuff
     * - Not on cooldown
     * - Online and active
     *
     * @param bot The bot to check
     * @return true if available, false otherwise
     */
    bool IsBotAvailable(Player* bot);

    /**
     * @brief Calculate priority score for a bot to fill a specific role
     *
     * Higher scores indicate better suitability. Scoring factors:
     * - Level match to desired level (+100 for exact, -10 per level difference)
     * - Gear quality/item level (+0 to +300)
     * - Role proficiency (+500 if primary role)
     * - Recent activity (-50 if used in last hour, +100 if unused)
     * - Geographic proximity (+50 if same continent)
     *
     * @param bot The bot to score
     * @param desiredRole The role needed (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     * @param desiredLevel The ideal level for the dungeon
     * @return Priority score (higher is better, 0-2000+)
     */
    uint32 CalculateBotPriority(Player* bot, uint8 desiredRole, uint8 desiredLevel);

    /**
     * @brief Set the last queue time for a bot
     *
     * Used to track bot usage and prefer less recently used bots.
     *
     * @param botGuid The bot's GUID
     * @param queueTime The time the bot was queued (timestamp)
     */
    void SetLastQueueTime(ObjectGuid botGuid, time_t queueTime);

    /**
     * @brief Get the last queue time for a bot
     *
     * @param botGuid The bot's GUID
     * @return Last queue timestamp, or 0 if never queued
     */
    time_t GetLastQueueTime(ObjectGuid botGuid);

    /**
     * @brief Clear tracking data for a bot
     *
     * @param botGuid The bot's GUID
     */
    void ClearBotTracking(ObjectGuid botGuid);

    /**
     * @brief Clear all tracking data
     */
    void ClearAllTracking();

private:
    /**
     * @brief Find bots matching specific criteria
     *
     * @param minLevel Minimum level
     * @param maxLevel Maximum level
     * @param desiredRole The role to find (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     * @param count Number of bots needed
     * @return Vector of matching bot Player pointers
     */
    std::vector<Player*> FindBotsForRole(uint8 minLevel, uint8 maxLevel, uint8 desiredRole, uint32 count);

    /**
     * @brief Get all online bots
     *
     * @return Vector of all online bot Player pointers
     */
    std::vector<Player*> GetAllOnlineBots();

    /**
     * @brief Check if bot has deserter debuff
     *
     * @param bot The bot to check
     * @return true if has deserter, false otherwise
     */
    bool HasDeserterDebuff(Player* bot);

    /**
     * @brief Check if bot is in a dungeon, raid, or battleground
     *
     * @param bot The bot to check
     * @return true if in instance, false otherwise
     */
    bool IsInInstance(Player* bot);

    /**
     * @brief Check if bot is in LFG queue or proposal
     *
     * @param bot The bot to check
     * @return true if in LFG, false otherwise
     */
    bool IsInLFG(Player* bot);

    /**
     * @brief Calculate gear score/item level for a bot
     *
     * @param bot The bot to evaluate
     * @return Average item level
     */
    float CalculateItemLevel(Player* bot);

    /**
     * @brief Calculate level difference penalty
     *
     * @param botLevel The bot's level
     * @param desiredLevel The desired level
     * @return Penalty value (0 = perfect match, higher = worse match)
     */
    uint32 CalculateLevelPenalty(uint8 botLevel, uint8 desiredLevel);

    // Data structures

    /**
     * @brief Bot usage tracking information
     */
    struct BotUsageInfo
    {
        time_t lastQueueTime;     ///< Last time bot was queued for LFG
        uint32 totalQueues;        ///< Total number of times queued
        time_t lastDungeonTime;    ///< Last time bot completed a dungeon

        BotUsageInfo() : lastQueueTime(0), totalQueues(0), lastDungeonTime(0) {}
    };

    // Member variables

    /// Map of bot GUID -> usage tracking info
    std::unordered_map<ObjectGuid, BotUsageInfo> _botUsageTracking;

    /// Preferred minimum item level difference threshold
    static constexpr float MIN_ILVL_DIFFERENCE = 5.0f;

    /// Maximum level difference allowed for dungeon suitability
    static constexpr uint8 MAX_LEVEL_DIFFERENCE = 5;

    /// Cooldown time before a bot can be reused (1 hour)
    static constexpr time_t BOT_REUSE_COOLDOWN = 3600;
};

#define sLFGBotSelector LFGBotSelector::instance()

#endif // _LFGBOTSELECTOR_H
