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
 * Rixxa Fluxflame (entry 129231) - MOTHERLODE boss 3/4.
 *
 * Azerite Catalyst dumps a fan of burning chemical trails behind a target;
 * the ground fire is tracked manually because spell 259533 self-targets the
 * boss instead of laying real patches. Chemical Burn DoTs two players,
 * Searing Reagent is the tank hit, and Propellant Blast fires a knockback
 * jet. Gushing Catalyst adds a long fire line on Heroic+.
 *
 * Ported from the Urlatek EncounterBatch implementation, rewritten on BossAI.
 */

#include "AreaTrigger.h"
#include "GameTime.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "ThreatManager.h"

#include "Custom_Motherlode_Defines.h"

namespace Scripts::Zandalar::TheMotherlode
{
    namespace
    {
        constexpr float FireRadius = 3.0f;
        constexpr uint32 FireLifetimeMs = 30000;
        constexpr uint32 FireTickMs = 1000;

        struct GroundFire
        {
            Position Pos;
            uint32 Until; // GameTime ms
        };
    }

    struct boss_rixxa_fluxflame : public BossAI
    {
        boss_rixxa_fluxflame(Creature* creature) : BossAI(creature, DataTypes::BOSS_RIXXA_FLUXFLAME) { }

        void Reset() override
        {
            _Reset();
            _fires.clear();
            _fireTick = 0;
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            Talk(Rixxa::Voice::Aggro);

            events.ScheduleEvent(Rixxa::Events::AzeriteCatalyst, 6s, 15s);
            events.ScheduleEvent(Rixxa::Events::ChemicalBurn, 10s, 18s);
            events.ScheduleEvent(Rixxa::Events::SearingReagent, 3s, 6s);
            events.ScheduleEvent(Rixxa::Events::PropellantBlast, 20s, 30s);
            if (me->GetMap()->IsHeroicOrHigher())
                events.ScheduleEvent(Rixxa::Events::GushingCatalyst, 30s, 45s);
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            Talk(Rixxa::Voice::Death);
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

        void LayFirePatch(Position const& pos)
        {
            _fires.push_back({ pos, GameTime::GetGameTimeMS() + FireLifetimeMs });
            AreaTrigger::CreateAreaTrigger({ AreaTriggerTelegraphs::CatalystFire, true }, pos, FireLifetimeMs,
                me, nullptr, { 0, 0 }, sSpellMgr->GetSpellInfo(Rixxa::Spells::AzeriteCatalyst, me->GetMap()->GetDifficultyID()));
        }

        // Offset `dist` yards from `origin` along `angle` - the kit's UrlatekKit::Offset.
        static Position Offset(Position const& origin, float angle, float dist)
        {
            Position pos = origin;
            pos.m_positionX += std::cos(angle) * dist;
            pos.m_positionY += std::sin(angle) * dist;
            return pos;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                // Approach intro: plays once when a living player wanders close in line of sight.
                if (!_introSaid)
                {
                    _introCheck += diff;
                    if (_introCheck >= 500)
                    {
                        _introCheck = 0;
                        if (Player* player = me->SelectNearestPlayer(40.0f))
                            if (player->IsAlive() && !player->IsGameMaster()
                                && me->IsWithinLOSInMap(player)
                                && std::fabs(player->GetPositionZ() - me->GetPositionZ()) < 15.0f)
                            {
                                _introSaid = true;
                                Talk(Rixxa::Voice::Intro);
                            }
                    }
                }
                return;
            }

            events.Update(diff);
            scheduler.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Rixxa::Events::AzeriteCatalyst:
                    {
                        SayVoice(Rixxa::Voice::AzeriteCatalyst);
                        // The catalyst trails the victim: one patch per second for 5s at their
                        // live position, so the fire follows them as they run.
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, true))
                        {
                            ObjectGuid targetGuid = target->GetGUID();
                            for (uint8 i = 0; i < 5; ++i)
                                scheduler.Schedule(Milliseconds(i * 1000), [this, targetGuid](TaskContext)
                                {
                                    if (Unit* victim = ObjectAccessor::GetUnit(*me, targetGuid))
                                        if (victim->IsAlive())
                                            LayFirePatch(victim->GetPosition());
                                });
                        }
                        events.ScheduleEvent(Rixxa::Events::AzeriteCatalyst, 6s, 15s);
                        break;
                    }
                    case Rixxa::Events::ChemicalBurn:
                    {
                        SayVoice(Rixxa::Voice::ChemicalBurn);
                        for (uint8 i = 0; i < 2; ++i)
                            if (Unit* victim = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, true))
                                me->AddAura(Rixxa::Spells::ChemicalBurn, victim);
                        events.ScheduleEvent(Rixxa::Events::ChemicalBurn, 10s, 18s);
                        break;
                    }
                    case Rixxa::Events::SearingReagent:
                    {
                        if (Unit* tank = SelectTarget(SelectTargetMethod::MaxThreat, 0, 30.0f, true))
                        {
                            DoCast(tank, Rixxa::Spells::SearingReagent);
                            Combat::PercentHit(me, tank, Rixxa::Spells::SearingReagent, 0.50f, SPELL_SCHOOL_MASK_NATURE);
                        }
                        events.ScheduleEvent(Rixxa::Events::SearingReagent, 3s, 6s);
                        break;
                    }
                    case Rixxa::Events::PropellantBlast:
                    {
                        SayVoice(Rixxa::Voice::PropellantBlast);
                        if (Unit* victim = SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
                        {
                            Position pos = victim->GetPosition();
                            AreaTrigger::CreateAreaTrigger({ AreaTriggerTelegraphs::PropellantJet, true }, pos, 7500,
                                me, nullptr, { 0, 0 }, sSpellMgr->GetSpellInfo(Rixxa::Spells::PropellantJet, me->GetMap()->GetDifficultyID()));
                            DoCast(victim, Rixxa::Spells::PropellantBlast);

                            // Four pulses, one per second: everything within 12y of the marked
                            // spot is blasted and knocked away from it.
                            for (uint8 i = 0; i < 4; ++i)
                                scheduler.Schedule(Milliseconds(3000 + i * 1000), [this, pos](TaskContext)
                                {
                                    for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                                        if (Unit* u = ref->GetVictim())
                                            if (u->GetExactDist2d(pos) <= 12.0f)
                                            {
                                                Combat::PercentHit(me, u, Rixxa::Spells::PropellantHit, 0.15f, SPELL_SCHOOL_MASK_FIRE);
                                                u->KnockbackFrom(pos, 10.0f, 3.0f);
                                            }
                                });
                        }
                        events.ScheduleEvent(Rixxa::Events::PropellantBlast, 20s, 30s);
                        break;
                    }
                    case Rixxa::Events::GushingCatalyst:
                    {
                        // Heroic+: a geyser line starts at a random arena-edge point and marches
                        // nine fire patches back across the room toward the center.
                        DoCastSelf(Rixxa::Spells::GushingCatalyst);
                        float angle = frand(0.0f, 6.28f);
                        Position from = Offset(me->GetHomePosition(), angle, 22.0f);
                        for (uint8 i = 0; i < 9; ++i)
                            LayFirePatch(Offset(from, angle + static_cast<float>(M_PI), float(i) * 5.0f));
                        events.ScheduleEvent(Rixxa::Events::GushingCatalyst, 30s, 45s);
                        break;
                    }
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            // Ground fire upkeep: burn anyone standing in a live patch.
            _fireTick += diff;
            if (_fireTick >= FireTickMs)
            {
                _fireTick = 0;
                uint32 now = GameTime::GetGameTimeMS();
                _fires.remove_if([now](GroundFire const& fire) { return now >= fire.Until; });
                for (GroundFire const& fire : _fires)
                    for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                        if (Unit* victim = ref->GetVictim())
                            if (victim->IsAlive() && victim->GetExactDist2d(fire.Pos) <= FireRadius)
                                Combat::PercentHit(me, victim, Rixxa::Spells::CatalystFire, 0.04f, SPELL_SCHOOL_MASK_FIRE);
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        std::list<GroundFire> _fires;
        uint32 _fireTick = 0;
        uint32 _lastVoiceCall = 0;
        uint32 _introCheck = 0;
        bool _introSaid = false;
    };
}

void AddSC_custom_boss_rixxa_fluxflame()
{
    using namespace Scripts::Zandalar::TheMotherlode;

    RegisterCreatureAI(boss_rixxa_fluxflame);
}
