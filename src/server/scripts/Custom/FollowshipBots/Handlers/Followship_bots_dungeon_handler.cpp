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
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Unit.h"
#include "Vehicle.h"

#include "Followship_bots_defines.h"
#include "Followship_bots_dungeon_handler.h"
#include "Followship_bots_combat_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_death_handler.h"
#include "Followship_bots_movement_handler.h"

#include "Custom_Instance_Deadmines.h"

using namespace FSBUtils;
using namespace FSBGroup;

namespace FSBDungeon
{
    // This is the entry point from Periodic Maintence in UpdateAI
    void CheckDungeonHandlingNeeded(Creature* bot)
    {
        if (!bot)
            return;

        if (!FSBDungeon::IsBotInDungeon(bot))
            return;

        // Deadmines
        // Schedule Deadmines Prototype Reaper check if in heroic Deadmines
        if (bot->GetMapId() == Maps::Deadmines && bot->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC)
        {
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_DEADMINES_CHECK_PROTOTYPE_REAPER, 1s);
            return;
        }
    }

    // This is the entry point from ICActions (Main actions during combat state)
    bool CheckDungeonInCombatHandlingNeeded(Creature* bot)
    {
        if (!bot)
            return false;

        if (!FSBDungeon::IsBotInDungeon(bot))
            return false;

        // Check and maintain distance from boss/units in dungeons (for casters)
        Unit* target = bot->GetVictim();
        if (target && target->IsAlive())
        {
            if (FSBUtils::BotIsCasterRole(bot))
            {
                uint32 targetEntry = target->GetEntry();
                if (targetEntry)
                {
                    float targetMinDistance = FSBDungeon::GetDungeonBossMinDistance(targetEntry);

                    bool needsDistance = FSBDungeon::ShouldMaintainBossDistance(bot, target, targetMinDistance);

                    if (needsDistance)
                    {
                        bot->GetMotionMaster()->Clear();
                        bot->GetMotionMaster()->MoveChase(target, ChaseRange(targetMinDistance, FSBCombatUtils::GetBotChaseDistance(bot)));
                        return false;
                    }
                }
            }

            // Dungeon Specific
            // Foe Reaper 5000 Harvest mechanic: tank and melee must back away during Harvest
            if (bot->GetMapId() == Maps::Deadmines && bot->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC)
                if (target && FSBDungeon::HandleFoeReaperAOEEvasion(bot, target))
                    return true;

            return false;
        }

        return false;
    }

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

    bool ShouldMaintainBossDistance(Creature* bot, Unit* target, float minDistance)
    {
        if (!bot || !target)
            return false;

        if (!IsTargetBoss(bot, target))
            return false;

        // GetDistance() subtracts both units' combat reach which makes large bosses trigger this from way too far
        float distance = bot->GetExactDist(target);
        return distance < minDistance;
    }

    float GetDungeonBossMinDistance(uint32 bossEntry)
    {
        if (!bossEntry)
            return DungeonBossMinDistance;

        switch (bossEntry)
        {
        case Scripts::EasternKingdoms::Deadmines::Creatures::FoeReaper5000:
            return Deadmines::FoeReaper5000MinDistance;
        default:
            break;
        }

        return DungeonBossMinDistance;
    }

    bool IsFoeReaperAOEActive(Unit* boss)
    {
        if (!boss)
            return false;
        if (InstanceScript* instance = boss->GetInstanceScript())
            return instance->GetData(Scripts::EasternKingdoms::Deadmines::Misc::FoeReaper5000AOEWarning);
        return false;
    }

    bool HandleFoeReaperAOEEvasion(Creature* bot, Unit* target)
    {
        if (!bot || !target)
            return false;

        // Do not interfere with vehicle combat
        if (bot->GetVehicle())
            return false;

        if (target->GetEntry() != Scripts::EasternKingdoms::Deadmines::Creatures::FoeReaper5000)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI || (baseAI->botRole != FSB_ROLE_TANK && !FSBMgr::BotIsMeleeRole(bot)))
            return false;

        if (IsFoeReaperAOEActive(target))
        {
            if (!baseAI->botDungeonBossEvasion)
            {
                baseAI->botDungeonBossEvasion = true;
                bot->GetMotionMaster()->Clear();
                bot->GetMotionMaster()->MoveChase(target, ChaseRange(Deadmines::FoeReaper5000MinAOEDistance, 20.f));
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleFoeReaperAOEEvasion Bot {} - Harvest/Overdrive active, backing away to {} yd", bot->GetName(), Deadmines::FoeReaper5000MinAOEDistance);
            }
            return true;
        }

        if (baseAI->botDungeonBossEvasion && !IsFoeReaperAOEActive(target))
        {
            baseAI->botDungeonBossEvasion = false;
            bot->GetMotionMaster()->Clear();
            FSBMovement::EnsureInRange(bot, target);
            TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleFoeReaperAOEEvasion Bot {} - Harvest/Overdrive ended, re-engaging melee", bot->GetName());
        }

        return false;
    }

    void CheckAndQueueDeadUnits(Creature* bot, float searchRange)
    {
        if (!bot || !bot->IsAlive())
            return;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return;

        // Check if bot is a resurrect-capable class (healer class)
        if (!BotIsHealerClass(bot))
            return;

        auto& resurrectQueue = baseAI->botResurrectQueue;

        // Build logical bot group (owner, bot, and other bots owned by same player)
        std::vector<Unit*> botGroup;
        BuildLogicalBotGroup(bot, botGroup);

        for (Unit* unit : botGroup)
        {
            if (!unit || unit->IsAlive())
                continue;

            if (bot->GetDistance(unit) > searchRange)
                continue;

            // Check if already in queue
            bool alreadyInQueue = false;
            std::queue<ObjectGuid> tempQueue = resurrectQueue;
            while (!tempQueue.empty())
            {
                if (tempQueue.front() == unit->GetGUID())
                {
                    alreadyInQueue = true;
                    break;
                }
                tempQueue.pop();
            }

            if (alreadyInQueue)
                continue;

            // Add to resurrect queue
            FSBDeath::AddToHealerResurrectQueue(unit, bot);
            TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: CheckAndQueueDeadUnits Bot {} added dead unit {} to resurrect queue", bot->GetName(), unit->GetName());
        }
    }

    namespace Deadmines
    {
        void CheckPrototypeReaperEntry(Creature* bot)
        {
            if (!bot || !bot->IsAlive())
                return;

            auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI)
                return;

            if (baseAI->botInVehicle)
                return;

            if (!FSBMgr::BotIsMeleeRole(bot))
                return;

            Creature* prototypeReaper = bot->FindNearestCreature(Scripts::EasternKingdoms::Deadmines::Creatures::PrototypeReaper, PROTOTYPE_REAPER_RANGE);
            if (!prototypeReaper)
                return;

            if (prototypeReaper->GetVehicleKit() && prototypeReaper->GetVehicleKit()->GetPassenger(0))
                return;

            baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_ENTER_PROTOTYPE_REAPER, 1s, 5s);
        }

        static bool IsAllowedVehicleCastTarget(uint32 entry)
        {
            return entry == Scripts::EasternKingdoms::Deadmines::Creatures::DefiasWatcher ||
                   entry == Scripts::EasternKingdoms::Deadmines::Creatures::DefiasReaper ||
                   entry == Scripts::EasternKingdoms::Deadmines::Creatures::MoltenSlag;
        }

        static void MoveToSlagsPosition(Creature* vehicleCreature)
        {
            vehicleCreature->GetMotionMaster()->Clear();
            vehicleCreature->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_DEADMINES_VEHICLE, PrototypeReaperSlagsPosition);
        }

        static void EngageVehicleTarget(Creature* bot, FSB_BaseAI* baseAI, Creature* vehicleCreature, Unit* target)
        {
            TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleVehicleCombatCheck Bot {} - engaging {} (entry {}), casting Charge and scheduling strikes", bot->GetName(), target->GetName(), target->GetEntry());

            vehicleCreature->GetMotionMaster()->Clear();
            baseAI->botMoveState = FSB_MOVE_STATE_IDLE;
            baseAI->botVehicleCombatTarget = target->GetGUID();

            vehicleCreature->CastSpell(target, Scripts::EasternKingdoms::Deadmines::Spells::PrototypeReaperCharge, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
            baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_REAPER_STRIKE, 2s);
            baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_PRESSURIZED_STRIKE, 4s);
        }

        void HandleVehicleCombatCheck(Creature* bot)
        {
            if (!bot || !bot->IsAlive())
                return;

            auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI)
                return;

            // Loop exit conditions: leave the loop entirely (no reschedule)
            if (!IsBotInDungeon(bot) || bot->GetMapId() != Maps::Deadmines || bot->GetMap()->GetDifficultyID() != DIFFICULTY_HEROIC ||
                !baseAI->botInVehicle || !bot->GetVehicle())
            {
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleVehicleCombatCheck Bot {} - loop conditions no longer met, stopping", bot->GetName());
                baseAI->botVehicleCombatTarget.Clear();
                return;
            }

            Unit* vehicle = bot->GetVehicleBase();
            Creature* vehicleCreature = vehicle ? vehicle->ToCreature() : nullptr;
            if (!vehicleCreature || vehicleCreature->GetEntry() != Scripts::EasternKingdoms::Deadmines::Creatures::PrototypeReaper)
            {
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleVehicleCombatCheck Bot {} - vehicle is not the Prototype Reaper, stopping", bot->GetName());
                baseAI->botVehicleCombatTarget.Clear();
                return;
            }

            Unit* target = FSBCombat::GetNextAttackTarget(bot);

            // Boss fight: park at the designated slags position, scan for Molten Slags from there
            if (target && target->GetEntry() == Scripts::EasternKingdoms::Deadmines::Creatures::FoeReaper5000)
            {
                // Not at the slags position yet: move there first, scan only once arrived
                if (vehicleCreature->GetExactDist2d(&PrototypeReaperSlagsPosition) > 5.0f)
                {
                    TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleVehicleCombatCheck Bot {} - boss phase, moving to slags position", bot->GetName());
                    MoveToSlagsPosition(vehicleCreature);
                    baseAI->botVehicleCombatTarget.Clear();
                    baseAI->botMoveState = FSB_MOVE_STATE_IDLE;
                    baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_VEHICLE_COMBAT_CHECK, 1s);
                    return;
                }

                Unit* slag = vehicleCreature->FindNearestCreature(Scripts::EasternKingdoms::Deadmines::Creatures::MoltenSlag, 100.0f);

                if (slag && slag->IsAlive())
                {
                    EngageVehicleTarget(bot, baseAI, vehicleCreature, slag);
                    return; // cycle continues via EVENT_DM_PRESSURIZED_STRIKE
                }

                // No slags up yet: hold the position, never fall back to following the owner into the boss
                baseAI->botVehicleCombatTarget.Clear();
                baseAI->botMoveState = FSB_MOVE_STATE_IDLE;
                baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_VEHICLE_COMBAT_CHECK, 1s);
                return;
            }

            if (target && target->IsAlive() && IsAllowedVehicleCastTarget(target->GetEntry()))
            {
                EngageVehicleTarget(bot, baseAI, vehicleCreature, target);
                return; // cycle continues via EVENT_DM_PRESSURIZED_STRIKE
            }

            // No valid target: return to following the owner and keep the idle loop running
            baseAI->botVehicleCombatTarget.Clear();

            // Safety: never follow the owner into a nearby Foe Reaper (e.g. owner is meleeing the boss)
            if (Creature* nearbyBoss = vehicleCreature->FindNearestCreature(Scripts::EasternKingdoms::Deadmines::Creatures::FoeReaper5000, BOSS_DISTANCE))
            {
                if (nearbyBoss->IsAlive() && nearbyBoss->IsInCombat())
                {
                    TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleVehicleCombatCheck Bot {} - no target but boss nearby, moving to slags position instead of following", bot->GetName());
                    if (vehicleCreature->GetExactDist2d(&PrototypeReaperSlagsPosition) > 5.0f)
                        MoveToSlagsPosition(vehicleCreature);
                    baseAI->botMoveState = FSB_MOVE_STATE_IDLE;
                    baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_VEHICLE_COMBAT_CHECK, 1s);
                    return;
                }
            }

            Player* owner = FSBMgr::Get()->GetBotOwner(bot);
            if (owner && owner->IsInWorld() && baseAI->botMoveState != FSB_MOVE_STATE_FOLLOWING && vehicleCreature->GetDistance(owner) > 10.0f)
            {
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleVehicleCombatCheck Bot {} - no valid target, returning to follow owner", bot->GetName());
                vehicleCreature->GetMotionMaster()->MoveFollow(owner, 5.0f, PET_FOLLOW_ANGLE);
                baseAI->botMoveState = FSB_MOVE_STATE_FOLLOWING;
            }

            baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_VEHICLE_COMBAT_CHECK, 1s);
        }
    }
}
