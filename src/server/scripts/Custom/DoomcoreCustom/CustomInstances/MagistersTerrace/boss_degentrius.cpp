/*
 * DoomCore Scripts - Magister's Terrace 12.0.1 Revamp (Map 2811)
 * Boss 4: Degentrius (Entry 231865)
 *
 * Mechanics:
 * - Unstable Void Essence: Bounces to 4 locations. Players soak each impact or Void Destruction AoE.
 *   Mythic: 5th bounce becomes Null Bomb (7s timer, must be triggered by player).
 * - Devouring Entropy: DoT on 2 players. On removal -> Entropy Orbs launched from target.
 * - Void Mote: Placed at Cynosure focal points, channels Void Torrent beam toward boss.
 *   Players in beam path take massive Shadow + stun.
 * - Umbral Eruption: Ground eruptions, Shadow damage + knockback.
 * - Entropy Blast: Tank hit, Physical damage.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "magisters_terrace_12.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "TemporarySummon.h"

enum DegentriusEvents
{
    EVENT_UNSTABLE_VOID_ESSENCE = 1,
    EVENT_UVE_BOUNCE_1,
    EVENT_UVE_BOUNCE_2,
    EVENT_UVE_BOUNCE_3,
    EVENT_NULL_BOMB,
    EVENT_DEVOURING_ENTROPY,
    EVENT_VOID_MOTE,
    EVENT_UMBRAL_ERUPTION,
    EVENT_ENTROPY_BLAST,
    EVENT_COMBAT_YELL,
    EVENT_DEGENTRIUS_MISC,
};

// Void Essence bounce positions (arena)
static constexpr Position BouncePositions[] =
{
    { 300.0f, 20.0f, 8.0f, 0.0f },
    { 310.0f, 5.0f, 8.0f, 0.0f },
    { 295.0f, -10.0f, 8.0f, 0.0f },
    { 315.0f, -5.0f, 8.0f, 0.0f },
};

// Void Mote socket positions (Cynosure focal points)
static constexpr Position MotePositions[] =
{
    { 290.0f, 25.0f, 6.0f, 0.0f },
    { 320.0f, 10.0f, 6.0f, 0.0f },
    { 305.0f, -15.0f, 6.0f, 0.0f },
};

// ============================================================================
// Boss AI: Degentrius
// ============================================================================
struct boss_degentrius : public BossAI
{
    boss_degentrius(Creature* creature) : BossAI(creature, DATA_DEGENTRIUS),
        _uveBounceCount(0) { }

    void Reset() override
    {
        _Reset();
        _uveBounceCount = 0;
        summons.DespawnAll();
    }

    void JustEngagedWith(Unit* who) override
    {
        Talk(SAY_DEGENTRIUS_AGGRO);
        BossAI::JustEngagedWith(who);

        DoCastSelf(SPELL_DEGENTRIUS_OPENING_BUFF);

        events.ScheduleEvent(EVENT_ENTROPY_BLAST, 5s, 8s);
        events.ScheduleEvent(EVENT_UMBRAL_ERUPTION, 12s, 18s);
        events.ScheduleEvent(EVENT_DEVOURING_ENTROPY, 20s, 25s);
        events.ScheduleEvent(EVENT_UNSTABLE_VOID_ESSENCE, 30s, 35s);
        events.ScheduleEvent(EVENT_VOID_MOTE, 40s, 50s);
        events.ScheduleEvent(EVENT_COMBAT_YELL, 15s, 25s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_DEGENTRIUS_DEATH);
        _JustDied();
        summons.DespawnAll();
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_DEGENTRIUS_DARKNESS);
    }

    void JustSummoned(Creature* summon) override
    {
        BossAI::JustSummoned(summon);

        switch (summon->GetEntry())
        {
            case NPC_VOID_MOTE_SOCKET:
                summon->SetReactState(REACT_PASSIVE);
                summon->CastSpell(me, SPELL_VOID_TORRENT, false);
                break;
            case NPC_NULL_BOMB_STALKER:
                summon->SetReactState(REACT_PASSIVE);
                break;
            default:
                break;
        }
    }

    void DoUnstableVoidEssence()
    {
        _uveBounceCount = 0;
        Talk(EMOTE_DEGENTRIUS_UVE);
        Talk(SAY_DEGENTRIUS_SHATTERING);
        DoCastAOE(SPELL_UNSTABLE_VOID_ESSENCE);

        // Schedule sequential bounces
        events.ScheduleEvent(EVENT_UVE_BOUNCE_1, 2s);
        events.ScheduleEvent(EVENT_UVE_BOUNCE_2, 4s);
        events.ScheduleEvent(EVENT_UVE_BOUNCE_3, 6s);

        // Mythic: 5th bounce -> Null Bomb
        if (IsHeroic())
            events.ScheduleEvent(EVENT_NULL_BOMB, 8s);
    }

    void DoVoidMote()
    {
        Talk(SAY_DEGENTRIUS_VOID_HUNGERS);
        uint8 moteIndex = urand(0, 2);
        me->SummonCreature(NPC_VOID_MOTE_SOCKET, MotePositions[moteIndex], TEMPSUMMON_TIMED_DESPAWN, 15s);
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
                case EVENT_ENTROPY_BLAST:
                    DoCastVictim(SPELL_ENTROPY_BLAST);
                    events.ScheduleEvent(EVENT_ENTROPY_BLAST, 8s, 12s);
                    break;
                case EVENT_UMBRAL_ERUPTION:
                    Talk(SAY_DEGENTRIUS_UMBRAL);
                    DoCastAOE(SPELL_UMBRAL_ERUPTION);
                    events.ScheduleEvent(EVENT_UMBRAL_ERUPTION, 20s, 30s);
                    break;
                case EVENT_DEVOURING_ENTROPY:
                {
                    Talk(EMOTE_DEGENTRIUS_ENTROPY);
                    Talk(SAY_DEGENTRIUS_DARKNESS);
                    // Apply to 2 non-tank players
                    std::list<Unit*> targets;
                    uint8 applied = 0;
                    for (uint8 i = 0; i < 5 && applied < 2; ++i)
                    {
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 1, 40.0f, true))
                        {
                            if (!target->HasAura(SPELL_DEVOURING_ENTROPY))
                            {
                                DoCast(target, SPELL_DEVOURING_ENTROPY);
                                ++applied;
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_DEVOURING_ENTROPY, 35s, 45s);
                    break;
                }
                case EVENT_UNSTABLE_VOID_ESSENCE:
                    DoUnstableVoidEssence();
                    events.ScheduleEvent(EVENT_UNSTABLE_VOID_ESSENCE, 40s, 50s);
                    break;
                case EVENT_UVE_BOUNCE_1:
                    me->CastSpell(BouncePositions[0], SPELL_UVE_BOUNCE_1, false);
                    break;
                case EVENT_UVE_BOUNCE_2:
                    me->CastSpell(BouncePositions[1], SPELL_UVE_BOUNCE_2, false);
                    break;
                case EVENT_UVE_BOUNCE_3:
                    me->CastSpell(BouncePositions[2], SPELL_UVE_BOUNCE_3, false);
                    break;
                case EVENT_NULL_BOMB:
                    Talk(SAY_DEGENTRIUS_SEALED);
                    me->SummonCreature(NPC_NULL_BOMB_STALKER, BouncePositions[3], TEMPSUMMON_TIMED_DESPAWN, 7s);
                    me->CastSpell(BouncePositions[3], SPELL_NULL_BOMB, false);
                    break;
                case EVENT_VOID_MOTE:
                    DoVoidMote();
                    events.ScheduleEvent(EVENT_VOID_MOTE, 45s, 55s);
                    break;
                case EVENT_COMBAT_YELL:
                {
                    static uint8 const yells[] = {
                        SAY_DEGENTRIUS_SHADOWS_SURGE,
                        SAY_DEGENTRIUS_DARKNESS,
                        SAY_DEGENTRIUS_SHATTERING,
                        SAY_DEGENTRIUS_VOID_HUNGERS,
                    };
                    Talk(yells[urand(0, 3)]);
                    events.ScheduleEvent(EVENT_COMBAT_YELL, 20s, 35s);
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
    uint8 _uveBounceCount;
};

void AddSC_boss_degentrius()
{
    RegisterMagistersTerrace12CreatureAI(boss_degentrius);
}
