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

#include "CellImpl.h"
#include "Containers.h"
#include "CreatureAI.h"
#include "CreatureAIImpl.h" // for RAND()
#include "DB2Stores.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedFollowerAI.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "TaskScheduler.h"
#include "TemporarySummon.h"
#include "ThreatManager.h"
#include "Vehicle.h"

namespace Scripts::Pandaria::TheWanderingIsle
{
namespace Spells
{
    // Singing Pools
    static constexpr uint32 CurseOfTheFrog = 102938;
    static constexpr uint32 CurseOfTheSkunk = 102939;
    static constexpr uint32 CurseOfTheTurtle = 102940;
    static constexpr uint32 CurseOfTheCrane = 102941;
    static constexpr uint32 CurseOfTheCrocodile = 102942;
    static constexpr uint32 RideVehiclePole = 102717;
    static constexpr uint32 TrainingBellPoleExitExclusion = 133381;

    // Only the Worthy Shall Pass
    static constexpr uint32 FireCrashCover = 108149;
    static constexpr uint32 FireCrashInvis = 108150;
    static constexpr uint32 FireCrashPhaseShift = 102515;
    static constexpr uint32 FlyingShadowKick = 108936;
    static constexpr uint32 FlyingShadowKickJump = 108943;
    static constexpr uint32 FeetOfFury = 108958;
    static constexpr uint32 FeetOfFuryDamage = 108957;
}

namespace Quests
{
    static constexpr uint32 OnlyTheWorthyShallPass = 29421;
}

namespace Creatures
{
    static constexpr uint32 MasterLiFei = 54135;
    static constexpr uint32 MasterLiFeiCombat = 54734;
}

namespace Talks
{
    static constexpr uint32 LiFeiDefeat = 0;
}

enum TraineeMisc
{
    SAY_FINISH_FIGHT                    = 0,

    SPELL_BLACKOUT_KICK                 = 109080,

    QUEST_29524_KILLCREDIT              = 54586,

    POINT_DESPAWN                       = 0,

    NPC_HUOJIN_TRAINEE_MALE             = 54586,
    NPC_HUOJIN_TRAINEE_FEMALE           = 65470,
    NPC_TUSHUI_TRAINEE_MALE             = 54587,
    NPC_TUSHUI_TRAINEE_FEMALE           = 65471,
};

Position const TraineeEndpoints[] = {
    { 1465.3872f, 3283.8604f, 137.69096f },
    { 1431.401f, 3264.001f, 136.02579f },
    { 1397.2067f, 3276.5618f, 133.84508f },
    { 1441.566f, 3232.8013f, 135.01802f },
    { 1403.632f, 3229.1094f, 132.14877f },
    { 1347.1927f, 3286.5842f, 131.94803f },
    { 1365.1865f, 3338.9502f, 128.57233f },
    { 1349.6024f, 3315.0574f, 130.97443f },
    { 1335.4618f, 3344.019f, 130.42047f },
    { 1360.1198f, 3378.02f, 127.34183f },
    { 1435.8524f, 3355.6423f, 173.77744f },
    { 1432.7031f, 3385.1572f, 184.4187f },
    { 1452.6094f, 3373.3315f, 187.0402f },
    { 1426.7778f, 3364.7517f, 184.39569f },
    { 1450.3646f, 3361.264f, 184.42484f },
};

Emote constexpr TraineeEmotes[5] =
{
    EMOTE_ONESHOT_MONKOFFENSE_ATTACKUNARMED,
    EMOTE_ONESHOT_MONKOFFENSE_SPECIALUNARMED,
    EMOTE_ONESHOT_MONKOFFENSE_PARRYUNARMED,
    EMOTE_ONESHOT_PALMSTRIKE,
    EMOTE_ONESHOT_MONKOFFENSE_ATTACKUNARMEDOFF,
};

// 54586 - Huojin Trainee
// 65470 - Huojin Trainee
// 54587 - Tushui Trainee
// 65471 - Tushui Trainee
struct npc_tushui_huojin_trainee : public ScriptedAI
{
    npc_tushui_huojin_trainee(Creature* creature) : ScriptedAI(creature), _defeated(false) { }

    Emote PlayRandomEmote() const
    {
        Emote emote = Trinity::Containers::SelectRandomContainerElement(TraineeEmotes);
        me->HandleEmoteCommand(emote);
        return emote;
    }

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (me->HealthBelowPctDamaged(20, damage))
        {
            damage = 0;
            if (_defeated)
                return;

            _defeated = true;
            if (attacker)
            {
                if (Player* player = attacker->ToPlayer())
                    player->KilledMonsterCredit(QUEST_29524_KILLCREDIT);
            }

            me->SetEmoteState(EMOTE_ONESHOT_NONE);
            me->SetImmuneToPC(true);
            me->CombatStop();

            _scheduler.Schedule(1s, [this](TaskContext /*task*/)
            {
                Talk(SAY_FINISH_FIGHT);
            });

            _scheduler.Schedule(3s, [this](TaskContext /*task*/)
            {
                Position currentPosition;
                float currentDist = 1000.0f;
                for (Position const& pos : TraineeEndpoints)
                {
                    float dist = pos.GetExactDist(me);
                    if (dist >= currentDist)
                        continue;

                    currentPosition = pos;
                    currentDist = dist;
                }
                me->GetMotionMaster()->MovePoint(POINT_DESPAWN, currentPosition);
            });
        }
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE)
            return;

        if (id != POINT_DESPAWN)
            return;

        me->DespawnOrUnsummon();
    }

    void JustEngagedWith(Unit* /*attacker*/) override
    {
        _scheduler.Schedule(4s, [this](TaskContext task)
        {
            if (me->GetVictim())
                DoCastVictim(SPELL_BLACKOUT_KICK);

            task.Repeat(8s);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);

        if (!UpdateVictim())
            return;
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        if (!_defeated)
            ScriptedAI::EnterEvadeMode(why);
    }

protected:
    TaskScheduler _scheduler;
    bool _defeated;
};

enum HuojinTraineeMisc
{
    ACTION_PARTNER_ENTERED_COMBAT = 1,
};

class HuojinTraineePartnerSearch
{
public:
    HuojinTraineePartnerSearch(Creature* partner) : _partner(partner), _minDist(10.0f) { }

    bool operator()(Creature const* target)
    {
        if (target->GetEntry() != NPC_HUOJIN_TRAINEE_MALE && target->GetEntry() != NPC_HUOJIN_TRAINEE_FEMALE)
            return false;
        if (target == _partner)
            return false;
        if (target->IsInCombat())
            return false;
        if (target->IsInEvadeMode())
            return false;
        if (target->isDead())
            return false;

        float dist = target->GetDistance(_partner);
        if (dist >= _minDist)
            return false;

        _minDist = dist;
        return true;
    }

private:
    Unit* _partner;
    float _minDist;
};

// 54586 - Huojin Trainee
// 65470 - Huojin Trainee
struct npc_huojin_trainee : public npc_tushui_huojin_trainee
{
    npc_huojin_trainee(Creature* creature) : npc_tushui_huojin_trainee(creature) { }

    void JustEngagedWith(Unit* attacker) override
    {
        _scheduler.CancelAll();
        npc_tushui_huojin_trainee::JustEngagedWith(attacker);

        Creature* partner = ObjectAccessor::GetCreature(*me, _partnerGuid);
        if (!partner)
            return;

        if (partner->AI())
            partner->AI()->DoAction(ACTION_PARTNER_ENTERED_COMBAT);
    }

    void DoAction(int32 action) override
    {
        if (action == ACTION_PARTNER_ENTERED_COMBAT)
        {
            _scheduler.CancelAll();

            me->SetEmoteState(EMOTE_ONESHOT_NONE);
            _scheduler.Schedule(1s, [this](TaskContext /*task*/ )
            {
                me->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
            });
        }
    }

    void BeginSparring(ObjectGuid guid)
    {
        _partnerGuid = guid;
        me->SetEmoteState(EMOTE_ONESHOT_NONE);
        me->HandleEmoteCommand(EMOTE_ONESHOT_BOW);

        _scheduler.Schedule(1s, [this](TaskContext /*task*/)
        {
            me->SetEmoteState(EMOTE_STATE_MONKOFFENSE_READYUNARMED);
        });

        _scheduler.Schedule(4s, [this](TaskContext task)
        {
            PlayRandomEmote();
            task.Repeat(4s);
        });
    }

    Creature* GetNewPartner() const
    {
        Creature* partner = nullptr;
        HuojinTraineePartnerSearch check(me);
        Trinity::CreatureLastSearcher<HuojinTraineePartnerSearch> searcher(me, partner, check);
        Cell::VisitGridObjects(me, searcher, 10.0f);
        return partner;
    }

    void BeginSparringDelayed(ObjectGuid partnerGuid)
    {
        _partnerGuid = partnerGuid;
        _scheduler.Schedule(1s, [this, partnerGuid](TaskContext /*task*/)
        {
            BeginSparring(partnerGuid);
        });
    }

    void InitiateSparring()
    {
        Creature* partner = GetNewPartner();

        if (!partner)
            return;

        BeginSparring(partner->GetGUID());
        if (Creature* partner = ObjectAccessor::GetCreature(*me, _partnerGuid))
        {
            if (npc_huojin_trainee* ai = CAST_AI(npc_huojin_trainee, partner->GetAI()))
                ai->BeginSparringDelayed(me->GetGUID());
        }
    }

    void JustReachedHome() override
    {
        InitiateSparring();
    }

    void JustAppeared() override
    {
        // partner is already assigned, sparring start is delayed
        if (!ObjectAccessor::GetCreature(*me, _partnerGuid))
            InitiateSparring();
    }
private:
    ObjectGuid _partnerGuid;
};

class TushuiTraineeSearch
{
public:
    TushuiTraineeSearch(Creature* leader, float maxDist) : _leader(leader), _maxDist(maxDist) { }

    bool operator()(Creature const* target) const
    {
        if (target->GetEntry() != NPC_TUSHUI_TRAINEE_MALE && target->GetEntry() != NPC_TUSHUI_TRAINEE_FEMALE)
            return false;
        if (target->IsInCombat())
            return false;
        if (target->IsInEvadeMode())
            return false;
        if (target->GetDistance(_leader) >= _maxDist)
            return false;
        if (target->isDead())
            return false;

        return true;
    }

private:
    Creature* _leader;
    float _maxDist;
};

void HandleEmoteNearbyTushuiTrainees(Creature* leader, Emote emote)
{
    std::list<Creature*> traineeList;
    TushuiTraineeSearch check(leader, 10.0f);
    Trinity::CreatureListSearcher<TushuiTraineeSearch> searcher(leader, traineeList, check);
    Cell::VisitGridObjects(leader, searcher, 10.0f);

    for (Creature* trainee : traineeList)
        trainee->HandleEmoteCommand(emote);
}

// 54587 - Tushui Trainee
// 65471 - Tushui Trainee
struct npc_tushui_leading_trainee : public npc_tushui_huojin_trainee
{
    npc_tushui_leading_trainee(Creature* creature) : npc_tushui_huojin_trainee(creature) { }

    void ScheduleEmoteExecution()
    {
        _scheduler.Schedule(1s, [this](TaskContext task)
        {
            Emote emote = PlayRandomEmote();
            HandleEmoteNearbyTushuiTrainees(me, emote);
            task.Repeat(6s);
        });
    }

    void JustReachedHome() override
    {
        ScheduleEmoteExecution();
    }

    void JustAppeared() override
    {
        ScheduleEmoteExecution();
    }

    void JustEngagedWith(Unit* attacker) override
    {
        _scheduler.CancelAll();
        npc_tushui_huojin_trainee::JustEngagedWith(attacker);
    }
};

// 61411 - Instructor Zhi
struct npc_instructor_zhi : public ScriptedAI
{
    npc_instructor_zhi(Creature* creature) : ScriptedAI(creature) { }

    void JustAppeared() override
    {
        _scheduler.Schedule(6s, [this](TaskContext task)
        {
            Emote emote = Trinity::Containers::SelectRandomContainerElement(TraineeEmotes);
            me->HandleEmoteCommand(emote);

            task.Schedule(1s, [this, emote](TaskContext /*task*/)
            {
                HandleEmoteNearbyTushuiTrainees(me, emote);
            });
            task.Repeat(6s);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 210986 - Edict of Temperance
struct go_edict_of_temperance : public GameObjectAI
{
    using GameObjectAI::GameObjectAI;

    bool OnGossipHello(Player* /*player*/) override
    {
        me->DespawnOrUnsummon(1ms);
        return false;
    }
};

enum JaominRoMisc
{
    // Spells
    SPELL_CSA_AREATRIGGER_DUMMY      = 88811,
    SPELL_HAWK_DIVING_TO_EARTH       = 108955,
    SPELL_HAWK_DIVING_TO_EARTH_DMG   = 108935,
    SPELL_BABY_ELEPHANT_TAKES_A_BATH = 108938,
    SPELL_FORCE_SUMMONER_TO_RIDE     = 108583,
    SPELL_EJECT_ALL_PASSENGERS       = 50630,
    SPELL_DIZZY                      = 108959,
    SPELL_RIDE_DRAKE                 = 108582,
    SPELL_SERVERSIDE_KILL_CREDIT     = 109837,
    SPELL_FULL_HEALTH                = 17683,

    // Texts
    SAY_INTRO                        = 0,
    SAY_DEFEATED                     = 1,

    // Movement
    POINT_RANDOM_DEST                = 0,

    // Events
    EVENT_RANDOM_SPELL               = 1,
    EVENT_MOVE,
    EVENT_HEAL,
    EVENT_MOVE_HOME
};

// 54611 - Jaomin Ro
struct npc_jaomin_ro : public ScriptedAI
{
    using ScriptedAI::ScriptedAI;

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_RANDOM_SPELL, 4s, 6s);
    }

    void JustReachedHome() override
    {
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE_2);
        me->InitializeReactState();
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_CSA_AREATRIGGER_DUMMY && target->GetTypeId() == TYPEID_PLAYER && !me->IsInCombat())
        {
            Talk(SAY_INTRO, target);
            me->SetOrientation(1.67690026f);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->SetEmoteState(EMOTE_STATE_READY_UNARMED);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasUnitState(UNIT_STATE_STUNNED) || me->HasAura(SPELL_RIDE_DRAKE))
            return;

        switch (_events.ExecuteEvent())
        {
            case EVENT_RANDOM_SPELL:
            {
                uint32 spellId = RAND(SPELL_BABY_ELEPHANT_TAKES_A_BATH, SPELL_HAWK_DIVING_TO_EARTH);
                DoCast(spellId);
                _events.ScheduleEvent(EVENT_RANDOM_SPELL, 4s, 6s);
                break;
            }
            case EVENT_HEAL:
            {
                DoCastSelf(SPELL_FULL_HEALTH);
                me->SetUninteractible(false);
                break;
            }
            case EVENT_MOVE_HOME:
            {
                EnterEvadeMode();
                break;
            }
            default:
                break;
        }
    }

    void OnHealthDepleted(Unit* attacker, bool /*isKill*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
        me->RemoveAllAuras();
        me->InterruptNonMeleeSpells(true);
        _events.Reset();
        me->SetUninteractible(true);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE_2);
        me->SetEmoteState(EMOTE_ONESHOT_NONE);
        DoCast(SPELL_SERVERSIDE_KILL_CREDIT);
        Talk(SAY_DEFEATED, attacker);

        _events.ScheduleEvent(EVENT_HEAL, 5s);
        _events.ScheduleEvent(EVENT_MOVE_HOME, 6s);
    }

private:
    EventMap _events;
};

// 57750 - Jaomin Ro (Hawk)
struct npc_jaomin_ro_hawk : public ScriptedAI
{
    using ScriptedAI::ScriptedAI;

    void JustAppeared() override
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetSpeedRate(MOVE_RUN, 2.5f);
    }

    void IsSummonedBy(WorldObject* summonerWO) override
    {
        Unit* summoner = summonerWO->ToUnit();
        if (!summoner)
            return;
        Unit* victim = summoner->GetVictim();
        if (!victim)
            return;

        DoCast(SPELL_FORCE_SUMMONER_TO_RIDE);
        _scheduler.Schedule(1s, [this, orientation = me->GetAbsoluteAngle(victim) - me->GetOrientation()](TaskContext /*context*/)
        {
            me->GetMotionMaster()->MovePoint(POINT_RANDOM_DEST, me->GetFirstCollisionPosition(40.0f, orientation));
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_HAWK_DIVING_TO_EARTH_DMG && target->GetTypeId() == TYPEID_PLAYER)
        {
            DoCast(SPELL_EJECT_ALL_PASSENGERS);
            me->DespawnOrUnsummon();
        }
    }

    void MovementInform(uint32 type, uint32 pointId) override
    {
        if (type != POINT_MOTION_TYPE)
            return;

        switch (pointId)
        {
            case POINT_RANDOM_DEST:
            {
                DoCast(SPELL_EJECT_ALL_PASSENGERS);
                me->DespawnOrUnsummon();
                break;
            }
            default:
                break;
        }
    }

private:
    TaskScheduler _scheduler;
};

// 108583 - Force Summoner to Ride Vehicle
class spell_force_summoner_to_ride_vehicle : public SpellScript
{
    void HandleScript(SpellEffIndex /*effIndex*/) const
    {
        GetHitUnit()->CastSpell(GetCaster(), GetEffectValue(), TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_force_summoner_to_ride_vehicle::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 108582 - Ride Drake
class spell_ride_drake : public AuraScript
{
    void OnRemoveVehicle(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(caster, SPELL_DIZZY, TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectRemoveFn(spell_ride_drake::OnRemoveVehicle, EFFECT_0, SPELL_AURA_CONTROL_VEHICLE, AURA_EFFECT_HANDLE_REAL);
    }
};

// Quest 29419 - The Missing Driver
enum TheMissingDriverMisc
{
    // Spells
    SPELL_SERVERSIDE_DRIVER_CREDIT  = 106231,
    SPELL_FORCE_SUMMON_CART_DRIVER  = 106206,
    SPELL_SUMMON_CART_DRIVER        = 106205,

    // Texts
    SAY_MIN_DIMWIND_TEXT_0          = 0,
    SAY_MIN_DIMWIND_TEXT_1          = 1,
    SAY_MIN_DIMWIND_TEXT_2          = 2,
    SAY_MIN_DIMWIND_TEXT_3          = 3,

    SAY_AMBERLEAF_SCAMP_0           = 0,
    SAY_AMBERLEAF_SCAMP_1           = 1,

    // Waypoint
    PATH_MOVE_RUN                   = 5650300,
    PATH_MOVE_WALK                  = 5650301,

    WAYPOINT_TALK_0                 = 0,
    WAYPOINT_TALK_1                 = 3,
    WAYPOINT_MOVE_WALK              = 11,

    WAYPOINT_DESPAWN                = 3,

    POINT_MOVE_RANDOM               = 0,

    // Quests
    QUEST_THE_MISSING_DRIVER        = 29419
};

constexpr Position amberleafPos[5] =
{
    { 1410.2014f, 3598.6494f, 89.59319f },
    { 1456.201f,  3568.265f,  88.39075f },
    { 1383.158f,  3595.447f,  90.3155f  },
    { 1367.333f,  3594.927f,  88.89806f },
    { 1350.278f,  3588.938f,  89.17908f }
};

// 6958 - Areatrigger
class at_min_dimwind_captured : public AreaTriggerScript
{
    public:
        at_min_dimwind_captured() : AreaTriggerScript("at_min_dimwind_captured") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
        {
            if (!player->isDead() && player->GetQuestStatus(QUEST_THE_MISSING_DRIVER) == QUEST_STATUS_INCOMPLETE)
            {
                Creature* minDimwind = player->FindNearestCreatureWithOptions(20.0f, { .StringId = "npc_min_dimwind" });

                if (!minDimwind)
                    return false;

                minDimwind->CastSpell(player, SPELL_FORCE_SUMMON_CART_DRIVER, TRIGGERED_FULL_MASK);
                player->CastSpell(player, SPELL_SERVERSIDE_DRIVER_CREDIT, TRIGGERED_FULL_MASK);
                PhasingHandler::OnConditionChange(player); // phase 630 is added when kill credit but immediately is removed to be added again when Min Dimwind reaches final waypoint
            }

            return false;
        }
};

// 56503 - Min Dimwind (Summon)
struct npc_min_dimwind_summon : public ScriptedAI
{
    using ScriptedAI::ScriptedAI;

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (!summoner->IsPlayer())
            return;

        Creature* amberleafScamp1 = me->FindNearestCreatureWithOptions(20.0f, { .StringId = "npc_amberleaf_scamp_1" });
        Creature* amberleafScamp2 = me->FindNearestCreatureWithOptions(20.0f, { .StringId = "npc_amberleaf_scamp_2" });
        Creature* amberleafScamp3 = me->FindNearestCreatureWithOptions(20.0f, { .StringId = "npc_amberleaf_scamp_3" });
        Creature* amberleafScamp5 = me->FindNearestCreatureWithOptions(20.0f, { .StringId = "npc_amberleaf_scamp_5" });

        if (!amberleafScamp1 || !amberleafScamp2 || !amberleafScamp3 || !amberleafScamp5)
            return;

        amberleafScamp1->AI()->Talk(SAY_AMBERLEAF_SCAMP_0);
        amberleafScamp1->GetMotionMaster()->MovePoint(0, amberleafPos[0]);

        amberleafScamp2->GetMotionMaster()->MovePoint(0, amberleafPos[1]);

        amberleafScamp3->GetMotionMaster()->MovePoint(0, amberleafPos[2]);

        amberleafScamp5->GetMotionMaster()->MovePoint(0, amberleafPos[4]);

        _scheduler.Schedule(2s, [this](TaskContext /*task*/)
        {
            Creature* amberleafScamp4 = me->FindNearestCreatureWithOptions(20.0f, { .StringId = "npc_amberleaf_scamp_4" });

            if (!amberleafScamp4)
                return;

            amberleafScamp4->AI()->Talk(SAY_AMBERLEAF_SCAMP_1);
            amberleafScamp4->GetMotionMaster()->MovePoint(0, amberleafPos[3]);
        });

        _scheduler.Schedule(5s, [this](TaskContext task)
        {
            Unit* summoner = me->ToTempSummon()->GetSummonerUnit();

            if (!summoner)
                return;

            me->SetFacingToObject(summoner);
            Talk(SAY_MIN_DIMWIND_TEXT_0, summoner);

            task.Schedule(4s, [this](TaskContext task)
            {
                Talk(SAY_MIN_DIMWIND_TEXT_1);

                task.Schedule(4s, [this](TaskContext /*task*/)
                {
                    me->GetMotionMaster()->MovePath(PATH_MOVE_RUN, false);
                });
            });
        });
    }

    void WaypointReached(uint32 waypointId, uint32 pathId) override
    {
        if (pathId == PATH_MOVE_RUN)
        {
            switch (waypointId)
            {
                case WAYPOINT_TALK_0:
                case WAYPOINT_TALK_1:
                {
                    Talk(SAY_MIN_DIMWIND_TEXT_2);
                    break;
                }
                case WAYPOINT_MOVE_WALK:
                {
                    Talk(SAY_MIN_DIMWIND_TEXT_3);
                    me->GetMotionMaster()->MovePath(PATH_MOVE_WALK, false);
                    break;
                }
            }
        }
        else if (pathId == PATH_MOVE_WALK)
        {
            if (waypointId == WAYPOINT_DESPAWN)
            {
                me->SetFacingTo(0.575958f);
                me->DespawnOrUnsummon(2s);

                _scheduler.Schedule(1s, [this](TaskContext /*task*/)
                {
                    if (me->IsSummon())
                    {
                        Unit* summoner = me->ToTempSummon()->GetSummonerUnit();

                        if (!summoner)
                            return;

                        summoner->RemoveAurasDueToSpell(SPELL_SUMMON_CART_DRIVER);
                        PhasingHandler::OnConditionChange(summoner);
                    }
                });
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 54130 - Amberleaf Scamp
struct npc_amberleaf_scamp : public ScriptedAI
{
    using ScriptedAI::ScriptedAI;

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == POINT_MOVE_RANDOM && !me->IsInCombat())
        {
            me->GetMotionMaster()->MoveRandom(10.0f);

            _scheduler.Schedule(10s, [this](TaskContext /*task*/)
            {
                if (!me->IsInCombat())
                    me->GetMotionMaster()->MoveTargetedHome();
            });
        }
    }

    void JustReachedHome() override
    {
        me->GetMotionMaster()->InitializeDefault();
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// Quest 29414 - The Way of the Tushui
enum AysaCloudsingerMisc
{
    // Texts
    SAY_GO_CAVE              = 0,

    // Waypoints
    POINT_JUMP               = 1,
    PATH_CAVE_OF_MEDITATION  = 5965200
};

constexpr Position aysaJumpPos[3] =
{
    { 1196.72f,   3492.85f,   90.9836f  },
    { 1192.29f,   3478.69f,   108.788f  },
    { 1197.99f,   3460.63f,   103.04f   }
};

// 59652 - Aysa Cloudsinger (summon)
struct npc_aysa_cloudsinger_summon : public ScriptedAI
{
    npc_aysa_cloudsinger_summon(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (!summoner->IsPlayer())
            return;

        Talk(SAY_GO_CAVE, summoner);

        _scheduler.Schedule(3s, [this](TaskContext task)
        {
            me->GetMotionMaster()->MoveJump(EVENT_JUMP, aysaJumpPos[0], 12.0f, {}, 5.0f);

            task.Schedule(1700ms, [this](TaskContext task)
            {
                me->GetMotionMaster()->MoveJump(EVENT_JUMP, aysaJumpPos[1], 12.0f, {}, 5.0f);

                task.Schedule(2s, [this](TaskContext /*task*/)
                {
                    me->GetMotionMaster()->MoveJump(POINT_JUMP, aysaJumpPos[2], 12.0f, {}, 5.0f);
                });
            });
        });
    }

    void MovementInform(uint32 type, uint32 pointId) override
    {
        if (type != EFFECT_MOTION_TYPE)
            return;

        if (pointId == POINT_JUMP)
            me->GetMotionMaster()->MovePath(PATH_CAVE_OF_MEDITATION, false);
    }

    void WaypointPathEnded(uint32 /*nodeId*/, uint32 pathId) override
    {
        if (pathId == PATH_CAVE_OF_MEDITATION)
            me->DespawnOrUnsummon();
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

enum CaveOfMeditationMisc
{
    // Quest
    QUEST_THE_WAY_OF_THE_TUSHUI     = 29414,

    // Spells
    SPELL_MEDITATION_TIMER_BAR      = 116421,
    SPELL_CAVE_OF_SCROLLS_CREDIT    = 102447,
    SPELL_AYSA_CAVE_OF_SCROLLS_COMP = 128598,
    SPELL_SUMMON_MASTER_LI_FEI      = 102445,
    SPELL_SUMMON_AMBERLEAF_TROUBLEMAKER = 114710,

    // Actions
    ACTION_START_MEDITATION  = 1,
    ACTION_FINISH_MEDITATION = 2,
    ACTION_FINISH_EVENT      = 3,

    // Events
    EVENT_SUMMON_AMBERLEAF = 1,

    // Texts
    SAY_AYSA_HELP                   = 0,
    SAY_AYSA_FINISH_MEDITATION      = 1
};

// 59642 - Aysa Cloudsinger (Cave of Meditation)
struct npc_aysa_cloudsinger_cave_of_meditation : public ScriptedAI
{
    npc_aysa_cloudsinger_cave_of_meditation(Creature* creature) : ScriptedAI(creature), _finishEvent(false) { }

    void DoAction(int32 action) override
    {
        switch (action)
        {
            case ACTION_START_MEDITATION:
            {
                _finishEvent = false;
                events.ScheduleEvent(EVENT_SUMMON_AMBERLEAF, 1s);
                break;
            }
            case ACTION_FINISH_MEDITATION:
            {
                events.CancelEvent(EVENT_SUMMON_AMBERLEAF);
                break;
            }
            case ACTION_FINISH_EVENT:
            {
                if (_finishEvent)
                    return;

                Creature* aysa = me->FindNearestCreatureWithOptions(40.0f, { .StringId = "npc_aysa_after_quest_29414", .IgnorePhases = true });

                if (!aysa)
                    return;

                aysa->AI()->Talk(SAY_AYSA_FINISH_MEDITATION);
                _finishEvent = true;
                break;
            }
            default:
                break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SUMMON_AMBERLEAF:
                {
                    DoCastSelf(SPELL_SUMMON_AMBERLEAF_TROUBLEMAKER);
                    events.Repeat(11s);
                    break;
                }
                default:
                    break;
            }
        }
    }

private:
    EventMap events;
    bool _finishEvent;
};

enum MasterLiFeiMisc
{
    // Waypoint
    PATH_MASTER_LI = 5485600,

    // Texts
    SAY_TEXT_0 = 0,
    SAY_TEXT_1 = 1,
    SAY_TEXT_2 = 2,
    SAY_TEXT_3 = 3,
    SAY_TEXT_4 = 4,
    SAY_TEXT_5 = 5,
    SAY_TEXT_6 = 6,
    SAY_TEXT_7 = 7,

    // Spells
    SPELL_UPDATE_PHASE_SHIFT_DEMON_CREATOR = 84034
};

// 54856 - Master Li Fei (Summon)
struct npc_master_li_fei_summon : public ScriptedAI
{
    npc_master_li_fei_summon(Creature* creature) : ScriptedAI(creature) { }

    void FaceToPlayer()
    {
        if (me->IsSummon())
        {
            Unit* summoner = me->ToTempSummon()->GetSummonerUnit();

            if (!summoner)
                return;

            me->SetFacingToObject(summoner);
        }
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (!summoner->IsPlayer())
            return;

        me->GetMotionMaster()->MovePath(PATH_MASTER_LI, false);

        Seconds delay = 23s;

        _scheduler.Schedule(delay, [this](TaskContext)
        {
            FaceToPlayer();
        });

        delay += 2s;

        _scheduler.Schedule(delay, [this](TaskContext)
        {
            Talk(SAY_TEXT_0);
        });

        delay += 10s;

        _scheduler.Schedule(delay, [this](TaskContext)
        {
            Talk(SAY_TEXT_1);
        });

        delay += 12s;

        _scheduler.Schedule(delay, [this](TaskContext)
        {
            Talk(SAY_TEXT_2);
        });

        delay += 11s;

        _scheduler.Schedule(delay, [this](TaskContext)
        {
            FaceToPlayer();
            Talk(SAY_TEXT_3);
        });

        delay += 11s;

        _scheduler.Schedule(delay, [this](TaskContext)
        {
            Talk(SAY_TEXT_4);
        });

        delay += 9s;

        _scheduler.Schedule(delay, [this](TaskContext)
        {
            FaceToPlayer();
        });

        delay += 2s;

        _scheduler.Schedule(delay, [this](TaskContext)
        {
            Talk(SAY_TEXT_5);
        });

        delay += 6s;

        _scheduler.Schedule(delay, [this](TaskContext)
        {
            Creature* aysa = me->FindNearestCreatureWithOptions(40.0f, { .StringId = "npc_aysa_quest_29414" });

            if (!aysa)
                return;

            aysa->AI()->DoAction(ACTION_FINISH_MEDITATION);
            DoCastSelf(SPELL_UPDATE_PHASE_SHIFT_DEMON_CREATOR);
            Talk(SAY_TEXT_6);
            me->DespawnOrUnsummon(200ms);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 7756 - Areatrigger
class at_cave_of_meditation : public AreaTriggerScript
{
public:
    at_cave_of_meditation() : AreaTriggerScript("at_cave_of_meditation") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        if (player->IsAlive() && player->GetQuestStatus(QUEST_THE_WAY_OF_THE_TUSHUI) == QUEST_STATUS_INCOMPLETE)
        {
            if (!player->HasAura(SPELL_MEDITATION_TIMER_BAR))
                player->CastSpell(player, SPELL_MEDITATION_TIMER_BAR, TRIGGERED_FULL_MASK);

            return true;
        }
        return false;
    }

    bool OnExit(Player* player, AreaTriggerEntry const* /*trigger*/) override
    {
        player->RemoveAura(SPELL_MEDITATION_TIMER_BAR);
        return true;
    }
};

// 7645 - Areatrigger
class at_inside_of_cave_of_meditation : public AreaTriggerScript
{
public:
    at_inside_of_cave_of_meditation() : AreaTriggerScript("at_inside_of_cave_of_meditation") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        if (player->IsAlive() && player->GetQuestStatus(QUEST_THE_WAY_OF_THE_TUSHUI) == QUEST_STATUS_INCOMPLETE)
        {
            if (!player->HasAura(SPELL_SUMMON_MASTER_LI_FEI))
            {
                Creature* aysa = player->FindNearestCreatureWithOptions(40.0f, { .StringId = "npc_aysa_quest_29414" });

                if (!aysa)
                    return false;

                aysa->AI()->Talk(SAY_AYSA_HELP, player);
                aysa->AI()->DoAction(ACTION_START_MEDITATION);
                player->CastSpell(player, SPELL_SUMMON_MASTER_LI_FEI, TRIGGERED_FULL_MASK);
            }

            return true;
        }
        return false;
    }

    bool OnExit(Player* player, AreaTriggerEntry const* /*trigger*/) override
    {
        if (player->IsAlive() && player->GetQuestStatus(QUEST_THE_WAY_OF_THE_TUSHUI) == QUEST_STATUS_COMPLETE)
        {
            if (Creature* aysa = player->FindNearestCreatureWithOptions(40.0f, { .StringId = "npc_aysa_quest_29414", .IgnorePhases = true }))
                aysa->AI()->DoAction(ACTION_FINISH_EVENT);
        }

        return true;
    }
};

// 116421 - Meditation Timer Bar
class spell_meditation_timer_bar : public AuraScript
{
    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        if (Unit* target = GetTarget())
        {
            target->ModifyPower(POWER_ALTERNATE_POWER, 1);

            if (target->GetPowerPct(POWER_ALTERNATE_POWER) == 100)
            {
                target->CastSpell(GetTarget(), SPELL_CAVE_OF_SCROLLS_CREDIT, TRIGGERED_FULL_MASK);
                target->CastSpell(GetTarget(), SPELL_AYSA_CAVE_OF_SCROLLS_COMP, TRIGGERED_FULL_MASK);
                Remove();
            }
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_meditation_timer_bar::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
    }
};

enum FlameSpoutSpell
{
    SPELL_FLAME_SPOUT_VISUAL = 114686
};

// 114684 - Flame Spout
class spell_flame_spout : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_FLAME_SPOUT_VISUAL });
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_FLAME_SPOUT_VISUAL);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_flame_spout::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

template<uint32 CurseSpellID>
class at_singing_pools_transform_base : public AreaTriggerScript
{
public:
    at_singing_pools_transform_base(char const* scriptName) : AreaTriggerScript(scriptName) {}

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        if (!player->IsAlive() || player->HasAura(Spells::RideVehiclePole))
            return true;

        if (!player->HasAura(CurseSpellID))
            player->CastSpell(player, CurseSpellID);

        return true;
    }

    bool OnExit(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        player->RemoveAurasDueToSpell(CurseSpellID);
        return true;
    }
};

// 6986
// 6987
class at_singing_pools_transform_frog : public AreaTriggerScript
{
public:
    at_singing_pools_transform_frog() : AreaTriggerScript("at_singing_pools_transform_frog") {}

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        if (!player->IsAlive() || player->HasAura(Spells::RideVehiclePole))
            return true;

        if (!player->HasAura(Spells::CurseOfTheFrog))
            player->CastSpell(player, Spells::CurseOfTheFrog);

        if (player->HasAura(Spells::TrainingBellPoleExitExclusion))
            player->RemoveAura(Spells::TrainingBellPoleExitExclusion);

        return true;
    }

    bool OnExit(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        player->RemoveAurasDueToSpell(Spells::CurseOfTheFrog);
        return true;
    }
};

// 54135 - Master Li Fei
struct npc_li_fei : public ScriptedAI
{
    npc_li_fei(Creature* creature) : ScriptedAI(creature) {}

    void OnQuestAccept(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == Quests::OnlyTheWorthyShallPass)
            player->CastSpell(player, Spells::FireCrashCover);
    }
};

// 102499 - Fire Crash
class spell_fire_crash : public SpellScript
{
    static constexpr Position PlayerJumpPos = { 1351.3334f, 3939.0347f, 109.32395f, 6.00115f };

    void SetDest(SpellDestination& dest) const
    {
        dest.Relocate(PlayerJumpPos);
    }

    void Register() override
    {
        OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_fire_crash::SetDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
    }
};

// 54734 - Master Li Fei (combat)
struct npc_li_fei_combat : public ScriptedAI
{
    npc_li_fei_combat(Creature* creature) : ScriptedAI(creature), _defeatTriggered(false) {}

    enum Events
    {
        EVENT_FEET_OF_FURY = 1,
        EVENT_SHADOW_KICK,
    };

    void Reset() override
    {
        _defeatTriggered = false;
    }

    void JustEngagedWith(Unit* /*attacker*/) override
    {
        _events.ScheduleEvent(EVENT_FEET_OF_FURY, 6s);
        _events.ScheduleEvent(EVENT_SHADOW_KICK, 10s);
    }

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
    {
        if (!_defeatTriggered && me->HealthBelowPctDamaged(50, damage))
        {
            _defeatTriggered = true;
            me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);

            Creature* liFei = me->FindNearestCreatureWithOptions(15.0f, { .CreatureId = Creatures::MasterLiFei, .IgnorePhases = true });
            if (!liFei)
                return;

            for (ObjectGuid const& guid : me->GetTapList())
            {
                Player* player = ObjectAccessor::GetPlayer(*me, guid);
                if (!player)
                    continue;

                player->KilledMonsterCredit(Creatures::MasterLiFeiCombat, ObjectGuid::Empty);
                player->RemoveAurasDueToSpell(Spells::FireCrashCover);
                player->RemoveAurasDueToSpell(Spells::FireCrashInvis);
                player->RemoveAurasDueToSpell(Spells::FireCrashPhaseShift);
            }

            liFei->AI()->Talk(Talks::LiFeiDefeat, attacker);

            EnterEvadeMode();
        }
    }

    void KilledUnit(Unit* victim) override
    {
        Player* player = victim->ToPlayer();
        if (!player)
            return;

        player->FailQuest(Quests::OnlyTheWorthyShallPass);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_FEET_OF_FURY:
                    DoCastVictim(Spells::FeetOfFury);

                    _events.ScheduleEvent(EVENT_FEET_OF_FURY, 12s);
                    break;
                case EVENT_SHADOW_KICK:
                    DoCastVictim(Spells::FlyingShadowKick);

                    _events.ScheduleEvent(EVENT_SHADOW_KICK, 15s);
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap _events;
    bool _defeatTriggered;
};

// 108958 - Feet of Fury
class spell_feet_of_fury : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ Spells::FeetOfFuryDamage });
    }

    void PeriodicTick(AuraEffect const* aurEff)
    {
        PreventDefaultAction();

        Unit* target = GetTarget();
        target->CastSpell(target->GetVictim(), Spells::FeetOfFuryDamage, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
            .TriggeringAura = aurEff
        });
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_feet_of_fury::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

// 108936 - Flying Shadow Kick
class spell_flying_shadow_kick : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ Spells::FlyingShadowKickJump });
    }

    void HandleHitTarget(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(GetHitUnit(), Spells::FlyingShadowKickJump, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
        });
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_flying_shadow_kick::HandleHitTarget, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};
}




// Quest: 29423 The Passion of Shen-zin Su
// OnQuestAccept the Huo entry 54787 spawns 54958

enum ThePassionOfShenZinSu
{
    QUEST_THE_PASSION_OF_SHENZIN_SU = 29423,

    NNPC_HUO_FOLLOWER = 54958,
    NPC_MASTER_SHANG_XI = 54786,
    NPC_FIRE_SPIRIT_CREDIT = 61128,
    NPC_JI_FIREPAW = 57720,
    NPC_AYSA_CLOUDSINGER = 57721,
    NPC_CHIA_HUI = 60248,
    NPC_BREWER_LIN = 60253,

    SPELL_BLESSING_OF_HUO = 102630,
    SPELL_SPAWN_SPIRIT_OF_FIRE = 128700,

    SAY_MASTER_SHANG_00 = 0,
    SAY_MASTER_SHANG_01 = 1,
    SAY_MASTER_SHANG_02 = 2,
    SAY_MASTER_SHANG_03 = 3,
    SAY_MASTER_SHANG_04 = 4,
    SAY_MASTER_SHANG_05 = 5,
    SAY_MASTER_SHANG_06 = 6,
    SAY_JI_FIREPAW_00 = 0,
    SAY_AYSA_CLOUDSINGER_00 = 0,
    SAY_NPC_HUO_NEARBY_00 = 0,

    EVENT_MASTER_SHANG_00 = 1,
    EVENT_MASTER_SHANG_01 = 2,
    EVENT_MASTER_SHANG_02 = 3,
    EVENT_MASTER_SHANG_03 = 4,
    EVENT_MASTER_SHANG_04 = 5,
    EVENT_MASTER_SHANG_05 = 6,
    EVENT_MASTER_SHANG_06 = 7,
    EVENT_JI_FIREPAW_00 = 8,
    EVENT_AYSA_CLOUDSINGER_00 = 9
};

// Quest: 29776 morning-breeze-village
// OnQuestAccept Master Shang casts 104396 on player
struct npc_master_shang : public ScriptedAI
{
    npc_master_shang(Creature* creature) : ScriptedAI(creature) { }

    EventMap events;
    std::set<ObjectGuid> triggeredPlayers;
    ObjectGuid _playerGUID;

    void Reset() override
    {
        events.Reset();
    }

    void OnQuestAccept(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == 29776)
        {
            if (!player)
                return;
            else
                player->CastSpell(player, 104396);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);

            std::list<Player*> players;
            Trinity::AnyPlayerInPositionRangeCheck check(me, 30.0f);
            Trinity::PlayerListSearcher<Trinity::AnyPlayerInPositionRangeCheck> searcher(me, players, check);
            Cell::VisitAllObjects(me, searcher, 30.0f);

            for (Player* player : players)
            {
                if (triggeredPlayers.contains(player->GetGUID()))
                    continue;
                _playerGUID = player->GetGUID();
                Creature* shang = player->FindNearestCreatureWithOptions(30.0f, { .StringId = "master_shang_trigger" });
                if (!shang || shang != me)
                    continue;
                if (player->GetQuestStatus(QUEST_THE_PASSION_OF_SHENZIN_SU) == QUEST_STATUS_COMPLETE)
                {
                    triggeredPlayers.insert(player->GetGUID());
                    me->SummonCreature(NPC_JI_FIREPAW, 963.893372f, 3607.619873f, 196.557587f, 3.430162f, TEMPSUMMON_TIMED_DESPAWN, 120s);
                    me->SummonCreature(NPC_AYSA_CLOUDSINGER, 964.222107f, 3603.798340f, 196.516296f, 2.761167f, TEMPSUMMON_TIMED_DESPAWN, 120s);
                    events.ScheduleEvent(EVENT_MASTER_SHANG_00, 1s);
                    break;
                }
            }
        
        while (uint32 eventId = events.ExecuteEvent())
        {
            Player* talkPlayer = ObjectAccessor::GetPlayer(*me, _playerGUID);
            switch (eventId)
            {
            case EVENT_MASTER_SHANG_00:
                Talk(SAY_MASTER_SHANG_00, talkPlayer);
                events.ScheduleEvent(EVENT_MASTER_SHANG_01, 4s);
                break;
            case EVENT_MASTER_SHANG_01:
                Talk(SAY_MASTER_SHANG_01, talkPlayer);
                events.ScheduleEvent(EVENT_MASTER_SHANG_02, 4s);
                break;
            case EVENT_MASTER_SHANG_02:
                Talk(SAY_MASTER_SHANG_02, talkPlayer);
                events.ScheduleEvent(EVENT_MASTER_SHANG_03, 4s);
                break;
            case EVENT_MASTER_SHANG_03:
                Talk(SAY_MASTER_SHANG_03, talkPlayer);
                events.ScheduleEvent(EVENT_MASTER_SHANG_04, 4s);
                break;
            case EVENT_MASTER_SHANG_04:
                Talk(SAY_MASTER_SHANG_04, talkPlayer);
                SignalNearbyCreature(NPC_JI_FIREPAW, SAY_JI_FIREPAW_00);
                events.ScheduleEvent(EVENT_MASTER_SHANG_05, 4s);
                break;
            case EVENT_MASTER_SHANG_05:
                Talk(SAY_MASTER_SHANG_05, talkPlayer);
                SignalNearbyCreature(NPC_AYSA_CLOUDSINGER, SAY_AYSA_CLOUDSINGER_00);
                events.ScheduleEvent(EVENT_MASTER_SHANG_06, 4s);
                break;
            case EVENT_MASTER_SHANG_06:
                Talk(SAY_MASTER_SHANG_06, talkPlayer);
                break;
            }
        }
    }

    void SignalNearbyCreature(uint32 entry, uint32 sayLine)
    {
        if (Creature* npc = me->FindNearestCreature(entry, 10.0f, true))
        {
            if (npc->AI())
                npc->AI()->Talk(sayLine);
            else
                TC_LOG_INFO("scripts.ai", "AI is null for creature {}", entry);
        }
        else
        {
            TC_LOG_INFO("scripts.ai", "Could not find creature {} nearby", entry);
        }
    }
};




// Quest: 29678 Shu, the Spirit of Water
// Spell: 103071 Forcecast Rock Jump A
// Spell: 103069 Rock Jump A -- this gets the scriptname
// Spell: 103070 Rock Jump B -- this gets the scriptname
// Spell: 103072 Forcecast Rock Jump B
// Spell: 103077 Rock Jump C -- this gets the scriptname
// Spell: 103078 Forcecast Rock Jump C




enum ANEWFRIEND
{
    QUEST_A_NEW_FRIEND = 29678,

    NPC_AYSA_CLOUDSINGER_SHU = 54975,

    SPELL_AYSA_CONGRATS_TIMER = 128589,
    SPELL_AYSA_CONGRATS_TRIGGER_AURA = 128588,
    SPELL_SUMMON_SPIRIT_OF_WATER = 103538,
    SPELL_WATER_SPOUT_QUEST_CREDIT = 117054 // this already works as expected

};




//STEFAL TODO: add correct usage of the ropes spells
enum TheSourceofOurLivelihood
{
    QUEST_THE_SOURCE_OF_LIVELIHOOD = 29680,

    NPC_CART = 57710,
    NPC_OX = 57712,
    NPC_VEHICLE_CART = 57208,
    NPC_VEHICLE_OX = 57207,
    NPC_VEHICLE_CART_FARMSTEAD = 59496,
    NPC_VEHICLE_OX_FARMSTEAD = 59498,
    NPC_CART_TENDER = 57712,

    SPELL_FORCE_VEHICLE_RIDE = 46598, //cast on the player
    SPELL_EJECT_PASSENGERS = 50630,
    SPELL_ROPE_LEFT = 108627, //cast on the cart
    SPELL_ROPE_RIGHT = 108691, //cast on the cart?
    SPELL_REMOVE_OX_CART_INVIS = 108888, // not needed as they are handled by spawn spell(?)
    SPELL_ADD_OX_CART_INVIS = 108887, // not needed as they are handled by spawn spell(?)

    PATH_CART_VEHICLE = 3,
    PATH_OX_VEHICLE = 2,
    PATH_CART_VEHICLE_FARMSTEAD = 11,
    PATH_OX_VEHICLE_FARMSTEAD = 12,

    NODE_DESPAWN = 33,
    NODE_REMOVE_PASSENGER = 27,

    EVENT_START_PATH = 1,
    EVENT_CAST_ROPE_LEFT = 2,
    EVENT_CAST_ROPE_RIGHT = 3,

    AREA_CART_LOCATION_1 = 7258,
    AREA_CART_LOCATION_2 = 7822,

    SAY_CART_TENDER_00 = 0,
    SAY_CART_TENDER_01 = 1

};





enum ShuAtTheFarm
{
    SPELL_SHU_SPLASH = 107030,

    PATH_SHU_AT_THE_FARM = 4,

    NODE_SHU_RUN_DISABLE = 4,
    NODE_SHU_DESPAWN = 5,

    EVENT_SHU_START_PATH = 1,
    EVENT_SHU_CHECK_QUEST_REWARDED = 2
};

enum ShuAtTheFarmsteadPool
{
    QUEST_NOT_IN_THE_FACE = 29774,

    NPC_SHU_AT_THE_POOL = 55556, //This script is applied to this creature entry
    NPC_WUGOU_NEAR_GONG = 55539,
    NPC_QUEST_29774_CREDIT_1 = 55548,
    NPC_QUEST_29774_CREDIT_2 = 55547,
    NPC_WATER_SPOUT_AT_FARM_POOL = 66941,

    GOSSIP_MENU_SHU_AT_THE_POOL = 13140,
    GOSSIP_ITEM_SHU_AT_THE_POOL = 0,

    PATH_SHU_QUEST_29774_1 = 8,
    PATH_SHU_QUEST_29774_2 = 9,

    NODE_SHU_PATH1_END = 5,
    NODE_SHU_PATH2_END = 7,


    EVENT_SHU_AT_THE_POOL_FIRST_WP = 1,
    EVENT_SHU_AT_THE_POOL_SECOND_WP = 2,
    EVENT_WUGOU_WAKEUP = 3,
    EVENT_SHU_QUEST_CHECK = 4,
    EVENT_SHU_AT_FARMSTEAD_PLAY = 6,

    SPELL_SHU_WATERSPLASH_ON_WUGOU = 118034,
    SPELL_SHU_WATERSPLASH_CREDIT = 104023,
    SPELL_WATER_SPIRIT_LAUGH = 118035,
    SPELL_SHU_WATERSPLASH = 118027,
    SPELL_AURA_SLEEP = 42386,
    SPELL_AURA_INVIS = 80797
};

enum ShuWugouToTheTempleFollow
{
    QUEST_THE_SPIRIT_AND_BODY_OF_SHENZINSU = 29775,

    NPC_SHU_SPAWNED_FOR_TEMPLE = 55558,
    NPC_WUGOU_SPAWNED_FOR_TEMPLE = 60916,

    NODE_SHU_PATH3_END = 7,
    PATH_SHU_QUEST_29775_1 = 10,
    EVENT_SHU_AT_THE_POOL_THIRD_WP = 5,

    SPELL_SUMMON_SPIRIT_OF_WATER_EARTH = 104017, //summons both spirits
    SPELL_SUMMON_SPIRIT_OF_EARTH = 118036,
    SPELL_WUGOU_SLEEP = 42386,

    AREA_TEMPLE_STAIRS_FROM_FARMSTEAD = 7116
};



//104450/summon-ji-yuan
enum SpellSummonJiAtTemple
{
    SPELL_SUMMON_JI_AT_TEMPLE = 104450, // this script spell

    NPC_JI_FIREPAW_AT_TEMPLE_SPIRE = 55694
};













enum MonkeyWisdomTexts
{
    TEXT_MONKEY_WISDOM = 54073,
    TEXT_MONKEY_WISDOM_2 = 54074,
    TEXT_MONKEY_WISDOM_3 = 54075,
    TEXT_MONKEY_WISDOM_4 = 54076,
    TEXT_MONKEY_WISDOM_5 = 54077,
    TEXT_MONKEY_WISDOM_6 = 54078,
    TEXT_MONKEY_WISDOM_7 = 54079,
    TEXT_MONKEY_WISDOM_8 = 54080
};

// Spell 104126 Monkey Wisdom
class spell_monkey_wisdom_text : public SpellScript
{
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sBroadcastTextStore.LookupEntry(TEXT_MONKEY_WISDOM) || !sBroadcastTextStore.LookupEntry(TEXT_MONKEY_WISDOM_2) ||
                !sBroadcastTextStore.LookupEntry(TEXT_MONKEY_WISDOM_3) || !sBroadcastTextStore.LookupEntry(TEXT_MONKEY_WISDOM_4) ||
                !sBroadcastTextStore.LookupEntry(TEXT_MONKEY_WISDOM_5) || !sBroadcastTextStore.LookupEntry(TEXT_MONKEY_WISDOM_6) ||
                !sBroadcastTextStore.LookupEntry(TEXT_MONKEY_WISDOM_7) || !sBroadcastTextStore.LookupEntry(TEXT_MONKEY_WISDOM_8))
                return false;
            return true;
        }

        bool Load() override
        {
            return GetCaster()->GetTypeId() == TYPEID_PLAYER;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            uint32 randomText = urand(0, 7);

            GetCaster()->Talk(TEXT_MONKEY_WISDOM + randomText, CHAT_MSG_RAID_BOSS_WHISPER, 0.0f, GetHitPlayer());
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_monkey_wisdom_text::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
};

enum RukRukEncounter
{
    SPELL_OOKSPLOSIONS_TRIGGERED = 125885,
    SPELL_AIM = 125609,
    SPELL_OOKSPLOSIONS = 125699,
    SPELL_AIM_VISUAL = 26079,

    EVENT_AIM = 1,
    EVENT_OOKSPLOSIONS = 2
};

class spell_ruk_ruk_ooksplosions : public AuraScript
{
    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        float radius = caster->GetBoundingRadius();
        float x, y, z;
        caster->GetClosePoint(x, y, z, radius, frand(0.0f, 3.0f), frand(0.0f, 2 * float(M_PI)));

        Position pos(x, y, z);
        CastSpellTargetArg target(pos);

        caster->CastSpell(target, SPELL_OOKSPLOSIONS_TRIGGERED, true);
    }

    void Register() override
    {
         OnEffectPeriodic += AuraEffectPeriodicFn(spell_ruk_ruk_ooksplosions::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

struct npc_ruk_ruk : public ScriptedAI
{
    npc_ruk_ruk(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_AIM, 10s);
        _events.ScheduleEvent(EVENT_OOKSPLOSIONS, 30s);
    }

    Position GetRocketTargetPos() const
    {
        return _pos;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_AIM:
                if (me->HasUnitState(UNIT_STATE_CASTING))
                {
                    _events.RescheduleEvent(EVENT_AIM, 1s);
                    break;
                }
                if (Unit* target = SelectTarget(SelectTargetMethod::Random))
                {
                    me->SetFacingToObject(target);
                    CalculateSpellVisual(target);
                    DoCast(target, SPELL_AIM);
                    _events.ScheduleEvent(EVENT_AIM, 15s, 25s);
                }
                break;
            case EVENT_OOKSPLOSIONS:
                if (me->HasUnitState(UNIT_STATE_CASTING))
                {
                    _events.RescheduleEvent(EVENT_OOKSPLOSIONS, 1s);
                    break;
                }
                DoCast(SPELL_OOKSPLOSIONS);
                _events.ScheduleEvent(EVENT_OOKSPLOSIONS, 25s, 35s);
                break;
            }
        }
        me->DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
    Position _pos;

    void CalculateSpellVisual(Unit* target)
    {
        float ori = me->GetOrientation();
        float z = me->GetPositionZ();
        float targetDist = target->GetExactDist(me->GetPosition());

        for (int radius = 1; ; radius++)
        {
            if (radius <= ceilf(targetDist))
            {
                float x = me->GetPositionX() + radius * cos(ori);
                float y = me->GetPositionY() + radius * sin(ori);
                me->UpdateGroundPositionZ(x, y, z);
                _pos = { x, y, z };
                me->SendPlaySpellVisual(_pos, SPELL_AIM_VISUAL, 0, 0, 0.0f, false, 2.0f);
            }
            else
                break;
        }
    }
};

enum RukRukRocketEvent
{
    SPELL_EXPLOSSION_VISUAL = 125612,
    SPELL_EXPLOSSION_DMG = 125619,

    EVENT_FIRE = 1
};

struct npc_ruk_ruk_rocket : public ScriptedAI
{
    npc_ruk_ruk_rocket(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        me->SetDisplayFromModel(0);
        _events.ScheduleEvent(EVENT_FIRE, 500ms);
    }

    void IsSummonedBy(WorldObject* summonerWO) override
    {
        if (Creature* summoner = summonerWO->ToCreature())
        {
            if (npc_ruk_ruk* rukRukAI = dynamic_cast<npc_ruk_ruk*>(summoner->AI()))
            {
                _rocketTargetPos = rukRukAI->GetRocketTargetPos();

                if (me->GetExactDist2d(_rocketTargetPos) > 30.0f)
                    RecalculateTargetPos();
            }
        }
    }


    void MovementInform(uint32 /*type*/, uint32 id) override
    {
        if (id == 1)
        {
            DoCast(SPELL_EXPLOSSION_VISUAL);
            DoCast(SPELL_EXPLOSSION_DMG);
            me->DespawnOrUnsummon(1s);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_FIRE:
                me->GetMotionMaster()->MovePoint(1, _rocketTargetPos);
                break;
            }
        }
    }

private:
    EventMap _events;
    Position _rocketTargetPos;

    void RecalculateTargetPos()
    {
        float ori = me->GetOrientation();
        float x = me->GetPositionX() + 30 * cos(ori);
        float y = me->GetPositionY() + 30 * sin(ori);
        float z = me->GetPositionZ();
        me->UpdateGroundPositionZ(x, y, z);
        _rocketTargetPos = { x, y, z };
    }
};

enum ChamberOfWhispers
{
    QUEST_DAFENG_THE_SPIRIT_OF_AIR = 29785,
    QUEST_BATTLE_FOR_THE_SKIES = 29786,

    QUEST_29785_KILLCREDIT = 55666,

    NPC_DAFENG_CHAMBER_WHISPERS = 55592,
    NPC_AYSA_CHAMBER_WHISPERS = 55595,
    NPC_JI_FIREPAW_CHAMBER_OF_WHISPERS = 64505,
    NPC_AYSA_CLOUDSINGER_CHAMBER_OF_WHISPERS = 64506,
    NPC_ZHAOREN = 55786,

    AREA_CHAMBER_OF_WHISPERS = 7037, // This area script

    PHASE_CHAMBER_OF_ASPECTS = 524
};

class at_chamber_of_whispers : public AreaTriggerScript
{
public:
    at_chamber_of_whispers() : AreaTriggerScript("at_chamber_of_whispers") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        if (player->IsAlive() && player->IsActiveQuest(QUEST_DAFENG_THE_SPIRIT_OF_AIR))
        {
            Creature* dafeng = GetClosestCreatureWithEntry(player, NPC_DAFENG_CHAMBER_WHISPERS, 20.0f);
            Creature* aysha = GetClosestCreatureWithEntry(player, NPC_AYSA_CHAMBER_WHISPERS, 20.0f);

            PhasingHandler::AddPhase(player, PHASE_CHAMBER_OF_ASPECTS, true);

            if(dafeng)
                PhasingHandler::AddPhase(dafeng, PHASE_CHAMBER_OF_ASPECTS, true);

            if (aysha)
            {
                PhasingHandler::AddPhase(aysha, PHASE_CHAMBER_OF_ASPECTS, true);
                aysha->AI()->Talk(0);
            }

            player->KilledMonsterCredit(QUEST_29785_KILLCREDIT);



            return true;
        }
        return false;
    }

    bool OnExit(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        if (player->IsAlive() && player->IsActiveQuest(QUEST_BATTLE_FOR_THE_SKIES))
        {
            Creature* ji = player->SummonCreature(NPC_JI_FIREPAW_CHAMBER_OF_WHISPERS, { 691.354f,	4152.99f,	197.629f, 0.59088f }, TEMPSUMMON_MANUAL_DESPAWN);
            Creature* aysa = player->SummonCreature(NPC_AYSA_CLOUDSINGER_CHAMBER_OF_WHISPERS, { 715.858f, 4165.51f, 196.064f, 5.85696f }, TEMPSUMMON_MANUAL_DESPAWN);

            if(ji && aysa)
                player->SummonCreature(NPC_ZHAOREN, { 713.917f,	4168.13f,	213.846f,	2.70526f }, TEMPSUMMON_MANUAL_DESPAWN);
            return true;
        }
        return false;
    }
};

enum ZhaorenEvents
{
    EVENT_LIGHTNING = 1,
    EVENT_MOVE_CENTER = 2,
    EVENT_STUN = 3,
    EVENT_SWEEP = 4,
    EVENT_RESUME_WP = 5
};

enum ZhaorenSpells
{
    SPELL_LIGHTNING_POOL = 126006,
    SPELL_STUNNED_BY_FIREWORKS = 125992,
    SPELL_SERPENT_SWEEP = 125990,
    SPELL_FORCECAST_SUMMON_SHANG = 128808,
    SPELL_OVERPACKED_FIREWORK = 104855,
    SPELL_FIREWORK_INACTIVE = 125964
};

enum ZhaorenPhases
{
    PHASE_FLYING = 1,
    PHASE_GROUNDED = 2,
    PHASE_STAY_IN_CENTER = 3
};

enum ZhaorenMisc
{
    ZHAOREN_PATH = 16,
    NPC_DAFENG = 64532,
    NPC_FIREWORK = 64507,
    NPC_DEAD_ZHAOREN = 55874,
    NPC_MASTER_SHANG_CHAMBER_OF_WHISPERS = 55586,
    DATA_1 = 1,
    DATA_COMBAT = 2,
    DATA_AYSA_TALK_3 = 3,
    DATA_PHASE_OOC = 4,
    DATA_ZHAOREN_DEATH = 5,
    DATA_EVADE = 6
};

Position ZhaoPos[] =
{
    {699.134f, 4170.06f, 216.06f}, // Center
};

//55786
struct npc_zhaoren : public ScriptedAI
{
    npc_zhaoren(Creature* creature) : ScriptedAI(creature), phase(0), _sweepScheduled(false) { }

    Position const pos = { 723.163f, 4163.8f, 204.999f };

private:
    uint8 phase;
    bool _sweepScheduled;
    EventMap events;

    void Reset() override
    {
        events.Reset();
        //me->SetReactState(REACT_PASSIVE);
        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
        phase = 0;
        _sweepScheduled = false;

        if (Creature* ji = me->FindNearestCreature(NPC_JI_FIREPAW_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
        {
            //creature->AI()->SetData(DATA_EVADE, DATA_EVADE);
            ji->SetReactState(REACT_AGGRESSIVE);
            ji->AI()->AttackStart(me);
        }

        if (Creature* aysa = me->FindNearestCreature(NPC_AYSA_CLOUDSINGER_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
        {
            //creature->AI()->SetData(DATA_EVADE, DATA_EVADE);
            aysa->SetReactState(REACT_AGGRESSIVE);
            aysa->AI()->AttackStart(me);
        }

        std::list<Creature*> fireworks;
        me->GetCreatureListWithEntryInGrid(fireworks, NPC_FIREWORK, me->GetVisibilityRange());
        for (std::list<Creature*>::iterator itr = fireworks.begin(); itr != fireworks.end(); ++itr)
        {
            (*itr)->RemoveAura(SPELL_FIREWORK_INACTIVE);
            (*itr)->AI()->SetData(DATA_1, DATA_1);
        }
        me->GetMotionMaster()->Clear();
        me->GetMotionMaster()->MovePoint(0, ZhaoPos[0].GetPositionX(), ZhaoPos[0].GetPositionY(), ZhaoPos[0].GetPositionZ());
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->GetMotionMaster()->MovePath(ZHAOREN_PATH, true);
        events.SetPhase(PHASE_FLYING);
        events.ScheduleEvent(EVENT_LIGHTNING, 5s);
    }

    void SpellHit(WorldObject* caster, SpellInfo const* spell) override
    {
        if (spell->Id == SPELL_OVERPACKED_FIREWORK)
        {
            if (!me->IsInCombat())
            {
                if (Unit* target = caster->ToUnit())
                {
                    me->Attack(target, true);
                }
            }
        }
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == EVENT_MOVE_CENTER)
            events.ScheduleEvent(EVENT_STUN, 0s);
    }

    void KilledUnit(Unit* who) override
    {
        if (who->IsPlayer())
        {
            if (me->GetThreatManager().IsThreatListEmpty(true))
            {
                if (Creature* ji = me->FindNearestCreature(NPC_JI_FIREPAW_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
                    ji->DespawnOrUnsummon(5s);
                if (Creature* aysa = me->FindNearestCreature(NPC_AYSA_CLOUDSINGER_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
                    aysa->DespawnOrUnsummon(5s);
                me->DespawnOrUnsummon(10s);

            }
        }
    }


    void JustDied(Unit* /*killer*/) override
    {
        //DoCastAOE(SPELL_FORCECAST_SUMMON_SHANG, true);

        Position const shangPos = { 711.335f, 4178.049f, 197.845f };
        Position const deadPos = { 723.163f, 4163.799f, 196.341f };

        if (Creature* ji = me->FindNearestCreature(NPC_JI_FIREPAW_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
            ji->DespawnOrUnsummon(1s);
        if (Creature* aysa = me->FindNearestCreature(NPC_AYSA_CLOUDSINGER_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
            aysa->DespawnOrUnsummon(1s);
        if (Creature* creature = me->FindNearestCreature(NPC_DAFENG, me->GetVisibilityRange(), true))
            creature->AI()->SetData(DATA_ZHAOREN_DEATH, DATA_ZHAOREN_DEATH);

        me->SummonCreature(NPC_MASTER_SHANG_CHAMBER_OF_WHISPERS, shangPos, TEMPSUMMON_TIMED_DESPAWN, 5min);
        me->SummonCreature(NPC_DEAD_ZHAOREN, deadPos, TEMPSUMMON_TIMED_DESPAWN, 5min);
        me->DespawnOrUnsummon(1s);
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);
        if (phase == 0 && HealthBelowPct(85))
        {
            phase++;
            if (Creature* creature = me->FindNearestCreature(NPC_AYSA_CLOUDSINGER_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
                creature->AI()->SetData(DATA_1, DATA_1);
        }
        if (phase == 1 && HealthBelowPct(75))
        {
            phase++;
            events.SetPhase(PHASE_GROUNDED);
            events.CancelEvent(EVENT_LIGHTNING);
            events.ScheduleEvent(EVENT_MOVE_CENTER, 0s);
        }
        if (phase == 2 && HealthBelowPct(25))
        {
            phase++;
            events.SetPhase(PHASE_STAY_IN_CENTER);
            events.CancelEvent(EVENT_LIGHTNING);
            events.ScheduleEvent(EVENT_MOVE_CENTER, 0s);
        }
        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_LIGHTNING:
            {
                auto const& threatList = me->GetThreatManager().GetUnsortedThreatList();
                if (threatList.begin() != threatList.end())
                {
                    for (ThreatReference const* ref : threatList)
                    {
                        if (Unit* target = ref->GetVictim())
                        {
                            if (target->IsPlayer())
                                DoCast(target, SPELL_LIGHTNING_POOL);
                        }
                    }

                    events.ScheduleEvent(EVENT_LIGHTNING, events.IsInPhase(PHASE_FLYING) ? 5s : 3500ms);

                    if (!_sweepScheduled && events.IsInPhase(PHASE_STAY_IN_CENTER))
                    {
                        events.ScheduleEvent(EVENT_SWEEP, 15s, 0, PHASE_STAY_IN_CENTER);
                        _sweepScheduled = true;
                    }
                }
                else
                {
                    if (Creature* ji = me->FindNearestCreature(NPC_JI_FIREPAW_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
                        ji->DespawnOrUnsummon(5s);
                    if (Creature* aysa = me->FindNearestCreature(NPC_AYSA_CLOUDSINGER_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
                        aysa->DespawnOrUnsummon(5s);
                    me->DespawnOrUnsummon(10s);
                }
                break;
            }
            case EVENT_MOVE_CENTER:
                me->GetMotionMaster()->MovePoint(EVENT_MOVE_CENTER, pos);
                break;

            case EVENT_STUN:
                DoCast(SPELL_STUNNED_BY_FIREWORKS);
                events.ScheduleEvent(EVENT_SWEEP, 12s);
                if (Creature* creature = me->FindNearestCreature(NPC_AYSA_CLOUDSINGER_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
                {
                    if (phase == 2)
                        creature->AI()->SetData(DATA_COMBAT, DATA_COMBAT);
                    else if (phase == 3)
                        creature->AI()->SetData(DATA_AYSA_TALK_3, DATA_AYSA_TALK_3);
                }
                if (Creature* creature = me->FindNearestCreature(NPC_JI_FIREPAW_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
                    creature->AI()->SetData(DATA_COMBAT, DATA_COMBAT);
                break;

            case EVENT_SWEEP:
                events.CancelEvent(EVENT_LIGHTNING);
                DoCast(SPELL_SERPENT_SWEEP);
                _sweepScheduled = false;
                events.ScheduleEvent(EVENT_LIGHTNING, 3500ms, 0, PHASE_STAY_IN_CENTER);
                events.ScheduleEvent(EVENT_RESUME_WP, 5s, 0, PHASE_GROUNDED);
                if (events.IsInPhase(PHASE_GROUNDED))
                    if (Creature* creature = me->FindNearestCreature(NPC_JI_FIREPAW_CHAMBER_OF_WHISPERS, me->GetVisibilityRange(), true))
                        creature->AI()->SetData(DATA_PHASE_OOC, DATA_PHASE_OOC);
                break;
            case EVENT_RESUME_WP:
                me->GetMotionMaster()->MovePath(ZHAOREN_PATH, true);
                events.SetPhase(PHASE_FLYING);
                events.ScheduleEvent(EVENT_LIGHTNING, 5s);
                break;
            }
        }
    }
};

enum SpellSummonShangWorthyOfPassing
{
    SPELL_SUMMON_SHANG_WORTHY_OF_PASSING = 105333, // this script spell

    NPC_SHANG_WORTHY_OF_PASSING = 56159
};

class spell_summon_worthy_of_passing : public SpellScript
{
    void HandleLaunch(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
    }

    void HandleSummon(SpellEffIndex /*effIndex*/)
    {
        PreventHitDefaultEffect(EFFECT_0);

        if (Unit* caster = GetCaster())
        {

            Position const shangPos = { 711.335f, 4178.049f, 197.845f };
            caster->SummonCreature(NPC_SHANG_WORTHY_OF_PASSING, shangPos, TEMPSUMMON_MANUAL_DESPAWN);
        }
    }

    void Register() override
    {
        OnEffectLaunch += SpellEffectFn(spell_summon_worthy_of_passing::HandleLaunch, EFFECT_0, SPELL_EFFECT_SUMMON);
        OnEffectHit += SpellEffectFn(spell_summon_worthy_of_passing::HandleSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
    }
};

enum EventHotAirBalloon
{
    SPELL_AISUMMON_HOT_AIR_BALLOON = 128815, // this spell script
    SPELL_SUMMON_HOT_AIR_BALLOON = 105002, 

    NPC_HOT_AIR_BALLOON = 55918,
    NPC_HOT_AIR_BALLOON_FROM_SPELL = 55649,
    NPC_AYSA_AT_BALLOON_SPAWN = 56661,
    NPC_AYSA_AT_BALLOON = 56662
};

enum ShenZinShuBunnySpells
{
    SPELL_TRIGGER_WITH_ANIM_0 = 114898,
    SPELL_TRIGGER = 106759,
    SPELL_TRIGGER_WITH_ANIM_1 = 118571,
    SPELL_TRIGGER_WITH_TURN = 118572
};

enum ShenZinShuBunnyTexts
{
    TEXT_1 = 55550,
    TEXT_2 = 55568,
    TEXT_3 = 55569,
    TEXT_4 = 55570,
    TEXT_5 = 55572,
    TEXT_6 = 63407
};

struct npc_hot_air_balloon_from_spell : public ScriptedAI
{
    npc_hot_air_balloon_from_spell(Creature* creature) : ScriptedAI(creature), _passengerGuid() { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        _passengerGuid = summoner->GetGUID(); // Store for later use (e.g., for eject)
        TC_LOG_DEBUG("scripts.ai", "Balloon summoned by GUID: {}", _passengerGuid.ToString());

        summoner->CastSpell(me, 46598, true);

/*        if (Creature* aysa = GetClosestCreatureWithEntry(me, NPC_AYSA_AT_BALLOON_SPAWN, 20.0f))
        {
            TC_LOG_DEBUG("scripts.ai", "We have found Aysa spawned from spell");
            if (aysa->AI()->DoCast(me, 63313))
                TC_LOG_DEBUG("scripts.ai", "We have AI Aysa spell cast on balloon");
            else if(aysa->CastSpell(me, 63313, true))
                TC_LOG_DEBUG("scripts.ai", "We have Aysa spell cast on balloon");
            else
                me->CastSpell(aysa, 106617, true);
        }*/

        if (Creature* balloon = GetClosestCreatureWithEntry(me, NPC_HOT_AIR_BALLOON, 10.0f))
        {
            TC_LOG_DEBUG("scripts.ai", "We have found balloon");
            balloon->DespawnOrUnsummon(0s, 30s);
        }
        else TC_LOG_DEBUG("scripts.ai", "We have NOT found balloon");

        if (Creature* aysha = GetClosestCreatureWithEntry(me, NPC_AYSA_AT_BALLOON, 20.0f))
        {
            TC_LOG_DEBUG("scripts.ai", "We have found npc aysa");
            aysha->DespawnOrUnsummon(0s, 30s);
        }
        else TC_LOG_DEBUG("scripts.ai", "We have NOT found aysha");

        me->SetSpeed(MOVE_RUN, 3.0f);

        me->LoadPath(23);
        me->GetMotionMaster()->MovePath(23, false);
    }

    void Reset() override
    {
        _events.Reset();
        Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
        if (player)
            TC_LOG_DEBUG("scripts.ai", "We have a player in RESET");

        TC_LOG_DEBUG("scripts.ai", "We are now in RESET");




    }

    void WaypointReached(uint32 nodeId, uint32 /*pathId*/) override
    {
        switch (nodeId)
        {
        case 0:
        {
            me->SetSpeed(MOVE_RUN, 9.0f);
            //Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (Vehicle* vehicle = me->GetVehicleKit()) // assuming 'me' is your vehicle creature
            {
                Unit* Aysa = vehicle->GetPassenger(1);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Aysa->ToCreature()->GetEntry());
                if (Creature* aisa = Aysa->ToCreature())
                    aisa->AI()->SetData(2, 1);

                Unit* Ji = vehicle->GetPassenger(2);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Ji->ToCreature()->GetEntry());
                if (Creature* ji = Ji->ToCreature())
                    ji->AI()->SetData(2, 1);
            }
            break;
        }
        case 1:
        {
            //Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (Vehicle* vehicle = me->GetVehicleKit()) // assuming 'me' is your vehicle creature
            {
                Unit* Aysa = vehicle->GetPassenger(1);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Aysa->ToCreature()->GetEntry());
                if (Creature* aisa = Aysa->ToCreature())
                    aisa->AI()->SetData(2, 2);

                Unit* Ji = vehicle->GetPassenger(2);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Ji->ToCreature()->GetEntry());
                if (Creature* ji = Ji->ToCreature())
                    ji->AI()->SetData(2, 2);
            }
            break;
        }
        case 3:
        {
            //Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (Vehicle* vehicle = me->GetVehicleKit()) // assuming 'me' is your vehicle creature
            {
                Unit* Aysa = vehicle->GetPassenger(1);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Aysa->ToCreature()->GetEntry());
                if (Creature* aisa = Aysa->ToCreature())
                    aisa->AI()->SetData(2, 3);

                Unit* Ji = vehicle->GetPassenger(2);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Ji->ToCreature()->GetEntry());
            }
            break;
        }
        case 4:
        {
            Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (player)
            {
                player->CastSpell(player, 114898, true);
                TC_LOG_DEBUG("scripts.ai", "Reached node 4 - cast spell");
            }

            Creature* turtle = GetClosestCreatureWithEntry(me, 56676, 100.0f);
            if (turtle)
            {
                me->CastSpell(turtle, 114898, true);
                TC_LOG_DEBUG("scripts.ai", "Reached node 4 - cast 1st spell on turtle");

                me->CastSpell(turtle, 106759, true);
                TC_LOG_DEBUG("scripts.ai", "Reached node 4 - cast 2nd spell on turtle");
            }
            break;
        }
        case 5:
        {
            //Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (Vehicle* vehicle = me->GetVehicleKit()) // assuming 'me' is your vehicle creature
            {
                Unit* Aysa = vehicle->GetPassenger(1);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Aysa->ToCreature()->GetEntry());
                if (Creature* aisa = Aysa->ToCreature())
                    aisa->AI()->SetData(2, 4);

                Unit* Ji = vehicle->GetPassenger(2);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Ji->ToCreature()->GetEntry());
            }

            Creature* turtle = GetClosestCreatureWithEntry(me, 56676, 100.0f);
            if (turtle)
            {
                me->CastSpell(turtle, 118571, true);
                TC_LOG_DEBUG("scripts.ai", "Reached node 5 - cast 3rd spell on turtle");

                me->CastSpell(turtle, 118571, true);
                TC_LOG_DEBUG("scripts.ai", "Reached node 5 - cast 4th spell on turtle");
            }
            break;
        }
        case 8:
        {
            //Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (Vehicle* vehicle = me->GetVehicleKit()) // assuming 'me' is your vehicle creature
            {
                Unit* Aysa = vehicle->GetPassenger(1);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Aysa->ToCreature()->GetEntry());
                if (Creature* aisa = Aysa->ToCreature())
                    aisa->AI()->SetData(2, 5);

                Unit* Ji = vehicle->GetPassenger(2);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Ji->ToCreature()->GetEntry());
                if (Creature* ji = Ji->ToCreature())
                    ji->AI()->SetData(2, 5);
            }
            break;
        }
        case 11:
        {
            //Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (Vehicle* vehicle = me->GetVehicleKit()) // assuming 'me' is your vehicle creature
            {
                Unit* Aysa = vehicle->GetPassenger(1);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Aysa->ToCreature()->GetEntry());
                if (Creature* aisa = Aysa->ToCreature())
                    aisa->AI()->SetData(2, 7);

                Unit* Ji = vehicle->GetPassenger(2);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Ji->ToCreature()->GetEntry());
                if (Creature* ji = Ji->ToCreature())
                    ji->AI()->SetData(2, 7);
            }
            break;
        }
        case 13:
        {
            //Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (Vehicle* vehicle = me->GetVehicleKit()) // assuming 'me' is your vehicle creature
            {
                Unit* Aysa = vehicle->GetPassenger(1);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Aysa->ToCreature()->GetEntry());
                if (Creature* aisa = Aysa->ToCreature())
                    aisa->AI()->SetData(2, 8);

                Unit* Ji = vehicle->GetPassenger(2);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Ji->ToCreature()->GetEntry());
                if (Creature* ji = Ji->ToCreature())
                    ji->AI()->SetData(2, 8);
            }
            break;
        }
        case 17:
        {
            //Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (Vehicle* vehicle = me->GetVehicleKit()) // assuming 'me' is your vehicle creature
            {
                Unit* Aysa = vehicle->GetPassenger(1);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Aysa->ToCreature()->GetEntry());
                if (Creature* aisa = Aysa->ToCreature())
                    aisa->AI()->SetData(2, 9);

                Unit* Ji = vehicle->GetPassenger(2);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Ji->ToCreature()->GetEntry());
            }
            break;
        }
        case 20:
        {
            //Player* player = ObjectAccessor::GetPlayer(*me, _passengerGuid);
            if (Vehicle* vehicle = me->GetVehicleKit()) // assuming 'me' is your vehicle creature
            {
                Unit* Aysa = vehicle->GetPassenger(1);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Aysa->ToCreature()->GetEntry());
                if (Creature* aisa = Aysa->ToCreature())
                    aisa->AI()->SetData(3, 1);

                Unit* Ji = vehicle->GetPassenger(2);
                TC_LOG_DEBUG("scripts.ai", "We have passenger {}", Ji->ToCreature()->GetEntry());
            }
            break;
        }
        case NODE_DESPAWN:
        {
            me->DespawnOrUnsummon(1s);
            break;
        }
        default:
            break;
        }
    }

private:
    EventMap _events;
    ObjectGuid _passengerGuid;
};

struct npc_shenzinsu : public ScriptedAI
{
    npc_shenzinsu(Creature* creature) : ScriptedAI(creature) { }

    void Initialize()
    {
        _hitCount = 0;
    }

    void Reset() override
    {
        Initialize();
        me->setActive(true);
    }

    void SpellHit(WorldObject* caster, SpellInfo const* spell) override
    {
        switch (spell->Id)
        {
        case SPELL_TRIGGER_WITH_ANIM_0:
            me->Talk(TEXT_1, CHAT_MSG_MONSTER_SAY, 300.0f, caster);
            me->PlayDirectSound(27822, caster->ToPlayer());
            break;
        case SPELL_TRIGGER:
            me->Talk(TEXT_2, CHAT_MSG_MONSTER_SAY, 300.0f, caster);
            me->PlayDirectSound(27823, caster->ToPlayer());
            break;
        case SPELL_TRIGGER_WITH_ANIM_1:
            if (_hitCount == 0)
            {
                me->Talk(TEXT_3, CHAT_MSG_MONSTER_SAY, 300.0f, caster);
                me->PlayDirectSound(27824, caster->ToPlayer());
                _hitCount++;
            }
            else if (_hitCount == 1)
            {
                me->Talk(TEXT_4, CHAT_MSG_MONSTER_SAY, 300.0f, caster);
                me->PlayDirectSound(27825, caster->ToPlayer());
                _hitCount++;
            }
            else if (_hitCount == 2)
            {
                me->Talk(TEXT_6, CHAT_MSG_MONSTER_SAY, 350.0f, caster);
                me->PlayDirectSound(27827, caster->ToPlayer());
                _hitCount = 0;
            }
            break;
        case SPELL_TRIGGER_WITH_TURN:
            me->Talk(TEXT_5, CHAT_MSG_MONSTER_SAY, 300.0f, caster);
            me->PlayDirectSound(27826, caster->ToPlayer());
            break;
        }
    }

private:
    uint8 _hitCount;
};

void AddSC_zone_the_wandering_isle()
{
	using namespace Scripts::Pandaria::TheWanderingIsle;
	
    RegisterCreatureAI(npc_tushui_huojin_trainee);
    RegisterCreatureAI(npc_huojin_trainee);
    RegisterCreatureAI(npc_tushui_leading_trainee);
    RegisterCreatureAI(npc_instructor_zhi);
    RegisterGameObjectAI(go_edict_of_temperance);
    RegisterCreatureAI(npc_jaomin_ro);
    RegisterCreatureAI(npc_jaomin_ro_hawk);
    RegisterCreatureAI(npc_min_dimwind_summon);
    RegisterCreatureAI(npc_amberleaf_scamp);
    RegisterCreatureAI(npc_aysa_cloudsinger_summon);
    RegisterCreatureAI(npc_aysa_cloudsinger_cave_of_meditation);
    RegisterCreatureAI(npc_master_li_fei_summon);
    RegisterCreatureAI(npc_master_shang);
    RegisterCreatureAI(npc_ruk_ruk);
    RegisterCreatureAI(npc_ruk_ruk_rocket);
    RegisterCreatureAI(npc_zhaoren);
    RegisterCreatureAI(npc_hot_air_balloon_from_spell);
    RegisterCreatureAI(npc_shenzinsu);
    
    RegisterSpellScript(spell_force_summoner_to_ride_vehicle);
    RegisterSpellScript(spell_ride_drake);
    RegisterSpellScript(spell_meditation_timer_bar);
    RegisterSpellScript(spell_flame_spout);
    
    RegisterSpellScript(spell_monkey_wisdom_text);
    RegisterSpellScript(spell_ruk_ruk_ooksplosions);
    RegisterSpellScript(spell_summon_worthy_of_passing);
	
	new at_singing_pools_transform_frog();
    new at_singing_pools_transform_base<Spells::CurseOfTheSkunk>("at_singing_pools_transform_skunk");
    new at_singing_pools_transform_base<Spells::CurseOfTheCrocodile>("at_singing_pools_transform_crocodile");
    new at_singing_pools_transform_base<Spells::CurseOfTheCrane>("at_singing_pools_transform_crane");
    new at_singing_pools_transform_base<Spells::CurseOfTheTurtle>("at_singing_pools_transform_turtle");

    new at_min_dimwind_captured();
    new at_cave_of_meditation();
    new at_inside_of_cave_of_meditation();
    
    
    new at_chamber_of_whispers();

    RegisterCreatureAI(npc_li_fei);
    RegisterSpellScript(spell_fire_crash);
    RegisterCreatureAI(npc_li_fei_combat);
    RegisterSpellScript(spell_feet_of_fury);
    RegisterSpellScript(spell_flying_shadow_kick);

}
