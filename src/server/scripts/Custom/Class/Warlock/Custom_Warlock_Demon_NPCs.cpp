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
#include "CellImpl.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GridNotifiersImpl.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PetAI.h"
#include "TemporarySummon.h"
#include "Spell.h"
#include <cmath>

#include "Custom_Warlock_Defines.h"

namespace Scripts::Custom::Warlock
{
    struct npc_warlock_dreadstalker : public ScriptedAI
    {
        npc_warlock_dreadstalker(Creature* creature) : ScriptedAI(creature) {}

        bool firstTick = true;

        void UpdateAI(uint32 /*diff*/) override
        {
            if (firstTick)
            {
                Unit* owner = me->GetOwner();
                if (!me->GetOwner() || !me->GetOwner()->ToPlayer())
                    return;

                me->SetMaxHealth(owner->CountPctFromMaxHealth(40));
                me->SetHealth(me->GetMaxHealth());

                if (Unit* target = owner->ToPlayer()->GetSelectedUnit())
                    me->CastSpell(target, Spells::DreadstalkersCharge, true);

                firstTick = false;

                me->CastSpell(me, Spells::SharpenedDreadfangs, true);
            }

            if (!UpdateVictim())
                return;

            me->DoMeleeAttackIfReady();
        }
    };

    // Wild Imp - 55659
    struct npc_pet_warlock_wild_imp : public PetAI
    {
        npc_pet_warlock_wild_imp(Creature* creature) : PetAI(creature)
        {
            if (Unit* owner = me->GetOwner())
            {
                me->SetLevel(owner->GetLevel());
                me->SetMaxHealth(owner->GetMaxHealth() / 3);
                me->SetHealth(owner->GetHealth() / 3);
            }
        }

        void JustAppeared() override
        {
            UpdateOwnerAura(1);

            if (Unit* owner = me->GetOwner())
            {
                float angle = frand(0.0f, float(M_PI) * 2);
                me->GetMotionMaster()->MoveFollow(owner, 2.5f, angle);
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            UpdateOwnerAura(-1);
        }

        void OnDespawn() override
        {
            UpdateOwnerAura(-1);
        }

        void JustEnteredCombat(Unit* /*who*/) override
        {
            // 30% chance to play a sound
            if (roll_chance(30))
            {
                static constexpr uint32 impSounds[] = {
                    55202, 55203, 55204, 55205, 55206, 55207, 55208, 55209, 55210
                };

                uint32 randomIndex = urand(0, std::size(impSounds) - 1);
                uint32 soundId = impSounds[randomIndex];

                me->PlayDirectSound(soundId);
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != EFFECT_MOTION_TYPE)
                return;

            if (id == EVENT_JUMP) // IMPLOSION_JUMP
            {
                if (Unit* owner = me->GetOwner())
                {
                    me->CastSpell(me, Spells::ImplosionDamage, CastSpellExtraArgs(TRIGGERED_FULL_MASK) .SetOriginalCaster(owner->GetGUID()));
                }

                UpdateOwnerAura(-1);
                me->DisappearAndDie();
            }
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            if (!me->GetVictim())
            {
                if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
                {
                    float angle = frand(0.0f, float(M_PI) * 2);
                    me->GetMotionMaster()->MoveFollow(owner, 2.5f, angle);
                }
                return;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            Unit* target = ObjectAccessor::GetUnit(*me, owner->GetTarget());
            if (!target || !me->IsValidAttackTarget(target))
                return;

            me->CastSpell(target, Spells::FelFirebolt, CastSpellExtraArgs(TRIGGERED_NONE) .SetOriginalCaster(owner->GetGUID()));
        }

        void UpdateOwnerAura(int8 change)
        {
            if (Unit* owner = me->GetOwner())
            {
                if (change > 0)
                {
                    if (Aura* aura = owner->GetAura(Spells::WildImpCounter))
                        aura->ModStackAmount(change);
                    else
                        owner->CastSpell(owner, Spells::WildImpCounter, true); // apply first stack
                }
                else
                {
                    if (Aura* aura = owner->GetAura(Spells::WildImpCounter))
                    {
                        if (aura->GetStackAmount() > 1)
                            aura->ModStackAmount(change);
                        else
                            aura->Remove();
                    }
                }
            }
        }
    };

    // Demonic Tyrant - 135002/250289 // new version added in Midnight
    struct npc_pet_warlock_demonic_tyrant : public PetAI
    {
        npc_pet_warlock_demonic_tyrant(Creature* creature) : PetAI(creature)
        {
            if (Unit* owner = me->GetOwner())
            {
                me->SetLevel(owner->GetLevel());
                me->SetMaxHealth(owner->GetMaxHealth() / 3);
                me->SetHealth(owner->GetHealth() / 3);
            }
        }

        void JustAppeared() override
        {
            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            if (owner->HasAura(Spells::AntoranArmaments) && me->GetEntry() == Creatures::WarlockDemonicTyrant1)
                me->DespawnOrUnsummon();

            if (!owner->HasAura(Spells::AntoranArmaments) && me->GetEntry() == Creatures::WarlockDemonicTyrant2)
                me->DespawnOrUnsummon();

            uint32 impCount = 0;
            uint32 dreadCount = 0;

            if (owner)
            {
                if (Aura* aura = owner->GetAura(Spells::WildImpCounter))
                    impCount = aura->GetStackAmount();

                if (Player* player = owner->ToPlayer())
                {
                    for (Unit* controlled : player->m_Controlled)
                    {
                        if (!controlled)
                            continue;

                        if (controlled->GetEntry() == Creatures::WarlockDreadstalker) // Dreadstalker
                            dreadCount++;
                    }
                }
            }

            float bonusPct = (impCount + dreadCount) * 10.0f;
            me->SetStatPctModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, 1.0f + bonusPct / 100.0f);

            HandleDemonicConsumption();
            HandleReignOfTyrany();

            if (Unit* owner = me->GetOwner())
            {
                float angle = frand(0.0f, float(M_PI) * 2);
                me->GetMotionMaster()->MoveFollow(owner, 2.5f, angle);
            }
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            uint32 spellId = Spells::TyrantsDemonfire;

            // Burning Cleave is currently not working
            //if (me->GetEntry() == NPC_WARLOCK_DEMONIC_TYRANT2)
            //    spellId = SPELL_WARLOCK_TYRANT_BURNING_CLEAVE;

            if (!me->GetVictim())
            {
                if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
                {
                    float angle = frand(0.0f, float(M_PI) * 2);
                    me->GetMotionMaster()->MoveFollow(owner, 2.5f, angle);
                }
                return;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            Unit* target = ObjectAccessor::GetUnit(*me, owner->GetTarget());
            if (!target || !me->IsValidAttackTarget(target))
                return;

            me->CastSpell(target, spellId, CastSpellExtraArgs(TRIGGERED_NONE) .SetOriginalCaster(owner->GetGUID()));
        }

        // Reign of Tyranny duration extension
        void HandleReignOfTyrany()
        {
            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            if (owner->HasAura(Spells::ReignOfTyranny))
            {
                if (TempSummon* summon = me->ToTempSummon())
                {
                    auto remaining = summon->GetTimer(); // current despawn timer
                    summon->DespawnOrUnsummon(remaining + 5s);
                    owner->CastSpell(owner, Spells::TyrantsOblation);
                }
            }
        }

        void HandleDemonicConsumption()
        {
            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            // Talent check
            if (!owner->HasAura(Spells::DemonicConsumption)) // Demonic Consumption
                return;

            uint32 drainedHealth = 0;

            if (Player* player = owner->ToPlayer())
            {
                for (Unit* demon : player->m_Controlled)
                {
                    if (!demon)
                        continue;

                    if (demon == me)
                        continue;

                    if (!demon->IsAlive())
                        continue;

                    uint32 entry = demon->GetEntry();

                    // Skip doomguard and infernal
                    if (entry == Creatures::WarlockInfernal || entry == Creatures::WarlockDoomguard)
                        continue;

                    uint32 hp = demon->GetHealth();
                    uint32 drain = uint32(hp * 0.15f);

                    if (drain == 0)
                        continue;

                    // Visual beam to Tyrant
                    //demon->CastSpell(me, 267218, true);

                    // Drain HP
                    demon->ModifyHealth(-int32(drain));
                    drainedHealth += drain;
                }
            }

            if (drainedHealth == 0)
                return;

            owner->CastSpell(owner, Spells::DemonicConsumptionBuff);

            // Convert drained HP into Tyrant damage bonus
            float bonusPct = float(drainedHealth) / float(me->GetMaxHealth());

            me->SetStatPctModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, 1.0f + bonusPct);
        }
    };

    struct npc_warl_demonic_gateway : public CreatureAI
    {
        npc_warl_demonic_gateway(Creature* creature) : CreatureAI(creature) {}

        bool firstTick = true;

        void UpdateAI(uint32 /*diff*/) override
        {
            if (firstTick && me->IsInWorld())
            {
                me->CastSpell(me, Spells::DemonicGatewayVisual, true);
                me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
                me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
                me->SetReactState(REACT_PASSIVE);
                me->SetControlled(true, UNIT_STATE_ROOT);

                firstTick = false;
            }
        }

        void OnSpellClick(Unit* player, bool /*result*/) override
        {
            if (!player)
                return;

            uint32 aurasToCheck[4] = { 121164, 121175, 121176, 121177 };
            for (uint32 auraId : aurasToCheck)
                if (player->HasAura(auraId))
                    return;

            TeleportTarget(player, true);
        }

        void TeleportTarget(Unit* target, bool allowAnywhere)
        {
            if (!target)
                return;

            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            if (!allowAnywhere && me->GetDistance2d(target) > 3.0f)
                return;
            if (target->HasAura(Spells::DemonicGatewayDebuff))
                return;
            if (!target->IsInRaidWith(owner) && target != owner)
                return;
            if (!target->CanFreeMove())
                return;

            uint32 otherGatewayEntry = me->GetEntry() == Creatures::DemonicGatewayGreen
                ? Creatures::DemonicGatewayPurple
                : Creatures::DemonicGatewayGreen;
            uint32 teleportSpell = me->GetEntry() == Creatures::DemonicGatewayGreen
                ? Spells::DemonicGatewayJumpGreen
                : Spells::DemonicGatewayJumpPurple;

            std::vector<Creature*> gateways;
            GetOwnedGateways(owner, gateways);

            for (Creature* gateway : gateways)
            {
                if (gateway->GetEntry() != otherGatewayEntry)
                    continue;

                target->CastSpell(gateway, teleportSpell, true);
                break;
            }
        }

        static void GetOwnedGateways(Unit* owner, std::vector<Creature*>& out)
        {
            Trinity::AnyUnitInObjectRangeCheck checker(owner, 200.f);
            Trinity::CreatureListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(owner, out, checker);
            Cell::VisitAllObjects(owner, searcher, 200.0f);

            out.erase(
                std::remove_if(out.begin(), out.end(),
                    [owner](Creature* c)
                    {
                        return !c ||
                            (c->GetEntry() != Creatures::DemonicGatewayGreen &&
                                c->GetEntry() != Creatures::DemonicGatewayPurple) ||
                            c->GetOwnerGUID() != owner->GetGUID();
                    }),
                out.end());
        }
    };
}

void AddSC_custom_warlock_demon_npcs()
{
    using namespace Scripts::Custom::Warlock;

    RegisterCreatureAI(npc_warlock_dreadstalker);
    RegisterCreatureAI(npc_pet_warlock_wild_imp);
    RegisterCreatureAI(npc_pet_warlock_demonic_tyrant);
    RegisterCreatureAI(npc_warl_demonic_gateway);
}
