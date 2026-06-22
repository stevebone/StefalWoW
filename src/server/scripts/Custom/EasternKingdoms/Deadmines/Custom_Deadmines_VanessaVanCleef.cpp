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

#include "CreatureAIImpl.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "MotionMaster.h"
#include "MoveSplineInit.h"
#include "ObjectMgr.h"
#include "PassiveAI.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "Player.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Vehicle.h"
#include "GridNotifiers.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "Creature.h"
#include "TemporarySummon.h"
#include "GameObject.h"
#include "G3D/Vector3.h"
#include <list>

#include "Custom_Instance_Deadmines.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    // 49552 - Rope Anchor
    struct npc_rope_ship : public ScriptedAI
    {
        npc_rope_ship(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            if (TempSummon* summon = me->ToTempSummon())
                if (Creature* summoner = summon->GetSummonerCreatureBase())
                    DoCast(summoner, Spells::VanessaVanCleef::RopeBeam);
        }
    };

    // 49481 - Collapsing Icicle
    struct npc_icycle_dm : public PassiveAI
    {
        npc_icycle_dm(Creature* creature) : PassiveAI(creature) { }

        void Reset() override
        {
            _events.ScheduleEvent(Events::VanessaVanCleef::IcicleAOE, 10s);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::VanessaVanCleef::IcicleAOE:
                        DoCastSelf(Spells::VanessaVanCleef::IcicleVisual);
                        if (Player* player = me->SelectNearestPlayer(20.0f))
                            DoCast(player, Spells::VanessaVanCleef::IcicleDamage);
                        _events.ScheduleEvent(Events::VanessaVanCleef::IcicleAOE, 10s);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
    };

    // 49564 - A Note From Vanessa
    struct npc_note : public ScriptedAI
    {
        npc_note(Creature* creature) : ScriptedAI(creature)
        {
            me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        }

        void Reset() override
        {
            _events.Reset();
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::VanessaVanCleef::NoteSummon:
                        me->SummonCreature(Creatures::VanessaIntro, Positions::VanessaSpawn, TEMPSUMMON_MANUAL_DESPAWN);
                        me->DespawnOrUnsummon();
                        break;
                    default:
                        break;
                }
            }
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
        {
            if (gossipListId == 0)
            {
                CloseGossipMenuFor(player);
                _events.ScheduleEvent(Events::VanessaVanCleef::NoteSummon, 5s);
            }
            return true;
        }

    private:
        EventMap _events;
    };

    // 49454 - Vanessa's Trap Bunny
    struct npc_magma_pull : public ScriptedAI
    {
        npc_magma_pull(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            Talk(Texts::VanessaVanCleef::VanessaTrapBunnyWarning);
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == 1 && value == 1)
                _triggered = true;
        }

        uint32 GetData(uint32 id) const override
        {
            if (id == 1)
                return _triggered ? 1 : 0;
            return 0;
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type != POINT_MOTION_TYPE || pointId != 0)
                return;

            _step++;
            if (_step < 5)
                _events.ScheduleEvent(Events::VanessaVanCleef::TrapDescent, 0s);
            else
                me->DespawnOrUnsummon();
        }

        void UpdateAI(uint32 diff) override
        {
            if (_triggered)
            {
                _triggered = false;
                _events.ScheduleEvent(Events::VanessaVanCleef::TrapPullPlayers, 0s);
            }

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::VanessaVanCleef::TrapPullPlayers:
                    {
                        std::list<Player*> players;
                        me->GetPlayerListInGrid(players, 50.0f);
                        for (Player* player : players)
                            if (player->IsAlive())
                                player->CastSpell(me, Spells::VanessaVanCleef::GenericRideVehicle);

                        me->NearTeleportTo(Positions::MagmaPullPlayers);
                        if (Creature* ropeAnchor = me->SummonCreature(Creatures::RopeAnchor, Positions::MagmaRopeCeiling, TEMPSUMMON_TIMED_DESPAWN, 30s))
                            ropeAnchor->SetDisableGravity(true);

                        me->SummonCreature(Creatures::VanessaNightmare, Positions::VanessaNightmare1, TEMPSUMMON_MANUAL_DESPAWN);
                        me->SummonCreature(Creatures::NightmareGlubtok, Positions::GlubtokNightmare1, TEMPSUMMON_MANUAL_DESPAWN);

                        _step = 0;
                        _events.ScheduleEvent(Events::VanessaVanCleef::TrapDescent, 1s);
                        break;
                    }
                    case Events::VanessaVanCleef::TrapDescent:
                    {
                        if (_step == 0)
                        {
                            Talk(Texts::VanessaVanCleef::VanessaTrapBunnyValves);

                            std::list<Creature*> valves;
                            me->GetCreatureListWithEntryInGrid(valves, Creatures::SteamValve, 30.f);
                            for (Creature* valve : valves)
                                valve->CastSpell(valve, Spells::VanessaVanCleef::GreenMarker);
                        }

                        float targetZ = Positions::MagmaPullPlayers.GetPositionZ() - ((_step + 1) * 2.0f);
                        Position dest(Positions::MagmaPullPlayers.GetPositionX(), Positions::MagmaPullPlayers.GetPositionY(), targetZ);
                        me->GetMotionMaster()->MovePoint(0, dest, false, {}, 0.2f);
                        break;
                    }
                    default:
                        break;
                }
            }
        }

    private:
        bool _triggered = false;
        uint8 _step = 0;
        EventMap _events;
    };

    // 49550 - Rope
    struct npc_rope_away : public ScriptedAI
    {
        npc_rope_away(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
        }

        void PassengerBoarded(Unit* /*passenger*/, int8 /*seatId*/, bool apply) override
        {
            if (apply)
            {
                me->SetSpeed(MOVE_WALK, 5.0f);
                me->SetWalk(false);
                me->GetMotionMaster()->MovePoint(0, -64.75f, -840.0f, 17.56f);
            }
        }

        void MovementInform(uint32 type, uint32 /*pointId*/) override
        {
            if (type != EFFECT_MOTION_TYPE)
                return;

            me->GetVehicleKit()->RemoveAllPassengers();
            me->DespawnOrUnsummon(1s);
        }
    };

    // 49520 - Lightning Platter
    struct npc_lightning_orbs : public ScriptedAI
    {
        npc_lightning_orbs(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            for (uint8 i = 0; i < 8; ++i)
                me->SummonCreature(Creatures::SparkPassenger, me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case 1:
                        me->SetOrientation(me->GetOrientation() + 0.05f);
                        me->SetFacingTo(me->GetOrientation());
                        _events.ScheduleEvent(1, 100ms);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
    };

    // 49505 - Defias Shadowguard
    struct npc_defias_shadowguard : public ScriptedAI
    {
        npc_defias_shadowguard(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _evasionCast = false;
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 3s);
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 8s);
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd3, 13s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!_evasionCast && me->HealthBelowPct(35))
            {
                _evasionCast = true;
                DoCastSelf(Spells::VanessaVanCleef::Evasion);
            }
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
                    case Events::VanessaVanCleef::SummonAdd1:
                        DoCastVictim(Spells::VanessaVanCleef::SinisterStrike);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 6s);
                        break;
                    case Events::VanessaVanCleef::SummonAdd2:
                        DoCastSelf(Spells::VanessaVanCleef::WhirlingBlades);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 12s);
                        break;
                    case Events::VanessaVanCleef::SummonAdd3:
                        DoCastVictim(Spells::VanessaVanCleef::Shadowstep);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd3, 10s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool _evasionCast = false;
    };

    // 49502 - Defias Enforcer
    struct npc_defias_enforcer : public ScriptedAI
    {
        npc_defias_enforcer(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                DoCast(target, Spells::VanessaVanCleef::Charge);

            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 5s);
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 10s);
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
                    case Events::VanessaVanCleef::SummonAdd1:
                        DoCastSelf(Spells::VanessaVanCleef::Bloodbath);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 12s);
                        break;
                    case Events::VanessaVanCleef::SummonAdd2:
                        DoCastSelf(Spells::VanessaVanCleef::Recklessness);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 15s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
    };

    // 48418 - Defias Envoker
    struct npc_defias_envoker : public ScriptedAI
    {
        npc_defias_envoker(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 3s);
            if (me->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC)
                _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 8s);
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
                    case Events::VanessaVanCleef::SummonAdd1:
                        DoCastVictim(Spells::VanessaVanCleef::HolyFire);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 8s);
                        break;
                    case Events::VanessaVanCleef::SummonAdd2:
                        DoCastSelf(Spells::VanessaVanCleef::Shield);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 20s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
    };

    // 49854 - Defias Blood Wizard
    struct npc_defias_bloodwizard : public ScriptedAI
    {
        npc_defias_bloodwizard(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 3s);
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 6s);
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd3, 10s);
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
                    case Events::VanessaVanCleef::SummonAdd1:
                        DoCastVictim(Spells::VanessaVanCleef::BloodBolt);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 6s);
                        break;
                    case Events::VanessaVanCleef::SummonAdd2:
                        DoCastSelf(Spells::VanessaVanCleef::BloodWash);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 15s);
                        break;
                    case Events::VanessaVanCleef::SummonAdd3:
                        DoCastSelf(Spells::VanessaVanCleef::Ragezone);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd3, 20s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
    };

    // 48445 - Oaf Lackey
    struct npc_oaf_lackey : public ScriptedAI
    {
        npc_oaf_lackey(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _enrageCast = false;
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 4s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!_enrageCast && me->HealthBelowPct(35))
            {
                _enrageCast = true;
                DoCastSelf(Spells::VanessaVanCleef::Enrage);
            }
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
                    case Events::VanessaVanCleef::SummonAdd1:
                        DoCastVictim(Spells::VanessaVanCleef::AxeHead);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 8s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool _enrageCast = false;
    };

    // 48279 - Goblin Overseer
    struct npc_goblin_overseer : public ScriptedAI
    {
        npc_goblin_overseer(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _threateningCast = false;
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 5s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!_threateningCast && me->HealthBelowPct(50))
            {
                _threateningCast = true;
                DoCastSelf(Spells::VanessaVanCleef::Threatening);
            }
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
                    case Events::VanessaVanCleef::SummonAdd1:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                            DoCast(target, Spells::VanessaVanCleef::Motivate);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 10s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool _threateningCast = false;
    };

    // 49532 - Enraged Worgen
    struct npc_enraged_worgen_dm : public ScriptedAI
    {
        npc_enraged_worgen_dm(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (Creature* vanessa = me->FindNearestCreature(Creatures::VanessaNightmare, 200.0f))
                vanessa->AI()->SetData(1, 1);
        }
    };

    // 49539 - James Harrington
    struct npc_james_dm : public ScriptedAI
    {
        npc_james_dm(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (Creature* vanessa = me->FindNearestCreature(Creatures::VanessaNightmare, 200.0f))
                vanessa->AI()->SetData(2, 2);
        }
    };

    // 49429 - Vanessa VanCleef (intro spawn)
    struct npc_vanessa_intro : public ScriptedAI
    {
        npc_vanessa_intro(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            me->AddAura(Spells::VanessaVanCleef::Sitting, me);
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            Talk(Texts::VanessaVanCleef::VanessaIntroWarning);
            _events.ScheduleEvent(Events::VanessaVanCleef::IntroSay1, 3s);
            _events.ScheduleEvent(Events::VanessaVanCleef::IntroJump, 5s);
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type == EFFECT_MOTION_TYPE && pointId == EVENT_JUMP)
                _events.ScheduleEvent(Events::VanessaVanCleef::IntroLaugh, 2s);
            else if (type == POINT_MOTION_TYPE && pointId == 0)
            {
                Talk(Texts::VanessaVanCleef::VanessaSay3);
                _events.ScheduleEvent(Events::VanessaVanCleef::IntroSay4, 8s);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::VanessaVanCleef::IntroSay1:
                        Talk(Texts::VanessaVanCleef::VanessaSay1);
                        break;
                    case Events::VanessaVanCleef::IntroJump:
                    {
                        me->RemoveAura(Spells::VanessaVanCleef::Sitting);
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        if (Creature* bunny = me->GetMap()->GetCreatureBySpawnId(CreatureSpawns::CuldronSpawnBunny))
                        {
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MoveJump(EVENT_JUMP, bunny->GetPosition(), 10.0f, {}, 8.0f);
                        }
                        break;
                    }
                    case Events::VanessaVanCleef::IntroLaugh:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                        _events.ScheduleEvent(Events::VanessaVanCleef::IntroMove, 2s);
                        break;
                    case Events::VanessaVanCleef::IntroMove:
                        me->SetWalk(true);
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MovePoint(0, Positions::VanessaMoveTo);
                        Talk(Texts::VanessaVanCleef::VanessaSay2);
                        break;
                    case Events::VanessaVanCleef::IntroSay4:
                    {
                        if (Creature* bunny = me->GetMap()->GetCreatureBySpawnId(CreatureSpawns::CuldronSpawnBunny))
                            me->SetFacingToObject(bunny);
                        Talk(Texts::VanessaVanCleef::VanessaSay4);
                        _events.ScheduleEvent(Events::VanessaVanCleef::IntroConcoction, 3s);
                        break;
                    }
                    case Events::VanessaVanCleef::IntroConcoction:
                        DoCastAOE(Spells::VanessaVanCleef::NoxiousConcoction);
                        _events.ScheduleEvent(Events::VanessaVanCleef::IntroElixir, 3s);
                        break;
                    case Events::VanessaVanCleef::IntroElixir:
                    {
                        DoCastAOE(Spells::VanessaVanCleef::NightmareElixir);
                        Talk(Texts::VanessaVanCleef::VanessaSay5);
                        _events.ScheduleEvent(Events::VanessaVanCleef::IntroSummonTrap, 0s);
                        _events.ScheduleEvent(Events::VanessaVanCleef::IntroBlackout1, 4s);
                        _events.ScheduleEvent(Events::VanessaVanCleef::IntroBlackout2, 6s);
                        me->DespawnOrUnsummon(10s);
                        break;
                    }
                    case Events::VanessaVanCleef::IntroSummonTrap:
                        me->SummonCreature(Creatures::TrapBunny, me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
                        break;
                    case Events::VanessaVanCleef::IntroBlackout1:
                    case Events::VanessaVanCleef::IntroBlackout2:
                    {
                        Map::PlayerList const& players = me->GetMap()->GetPlayers();
                        for (MapReference const& ref : players)
                        {
                            if (Player* player = ref.GetSource())
                                player->CastSpell(player, Spells::VanessaVanCleef::Blackout, true);
                        }
                        if (eventId == Events::VanessaVanCleef::IntroBlackout2)
                        {
                            if (Creature* trapBunny = me->FindNearestCreature(Creatures::TrapBunny, 50.0f))
                                trapBunny->AI()->SetData(1, 1);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
    };

    // 49671 - Vanessa VanCleef (nightmare)
    struct npc_vanessa_nightmare : public ScriptedAI
    {
        npc_vanessa_nightmare(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _phase = 0;
            _bossKilled = 0;
            _worgenKilled = 0;
            _jamesKilled = false;
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == 4)
            {
                Talk(Texts::VanessaVanCleef::VanessaNightmareWarning);
                _events.ScheduleEvent(Events::VanessaVanCleef::NightmareSay1, 2s);
            }

            if (id == 1 && value == 1)
                _worgenKilled++;

            if (id == 2 && value == 2)
                _jamesKilled = true;

            if (id == 3 && value == 3)
            {
                _bossKilled++;
                _events.ScheduleEvent(Events::VanessaVanCleef::NightmareShift, 3s);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::VanessaVanCleef::NightmareSay1:
                        if (_phase == 0)
                            Talk(Texts::VanessaVanCleef::VanessaNightmareGlubtok1);
                        else if (_phase == 1)
                            Talk(Texts::VanessaVanCleef::VanessaHelix1);
                        else if (_phase == 2)
                            Talk(Texts::VanessaVanCleef::VanessaMechanical1);
                        else if (_phase == 3)
                            Talk(Texts::VanessaVanCleef::VanessaRipsnarl1);
                        _events.ScheduleEvent(Events::VanessaVanCleef::NightmareSay2, 7s);
                        break;
                    case Events::VanessaVanCleef::NightmareSay2:
                        if (_phase == 0)
                            Talk(Texts::VanessaVanCleef::VanessaNightmareGlubtok2);
                        else if (_phase == 1)
                            Talk(Texts::VanessaVanCleef::VanessaHelix2);
                        else if (_phase == 2)
                            Talk(Texts::VanessaVanCleef::VanessaMechanical2);
                        else if (_phase == 3)
                            Talk(Texts::VanessaVanCleef::VanessaRipsnarl2);
                        _events.ScheduleEvent(Events::VanessaVanCleef::NightmareSummon, 2s);
                        break;
                    case Events::VanessaVanCleef::NightmareSummon:
                        if (_phase == 0)
                            me->SummonCreature(Creatures::NightmareGlubtok, Positions::GlubtokNightmare1, TEMPSUMMON_MANUAL_DESPAWN);
                        else if (_phase == 1)
                        {
                            me->SummonCreature(Creatures::NightmareHelix, me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
                            for (uint8 i = 0; i < 4; ++i)
                                me->SummonCreature(Creatures::NightmareSpider, Positions::NightmareSpidersSpawn[i], TEMPSUMMON_MANUAL_DESPAWN);
                        }
                        else if (_phase == 2)
                            me->SummonCreature(Creatures::NightmareFoeReaper, me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
                        else if (_phase == 3)
                        {
                            for (uint8 i = 0; i < 3; ++i)
                                me->SummonCreature(Creatures::EnragedWorgen, Positions::EnragedWorgen1[i], TEMPSUMMON_MANUAL_DESPAWN);
                            for (uint8 i = 0; i < 3; ++i)
                                me->SummonCreature(Creatures::EnragedWorgen, Positions::EnragedWorgen2[i], TEMPSUMMON_MANUAL_DESPAWN);
                            me->SummonCreature(Creatures::JamesHarrington, Positions::FamilySpawn[0], TEMPSUMMON_MANUAL_DESPAWN);
                        }
                        else if (_phase == 4)
                        {
                            Talk(Texts::VanessaVanCleef::VanessaNightmare1);
                            me->SummonCreature(Creatures::VanessaVanCleef, me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
                            me->DespawnOrUnsummon(3s);
                        }
                        break;
                    case Events::VanessaVanCleef::NightmareShift:
                        _phase = _bossKilled;
                        if (_phase < 4)
                            _events.ScheduleEvent(Events::VanessaVanCleef::NightmareSay1, 1s);
                        break;
                    default:
                        break;
                }
            }

            if (_phase == 3 && _worgenKilled >= 6 && _jamesKilled && !_finalTriggered)
            {
                _finalTriggered = true;
                _phase = 4;
                _events.ScheduleEvent(Events::VanessaVanCleef::NightmareSay1, 1s);
            }
        }

    private:
        EventMap _events;
        uint8 _phase = 0;
        uint8 _bossKilled = 0;
        uint8 _worgenKilled = 0;
        bool _jamesKilled = false;
        bool _finalTriggered = false;
    };

    // 49670 - Nightmare Glubtok
    struct npc_glubtok_dm : public ScriptedAI
    {
        npc_glubtok_dm(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::VanessaVanCleef::IcicleAOE, 5s);
            _events.ScheduleEvent(Events::VanessaVanCleef::SpiritStrike, 8s);
        }

        void JustDied(Unit* /*killer*/) override
        {
            DoCastSelf(Spells::VanessaVanCleef::NightmareElixirEffect);
            if (Creature* vanessa = me->FindNearestCreature(Creatures::VanessaNightmare, 200.0f))
                vanessa->AI()->SetData(3, 3);
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
                    case Events::VanessaVanCleef::IcicleAOE:
                        DoCastVictim(Spells::VanessaVanCleef::Icicle);
                        _events.ScheduleEvent(Events::VanessaVanCleef::IcicleAOE, 8s);
                        break;
                    case Events::VanessaVanCleef::SpiritStrike:
                        DoCastVictim(Spells::VanessaVanCleef::SpiritStrike);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SpiritStrike, 10s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
    };

    // 49674 - Nightmare Helix
    struct npc_helix_dm : public ScriptedAI
    {
        npc_helix_dm(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::VanessaVanCleef::Spiders, 5s);
            _events.ScheduleEvent(Events::VanessaVanCleef::SpiritStrike, 8s);
        }

        void JustDied(Unit* /*killer*/) override
        {
            DoCastSelf(Spells::VanessaVanCleef::NightmareElixirEffect);
            if (Creature* vanessa = me->FindNearestCreature(Creatures::VanessaNightmare, 200.0f))
                vanessa->AI()->SetData(3, 3);
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
                    case Events::VanessaVanCleef::Spiders:
                        me->SummonCreature(Creatures::NightmareSpider, me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
                        me->SummonCreature(Creatures::MainSpider, me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
                        me->SummonCreature(Creatures::ChatteringHorror, me->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
                        _events.ScheduleEvent(Events::VanessaVanCleef::Spiders, 12s);
                        break;
                    case Events::VanessaVanCleef::SpiritStrike:
                        DoCastVictim(Spells::VanessaVanCleef::SpiritStrike);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SpiritStrike, 10s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
    };

    // 49681 - Nightmare Foe Reaper 5000
    struct npc_mechanical_dm : public ScriptedAI
    {
        npc_mechanical_dm(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            me->RemoveAura(Spells::Offline);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::VanessaVanCleef::SpiritStrike, 8s);
        }

        void JustDied(Unit* /*killer*/) override
        {
            DoCastSelf(Spells::VanessaVanCleef::NightmareElixirEffect);
            if (Creature* vanessa = me->FindNearestCreature(Creatures::VanessaNightmare, 200.0f))
                vanessa->AI()->SetData(3, 3);
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
                    case Events::VanessaVanCleef::SpiritStrike:
                        DoCastVictim(Spells::VanessaVanCleef::SpiritStrike);
                        _events.ScheduleEvent(Events::VanessaVanCleef::SpiritStrike, 10s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
    };

    // 49541 - Vanessa VanCleef
    struct boss_vanessa_vancleef : public BossAI
    {
        boss_vanessa_vancleef(Creature* creature) : BossAI(creature, DataTypes::BOSS_VANESSA_VANCLEEF)
        {
            ApplyCrowdControlImmunities(me);
        }

        void Reset() override
        {
            _Reset();
            _disappearCount = 0;
            _disappearActive = false;
            _finalExplosion = false;
            me->SetVisible(true);
            me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_PACIFIED);
            me->RemoveAura(Spells::VanessaVanCleef::AynashasRoot);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void JustEngagedWith(Unit* who) override
        {
            _JustEngagedWith(who);
            Talk(Texts::VanessaVanCleef::Aggro);
            events.ScheduleEvent(Events::VanessaVanCleef::DeadlyBlades, 10s);
            events.ScheduleEvent(Events::VanessaVanCleef::Backslash, 15s);
            events.ScheduleEvent(Events::VanessaVanCleef::Deflection, 20s);
            events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 12s);
            events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 18s);
            events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd3, 24s);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim != me)
                Talk(Texts::VanessaVanCleef::KilledUnit);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!_disappearActive && !_finalExplosion)
            {
                if (_disappearCount == 0 && me->HealthBelowPctDamaged(50, damage))
                {
                    _disappearCount = 1;
                    events.ScheduleEvent(Events::VanessaVanCleef::Disappear, 1s);
                }
                else if (_disappearCount == 1 && me->HealthBelowPctDamaged(25, damage))
                {
                    _disappearCount = 2;
                    events.ScheduleEvent(Events::VanessaVanCleef::Disappear, 1s);
                }
            }

            if (!_finalExplosion && me->HealthBelowPctDamaged(1, damage))
            {
                _finalExplosion = true;
                damage = 0;
                events.Reset();
                DoCastSelf(Spells::VanessaVanCleef::PowderExplosion);
                me->KillSelf();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::VanessaVanCleef::DeadlyBlades:
                        DoCastSelf(Spells::VanessaVanCleef::DeadlyBlades);
                        events.ScheduleEvent(Events::VanessaVanCleef::DeadlyBlades, 20s);
                        break;
                    case Events::VanessaVanCleef::Backslash:
                        DoCastVictim(Spells::VanessaVanCleef::Backslash);
                        events.ScheduleEvent(Events::VanessaVanCleef::Backslash, 15s);
                        break;
                    case Events::VanessaVanCleef::Deflection:
                        DoCastSelf(Spells::VanessaVanCleef::Deflection);
                        events.ScheduleEvent(Events::VanessaVanCleef::Deflection, 30s);
                        break;
                    case Events::VanessaVanCleef::SummonAdd1:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                            if (Creature* add = me->SummonCreature(Creatures::DefiasShadowguardVanessa, *me, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5s))
                                add->AI()->AttackStart(target);
                        events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd1, 30s);
                        break;
                    case Events::VanessaVanCleef::SummonAdd2:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                            if (Creature* add = me->SummonCreature(Creatures::DefiasEnforcerVanessa, *me, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5s))
                                add->AI()->AttackStart(target);
                        events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd2, 30s);
                        break;
                    case Events::VanessaVanCleef::SummonAdd3:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                            if (Creature* add = me->SummonCreature(Creatures::DefiasBloodWizardVanessa, *me, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5s))
                                add->AI()->AttackStart(target);
                        events.ScheduleEvent(Events::VanessaVanCleef::SummonAdd3, 30s);
                        break;
                    case Events::VanessaVanCleef::Disappear:
                        _disappearActive = true;
                        Talk(Texts::VanessaVanCleef::VanessaDetonate);
                        me->AttackStop();
                        me->SetReactState(REACT_PASSIVE);
                        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_PACIFIED);
                        me->SetVisible(false);
                        DoCastSelf(Spells::VanessaVanCleef::AynashasRoot);
                        for (uint8 i = 0; i < 5; ++i)
                            if (Creature* rope = me->SummonCreature(Creatures::Rope, Positions::RopeSpawn[i], TEMPSUMMON_TIMED_DESPAWN, 15s))
                                rope->CastSpell(rope, Spells::VanessaVanCleef::ClickMe, true);
                        events.ScheduleEvent(Events::VanessaVanCleef::FireBoom, 8s);
                        break;
                    case Events::VanessaVanCleef::FireBoom:
                        for (uint8 i = 0; i < 43; ++i)
                            if (Creature* fire = me->SummonCreature(Creatures::FireBunny, Positions::FieryBlaze[i], TEMPSUMMON_TIMED_DESPAWN, 10s))
                                fire->CastSpell(fire, Spells::VanessaVanCleef::FieryBlaze, true);
                        events.ScheduleEvent(Events::VanessaVanCleef::ClearShip, 10s);
                        break;
                    case Events::VanessaVanCleef::ClearShip:
                    {
                        std::list<Creature*> fires;
                        me->GetCreatureListWithEntryInGrid(fires, Creatures::FireBunny, 100.0f);
                        for (Creature* fire : fires)
                            fire->DespawnOrUnsummon();

                        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_PACIFIED);
                        me->RemoveAura(Spells::VanessaVanCleef::AynashasRoot);
                        me->SetVisible(true);
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, true))
                            AttackStart(target);
                        _disappearActive = false;
                        break;
                    }
                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            if (!_disappearActive)
                me->DoMeleeAttackIfReady();
        }

    private:
        uint8 _disappearCount = 0;
        bool _disappearActive = false;
        bool _finalExplosion = false;
    };

    // 49457 - Steam Valve
    struct npc_steam_valve : public ScriptedAI
    {
        npc_steam_valve(Creature* creature) : ScriptedAI(creature)
        {
            me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        }

        bool OnGossipHello(Player* player) override
        {
            InstanceScript* instance = me->GetInstanceScript();
            if (!instance)
                return false;

            me->RemoveAllAuras();
            me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);

            uint8 count = instance->GetData(Misc::SteamValveCount);
            instance->SetData(Misc::SteamValveCount, count + 1);

            if (count + 1 >= 4)
            {
                for (MapReference const& ref : me->GetMap()->GetPlayers())
                {
                    Player* p = ref.GetSource();
                    if (!p || !p->IsInWorld())
                        continue;

                    if (p->GetVehicle())
                    {
                        Unit* vehicleBase = p->GetVehicle()->GetBase();
                        if (vehicleBase && vehicleBase->GetEntry() == Creatures::TrapBunny)
                        {
                            p->ExitVehicle(&Positions::MagmaPullPlayerExit);
                            p->m_Events.AddEventAtOffset([p]()
                            {
                                if (p && p->IsInWorld())
                                    p->CastSpell(p, Spells::VanessaVanCleef::NightmareElixirEffect, true);
                            }, 3s);
                        }
                    }
                }

                instance->SetData(Misc::SteamValveCount, 0);

                if (Creature* vanessaNightmare = me->FindNearestCreature(Creatures::VanessaNightmare, 50.f))
                    vanessaNightmare->AI()->SetData(4, 1);
            }

            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
    };
}

void AddSC_custom_deadmines_vanessa_vancleef()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    RegisterCreatureAI(npc_rope_ship);
    RegisterCreatureAI(npc_icycle_dm);
    RegisterCreatureAI(npc_note);
    RegisterCreatureAI(npc_steam_valve);
    RegisterCreatureAI(npc_magma_pull);
    RegisterCreatureAI(npc_rope_away);
    RegisterCreatureAI(npc_lightning_orbs);
    RegisterCreatureAI(npc_defias_shadowguard);
    RegisterCreatureAI(npc_defias_enforcer);
    RegisterCreatureAI(npc_defias_envoker);
    RegisterCreatureAI(npc_defias_bloodwizard);
    RegisterCreatureAI(npc_oaf_lackey);
    RegisterCreatureAI(npc_goblin_overseer);
    RegisterCreatureAI(npc_enraged_worgen_dm);
    RegisterCreatureAI(npc_james_dm);
    RegisterCreatureAI(npc_vanessa_intro);
    RegisterCreatureAI(npc_vanessa_nightmare);
    RegisterCreatureAI(npc_glubtok_dm);
    RegisterCreatureAI(npc_helix_dm);
    RegisterCreatureAI(npc_mechanical_dm);
    RegisterCreatureAI(boss_vanessa_vancleef);
}
