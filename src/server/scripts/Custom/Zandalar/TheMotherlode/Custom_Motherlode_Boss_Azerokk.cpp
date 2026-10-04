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

/*
 * Azerokk (entry 129227) - MOTHERLODE boss 2/4.
 *
 * Calls Earthragers that fixate a player and stack a Jagged Cut bleed on them.
 * Azerite Infusion empowers one rager (heals + arcane pulses) for 20s.
 * Resonant Quake hits the raid and makes every rager erupt an aftershock.
 * Tectonic Smash is a long frontal line on the tank.
 *
 * Ported from the Urlatek EncounterBatch implementation, rewritten on BossAI.
 */

#include "AreaTrigger.h"
#include "GameTime.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "ThreatManager.h"

#include "Custom_Motherlode_Defines.h"

namespace Scripts::Zandalar::TheMotherlode
{
    namespace
    {
        constexpr float TectonicSmashLength = 30.0f;
        constexpr float TectonicSmashWidth = 6.0f;
        constexpr float RagerMeleeRange = 5.0f;
        constexpr uint32 RagerInfusionMs = 20000;
        constexpr uint32 RagerTickMs = 2000;

        // A point on the segment a->b within `width` (generous z tolerance for terrain).
        bool OnSegment(Position const& point, Position const& a, Position const& b, float width)
        {
            float dx = b.GetPositionX() - a.GetPositionX(), dy = b.GetPositionY() - a.GetPositionY(), length = dx * dx + dy * dy;
            if (length < 0.01f)
                return false;
            float t = ((point.GetPositionX() - a.GetPositionX()) * dx + (point.GetPositionY() - a.GetPositionY()) * dy) / length;
            if (t <= 0.0f || t >= 1.0f || std::abs(point.GetPositionZ() - (a.GetPositionZ() + t * (b.GetPositionZ() - a.GetPositionZ()))) > 8.0f)
                return false;
            float x = point.GetPositionX() - (a.GetPositionX() + t * dx), y = point.GetPositionY() - (a.GetPositionY() + t * dy);
            return x * x + y * y <= width * width;
        }
    }

    struct boss_azerokk : public BossAI
    {
        boss_azerokk(Creature* creature) : BossAI(creature, DataTypes::BOSS_AZEROKK) { }

        void Reset() override
        {
            _Reset();
            _ragerFixate.clear();
            _ragerNextTick.clear();
            _ragerTick = 0;
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            Talk(Azerokk::Voice::Aggro);

            events.ScheduleEvent(Azerokk::Events::CallEarthrager, 20s, 40s);
            events.ScheduleEvent(Azerokk::Events::AzeriteInfusion, 12s, 30s);
            events.ScheduleEvent(Azerokk::Events::ResonantQuake, 28s, 40s);
            events.ScheduleEvent(Azerokk::Events::TectonicSmash, 7s, 18s);

            // Two ragers burst out of the ground on the pull.
            CallRager();
            CallRager();
        }

        void JustSummoned(Creature* summon) override
        {
            BossAI::JustSummoned(summon);

            if (summon->GetEntry() == Creatures::Earthrager)
            {
                summon->SetReactState(REACT_AGGRESSIVE);
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
                {
                    _ragerFixate[summon->GetGUID()] = target->GetGUID();
                    me->AddAura(Azerokk::Spells::RagingGaze, target);
                    summon->GetMotionMaster()->MoveChase(target);
                }
            }
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            BossAI::SummonedCreatureDespawn(summon);
            _ragerFixate.erase(summon->GetGUID());
            _ragerNextTick.erase(summon->GetGUID());
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            Talk(Azerokk::Voice::Death);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                SayVoice(Azerokk::Voice::Slay);
        }

        // Shared 10s cooldown for ability callouts so rapid-fire events don't spam chat.
        void SayVoice(uint8 group)
        {
            uint32 now = GameTime::GetGameTimeMS();
            if (now - _lastVoiceCall < 10000)
                return;
            _lastVoiceCall = now;
            Talk(group);
        }

        void CallRager()
        {
            SayVoice(Azerokk::Voice::CallEarthrager);
            Position pos = me->GetRandomPoint(me->GetHomePosition(), 12.0f);
            me->SummonCreature(Creatures::Earthrager, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3s);
            DoCastSelf(Azerokk::Spells::CallEarthrager, true);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            scheduler.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Azerokk::Events::CallEarthrager:
                        CallRager();
                        events.ScheduleEvent(Azerokk::Events::CallEarthrager, 20s, 40s);
                        break;
                    case Azerokk::Events::AzeriteInfusion:
                    {
                        SayVoice(Azerokk::Voice::AzeriteInfusion);
                        // Prefer a rager that is not already infused or stunned.
                        Creature* pick = nullptr;
                        for (auto const& pair : _ragerFixate)
                            if (Creature* rager = ObjectAccessor::GetCreature(*me, pair.first))
                                if (rager->IsAlive() && !rager->HasAura(Azerokk::Spells::AzeriteInfusion)
                                    && (!pick || !rager->HasUnitState(UNIT_STATE_STUNNED)))
                                    pick = rager;
                        if (pick)
                        {
                            DoCastSelf(Azerokk::Spells::AzeriteInfusion, true);
                            me->AddAura(Azerokk::Spells::AzeriteInfusion, pick);
                            if (Aura* infusion = pick->GetAura(Azerokk::Spells::AzeriteInfusion))
                            {
                                infusion->SetMaxDuration(RagerInfusionMs);
                                infusion->SetDuration(RagerInfusionMs);
                            }
                        }
                        events.ScheduleEvent(Azerokk::Events::AzeriteInfusion, 12s, 30s);
                        break;
                    }
                    case Azerokk::Events::ResonantQuake:
                    {
                        SayVoice(Azerokk::Voice::ResonantQuake);
                        DoCastSelf(Azerokk::Spells::ResonantQuake, true);
                        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                            if (Unit* victim = ref->GetVictim())
                                Combat::PercentHit(me, victim, Azerokk::Spells::ResonantQuakeHit, 0.20f, SPELL_SCHOOL_MASK_NATURE);

                        // Every rager still standing erupts an aftershock.
                        for (auto const& pair : _ragerFixate)
                            if (Creature* rager = ObjectAccessor::GetCreature(*me, pair.first))
                                if (rager->IsAlive())
                                    for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                                        if (Unit* victim = ref->GetVictim())
                                            Combat::PercentHit(rager, victim, Azerokk::Spells::Aftershock, 0.10f, SPELL_SCHOOL_MASK_NATURE);

                        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                            if (Unit* victim = ref->GetVictim())
                                Combat::PercentHit(me, victim, Azerokk::Spells::AzeriteAftershock, 0.12f, SPELL_SCHOOL_MASK_ARCANE);
                        events.ScheduleEvent(Azerokk::Events::ResonantQuake, 28s, 40s);
                        break;
                    }
                    case Azerokk::Events::TectonicSmash:
                    {
                        SayVoice(Azerokk::Voice::TectonicSmash);
                        if (Unit* tank = SelectTarget(SelectTargetMethod::MaxThreat, 0, 50.0f, true))
                        {
                            float angle = me->GetAbsoluteAngle(tank);
                            Position origin = me->GetPosition();
                            Position end = origin;
                            end.m_positionX = origin.GetPositionX() + std::cos(angle) * TectonicSmashLength;
                            end.m_positionY = origin.GetPositionY() + std::sin(angle) * TectonicSmashLength;
                            end.m_positionZ = origin.GetPositionZ();

                            AreaTrigger::CreateAreaTrigger({ AreaTriggerTelegraphs::TectonicSmash, false }, end, 3500,
                                me, nullptr, { 0, 0 }, sSpellMgr->GetSpellInfo(Azerokk::Spells::TectonicSmash, me->GetMap()->GetDifficultyID()));
                            DoCast(tank, Azerokk::Spells::TectonicSmash);
                            scheduler.Schedule(3500ms, [this, origin, end](TaskContext)
                            {
                                for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                                    if (Unit* victim = ref->GetVictim())
                                        if (OnSegment(victim->GetPosition(), origin, end, TectonicSmashWidth))
                                        {
                                            Combat::PercentHit(me, victim, Azerokk::Spells::TectonicSmash, 0.30f, SPELL_SCHOOL_MASK_NATURE);
                                            victim->KnockbackFrom(origin, 4.0f, 12.0f);
                                        }
                            });
                        }
                        events.ScheduleEvent(Azerokk::Events::TectonicSmash, 7s, 18s);
                        break;
                    }
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            // Rager upkeep: chase the fixated player (re-fixate if it died), stack
            // Jagged Cut in melee, and pulse while under Azerite Infusion.
            _ragerTick += diff;
            if (_ragerTick >= 500)
            {
                uint32 step = _ragerTick;
                _ragerTick = 0;

                for (auto const& pair : _ragerFixate)
                {
                    Creature* rager = ObjectAccessor::GetCreature(*me, pair.first);
                    if (!rager || !rager->IsAlive())
                        continue;

                    _ragerNextTick[pair.first] += step;
                    if (_ragerNextTick[pair.first] < RagerTickMs)
                        continue;
                    _ragerNextTick[pair.first] = 0;

                    Unit* target = ObjectAccessor::GetUnit(*me, pair.second);
                    if (!target || !target->IsAlive())
                    {
                        if (target)
                            target->RemoveAurasDueToSpell(Azerokk::Spells::RagingGaze);
                        if (Unit* next = SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
                        {
                            _ragerFixate[pair.first] = next->GetGUID();
                            me->AddAura(Azerokk::Spells::RagingGaze, next);
                            target = next;
                        }
                        else
                            continue;
                    }

                    rager->GetMotionMaster()->MoveChase(target);

                    if (rager->GetExactDist2d(target) <= RagerMeleeRange && !rager->HasUnitState(UNIT_STATE_STUNNED))
                    {
                        if (Aura* cut = target->GetAura(Azerokk::Spells::JaggedCut, me->GetGUID()))
                            cut->ModStackAmount(1);
                        else
                            me->AddAura(Azerokk::Spells::JaggedCut, target);
                    }

                    if (rager->HasAura(Azerokk::Spells::AzeriteInfusion))
                    {
                        rager->ModifyHealth(int64(rager->CountPctFromMaxHealth(5)));
                        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                            if (Unit* victim = ref->GetVictim())
                                Combat::PercentHit(rager, victim, Azerokk::Spells::InfusionPulse, 0.06f, SPELL_SCHOOL_MASK_ARCANE);
                    }
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        // rager guid -> fixated victim guid
        std::unordered_map<ObjectGuid, ObjectGuid> _ragerFixate;
        std::unordered_map<ObjectGuid, uint32> _ragerNextTick;
        uint32 _ragerTick = 0;
        uint32 _lastVoiceCall = 0;
    };
}

void AddSC_custom_boss_azerokk()
{
    using namespace Scripts::Zandalar::TheMotherlode;

    RegisterCreatureAI(boss_azerokk);
}
