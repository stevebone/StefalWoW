/*
* Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Duskwood
SD%Complete: 100
SDComment: Quest Support:8735
SDCategory: Duskwood
EndScriptData */

#include "MotionMaster.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "TemporarySummon.h"

enum SpellSummonStalvanData
{
    NPC_STALVAN                                         = 315,
    NPC_WORGEN_TOBIAS                                   = 43797,

    DISPLAYID_WORGEN_TOBIAS                             = 33508,
    DISPLAYID_TOBIAS                                    = 31161,
    QUEST_MISTMANTLES_REVENGE                           = 26674,
};

const Position stalvanPosition      = { -10371.72f, -1251.92f, 35.99339f };
const Position stalvanDestination   = { -10369.932617f, -1253.7677f, 35.909294f };
const float stalvanOrientation      = 5.532694f;

const uint8 tobiasPositionsCount = 4;
const Position tobiasPositions[tobiasPositionsCount] =
{
    { -10351.5f, -1256.7f, 34.8566f },
    { -10357.5f, -1256.8f, 35.3863f },
    { -10363.5f, -1257.0f, 35.9107f },
    { -10365.8f, -1255.7f, 35.9098f }
};

const Position tobiasForestEscape = { -10362.98f, -1220.066f, 39.45f };

enum StalvanData
{
    SPELL_WORGEN_TRANSFORMATION                         = 81908,
    SPELL_CURSE_OF_STALVAN                              = 3105,
    SPELL_FETID_BREATH                                  = 85234,

    EVENT_STALVAN_STEP_1                                = 1,
    EVENT_STALVAN_STEP_2                                = 2,
    EVENT_STALVAN_STEP_3                                = 3,
    EVENT_STALVAN_STEP_4                                = 4,
    EVENT_STALVAN_STEP_5                                = 5,
    EVENT_STALVAN_STEP_6                                = 6,

    EVENT_TOBIAS_STEP_1                                 = 7,
    EVENT_TOBIAS_STEP_2                                 = 8,
    EVENT_TOBIAS_STEP_3                                 = 9,
    EVENT_TOBIAS_STEP_4                                 = 10,

    EVENT_CAST_SPELL_CURSE_OF_STALVAN                   = 11,
    EVENT_CAST_SPELL_FETID_BREATH                       = 12,

    SAY_00                                              = 0,
    SAY_01                                              = 1,
    SAY_02                                              = 2,
    SAY_03                                              = 3,
    SAY_04                                              = 4,
    SAY_05                                              = 5,
    SAY_06                                              = 6,
};

enum TobiasData
{
    EVENT_TOBIAS_SAY_DEATH                              = 1,
    EVENT_TOBIAS_ESCAPE                                 = 2,
};

// 315 - Stalvan Mistmantle
struct npc_stalvan : public ScriptedAI
{
    npc_stalvan(Creature* creature) : ScriptedAI(creature) {}

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_STALVAN_STEP_1, 3s);
        _events.ScheduleEvent(EVENT_STALVAN_STEP_2, 8s);
        _events.ScheduleEvent(EVENT_STALVAN_STEP_3, 14s);
        _events.ScheduleEvent(EVENT_STALVAN_STEP_4, 22s);
        _events.ScheduleEvent(EVENT_STALVAN_STEP_5, 26s);
        _events.ScheduleEvent(EVENT_STALVAN_STEP_6, 32s);

        _events.ScheduleEvent(EVENT_TOBIAS_STEP_1, 5s);
        _events.ScheduleEvent(EVENT_TOBIAS_STEP_2, 9s);
        _events.ScheduleEvent(EVENT_TOBIAS_STEP_3, 16s);
        _events.ScheduleEvent(EVENT_TOBIAS_STEP_4, 31s);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_CAST_SPELL_CURSE_OF_STALVAN, 5s);
        _events.ScheduleEvent(EVENT_CAST_SPELL_FETID_BREATH, 8s, 25s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* tobias = GetTobias())
        {
            Talk(SAY_06, tobias); // You see, brother... we''re not so different...
            tobias->AI()->SetData(1, 1);
        }
    }

    Creature* GetTobias()
    {
        if (Creature* tobias = me->FindNearestCreature(NPC_WORGEN_TOBIAS, 20, true))
            return tobias;

        return nullptr;
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_STALVAN_STEP_1:
                me->SetWalk(true);
                me->SetSpeed(MOVE_WALK, 2.5);
                me->GetMotionMaster()->MovePoint(0, stalvanDestination, true);
                Talk(SAY_00, GetTobias()); // My ring... Who holds my family ring... Tilloa, is that you?
                break;

            case EVENT_STALVAN_STEP_2:
                Talk(SAY_01, GetTobias()); // Tobias...
                break;

            case EVENT_STALVAN_STEP_3:
                Talk(SAY_02, GetTobias()); // It''s all true, brother. Every word. You doubted it?
                break;

            case EVENT_STALVAN_STEP_4:
                Talk(SAY_03, GetTobias()); // You know why!
                break;

            case EVENT_STALVAN_STEP_5:
                Talk(SAY_04, GetTobias()); // Surely you''ve felt anger. Anger so foul and vicious that it makes you want to tear someone to shreds...
                break;

            case EVENT_STALVAN_STEP_6:
                Talk(SAY_05, GetTobias()); // Aren''t you feeling it right now?
                break;

            case EVENT_TOBIAS_STEP_1:
                if (Creature* tobias = GetTobias())
                {
                    tobias->SetFacingToObject(me, true);
                    me->SetFacingToObject(tobias, true);
                    tobias->AI()->Talk(SAY_00, me); // Brother!
                }
                break;

            case EVENT_TOBIAS_STEP_2:
                if (Creature* tobias = GetTobias())
                    tobias->AI()->Talk(SAY_01, me); // Tell me it''s not true, brother. Tell me you didn''t die a murderer!
                break;

            case EVENT_TOBIAS_STEP_3:
                if (Creature* tobias = GetTobias())
                    tobias->AI()->Talk(SAY_02, me); // But why?! How could you?
                break;

            case EVENT_TOBIAS_STEP_4:
                if (Creature* tobias = GetTobias())
                {
                    tobias->CastSpell(tobias, SPELL_WORGEN_TRANSFORMATION, true);
                    tobias->SetDisplayId(DISPLAYID_WORGEN_TOBIAS);
                    tobias->SetReactState(REACT_AGGRESSIVE);

                    tobias->AI()->Talk(SAY_03, me); // No... NO! STOP IT!

                    me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
                    me->GetThreatManager().AddThreat(tobias, 10.f);
                    tobias->GetThreatManager().AddThreat(me, 10.f);
                    tobias->SetInCombatWith(me);

                    if (me->Attack(tobias, true))
                        me->GetMotionMaster()->MoveChase(tobias);

                    if (tobias->Attack(me, true))
                        tobias->GetMotionMaster()->MoveChase(me);

                    me->SetReactState(REACT_AGGRESSIVE);
                }
                break;

            case EVENT_CAST_SPELL_CURSE_OF_STALVAN:
                if (Unit* target = SelectTarget(SelectTargetMethod::MaxThreat))
                    me->CastSpell(target, SPELL_CURSE_OF_STALVAN, true);

                _events.ScheduleEvent(EVENT_CAST_SPELL_CURSE_OF_STALVAN, 5s);
                break;

            case EVENT_CAST_SPELL_FETID_BREATH:
                if (Unit* target = SelectTarget(SelectTargetMethod::MaxThreat))
                    me->CastSpell(target, SPELL_FETID_BREATH, false);

                _events.ScheduleEvent(EVENT_CAST_SPELL_FETID_BREATH, 8s, 25s);
                break;

            default:
                break;
            }
        }

        UpdateVictim();
    }

private:
    EventMap _events;
};

// 43797 - Tobias Mistmantle
struct npc_tobias : public ScriptedAI
{
    npc_tobias(Creature* creature) : ScriptedAI(creature)
    {
        _stalvanDead = false;
    }

    void Reset() override
    {
        _events.Reset();
        _stalvanDead = false;
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type == 1 && data == 1)
        {
            _stalvanDead = true;
            _events.ScheduleEvent(EVENT_TOBIAS_SAY_DEATH, 6s);
            _events.ScheduleEvent(EVENT_TOBIAS_ESCAPE, 10s);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_TOBIAS_SAY_DEATH:
                Talk(SAY_04); // No...
                break;

            case EVENT_TOBIAS_ESCAPE:
                me->GetMotionMaster()->MovePoint(0, tobiasForestEscape, true);
                me->SetSpeedRate(MOVE_WALK, 3.0);
                me->SetSpeedRate(MOVE_RUN, 3.0);
                me->DespawnOrUnsummon(4s);
                break;

            default:
                break;
            }
        }

        UpdateVictim();
    }

private:
    EventMap _events;
    bool _stalvanDead;
};

enum SoothingIncenseCloudData
{
    EVENT_SELECT_TARGET = 1,
    NPC_FORLORN_SPIRIT = 43923,
    NPC_FORLORN_SPIRIT_KILLCREDIT = 43930,
};

struct npc_soothing_incense_cloud : public ScriptedAI
{
    npc_soothing_incense_cloud(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_SELECT_TARGET, 500ms);
    }

    Player* GetOwner()
    {
        return me->ToTempSummon()->GetSummoner()->ToPlayer();
    }

    void SelectTargets()
    {
        me->GetCreatureListWithEntryInGrid(_selectedTargets, NPC_FORLORN_SPIRIT, 5.f);
    }

    void KillSelectedCreaturesAndRewardPlayer()
    {
        for (auto creature : _selectedTargets)
        {
            if (!creature->IsAlive())
                continue;

            GetOwner()->RewardPlayerAndGroupAtEvent(NPC_FORLORN_SPIRIT_KILLCREDIT, GetOwner());
            creature->DisappearAndDie();
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_SELECT_TARGET:
                SelectTargets();
                KillSelectedCreaturesAndRewardPlayer();
                break;
            }
        }
    }

private:
    EventMap _events;
    std::list<Creature*> _selectedTargets;
};

enum MorbentFel
{
    SPELL_SACRED_CLEANSING   = 82130,
    NPC_WEAKENED_MORBENT_FEL = 43762,
    NPC_MORBENT_FEL          = 43761
};

class spell_sacred_cleansing : public SpellScript
{
    void SelectTarget(WorldObject*& target)
    {
        target = GetCaster()->FindNearestCreature(NPC_MORBENT_FEL, 15.0f, true);
    }

    SpellCastResult CheckRequirement()
    {
        if (Creature* fel = GetCaster()->FindNearestCreature(NPC_MORBENT_FEL, 15.0f, true))
            return SPELL_CAST_OK;

        return SPELL_FAILED_INCORRECT_AREA;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* hitUnit = GetHitUnit();
        if (!hitUnit || !GetCaster()->IsPlayer())
            return;

        if (Creature* target = hitUnit->ToCreature())
        {
            if (target->GetEntry() == NPC_MORBENT_FEL)
            {
                GetCaster()->SummonCreature(NPC_WEAKENED_MORBENT_FEL, target->GetPosition());
                target->DespawnOrUnsummon();
            }
        }
    }

    void Register() override
    {
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_sacred_cleansing::SelectTarget, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        OnEffectHitTarget += SpellEffectFn(spell_sacred_cleansing::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        OnCheckCast += SpellCheckCastFn(spell_sacred_cleansing::CheckRequirement);
    }
};

// 82029 - Call Stalvan
class spell_summon_stalvan : public SpellScript
{
    bool IsEventRunning()
    {
        return GetCaster()->FindNearestCreature(NPC_STALVAN, 20, true) != nullptr;
    }

    SpellCastResult CheckRequirement()
    {
        return SPELL_CAST_OK;
    }

    void HandleSendEvent(SpellEffIndex /*effIndex*/)
    {
        if (!GetCaster()->IsPlayer())
            return;

        if (GetCaster()->ToPlayer()->GetQuestStatus(QUEST_MISTMANTLES_REVENGE) != QUEST_STATUS_INCOMPLETE)
            return;

        if (IsEventRunning())
            return;

        SummonStalvan();
        SummonTobias();
    }

    void SummonStalvan()
    {
        if (TempSummon* stalvan = GetCaster()->SummonCreature(NPC_STALVAN, stalvanPosition))
        {
            stalvan->SetFacingTo(stalvanOrientation, true);
            stalvan->SetReactState(REACT_PASSIVE);
            stalvan->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
        }
    }

    void SummonTobias()
    {
        if (TempSummon* tobias = GetCaster()->SummonCreature(NPC_WORGEN_TOBIAS, tobiasPositions[0]))
        {
            tobias->SetDisplayId(DISPLAYID_TOBIAS);
            tobias->RemoveNpcFlag(UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
            tobias->SetWalk(true);
            tobias->SetSpeed(MOVE_WALK, 3.5);
            tobias->GetMotionMaster()->MovePoint(0, tobiasPositions[3], true);
            tobias->SetReactState(REACT_PASSIVE);
        }
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_summon_stalvan::HandleSendEvent, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
        OnCheckCast += SpellCheckCastFn(spell_summon_stalvan::CheckRequirement);
    }

};

struct npc_ebenlocke : public ScriptedAI
{
    enum Data
    {
        QUEST_EMBALMERS_REVENGE    = 26727,
        NPC_STITCHES               = 43862
    };
    
    npc_ebenlocke(Creature* creature) : ScriptedAI(creature) { }

    void OnQuestAccept(Player* /*player*/, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_EMBALMERS_REVENGE)
        {
            me->GetScheduler().Schedule(2s, [this](TaskContext& /*context*/)
            {
                me->SummonCreature(NPC_STITCHES, Position(-10553.90f, -1171.27f, 27.8604f, 1.48514f), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s, true);
            });
        }
    }
};

void AddSC_duskwood_custom()
{
    RegisterSpellScript(spell_summon_stalvan);
    RegisterCreatureAI(npc_stalvan);
    RegisterCreatureAI(npc_tobias);
    RegisterCreatureAI(npc_soothing_incense_cloud);
    RegisterSpellScript(spell_sacred_cleansing);
    RegisterCreatureAI(npc_ebenlocke);
}
