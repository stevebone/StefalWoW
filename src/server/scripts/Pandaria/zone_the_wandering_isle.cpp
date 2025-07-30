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
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GridNotifiersImpl.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedFollowerAI.h"
#include "SpellScript.h"
#include "TaskScheduler.h"
#include "TemporarySummon.h"
#include "ThreatManager.h"

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
            me->GetMotionMaster()->MoveJumpWithGravity(aysaJumpPos[0], 12.0f, 17.4735f);

            task.Schedule(1700ms, [this](TaskContext task)
            {
                me->GetMotionMaster()->MoveJumpWithGravity(aysaJumpPos[1], 12.0f, 10.7163f);

                task.Schedule(2s, [this](TaskContext /*task*/)
                {
                    me->GetMotionMaster()->MoveJumpWithGravity(aysaJumpPos[2], 12.0f, 14.6923f, POINT_JUMP);
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

/*
Stefal TODO:
- Kickback spell or jumping to center is missing
- No phase change - not sure if this is needed
- The fire walls are removed on quest completion - not sure if that is expected behavior
- Fei clone is killed and despawns - not sure if that is expected behavior
*/

enum OnlyTheWorthyShallPass
{
    NPC_MASTER_LI_FEI = 54135,
    NPC_MASTER_LI_FEI_CHALLENGE = 54734,

    OBJECT_GUID_WALL_FIRE = 209367,
    OBJECT_GUID_WALL_FIRE2 = 209375,

    QUEST_ONLY_THE_WORTHY_SHALL_PASS = 29421,

    SPELL_FEET_OF_FURY = 108958,
    SPELL_FLYING_SHADOW_KICK = 108936,

    EVENT_FEI_RANDOM_SPELL = 1,

    SAY_FEI_TEXT_0 = 0
};

struct npc_master_li_fei : public ScriptedAI
{
    npc_master_li_fei(Creature* creature) : ScriptedAI(creature) { }

    void OnQuestAccept(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_ONLY_THE_WORTHY_SHALL_PASS)
        {
            if (Creature* summon = me->SummonCreature(NPC_MASTER_LI_FEI_CHALLENGE,
                1351.666f, 3940.699f, 109.318f, 2.781f,
                TEMPSUMMON_MANUAL_DESPAWN))
            {
                summon->AI()->AttackStart(player);
            }

            me->DespawnOrUnsummon();
        }
    }
};

struct npc_master_li_fei_challenge : public ScriptedAI
{
    npc_master_li_fei_challenge(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
    }

    void JustAppeared() override
    {
        me->SetReactState(REACT_AGGRESSIVE); 
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE_2);
        me->SetImmuneToPC(false);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_FEI_RANDOM_SPELL, 4s, 6s);
    }

    void OnHealthDepleted(Unit* /*attacker*/, bool /*isKill*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
        me->RemoveAllAuras();
        me->InterruptNonMeleeSpells(true);
        _events.Reset();
        me->SetUninteractible(true);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE_2);

        Creature* liFei = me->SummonCreature(NPC_MASTER_LI_FEI, 1331.78f, 3942.8f, 110.606f, 6.00393f, TEMPSUMMON_TIMED_DESPAWN, 120s);
        if (liFei && liFei->IsAlive())
                liFei->AI()->Talk(SAY_FEI_TEXT_0);

        std::list<GameObject*> goList;
        GetGameObjectListWithEntryInGrid(goList, me, OBJECT_GUID_WALL_FIRE, 50.0f);

        for (GameObject* go : goList)
        {
            if (go)  // optional, depending on your use-case
                go->DespawnOrUnsummon(1s, 720s);
        }

        std::list<GameObject*> goList2;
        GetGameObjectListWithEntryInGrid(goList2, me, OBJECT_GUID_WALL_FIRE2, 50.0f);

        for (GameObject* go2 : goList2)
        {
            if (go2)  // optional, depending on your use-case
                go2->DespawnOrUnsummon(1s, 720s);
        }

        me->DespawnOrUnsummon(2s);
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
                case EVENT_RANDOM_SPELL:
                {
                    uint32 spellId = RAND(SPELL_FEET_OF_FURY, SPELL_FLYING_SHADOW_KICK);
                    DoCastVictim(spellId);
                    _events.ScheduleEvent(EVENT_FEI_RANDOM_SPELL, 4s, 6s);
                    break;
                }
                default:
                    break;
            }
        }
    }
private:
    EventMap _events;
};

enum FanTheFlames
{
    spell_fan_the_flames = 102522,
    spell_ftf_throw_wood = 109090,
    spell_ftf_throw_air = 109095,
    spell_ftf_throw_bigair = 109105,
    spell_ftf_throw_bigger = 109109,
    spell_ftf_credit = 109107,

    npc_huo_despawn = 57779,
    npc_huo_spawn = 54787
};

class spell_fan_the_flames_script : public SpellScript
{
    //PrepareSpellScript(spell_fan_the_flames_script);

    void HandleAfterCast()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;

        Creature* target = GetClosestCreatureWithEntry(player, npc_huo_despawn, 5.0f, true);
            if (!target)
                return;

        // Step 1: Cast throw wood
        player->m_Events.AddEvent(new PlayerDelayedCast(player, spell_ftf_throw_wood, target),
            player->m_Events.CalculateTime(1s));

        // Step 2: Throw air
        player->m_Events.AddEvent(new PlayerDelayedCast(player, spell_ftf_throw_air, target),
            player->m_Events.CalculateTime(2s));

        // Step 3: Throw big air
        player->m_Events.AddEvent(new PlayerDelayedCast(player, spell_ftf_throw_bigair, target),
            player->m_Events.CalculateTime(3ms));

        // Step 4: Throw bigger
        player->m_Events.AddEvent(new PlayerDelayedCast(player, spell_ftf_throw_bigger, target),
            player->m_Events.CalculateTime(4s));

        // Step 5: Credit spell + NPC changes
        player->m_Events.AddEvent(new PlayerFinalFTFStep(player),
            player->m_Events.CalculateTime(7s));
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_fan_the_flames_script::HandleAfterCast);
    }

private:
    // Helper class for delayed casts
    class PlayerDelayedCast : public BasicEvent
    {
    public:
        PlayerDelayedCast(Player* player, uint32 spellId, Creature* target)
            : _player(player), _spellId(spellId), _target(target) {}

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_player || !_player->IsInWorld() || !_target || !_target->IsInWorld())
                return true;

            _player->CastSpell(_target, _spellId, true); // cast on target
            return true;
        }

    private:
        Player* _player;
        uint32 _spellId;
        Creature* _target;
    };

    // Final step: credit + NPC swap
    class PlayerFinalFTFStep : public BasicEvent
    {
    public:
        PlayerFinalFTFStep(Player* player) : _player(player) {}

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_player || !_player->IsInWorld())
                return true;

            // Credit spell
            _player->CastSpell(_player, spell_ftf_credit, true);

            Creature* target = GetClosestCreatureWithEntry(_player, npc_huo_despawn, 5.0f, true);
            if (target)
                target->DespawnOrUnsummon(100ms); // 1s

            // Spawn new Huo
            _player->SummonCreature(npc_huo_spawn, 1257.89f, 3925.59f, 127.856f, 0.506145f, TEMPSUMMON_TIMED_DESPAWN, 120s);

            return true;
        }

    private:
        Player* _player;
    };
};

// Quest: 29423 The Passion of Shen-zin Su
// OnQuestAccept the Huo entry 54787 spawns 54958

enum ThePassionOfShenZinSu
{
    QUEST_THE_PASSION_OF_SHENZIN_SU = 29423,

    NPC_HUO_FOLLOWER = 54958,
    NPC_MASTER_SHANG_XI = 54786,
    NPC_FIRE_SPIRIT_CREDIT = 61128,
    NPC_JI_FIREPAW = 57720,
    NPC_AYSA_CLOUDSINGER = 57721,
    NPC_CHIA_HUI = 60248,
    NPC_BREWER_LIN = 60253,

    SPELL_BLESSING_OF_HUO = 102630,

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

#define FOLLOW_DIST 1.5f
#define FOLLOW_ANGLE 0.0f

struct npc_huo_spawn_follower : public ScriptedAI
{
    npc_huo_spawn_follower(Creature* creature) : ScriptedAI(creature) { }

    void OnQuestAccept(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_THE_PASSION_OF_SHENZIN_SU)
        {
            // Cast the Blessing of Huo spell on the player
            player->CastSpell(player, SPELL_BLESSING_OF_HUO, true);

            if (Creature* summon = me->SummonCreature(NPC_HUO_FOLLOWER, 1257.89f, 3925.59f, 127.856f, 0.506145f, TEMPSUMMON_MANUAL_DESPAWN, 0s, player->GetGUID()))
            {
                // Make follower non - attackable and passive
                summon->SetReactState(REACT_PASSIVE);
                //summon->SetUninteractible(true);
                summon->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE_2);

                if (summon->AI())
                {
                    // Set the summoned creature to follow the player via FollowerAI
                    if (FollowerAI* followerAI = dynamic_cast<FollowerAI*>(summon->AI()))
                    {
                        followerAI->StartFollow(player, QUEST_THE_PASSION_OF_SHENZIN_SU);
                    }
                }
            }
        }
    }
};

struct npc_huo_follower : public FollowerAI
{
    npc_huo_follower(Creature* creature) : FollowerAI(creature) { }

    EventMap events;
    std::set<uint32> interactedEntries;

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (Player* player = summoner->ToPlayer())
            StartFollow(player, FOLLOW_DIST, FOLLOW_ANGLE);
    }

    // Add quest completion logic or movement change later
    void UpdateAI(uint32 diff) override
    {
        // Optional: call base class first
        FollowerAI::UpdateAI(diff);

        if (Player* player = GetLeaderForFollower())
        {
            // Define range check
            float checkRange = 20.0f; // Adjust as needed

            // Attempt to find the quest completion NPC by entry ID
            Creature* completionNPC = me->FindNearestCreature(NPC_MASTER_SHANG_XI, checkRange, true);
            if (completionNPC)
            {
                if (player->GetQuestStatus(QUEST_THE_PASSION_OF_SHENZIN_SU) == QUEST_STATUS_INCOMPLETE)
                {
                    player->KilledMonsterCredit(NPC_FIRE_SPIRIT_CREDIT);
                    if (player->HasAura(SPELL_BLESSING_OF_HUO))
                        player->RemoveAurasDueToSpell(SPELL_BLESSING_OF_HUO);

                    // Follower moves
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MovePoint(0, 957.5355f, 3603.9404f, 197.1533f);
                }

            }
        }

        CheckForNearbyCreature(NPC_CHIA_HUI, 30.0f);
        CheckForNearbyCreature(NPC_BREWER_LIN, 10.0f);

    }


    void CheckForNearbyCreature(uint32 entry, float range)
    {
        // Only do it once per entry
        if (interactedEntries.contains(entry))
            return;

        std::list<Creature*> nearbyCreatures;
        GetCreatureListWithEntryInGrid(nearbyCreatures, me, entry, range);

        if (!nearbyCreatures.empty())
        {
            if (Player* player = GetLeaderForFollower())
            {
                Creature* talker = GetClosestCreatureWithEntry(me, entry, range);
                if(talker)
                {
                    talker->AI()->Talk(SAY_NPC_HUO_NEARBY_00, player);
                    interactedEntries.insert(entry); // Mark as done
                }
            }
        }
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        FollowerAI::MovementInform(type, id); // Base call

        if (type == POINT_MOTION_TYPE && id == 0)
        {
            me->StopMoving(); // Kill leftover motion flags
            me->GetMotionMaster()->Clear();

            float x = 957.5355f;
            float y = 3603.9404f;
            float z = 197.1533f;
            float o = 6.2444f;

            me->NearTeleportTo(x, y, z, o);

            me->SetEmoteState(EMOTE_STATE_STAND); // Idle stance
            //me->SetOrientation(6.2444f);
            me->DespawnOrUnsummon(120s); // Despawn after 120 sec
        }
    }

};

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

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);

            std::list<Player*> players;
            Trinity::PlayerAtMinimumRangeAway predicate(me, 5.0f);
            Trinity::PlayerListSearcher<Trinity::PlayerAtMinimumRangeAway> searcher(me, players, predicate);
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

class OnLoginSpawnHuo : public PlayerScript
{
public:
    OnLoginSpawnHuo() : PlayerScript("OnLoginSpawnHuo") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        const uint32 questId = QUEST_THE_PASSION_OF_SHENZIN_SU;
        const uint32 npcEntry = NPC_HUO_FOLLOWER;

        // Check if player has the quest active
        if (player->IsInWorld() && player->IsActiveQuest(questId))
        {
            // Spawn the NPC near the player
            float x = player->GetPositionX() + 2.0f;
            float y = player->GetPositionY() + 2.0f;
            float z = player->GetPositionZ();
            float o = player->GetOrientation();

            if (Creature* summon = player->SummonCreature(npcEntry, x, y, z, o, TEMPSUMMON_MANUAL_DESPAWN, 0s, player->GetGUID()))
            {
                // Make follower non - attackable and passive
                summon->SetReactState(REACT_PASSIVE);
                summon->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE_2);
            }
        }
    }
};

//The Singing Pools
enum SingingPoolsATSpells
{
    SPELL_CURSE_OF_THE_FROG = 102938,
    SPELL_CURSE_OF_THE_SKUNK = 102939,
    SPELL_CURSE_OF_THE_TURTLE = 102940,
    SPELL_CURSE_OF_THE_CRANE = 102941,
    SPELL_CURSE_OF_THE_CROCODILE = 102942,
    SPELL_RIDE_VEHICLE_POLE = 102717,
    SPELL_RIDE_VEHICLE_BELL_POLE = 107049,
    SPELL_TRAINING_BELL_EXCLUSION_AURA = 133381
};

class at_singing_pools_transform : public AreaTriggerScript
{
public:
    at_singing_pools_transform() : AreaTriggerScript("at_singing_pools_transform") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger) override
    {
        if (player->IsAlive() && !player->HasAura(SPELL_RIDE_VEHICLE_POLE))
        {
            switch (areaTrigger->ID)
            {
            case 6986:

            case 6987:
                if (!player->HasAura(SPELL_CURSE_OF_THE_FROG))
                    player->CastSpell(player, SPELL_CURSE_OF_THE_FROG, true);
                if (player->HasAura(SPELL_TRAINING_BELL_EXCLUSION_AURA))
                    player->RemoveAura(SPELL_TRAINING_BELL_EXCLUSION_AURA);
                break;
            case 6988:
            case 6989:
                if (!player->HasAura(SPELL_CURSE_OF_THE_SKUNK))
                    player->CastSpell(player, SPELL_CURSE_OF_THE_SKUNK, true);
                break;
            case 6990:
                if (!player->HasAura(SPELL_CURSE_OF_THE_CROCODILE))
                    player->CastSpell(player, SPELL_CURSE_OF_THE_CROCODILE, true);
                break;
            case 6991:
            case 6992:
                if (!player->HasAura(SPELL_CURSE_OF_THE_CRANE))
                    player->CastSpell(player, SPELL_CURSE_OF_THE_CRANE, true);
                break;
            case 7011:
            case 7012:
                if (!player->HasAura(SPELL_CURSE_OF_THE_TURTLE))
                    player->CastSpell(player, SPELL_CURSE_OF_THE_TURTLE, true);
                break;
            }
            return true;
        }
        return false;
    }

    bool OnExit(Player* player, AreaTriggerEntry const* areaTrigger) override
    {
        if (player->IsAlive() && !player->HasAura(SPELL_RIDE_VEHICLE_POLE))
        {
            switch (areaTrigger->ID)
            {
            case 6986:
            case 6987:
                if (!player->IsInAreaTrigger(sAreaTriggerStore.LookupEntry(6986)) && !player->IsInAreaTrigger(sAreaTriggerStore.LookupEntry(6987)))
                    if (player->HasAura(SPELL_CURSE_OF_THE_FROG))
                        player->RemoveAura(SPELL_CURSE_OF_THE_FROG);
                break;
            case 6988:
            case 6989:
                if (!player->IsInAreaTrigger(sAreaTriggerStore.LookupEntry(6988)) && !player->IsInAreaTrigger(sAreaTriggerStore.LookupEntry(6989)))
                    if (player->HasAura(SPELL_CURSE_OF_THE_SKUNK))
                        player->RemoveAura(SPELL_CURSE_OF_THE_SKUNK);
                break;
            case 6990:
                if (player->HasAura(SPELL_CURSE_OF_THE_CROCODILE))
                    player->RemoveAura(SPELL_CURSE_OF_THE_CROCODILE);
                break;
            case 6991:
            case 6992:
                if (!player->IsInAreaTrigger(sAreaTriggerStore.LookupEntry(6991)) && !player->IsInAreaTrigger(sAreaTriggerStore.LookupEntry(6992)))
                    if (player->HasAura(SPELL_CURSE_OF_THE_CRANE))
                        player->RemoveAura(SPELL_CURSE_OF_THE_CRANE);
                break;
            case 7011:
            case 7012:
                if (!player->IsInAreaTrigger(sAreaTriggerStore.LookupEntry(7011)) && !player->IsInAreaTrigger(sAreaTriggerStore.LookupEntry(7012)))
                    if (player->HasAura(SPELL_CURSE_OF_THE_TURTLE))
                        player->RemoveAura(SPELL_CURSE_OF_THE_TURTLE);
                break;
            }
            return true;
        }
        return false;
    }
};

class at_singing_pools_training_bell : public AreaTriggerScript
{
public:
    at_singing_pools_training_bell() : AreaTriggerScript("at_singing_pools_training_bell") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        if (player->IsAlive())
        {
            player->AddAura(SPELL_TRAINING_BELL_EXCLUSION_AURA, player);
            return true;
        }
        return false;
    }

    bool OnExit(Player* player, AreaTriggerEntry const* /*areaTrigger*/) override
    {
        if (player->IsAlive())
        {
            player->RemoveAura(SPELL_TRAINING_BELL_EXCLUSION_AURA);
            return true;
        }
        return false;
    }
};

enum BalancePoleEvents
{
    EVENT_CAST_TRANSFORM = 1
};

enum BalancePoleNPCs
{
    NPC_BALANCE_POLE_1 = 54993,
    NPC_BALANCE_POLE_2 = 57431,
    NPC_TRAINING_BELL_BALANCE_POLE = 55083,
    NPC_CURSED_POOL_CONTROLLER = 55123
};

enum BalancePoleSpells
{
    SPELL_MONK_RIDE_POLE = 103030,
    SPELL_TRAINING_BELL_FORCECAST_RIDE_VEHICLE = 107050,
    SPELL_TRAINING_BELL_RIDE_VEHICLE = 107049
};

struct npc_balance_pole : ScriptedAI
{
    npc_balance_pole(Creature* creature) : ScriptedAI(creature) { }

    void PassengerBoarded(Unit* passenger, int8 /*seat*/, bool apply) override
    {
        if (passenger->GetTypeId() == TYPEID_PLAYER)
        {
            _passengerGuid = passenger->GetGUID();
            if (!apply)
                _events.ScheduleEvent(EVENT_CAST_TRANSFORM, 1s);
            else
            {
                if (me->GetEntry() == NPC_TRAINING_BELL_BALANCE_POLE)
                {
                    DoCast(passenger, SPELL_TRAINING_BELL_FORCECAST_RIDE_VEHICLE, true);
                }
                else
                {
                    passenger->RemoveAura(SPELL_CURSE_OF_THE_FROG);
                }
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_CAST_TRANSFORM:
                // Transform is casted only when in frog pool
                Unit* passenger = ObjectAccessor::GetUnit(*me, _passengerGuid);
                if (passenger->GetPositionZ() < 118.0f && !passenger->HasAura(SPELL_TRAINING_BELL_RIDE_VEHICLE) && !passenger->HasAura(SPELL_RIDE_VEHICLE_POLE))
                {
                    passenger->CastSpell(passenger, SPELL_CURSE_OF_THE_FROG, true);

                    if (passenger->HasAura(SPELL_TRAINING_BELL_EXCLUSION_AURA))
                        passenger->RemoveAura(SPELL_TRAINING_BELL_EXCLUSION_AURA);
                }
                _passengerGuid.Clear(); // ? Clear after you've finished handling the passenger
                break;
            }
        }
    }

private:
    EventMap _events;
    ObjectGuid _passengerGuid;
};

enum TushuiMonkOnPoleEvents
{
    EVENT_THROW_ROCK = 1,
    EVENT_SWITCH_POLE = 2,
    EVENT_DESPAWN = 3
};

enum TushuiMonkOnPoleNPCs
{
    NPC_MONK_ON_POLE_1 = 55019,
    NPC_MONK_ON_POLE_2 = 65468,
};

enum TushuiMonkOnPoleSpells
{
    SPELL_FORCECAST_RIDE_POLE = 103031,
    SPELL_THROW_ROCK = 109308
};

enum TushuiMonkOnPoleMisc
{
    QUEST_LESSON_OF_BALANCED_ROCK = 29663
};

struct npc_tushui_monk_on_pole : ScriptedAI
{
    npc_tushui_monk_on_pole(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_SWITCH_POLE, 0s);
        me->RestoreFaction();
        me->SetReactState(REACT_DEFENSIVE);
    }

    void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_FORCECAST_RIDE_POLE)
        {
            if (Unit* unitCaster = caster->ToUnit())
                DoCast(unitCaster, SPELL_MONK_RIDE_POLE, true);
        }
    }

    void JustEnteredCombat(Unit* who) override
    {
        me->SetReactState(REACT_AGGRESSIVE);
        //me->Attack(who, true);           // Actively set who to attack
        AttackStart(who);    

        _events.ScheduleEvent(EVENT_THROW_ROCK, 0s);
    }

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (damage >= me->GetHealth())
        {
            damage = 0; // Prevent lethal damage
            me->SetHealth(10);
            _events.Reset();
            me->RemoveAllAuras();
            me->SetFaction(35);
            me->SetUnitFlag(UnitFlags(UNIT_FLAG_CAN_SWIM | UNIT_FLAG_IMMUNE_TO_PC));
            me->AttackStop();
            attacker->AttackStop();
            me->_ExitVehicle();
            attacker->ToPlayer()->KilledMonsterCredit(NPC_MONK_ON_POLE_1);
            me->DespawnOrUnsummon(1s); // tempfix.
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        Unit* victim = nullptr;
        if (Unit* rawVictim = me->GetVictim())
        {
            if (rawVictim->IsVehicle() && rawVictim->GetVehicleKit()->GetPassenger(0))
                victim = rawVictim->GetVehicleKit()->GetPassenger(0);
            else
                victim = rawVictim;
        }

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_THROW_ROCK:
                {
                    if (victim && !me->IsWithinMeleeRange(victim))
                            DoCast(victim, SPELL_THROW_ROCK);
                }
                _events.ScheduleEvent(EVENT_THROW_ROCK, 2500ms);
                break;
            case EVENT_SWITCH_POLE:
                if (!me->IsInCombat())
                {
                    SwitchPole();
                    _events.ScheduleEvent(EVENT_SWITCH_POLE, 15s, 30s);
                }
                break;
            }
        }
        AttackStart(victim);
    }

    private:
        EventMap _events;

        void SwitchPole()
        {
            std::list<Creature*> polesList;
            std::list<Creature*> polesList2;
            // This stores objects that are too far away due to big combat reach
            me->GetCreatureListWithEntryInGrid(polesList, NPC_BALANCE_POLE_1, 1.0f);
            me->GetCreatureListWithEntryInGrid(polesList2, NPC_BALANCE_POLE_2, 1.0f);
            // Join both lists with possible different NPC entries
            polesList.splice(polesList.end(), polesList2);
            // Convert list to vector, so we can access iterator to be able to shuffle the list
            std::vector<Creature*> balancePolesList{ std::make_move_iterator(std::begin(polesList)), std::make_move_iterator(std::end(polesList)) };
            // Shuffle the list so NPCs won't jump always on the same poles
            Trinity::Containers::RandomShuffle(balancePolesList);

            for (std::vector<Creature*>::const_iterator itr = balancePolesList.begin(); itr != balancePolesList.end(); ++itr)
            {
                Position offset;
                offset.m_positionX = fabsf((*itr)->GetPositionX() - me->GetPositionX());
                offset.m_positionY = fabsf((*itr)->GetPositionY() - me->GetPositionY());

                // Object is too far
                if (offset.m_positionX > 5.0f || offset.m_positionY > 5.0f)
                    continue;

                if (!(*itr)->HasAura(SPELL_MONK_RIDE_POLE) && !(*itr)->HasAura(SPELL_RIDE_VEHICLE_POLE))
                {
                    (*itr)->CastSpell(me, SPELL_FORCECAST_RIDE_POLE, true);
                    break;
                }
            }
        }

        void DeleteAllFromThreatList(Unit* target, ObjectGuid except)
        {
            for (ThreatReference* ref : target->GetThreatManager().GetModifiableThreatList())
            {
                Unit* victim = ref->GetVictim();
                if (victim && victim->GetGUID() != except)
                {
                    ref->ClearThreat();
                    victim->ClearInCombat();
                }
            }
        }

        void MoveForward(float distance)
        {
            Position movePos;
            float ori = me->GetOrientation();
            float x = me->GetPositionX() + distance * cos(ori);
            float y = me->GetPositionY() + distance * sin(ori);
            float z = me->GetPositionZ();
            me->UpdateGroundPositionZ(x, y, z);
            movePos = { x, y, z };
            me->GetMotionMaster()->MovePoint(1, movePos);
        }

        void AttackStart(Unit* who) override
        {
            if (!who)
                return;

            // If the attacker is riding a pole, target the rider
            if (who->IsVehicle() && who->GetVehicleKit())
            {
                if (Unit* rider = who->GetVehicleKit()->GetPassenger(0))
                {
                    me->Attack(rider, true);
                    return;
                }
            }

            me->Attack(who, true);
        }
};

void AddSC_zone_the_wandering_isle()
{
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
    RegisterCreatureAI(npc_master_li_fei);
    RegisterCreatureAI(npc_master_li_fei_challenge);
    RegisterCreatureAI(npc_huo_spawn_follower);
    RegisterCreatureAI(npc_huo_follower);
    RegisterCreatureAI(npc_master_shang);
    RegisterCreatureAI(npc_balance_pole);
    RegisterCreatureAI(npc_tushui_monk_on_pole);
    

    RegisterSpellScript(spell_force_summoner_to_ride_vehicle);
    RegisterSpellScript(spell_ride_drake);
    RegisterSpellScript(spell_meditation_timer_bar);
    RegisterSpellScript(spell_flame_spout);
    RegisterSpellScript(spell_fan_the_flames_script);

    new at_min_dimwind_captured();
    new at_cave_of_meditation();
    new at_inside_of_cave_of_meditation();
    new at_singing_pools_transform();
    new at_singing_pools_training_bell();

    new OnLoginSpawnHuo();
}
