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

#include "Followship_bots_defines.h"
#include "Followship_bots_dungeon_deadmines.h"
#include "Followship_bots_dungeon_handler.h"
#include "Followship_bots_combat_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_movement_handler.h"

#include "Custom_Instance_Deadmines.h"

namespace DM = Scripts::EasternKingdoms::Deadmines;

namespace FSBDeadmines
{
    bool Handle(Creature* bot, Unit* target)
    {
        if (!bot)
            return false;

        if (bot->GetMapId() != FSBDungeon::Maps::Deadmines)
            return false;

        InstanceScript* instance = bot->GetInstanceScript();
        if (!instance)
            return false;

        FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        // Heroic-only encounter: Foe Reaper 5000
        if (bot->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC)
        {
            if (instance->GetBossState(DM::DataTypes::BOSS_FOE_REAPER_5000) == IN_PROGRESS)
            {
                baseAI->GetDungeonData()->SetActiveEncounter(DM::DataTypes::BOSS_FOE_REAPER_5000);
                return Encounters::HandleFoeReaper5000(bot, target);
            }
        }

        // Captain Cookie (any difficulty)
        if (instance->GetBossState(DM::DataTypes::BOSS_CAPTAIN_COOKIE) == IN_PROGRESS)
        {
            baseAI->GetDungeonData()->SetActiveEncounter(DM::DataTypes::BOSS_CAPTAIN_COOKIE);
            return Encounters::HandleCaptainCookie(bot, target);
        }

        // If no tracked deadmines encounter is active, clear state
        if (baseAI->GetDungeonData()->activeEncounterId != 0)
            baseAI->GetDungeonData()->Reset();

        return false;
    }

    void HandleOutOfCombat(Creature* bot)
    {
        if (!bot)
            return;

        if (bot->GetMapId() != FSBDungeon::Maps::Deadmines)
            return;

        InstanceScript* instance = bot->GetInstanceScript();
        if (!instance)
            return;

        FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return;

        // Heroic-only: schedule Prototype Reaper check
        if (bot->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC)
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_DEADMINES_CHECK_PROTOTYPE_REAPER, 1s);

        // Captain Cookie: schedule food cycle when boss becomes active
        if (instance->GetBossState(DM::DataTypes::BOSS_CAPTAIN_COOKIE) == IN_PROGRESS)
        {
            if (baseAI->GetDungeonData()->mechanicCounterA == 0 && !baseAI->GetDungeonData()->mechanicFlagB)
            {
                baseAI->GetDungeonData()->mechanicFlagB = true;
                baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_COOKIE_FOOD_CYCLE, 1s);
            }
            else if (!bot->IsInCombat() && baseAI->GetDungeonData()->mechanicCounterA > 0)
            {
                Encounters::ProcessCaptainCookieFoodClick(bot);
            }
        }
        else
        {
            // Boss not in progress; clear cookie state (ResumeFollow is handled generically)
            if (baseAI->GetDungeonData()->mechanicCounterA > 0 ||
                baseAI->GetDungeonData()->mechanicFlagA ||
                baseAI->GetDungeonData()->mechanicFlagB)
            {
                baseAI->GetDungeonData()->Reset();
            }
        }
    }

    namespace Encounters
    {
        static bool IsFoeReaperAOEActive(Unit* boss)
        {
            if (!boss)
                return false;
            if (InstanceScript* instance = boss->GetInstanceScript())
                return instance->GetData(DM::Misc::FoeReaper5000AOEWarning);
            return false;
        }

        bool HandleFoeReaper5000(Creature* bot, Unit* target)
        {
            if (!bot || !target)
                return false;

            // Do not interfere with vehicle combat
            if (bot->GetVehicle())
                return false;

            if (target->GetEntry() != DM::Creatures::FoeReaper5000)
                return false;

            FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI || (baseAI->botRole != FSB_ROLE_TANK && !FSBMgr::BotIsMeleeRole(bot)))
                return false;

            if (IsFoeReaperAOEActive(target))
            {
                if (!baseAI->GetDungeonData()->mechanicFlagC)
                {
                    baseAI->GetDungeonData()->mechanicFlagC = true;
                    bot->GetMotionMaster()->Clear();
                    bot->GetMotionMaster()->MoveChase(target, ChaseRange(FSBDungeon::Deadmines::FoeReaper5000MinAOEDistance, 20.f));
                    TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleFoeReaper5000 Bot {} - Harvest/Overdrive active, backing away to {} yd", bot->GetName(), FSBDungeon::Deadmines::FoeReaper5000MinAOEDistance);
                }
                return true;
            }

            if (baseAI->GetDungeonData()->mechanicFlagC && !IsFoeReaperAOEActive(target))
            {
                baseAI->GetDungeonData()->mechanicFlagC = false;
                bot->GetMotionMaster()->Clear();
                FSBMovement::EnsureInRange(bot, target);
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleFoeReaper5000 Bot {} - Harvest/Overdrive ended, re-engaging melee", bot->GetName());
            }

            return false;
        }

        static constexpr float COOKIE_FOOD_SEARCH_RADIUS = 30.0f;
        static constexpr float COOKIE_FOOD_SPELLCLICK_RANGE = 5.0f;
        static constexpr float COOKIE_BAD_FOOD_AVOID_DISTANCE = 10.0f;

        bool HandleCaptainCookie(Creature* bot, Unit* target)
        {
            if (!bot)
                return false;

            FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI)
                return false;

            // Defensive gate: only run when cookie is the active encounter
            if (baseAI->GetDungeonData()->activeEncounterId != DM::DataTypes::BOSS_CAPTAIN_COOKIE)
                return false;

            // Process food clicks first (works both IC and when target is null during OOC continuation)
            if (ProcessCaptainCookieFoodClick(bot))
                return true;

            // Non-tank bad food avoidance
            if (target && target->IsAlive() && baseAI->botRole != FSB_ROLE_TANK)
            {
                static constexpr uint32 badFoodEntries[] =
                {
                    DM::Creatures::RottenCorn,
                    DM::Creatures::RottenMelon,
                    DM::Creatures::RottenSteak,
                    DM::Creatures::RottenMysteryMeat,
                    DM::Creatures::RottenLoaf,
                    DM::Creatures::RottenBun
                };

                for (uint32 entry : badFoodEntries)
                {
                    if (Creature* badFood = bot->FindNearestCreature(entry, COOKIE_BAD_FOOD_AVOID_DISTANCE))
                    {
                        if (badFood->IsAlive())
                        {
                            float angle = bot->GetAbsoluteAngle(badFood);
                            Position pos = bot->GetNearPosition(COOKIE_BAD_FOOD_AVOID_DISTANCE + 2.0f, angle + static_cast<float>(M_PI));
                            bot->GetMotionMaster()->Clear();
                            bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_COOKIE_FOOD, pos);
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        Creature* FindNearestCookieFood(Creature* bot, bool searchBadFood)
        {
            if (!bot)
                return nullptr;

            if (searchBadFood)
            {
                static constexpr uint32 badFoodEntries[] =
                {
                    DM::Creatures::RottenCorn,
                    DM::Creatures::RottenMelon,
                    DM::Creatures::RottenSteak,
                    DM::Creatures::RottenMysteryMeat,
                    DM::Creatures::RottenLoaf,
                    DM::Creatures::RottenBun
                };

                Creature* nearest = nullptr;
                float nearestDist = COOKIE_FOOD_SEARCH_RADIUS;
                for (uint32 entry : badFoodEntries)
                {
                    if (Creature* food = bot->FindNearestCreature(entry, COOKIE_FOOD_SEARCH_RADIUS))
                    {
                        if (food->IsAlive())
                        {
                            float dist = bot->GetDistance(food);
                            if (dist < nearestDist)
                            {
                                nearestDist = dist;
                                nearest = food;
                            }
                        }
                    }
                }
                return nearest;
            }
            else
            {
                static constexpr uint32 goodFoodEntries[] =
                {
                    DM::Creatures::GoodFoodCorn,
                    DM::Creatures::GoodFoodMelon,
                    DM::Creatures::GoodFoodSteak,
                    DM::Creatures::GoodFoodMysteryMeat,
                    DM::Creatures::GoodFoodLoaf,
                    DM::Creatures::GoodFoodBun
                };

                Creature* nearest = nullptr;
                float nearestDist = COOKIE_FOOD_SEARCH_RADIUS;
                for (uint32 entry : goodFoodEntries)
                {
                    if (Creature* food = bot->FindNearestCreature(entry, COOKIE_FOOD_SEARCH_RADIUS))
                    {
                        if (food->IsAlive())
                        {
                            float dist = bot->GetDistance(food);
                            if (dist < nearestDist)
                            {
                                nearestDist = dist;
                                nearest = food;
                            }
                        }
                    }
                }
                return nearest;
            }
        }

        void HandleCaptainCookieFoodCycle(Creature* bot)
        {
            if (!bot || !bot->IsAlive())
                return;

            if (bot->GetMapId() != FSBDungeon::Maps::Deadmines)
                return;

            InstanceScript* instance = bot->GetInstanceScript();
            if (!instance || instance->GetBossState(DM::DataTypes::BOSS_CAPTAIN_COOKIE) != IN_PROGRESS)
                return;

            FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI)
                return;

            // Ignore stale events from a previous encounter
            if (!baseAI->GetDungeonData()->mechanicFlagB)
                return;

            // Reset if already active (shouldn't happen, but safety)
            if (baseAI->GetDungeonData()->mechanicCounterA != 0)
            {
                baseAI->GetDungeonData()->mechanicCounterA = 0;
                baseAI->GetDungeonData()->mechanicTargetGuid = ObjectGuid::Empty;
            }

            baseAI->GetDungeonData()->mechanicFlagA = true;

            // Tank: disengage boss and prepare for 4 clicks (bad, good, bad, good)
            if (baseAI->botRole == FSB_ROLE_TANK)
            {
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleCaptainCookieFoodCycle Bot {} - tank starting cycle (4 clicks)", bot->GetName());
                bot->AttackStop();
                bot->GetMotionMaster()->Clear();
                baseAI->GetDungeonData()->mechanicCounterA = 4;
            }
            // Non-tank: keep attacking boss, prepare for 2 good food clicks
            else
            {
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleCaptainCookieFoodCycle Bot {} - non-tank starting cycle (2 good clicks)", bot->GetName());
                baseAI->GetDungeonData()->mechanicCounterA = 2;
            }

            // Schedule next cycle
            baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_COOKIE_FOOD_CYCLE, 30s);
        }

        bool ProcessCaptainCookieFoodClick(Creature* bot)
        {
            if (!bot)
                return false;

            FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI || baseAI->GetDungeonData()->mechanicCounterA == 0)
                return false;

            // If we don't have a committed target (empty GUID or invalid), pick one now
            Creature* food = nullptr;
            if (!baseAI->GetDungeonData()->mechanicTargetGuid.IsEmpty())
                food = ObjectAccessor::GetCreature(*bot, baseAI->GetDungeonData()->mechanicTargetGuid);

            if (!food || !food->IsAlive())
            {
                // Determine what to search for based on role and remaining clicks
                bool needBadFood = false;
                if (baseAI->botRole == FSB_ROLE_TANK)
                    needBadFood = (baseAI->GetDungeonData()->mechanicCounterA % 2 == 0); // 4,2 -> bad; 3,1 -> good

                food = FindNearestCookieFood(bot, needBadFood);
                if (!food)
                {
                    TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: ProcessCaptainCookieFoodClick Bot {} - no {}food found, aborting cycle", bot->GetName(), needBadFood ? "rotten " : "good ");
                    // No food available; abort cycle and re-engage boss
                    baseAI->GetDungeonData()->mechanicCounterA = 0;
                    baseAI->GetDungeonData()->mechanicTargetGuid = ObjectGuid::Empty;
                    baseAI->GetDungeonData()->mechanicFlagA = false;
                    if (Unit* boss = bot->FindNearestCreature(DM::Creatures::CaptainCookie, 100.0f))
                        if (boss->IsAlive())
                            FSBCombat::BotDoAttack(bot, boss);
                    return false;
                }
                baseAI->GetDungeonData()->mechanicTargetGuid = food->GetGUID();
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: ProcessCaptainCookieFoodClick Bot {} - found {}food {} (entry {}), moving to click", bot->GetName(), needBadFood ? "rotten " : "good ", food->GetName(), food->GetEntry());
            }

            // Wait until within spellclick range
            if (bot->GetDistance(food) > COOKIE_FOOD_SPELLCLICK_RANGE)
            {
                if (!bot->isMoving())
                    bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_COOKIE_FOOD, food->GetPosition());
                return true;
            }

            // Perform the spellclick
            TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: ProcessCaptainCookieFoodClick Bot {} - spellclicking {} (entry {}), clicks remaining {}", bot->GetName(), food->GetName(), food->GetEntry(), baseAI->GetDungeonData()->mechanicCounterA - 1);
            food->HandleSpellClick(bot);

            // Clear committed target so next tick picks the next food
            baseAI->GetDungeonData()->mechanicTargetGuid = ObjectGuid::Empty;

            // Decrement remaining clicks
            --baseAI->GetDungeonData()->mechanicCounterA;

            // If cycle finished, re-engage boss
            if (baseAI->GetDungeonData()->mechanicCounterA == 0)
            {
                baseAI->GetDungeonData()->mechanicFlagA = false;
                if (Unit* boss = bot->FindNearestCreature(DM::Creatures::CaptainCookie, 100.0f))
                    if (boss->IsAlive())
                    {
                        TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: ProcessCaptainCookieFoodClick Bot {} - cycle complete, re-engaging boss", bot->GetName());
                        FSBCombat::BotDoAttack(bot, boss);
                    }
            }

            return true;
        }
    }
}
