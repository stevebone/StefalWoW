/*
 * DoomCore Scripts - Magister's Terrace 12.0.1 Revamp (Map 2811)
 * Boss 1: Arcanotron Custos (Entry 231861)
 *
 * Mechanics:
 * - Refueling Protocol: At low energy, channels and spawns Energy Orbs that move toward boss.
 *   Players intercept orbs (Arcane Residue pool + Unstable Energy debuff).
 *   Orbs reaching boss grant Arcane Empowerment (raid AoE + 20% damage buff).
 * - Arcane Expulsion: AoE knockback + Arcane Residue pool at boss feet.
 * - Ethereal Shackles: Root + DoT on a player.
 * - Repulsing Slam: Tank hit with knockback.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "magisters_terrace_12.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "TemporarySummon.h"

enum ArcanotronEvents
{
    EVENT_ARCANE_EXPULSION      = 1,
    EVENT_ETHEREAL_SHACKLES,
    EVENT_REPULSING_SLAM,
    EVENT_REFUELING_PROTOCOL,
    EVENT_REFUELING_END,
    EVENT_COMBAT_YELL,
    EVENT_RESUME_COMBAT,
};

enum ArcanotronPhases
{
    PHASE_COMBAT                = 1,
    PHASE_REFUELING             = 2,
};

// Energy Orb spawn positions (arena edges)
static constexpr Position OrbSpawnPositions[] =
{
    { 196.0f, 3.0f, 2.0f, 0.0f },
    { 175.0f, 25.0f, 2.0f, 0.0f },
    { 210.0f, -15.0f, 2.0f, 0.0f },
    { 185.0f, -20.0f, 2.0f, 0.0f },
    { 200.0f, 18.0f, 2.0f, 0.0f },
};

// ============================================================================
// Boss AI: Arcanotron Custos
// ============================================================================
struct boss_arcanotron_custos : public BossAI
{
    boss_arcanotron_custos(Creature* creature) : BossAI(creature, DATA_ARCANOTRON_CUSTOS),
        _refuelingCount(0) { }

    void Reset() override
    {
        _Reset();
        _refuelingCount = 0;
        DespawnEnergyOrbs();
    }

    void JustEngagedWith(Unit* who) override
    {
        Talk(SAY_ARCANOTRON_AGGRO);
        BossAI::JustEngagedWith(who);

        events.SetPhase(PHASE_COMBAT);
        events.ScheduleEvent(EVENT_ARCANE_EXPULSION, 8s, 12s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_ETHEREAL_SHACKLES, 10s, 15s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_REPULSING_SLAM, 5s, 8s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_REFUELING_PROTOCOL, 30s, 40s, 0, PHASE_COMBAT);
        events.ScheduleEvent(EVENT_COMBAT_YELL, 15s, 25s, 0, PHASE_COMBAT);
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_ARCANOTRON_DEATH);
        _JustDied();
        DespawnEnergyOrbs();
    }

    void DespawnEnergyOrbs()
    {
        summons.DespawnEntry(NPC_ENERGY_ORB);
    }

    void JustSummoned(Creature* summon) override
    {
        BossAI::JustSummoned(summon);

        if (summon->GetEntry() == NPC_ENERGY_ORB)
        {
            summon->SetReactState(REACT_PASSIVE);
            summon->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
        }
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (summon->GetEntry() == NPC_ENERGY_ORB)
            summons.Despawn(summon);
    }

    void SpawnEnergyOrbs()
    {
        uint8 orbCount = 3 + _refuelingCount; // More orbs each refueling
        if (orbCount > 5)
            orbCount = 5;

        for (uint8 i = 0; i < orbCount; ++i)
            me->SummonCreature(NPC_ENERGY_ORB, OrbSpawnPositions[i], TEMPSUMMON_CORPSE_DESPAWN);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_ARCANE_EXPULSION:
                    DoCastAOE(SPELL_ARCANE_EXPULSION);
                    DoCastAOE(SPELL_ARCANE_EXPULSION_POOL);
                    events.ScheduleEvent(EVENT_ARCANE_EXPULSION, 15s, 20s, 0, PHASE_COMBAT);
                    break;
                case EVENT_ETHEREAL_SHACKLES:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                    {
                        Talk(SAY_ARCANOTRON_STILL);
                        DoCast(target, SPELL_ETHEREAL_SHACKLES);
                    }
                    events.ScheduleEvent(EVENT_ETHEREAL_SHACKLES, 15s, 20s, 0, PHASE_COMBAT);
                    break;
                case EVENT_REPULSING_SLAM:
                    DoCastVictim(SPELL_REPULSING_SLAM);
                    events.ScheduleEvent(EVENT_REPULSING_SLAM, 8s, 12s, 0, PHASE_COMBAT);
                    break;
                case EVENT_REFUELING_PROTOCOL:
                    Talk(SAY_ARCANOTRON_REFUELING);
                    DoCastSelf(SPELL_REFUELING_PROTOCOL);
                    events.SetPhase(PHASE_REFUELING);
                    SpawnEnergyOrbs();
                    events.ScheduleEvent(EVENT_REFUELING_END, 20s, 0, PHASE_REFUELING);
                    break;
                case EVENT_REFUELING_END:
                    ++_refuelingCount;
                    Talk(SAY_ARCANOTRON_RESUME);
                    events.SetPhase(PHASE_COMBAT);
                    events.ScheduleEvent(EVENT_ARCANE_EXPULSION, 5s, 8s, 0, PHASE_COMBAT);
                    events.ScheduleEvent(EVENT_ETHEREAL_SHACKLES, 8s, 12s, 0, PHASE_COMBAT);
                    events.ScheduleEvent(EVENT_REPULSING_SLAM, 4s, 6s, 0, PHASE_COMBAT);
                    events.ScheduleEvent(EVENT_REFUELING_PROTOCOL, 30s, 40s, 0, PHASE_COMBAT);
                    break;
                case EVENT_COMBAT_YELL:
                {
                    uint8 sayIndex = urand(SAY_ARCANOTRON_POLICY, SAY_ARCANOTRON_COMPLIANCE);
                    Talk(sayIndex);
                    events.ScheduleEvent(EVENT_COMBAT_YELL, 20s, 35s, 0, PHASE_COMBAT);
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
    uint8 _refuelingCount;
};

// ============================================================================
// NPC AI: Energy Orb (Entry 234775)
// ============================================================================
struct npc_energy_orb_mt12 : public ScriptedAI
{
    npc_energy_orb_mt12(Creature* creature) : ScriptedAI(creature), _reachedBoss(false) { }

    void Reset() override
    {
        _reachedBoss = false;
        me->SetReactState(REACT_PASSIVE);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (_reachedBoss)
            return;

        // Player intercepts the orb
        if (who->GetTypeId() == TYPEID_PLAYER && me->GetExactDist2d(who) <= 2.0f)
        {
            DoCast(who, SPELL_ARCANE_EXPULSION_POOL); // Arcane Residue at orb location
            me->DespawnOrUnsummon();
            return;
        }
    }

    void MovementInform(uint32 type, uint32 /*id*/) override
    {
        if (type != POINT_MOTION_TYPE)
            return;

        // Orb reached the boss -> Arcane Empowerment
        _reachedBoss = true;

        if (InstanceScript* instance = me->GetInstanceScript())
        {
            if (Creature* boss = instance->GetCreature(DATA_ARCANOTRON_CUSTOS))
            {
                boss->CastSpell(boss, SPELL_ARCANE_EMPOWERMENT, true);
                boss->CastSpell(nullptr, SPELL_ARCANE_EXPULSION, true); // Raid AoE on empowerment
            }
        }

        me->DespawnOrUnsummon();
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        // Orb does not engage in melee, it just moves toward the boss
    }

private:
    bool _reachedBoss;
};

void AddSC_boss_arcanotron_custos()
{
    RegisterMagistersTerrace12CreatureAI(boss_arcanotron_custos);
    RegisterMagistersTerrace12CreatureAI(npc_energy_orb_mt12);
}
