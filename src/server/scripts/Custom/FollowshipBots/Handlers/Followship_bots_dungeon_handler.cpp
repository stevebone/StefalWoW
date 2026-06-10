/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Log.h"
#include "Creature.h"
#include "Map.h"

#include "Followship_bots_defines.h"
#include "Followship_bots_dungeon_handler.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_group_handler.h"

namespace FSBDungeon
{
    uint32 GetBotDungeonId(Creature* bot)
    {
        if (!bot)
            return 0;

        if (bot->GetMap()->IsDungeon())
            return bot->GetMapId();

        return 0;
    }

    void UpdateBotDungeonId(Creature* bot)
    {
        if (!bot)
            return;

        uint32 dungeonId = GetBotDungeonId(bot);

        if (FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI()))
        {
            uint32 oldDungeonId = baseAI->DungeonID;
            baseAI->DungeonID = dungeonId;

            if (oldDungeonId != dungeonId)
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: Bot {} dungeon ID updated from {} to {}", 
                    bot->GetName(), oldDungeonId, dungeonId);
        }
    }

    bool IsBotInDungeon(Creature* bot)
    {
        return GetBotDungeonId(bot) != 0;
    }

    bool ShouldPreserveMana(Creature* bot)
    {
        if (!bot)
            return false;

        if (!FSBUtils::BotIsHealerClass(bot))
            return false;

        if (!IsBotInDungeon(bot))
            return false;

        if (bot->GetPowerType() != POWER_MANA)
            return false;

        float manaPct = (float)bot->GetPower(POWER_MANA) / (float)bot->GetMaxPower(POWER_MANA) * 100.0f;
        return manaPct < 70.0f;
    }

    bool IsTargetBoss(Creature* bot, Unit* target)
    {
        if (!bot || !target)
            return false;

        if (!target->ToCreature())
            return false;

        uint32 dungeonId = GetBotDungeonId(bot);
        if (dungeonId == 0)
            return false;

        uint32 targetEntry = target->GetEntry();

        if (dungeonId == Maps::Deadmines)
        {
            for (uint32 bossId : Creatures::Deadmines::BossIds)
                if (targetEntry == bossId)
                    return true;
        }

        return false;
    }

    bool ShouldDPSInExecutePhase(Creature* bot, Unit* target)
    {
        if (!bot || !target)
            return false;

        if (!FSBUtils::BotIsHealerClass(bot))
            return false;

        if (!IsBotInDungeon(bot))
            return false;

        if (!IsTargetBoss(bot, target))
            return false;

        if (!FSBGroup::BotGroupIsHealthy_Average(bot, 50))
            return false;

        float bossHpPct = target->GetHealthPct();
        return bossHpPct < 15.0f;
    }
}
