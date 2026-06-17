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

namespace DM = Scripts::EasternKingdoms::Deadmines;

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
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_DEADMINES_CHECK_PROTOTYPE_REAPER, 1s);

        // Captain Cookie food interaction cycle
        if (bot->GetMapId() == Maps::Deadmines)
        {
            if (InstanceScript* instance = bot->GetInstanceScript())
            {
                if (instance->GetBossState(DM::DataTypes::BOSS_CAPTAIN_COOKIE) == IN_PROGRESS)
                {
                    if (FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI()))
                    {
                        if (baseAI->botCookieClicksRemaining == 0 && !baseAI->botCookieCycleInitiated)
                        {
                            // Not yet cycling; schedule the first cycle
                            TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: CheckDungeonHandlingNeeded Bot {} - scheduling Captain Cookie food cycle (boss IN_PROGRESS)", bot->GetName());
                            baseAI->botCookieCycleInitiated = true;
                            baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_COOKIE_FOOD_CYCLE, 1s);
                        }
                        else if (!bot->IsInCombat() && baseAI->botCookieClicksRemaining > 0)
                        {
                            // Bot dropped combat but still has pending clicks; continue processing OOC
                            FSBDungeon::Deadmines::ProcessCaptainCookieFoodClick(bot);
                        }
                    }
                }
                else
                {
                    // Boss not in progress (e.g. DONE or NOT_STARTED); clear any stuck cookie state
                    if (FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI()))
                    {
                        if (baseAI->botCookieClicksRemaining > 0 || baseAI->botCookieFoodCycleActive || baseAI->botCookieCycleInitiated)
                        {
                            TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: CheckDungeonHandlingNeeded Bot {} - clearing cookie state (boss not IN_PROGRESS)", bot->GetName());
                            baseAI->botCookieClicksRemaining = 0;
                            baseAI->botCookieFoodTarget = ObjectGuid::Empty;
                            baseAI->botCookieFoodCycleActive = false;
                            baseAI->botCookieCycleInitiated = false;
                        }

                        FSBMovement::ResumeFollow(bot, baseAI->botFollowDistance, baseAI->botFollowAngle);
                    }
                }
            }
        }
    }

    // This is the entry point from ICActions (Main actions during combat state)
    bool CheckDungeonInCombatHandlingNeeded(Creature* bot)
    {
        if (!bot)
            return false;

        if (!FSBDungeon::IsBotInDungeon(bot))
            return false;

        // Captain Cookie food interaction cycle
        if (bot->GetMapId() == Maps::Deadmines)
        {
            if (InstanceScript* instance = bot->GetInstanceScript())
            {
                if (instance->GetBossState(DM::DataTypes::BOSS_CAPTAIN_COOKIE) == IN_PROGRESS)
                {
                    if (FSBDungeon::Deadmines::ProcessCaptainCookieFoodClick(bot))
                        return true;
                }
                else
                {
                    // Boss not in progress; clear any stuck cookie state so bots resume normal behavior
                    if (FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI()))
                    {
                        if (baseAI->botCookieClicksRemaining > 0 || baseAI->botCookieFoodCycleActive || baseAI->botCookieCycleInitiated)
                        {
                            TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: CheckDungeonInCombatHandlingNeeded Bot {} - clearing cookie state (boss not IN_PROGRESS)", bot->GetName());
                            baseAI->botCookieClicksRemaining = 0;
                            baseAI->botCookieFoodTarget = ObjectGuid::Empty;
                            baseAI->botCookieFoodCycleActive = false;
                            baseAI->botCookieCycleInitiated = false;
                        }
                    }
                }
            }
        }

        // Non-tank bad food avoidance during Captain Cookie
        if (bot->GetMapId() == Maps::Deadmines)
        {
            if (FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI()))
            {
                if (baseAI->botRole != FSB_ROLE_TANK)
                {
                    if (InstanceScript* instance = bot->GetInstanceScript())
                    {
                        if (instance->GetBossState(DM::DataTypes::BOSS_CAPTAIN_COOKIE) == IN_PROGRESS)
                        {
                            // Check all 6 rotten food entries
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
                                if (Creature* badFood = bot->FindNearestCreature(entry, Deadmines::COOKIE_BAD_FOOD_AVOID_DISTANCE))
                                {
                                    if (badFood->IsAlive())
                                    {
                                        // Back away from bad food
                                        float angle = bot->GetAbsoluteAngle(badFood);
                                        Position pos = bot->GetNearPosition(Deadmines::COOKIE_BAD_FOOD_AVOID_DISTANCE + 2.0f, angle + static_cast<float>(M_PI));
                                        bot->GetMotionMaster()->Clear();
                                        bot->GetMotionMaster()->MovePoint(FSBMovement::MOVEMENT_POINT_COOKIE_FOOD, pos);
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

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
        case DM::Creatures::FoeReaper5000:
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
            return instance->GetData(DM::Misc::FoeReaper5000AOEWarning);
        return false;
    }

    bool HandleFoeReaperAOEEvasion(Creature* bot, Unit* target)
    {
        if (!bot || !target)
            return false;

        // Do not interfere with vehicle combat
        if (bot->GetVehicle())
            return false;

        if (target->GetEntry() != DM::Creatures::FoeReaper5000)
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

    Unit* GetDungeonTargetOverride(Creature* bot, Unit* target)
    {
        if (!bot || !target)
            return nullptr;

        if (!IsBotInDungeon(bot))
            return nullptr;

        uint32 mapId = GetBotDungeonId(bot);
        switch (mapId)
        {
            case Maps::Deadmines:
            {
                // Override for Glubtok blossom adds
                if (target->GetEntry() == DM::Creatures::Glubtok)
                {
                    Unit* blossom = nullptr;
                    if (Creature* frostBlossom = target->FindNearestCreature(DM::Creatures::FrostBlossom, 30.0f))
                    {
                        if (frostBlossom->IsAlive() && bot->IsValidAttackTarget(frostBlossom))
                            blossom = frostBlossom;
                    }
                    else if (Creature* fireBlossom = target->FindNearestCreature(DM::Creatures::FireBlossom, 30.0f))
                        if (fireBlossom->IsAlive() && bot->IsValidAttackTarget(fireBlossom))
                            blossom = fireBlossom;

                    return blossom;
                }
                // Override for Admiral Ripsnarl adds
                if (target->GetEntry() == DM::Creatures::AdmiralRipsnarl)
                {

                    InstanceScript* instance = bot->GetInstanceScript();
                    if (!instance)
                        return nullptr;

                    // Fog phase active -> redirect to vapors
                    if (instance->GetData(DM::Misc::RipsnarlFogActive) != 0)
                    {
                        if (Creature* vapor = target->FindNearestCreature(
                            DM::Creatures::Vapor, 100.0f))
                        {
                            if (vapor->IsAlive() && bot->IsValidAttackTarget(vapor))
                                return vapor;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }

        return nullptr;
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
        auto group = ResolveGroup(bot, baseAI->botLogicalGroup, false);
        if (group.empty())
            return;

        for (Unit* unit : group)
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

            Creature* prototypeReaper = bot->FindNearestCreature(DM::Creatures::PrototypeReaper, PROTOTYPE_REAPER_RANGE);
            if (!prototypeReaper)
                return;

            if (prototypeReaper->GetVehicleKit() && prototypeReaper->GetVehicleKit()->GetPassenger(0))
                return;

            baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_ENTER_PROTOTYPE_REAPER, 1s, 5s);
        }

        static bool IsAllowedVehicleCastTarget(uint32 entry)
        {
            return entry == DM::Creatures::DefiasWatcher ||
                   entry == DM::Creatures::DefiasReaper ||
                   entry == DM::Creatures::MoltenSlag;
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

            vehicleCreature->CastSpell(target, DM::Spells::PrototypeReaperCharge, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
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
            if (!vehicleCreature || vehicleCreature->GetEntry() != DM::Creatures::PrototypeReaper)
            {
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleVehicleCombatCheck Bot {} - vehicle is not the Prototype Reaper, stopping", bot->GetName());
                baseAI->botVehicleCombatTarget.Clear();
                return;
            }

            Unit* target = FSBCombat::GetNextAttackTarget(bot);

            // Boss fight: park at the designated slags position, scan for Molten Slags from there
            if (target && target->GetEntry() == DM::Creatures::FoeReaper5000)
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

                Unit* slag = vehicleCreature->FindNearestCreature(DM::Creatures::MoltenSlag, 100.0f);

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
            if (Creature* nearbyBoss = vehicleCreature->FindNearestCreature(DM::Creatures::FoeReaper5000, BOSS_DISTANCE))
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

            if (bot->GetMapId() != Maps::Deadmines)
                return;

            InstanceScript* instance = bot->GetInstanceScript();
            if (!instance || instance->GetBossState(DM::DataTypes::BOSS_CAPTAIN_COOKIE) != IN_PROGRESS)
                return;

            FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI)
                return;

            // Ignore stale events from a previous encounter
            if (!baseAI->botCookieCycleInitiated)
                return;

            // Reset if already active (shouldn't happen, but safety)
            if (baseAI->botCookieClicksRemaining != 0)
            {
                baseAI->botCookieClicksRemaining = 0;
                baseAI->botCookieFoodTarget = ObjectGuid::Empty;
            }

            baseAI->botCookieFoodCycleActive = true;

            // Tank: disengage boss and prepare for 4 clicks (bad, good, bad, good)
            if (baseAI->botRole == FSB_ROLE_TANK)
            {
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleCaptainCookieFoodCycle Bot {} - tank starting cycle (4 clicks)", bot->GetName());
                bot->AttackStop();
                bot->GetMotionMaster()->Clear();
                baseAI->botCookieClicksRemaining = 4;
            }
            // Non-tank: keep attacking boss, prepare for 2 good food clicks
            else
            {
                TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: HandleCaptainCookieFoodCycle Bot {} - non-tank starting cycle (2 good clicks)", bot->GetName());
                baseAI->botCookieClicksRemaining = 2;
            }

            // Schedule next cycle in 20s
            baseAI->ScheduleBotEvent(FSBEvents::EVENT_DM_COOKIE_FOOD_CYCLE, 30s);
        }

        bool ProcessCaptainCookieFoodClick(Creature* bot)
        {
            if (!bot)
                return false;

            FSB_BaseAI* baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI || baseAI->botCookieClicksRemaining == 0)
                return false;

            // If we don't have a committed target (empty GUID or invalid), pick one now
            Creature* food = nullptr;
            if (!baseAI->botCookieFoodTarget.IsEmpty())
                food = ObjectAccessor::GetCreature(*bot, baseAI->botCookieFoodTarget);

            if (!food || !food->IsAlive())
            {
                // Determine what to search for based on role and remaining clicks
                bool needBadFood = false;
                if (baseAI->botRole == FSB_ROLE_TANK)
                    needBadFood = (baseAI->botCookieClicksRemaining % 2 == 0); // 4,2 -> bad; 3,1 -> good

                food = FindNearestCookieFood(bot, needBadFood);
                if (!food)
                {
                    TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: ProcessCaptainCookieFoodClick Bot {} - no {}food found, aborting cycle", bot->GetName(), needBadFood ? "rotten " : "good ");
                    // No food available; abort cycle and re-engage boss
                    baseAI->botCookieClicksRemaining = 0;
                    baseAI->botCookieFoodTarget = ObjectGuid::Empty;
                    baseAI->botCookieFoodCycleActive = false;
                    if (Unit* boss = bot->FindNearestCreature(DM::Creatures::CaptainCookie, 100.0f))
                        if (boss->IsAlive())
                            FSBCombat::BotDoAttack(bot, boss);
                    return false;
                }
                baseAI->botCookieFoodTarget = food->GetGUID();
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
            TC_LOG_DEBUG("scripts.fsb.dungeon", "FSB: ProcessCaptainCookieFoodClick Bot {} - spellclicking {} (entry {}), clicks remaining {}", bot->GetName(), food->GetName(), food->GetEntry(), baseAI->botCookieClicksRemaining - 1);
            food->HandleSpellClick(bot);

            // Clear committed target so next tick picks the next food
            baseAI->botCookieFoodTarget = ObjectGuid::Empty;

            // Decrement remaining clicks
            --baseAI->botCookieClicksRemaining;

            // If cycle finished, re-engage boss
            if (baseAI->botCookieClicksRemaining == 0)
            {
                baseAI->botCookieFoodCycleActive = false;
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
