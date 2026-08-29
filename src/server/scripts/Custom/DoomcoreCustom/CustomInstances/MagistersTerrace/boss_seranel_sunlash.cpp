/*
 * DoomCore Scripts - Magister's Terrace 12.0.1 Revamp (Map 2811)
 * Boss 2: Seranel Sunlash (Entry 231863)
 *
 * Mechanics:
 * - Runic Mark: 3s cast on player, moderate AoE damage. If target enters Suppression Zone -> Feedback (silence/pacify).
 * - Suppression Zone: Ground zones placed periodically. Players inside are silenced/pacified.
 * - Vow of Silence: At 100 energy, massive AoE + silence to all players OUTSIDE Suppression Zones.
 * - Hastening Ward: Tank debuff increasing attack speed + Arcane melee damage.
 * - Pre-fight RP exchange with Umbric.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "magisters_terrace_12.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"

enum SeranelEvents
{
    EVENT_RUNIC_MARK            = 1,
    EVENT_SUPPRESSION_ZONE,
    EVENT_HASTENING_WARD,
    EVENT_VOW_OF_SILENCE,
    EVENT_COMBAT_YELL,
    EVENT_ADDITIONAL_ABILITY,

    // Pre-fight RP
    EVENT_RP_SERANEL_1          = 20,
    EVENT_RP_SERANEL_2,
    EVENT_RP_UMBRIC,
    EVENT_RP_SERANEL_AGGRO,
};

// ============================================================================
// Boss AI: Seranel Sunlash
// ============================================================================
struct boss_seranel_sunlash : public BossAI
{
    boss_seranel_sunlash(Creature* creature) : BossAI(creature, DATA_SERANEL_SUNLASH),
        _rpStarted(false) { }

    void Reset() override
    {
        _Reset();
        _rpStarted = false;
        me->SetPower(POWER_ENERGY, 0);
        me->SetMaxPower(POWER_ENERGY, 100);
        me->SetPowerType(POWER_ENERGY);
    }

    void DoAction(int32 action) override
    {
        if (action == ACTION_SERANEL_START_RP && !_rpStarted)
        {
            _rpStarted = true;
            me->SetReactState(REACT_PASSIVE);
            events.ScheduleEvent(EVENT_RP_SERANEL_1, 1s);
        }
    }

    void JustEngagedWith(Unit* who) override
    {
        Talk(SAY_SERANEL_AGGRO_1);
        BossAI::JustEngagedWith(who);

        me->SetPower(POWER_ENERGY, 0);

        events.ScheduleEvent(EVENT_RUNIC_MARK, 6s, 10s);
        events.ScheduleEvent(EVENT_SUPPRESSION_ZONE, 15s, 20s);
        events.ScheduleEvent(EVENT_HASTENING_WARD, 8s, 12s);
        events.ScheduleEvent(EVENT_COMBAT_YELL, 20s, 30s);
        events.ScheduleEvent(EVENT_ADDITIONAL_ABILITY, 25s, 35s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_SERANEL_DEATH);
        _JustDied();
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_SERANEL_SILENCE_1);
    }

    void UpdateAI(uint32 diff) override
    {
        // Handle pre-fight RP events even out of combat
        if (!me->IsInCombat())
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_RP_SERANEL_1:
                        Talk(SAY_SERANEL_PREFIGHT_1);
                        events.ScheduleEvent(EVENT_RP_SERANEL_2, 3s);
                        break;
                    case EVENT_RP_SERANEL_2:
                        Talk(SAY_SERANEL_PREFIGHT_2);
                        events.ScheduleEvent(EVENT_RP_UMBRIC, 4s);
                        break;
                    case EVENT_RP_UMBRIC:
                        if (InstanceScript* instance = me->GetInstanceScript())
                            if (Creature* umbric = instance->GetCreature(DATA_MAGISTER_UMBRIC))
                                umbric->AI()->Talk(SAY_UMBRIC_SERANEL_CONFRONT);
                        events.ScheduleEvent(EVENT_RP_SERANEL_AGGRO, 5s);
                        break;
                    case EVENT_RP_SERANEL_AGGRO:
                        Talk(SAY_SERANEL_AGGRO_2);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
                        break;
                    default:
                        break;
                }
            }
            return;
        }

        if (!UpdateVictim())
            return;

        events.Update(diff);

        // Energy regeneration: gain ~2 energy per second
        if (me->GetPower(POWER_ENERGY) < 100)
        {
            uint32 currentEnergy = me->GetPower(POWER_ENERGY);
            // Approximate: 2 energy per second = ~2 per 1000ms
            // Since UpdateAI is called frequently, we accumulate fractionally
            static uint32 energyTimer = 0;
            energyTimer += diff;
            if (energyTimer >= 500) // Every 0.5s gain 1 energy
            {
                me->SetPower(POWER_ENERGY, std::min<uint32>(currentEnergy + 1, 100));
                energyTimer = 0;
            }
        }

        // Check for Vow of Silence at 100 energy
        if (me->GetPower(POWER_ENERGY) >= 100)
        {
            Talk(EMOTE_SERANEL_WAVE_OF_SILENCE);
            Talk(SAY_SERANEL_SILENCE_2);
            DoCastAOE(SPELL_VOW_OF_SILENCE);
            me->SetPower(POWER_ENERGY, 0);
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_RUNIC_MARK:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                    {
                        Talk(EMOTE_SERANEL_RUNIC_MARK);
                        Talk(SAY_SERANEL_RUNIC_MARK);
                        DoCastSelf(SPELL_RUNIC_MARK_TRIGGER);
                        DoCast(target, SPELL_RUNIC_MARK);
                    }
                    events.ScheduleEvent(EVENT_RUNIC_MARK, 8s, 12s);
                    break;
                case EVENT_SUPPRESSION_ZONE:
                    DoCastSelf(SPELL_SUPPRESSION_ZONE);
                    events.ScheduleEvent(EVENT_SUPPRESSION_ZONE, 30s, 40s);
                    break;
                case EVENT_HASTENING_WARD:
                    DoCastVictim(SPELL_HASTENING_WARD_MELEE);
                    events.ScheduleEvent(EVENT_HASTENING_WARD, 20s, 25s);
                    break;
                case EVENT_COMBAT_YELL:
                {
                    uint8 say = urand(0, 1) ? SAY_SERANEL_UNYIELDING : SAY_SERANEL_SILENCE_1;
                    Talk(say);
                    events.ScheduleEvent(EVENT_COMBAT_YELL, 20s, 35s);
                    break;
                }
                case EVENT_ADDITIONAL_ABILITY:
                    DoCastAOE(SPELL_SERANEL_ADDITIONAL);
                    events.ScheduleEvent(EVENT_ADDITIONAL_ABILITY, 20s, 30s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    bool _rpStarted;
};

void AddSC_boss_seranel_sunlash()
{
    RegisterMagistersTerrace12CreatureAI(boss_seranel_sunlash);
}
