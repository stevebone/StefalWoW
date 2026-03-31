/*
 * DoomCore Scripts - Magister's Terrace 12.0.1 Revamp (Map 2811)
 * Scripted Trash Creatures
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "magisters_terrace_12.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"

enum TrashEvents
{
    EVENT_TRASH_SPELL_1     = 1,
    EVENT_TRASH_SPELL_2,
    EVENT_TRASH_SPELL_3,
    EVENT_TRASH_SPELL_4,
    EVENT_TRASH_SPELL_5,
    EVENT_TRASH_HEAL,
    EVENT_TRASH_CHANNEL,
    EVENT_TRASH_YELL,
};

// ============================================================================
// Arcane Sentry (234062)
// ============================================================================
struct npc_arcane_sentry_mt12 : public ScriptedAI
{
    npc_arcane_sentry_mt12(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        Talk(SAY_SENTRY_AGGRO);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 3s, 5s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_2, 6s, 10s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_3, 12s, 15s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_4, 8s, 12s);
        _events.ScheduleEvent(EVENT_TRASH_YELL, 15s, 25s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_SENTRY_DEATH);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TRASH_SPELL_1:
                    DoCastVictim(SPELL_SENTRY_1);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 8s, 12s);
                    break;
                case EVENT_TRASH_SPELL_2:
                    DoCastVictim(SPELL_SENTRY_2);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_2, 10s, 15s);
                    break;
                case EVENT_TRASH_SPELL_3:
                    DoCastAOE(SPELL_SENTRY_3);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_3, 15s, 20s);
                    break;
                case EVENT_TRASH_SPELL_4:
                    DoCastVictim(SPELL_SENTRY_4);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_4, 12s, 18s);
                    break;
                case EVENT_TRASH_YELL:
                    Talk(SAY_SENTRY_COMBAT);
                    _events.ScheduleEvent(EVENT_TRASH_YELL, 20s, 30s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
};

// ============================================================================
// Blazing Pyromancer (251861 / 257161)
// ============================================================================
struct npc_blazing_pyromancer_mt12 : public ScriptedAI
{
    npc_blazing_pyromancer_mt12(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        Talk(SAY_PYRO_AGGRO);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 2s, 4s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_2, 6s, 10s);
        _events.ScheduleEvent(EVENT_TRASH_CHANNEL, 12s, 18s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_4, 15s, 20s);
        _events.ScheduleEvent(EVENT_TRASH_YELL, 10s, 20s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TRASH_SPELL_1:
                    DoCastVictim(SPELL_PYROMANCER_1);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 6s, 10s);
                    break;
                case EVENT_TRASH_SPELL_2:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 35.0f, true))
                        DoCast(target, SPELL_PYROMANCER_2);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_2, 10s, 15s);
                    break;
                case EVENT_TRASH_CHANNEL:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                        DoCast(target, SPELL_PYROMANCER_CHANNEL);
                    _events.ScheduleEvent(EVENT_TRASH_CHANNEL, 18s, 25s);
                    break;
                case EVENT_TRASH_SPELL_4:
                    DoCastAOE(SPELL_PYROMANCER_4);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_4, 20s, 25s);
                    break;
                case EVENT_TRASH_YELL:
                {
                    uint8 say = urand(0, 1) ? SAY_PYRO_COMBAT : SAY_PYRO_COMBAT_2;
                    Talk(say);
                    _events.ScheduleEvent(EVENT_TRASH_YELL, 15s, 25s);
                    break;
                }
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
};

// ============================================================================
// Arcane Magister (232369 / 241326 / 257644)
// ============================================================================
struct npc_arcane_magister_mt12 : public ScriptedAI
{
    npc_arcane_magister_mt12(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 2s, 5s);
        _events.ScheduleEvent(EVENT_TRASH_CHANNEL, 8s, 14s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_3, 12s, 18s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TRASH_SPELL_1:
                    DoCastVictim(SPELL_MAGISTER_1);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 6s, 10s);
                    break;
                case EVENT_TRASH_CHANNEL:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                        DoCast(target, SPELL_MAGISTER_CHANNEL);
                    _events.ScheduleEvent(EVENT_TRASH_CHANNEL, 15s, 22s);
                    break;
                case EVENT_TRASH_SPELL_3:
                    DoCastAOE(SPELL_MAGISTER_3);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_3, 15s, 20s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
};

// ============================================================================
// Lightward Healer (234486 / 241324)
// ============================================================================
struct npc_lightward_healer_mt12 : public ScriptedAI
{
    npc_lightward_healer_mt12(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_TRASH_HEAL, 3s, 6s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TRASH_HEAL:
                    if (Unit* ally = DoSelectLowestHpFriendly(40.0f))
                        DoCast(ally, SPELL_HEALER_HEAL);
                    else
                        DoCastSelf(SPELL_HEALER_HEAL);
                    _events.ScheduleEvent(EVENT_TRASH_HEAL, 6s, 10s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
};

// ============================================================================
// Sunblade Enforcer (234124 / 241325)
// ============================================================================
struct npc_sunblade_enforcer_mt12 : public ScriptedAI
{
    npc_sunblade_enforcer_mt12(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 4s, 8s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TRASH_SPELL_1:
                    DoCastVictim(SPELL_ENFORCER_1);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 8s, 12s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
};

// ============================================================================
// Vigilant Librarian (234067)
// ============================================================================
struct npc_vigilant_librarian_mt12 : public ScriptedAI
{
    npc_vigilant_librarian_mt12(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 3s, 6s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TRASH_SPELL_1:
                    DoCastAOE(SPELL_LIBRARIAN_AT); // Creates AreaTrigger (Entry 41944, Cylinder R=2 H=4)
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 12s, 18s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
};

// ============================================================================
// Lady Sacrolash (260076)
// ============================================================================
struct npc_lady_sacrolash_mt12 : public ScriptedAI
{
    npc_lady_sacrolash_mt12(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 3s, 6s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TRASH_SPELL_1:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 35.0f, true))
                        DoCast(target, SPELL_SACROLASH_1);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 8s, 12s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
};

// ============================================================================
// Antoran Inquisitor (258242)
// ============================================================================
struct npc_antoran_inquisitor_mt12 : public ScriptedAI
{
    npc_antoran_inquisitor_mt12(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 2s, 5s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_2, 8s, 12s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_3, 14s, 18s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_4, 20s, 25s);
        _events.ScheduleEvent(EVENT_TRASH_SPELL_5, 10s, 15s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TRASH_SPELL_1:
                    DoCastVictim(SPELL_INQUISITOR_1);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_1, 8s, 12s);
                    break;
                case EVENT_TRASH_SPELL_2:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 35.0f, true))
                        DoCast(target, SPELL_INQUISITOR_2);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_2, 12s, 18s);
                    break;
                case EVENT_TRASH_SPELL_3:
                    DoCastAOE(SPELL_INQUISITOR_3);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_3, 18s, 25s);
                    break;
                case EVENT_TRASH_SPELL_4:
                    DoCastVictim(SPELL_INQUISITOR_4);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_4, 15s, 22s);
                    break;
                case EVENT_TRASH_SPELL_5:
                    DoCastAOE(SPELL_INQUISITOR_5);
                    _events.ScheduleEvent(EVENT_TRASH_SPELL_5, 12s, 16s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
};

void AddSC_magisters_terrace_12_trash()
{
    RegisterMagistersTerrace12CreatureAI(npc_arcane_sentry_mt12);
    RegisterMagistersTerrace12CreatureAI(npc_blazing_pyromancer_mt12);
    RegisterMagistersTerrace12CreatureAI(npc_arcane_magister_mt12);
    RegisterMagistersTerrace12CreatureAI(npc_lightward_healer_mt12);
    RegisterMagistersTerrace12CreatureAI(npc_sunblade_enforcer_mt12);
    RegisterMagistersTerrace12CreatureAI(npc_vigilant_librarian_mt12);
    RegisterMagistersTerrace12CreatureAI(npc_lady_sacrolash_mt12);
    RegisterMagistersTerrace12CreatureAI(npc_antoran_inquisitor_mt12);
}
