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

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Unit.h"

#include "Custom_VaultWardens_Defines.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    // 191029 - Throwing Blade (warden SAIs cast it while out of melee range)
    // Both DUMMY effects hold a blade-missile spell id in their base points
    // (Effect 0 -> 190898, Effect 1 -> 191026, both TRIGGER_MISSILE at a unit
    // target). Retail throws ONE blade per target - the sniffed pairs are
    // always 191029 + a single variant, never both - so effect 0 rolls the
    // pick and effect 1's dummy does nothing.
    class spell_vault_throwing_blade : public SpellScript
    {
        void CastBladeVariant(SpellEffIndex /*effIndex*/)
        {
            if (Unit* target = GetHitUnit())
                GetCaster()->CastSpell(target, urand(0, 1) ? Spells::ThrowingBladeMissile : Spells::ThrowingBladeMissileAlt, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_vault_throwing_blade::CastBladeVariant, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    // 133508 - Fel Infusion (quest 38689 soul-absorb aura, applied on accept)
    // Sniffed chain: dead demons release souls (their own 133578 death cast),
    // the player then casts 202154 Fill Fel which parents one 133511 kill
    // credit per absorbed soul - bursts of ~8-13 while packs died nearby.
    // Aura heartbeat is 5.2s; skipping one tick spaces the sweeps to ~10s,
    // matching the sniffed Fill Fel cadence (~10-25s). _absorbedSouls keeps
    // each corpse from yielding more than one fragment for this player.
    class spell_vault_fel_infusion : public AuraScript
    {
        void SweepSouls()
        {
            if (++_sweepTicks < 2) // every other heartbeat -> ~10s sweep
                return;
            _sweepTicks = 0;

            Unit* owner = GetUnitOwner();
            Player* player = owner->ToPlayer();
            if (!player)
                return;

            // retail emits no kill-credit casts once the objective is filled
            if (player->GetQuestStatus(Quests::FelInfusion) != QUEST_STATUS_INCOMPLETE)
                return;

            std::vector<Creature*> souls;
            for (uint32 entry : { Creatures::SavageFelstalker, Creatures::FelShocktrooper })
            {
                std::list<Creature*> found;
                owner->GetCreatureListWithEntryInGrid(found, entry, 50.0f);
                for (Creature* creature : found)
                    if (!creature->IsAlive() && _absorbedSouls.insert(creature->GetGUID()).second)
                        souls.push_back(creature);
            }

            if (souls.empty())
                return;

            owner->CastSpell(owner, Spells::FillFel, true);
            for (Creature* soul [[maybe_unused]] : souls)
                owner->CastSpell(owner, Spells::FelInfusionKillCredit, true);
            // 133509 Fill Fel Bar - child of the Fill Fel cast on retail,
            // energizes the alternate power bar by the wave's soul count
            owner->CastSpell(owner, Spells::FillFelBar, CastSpellExtraArgs(TRIGGERED_FULL_MASK)
                .AddSpellMod(SPELLVALUE_BASE_POINT0, int32(souls.size())));
        }

        void Register() override
        {
            OnHeartbeat += AuraHeartbeatFn(spell_vault_fel_infusion::SweepSouls);
        }

        uint32 _sweepTicks = 0;
        GuidUnorderedSet _absorbedSouls;
    };
}

void AddSC_custom_vault_wardens_spells()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    RegisterSpellScript(spell_vault_throwing_blade);
    RegisterSpellScript(spell_vault_fel_infusion);
}
