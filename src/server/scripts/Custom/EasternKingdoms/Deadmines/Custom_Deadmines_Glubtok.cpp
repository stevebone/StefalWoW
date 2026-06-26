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

#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "PassiveAI.h"
#include "MotionMaster.h"
#include "MoveSplineInit.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Map.h"
#include "Vehicle.h"
#include "GridNotifiers.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "TemporarySummon.h"
#include "G3D/Vector3.h"
#include <list>
#include <vector>

#include "Custom_Instance_Deadmines.h"

namespace Scripts::EasternKingdoms::Deadmines
{
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
            if (victim != me)
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
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, false))
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
                    events.RescheduleEvent(Events::GlubtokCastFistOfFlame, 1s);
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
}

void AddSC_custom_deadmines_glubtok()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    RegisterCreatureAI(boss_glubtok);
    RegisterCreatureAI(npc_glubtok_main_platter);
    RegisterCreatureAI(npc_glubtok_secondary_platter);
}
