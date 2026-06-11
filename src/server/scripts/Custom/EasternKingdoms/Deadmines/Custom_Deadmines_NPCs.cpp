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
#include "G3D/Vector3.h"
#include <list>

#include "Custom_Instance_Deadmines.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    static void ApplyCrowdControlImmunities(Creature* creature)
    {
        creature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
        creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);
    }

    // 47162 - Glubtok
    struct boss_glubtok : public BossAI
    {
        boss_glubtok(Creature* creature) : BossAI(creature, DataTypes::BOSS_GLUBTOK)
        {
            me->SetCanDualWield(true);
            ApplyCrowdControlImmunities(me);
        }

        void Reset() override
        {
            _Reset();
            _phase = 0;
            _dying = false;
            _transitionDone = false;
            me->SetReactState(REACT_AGGRESSIVE);
            SetCombatMovement(true);
            me->SetDisableGravity(false);
            me->SetCanFly(false);
            me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
            me->RemoveUnitFlag2(UNIT_FLAG2_FEIGN_DEATH);
            me->ClearUnitState(UNIT_STATE_CANNOT_TURN);
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            Talk(Texts::GlubtokAggro);
            _phase = 1;
            events.RescheduleEvent(Events::GlubtokCastFistOfFlame, std::chrono::seconds(Glubtok::FistOfFlameTimer));
        }

        void KilledUnit(Unit* victim) override
        {
            if(victim != me)
                Talk(Texts::GlubtokKill);
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);

            // Clean up all summoned units
            for (Creature* unit : _summonedUnits)
                if (unit)
                    unit->DespawnOrUnsummon();
            _summonedUnits.clear();

            if (_platter)
            {
                _platter->AI()->DoAction(2); // Cleanup action
                _platter->DespawnOrUnsummon();
                _platter = nullptr;
            }

            // Check for Ready for Raiding achievement (heroic only, no firewall hits)
            if (killer->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC)
            {
                // Award achievement only to players who were NOT hit by firewall
                AchievementEntry const* achievement = sAchievementStore.LookupEntry(Achievements::ReadyForRaidingDeadmines);
                if (achievement)
                {
                    Map::PlayerList const& players = instance->instance->GetPlayers();
                    if (players.empty())
                        return;

                    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        if (Player* player = itr->GetSource())
                        {
                            // Check if this specific player was hit by firewall
                            if (instance->GetData(player->GetGUID().GetCounter()) == 0)
                                player->CompletedAchievement(achievement);
                        }
                    }
                }
            }
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            me->ClearUnitState(UNIT_STATE_CANNOT_TURN);

            // Clean up all summoned units
            for (Creature* unit : _summonedUnits)
                if (unit)
                    unit->DespawnOrUnsummon();
            _summonedUnits.clear();

            if (_platter)
                _platter->AI()->DoAction(2); // Cleanup action

            // Reset dying state on evade
            _dying = false;

            BossAI::EnterEvadeMode();
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (me->HealthBelowPctDamaged(Glubtok::PhaseTransitionHealthPct, damage) && _phase == 1)
            {
                _phase = 2;
                damage = 0;
                events.Reset();
                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();
                me->GetMotionMaster()->Clear();
                me->NearTeleportTo(Glubtok::Phase2Center, true);
                DoCast(Spells::TeleportVisual);
                Talk(Texts::GlubtokHead1);
                events.RescheduleEvent(Events::GlubtokCastArcanePower1, std::chrono::seconds(Glubtok::ArcanePower1Timer));
            }

            if (me->HealthBelowPctDamaged(Glubtok::DeathHealthPct, damage))
            {
                if (!_dying && _transitionDone)
                {
                    damage = 0;
                    events.Reset();
                    DoCast(me, Spells::ArcaneOverloadInitial, true);
                    SummonBeams();
                    Talk(Texts::GlubtokDeath);
                    _dying = true;
                    events.RescheduleEvent(Events::GlubtokFallGround, std::chrono::seconds(Glubtok::FallGroundTimer));
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != EFFECT_MOTION_TYPE)
                return;

            if (id == 1) // POINT_FALL_GROUND
            {
                me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);

                me->m_Events.AddEventAtOffset([this]()
                {
                    if (me && me->IsInWorld())
                        me->CastSpell(me, Spells::ArcaneOverloadBoom, true);
                }, std::chrono::seconds(Glubtok::BoomTimer));

                me->m_Events.AddEventAtOffset([this]()
                {
                    if (me && me->IsInWorld())
                    {
                        // Suicide spell causes bug with dungeon objective/loot (clears tap list)
                        // Using FeignDeath + player->Kill workaround instead
                        DoCast(Spells::FeignDeath);
                        //me->CastSpell(me, Spells::ArcaneOverloadSuicide, true);
                    }
                }, std::chrono::seconds(Glubtok::SuicideTimer));

                // Schedule model change separately, not nested, to ensure it executes after FeignDeath
                me->m_Events.AddEventAtOffset([this]()
                {
                    if (me && me->IsInWorld())
                    {
                        if (CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(Creatures::GlubtokDead))
                        {
                            if (CreatureModel const* model = ObjectMgr::ChooseDisplayId(ci))
                                me->SetDisplayId(model->CreatureDisplayID);

                            Map::PlayerList const& players = instance->instance->GetPlayers();
                            if (players.empty())
                                return;

                            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                            {
                                if (Player* player = itr->GetSource())
                                {
                                    player->Kill(player, me);
                                }
                            }
                        }
                    }
                }, std::chrono::seconds(Glubtok::SuicideTimer) + 500ms);
            }
        }

        Position GenerateTargetForBeamBunny(bool left)
        {
            float angle;
            Position pos;

            for (uint8 i = 0; i < 8; ++i)
            {
                if (left)
                    angle = me->GetOrientation() - float(M_PI) / 2 + frand(-float(M_PI) / 3.0f, float(M_PI) / 6.0f);
                else
                    angle = me->GetOrientation() + float(M_PI) / 2 + frand(-float(M_PI) / 6.0f, float(M_PI) / 3.0f);

                pos.m_positionZ = me->GetPositionZ() + frand(4.0f, 23.0f);

                me->GetNearPoint2D(me, pos.m_positionX, pos.m_positionY, 25.0f, angle);

                if (me->GetExactDist2d(pos.m_positionX, pos.m_positionY) >= 7.5f)
                    break;
            }
            return pos;
        }

        void SummonBeams()
        {
            Position pos1, pos2, pos3;
            uint32 spellID;
            for (int i = 0; i < 8; ++i)
            {
                if (i < 4)
                {
                    pos1 = GenerateTargetForBeamBunny(true);
                    pos2 = GenerateTargetForBeamBunny(true);
                    pos3 = GenerateTargetForBeamBunny(true);
                    spellID = Spells::ArcaneFireBeam;
                }
                else
                {
                    pos1 = GenerateTargetForBeamBunny(false);
                    pos2 = GenerateTargetForBeamBunny(false);
                    pos3 = GenerateTargetForBeamBunny(false);
                    spellID = Spells::ArcaneFrostBeam;
                }

                if (Creature* dummy = me->SummonCreature(Creatures::BeamBunny, pos1, TEMPSUMMON_TIMED_DESPAWN, 7s))
                {
                    // Start the movement path first
                    Movement::MoveSplineInit init(dummy);
                    init.Path().push_back(G3D::Vector3(pos1.m_positionX, pos1.m_positionY, pos1.m_positionZ));
                    init.Path().push_back(G3D::Vector3(pos2.m_positionX, pos2.m_positionY, pos2.m_positionZ));
                    init.Path().push_back(G3D::Vector3(pos3.m_positionX, pos3.m_positionY, pos3.m_positionZ));
                    init.SetVelocity(1.5f);
                    init.SetFly();
                    init.SetCyclic();
                    init.Launch();

                    // Cast the beam spell after movement starts - capture me by GUID to ensure it works even if AI state changes
                    ObjectGuid dummyGUID = dummy->GetGUID();
                    me->m_Events.AddEventAtOffset([this, dummyGUID, spellID]()
                        {
                            if (Unit* dummy = ObjectAccessor::GetUnit(*me, dummyGUID))
                                dummy->CastSpell(me, spellID, true);
                        }, 100ms);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            // During dying phase, process events even without a victim
            if (!_dying && !UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) && !me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::GlubtokCastArcanePower1:
                        Talk(Texts::GlubtokHead2);
                        events.RescheduleEvent(Events::GlubtokCastArcanePower2, std::chrono::seconds(Glubtok::ArcanePower2Timer));
                        break;
                    case Events::GlubtokCastArcanePower2:
                        SetCombatMovement(false);
                        me->AttackStop();
                        me->RemoveAllAuras();
                        events.RescheduleEvent(Events::GlubtokCastArcanePower3, std::chrono::seconds(Glubtok::ArcanePower3Timer));
                        break;
                    case Events::GlubtokCastArcanePower3:
                    {
                        SetCombatMovement(false);
                        DoCast(me, Spells::ArcanePower, true);
                        Talk(Texts::GlubtokArcanePower);
                        SummonBeams();
                        me->AddUnitState(UNIT_STATE_CANNOT_TURN);
                        me->SetDisableGravity(true);
                        Position takeoffPos = { me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 2.0f, me->GetOrientation() };
                        me->GetMotionMaster()->MoveTakeoff(0, takeoffPos, {}, 1.5f);
                        _transitionDone = true;

                        // Summon main platter for heroic fire wall
                        if (IsHeroic())
                        {
                            _platter = me->SummonCreature(Creatures::GlubtokMainPlatter, Glubtok::Phase2Center.m_positionX, Glubtok::Phase2Center.m_positionY, Glubtok::Phase2Center.m_positionZ + 2.0f);
                            if (_platter)
                            {
                                _platter->setActive(true);
                                Talk(Texts::GlubtokFirewall);

                                _firewallPhase = true;

                                // Schedule firewall bunny spawn after Glubtok is in the air (1.5s takeoff)
                                events.RescheduleEvent(Events::GlubtokSpawnFirewallBunny, std::chrono::milliseconds(1500));
                            }
                        }

                        events.RescheduleEvent(Events::GlubtokCastBlossom, std::chrono::seconds(Glubtok::BlossomTimer));
                        break;
                    }
                    case Events::GlubtokCastBlossom:
                    {
                        events.RescheduleEvent(Events::GlubtokCastBlossom, std::chrono::milliseconds(urand(0, 2500)));

                        uint32 targetEntry = 0;
                        uint32 targetSpellID = 0;
                        uint32 indicatorSpellID = 0;
                        uint32 summonSpellID = 0;
                        if (roll_chance(50))
                        {
                            targetEntry = Creatures::FrostBlossomDummy;
                            targetSpellID = Spells::FrostBlossom;
                            indicatorSpellID = Spells::FrostBlossomVisual;
                            summonSpellID = Spells::SummonFrostBlossom;
                        }
                        else
                        {
                            targetEntry = Creatures::FireBlossomDummy;
                            targetSpellID = Spells::FireBlossom;
                            indicatorSpellID = Spells::FireBlossomVisual;
                            summonSpellID = Spells::SummonFireBlossom;
                        }

                        std::list<Creature*> cList;
                        me->GetCreatureListWithEntryInGrid(cList, targetEntry, 100.0f);

                        cList.remove_if(Trinity::UnitAuraCheck(true, indicatorSpellID));

                        if (cList.empty())
                            break;

                        if (Creature* target = Trinity::Containers::SelectRandomContainerElement(cList))
                        {
                            target->CastSpell(target, indicatorSpellID, true);
                            me->CastSpell(target, targetSpellID);

                            // In firewall phase, also cast the summon spell to spawn the add
                            if (_firewallPhase)
                                target->CastSpell(target, summonSpellID, true);
                        }
                        break;
                    }
                    case Events::GlubtokCastFistOfFlame:
                    {
                        DoCast(me, Spells::FistOfFlame);
                        Talk(Texts::GlubtokFire);
                        events.RescheduleEvent(Events::GlubtokCastBlink, std::chrono::seconds(Glubtok::BlinkTimer));
                        events.RescheduleEvent(Events::GlubtokCastFistOfFrost, std::chrono::seconds(Glubtok::FistOfFrostTimer));
                        me->SetReactState(REACT_AGGRESSIVE);
                        // Re-engage with saved blink target or current victim
                        if (_blinkTarget && _blinkTarget->IsAlive())
                            AttackStart(_blinkTarget);
                        else if (Unit* victim = me->GetVictim())
                            AttackStart(victim);
                        _blinkTarget = nullptr;
                        break;
                    }
                    case Events::GlubtokCastFistOfFrost:
                    {
                        DoCast(me, Spells::FistOfFrost);
                        Talk(Texts::GlubtokFrost);
                        events.RescheduleEvent(Events::GlubtokCastBlink, std::chrono::seconds(Glubtok::BlinkTimer));
                        events.RescheduleEvent(Events::GlubtokCastFistOfFlame, std::chrono::seconds(Glubtok::FistOfFlameTimer));
                        me->SetReactState(REACT_AGGRESSIVE);
                        // Re-engage with saved blink target or current victim
                        if (_blinkTarget && _blinkTarget->IsAlive())
                            AttackStart(_blinkTarget);
                        else if (Unit* victim = me->GetVictim())
                            AttackStart(victim);
                        _blinkTarget = nullptr;
                        break;
                    }
                    case Events::GlubtokCastBlink:
                    {
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, false))
                        {
                            DoCast(target, Spells::Blink);
                            me->SetReactState(REACT_PASSIVE);
                            me->AttackStop();
                            me->SetFacingToObject(target);
                            if (IsHeroic())
                            {
                                // Save the blinked target to re-engage after threat reset
                                _blinkTarget = target;
                                me->GetThreatManager().ClearAllThreat();
                            }
                        }
                        events.RescheduleEvent(Events::GlubtokCastFistOfFlame, std::chrono::seconds(1));
                        break;
                    }
                    case Events::GlubtokFallGround:
                        me->ClearUnitState(UNIT_STATE_CANNOT_TURN);
                        me->GetMotionMaster()->MoveFall(1);
                        break;
                    case Events::GlubtokSpawnFirewallBunny:
                    {
                        // Spawn firewall bunny at same Z as platter
                        if (Creature* glubtokBunny = me->SummonCreature(Creatures::FirewallBunny, me->GetPositionX(), me->GetPositionY(), Glubtok::Phase2Center.m_positionZ + 2.0f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                        {
                            glubtokBunny->CastSpell(glubtokBunny, Spells::TriggerFireWall, true);
                            _summonedUnits.push_back(glubtokBunny);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            if (_phase == 1)
                me->DoMeleeAttackIfReady();
        }

    private:
        uint8 _phase = 0;
        bool _dying = false;
        bool _transitionDone = false;
        bool _firewallPhase = false;
        Creature* _platter = nullptr;
        Unit* _blinkTarget = nullptr;
        std::vector<Creature*> _summonedUnits;
    };

    // 48974 - Glubtok Main Platter (Heroic Fire Wall Platform)
    struct npc_glubtok_main_platter : public PassiveAI
    {
        npc_glubtok_main_platter(Creature* creature) : PassiveAI(creature)
        {
            _turnTimer = 100ms;
            me->SetDisableGravity(true);
            Talk(0);
        }

        void Reset() override
        {
            _events.Reset();
            _summonedUnits.clear();
            // Schedule first wave of summons (firewall bunnies and secondary platters)
            _events.RescheduleEvent(1, std::chrono::milliseconds(100));
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case 1: // Summon first wave - firewall bunnies and secondary platters
                    {
                        // Summon 2 firewall bunnies for seats 0 and 7
                        if (Creature* bunny1 = me->SummonCreature(Creatures::FirewallBunny, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                        {
                            bunny1->CastSpell(bunny1, Spells::TriggerFireWall, true);
                            bunny1->EnterVehicle(me, 0);
                            _summonedUnits.push_back(bunny1);
                        }
                        if (Creature* bunny2 = me->SummonCreature(Creatures::FirewallBunny, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                        {
                            bunny2->CastSpell(bunny2, Spells::TriggerFireWall, true);
                            bunny2->EnterVehicle(me, 7);
                            _summonedUnits.push_back(bunny2);
                        }

                        // Summon secondary platters for seats 1-6
                        struct PlatterEntry { uint32 entry; int8 seat; };
                        PlatterEntry platters[] = {
                            { Creatures::FireWall1A, 1 },
                            { Creatures::FireWall2A, 2 },
                            { Creatures::FireWall1B, 3 },
                            { Creatures::FireWall2B, 4 },
                            { Creatures::FireWall1C, 5 },
                            { Creatures::FireWall2C, 6 }
                        };

                        for (const auto& platter : platters)
                        {
                            if (Creature* secondary = me->SummonCreature(platter.entry, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                secondary->CastSpell(secondary, Spells::TriggerFireWall, true);
                                secondary->EnterVehicle(me, platter.seat);
                                _summonedUnits.push_back(secondary);
                                _secondaryPlatters.push_back(secondary);
                            }
                        }

                        // Schedule second wave for secondary platters to summon their own bunnies
                        _events.RescheduleEvent(2, std::chrono::milliseconds(500));
                        break;
                    }
                    case 2: // Trigger secondary platters to summon their bunnies
                    {
                        for (Creature* secondary : _secondaryPlatters)
                            if (secondary)
                                secondary->AI()->DoAction(1); // Action to summon bunnies
                        break;
                    }
                }
            }

            if (_turnTimer > Milliseconds(diff))
                _turnTimer -= Milliseconds(diff);
            else
            {
                me->SetFacingTo(me->GetOrientation() - float(M_PI) / 150);
                _turnTimer = 100ms;
            }
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool /*apply*/) override
        {
            who->SetDisableGravity(true);
        }

        void DoAction(int32 actionID) override
        {
            if (actionID == 2) // Cleanup action
            {
                // First tell secondary platters to cleanup their bunnies
                for (Creature* secondary : _secondaryPlatters)
                    if (secondary)
                        secondary->AI()->DoAction(2); // Cleanup secondary platters
                // Then cleanup our own summons (which includes the secondary platters)
                CleanupSummons();
            }
        }

        void CleanupSummons()
        {
            for (Creature* unit : _summonedUnits)
            {
                if (unit)
                {
                    // Remove any fire wall auras before despawning
                    unit->RemoveAllAuras();
                    unit->DespawnOrUnsummon();
                }
            }
            _summonedUnits.clear();

            // Also despawn secondary platters themselves
            for (Creature* secondary : _secondaryPlatters)
            {
                if (secondary)
                {
                    secondary->RemoveAllAuras();
                    secondary->DespawnOrUnsummon();
                }
            }
            _secondaryPlatters.clear();
        }

    private:
        Milliseconds _turnTimer;
        EventMap _events;
        std::vector<Creature*> _summonedUnits;
        std::vector<Creature*> _secondaryPlatters;
    };

    // Glubtok Secondary Platter (Fire Wall passengers)
    // 48975,48976,49039,49040,49041,49042
    struct npc_glubtok_secondary_platter : public PassiveAI
    {
        npc_glubtok_secondary_platter(Creature* creature) : PassiveAI(creature)
        {
            _summonedUnits.clear();
        }

        void DoAction(int32 actionID) override
        {
            if (actionID == 1) // Summon firewall bunnies
            {
                if (!me->GetVehicleKit())
                    return;

                uint8 seatCount = me->GetVehicleKit()->GetAvailableSeatCount();
                for (uint8 i = 0; i < seatCount; ++i)
                {
                    if (Creature* bunny = me->SummonCreature(Creatures::FirewallBunny, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        bunny->CastSpell(bunny, Spells::TriggerFireWall, true);
                        bunny->EnterVehicle(me, i);
                        _summonedUnits.push_back(bunny);
                    }
                }
            }
            else if (actionID == 2) // Cleanup action
            {
                CleanupSummons();
            }
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool /*apply*/) override
        {
            who->SetDisableGravity(true);
        }

        void CleanupSummons()
        {
            for (Creature* unit : _summonedUnits)
                if (unit)
                    unit->DespawnOrUnsummon();
            _summonedUnits.clear();
        }

    private:
        std::vector<Creature*> _summonedUnits;
    };

    // 48230 - Ogre Henchman
    struct npc_ogre_henchman : public ScriptedAI
    {
        npc_ogre_henchman(Creature* creature) : ScriptedAI(creature)
        {
            ApplyCrowdControlImmunities(me);
        }

        void Reset() override
        {
            me->RemoveAllAuras();
            _hasCastMotivation = false;
            _isSleeping = false;
            _events.Reset();

            // Schedule OOC say event (initial spawn)
            _events.RescheduleEvent(Events::OgreHenchmanOOCSay, 30s, 45s);

            // Special event for spawn ID 375925 (initial spawn)
            if (me->GetSpawnId() == CreatureSpawns::OgreHenchmanSleeper)
                _events.RescheduleEvent(Events::OgreHenchmanSleepEvent, 60s);
        }

        void JustEngagedWith(Unit* who) override
        {
            ScriptedAI::JustEngagedWith(who);
            _events.RescheduleEvent(Events::OgreHenchmanUppercut, std::chrono::seconds(5));

            if (roll_chance(30))
                Talk(Texts::OgreHenchmanAggro, who);
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);
            _events.RescheduleEvent(Events::OgreHenchmanOOCSay, 30s, 45s);
        }

        void JustReachedHome() override
        {
            _events.RescheduleEvent(Events::OgreHenchmanOOCSay, 30s, 45s);

            // Special event for spawn ID 375925
            if (me->GetSpawnId() == CreatureSpawns::OgreHenchmanSleeper)
                _events.RescheduleEvent(Events::OgreHenchmanSleepEvent, 60s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (me->HealthBelowPctDamaged(25, damage) && !_hasCastMotivation)
            {
                _hasCastMotivation = true;
                uint32 spellId = (me->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC) ? Spells::OgrishMotivationHeroic : Spells::OgrishMotivationNormal;

                DoCastSelf(spellId);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case Events::OgreHenchmanOOCSay:
                        {
                            // Don't say OOC if sleeping (spawn ID 375925)
                            if (_isSleeping)
                            {
                                _events.RescheduleEvent(Events::OgreHenchmanOOCSay, 30s, 45s);
                                break;
                            }

                            uint32 sayId = (me->GetSpawnId() == CreatureSpawns::OgreHenchmanByCannon) ? Texts::OgreHenchmanCandle : Texts::OgreHenchmanOOC;
                            uint32 minTime = (me->GetSpawnId() == CreatureSpawns::OgreHenchmanByCannon) ? 45 : 30;
                            uint32 maxTime = (me->GetSpawnId() == CreatureSpawns::OgreHenchmanByCannon) ? 60 : 45;

                            if (roll_chance(30))
                                Talk(sayId);

                            _events.RescheduleEvent(Events::OgreHenchmanOOCSay, std::chrono::seconds(minTime), std::chrono::seconds(maxTime));
                            break;
                        }
                        case Events::OgreHenchmanSleepEvent:
                        {
                            if (me->GetSpawnId() == CreatureSpawns::OgreHenchmanSleeper)
                            {
                                _isSleeping = true;
                                Talk(Texts::OgreHenchmanSleep);
                                _events.RescheduleEvent(Events::OgreHenchmanSleepAura, 3s);
                            }
                            _events.RescheduleEvent(Events::OgreHenchmanSleepEvent, 90s);
                            break;
                        }
                        case Events::OgreHenchmanSleepAura:
                        {
                            DoCastSelf(Spells::AuraSleep);
                            _events.RescheduleEvent(Events::OgreHenchmanWakeUp, 30s);
                            break;
                        }
                        case Events::OgreHenchmanWakeUp:
                        {
                            me->RemoveAurasDueToSpell(Spells::AuraSleep);
                            _isSleeping = false;
                            break;
                        }
                        default:
                            break;
                    }
                }
                return;
            }

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::OgreHenchmanUppercut:
                        if (Unit* victim = me->GetVictim())
                            DoCast(victim, Spells::Uppercut);
                        _events.RescheduleEvent(Events::OgreHenchmanUppercut, 10s, 14s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool _hasCastMotivation = false;
        bool _isSleeping = false;
    };

    // Ogre Bodyguard Dialogue Entry
    struct OgreBodyguardDialogueEntry
    {
        int8 talkId;
        Milliseconds delay;
        bool isFinal;
    };

    // 48262 - Ogre Bodyguard
    struct npc_ogre_bodyguard : public ScriptedAI
    {
        npc_ogre_bodyguard(Creature* creature) : ScriptedAI(creature), _dialogueIndex(0)
        {
            _hasCastFrenzy = false;
            _conversationInProgress = false;
        }

        void Reset() override
        {
            ScriptedAI::Reset();
            _hasCastFrenzy = false;
            _dialogueIndex = 0;
            _conversationInProgress = false;
            _events.Reset();

            // Schedule conversation start event (1-5s random)
            _events.RescheduleEvent(Events::OgreBodyguardConversation, 1s, 5s);
        }

        void JustEngagedWith(Unit* who) override
        {
            ScriptedAI::JustEngagedWith(who);

            if (roll_chance(50))
                Talk(Texts::OgreBodyguardAggro, who);

            _events.RescheduleEvent(Events::OgreBodyguardBonk, 4s, 8s);

            // Stop conversation on combat
            _events.CancelEvent(Events::OgreBodyguardConversation);
            _events.CancelEvent(Events::OgreBodyguardDialogueLine);
            _conversationInProgress = false;
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);
            _conversationInProgress = false;
            _dialogueIndex = 0;
            _events.RescheduleEvent(Events::OgreBodyguardConversation, 1s, 5s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (me->HealthBelowPctDamaged(30, damage) && !_hasCastFrenzy)
            {
                _hasCastFrenzy = true;
                DoCastSelf(Spells::Frenzy);
                Talk(Texts::OgreBodyguardFrenzy);
            }
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == 1 && value == 1) // Cancel conversation start event
            {
                _events.CancelEvent(Events::OgreBodyguardConversation);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::OgreBodyguardConversation:
                        StartConversation();
                        break;
                    case Events::OgreBodyguardDialogueLine:
                        ProcessDialogueLine();
                        break;
                    case Events::OgreBodyguardBonk:
                        if (Unit* victim = me->GetVictim())
                            me->CastSpell(victim, Spells::Bonk);
                        _events.RescheduleEvent(Events::OgreBodyguardBonk, 14s, 17s);
                        break;
                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            me->DoMeleeAttackIfReady();
        }

        void StartConversation()
        {
            if (_conversationInProgress)
                return;

            if (Creature* other = me->FindNearestCreature(Creatures::OgreBodyguard, 10.f, true))
            {
                if (other && other->IsAlive())
                {
                    // Only the ogre with the lower GUID initiates the conversation
                    if (me->GetGUID().GetCounter() < other->GetGUID().GetCounter())
                    {
                        _conversationInProgress = true;
                        other->AI()->SetData(1, 1); // Tell other to cancel their start event
                        _dialogueIndex = 0;
                        _events.RescheduleEvent(Events::OgreBodyguardDialogueLine, 1s);
                    }
                    return;
                }
            }
        }

        void ProcessDialogueLine()
        {
            if (_dialogueIndex >= _dialogueSequence.size())
            {
                _conversationInProgress = false;
                _dialogueIndex = 0;
                // Schedule next conversation cycle (45-60s)
                _events.RescheduleEvent(Events::OgreBodyguardConversation, 45s, 60s);
                return;
            }

            const OgreBodyguardDialogueEntry& entry = _dialogueSequence[_dialogueIndex];

            Creature* speaker = me;
            Creature* otherOgre = me->FindNearestCreature(Creatures::OgreBodyguard, 10.f, true);

            // Determine speaker based on dialogue index (alternating)
            if (speaker && otherOgre)
            {
                // Even indices: lower GUID speaks, Odd indices: higher GUID speaks
                if (_dialogueIndex % 2 == 0)
                {
                    if (speaker->GetGUID().GetCounter() > otherOgre->GetGUID().GetCounter())
                        std::swap(speaker, otherOgre);
                }
                else
                {
                    if (speaker->GetGUID().GetCounter() < otherOgre->GetGUID().GetCounter())
                        std::swap(speaker, otherOgre);
                }
            }

            if (speaker)
                speaker->AI()->Talk(entry.talkId);

            ++_dialogueIndex;

            if (entry.isFinal)
            {
                _conversationInProgress = false;
                _dialogueIndex = 0;
                _events.RescheduleEvent(Events::OgreBodyguardConversation, 45s, 60s);
            }
            else
            {
                _events.RescheduleEvent(Events::OgreBodyguardDialogueLine, entry.delay);
            }
        }

    private:
        EventMap _events;
        bool _hasCastFrenzy = false;
        size_t _dialogueIndex;
        bool _conversationInProgress;

        static const std::vector<OgreBodyguardDialogueEntry> _dialogueSequence;
    };

    const std::vector<OgreBodyguardDialogueEntry> npc_ogre_bodyguard::_dialogueSequence =
    {
        { Texts::OgreBodyguardConversation1, 3s, false },
        { Texts::OgreBodyguardConversation2, 5s, false },
        { Texts::OgreBodyguardConversation3, 4s, false },
        { Texts::OgreBodyguardConversation4, 3s, false },
        { Texts::OgreBodyguardConversation5, 5s, false },
        { Texts::OgreBodyguardConversation6, 4s, false },
        { Texts::OgreBodyguardConversation7, 3s, false },
        { Texts::OgreBodyguardConversation8, 5s, false },
        { Texts::OgreBodyguardConversation9, 4s, false },
        { Texts::OgreBodyguardConversation10, 3s, false },
        { Texts::OgreBodyguardConversation10, 3s, true } // Both say line 11
    };

    // 49136 - Helix Crew
    struct npc_helix_crew : public ScriptedAI
    {
        npc_helix_crew(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            ApplyCrowdControlImmunities(me);
            me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
        }

        void Reset() override
        {
            ScriptedAI::Reset();
            _events.Reset();
            summons.DespawnAll();
            _events.RescheduleEvent(Events::HelixCrewStickyBomb, 8s, 30s);
        }

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (summoner && summoner->ToCreature())
                _helixGUID = summoner->GetGUID();
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            summons.Despawn(summon);
        }

        void JustDied(Unit* /*killer*/) override
        {
            summons.DespawnAll();
            me->DespawnOrUnsummon();
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::HelixCrewStickyBomb:
                    {
                        Unit* target = nullptr;
                        if (Creature* helix = ObjectAccessor::GetCreature(*me, _helixGUID))
                        {
                            if (helix->GetThreatManager().GetThreatListSize() > 0)
                            {
                                std::vector<ThreatReference*> threatList = helix->GetThreatManager().GetModifiableThreatList();
                                if (!threatList.empty())
                                {
                                    auto it = threatList.begin();
                                    std::advance(it, urand(0, threatList.size() - 1));
                                    target = (*it)->GetVictim();
                                }
                            }
                        }

                        if (target && target->IsAlive())
                        {
                            me->SetFacingToObject(target);
                            Talk(Texts::HelixCrewStickyBomb);
                            me->HandleEmoteCommand(Emote::EMOTE_ONESHOT_ATTACK_THROWN);
                            if (Creature* stickyBomb = me->SummonCreature(Creatures::StickyBomb, me->GetPosition()))
                                stickyBomb->GetMotionMaster()->MoveJump(EVENT_JUMP, target->GetPosition());
                        }
                        _events.RescheduleEvent(Events::HelixCrewStickyBomb, 15s, 35s);
                        break;
                    }
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
        SummonList summons;
        ObjectGuid _helixGUID;
    };

    // 47314 - Sticky Bomb
    struct npc_sticky_bomb : public ScriptedAI
    {
        npc_sticky_bomb(Creature* creature) : ScriptedAI(creature)
        {
            ApplyCrowdControlImmunities(me);
            me->SetReactState(REACT_PASSIVE);
        }

        void Reset() override
        {
            ScriptedAI::Reset();
            _events.Reset();
            _events.RescheduleEvent(Events::StickyBombInitial, 200ms);
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->DespawnOrUnsummon();
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::StickyBombInitial:
                        _events.RescheduleEvent(Events::StickyBombYellow, 2s);
                        _events.RescheduleEvent(Events::StickyBombOrange, 4s);
                        _events.RescheduleEvent(Events::StickyBombRed, 5500ms);
                        _events.RescheduleEvent(Events::StickyBombArmed, 5800ms);
                        _events.RescheduleEvent(Events::StickyBombExplode, 17800ms);
                        break;
                    case Events::StickyBombYellow:
                        DoCastSelf(Spells::ArmingVisualYellow);
                        break;
                    case Events::StickyBombOrange:
                        DoCastSelf(Spells::ArmingVisualOrange);
                        break;
                    case Events::StickyBombRed:
                        DoCastSelf(Spells::ArmingVisualRed);
                        break;
                    case Events::StickyBombArmed:
                        DoCastSelf(Spells::ArmedState);
                        break;
                    case Events::StickyBombExplode:
                        DoCastSelf(Spells::StickyBombExplode);
                        me->DespawnOrUnsummon(500ms);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
    };

    // 47297 - Lumbering Oaf
    struct npc_lumbering_oaf : public ScriptedAI
    {
        npc_lumbering_oaf(Creature* creature) : ScriptedAI(creature)
        {
            ApplyCrowdControlImmunities(me);
            me->setActive(true);
        }

        void Reset() override
        {
            ScriptedAI::Reset();
            _events.Reset();
            _helixGUID.Clear();
            _ridingOaf = false;
            _helixFaceRiding = false;
        }

        void DoAction(int32 action) override
        {
            if (action == Actions::OafCharge)
                _events.RescheduleEvent(Events::OafCharge1, 1500ms);
        }

        void JustEngagedWith(Unit* who) override
        {
            if (Unit* helix = me->GetVehicleKit()->GetPassenger(0))
            {
                helix->EngageWithTarget(who);
            }
            int8 randomEvent = RAND(Events::OafCharge0, Events::OafThrowHelix);

            _events.RescheduleEvent(randomEvent, 15s, 20s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (me->GetHealth() <= damage)
                if (Vehicle* vehicle = me->GetVehicleKit())
                    vehicle->RemoveAllPassengers();
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (Creature* helix = me->FindNearestCreature(Creatures::HelixGearBreaker, 20.0f, true))
                if (CreatureAI* ai = helix->AI())
                    ai->SetData(Misc::HelixOafDead, 1);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE)
            {
                if (id == 1)
                    _events.RescheduleEvent(Events::OafCharge2, 500ms);
                else if (id == 2)
                    _events.RescheduleEvent(Events::OafCharge3, 200ms);
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
                case Events::OafCharge0:
                {
                    if (_helixFaceRiding)
                    {
                        _events.RescheduleEvent(Events::OafCharge0, 5s);
                        break;
                    }
                    if (Unit* target = SelectTarget(SelectTargetMethod::MaxThreat, 0, 100.0f, true))
                    {
                        _ridingOaf = true;
                        me->SetReactState(REACT_PASSIVE);
                        DoCast(target, Spells::OafCharge, true);
                        Talk(Texts::OafCharge);
                    }
                    break;
                }
                case Events::OafCharge1:
                    if (Creature* moveBunny = me->GetMap()->GetCreatureBySpawnId(CreatureSpawns::OafMoveToBunny))
                        me->GetMotionMaster()->MovePoint(1, moveBunny->GetPosition());
                    break;
                case Events::OafCharge2:
                    if (Creature* chargeBunny = me->GetMap()->GetCreatureBySpawnId(CreatureSpawns::OafChargePosition))
                        me->GetMotionMaster()->MoveCharge(chargeBunny->GetPositionX(), chargeBunny->GetPositionY(), chargeBunny->GetPositionZ(), 42.0f, 2);
                    break;

                case Events::OafCharge3:
                {
                    DoCastAOE(Spells::OafSmash);
                    Talk(Texts::OafSmash);
                    me->SetReactState(REACT_AGGRESSIVE);
                    if (Vehicle* vehicle = me->GetVehicleKit())
                    {
                        if (Unit* passenger = vehicle->GetPassenger(1))
                        {
                            passenger->ExitVehicle();
                            me->Attack(passenger, true);

                            if (!passenger->HasAura(Spells::HelixRide))
                                passenger->AddAura(Spells::HelixRide, passenger);
                        }
                    }
                    me->RemoveAurasDueToSpell(Spells::RideOaf);
                    _ridingOaf = false;
                    if (me->GetVictim())
                        me->GetMotionMaster()->MoveChase(me->GetVictim());

                    // Spawn rats with random timing (3-5s between each)
                    // For achievement 5367 Rat Pack
                    if (me->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC)
                    {
                        for (uint8 i = 0; i < 8; ++i)
                        {
                            me->m_Events.AddEventAtOffset([this, i]()
                                {
                                    if (me && me->IsAlive())
                                        if (Creature* rat = me->SummonCreature(Creatures::MineRat, Positions::OafRatPos[i], TEMPSUMMON_CORPSE_DESPAWN))
                                            rat->GetMotionMaster()->MoveRandom(3.f);
                                }, std::chrono::milliseconds(urand(3000, 5000) * (i + 1)));
                        }
                    }

                    uint8 randomEvent = RAND(Events::OafThrowHelix, Events::OafCharge0);
                    _events.RescheduleEvent(randomEvent, 25s, 35s);
                    break;
                }
                case Events::OafThrowHelix:
                {
                    if (_ridingOaf)
                    {
                        _events.RescheduleEvent(Events::OafThrowHelix, 5s);
                        break;
                    }
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, false))
                    {
                        if (Vehicle* vehicle = me->GetVehicleKit())
                        {
                            if (Unit* helix = vehicle->GetPassenger(0))
                            {
                                _helixGUID = helix->GetGUID();
                                _helixFaceRiding = true;
                                ObjectGuid targetGUID = target->GetGUID();

                                helix->ToCreature()->AI()->Talk(Texts::HelixFaceRide);

                                me->m_Events.AddEventAtOffset([this, helixGUID = _helixGUID, targetGUID]()
                                    {
                                        if (!me || !me->IsAlive())
                                            return;

                                        if (Unit* helix = ObjectAccessor::GetUnit(*me, helixGUID))
                                        {
                                            Unit* target = ObjectAccessor::GetUnit(*me, targetGUID);
                                            if (target && target->IsAlive())
                                            {

                                                Position targetPos = target->GetPosition();

                                                if (target->IsPlayer())
                                                    me->CastSpell(target, Spells::HelixRide, true);
                                                else me->AddAura(Spells::HelixRide, target);

                                                helix->ExitVehicle(&targetPos);

                                                helix->m_Events.AddEventAtOffset([helix, targetGUID]()
                                                    {
                                                        helix->AddAura(Spells::HelixRideFaceTimerAura, helix);

                                                        if(Unit* target = ObjectAccessor::GetUnit(*helix, targetGUID))
                                                            helix->CastSpell(target, Spells::RideVehicle, true);
                                                    }, std::chrono::milliseconds(200));
                                            }
                                        }
                                        else _helixFaceRiding = false;
                                    }, std::chrono::seconds(2));

                                _events.RescheduleEvent(Events::OafPickupHelix, 12s);
                            }
                        }
                    }
                    break;
                }
                case Events::OafPickupHelix:
                {
                    if (Unit* helix = ObjectAccessor::GetUnit(*me, _helixGUID))
                    {
                        if (Vehicle* helixVehicle = helix->GetVehicle())
                        {
                            Unit* targetVehicle = helixVehicle->GetBase();
                            if (targetVehicle)
                            {
                                me->CastSpell(targetVehicle, Spells::ChestBomb, true);
                                helix->ToCreature()->AI()->Talk(Texts::HelixChestBomb, targetVehicle);

                                targetVehicle->m_Events.AddEventAtOffset([targetVehicle]()
                                    {
                                        if (targetVehicle && targetVehicle->IsAlive())
                                        {
                                            CastSpellExtraArgs args(TRIGGERED_FULL_MASK);
                                            args.AddSpellMod(SpellValueMod(SPELLVALUE_BASE_POINT0), 1500); // Override damage to ~1500
                                            targetVehicle->CastSpell(targetVehicle, Spells::ChestBombDMG, args);
                                        }
                                    }, std::chrono::seconds(10));
                            }
                        }

                        if (Vehicle* oafVehicle = me->GetVehicleKit())
                            helix->EnterVehicle(oafVehicle->GetBase(), 0);

                    }
                    _helixFaceRiding = false;

                    uint8 randomEvent = RAND(Events::OafThrowHelix, Events::OafCharge0);
                    _events.RescheduleEvent(randomEvent, 25s, 35s);
                    break;
                }
                default:
                    break;
                    
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        ObjectGuid _helixGUID;
        bool _ridingOaf = false;
        bool _helixFaceRiding = false;
    };

    // 47296 - Helix Gearbreaker
    struct boss_helix_gearbreaker : public BossAI
    {
        boss_helix_gearbreaker(Creature* creature) : BossAI(creature, DataTypes::BOSS_HELIX_GEARBREAKER), summons(me)
        {
            ApplyCrowdControlImmunities(creature);
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        void Reset() override
        {
            _events.Reset();
            summons.DespawnAll();
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            _oafDead = false;
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);

            Talk(Texts::HelixAggro);

            if (me->GetMap()->IsHeroic())
                _events.RescheduleEvent(Events::HelixSummonCrew, 1s);

            _events.RescheduleEvent(Events::HelixThrowBomb, 5s);
        }

        void JustReachedHome() override
        {
            BossAI::JustReachedHome();

            if(_oafDead)
                me->DespawnOrUnsummon();

            summons.DespawnAll();

            if (InstanceScript* instance = me->GetInstanceScript())
                instance->SetBossState(DataTypes::BOSS_HELIX_GEARBREAKER, NOT_STARTED);
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == Misc::HelixOafDead && value == 1)
            {
                _oafDead = true;
                me->RemoveAura(Spells::OafGuard);
                Talk(Texts::HelixOafDead);
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE);

                // If Helix is currently face riding a player, exit the vehicle immediately
                if (Vehicle* vehicle = me->GetVehicle())
                {
                    Unit* playerVehicle = vehicle->GetBase();
                    if (playerVehicle)
                    {
                        me->CastSpell(playerVehicle, Spells::ChestBomb, true);
                        Talk(Texts::HelixChestBomb, playerVehicle);
                    }
                    me->ExitVehicle();
                    _events.CancelEvent(Events::HelixExitFaceRide);
                }

                _events.RescheduleEvent(Events::HelixFaceRide, 10s, 15s);
            }
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            summons.Despawn(summon);
        }

        void KilledUnit(Unit* /*victim*/) override
        {
            Talk(Texts::HelixKill);
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            Talk(Texts::HelixDeath);
            summons.DespawnAll();
        }

        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (!aurApp)
                return;

            if (aurApp->GetBase()->GetId() == Spells::HelixRideFaceTimerAura && !_oafDead)
            {
                me->SetReactState(REACT_AGGRESSIVE);
                if (Vehicle* vehicle = me->GetVehicle())
                    if (Unit* base = vehicle->GetBase())
                        me->Attack(base, true);
            }
        }

        void OnAuraRemoved(AuraApplication const* aurApp) override
        {
            if (!aurApp)
                return;

            if (aurApp->GetBase()->GetId() == Spells::HelixRideFaceTimerAura && !_oafDead)
                me->SetReactState(REACT_PASSIVE);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::HelixSummonCrew:
                {
                    for (uint8 i = 0; i < 4; ++i)
                        me->SummonCreature(Creatures::HelixCrew, Positions::HelixCrewSpawn[i], TEMPSUMMON_MANUAL_DESPAWN);
                    break;
                }
                case Events::HelixThrowBomb:
                {
                    if (!_oafDead && !me->HasAura(Spells::HelixRideFaceTimerAura))
                    {
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, false))
                        {
                            Talk(Texts::HelixBombs);
                            me->CastSpell(target, Spells::ThrowBomb, true);
                        }
                        _events.RescheduleEvent(Events::HelixThrowBomb, 10s, 15s);
                    }
                    else if (_oafDead)
                    {
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, false))
                        {
                            Talk(Texts::HelixBombs);
                            me->CastSpell(target, Spells::ThrowBomb, true);
                        }
                        _events.RescheduleEvent(Events::HelixThrowBomb, 5s, 10s);
                    }
                    break;
                }
                case Events::HelixFaceRide:
                {
                    if (_oafDead)
                    {
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, false))
                        {
                            me->CastSpell(target, Spells::HelixLeap, true);

                            me->m_Events.AddEventAtOffset([this, targetGUID = target->GetGUID()]()
                                {
                                    if (!me || !me->IsAlive())
                                        return;

                                    if (Unit* target = ObjectAccessor::GetUnit(*me, targetGUID))
                                    {
                                        if (!target->IsAlive())
                                            return;

                                        if (target->IsPlayer())
                                            me->CastSpell(target, Spells::HelixRide, true);
                                        else target->AddAura(Spells::HelixRide, target);
                                        me->CastSpell(me, Spells::HelixRideFaceTimerAura, true);
                                        me->CastSpell(target, Spells::RideVehicle, true);
                                        _events.RescheduleEvent(Events::HelixExitFaceRide, 10s);
                                    }
                                }, std::chrono::seconds(1));
                        }
                    }
                    break;
                }
                case Events::HelixExitFaceRide:
                {
                    if (Vehicle* vehicle = me->GetVehicle())
                    {
                        Unit* targetVehicle = vehicle->GetBase();
                        if (targetVehicle)
                        {
                            me->CastSpell(targetVehicle, Spells::ChestBomb, true);
                            Talk(Texts::HelixChestBomb, targetVehicle);

                            targetVehicle->m_Events.AddEventAtOffset([targetVehicle]()
                                {
                                    if (targetVehicle && targetVehicle->IsAlive())
                                    {
                                        CastSpellExtraArgs args(TRIGGERED_FULL_MASK);
                                        args.AddSpellMod(SpellValueMod(SPELLVALUE_BASE_POINT0), 1500); // Override damage to ~1500
                                        targetVehicle->CastSpell(targetVehicle, Spells::ChestBombDMG, args);
                                    }
                                }, std::chrono::seconds(10));
                        }
                    }
                    me->ExitVehicle();
                    _events.RescheduleEvent(Events::HelixFaceRide, 15s, 20s);
                    break;
                }
                default:
                    break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        SummonList summons;
        bool _oafDead = false;
    };

    // 48284 - Mining Powder
    struct npc_mining_powder : public ScriptedAI
    {
        npc_mining_powder(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            ScriptedAI::Reset();
            _hasExploded = false;
            _attackerGUID.Clear();
            me->SetReactState(REACT_PASSIVE);
        }

        void DamageTaken(Unit* attacker, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (_hasExploded)
                return;

            _hasExploded = true;
            _attackerGUID = attacker->GetGUID();
            DoCastSelf(Spells::Explode);
            me->DespawnOrUnsummon(100ms);
        }

        ObjectGuid GetGUID(int32 id) const override
        {
            if (id == 1)
                return _attackerGUID;
            return ObjectGuid::Empty;
        }

    private:
        bool _hasExploded = false;
        ObjectGuid _attackerGUID;
    };

    // 48421 - Defias Overseer
    struct npc_defias_overseer : public ScriptedAI
    {
        npc_defias_overseer(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            ScriptedAI::Reset();
            _isController = false;
            _hasFled = false;
            _dialogueLine = 0;
            _events.Reset();

            // Delay controller determination to ensure both NPCs are spawned
            _events.RescheduleEvent(Events::OverseerInitController, 1s);
        }

        void JustEngagedWith(Unit* who) override
        {
            ScriptedAI::JustEngagedWith(who);
            _events.Reset();

            // Link combat with partner
            if (Creature* partner = me->FindNearestCreature(Creatures::DefiasOverseer, 20.0f, true))
                if (partner->IsAlive() && !partner->IsInCombat())
                    partner->AI()->AttackStart(who);
        }

        void JustReachedHome() override
        {
            ScriptedAI::JustReachedHome();

            // Check if partner is dead, if so respawn them
            if (Creature* partner = me->FindNearestCreature(Creatures::DefiasOverseer, 20.0f, false))
                if (!partner->IsAlive())
                    partner->Respawn(true);

            // Reschedule dialogue start after respawn
            _events.RescheduleEvent(Events::OverseerInitController, 10s);
        }

        void JustDied(Unit* killer) override
        {
            ScriptedAI::JustDied(killer);
            _events.Reset();
        }

        void DamageTaken(Unit* attacker, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (HealthBelowPct(15) && !_hasFled)
            {
                _hasFled = true;
                Talk(Texts::OverseerFlee);
                me->GetMotionMaster()->MoveFleeing(attacker, 10s);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case Events::OverseerInitController:
                            // Determine if this is the controller (lower GUID)
                            if (Creature* partner = me->FindNearestCreature(Creatures::DefiasOverseer, 20.0f, true))
                            {
                                _isController = (me->GetGUID().GetCounter() < partner->GetGUID().GetCounter());
                                if (_isController)
                                    _events.RescheduleEvent(Events::OverseerDialogueStart, 4s); // 1s already elapsed
                            }
                            break;
                        case Events::OverseerDialogueStart:
                            if (_isController)
                                StartDialogue();
                            break;
                        case Events::OverseerDialogueLine:
                            if (_isController)
                                SayNextDialogueLine();
                            break;
                        default:
                            break;
                    }
                }
                return;
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        void StartDialogue()
        {
            _dialogueLine = 0;
            SayNextDialogueLine();
        }

        void SayNextDialogueLine()
        {
            Creature* partner = me->FindNearestCreature(Creatures::DefiasOverseer, 20.0f, true);
            if (!partner || partner == me)
            {
                // Partner not available, restart dialogue after delay
                _events.RescheduleEvent(Events::OverseerDialogueStart, 30s, 45s);
                return;
            }

            if (_dialogueLine >= 10)
            {
                // Dialogue complete, restart after delay
                _dialogueLine = 0;
                _events.RescheduleEvent(Events::OverseerDialogueStart, 30s, 45s);
                return;
            }

            // Determine which overseer should speak (alternating)
            // Even lines (0, 2, 4, 6, 8) - this overseer speaks
            // Odd lines (1, 3, 5, 7, 9) - partner speaks
            Creature* speaker = (_dialogueLine % 2 == 0) ? me : partner;
            speaker->AI()->Talk(_dialogueLine);

            // Calculate delay based on text length
            uint32 delay = 3000; // default 3s
            switch (_dialogueLine)
            {
                case 0: delay = 3500; break; // "It's broken."
                case 1: delay = 3500; break; // "It's not broken!"
                case 2: delay = 5000; break; // "Why's it shooting steam..."
                case 3: delay = 5000; break; // "That's the...pressure release valve."
                case 4: delay = 4500; break; // "In the middle of the pipe?"
                case 5: delay = 4500; break; // "Er...backup release valve?"
                case 6: delay = 4000; break; // "We should tell the Admiral."
                case 7: delay = 3500; break; // "You tell him."
                case 8: delay = 4500; break; // "No way! He gives me the creeps!"
                case 9: delay = 7000; break; // "I know! The way he looks at you..."
                default: delay = 3000; break;
            }

            _dialogueLine++;
            _events.RescheduleEvent(Events::OverseerDialogueLine, std::chrono::milliseconds(delay));
        }

        bool _isController = false;
        int8 _dialogueLine = 0;
        bool _hasFled = false;
        EventMap _events;
    };

    // 48280, 48439 - Goblin Foundry Event (Engineer & Craftsman)
    struct npc_goblin_foundry_worker : public ScriptedAI
    {
        npc_goblin_foundry_worker(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            ScriptedAI::Reset();
            _events.Reset();
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == Misc::GoblinFoundryEventNPC)
            {
                _npcRole = value;
                _isEventNPC = true;
            }
        }

        uint32 GetData(uint32 id) const override
        {
            if (id == Misc::GoblinFoundryEventNPC)
                return _npcRole;
            return 0;
        }

        void DoAction(int32 action) override
        {
            if (!_isEventNPC)
                return;

            if (action == 1)
                StartFirstMovement();
        }

        void MovementInform(uint32 motionType, uint32 pointId) override
        {
            if (motionType != POINT_MOTION_TYPE)
                return;

            if (!_isEventNPC)
                return;

            switch (pointId)
            {
                case 1: // Reached first movement point (Craftsman 1 only)
                    if (_npcRole == 1)
                    {
                        Talk(Texts::GoblinCraftsmanAlive);
                        _events.RescheduleEvent(Events::GoblinWorkerRunToFinal, 7s);
                    }
                    break;
                case 2: // Reached GoblinMoveTo
                    OnReachMoveTo();
                    break;
                case 3: // Reached final destination
                    // NPC stays here until instance reset
                    break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!_isEventNPC)
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::GoblinWorkerStartMovement: // Start first movement after delay
                    if (_npcRole == 1)
                        me->GetMotionMaster()->MovePoint(1, Positions::GoblinCraftsman1MoveTo);
                    else if (_npcRole == 2 || _npcRole == 3 || _npcRole == 4)
                        me->GetMotionMaster()->MovePoint(2, Positions::GoblinMoveTo);
                    break;
                case Events::GoblinWorkerRunToFinal: // Run to final destination
                    me->GetMotionMaster()->MovePoint(3, GetRunToPosition());
                    break;
                case Events::GoblinWorkerCraftsmanNo: // Craftsman 2 SAY GoblinCraftsmanNo
                    Talk(Texts::GoblinCraftsmanNo);
                    _events.RescheduleEvent(Events::GoblinWorkerRunToFinal, 3s);
                    break;
                }
            }
        }

    private:
        void StartFirstMovement()
        {
            switch (_npcRole)
            {
            case 1:
            case 2:
                _events.RescheduleEvent(Events::GoblinWorkerStartMovement, 1s);
                break;
            case 3:
                _events.RescheduleEvent(Events::GoblinWorkerStartMovement, 2s);
                break;
            case 4:
                _events.RescheduleEvent(Events::GoblinWorkerStartMovement, 6s);
                break;
            default:
                break;
            }
        }

        void OnReachMoveTo()
        {
            // Role 2: Craftsman 2 - SAY GoblinCraftsmanYellAlive, then SAY GoblinCraftsmanNo 2s later, then run
            // Role 3: Engineer 1 - SAY GoblinEngineerRun, then immediately run to GoblinRunTo
            // Role 4: Engineer 2 - Run to GoblinRunTo
            if (_npcRole == 2)
            {
                Talk(Texts::GoblinCraftsmanYellAlive);
                _events.RescheduleEvent(Events::GoblinWorkerCraftsmanNo, 2s);
            }
            else if (_npcRole == 3)
            {
                Talk(Texts::GoblinEngineerRun);
                me->GetMotionMaster()->MovePoint(3, GetRunToPosition());
            }
            else if (_npcRole == 4)
                me->GetMotionMaster()->MovePoint(3, GetRunToPosition());
        }

        Position GetRunToPosition()
        {
            // Assign one of the 4 GoblinRunTo positions
            static const Position runToPositions[] =
            {
                Positions::GoblinRunTo1,
                Positions::GoblinRunTo2,
                Positions::GoblinRunTo3,
                Positions::GoblinRunTo4
            };

            // Use a simple hash of GUID to determine position
            uint32 index = me->GetGUID().GetCounter() % 4;
            return runToPositions[index];
        }

        bool _isEventNPC = false;
        uint32 _npcRole = 0;
        EventMap _events;
    };

    // 43778 - Foe Reaper 5000
    struct boss_foe_reaper_5000 : public BossAI
    {
        boss_foe_reaper_5000(Creature* creature) : BossAI(creature, DataTypes::BOSS_FOE_REAPER_5000), _summons(creature)
        {
            ApplyCrowdControlImmunities(creature);
        }

        void Reset() override
        {
            BossAI::Reset();

            _events.Reset();
            _summons.DespawnAll();
            _safetyOfflineCast = false;
            _harvestActive = false;
            instance->SetData(Misc::FoeReaperAddCounter, 0); // Reset counter for starting encounter

            if (IsHeroic())
                me->SummonCreature(Creatures::PrototypeReaper, Positions::PrototypeSpawn, TEMPSUMMON_MANUAL_DESPAWN);

            me->SummonCreature(Creatures::DefiasWatcher, Positions::FoeReaper5000AddSpawns[0], TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(Creatures::DefiasReaper,  Positions::FoeReaper5000AddSpawns[1], TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(Creatures::DefiasWatcher, Positions::FoeReaper5000AddSpawns[2], TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(Creatures::DefiasReaper,  Positions::FoeReaper5000AddSpawns[3], TEMPSUMMON_MANUAL_DESPAWN);
        }

        void JustSummoned(Creature* summon) override
        {
            _summons.Summon(summon);
        }

        void JustReachedHome() override
        {
            me->AddAura(Spells::Offline, me);
            me->RemoveAurasDueToSpell(Spells::RedEyes);
            me->SetUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_STUNNED));
        }

        void DoAction(int32 action) override
        {
            if (action == 1)
            {
                _events.ScheduleEvent(Events::FoeReaperInit, 500ms);
                _events.ScheduleEvent(Events::FoeReaperAggro, 4s);
            }
        }

        void OnSpellCast(SpellInfo const* spellInfo) override
        {
            if(spellInfo && spellInfo->Id == Spells::Harvest)
                me->GetMotionMaster()->MovePoint(0, _harvestPos);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::FoeReaperInit:
                        me->CastSpell(me, Spells::FoeEnergize, true);
                        me->RemoveAura(Spells::Offline);
                        me->AddAura(Spells::RedEyes, me);
                        me->RemoveUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_STUNNED));
                        Talk(Texts::FoeReaperInit);
                        break;
                    case Events::FoeReaperAggro:
                        Talk(Texts::FoeReaperAggro);
                        if (Unit* victim = SelectTarget(SelectTargetMethod::Random, 0, 150.f, false))
                            me->Attack(victim, true);
                        _events.ScheduleEvent(Events::FoeReaperReaperStrike, 8s);
                        if(IsHeroic())
                            _events.ScheduleEvent(Events::FoeReaperMoltenSlagWarning, 8s);
                        _events.ScheduleEvent(Events::FoeReaperOverdriveWarning, 4s);
                        _events.ScheduleEvent(Events::FoeReaperHarvest, 23s);
                        break;
                    case Events::FoeReaperReaperStrike:
                        if (Unit* victim = me->GetVictim())
                            me->CastSpell(victim, Spells::ReaperStrike);
                        _events.RescheduleEvent(Events::FoeReaperReaperStrike, 5s, 7s);
                        break;
                    case Events::FoeReaperMoltenSlagWarning:
                        Talk(Texts::FoeReaperMoltenSlag);
                        _events.ScheduleEvent(Events::FoeReaperMoltenSlag, 5s);
                        break;
                    case Events::FoeReaperMoltenSlag:
                        if (IsHeroic())
                        {
                            uint32 randomIndex = urand(0, 3);
                            me->CastSpell(Positions::MoltenSlagSpawn[randomIndex], Spells::FoeMoltenSlag);
                        }
                        _events.ScheduleEvent(Events::FoeReaperMoltenSlagWarning, 15s);
                        break;
                    case Events::FoeReaperOverdriveWarning:
                        Talk(Texts::FoeReaperOverdriveWarning);
                        _events.ScheduleEvent(Events::FoeReaperOverdrive, 5s);
                        break;
                    case Events::FoeReaperOverdrive:
                        me->CastSpell(me, Spells::Overdrive, true);
                        Talk(Texts::FoeReaperOverdrive);
                        _events.ScheduleEvent(Events::FoeReaperOverdriveWarning, 25s);
                        _events.ScheduleEvent(Events::FoeReaperOverdriveSwitchTarget, 1500ms);
                        break;
                    case Events::FoeReaperOverdriveSwitchTarget:
                        if (me->HasAura(Spells::Overdrive))
                        {
                            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 150.0f, false))
                                me->Attack(target, true);
                            _events.ScheduleEvent(Events::FoeReaperOverdriveSwitchTarget, 1500ms);
                        }
                        break;
                    case Events::FoeReaperHarvest:
                        Talk(Texts::FoeReaperHarvest);
                        if(Unit* target  = GetHarvestTarget())
                        {
                            _harvestActive = true;
                            _harvestPos = target->GetPosition();
                            me->CastSpell(_harvestPos, Spells::Harvest);
                        }
                        break;
                    case Events::FoeReaperSafetyOfflineWarning:
                        Talk(Texts::FoeReaperSafetyOffWarning);
                        _events.ScheduleEvent(Events::FoeReaperSafetyOffline, 5s);
                        break;
                    case Events::FoeReaperSafetyOffline:
                        me->CastSpell(me, Spells::SafetyOffline, true);
                        Talk(Texts::FoeReaperSafetyOff);
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            // Fix needed if we have bots that are reviving at dungeon entrance and boss still in combat
            // This will force the boss to reset
            if (Unit* victim = me->GetVictim())
                if (victim && victim->IsBot())
                    if (me->GetDistance2d(victim) > 200.f)
                        me->GetThreatManager().ResetThreat(victim);

            // Add melee here if needed
        }

        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == Spells::Overdrive)
                me->SetSpeedRate(MOVE_RUN, 2.0f);
        }

        void OnAuraRemoved(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == Spells::Overdrive)
                me->SetSpeedRate(MOVE_RUN, 1.0f);
        }

        void MovementInform(uint32 motionType, uint32 pointId) override
        {
            if (motionType == POINT_MOTION_TYPE && pointId == 0 && _harvestActive)
            {
                me->StopMoving();
                me->RemoveAurasDueToSpell(Spells::Harvest);
                me->CastSpell(me, Spells::HarvestSweep, false);
                Talk(Texts::FoeReaperHarvestSweep);
                _harvestActive = false;
                _events.ScheduleEvent(Events::FoeReaperHarvest, 30s);
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->ToUnit() && victim->GetEntry() == Creatures::GeneralPurposeBunny)
                return;

            Talk(Texts::FoeReaperKill, victim);
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            _events.Reset();
            Talk(Texts::FoeReaperDeath);
            _summons.DespawnAll();
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!_safetyOfflineCast && me->HealthBelowPctDamaged(25, damage))
            {
                _safetyOfflineCast = true;
                _events.ScheduleEvent(Events::FoeReaperSafetyOfflineWarning, 0s);
            }
        }

    private:
        bool _safetyOfflineCast = false;
        bool _harvestActive = false;
        EventMap _events;
        SummonList _summons;
        Position _harvestPos;

        Unit* GetHarvestTarget()
        {
            // 1) Try to select a target at least 10 yards away
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, -10.0f, false))
            {
                if (me->GetDistance(target) >= 10.0f)
                    return target;
            }

            // 2) Fallback: find a specific NPC in the room
            if (Creature* fallback = me->FindNearestCreature(45979, 100.0f))
                return fallback;

            // 3) Last resort: return nullptr
            return nullptr;
        }
    };

    // 47403, 47404 - Defias Reaper & Watcher (Foe Reaper 5000 adds)
    struct npc_defias_foe_reaper_add : public ScriptedAI
    {
        npc_defias_foe_reaper_add(Creature* creature) : ScriptedAI(creature)
        {
            creature->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FIRE, true);
            me->RemoveNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
        }

        void Reset() override
        {
            ScriptedAI::Reset();
            _events.Reset();
            _energizeCast = false;
            _onFireCast = false;
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            if (_onFireCast)
                return;
            ScriptedAI::EnterEvadeMode();
        }

        void IncrementFoeReaperAddCounter()
        {
            if (InstanceScript* instance = me->GetInstanceScript())
            {
                uint8 currentCount = instance->GetData(Misc::FoeReaperAddCounter);
                currentCount++;
                instance->SetData(Misc::FoeReaperAddCounter, currentCount);

                if (currentCount == 4)
                {
                    ObjectGuid bossGUID = instance->GetGuidData(DataTypes::BOSS_FOE_REAPER_5000);

                    if (Creature* boss = ObjectAccessor::GetCreature(*me, bossGUID))
                        boss->AI()->DoAction(1);
                }
            }
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!_energizeCast && me->HealthBelowPctDamaged(60, damage))
            {
                _energizeCast = true;
                me->CastSpell(me, Spells::Energized, true);
            }

            if (!_onFireCast && me->HealthBelowPctDamaged(15, damage))
            {
                me->SetRegenerateHealth(false);
                damage = 0;
                me->SetHealth(me->GetMaxHealth() * 0.15);
                _onFireCast = true;
                IncrementFoeReaperAddCounter();
                me->CombatStop(true);
                me->ClearInCombat();
                me->RemoveAllAuras();
                me->CastSpell(me, Spells::OnFire, true);
                me->SetFaction(35);
                me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);

                if (Creature* foeReaper = me->FindNearestCreature(Creatures::FoeReaper5000, 100.0f))
                    me->CastSpell(foeReaper, Spells::Energize);

                _events.ScheduleEvent(Events::FoeReaperAddDeath, 30s);
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.RescheduleEvent(Events::FoeReaperAddCleave, randtime(7s, 9s));
            _events.RescheduleEvent(Events::FoeReaperAddWatch, 4s);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::FoeReaperAddCleave:
                {
                    if (!UpdateVictim())
                        return;

                    if (Unit* victim = me->GetVictim())
                        if (me->IsWithinDist(victim, 5.0f))
                            me->CastSpell(victim, Spells::Cleave, false);
                    _events.RescheduleEvent(Events::FoeReaperAddCleave, randtime(7s, 9s));
                    break;
                }
                case Events::FoeReaperAddWatch:
                {
                    if (!UpdateVictim())
                        return;

                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, false))
                        me->CastSpell(target, Spells::Watch, false);
                    _events.RescheduleEvent(Events::FoeReaperAddWatch, randtime(12s, 14s));
                    break;
                }
                case Events::FoeReaperAddDeath:
                    me->KillSelf();
                    break;
                }
            }

            if (UpdateVictim())
                me->DoMeleeAttackIfReady();
        }

    private:
        bool _energizeCast = false;
        bool _onFireCast = false;
        EventMap _events;
    };
}

void AddSC_custom_deadmines_npcs()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    RegisterCreatureAI(boss_glubtok);
    RegisterCreatureAI(npc_glubtok_main_platter);
    RegisterCreatureAI(npc_glubtok_secondary_platter);
    RegisterCreatureAI(npc_ogre_henchman);
    RegisterCreatureAI(npc_ogre_bodyguard);
    RegisterCreatureAI(npc_helix_crew);
    RegisterCreatureAI(npc_sticky_bomb);
    RegisterCreatureAI(npc_lumbering_oaf);
    RegisterCreatureAI(boss_helix_gearbreaker);
    RegisterCreatureAI(npc_mining_powder);
    RegisterCreatureAI(npc_defias_overseer);
    RegisterCreatureAI(npc_goblin_foundry_worker);
    RegisterCreatureAI(boss_foe_reaper_5000);
    RegisterCreatureAI(npc_defias_foe_reaper_add);
}
