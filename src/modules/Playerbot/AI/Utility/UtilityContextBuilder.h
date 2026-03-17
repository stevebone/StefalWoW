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

#ifndef TRINITYCORE_UTILITY_CONTEXT_BUILDER_H
#define TRINITYCORE_UTILITY_CONTEXT_BUILDER_H

#include "UtilitySystem.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Group.h"
#include "Unit.h"
#include "GridNotifiers.h"
#include "ThreatManager.h"
#include <shared_mutex>

namespace Playerbot
{

class SharedBlackboard;

/**
 * @brief Builds utility context from game world state
 * Extracts relevant information from bot, group, and world for decision-making
 */
class TC_GAME_API UtilityContextBuilder
{
public:
    /**
     * @brief Build context from current game state
     * @param ai Bot AI instance
     * @param blackboard Shared blackboard (can be nullptr)
     * @return Populated utility context
     */
    static UtilityContext Build(BotAI* ai, SharedBlackboard* blackboard = nullptr)
    {
        UtilityContext context;
        context.bot = ai;
        context.blackboard = blackboard;

        if (!ai)
            return context;

        Player* bot = ai->GetBot();
        if (!bot || !bot->IsInWorld())
            return context;

        // Bot state
        context.healthPercent = bot->GetHealthPct() / 100.0f;

        // Mana percentage (handle power types correctly)
        // CRITICAL: GetMaxPower() can crash if bot's power systems aren't ready
        int32 maxMana = bot->GetMaxPower(POWER_MANA);
        if (maxMana > 0)
            context.manaPercent = bot->GetPower(POWER_MANA) / static_cast<float>(maxMana);
        else
            context.manaPercent = 1.0f; // Non-mana classes always at "full" mana

        context.inCombat = bot->IsInCombat();
        context.hasAggro = HasAggro(bot);

        // Group state
        Group* group = bot->GetGroup();
        context.inGroup = (group != nullptr);
        context.groupSize = group ? group->GetMembersCount() : 1;
        context.lowestAllyHealthPercent = GetLowestAllyHealth(bot, group);
        context.enemiesInRange = CountEnemiesInRange(bot, 40.0f);

        // Role detection
        context.role = DetectRole(bot);

        // Timing
        context.timeSinceCombatStart = GetTimeSinceCombatStart(ai);
        context.lastDecisionTime = GameTime::GetGameTimeMS();

        return context;
    }

private:
    /**
     * @brief Check if bot currently has aggro
     * Uses ThreatManager to accurately check if bot is being targeted
     */
    static bool HasAggro(Player* bot)
    {
        if (!bot || !bot->IsInCombat())
            return false;

        // Check if bot has any attackers via direct combat check
        Unit* victim = bot->GetVictim();
        if (victim && victim->GetVictim() == bot)
            return true;

        // Check threat tables of nearby hostile units using ThreatManager
        // Find any unit that has bot at the top of their threat list
        ::std::list<Unit*> targets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, 40.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, targets, u_check);
        Cell::VisitAllObjects(bot, searcher, 40.0f);

        for (Unit* hostile : targets)
        {
            if (!hostile || hostile->isDead())
                continue;

            // Check if hostile is targeting the bot using ThreatManager
            ThreatManager& threatMgr = hostile->GetThreatManager();
            Unit* topThreat = hostile->GetVictim();

            if (topThreat && topThreat == bot)
                return true;

            // Also check if bot has significant threat on this target
            float botThreat = threatMgr.GetThreat(bot);
            if (topThreat)
            {
                float topThreatValue = threatMgr.GetThreat(topThreat);
                // Bot has aggro if it's at 90%+ of top threat (melee range threshold)
                if (topThreatValue > 0.0f && (botThreat / topThreatValue) >= 0.9f)
                    return true;
            }
        }

        return false;
    }

    /**
     * @brief Get lowest health percentage among group members
     */
    static float GetLowestAllyHealth(Player* bot, Group* group)
    {
        if (!group)
            return bot ? (bot->GetHealthPct() / 100.0f) : 1.0f;

        float lowest = 1.0f;

        for (GroupReference& ref : group->GetMembers())
        {
            Player* member = ref.GetSource();
            if (!member || !member->IsInWorld() || member->isDead())
                continue;

            float healthPct = member->GetHealthPct() / 100.0f;
            if (healthPct < lowest)
                lowest = healthPct;
        }

        return lowest;
    }

    /**
     * @brief Count enemies within specified range
     */
    static uint32 CountEnemiesInRange(Player* bot, float range)
    {
        if (!bot)
            return 0;

        // Use Trinity's SearcherInRange to find nearby hostile units
        ::std::list<Unit*> targets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, range);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, targets, u_check);
        Cell::VisitAllObjects(bot, searcher, range);

        return static_cast<uint32>(targets.size());
    }

    /**
     * @brief Detect bot's role based on class, spec, and talents
     */
    static UtilityContext::Role DetectRole(Player* bot)
    {
        if (!bot)
            return UtilityContext::Role::DPS;

        uint8 classId = bot->GetClass();
        uint8 spec = uint8(bot->GetPrimarySpecialization());

        // Tank specs
        if (classId == CLASS_WARRIOR && spec == 2) // Protection
            return UtilityContext::Role::TANK;
        if (classId == CLASS_PALADIN && spec == 1) // Protection
            return UtilityContext::Role::TANK;
        if (classId == CLASS_DEATH_KNIGHT && spec == 0) // Blood
            return UtilityContext::Role::TANK;
        if (classId == CLASS_DRUID && spec == 1) // Feral (Bear form)
            return UtilityContext::Role::TANK;

        // Healer specs
        if (classId == CLASS_PRIEST && (spec == 1 || spec == 2)) // Discipline or Holy
            return UtilityContext::Role::HEALER;
        if (classId == CLASS_PALADIN && spec == 0) // Holy
            return UtilityContext::Role::HEALER;
        if (classId == CLASS_SHAMAN && spec == 2) // Restoration
            return UtilityContext::Role::HEALER;
        if (classId == CLASS_DRUID && spec == 2) // Restoration
            return UtilityContext::Role::HEALER;

        // Support (some specs can be hybrid)
        if (classId == CLASS_SHAMAN && spec == 1) // Enhancement (can off-heal)
            return UtilityContext::Role::SUPPORT;
        if (classId == CLASS_DRUID && spec == 1) // Feral (can off-tank/DPS)
            return UtilityContext::Role::SUPPORT;

        // Default: DPS
        return UtilityContext::Role::DPS;
    }

    /**
     * @brief Get time since combat started (in milliseconds)
     * Uses static tracking per player GUID to monitor combat state transitions
     * Thread-safe: Uses shared_mutex for concurrent access from ThreadPool workers
     */
    static uint32 GetTimeSinceCombatStart(BotAI* ai)
    {
        if (!ai)
            return 0;

        Player* bot = ai->GetBot();
        if (!bot)
            return 0;

        // Static storage for combat start times (thread-safe)
        // Map player GUID -> combat start time
        static ::std::unordered_map<ObjectGuid, uint32> combatStartTimes;
        static ::std::shared_mutex combatStartTimesMutex;

        ObjectGuid botGuid = bot->GetGUID();
        uint32 currentTime = GameTime::GetGameTimeMS();

        if (!bot->IsInCombat())
        {
            // Not in combat - clear tracking entry (exclusive lock for write)
            ::std::unique_lock lock(combatStartTimesMutex);
            combatStartTimes.erase(botGuid);
            return 0;
        }

        // In combat - check if we have a start time recorded (shared lock for read)
        {
            ::std::shared_lock readLock(combatStartTimesMutex);
            auto it = combatStartTimes.find(botGuid);
            if (it != combatStartTimes.end())
            {
                // Calculate duration since combat start
                uint32 startTime = it->second;
                return currentTime >= startTime ? (currentTime - startTime) : 0;
            }
        }

        // Just entered combat - record start time (exclusive lock for write)
        {
            ::std::unique_lock writeLock(combatStartTimesMutex);
            // Double-check after acquiring write lock (another thread may have inserted)
            auto it = combatStartTimes.find(botGuid);
            if (it == combatStartTimes.end())
            {
                combatStartTimes[botGuid] = currentTime;
                return 0;  // Just started, 0 duration
            }
            // Another thread inserted while we were waiting for lock
            uint32 startTime = it->second;
            return currentTime >= startTime ? (currentTime - startTime) : 0;
        }
    }
};

} // namespace Playerbot

#endif // TRINITYCORE_UTILITY_CONTEXT_BUILDER_H
