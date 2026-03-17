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

#include "LFGBotSelector.h"
#include "LFGRoleDetector.h"
#include "LFGBotManager.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "Group.h"
#include "LFGMgr.h"
#include "LFG.h"
#include "Log.h"
#include "Map.h"
#include "SpellAuras.h"
#include "../../Core/PlayerBotHooks.h"
#include "../../Session/BotWorldSessionMgr.h"
#include <algorithm>

LFGBotSelector::LFGBotSelector()
{
}

LFGBotSelector::~LFGBotSelector()
{
}

LFGBotSelector* LFGBotSelector::instance()
{
    static LFGBotSelector instance;
    return &instance;
}

// ============================================================================
// STATIC UTILITY METHODS (Phase 7 - System-wide bot discovery)
// ============================================================================

std::vector<Player*> LFGBotSelector::FindAvailableTanks(
    uint8 minLevel,
    uint8 maxLevel,
    uint32 count,
    Player* humanPlayer)
{
    // Request extra bots to account for faction filtering (request 10x or at least 50)
    uint32 requestCount = std::max(count * 10, 50u);
    std::vector<Player*> tanks = instance()->FindTanks(minLevel, maxLevel, requestCount);
    size_t beforeFactionFilter = tanks.size();

    if (humanPlayer)
    {
        Team humanFaction = humanPlayer->GetTeam();

        // Filter by faction - bots must be same faction as human player
        tanks.erase(
            std::remove_if(tanks.begin(), tanks.end(),
                [humanFaction](Player* bot) {
                    return bot->GetTeam() != humanFaction;
                }),
            tanks.end());

        TC_LOG_INFO("module.playerbot.lfg", "FindAvailableTanks: Human {} faction={}, before filter={}, after faction filter={}",
                    humanPlayer->GetName(), humanFaction == HORDE ? "HORDE" : "ALLIANCE", beforeFactionFilter, tanks.size());

        // Filter out bots already grouped with the human player
        if (humanPlayer->GetGroup())
        {
            ObjectGuid humanGroupGuid = humanPlayer->GetGroup()->GetGUID();
            tanks.erase(
                std::remove_if(tanks.begin(), tanks.end(),
                    [humanGroupGuid](Player* bot) {
                        Group* botGroup = bot->GetGroup();
                        return botGroup && botGroup->GetGUID() == humanGroupGuid;
                    }),
                tanks.end());
        }
    }

    // Limit to requested count after faction filtering
    if (tanks.size() > count)
        tanks.resize(count);

    return tanks;
}

std::vector<Player*> LFGBotSelector::FindAvailableHealers(
    uint8 minLevel,
    uint8 maxLevel,
    uint32 count,
    Player* humanPlayer)
{
    // Request extra bots to account for faction filtering (request 10x or at least 50)
    uint32 requestCount = std::max(count * 10, 50u);
    std::vector<Player*> healers = instance()->FindHealers(minLevel, maxLevel, requestCount);
    size_t beforeFactionFilter = healers.size();

    if (humanPlayer)
    {
        Team humanFaction = humanPlayer->GetTeam();

        // Filter by faction - bots must be same faction as human player
        healers.erase(
            std::remove_if(healers.begin(), healers.end(),
                [humanFaction](Player* bot) {
                    return bot->GetTeam() != humanFaction;
                }),
            healers.end());

        TC_LOG_INFO("module.playerbot.lfg", "FindAvailableHealers: Human {} faction={}, before filter={}, after faction filter={}",
                    humanPlayer->GetName(), humanFaction == HORDE ? "HORDE" : "ALLIANCE", beforeFactionFilter, healers.size());

        // Filter out bots already grouped with the human player
        if (humanPlayer->GetGroup())
        {
            ObjectGuid humanGroupGuid = humanPlayer->GetGroup()->GetGUID();
            healers.erase(
                std::remove_if(healers.begin(), healers.end(),
                    [humanGroupGuid](Player* bot) {
                        Group* botGroup = bot->GetGroup();
                        return botGroup && botGroup->GetGUID() == humanGroupGuid;
                    }),
                healers.end());
        }
    }

    // Limit to requested count after faction filtering
    if (healers.size() > count)
        healers.resize(count);

    return healers;
}

std::vector<Player*> LFGBotSelector::FindAvailableDPS(
    uint8 minLevel,
    uint8 maxLevel,
    uint32 count,
    Player* humanPlayer)
{
    // Request extra bots to account for faction filtering (request 10x or at least 50)
    uint32 requestCount = std::max(count * 10, 50u);
    std::vector<Player*> dps = instance()->FindDPS(minLevel, maxLevel, requestCount);
    size_t beforeFactionFilter = dps.size();

    if (humanPlayer)
    {
        Team humanFaction = humanPlayer->GetTeam();

        // Filter by faction - bots must be same faction as human player
        dps.erase(
            std::remove_if(dps.begin(), dps.end(),
                [humanFaction](Player* bot) {
                    return bot->GetTeam() != humanFaction;
                }),
            dps.end());

        TC_LOG_INFO("module.playerbot.lfg", "FindAvailableDPS: Human {} faction={}, before filter={}, after faction filter={}",
                    humanPlayer->GetName(), humanFaction == HORDE ? "HORDE" : "ALLIANCE", beforeFactionFilter, dps.size());

        // Filter out bots already grouped with the human player
        if (humanPlayer->GetGroup())
        {
            ObjectGuid humanGroupGuid = humanPlayer->GetGroup()->GetGUID();
            dps.erase(
                std::remove_if(dps.begin(), dps.end(),
                    [humanGroupGuid](Player* bot) {
                        Group* botGroup = bot->GetGroup();
                        return botGroup && botGroup->GetGUID() == humanGroupGuid;
                    }),
                dps.end());
        }
    }

    // Limit to requested count after faction filtering
    if (dps.size() > count)
        dps.resize(count);

    return dps;
}

// ============================================================================
// INSTANCE METHODS (Original singleton interface)
// ============================================================================

std::vector<Player*> LFGBotSelector::FindTanks(uint8 minLevel, uint8 maxLevel, uint32 count)
{
    return FindBotsForRole(minLevel, maxLevel, lfg::PLAYER_ROLE_TANK, count);
}

std::vector<Player*> LFGBotSelector::FindHealers(uint8 minLevel, uint8 maxLevel, uint32 count)
{
    return FindBotsForRole(minLevel, maxLevel, lfg::PLAYER_ROLE_HEALER, count);
}

std::vector<Player*> LFGBotSelector::FindDPS(uint8 minLevel, uint8 maxLevel, uint32 count)
{
    return FindBotsForRole(minLevel, maxLevel, lfg::PLAYER_ROLE_DAMAGE, count);
}

bool LFGBotSelector::IsBotAvailable(Player* bot)
{
    if (!bot)
        return false;

    // Must be a bot
    if (!Playerbot::PlayerBotHooks::IsPlayerBot(bot))
        return false;
    // Must be online (have a valid session)
    if (!bot->GetSession())
        return false;

    // Minimum level 10 required for LFG (same as retail)
    // This prevents low-level bots (like Death Knights in starting zone) from joining
    constexpr uint8 MIN_LFG_LEVEL = 10;
    if (bot->GetLevel() < MIN_LFG_LEVEL)
    {
        TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::IsBotAvailable - Bot {} is level {} (minimum {} required for LFG)",
                     bot->GetName(), bot->GetLevel(), MIN_LFG_LEVEL);
        return false;
    }

    // Must not be in a group
    if (bot->GetGroup())
    {
        TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::IsBotAvailable - Bot {} is in a group", bot->GetName());
        return false;
    }

    // Must not be in LFG already
    if (IsInLFG(bot))
    {
        TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::IsBotAvailable - Bot {} is already in LFG", bot->GetName());
        return false;
    }

    // Must not have deserter debuff
    if (HasDeserterDebuff(bot))
    {
        TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::IsBotAvailable - Bot {} has deserter debuff", bot->GetName());
        return false;
    }
    // Must not be in an instance (except if it's the normal world)
    if (IsInInstance(bot))
    {
        TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::IsBotAvailable - Bot {} is in an instance", bot->GetName());
        return false;
    }

    // Must not be dead
    if (!bot->IsAlive())
    {
        TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::IsBotAvailable - Bot {} is dead", bot->GetName());
        return false;
    }

    // Must not be in combat
    if (bot->IsInCombat())
    {
        TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::IsBotAvailable - Bot {} is in combat", bot->GetName());
        return false;
    }

    // Check if bot is already queued via our manager
    if (sLFGBotManager->IsBotQueued(bot->GetGUID()))
    {
        TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::IsBotAvailable - Bot {} is already queued via bot manager", bot->GetName());
        return false;
    }

    return true;
}

uint32 LFGBotSelector::CalculateBotPriority(Player* bot, uint8 desiredRole, uint8 desiredLevel)
{
    if (!bot)
        return 0;

    uint32 priority = 1000; // Base priority

    // Level matching
    uint8 botLevel = bot->GetLevel();
    uint32 levelPenalty = CalculateLevelPenalty(botLevel, desiredLevel);

    if (levelPenalty == 0)
        priority += 100; // Perfect level match
    else
        priority -= std::min(levelPenalty * 10, 500u); // Penalty for level difference

    // Gear quality
    float itemLevel = CalculateItemLevel(bot);
    priority += static_cast<uint32>(itemLevel); // +0 to +300 typically

    // Role proficiency - check if this is the bot's primary role
    uint8 botRole = sLFGRoleDetector->DetectBotRole(bot);
    if (botRole == desiredRole)
        priority += 500; // Significant bonus for primary role match
    else if (sLFGRoleDetector->CanPerformRole(bot, desiredRole))
        priority += 100; // Small bonus if bot can perform role but it's not primary

    // Recent activity - prefer bots that haven't been used recently
    time_t lastQueueTime = GetLastQueueTime(bot->GetGUID());
    if (lastQueueTime > 0)
    {
        time_t timeSinceQueue = time(nullptr) - lastQueueTime;
        if (timeSinceQueue < BOT_REUSE_COOLDOWN)
        {
            // Recently used, apply penalty
            priority = (priority > 200) ? priority - 200 : 0;
        }
        else
        {
            // Not recently used, apply bonus
            priority += 100;
        }
    }
    else
    {
        // Never used, give bonus
        priority += 150;
    }

    // Geographic proximity - bots on the same continent are preferred
    // This helps reduce teleportation distance
    // Note: This is a simplified check - real implementation would compare with
    // the human player's location
    Map* botMap = bot->GetMap();
    if (botMap && !botMap->IsDungeon() && !botMap->IsRaid() && !botMap->IsBattleground())
    {
        priority += 50; // Bonus for being in the normal world
    }

    TC_LOG_TRACE("module.playerbot.lfg", "LFGBotSelector::CalculateBotPriority - Bot {} priority: {} (Level: {}, Role: {}, ItemLevel: {})",
                 bot->GetName(), priority, botLevel, desiredRole, itemLevel);

    return priority;
}

void LFGBotSelector::SetLastQueueTime(ObjectGuid botGuid, time_t queueTime)
{
    auto& usage = _botUsageTracking[botGuid];
    usage.lastQueueTime = queueTime;
    usage.totalQueues++;
}

time_t LFGBotSelector::GetLastQueueTime(ObjectGuid botGuid)
{
    auto itr = _botUsageTracking.find(botGuid);
    if (itr != _botUsageTracking.end())
        return itr->second.lastQueueTime;

    return 0;
}

void LFGBotSelector::ClearBotTracking(ObjectGuid botGuid)
{
    _botUsageTracking.erase(botGuid);
}

void LFGBotSelector::ClearAllTracking()
{
    _botUsageTracking.clear();
}

std::vector<Player*> LFGBotSelector::FindBotsForRole(uint8 minLevel, uint8 maxLevel, uint8 desiredRole, uint32 count)
{
    std::vector<Player*> result;

    if (count == 0)
        return result;

    // Enforce minimum level 10 for LFG (same as retail)
    // This prevents low-level bots (like Death Knights in starting zone) from joining
    constexpr uint8 MIN_LFG_LEVEL = 10;
    if (minLevel < MIN_LFG_LEVEL)
        minLevel = MIN_LFG_LEVEL;

    // Get all online bots
    std::vector<Player*> allBots = GetAllOnlineBots();

    TC_LOG_INFO("module.playerbot.lfg", "LFGBotSelector::FindBotsForRole - Searching {} online bots for role {} (level {}-{})",
                allBots.size(), desiredRole, minLevel, maxLevel);

    // Structure to hold bot and priority
    struct BotPriority
    {
        Player* bot;
        uint32 priority;

        bool operator<(BotPriority const& other) const
        {
            return priority > other.priority; // Higher priority first
        }
    };

    std::vector<BotPriority> candidates;

    // Calculate midpoint level for priority calculation
    uint8 idealLevel = (minLevel + maxLevel) / 2;

    // Diagnostic counters
    uint32 filteredByAvailability = 0;
    uint32 filteredByLevel = 0;
    uint32 filteredByRole = 0;

    // Detailed availability counters
    uint32 failedGroup = 0, failedLFG = 0, failedDeserter = 0;
    uint32 failedInstance = 0, failedDead = 0, failedCombat = 0, failedQueued = 0;

    // Filter and score bots
    for (Player* bot : allBots)
    {
        // Check basic availability with detailed counters
        if (bot->GetGroup()) { ++failedGroup; ++filteredByAvailability; continue; }
        if (IsInLFG(bot)) { ++failedLFG; ++filteredByAvailability; continue; }
        if (HasDeserterDebuff(bot)) { ++failedDeserter; ++filteredByAvailability; continue; }
        if (IsInInstance(bot)) { ++failedInstance; ++filteredByAvailability; continue; }
        if (!bot->IsAlive()) { ++failedDead; ++filteredByAvailability; continue; }
        if (bot->IsInCombat()) { ++failedCombat; ++filteredByAvailability; continue; }
        if (sLFGBotManager->IsBotQueued(bot->GetGUID())) { ++failedQueued; ++filteredByAvailability; continue; }

        // Check level range
        uint8 botLevel = bot->GetLevel();
        if (botLevel < minLevel || botLevel > maxLevel)
        {
            TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::FindBotsForRole - Bot {} level {} outside range {}-{}",
                        bot->GetName(), botLevel, minLevel, maxLevel);
            ++filteredByLevel;
            continue;
        }

        // Check if bot can perform the desired role
        if (!sLFGRoleDetector->CanPerformRole(bot, desiredRole))
        {
            TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::FindBotsForRole - Bot {} cannot perform role {}",
                        bot->GetName(), desiredRole);
            ++filteredByRole;
            continue;
        }

        // Calculate priority
        uint32 priority = CalculateBotPriority(bot, desiredRole, idealLevel);
        candidates.push_back({ bot, priority });
    }

    TC_LOG_INFO("module.playerbot.lfg", "LFGBotSelector::FindBotsForRole - Filter results: {} candidates, filtered by: availability={}, level={}, role={}",
                candidates.size(), filteredByAvailability, filteredByLevel, filteredByRole);
    TC_LOG_INFO("module.playerbot.lfg", "LFGBotSelector::FindBotsForRole - Availability breakdown: group={}, lfg={}, deserter={}, instance={}, dead={}, combat={}, queued={}",
                failedGroup, failedLFG, failedDeserter, failedInstance, failedDead, failedCombat, failedQueued);

    // Sort by priority (highest first)
    std::sort(candidates.begin(), candidates.end());

    // Select top 'count' bots
    uint32 selected = 0;
    for (auto const& candidate : candidates)
    {
        if (selected >= count)
            break;

        result.push_back(candidate.bot);

        // Track this bot's selection
        SetLastQueueTime(candidate.bot->GetGUID(), time(nullptr));

        selected++;
    }

    TC_LOG_DEBUG("module.playerbot.lfg", "LFGBotSelector::FindBotsForRole - Found {}/{} bots for role {} (level {}-{})",
                 result.size(), count, desiredRole, minLevel, maxLevel);

    return result;
}

std::vector<Player*> LFGBotSelector::GetAllOnlineBots()
{
    // CRITICAL FIX: Bot sessions are NOT in sWorld->GetAllSessions()!
    // They are managed separately by BotWorldSessionMgr.
    // Use the new GetAllBotPlayers() method which iterates through _botSessions.
    std::vector<Player*> bots = Playerbot::sBotWorldSessionMgr->GetAllBotPlayers();

    TC_LOG_INFO("module.playerbot.lfg", "LFGBotSelector::GetAllOnlineBots - Found {} bots via BotWorldSessionMgr",
                bots.size());

    return bots;
}

bool LFGBotSelector::HasDeserterDebuff(Player* bot)
{
    if (!bot)
        return false;

    // Check for LFG deserter debuff
    return bot->HasAura(lfg::LFG_SPELL_DUNGEON_DESERTER);
}

bool LFGBotSelector::IsInInstance(Player* bot)
{
    if (!bot)
        return false;

    Map* map = bot->GetMap();
    if (!map)
        return false;

    // Consider bot unavailable if in dungeon, raid, or battleground
    return map->IsDungeon() || map->IsRaid() || map->IsBattleground();
}

bool LFGBotSelector::IsInLFG(Player* bot)
{
    if (!bot)
        return false;

    lfg::LfgState state = sLFGMgr->GetState(bot->GetGUID());
    // Bot is in LFG if queued, in proposal, or in role check
    return state == lfg::LFG_STATE_QUEUED ||
           state == lfg::LFG_STATE_PROPOSAL ||
           state == lfg::LFG_STATE_ROLECHECK ||
           state == lfg::LFG_STATE_DUNGEON;
}

float LFGBotSelector::CalculateItemLevel(Player* bot)
{
    if (!bot)
        return 0.0f;

    // Use TrinityCore's built-in average item level calculation
    return bot->GetAverageItemLevel();
}

uint32 LFGBotSelector::CalculateLevelPenalty(uint8 botLevel, uint8 desiredLevel)
{
    if (botLevel == desiredLevel)
        return 0;

    uint32 difference = std::abs(static_cast<int32>(botLevel) - static_cast<int32>(desiredLevel));

    // Penalize level differences more heavily if outside reasonable range
    if (difference > MAX_LEVEL_DIFFERENCE)
    {
        // Exponential penalty for being way off level
        return difference * difference;
    }

    return difference;
}
