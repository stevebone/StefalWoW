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
 * Mogul Razdunk (entry 129232) - MOTHERLODE boss 4/4.
 *
 * Stage One (Big Guns): Alpha Cannon on the tank, a six-second Gatling Gun
 * sweep that rotates the boss while burning a frontal cone, Homing Missiles
 * that crawl after a player and detonate on contact, and B.O.O.M.B.A. drones
 * on Heroic+. At 50% health Stage Two (Drill) begins: Drill Smash leaps onto a
 * player, Big Red Rockets rain telegraphed impacts, and Skyscorchers drop in
 * to snipe players with Buster Shot.
 *
 * Ported from the Urlatek EncounterBatch implementation, rewritten on BossAI.
 */

#include "AreaTrigger.h"
#include "GameTime.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "ThreatManager.h"

#include "Custom_Motherlode_Defines.h"

namespace Scripts::Zandalar::TheMotherlode
{
    namespace
    {
        constexpr float MissileSpeed = 5.0f;         // yards per second
        constexpr float MissileContactRange = 2.5f;
        constexpr float MissileBlastRadius = 8.0f;
        constexpr uint32 MissileLifetimeMs = 10000;
        constexpr uint32 GatlingDurationMs = 6000;
        constexpr float GatlingSweepStep = 0.35f;    // radians per 500ms tick
        constexpr float GatlingHalfAngle = 0.35f;
        constexpr float GatlingRange = 30.0f;
        constexpr float ArenaRadius = 22.0f;

        struct HomingMissile
        {
            ObjectGuid Target;
            ObjectGuid Visual;   // moving areatrigger acting as the missile body
            Position Pos;
            uint32 Until;        // GameTime ms
        };
    }

    struct boss_mogul_razdunk : public BossAI
    {
        boss_mogul_razdunk(Creature* creature) : BossAI(creature, DataTypes::BOSS_MOGUL_RAZDUNK) { }

        void Reset() override
        {
            _Reset();
            ClearMissiles();
            _stageTwo = false;
            _gatlingRemaining = 0;
            _gatlingNext = 0;
            _missileTick = 0;
            _addTick = 0;
            _addNextAct.clear();
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            Talk(Razdunk::Voice::Aggro);

            events.ScheduleEvent(Razdunk::Events::AlphaCannon, 6s, 15s);
            events.ScheduleEvent(Razdunk::Events::GatlingGun, 12s, 30s);
            events.ScheduleEvent(Razdunk::Events::HomingMissile, 20s, 30s);
            if (me->GetMap()->IsHeroicOrHigher())
                events.ScheduleEvent(Razdunk::Events::BoombaDrone, 25s, 40s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
        {
            if (_stageTwo || !me->HealthBelowPct(50))
                return;

            // Stage Two: swap the Big Guns rotation for the Drill rotation.
            _stageTwo = true;
            Talk(Razdunk::Voice::Insurance);
            _gatlingRemaining = 0;
            me->InterruptNonMeleeSpells(false);
            events.CancelEvent(Razdunk::Events::AlphaCannon);
            events.CancelEvent(Razdunk::Events::GatlingGun);
            events.CancelEvent(Razdunk::Events::HomingMissile);
            events.CancelEvent(Razdunk::Events::BoombaDrone);
            events.ScheduleEvent(Razdunk::Events::DrillSmash, 4s, 14s);
            events.ScheduleEvent(Razdunk::Events::BigRedRocket, 9s, 20s);
            events.ScheduleEvent(Razdunk::Events::Skyscorchers, 15s, 35s);
        }

        void JustSummoned(Creature* summon) override
        {
            BossAI::JustSummoned(summon);

            switch (summon->GetEntry())
            {
                case Creatures::BoombaDrone:
                    // Drones are visual-only bombers; players cannot attack them.
                    summon->SetReactState(REACT_PASSIVE);
                    summon->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                    _addNextAct[summon->GetGUID()] = 2000;
                    break;
                case Creatures::Skyscorcher:
                    summon->SetReactState(REACT_AGGRESSIVE);
                    _addNextAct[summon->GetGUID()] = 3000;
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
                        summon->GetMotionMaster()->MoveChase(target);
                    break;
                default:
                    break;
            }
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            BossAI::SummonedCreatureDespawn(summon);
            _addNextAct.erase(summon->GetGUID());
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            Talk(Razdunk::Voice::Death);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                SayVoice(roll_chance(50) ? Razdunk::Voice::Slay : Razdunk::Voice::YoullPay);
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

        void ClearMissiles()
        {
            for (HomingMissile const& missile : _missiles)
                if (AreaTrigger* at = ObjectAccessor::GetAreaTrigger(*me, missile.Visual))
                    at->Remove();
            _missiles.clear();
        }

        // Drops a warning circle under `victim` and resolves it after `delayMs`.
        void WarnThenBlast(uint32 telegraphId, uint32 spellId, Unit* victim, uint32 delayMs, float radius, float fraction)
        {
            Position pos = victim->GetPosition();
            AreaTrigger::CreateAreaTrigger({ telegraphId, true }, pos, delayMs,
                me, nullptr, { 0, 0 }, sSpellMgr->GetSpellInfo(spellId, me->GetMap()->GetDifficultyID()));
            scheduler.Schedule(Milliseconds(delayMs), [this, pos, spellId, radius, fraction](TaskContext)
            {
                for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                    if (Unit* target = ref->GetVictim())
                        if (target->GetExactDist2d(pos) <= radius)
                            Combat::PercentHit(me, target, spellId, fraction, SPELL_SCHOOL_MASK_FIRE);
            });
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
                    case Razdunk::Events::AlphaCannon:
                        DoCastVictim(Razdunk::Spells::AlphaCannon);
                        if (Unit* tank = SelectTarget(SelectTargetMethod::MaxThreat, 0, 50.0f, true))
                            Combat::PercentHit(me, tank, Razdunk::Spells::AlphaCannonHit, 0.45f, SPELL_SCHOOL_MASK_FIRE);
                        events.ScheduleEvent(Razdunk::Events::AlphaCannon, 6s, 15s);
                        break;
                    case Razdunk::Events::GatlingGun:
                        SayVoice(uint8(urand(Razdunk::Voice::GatlingGunMin, Razdunk::Voice::GatlingGunMax)));
                        DoCastSelf(Razdunk::Spells::GatlingGun);
                        _gatlingRemaining = GatlingDurationMs;
                        _gatlingNext = 0;
                        _gatlingAngle = me->GetOrientation();
                        events.ScheduleEvent(Razdunk::Events::GatlingGun, 12s, 30s);
                        break;
                    case Razdunk::Events::HomingMissile:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
                        {
                            Position pos = me->GetPosition();
                            ObjectGuid visual;
                            if (AreaTrigger* at = AreaTrigger::CreateAreaTrigger({ AreaTriggerTelegraphs::MissileBlast, false }, pos,
                                    MissileLifetimeMs + 500, me, nullptr, { 0, 0 },
                                    sSpellMgr->GetSpellInfo(Razdunk::Spells::MissileBlast, me->GetMap()->GetDifficultyID())))
                                visual = at->GetGUID();
                            _missiles.push_back({ target->GetGUID(), visual, pos, GameTime::GetGameTimeMS() + MissileLifetimeMs });
                        }
                        events.ScheduleEvent(Razdunk::Events::HomingMissile, 20s, 30s);
                        break;
                    case Razdunk::Events::BoombaDrone:
                        SayVoice(Razdunk::Voice::BoombaDrone);
                        me->SummonCreature(Creatures::BoombaDrone,
                            me->GetRandomPoint(me->GetHomePosition(), 12.0f), TEMPSUMMON_TIMED_DESPAWN, 20s);
                        events.ScheduleEvent(Razdunk::Events::BoombaDrone, 25s, 40s);
                        break;
                    case Razdunk::Events::DrillSmash:
                    {
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
                        {
                            static constexpr uint8 DrillCallouts[] = { Razdunk::Voice::DrillSmashMin,
                                Razdunk::Voice::DrillSmashAlt1, Razdunk::Voice::DrillSmashAlt2 };
                            SayVoice(DrillCallouts[urand(0, 2)]);
                            Talk(Razdunk::Voice::DrillSmashEmote, target);
                            Position pos = target->GetPosition();
                            AreaTrigger::CreateAreaTrigger({ AreaTriggerTelegraphs::DrillSmash, true }, pos, 1800,
                                me, nullptr, { 0, 0 }, sSpellMgr->GetSpellInfo(Razdunk::Spells::DrillSmashHit, me->GetMap()->GetDifficultyID()));
                            scheduler.Schedule(1800ms, [this, pos](TaskContext)
                            {
                                me->NearTeleportTo(pos);
                                // Inside the drill: full hit. The rim takes a glancing blow.
                                for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                                    if (Unit* victim = ref->GetVictim())
                                    {
                                        float dist = victim->GetExactDist2d(pos);
                                        if (dist <= 8.0f)
                                            Combat::PercentHit(me, victim, Razdunk::Spells::DrillSmashHit, 0.40f, SPELL_SCHOOL_MASK_NORMAL);
                                        else
                                            Combat::PercentHit(me, victim, Razdunk::Spells::DrillSmash, 0.12f, SPELL_SCHOOL_MASK_NORMAL);
                                    }
                            });
                        }
                        events.ScheduleEvent(Razdunk::Events::DrillSmash, 4s, 14s);
                        break;
                    }
                    case Razdunk::Events::BigRedRocket:
                    {
                        SayVoice(Razdunk::Voice::BigRedRocket);
                        DoCastSelf(Razdunk::Spells::BigRedRocket);
                        uint8 count = me->GetMap()->IsHeroicOrHigher() ? 4 : 3;
                        for (uint8 i = 0; i < count; ++i)
                        {
                            Position pos = me->GetRandomPoint(me->GetHomePosition(), frand(4.0f, ArenaRadius));
                            AreaTrigger::CreateAreaTrigger({ AreaTriggerTelegraphs::BigRedRocket, true }, pos, 3000,
                                me, nullptr, { 0, 0 }, sSpellMgr->GetSpellInfo(Razdunk::Spells::BigRedRocket, me->GetMap()->GetDifficultyID()));
                            scheduler.Schedule(3s, [this, pos](TaskContext)
                            {
                                for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                                    if (Unit* victim = ref->GetVictim())
                                        if (victim->GetExactDist2d(pos) <= 6.0f)
                                            Combat::PercentHit(me, victim, Razdunk::Spells::BigRedRocket, 0.25f, SPELL_SCHOOL_MASK_FIRE);
                            });
                        }
                        events.ScheduleEvent(Razdunk::Events::BigRedRocket, 9s, 20s);
                        break;
                    }
                    case Razdunk::Events::Skyscorchers:
                        for (uint8 i = 0; i < 2; ++i)
                            me->SummonCreature(Creatures::Skyscorcher,
                                me->GetRandomPoint(me->GetHomePosition(), 15.0f), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5s);
                        events.ScheduleEvent(Razdunk::Events::Skyscorchers, 15s, 35s);
                        break;
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            // Gatling Gun: the boss slowly spins while a frontal cone burns.
            if (_gatlingRemaining)
            {
                if (_gatlingRemaining <= diff)
                    _gatlingRemaining = 0;
                else
                {
                    _gatlingRemaining -= diff;
                    _gatlingNext += diff;
                    if (_gatlingNext >= 500)
                    {
                        _gatlingNext = 0;
                        _gatlingAngle += GatlingSweepStep;
                        me->SetFacingTo(_gatlingAngle);

                        Position end;
                        end.m_positionX = me->GetPositionX() + std::cos(_gatlingAngle) * GatlingRange;
                        end.m_positionY = me->GetPositionY() + std::sin(_gatlingAngle) * GatlingRange;
                        end.m_positionZ = me->GetPositionZ();
                        AreaTrigger::CreateAreaTrigger({ AreaTriggerTelegraphs::GatlingGun, false }, end, 600,
                            me, nullptr, { 0, 0 }, sSpellMgr->GetSpellInfo(Razdunk::Spells::GatlingGun, me->GetMap()->GetDifficultyID()));

                        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                            if (Unit* victim = ref->GetVictim())
                            {
                                float delta = Position::NormalizeOrientation(me->GetAbsoluteAngle(victim) - _gatlingAngle);
                                if (std::abs(delta) <= GatlingHalfAngle && me->GetExactDist2d(victim) <= GatlingRange)
                                    Combat::PercentHit(me, victim, Razdunk::Spells::GatlingGun, 0.15f, SPELL_SCHOOL_MASK_FIRE);
                            }
                    }
                }
            }

            // Homing Missiles: crawl toward their target, detonate on contact or timeout.
            _missileTick += diff;
            if (_missileTick >= 200)
            {
                _missileTick = 0;
                uint32 now = GameTime::GetGameTimeMS();
                _missiles.erase(std::remove_if(_missiles.begin(), _missiles.end(), [this, now](HomingMissile& missile)
                {
                    Unit* target = ObjectAccessor::GetUnit(*me, missile.Target);
                    AreaTrigger* at = ObjectAccessor::GetAreaTrigger(*me, missile.Visual);
                    bool hit = target && target->IsAlive() && target->GetExactDist2d(missile.Pos) <= MissileContactRange;
                    if (hit || now >= missile.Until || !target || !target->IsAlive())
                    {
                        if (at)
                            at->Remove();
                        for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                            if (Unit* victim = ref->GetVictim())
                                if (victim->GetExactDist2d(missile.Pos) <= MissileBlastRadius)
                                {
                                    Combat::PercentHit(me, victim, Razdunk::Spells::MissileBlast, 0.25f, SPELL_SCHOOL_MASK_FIRE);
                                    victim->KnockbackFrom(missile.Pos, 8.0f, 4.0f);
                                }
                        return true;
                    }

                    float angle = missile.Pos.GetAbsoluteAngle(target);
                    float step = std::min(MissileSpeed / 5.0f, missile.Pos.GetExactDist2d(target)); // 200ms tick
                    Position next;
                    next.m_positionX = missile.Pos.GetPositionX() + std::cos(angle) * step;
                    next.m_positionY = missile.Pos.GetPositionY() + std::sin(angle) * step;
                    next.m_positionZ = missile.Pos.GetPositionZ();
                    if (at)
                        at->InitSplines({ { missile.Pos.GetPositionX(), missile.Pos.GetPositionY(), missile.Pos.GetPositionZ() },
                                          { next.GetPositionX(), next.GetPositionY(), next.GetPositionZ() } }, MissileSpeed, false);
                    missile.Pos = next;
                    return false;
                }), _missiles.end());
            }

            // Add upkeep: drones drop bomb warnings, skyscorchers snipe with Buster Shot.
            _addTick += diff;
            if (_addTick >= 250)
            {
                uint32 step = _addTick;
                _addTick = 0;

                for (auto& pair : _addNextAct)
                {
                    Creature* add = ObjectAccessor::GetCreature(*me, pair.first);
                    if (!add || !add->IsAlive())
                        continue;

                    pair.second += step;
                    uint32 period = add->GetEntry() == Creatures::BoombaDrone ? 2000 : 3000;
                    if (pair.second < period)
                        continue;
                    pair.second = 0;

                    if (add->GetEntry() == Creatures::BoombaDrone)
                    {
                        for (uint8 i = 0; i < 2; ++i)
                            if (Unit* victim = SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
                                WarnThenBlast(AreaTriggerTelegraphs::BoombaMissile, Razdunk::Spells::Boomba, victim, 1500, 6.0f, 0.20f);
                    }
                    else if (Unit* victim = SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
                        Combat::PercentHit(add, victim, Razdunk::Spells::BusterShot, 0.10f, SPELL_SCHOOL_MASK_FIRE);
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        std::vector<HomingMissile> _missiles;
        std::unordered_map<ObjectGuid, uint32> _addNextAct;
        bool _stageTwo = false;
        uint32 _gatlingRemaining = 0;
        uint32 _gatlingNext = 0;
        float _gatlingAngle = 0.0f;
        uint32 _missileTick = 0;
        uint32 _addTick = 0;
        uint32 _lastVoiceCall = 0;
    };
}

void AddSC_custom_boss_mogul_razdunk()
{
    using namespace Scripts::Zandalar::TheMotherlode;

    RegisterCreatureAI(boss_mogul_razdunk);
}
