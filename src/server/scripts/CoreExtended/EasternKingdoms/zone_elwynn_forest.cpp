#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "Creature.h"
#include "CreatureAIImpl.h"
#include "EventMap.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Position.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "ObjectMgr.h"
#include "World.h"
#include "PetAI.h"
#include "PassiveAI.h"
#include "CombatAI.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "Vehicle.h"
#include "Player.h"
#include "SpellScript.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"
#include "Unit.h"
#include "zone_elwynn_forest.h"

enum {
    NPC_BROTHER_PAXTON = 951,

    NPC_HEAL_TARGET = 49869,
    NPC_WOLF = 49871,

    SPELL_HEAL = 93094,

    ACTION_TALK_EVENT = 1
};

// 49869
struct npc_stormwind_infantry : public ScriptedAI
{
    explicit npc_stormwind_infantry(Creature* creature) : ScriptedAI(creature) {}

    TaskScheduler _scheduler;
    uint32 talkCooldown = 0;

    void JustAppeared() override
    {
        ScriptedAI::JustAppeared();
        talkCooldown = urand(10000, 30000);
        ScheduleCombatCheck();
    }

    void Reset() override
    {
        me->SetSheath(SHEATH_STATE_MELEE);
    }

    void ScheduleCombatCheck()
    {
        _scheduler.Schedule(2s, [this](TaskContext context)
        {
            if (!me->IsInCombat())
            {
                if (me->GetDistance(me->GetHomePosition()) < 2.0f)
                {
                    if (Creature* wolf = me->FindNearestCreature(NPC_WOLF, 10.0f))
                    {
                        if (wolf->IsAlive() && !wolf->IsInCombat())
                        {
                            AttackStart(wolf);

                            if (wolf->IsAIEnabled())
                                wolf->AI()->AttackStart(me);
                        }
                    }
                }
            }

            context.Repeat(2s);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);

        if (talkCooldown <= diff)
        {
            if (Creature* paxton = me->FindNearestCreature(NPC_BROTHER_PAXTON, 30.0f))
            {
                Talk(0);

                if (paxton->IsAIEnabled())
                    paxton->AI()->DoAction(ACTION_TALK_EVENT);
            }

            talkCooldown = urand(30000, 120000);
        }
        else
        {
            talkCooldown -= diff;
        }

        if (!UpdateVictim())
            return;

        if (me->isAttackReady() && me->IsWithinMeleeRange(me->GetVictim()))
        {
            me->AttackerStateUpdate(me->GetVictim());
            me->resetAttackTimer();
        }
    }
};

// 951 - Brother Paxton
struct npc_brother_paxton : public ScriptedAI
{
    explicit npc_brother_paxton(Creature* creature) : ScriptedAI(creature) {}

    void JustAppeared() override
    {
        ScriptedAI::JustAppeared();

        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);

        me->SetNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);

        me->GetMotionMaster()->MovePath(720313121, true);
    }

    void DoAction(int32 actionId) override
    {
        if (actionId == ACTION_TALK_EVENT)
        {
            if (Creature* soldier = me->FindNearestCreature(NPC_HEAL_TARGET, 30.0f))
            {
                Talk(0);
                DoCast(soldier, SPELL_HEAL, true);
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->IsInCombat())
        {
            me->ClearInCombat();
        }

        if (!me->HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER))
        {
            me->SetNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
        }

        ScriptedAI::UpdateAI(diff);
    }
};

// 448 - Hogger
struct npc_custom_hogger : public ScriptedAI
{
    npc_custom_hogger(Creature* creature) : ScriptedAI(creature) { }

    bool _minionsSummoned = false;
    bool _endingSceneActive = false;
    bool _alreadyEaten = false;
    bool _isEating = false;

    void Reset() override
    {
        _events.Reset();

        _minionsSummoned = false;
        _endingSceneActive = false;
        _alreadyEaten = false;
        _isEating = false;

        me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetReactState(REACT_AGGRESSIVE);
        me->SetWalk(false);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_endingSceneActive)
        {
            damage = 0;
            return;
        }

        if (!_minionsSummoned && me->HealthBelowPctDamaged(50, damage))
            SummonMinions();

        if (!_alreadyEaten && me->HealthBelowPctDamaged(30, damage))
            MoveToEatingPosition();


        if (me->GetHealth() <= damage)
        {
            damage = 0;
            me->SetHealth(1);
            StartEndingScene();
            RewardPlayers();
        }

        if (_isEating)
        {
            _isEating = false;
            me->SetReactState(REACT_AGGRESSIVE);
            Talk(SAY_STUNNED);
        }
    }

    void JustEnteredCombat(Unit* /*who*/) override
    {
        Talk(SAY_HOGGER_AGGRO);
        _events.ScheduleEvent(EVENT_VICIOUS_SLICE, 3s);
    }

    void MoveToEatingPosition()
    {
        _alreadyEaten = true;

        if (Creature* target = me->FindNearestCreature(NPC_EATING_TARGET, 100.0f))
        {
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MovePoint(0, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), true);
            _events.ScheduleEvent(EVENT_CHECK_EAT_RANGE, 200ms);
        }
    }

    void StartEndingScene()
    {
        _endingSceneActive = true;
        _events.Reset();

        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        me->StopMoving();
        me->AttackStop();

        Talk(SAY_BEG);

        SummonGeneralHammondClay();
        SummonAndromath();
        SummonDumas();

        _events.ScheduleEvent(EVENT_HAMMOND_GROUP_START_WALKING, 1s);
    }

    void RewardPlayers()
    {
        for (auto const& ref : me->GetThreatManager().GetUnsortedThreatList())
        {
            if (!ref->IsOnline())
                continue;

            if (Unit* unit = ref->GetVictim())
            {
                if (Player* player = unit->ToPlayer())
                    player->RewardPlayerAndGroupAtEvent(NPC_HOGGER, me);
            }
        }
    }

    void SummonGeneralHammondClay()
    {
        if (TempSummon* hammond = me->SummonCreature(NPC_GENERAL_HAMMOND_CLAY, generalHammondClayPositions[0]))
        {
            _generalHammondGUID = hammond->GetGUID();
            hammond->CastSpell(hammond, SPELL_TELEPORT_VISUAL_ONLY_1, true);
            hammond->Mount(DISPLAYID_GENERAL_HAMMOND_CLAYS_MOUNT);
            hammond->AI()->Talk(YELL_OPENING);
        }
        // summon ragamuffins and do text
        if (TempSummon* ragamuffin1 = me->SummonCreature(NPC_RAGAMUFFIN, ragamuffinPositions[0], TEMPSUMMON_TIMED_DESPAWN, 10s))
        {
            _ragamuffin1GUID = ragamuffin1->GetGUID();
            ragamuffin1->SetFacingToObject(GetHammond());
            ragamuffin1->AI()->Talk(SAY_CLAY);
            ragamuffin1->SetWalk(false);
            _events.ScheduleEvent(EVENT_RAGAMUFFIN_LEAVE1, 3s, 8s);            
        }
        if (TempSummon* ragamuffin2 = me->SummonCreature(NPC_RAGAMUFFIN, ragamuffinPositions[1], TEMPSUMMON_TIMED_DESPAWN, 10s))
        {
            _ragamuffin2GUID = ragamuffin2->GetGUID();
            ragamuffin2->SetFacingToObject(GetHammond());
            ragamuffin2->AI()->Talk(SAY_WOW);
            ragamuffin2->SetWalk(false);
            _events.ScheduleEvent(EVENT_RAGAMUFFIN_LEAVE2, 3s, 8s);
        }
    }

    void MoveGeneralHammondClay()
    {
        if (GetHammond())
        {
            GetHammond()->SetWalk(true);
            GetHammond()->GetMotionMaster()->MovePoint(0, generalHammondClayPositions[2], true);
            _events.ScheduleEvent(EVENT_DISMOUNT_HAMMOND_CLAY, 8500ms);
        }
    }

    void SummonAndromath()
    {
        TempSummon* andromath = me->SummonCreature(NPC_ANDROMATH, andromathPositions[0]);
        if (andromath)
        {
            _andromathGUID = andromath->GetGUID();
            andromath->CastSpell(andromath, SPELL_TELEPORT_VISUAL_ONLY_1, true);
        }
    }

    void MoveAndromath()
    {
        if (GetAndromath())
        {
            GetAndromath()->SetWalk(true);
            GetAndromath()->GetMotionMaster()->MovePoint(0, andromathPositions[2], true);
        }
    }

    void SummonDumas()
    {
        TempSummon* dumas = me->SummonCreature(NPC_DUMAS, dumasPositions[0]);
        if (dumas)
        {
            _dumasGUID = dumas->GetGUID();
            dumas->CastSpell(dumas, SPELL_TELEPORT_VISUAL_ONLY_1, true);
        }
    }

    void MoveDumas()
    {
        if (GetDumas())
        {
            GetDumas()->SetWalk(true);
            GetDumas()->GetMotionMaster()->MovePoint(0, dumasPositions[2], true);
        }
    }

    void SummonMinions()
    {
        me->CastStop();
        Talk(SAY_HOGGER_SUMMON_MINIONS);

        for (float distance : { 0.5f, 1.5f, 2.5f })
        {
            // Build offset
            Position offset;
            offset.m_positionX = distance;  // forward
            offset.m_positionY = 0.0f;      // no sideways offset
            offset.m_positionZ = 0.0f;

            // Apply offset to Hogger's current position
            Position pos = me->GetPositionWithOffset(offset);

            // Fix Z height
            pos.m_positionZ = me->GetMap()->GetHeight(
                me->GetPhaseShift(),
                pos.GetPositionX(),
                pos.GetPositionY(),
                pos.GetPositionZ()
            );

            // Summon minion
            me->SummonCreature(NPC_HOGGER_MINION, pos);
        }

        _minionsSummoned = true;
    }

    void MoveHoggerToFinalPosition()
    {
        me->SetWalk(true);
        me->GetMotionMaster()->MovePoint(0, hoggerPositions[0]);
    }

    void TeleportBack()
    {
        if (GetHammond() && GetAndromath() && GetDumas())
        {
            GetHammond()->CastSpell(GetHammond(), SPELL_TELEPORT_VISUAL_ONLY_2, true);
            GetAndromath()->CastSpell(GetAndromath(), SPELL_TELEPORT_VISUAL_ONLY_2, true);
            GetDumas()->CastSpell(GetDumas(), SPELL_TELEPORT_VISUAL_ONLY_2, true);
            DoCastSelf(SPELL_TELEPORT_VISUAL_ONLY_2, true);

            me->DisappearAndDie();
            GetHammond()->DisappearAndDie();
            GetAndromath()->DisappearAndDie();
            GetDumas()->DisappearAndDie();
        }
    }

    Creature* GetHammond()
    {
        return me->GetMap()->GetCreature(_generalHammondGUID);
    }

    Creature* GetAndromath()
    {
        return me->GetMap()->GetCreature(_andromathGUID);
    }

    Creature* GetDumas()
    {
        return me->GetMap()->GetCreature(_dumasGUID);
    }

    Creature* GetRagamuffin1()
    {
        return me->GetMap()->GetCreature(_ragamuffin1GUID);
    }

    Creature* GetRagamuffin2()
    {
        return me->GetMap()->GetCreature(_ragamuffin2GUID);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim() && !_endingSceneActive)
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_VICIOUS_SLICE:
                DoCastVictim(SPELL_VICIOUS_SLICE);
                _events.Repeat(3s);
                break;

            case EVENT_HAMMOND_GROUP_START_WALKING:
                MoveGeneralHammondClay();
                MoveAndromath();
                MoveDumas();
                MoveHoggerToFinalPosition();
                break;

            case EVENT_DISMOUNT_HAMMOND_CLAY:
                if (GetHammond())
                {
                    me->SetFacingToObject(GetHammond());

                    GetHammond()->Dismount();
                    GetHammond()->GetMotionMaster()->MovePoint(0, generalHammondClayPositions[3], true);

                    _events.ScheduleEvent(EVENT_HOGGER_SAY_GRR, 3s);
                }
                break;

            case EVENT_HOGGER_SAY_GRR:
                Talk(SAY_RAND);
                _events.ScheduleEvent(EVENT_CLAYS_EXPLAINATION, 3s);
                break;

            case EVENT_CLAYS_EXPLAINATION:
                if (GetHammond())
                    GetHammond()->AI()->Talk(SAY_EXPLAINATION);
                _events.ScheduleEvent(EVENT_CLAY_SAYS_TAKE_HIM, 4500ms);
                break;

            case EVENT_CLAY_SAYS_TAKE_HIM:
                if (GetHammond())
                    GetHammond()->AI()->Talk(SAY_TAKE_HIM);
                _events.ScheduleEvent(EVENT_HOGGER_SAYS_NOO, 2s);
                break;

            case EVENT_HOGGER_SAYS_NOO:
                Talk(SAY_FINAL);
                _events.ScheduleEvent(EVENT_CLAY_SPEAKS_TO_ANDROMATH, 3s);
                break;

            case EVENT_CLAY_SPEAKS_TO_ANDROMATH:
                if (GetHammond() && GetAndromath())
                {
                    GetHammond()->SetFacingToObject(GetAndromath());
                    GetAndromath()->SetFacingToObject(GetHammond());
                    GetHammond()->AI()->Talk(SAY_TO_ANDROMATH);
                }
                _events.ScheduleEvent(EVENT_TELEPORT_BACK, 4s);
                break;

            case EVENT_TELEPORT_BACK:
                TeleportBack();
                break;

            case EVENT_CHECK_EAT_RANGE:
                if (!me->FindNearestCreature(NPC_EATING_TARGET, 3.0f))
                {
                    _events.ScheduleEvent(EVENT_CHECK_EAT_RANGE, 200ms);
                    break;
                }
                else
                {
                    DoCast(SPELL_EATING);
                    Talk(SAY_EATING);
                    _events.ScheduleEvent(EVENT_BLOODY_STRIKE, 100ms);
                    _isEating = true;
                }
                break;

            case EVENT_BLOODY_STRIKE:
                if (_isEating)
                {
                    if (Creature* dummy = me->FindNearestCreature(NPC_EATING_TARGET, 10.0f))
                        DoCast(dummy, SPELL_BLOODY_STRIKE, true);
                    _events.ScheduleEvent(EVENT_BLOODY_STRIKE, 1s);
                }
                break;

            case EVENT_RAGAMUFFIN_LEAVE1:
            {
                if(GetRagamuffin1())
                    GetRagamuffin1()->GetMotionMaster()->MovePoint(0, ragamuffinPositionsLeave[0], true);

                break;
            }

            case EVENT_RAGAMUFFIN_LEAVE2:
            {
                if (GetRagamuffin2())
                    GetRagamuffin2()->GetMotionMaster()->MovePoint(0, ragamuffinPositionsLeave[0], true);

                break;
            }

            default:
                break;
            }
        }

        if (!_endingSceneActive)
            me->DoMeleeAttackIfReady();
    }

private:
    EventMap _events;

    ObjectGuid _generalHammondGUID;
    ObjectGuid _andromathGUID;
    ObjectGuid _dumasGUID;
    ObjectGuid _ragamuffin1GUID;
    ObjectGuid _ragamuffin2GUID;
};

// 46932 - Hogger Minion
struct npc_custom_hogger_minion : public ScriptedAI
{
    npc_custom_hogger_minion(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        me->SetReactState(REACT_AGGRESSIVE);

        if (Creature* hogger = me->FindNearestCreature(NPC_HOGGER, 35.0f, true))
            me->CastSpell(hogger, SPELL_ADVENTURERS_RUSH, true);
    }
};

void AddSC_zone_elwyn_forest()
{
    // Northshire zone
    RegisterCreatureAI(npc_stormwind_infantry);
    RegisterCreatureAI(npc_brother_paxton);
	
	// Hogger
	RegisterCreatureAI(npc_custom_hogger);
    RegisterCreatureAI(npc_custom_hogger_minion);
}
