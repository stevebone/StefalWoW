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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "MotionMaster.h"
#include "MoveSplineInit.h"
#include "ObjectMgr.h"
#include "PassiveAI.h"
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
            _lastElement = false; // false = Flame, true = Frost
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
                    ObjectGuid glubtokGUID = me->GetGUID();
                    dummy->m_Events.AddEventAtOffset([dummy, glubtokGUID, spellID]()
                    {
                        if (dummy && dummy->IsInWorld())
                        {
                            if (Unit* glubtok = ObjectAccessor::GetUnit(*dummy, glubtokGUID))
                                dummy->CastSpell(glubtok, spellID, true);
                        }
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
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
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
        bool _lastElement = false;
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
                if (other && other != me && other->IsAlive())
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
        }

        void Reset() override
        {
            ScriptedAI::Reset();
            summons.DespawnAll();
            SetCombatMovement(false);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.RescheduleEvent(Events::HelixCrewStickyBomb, 8s);
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
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                        {
                            Talk(Texts::HelixCrewStickyBomb);
                            me->SummonCreature(Creatures::StickyBomb, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN);
                        }
                        _events.RescheduleEvent(Events::HelixCrewStickyBomb, 18s);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
        SummonList summons;
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
                        DoCastSelf(Spells::ChestBomb);
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
                        DoCastSelf(me->GetMap()->IsHeroic() ? Spells::StickyBombExplodeHeroic : Spells::StickyBombExplode);
                        me->DespawnOrUnsummon(5s);
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

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.RescheduleEvent(Events::OafCharge0, 15s, 20s);
            _events.RescheduleEvent(Events::OafThrowHelix, 15s, 20s);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            me->DespawnOrUnsummon();
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
                    ai->SetData(0, 0);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE)
            {
                if (id == 1)
                    _events.RescheduleEvent(Events::OafCharge2, 1500ms);
                else if (id == 2)
                    _events.RescheduleEvent(Events::OafCharge3, 1500ms);
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
                    case Events::OafCharge1:
                        if (Creature* moveBunny = me->GetMap()->GetCreatureBySpawnId(CreatureSpawns::OafMoveToBunny))
                            me->GetMotionMaster()->MovePoint(1, moveBunny->GetPosition());
                        break;
                    case Events::OafCharge2:
                        if (Creature* chargeBunny = me->GetMap()->GetCreatureBySpawnId(CreatureSpawns::OafChargePosition))
                            me->GetMotionMaster()->MoveCharge(chargeBunny->GetPositionX(), chargeBunny->GetPositionY(), chargeBunny->GetPositionZ(), 42.0f, 2);
                        break;
                    case Events::OafCharge3:
                        DoCastAOE(me->GetMap()->IsHeroic() ? Spells::OafSmashHeroic : Spells::OafSmash);
                        Talk(Texts::OafSmash);
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (Vehicle* vehicle = me->GetVehicleKit())
                        {
                            if (Unit* passenger = vehicle->GetPassenger(1))
                            {
                                passenger->ExitVehicle();
                                me->Attack(passenger, true);
                            }
                        }
                        me->RemoveAurasDueToSpell(Spells::RideOaf);
                        _ridingOaf = false;
                        if (me->GetVictim())
                            me->GetMotionMaster()->MoveChase(me->GetVictim());
                        _events.RescheduleEvent(Events::OafCharge0, 30s);
                        break;
                    case Events::OafThrowHelix:
                        if (_ridingOaf)
                        {
                            _events.RescheduleEvent(Events::OafThrowHelix, 5s);
                            break;
                        }
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                        {
                            if (Vehicle* vehicle = me->GetVehicleKit())
                            {
                                if (Unit* helix = vehicle->GetPassenger(0))
                                {
                                    target->CastSpell(target, Spells::HelixRide, true);

                                    _helixGUID = helix->GetGUID();

                                    if (Vehicle* playerVehicle = target->GetVehicleKit())
                                        helix->EnterVehicle(playerVehicle->GetBase(), 0);
                                    _helixFaceRiding = true;
                                    _events.RescheduleEvent(Events::OafPickupHelix, 10s);
                                }
                            }
                        }
                        break;
                    case Events::OafPickupHelix:
                        if (Unit* helix = ObjectAccessor::GetUnit(*me, _helixGUID))
                        {
                            if (Vehicle* oafVehicle = me->GetVehicleKit())
                                helix->EnterVehicle(oafVehicle->GetBase(), 0);
                        }
                        _helixFaceRiding = false;
                        _events.RescheduleEvent(Events::OafThrowHelix, 20s, 35s);
                        break;
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
    RegisterCreatureAI(npc_mining_powder);
}
