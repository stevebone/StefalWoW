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
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Unit.h"

#include "Custom_Paladin_Defines.h"

namespace Scripts::Custom::Paladin
{
    // 406064 - Art of War
    // EFFECT_0: base proc chance (15%) on melee attacks to reset Blade of Justice.
    // EFFECT_1: extra chance (10%) added on critical strikes.
    // Blade of Justice is charge-based (ChargeCategory 2128) in 12.1, so RestoreCharge is used
    // instead of ResetCooldown.
    class spell_pal_art_of_war_custom : public AuraScript
    {
        bool Validate(SpellInfo const* spellInfo) override
        {
            return ValidateSpellInfo({ Spells::ArtOfWarTriggered, Spells::BladeOfJustice })
                && ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
        }

        bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            int32 chance = aurEff->GetAmountAsInt();
            if (eventInfo.GetHitMask() & PROC_HIT_CRITICAL)
                if (AuraEffect const* critBonus = GetEffect(EFFECT_1))
                    chance += critBonus->GetAmountAsInt();

            return roll_chance(chance);
        }

        bool CheckCritBonus(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
        {
            // EFFECT_1 is only the crit chance bonus; it must never count as the proccing effect.
            return false;
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* target = GetTarget();
            target->GetSpellHistory()->RestoreCharge(
                sSpellMgr->AssertSpellInfo(Spells::BladeOfJustice, GetCastDifficulty())->ChargeCategoryId);

            target->CastSpell(target, Spells::ArtOfWarTriggered, CastSpellExtraArgsInit
            {
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = eventInfo.GetProcSpell()
            });
        }

        void Register() override
        {
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_art_of_war_custom::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_art_of_war_custom::CheckCritBonus, EFFECT_1, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_art_of_war_custom::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };
}

void AddSC_custom_paladin_spell_fixes()
{
    using namespace Scripts::Custom::Paladin;

    RegisterSpellScript(spell_pal_art_of_war_custom);
}
