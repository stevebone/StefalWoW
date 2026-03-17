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
#include "Creature.h"
#include "CreatureAI.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PetAI.h"
#include "TemporarySummon.h"
#include "Spell.h"
#include <cmath>

#include "Custom_Warlock_Defines.h"

class npc_pet_warlock_wild_imp : public CreatureScript
{
public:
    npc_pet_warlock_wild_imp() : CreatureScript("npc_pet_warlock_wild_imp") { }

    // Wild Imp - 55659
    struct npc_pet_warlock_wild_impAI : public PetAI
    {
        npc_pet_warlock_wild_impAI(Creature* creature) : PetAI(creature)
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
            if (roll_chance_i(30))
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
                    me->CastSpell(
                        me,
                        SPELL_WARLOCK_IMPLOSION_DAMAGE,
                        CastSpellExtraArgs(TRIGGERED_FULL_MASK)
                        .SetOriginalCaster(owner->GetGUID()));
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

            me->CastSpell(target, SPELL_WARLOCK_FEL_FIREBOLT,
                CastSpellExtraArgs(TRIGGERED_NONE)
                .SetOriginalCaster(owner->GetGUID()));
        }

        void UpdateOwnerAura(int8 change)
        {
            if (Unit* owner = me->GetOwner())
            {
                if (change > 0)
                {
                    if (Aura* aura = owner->GetAura(SPELL_WARLOCK_WILD_IMP_COUNTER))
                        aura->ModStackAmount(change);
                    else
                        owner->CastSpell(owner, SPELL_WARLOCK_WILD_IMP_COUNTER, true); // apply first stack
                }
                else
                {
                    if (Aura* aura = owner->GetAura(SPELL_WARLOCK_WILD_IMP_COUNTER))
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_pet_warlock_wild_impAI(creature);
    }
};

class npc_pet_warlock_demonic_tyrant : public CreatureScript
{
public:
    npc_pet_warlock_demonic_tyrant() : CreatureScript("npc_pet_warlock_demonic_tyrant") { }

    // Demonic Tyrant - 135002/250289 // new version added in Midnight
    struct npc_pet_warlock_demonic_tyrantAI : public PetAI
    {
        npc_pet_warlock_demonic_tyrantAI(Creature* creature) : PetAI(creature)
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

            if (owner->HasAura(SPELL_WARLOCK_ANTORAN_ARMAMENTS) && me->GetEntry() == NPC_WARLOCK_DEMONIC_TYRANT1)
                me->DespawnOrUnsummon();

            if (!owner->HasAura(SPELL_WARLOCK_ANTORAN_ARMAMENTS) && me->GetEntry() == NPC_WARLOCK_DEMONIC_TYRANT2)
                me->DespawnOrUnsummon();

            uint32 impCount = 0;
            uint32 dreadCount = 0;

            if (owner)
            {
                if (Aura* aura = owner->GetAura(SPELL_WARLOCK_WILD_IMP_COUNTER))
                    impCount = aura->GetStackAmount();

                if (Player* player = owner->ToPlayer())
                {
                    for (Unit* controlled : player->m_Controlled)
                    {
                        if (!controlled)
                            continue;

                        if (controlled->GetEntry() == NPC_WARLOCK_DREADSTALKER) // Dreadstalker
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

        void JustDied(Unit* /*killer*/) override
        {
        }

        void OnDespawn() override
        {
        }

        void JustEnteredCombat(Unit* /*who*/) override
        {
        }

        void MovementInform(uint32 /*type*/, uint32 /*id*/) override
        {
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

            me->CastSpell(target, SPELL_WARLOCK_TYRANT_DEMONFIRE,
                CastSpellExtraArgs(TRIGGERED_NONE)
                .SetOriginalCaster(owner->GetGUID()));
        }

        // Reign of Tyranny duration extension
        void HandleReignOfTyrany()
        {
            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            if (owner->HasAura(SPELL_WARLOCK_REIGN_OF_TYRANNY))
            {
                if (TempSummon* summon = me->ToTempSummon())
                {
                    auto remaining = summon->GetTimer(); // current despawn timer

                    summon->DespawnOrUnsummon(remaining + 5s);

                    owner->CastSpell(owner, SPELL_WARLOCK_TYRANT_OBLATION);
                }
            }
        }

        void HandleDemonicConsumption()
        {
            Unit* owner = me->GetOwner();
            if (!owner)
                return;

            // Talent check
            if (!owner->HasAura(SPELL_WARLOCK_DEMONIC_CONSUMPTION)) // Demonic Consumption
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
                    if (entry == NPC_WARLOCK_INFERNAL || entry == NPC_WARLOCK_DOOMGUARD)
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

            owner->CastSpell(owner, SPELL_WARLOCK_DEMONIC_CONSUMPTION_BUFF);

            // Convert drained HP into Tyrant damage bonus
            float bonusPct = float(drainedHealth) / float(me->GetMaxHealth());

            me->SetStatPctModifier(
                UNIT_MOD_DAMAGE_MAINHAND,
                TOTAL_PCT,
                1.0f + bonusPct
            );
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_pet_warlock_demonic_tyrantAI(creature);
    }
};

void AddSC_custom_warlock_demon_npcs()
{
    new npc_pet_warlock_wild_imp();
    new npc_pet_warlock_demonic_tyrant();
}
