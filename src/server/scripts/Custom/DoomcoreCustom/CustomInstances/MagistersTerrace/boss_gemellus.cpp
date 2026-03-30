/*
 * DoomCore Scripts - Magister's Terrace 12.0.1 Revamp (Map 2811)
 * Boss 3: Gemellus (Entry 231864, Clone: 239636)
 *
 * Mechanics:
 * - Triplicate: At 90% and 50% HP, creates 2 clones (Entry 239636). All share health via Synaptic Nexus.
 * - Neural Link: Links boss to a player — target takes 30% less damage, linked player takes 30% more for 7s.
 * - Astral Grasp: At 100 energy, tentacle pulls players toward their respective Gemellus. Shadow damage.
 * - Cosmic Radiation: Damage to players pulled within grasp range.
 * - Void Secretions: Pool at player location.
 * - Cosmic Sting: Shadow DoT.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "magisters_terrace_12.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "TemporarySummon.h"

enum GemellusEvents
{
    EVENT_COSMIC_STING          = 1,
    EVENT_VOID_SECRETIONS,
    EVENT_NEURAL_LINK,
    EVENT_COSMIC_RADIATION,
    EVENT_ASTRAL_GRASP,
    EVENT_ASTRAL_GRASP_END,
    EVENT_GEMELLUS_MISC,
};

enum GemellusPhases
{
    PHASE_ONE                   = 1,  // 100% - 90%
    PHASE_TWO                   = 2,  // 90% - 50% (2 clones)
    PHASE_THREE                 = 3,  // 50% - 0% (2 more clones)
};

static constexpr Position CloneSpawnPositions[] =
{
    { 240.0f, 35.0f, 5.0f, 0.0f },
    { 260.0f, 15.0f, 5.0f, 0.0f },
    { 250.0f, -10.0f, 5.0f, 0.0f },
    { 235.0f, -5.0f, 5.0f, 0.0f },
};

// ============================================================================
// Boss AI: Gemellus (Main)
// ============================================================================
struct boss_gemellus : public BossAI
{
    boss_gemellus(Creature* creature) : BossAI(creature, DATA_GEMELLUS),
        _triplicated90(false), _triplicated50(false) { }

    void Reset() override
    {
        _Reset();
        _triplicated90 = false;
        _triplicated50 = false;
        me->SetPower(POWER_ENERGY, 0);
        me->SetMaxPower(POWER_ENERGY, 100);
        me->SetPowerType(POWER_ENERGY);
        DespawnClones();
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);

        DoCastSelf(SPELL_SYNAPTIC_NEXUS);
        DoCastSelf(SPELL_SYNAPTIC_NEXUS_VISUAL);

        events.ScheduleEvent(EVENT_COSMIC_STING, 6s, 10s);
        events.ScheduleEvent(EVENT_VOID_SECRETIONS, 12s, 18s);
        events.ScheduleEvent(EVENT_NEURAL_LINK, 15s, 20s);
        events.ScheduleEvent(EVENT_COSMIC_RADIATION, 20s, 25s);
        events.ScheduleEvent(EVENT_GEMELLUS_MISC, 30s, 40s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        DespawnClones();
    }

    void DespawnClones()
    {
        summons.DespawnEntry(NPC_GEMELLUS_CLONE);
        summons.DespawnEntry(NPC_GEMELLUS_SPAWNER);
    }

    void JustSummoned(Creature* summon) override
    {
        BossAI::JustSummoned(summon);

        if (summon->GetEntry() == NPC_GEMELLUS_CLONE)
        {
            summon->CastSpell(summon, SPELL_SYNAPTIC_NEXUS, true);
            summon->CastSpell(summon, SPELL_SYNAPTIC_NEXUS_VISUAL, true);

            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
            {
                summon->AI()->AttackStart(target);
                summon->GetMotionMaster()->MoveChase(target);
            }
        }
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
    {
        // Triplicate at 90% HP
        if (!_triplicated90 && me->HealthBelowPctDamaged(90, damage))
        {
            _triplicated90 = true;
            DoTriplicate();
        }

        // Triplicate at 50% HP
        if (!_triplicated50 && me->HealthBelowPctDamaged(50, damage))
        {
            _triplicated50 = true;
            DoTriplicate();
        }

        // Forward damage to clones (shared health via Synaptic Nexus)
        SyncHealthToClones();
    }

    void DoTriplicate()
    {
        Talk(EMOTE_GEMELLUS_TRIPLICATE);
        DoCastSelf(SPELL_TRIPLICATE);

        // Spawn 2 clones
        for (uint8 i = 0; i < 2; ++i)
        {
            uint8 idx = _triplicated50 ? i + 2 : i;
            if (idx < 4)
                me->SummonCreature(NPC_GEMELLUS_CLONE, CloneSpawnPositions[idx], TEMPSUMMON_CORPSE_DESPAWN);
        }
    }

    void SyncHealthToClones()
    {
        float pct = me->GetHealthPct();
        for (ObjectGuid const& guid : summons)
        {
            if (Creature* clone = ObjectAccessor::GetCreature(*me, guid))
            {
                if (clone->GetEntry() == NPC_GEMELLUS_CLONE && clone->IsAlive())
                    clone->SetHealth(clone->CountPctFromMaxHealth(static_cast<int32>(pct)));
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        // Energy regeneration: ~2 energy per second
        {
            static uint32 energyTimer = 0;
            energyTimer += diff;
            if (energyTimer >= 500)
            {
                uint32 currentEnergy = me->GetPower(POWER_ENERGY);
                if (currentEnergy < 100)
                    me->SetPower(POWER_ENERGY, std::min<uint32>(currentEnergy + 1, 100));
                energyTimer = 0;
            }
        }

        // Astral Grasp at 100 energy
        if (me->GetPower(POWER_ENERGY) >= 100)
        {
            Talk(EMOTE_GEMELLUS_ASTRAL_GRASP);
            DoCastAOE(SPELL_ASTRAL_GRASP);
            me->SetPower(POWER_ENERGY, 0);
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_COSMIC_STING:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                        DoCast(target, SPELL_COSMIC_STING);
                    events.ScheduleEvent(EVENT_COSMIC_STING, 10s, 15s);
                    break;
                case EVENT_VOID_SECRETIONS:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                        DoCast(target, SPELL_VOID_SECRETIONS);
                    events.ScheduleEvent(EVENT_VOID_SECRETIONS, 12s, 18s);
                    break;
                case EVENT_NEURAL_LINK:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 40.0f, true))
                    {
                        DoCast(target, SPELL_NEURAL_LINK);
                        DoCast(target, SPELL_NEURAL_LINK_DEBUFF_1);
                    }
                    events.ScheduleEvent(EVENT_NEURAL_LINK, 20s, 30s);
                    break;
                case EVENT_COSMIC_RADIATION:
                    DoCastAOE(SPELL_COSMIC_RADIATION);
                    events.ScheduleEvent(EVENT_COSMIC_RADIATION, 15s, 25s);
                    break;
                case EVENT_GEMELLUS_MISC:
                    DoCastAOE(SPELL_GEMELLUS_MISC_2);
                    events.ScheduleEvent(EVENT_GEMELLUS_MISC, 25s, 35s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    bool _triplicated90;
    bool _triplicated50;
};

// ============================================================================
// NPC AI: Gemellus Clone (Entry 239636)
// ============================================================================
struct npc_gemellus_clone : public ScriptedAI
{
    npc_gemellus_clone(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        me->SetPower(POWER_ENERGY, 0);
        me->SetMaxPower(POWER_ENERGY, 100);
        me->SetPowerType(POWER_ENERGY);
        _events.ScheduleEvent(EVENT_COSMIC_STING, 8s, 12s);
        _events.ScheduleEvent(EVENT_VOID_SECRETIONS, 14s, 20s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
    {
        // Sync damage back to main boss
        if (InstanceScript* instance = me->GetInstanceScript())
        {
            if (Creature* boss = instance->GetCreature(DATA_GEMELLUS))
            {
                if (boss->IsAlive())
                {
                    float pct = me->GetHealthPct();
                    boss->SetHealth(boss->CountPctFromMaxHealth(static_cast<int32>(pct)));
                }
            }
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->DespawnOrUnsummon(3s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        // Energy regeneration
        {
            static uint32 energyTimer = 0;
            energyTimer += diff;
            if (energyTimer >= 500)
            {
                uint32 currentEnergy = me->GetPower(POWER_ENERGY);
                if (currentEnergy < 100)
                    me->SetPower(POWER_ENERGY, std::min<uint32>(currentEnergy + 1, 100));
                energyTimer = 0;
            }
        }

        // Astral Grasp at 100 energy (clone version)
        if (me->GetPower(POWER_ENERGY) >= 100)
        {
            DoCastAOE(SPELL_ASTRAL_GRASP);
            me->SetPower(POWER_ENERGY, 0);
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_COSMIC_STING:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                        DoCast(target, SPELL_COSMIC_STING);
                    _events.ScheduleEvent(EVENT_COSMIC_STING, 10s, 15s);
                    break;
                case EVENT_VOID_SECRETIONS:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                        DoCast(target, SPELL_VOID_SECRETIONS);
                    _events.ScheduleEvent(EVENT_VOID_SECRETIONS, 12s, 18s);
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

void AddSC_boss_gemellus()
{
    RegisterMagistersTerrace12CreatureAI(boss_gemellus);
    RegisterMagistersTerrace12CreatureAI(npc_gemellus_clone);
}
