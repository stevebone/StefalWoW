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
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Unit.h"

#include "Custom_Druid_Defines.h"

namespace Scripts::Custom::Druid
{
    // 450347 - Nature's Grace
    // Custom version: Midnight DB2 no longer has EFFECT_2 and the spell no longer stacks.
    // Trigger casts Dreamstate without SPELLVALUE_AURA_STACK override.
    class spell_dru_natures_grace_custom : public AuraScript
    {
    public:
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::NaturesGraceTalent, Spells::Dreamstate });
        }

        static void Trigger(Unit* caster)
        {
            caster->CastSpell(caster, Spells::Dreamstate, CastSpellExtraArgsInit{});
        }

        void OnOwnerInCombat(bool isNowInCombat) const
        {
            if (isNowInCombat)
                Trigger(GetTarget());
        }

        void Register() override
        {
            OnEnterLeaveCombat += AuraEnterLeaveCombatFn(spell_dru_natures_grace_custom::OnOwnerInCombat);
        }
    };

    // 48517 Eclipse (Solar) + 48518 Eclipse (Lunar)
    // Custom version: Midnight DB2 no longer has EFFECT_2 on Nature's Grace talent (450347).
    // Uses HasAura() instead of HasAuraEffect(..., EFFECT_2) and calls simplified Trigger().
    class spell_dru_natures_grace_eclipse_custom : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::Dreamstate });
        }

        bool Load() override
        {
            return GetCaster()->HasAura(Spells::NaturesGraceTalent);
        }

        void HandleRemoved(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
        {
            spell_dru_natures_grace_custom::Trigger(GetTarget());
        }

        void Register() override
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_dru_natures_grace_eclipse_custom::HandleRemoved, EFFECT_0, SPELL_AURA_ADD_PCT_MODIFIER, AURA_EFFECT_HANDLE_REAL);
        }
    };
}

void AddSC_custom_druid_spell_fixes()
{
    using namespace Scripts::Custom::Druid;

    RegisterSpellScript(spell_dru_natures_grace_custom);
    RegisterSpellScript(spell_dru_natures_grace_eclipse_custom);
}
