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
 * Coin-Operated Crowd Pummeler (entry 129214) - MOTHERLODE boss 1/4.
 *
 * The machine sits switched off (Out of Order stun) until pulled. The crowd keeps
 * tossing coins onto the arena; Coin Magnet vacuums every pile into a Paid to Win
 * stack on the boss. Footbombs roll out periodically - a player who touches one
 * punts it into the Pummeler (strips Paid to Win stacks, applies Blazing Azerite);
 * ignored bombs detonate in place.
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
#include "TemporarySummon.h"
#include "ThreatManager.h"

#include "Custom_Motherlode_Defines.h"

namespace Scripts::Zandalar::TheMotherlode
{
    namespace
    {
        constexpr float FootbombKickRange = 2.5f;
        constexpr float FootbombBlastRadius = 8.0f;
        constexpr float CoinImpactRadius = 3.0f;
        constexpr uint8 MaxCoinPiles = 12;
        constexpr uint32 FootbombFuseMs = 12000;
    }

    struct boss_coin_operated_crowd_pummeler : public BossAI
    {
        boss_coin_operated_crowd_pummeler(Creature* creature)
            : BossAI(creature, DataTypes::BOSS_COIN_OPERATED_CROWD_PUMMELER) { }

        void InitializeAI() override
        {
            BossAI::InitializeAI();
            SetOutOfOrder(true);
        }

        void Reset() override
        {
            _Reset();
            _footbombFuse.clear();
            _footbombCheck = 0;
            summons.DespawnEntry(Creatures::CoinPile);
            me->RemoveAurasDueToSpell(Pummeler::Spells::PaidToWin);
        }

        void JustReachedHome() override
        {
            BossAI::JustReachedHome();
            SetOutOfOrder(true);
        }

        void JustEngagedWith(Unit* who) override
        {
            SetOutOfOrder(false);
            BossAI::JustEngagedWith(who);
            Talk(Pummeler::Voice::Aggro);

            events.ScheduleEvent(Pummeler::Events::CoinMagnet, 20s, 35s);
            events.ScheduleEvent(Pummeler::Events::StaticPulse, 9s, 20s);
            events.ScheduleEvent(Pummeler::Events::ShockingClaw, 5s, 14s);
            events.ScheduleEvent(Pummeler::Events::Footbomb, 12s, 28s);
            events.ScheduleEvent(Pummeler::Events::CoinToss, 6s);
        }

        void JustSummoned(Creature* summon) override
        {
            BossAI::JustSummoned(summon);

            if (summon->GetEntry() == Creatures::Footbomb)
                _footbombFuse[summon->GetGUID()] = FootbombFuseMs;
            summon->SetReactState(REACT_PASSIVE);
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            BossAI::SummonedCreatureDespawn(summon);
            _footbombFuse.erase(summon->GetGUID());
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            Talk(Pummeler::Voice::Death);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                SayVoice(Pummeler::Voice::Slay);
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

        void SetOutOfOrder(bool offline)
        {
            if (offline)
            {
                me->AddAura(Pummeler::Spells::OutOfOrder, me);
                me->SetStandState(UNIT_STAND_STATE_SIT);
            }
            else
            {
                me->RemoveAurasDueToSpell(Pummeler::Spells::OutOfOrder);
                me->SetStandState(UNIT_STAND_STATE_STAND);
            }
        }

        void ModifyPaidToWin(int32 stacks)
        {
            if (Aura* aura = me->GetAura(Pummeler::Spells::PaidToWin))
            {
                if (stacks < 0 && aura->GetStackAmount() <= uint8(-stacks))
                    aura->Remove();
                else
                    aura->ModStackAmount(stacks);
            }
            else if (stacks > 0)
            {
                if (Aura* aura = me->AddAura(Pummeler::Spells::PaidToWin, me))
                {
                    aura->SetStackAmount(uint8(stacks));
                    aura->SetMaxDuration(-1);
                    aura->SetDuration(-1);
                }
            }
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
                    case Pummeler::Events::CoinMagnet:
                    {
                        SayVoice(Pummeler::Voice::CoinMagnet);
                        DoCastSelf(Pummeler::Spells::CoinMagnet, true);
                        uint32 absorbed = 0;
                        for (ObjectGuid guid : summons)
                            if (Creature* pile = ObjectAccessor::GetCreature(*me, guid))
                                if (pile->GetEntry() == Creatures::CoinPile)
                                {
                                    pile->DespawnOrUnsummon(1s);
                                    ++absorbed;
                                }
                        if (absorbed)
                            ModifyPaidToWin(int32(absorbed));
                        events.ScheduleEvent(Pummeler::Events::CoinMagnet, 20s, 35s);
                        break;
                    }
                    case Pummeler::Events::StaticPulse:
                    {
                        SayVoice(Pummeler::Voice::StaticPulse);
                        DoCastSelf(Pummeler::Spells::StaticPulse, true);
                        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                            if (Unit* victim = ref->GetVictim())
                            {
                                Combat::PercentHit(me, victim, Pummeler::Spells::StaticPulse, 0.15f, SPELL_SCHOOL_MASK_NATURE);
                                victim->KnockbackFrom(me->GetPosition(), 14.0f, 5.0f);
                            }
                        events.ScheduleEvent(Pummeler::Events::StaticPulse, 9s, 20s);
                        break;
                    }
                    case Pummeler::Events::ShockingClaw:
                    {
                        SayVoice(Pummeler::Voice::ShockingClaw);
                        if (Unit* tank = SelectTarget(SelectTargetMethod::MaxThreat, 0, 50.0f, true))
                        {
                            Position target = tank->GetPosition();
                            AreaTrigger::CreateAreaTrigger({ AreaTriggerTelegraphs::ShockingClaw, true }, target, 4000,
                                me, nullptr, { 0, 0 }, sSpellMgr->GetSpellInfo(Pummeler::Spells::ShockingClawHit, me->GetMap()->GetDifficultyID()));
                            DoCast(tank, Pummeler::Spells::ShockingClaw);
                            scheduler.Schedule(4s, [this, target](TaskContext)
                            {
                                for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                                    if (Unit* victim = ref->GetVictim())
                                        if (victim->GetExactDist2d(target) <= 6.0f)
                                            Combat::PercentHit(me, victim, Pummeler::Spells::ShockingClawHit, 0.25f, SPELL_SCHOOL_MASK_NATURE);
                            });
                        }
                        events.ScheduleEvent(Pummeler::Events::ShockingClaw, 5s, 14s);
                        break;
                    }
                    case Pummeler::Events::Footbomb:
                    {
                        SayVoice(Pummeler::Voice::FootbombLauncher);
                        uint8 count = me->GetMap()->IsHeroicOrHigher() ? 3 : 2;
                        for (uint8 i = 0; i < count; ++i)
                        {
                            Position pos = me->GetRandomPoint(me->GetHomePosition(), frand(8.0f, 16.0f));
                            me->SummonCreature(Creatures::Footbomb, pos, TEMPSUMMON_TIMED_DESPAWN, 15s);
                        }
                        events.ScheduleEvent(Pummeler::Events::Footbomb, 12s, 28s);
                        break;
                    }
                    case Pummeler::Events::CoinToss:
                    {
                        for (uint8 i = 0; i < 3; ++i)
                        {
                            Position pos = me->GetRandomPoint(me->GetHomePosition(), frand(4.0f, 18.0f));
                            AreaTrigger::CreateAreaTrigger({ AreaTriggerTelegraphs::CoinImpact, true }, pos, 1500,
                                me, nullptr, { 0, 0 }, sSpellMgr->GetSpellInfo(Pummeler::Spells::CoinToss, me->GetMap()->GetDifficultyID()));
                            scheduler.Schedule(1500ms, [this, pos](TaskContext)
                            {
                                for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                                    if (Unit* victim = ref->GetVictim())
                                        if (victim->GetExactDist2d(pos) <= CoinImpactRadius)
                                            Combat::PercentHit(me, victim, Pummeler::Spells::CoinToss, 0.10f, SPELL_SCHOOL_MASK_NATURE);

                                // Coin piles left behind get vacuumed by Coin Magnet later (cap 12 out).
                                uint32 piles = 0;
                                for (ObjectGuid guid : summons)
                                    if (Creature* pile = ObjectAccessor::GetCreature(*me, guid))
                                        if (pile->GetEntry() == Creatures::CoinPile)
                                            ++piles;
                                if (piles < MaxCoinPiles)
                                    if (Creature* pile = me->SummonCreature(Creatures::CoinPile, pos, TEMPSUMMON_TIMED_DESPAWN, 90s))
                                        pile->SetReactState(REACT_PASSIVE);
                            });
                        }
                        events.ScheduleEvent(Pummeler::Events::CoinToss, 8s);
                        break;
                    }
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            // Footbomb handling: a player standing on a bomb punts it into the Pummeler;
            // an ignored bomb detonates where it sits once the fuse burns down.
            _footbombCheck += diff;
            if (_footbombCheck >= 200)
            {
                uint32 step = _footbombCheck;
                _footbombCheck = 0;

                for (auto it = _footbombFuse.begin(); it != _footbombFuse.end();)
                {
                    Creature* bomb = ObjectAccessor::GetCreature(*me, it->first);
                    if (!bomb || !bomb->IsAlive())
                    {
                        it = _footbombFuse.erase(it);
                        continue;
                    }

                    Unit* kicker = nullptr;
                    for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                        if (Unit* victim = ref->GetVictim())
                            if (victim->IsAlive() && victim->GetExactDist2d(bomb) <= FootbombKickRange)
                            {
                                kicker = victim;
                                break;
                            }

                    if (kicker)
                    {
                        bomb->GetMotionMaster()->MoveJump(EVENT_JUMP, me->GetPosition(), 25.0f, {}, 8.0f);
                        bomb->DespawnOrUnsummon(1500ms);
                        scheduler.Schedule(1200ms, [this](TaskContext)
                        {
                            ModifyPaidToWin(-5);
                            if (Aura* blazing = me->GetAura(Pummeler::Spells::BlazingAzerite))
                                blazing->ModStackAmount(1);
                            else
                                me->AddAura(Pummeler::Spells::BlazingAzerite, me);
                        });
                        it = _footbombFuse.erase(it);
                        continue;
                    }

                    if (it->second <= step)
                    {
                        Position pos = bomb->GetPosition();
                        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                            if (Unit* victim = ref->GetVictim())
                                if (victim->GetExactDist2d(pos) <= FootbombBlastRadius)
                                {
                                    Combat::PercentHit(me, victim, Pummeler::Spells::FootbombBlast, 0.15f, SPELL_SCHOOL_MASK_FIRE);
                                    me->AddAura(Pummeler::Spells::BlazingAzerite, victim);
                                }
                        bomb->DespawnOrUnsummon(1ms);
                        it = _footbombFuse.erase(it);
                        continue;
                    }

                    it->second -= step;
                    ++it;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        // guid -> remaining fuse (ms) before the bomb detonates in place
        std::unordered_map<ObjectGuid, uint32> _footbombFuse;
        uint32 _footbombCheck = 0;
        uint32 _lastVoiceCall = 0;
    };
}

void AddSC_custom_boss_crowd_pummeler()
{
    using namespace Scripts::Zandalar::TheMotherlode;

    RegisterCreatureAI(boss_coin_operated_crowd_pummeler);
}
