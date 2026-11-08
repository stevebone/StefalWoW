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
}

void AddSC_custom_vault_wardens_spells()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    RegisterSpellScript(spell_vault_throwing_blade);
}
