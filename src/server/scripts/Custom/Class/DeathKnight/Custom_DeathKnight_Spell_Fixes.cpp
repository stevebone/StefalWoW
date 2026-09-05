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
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Unit.h"

#include "Custom_DeathKnight_Defines.h"

namespace Scripts::Custom::DeathKnight
{
    // 374606 - Blood Draw (drain)
    // Triggered by talent 374598 (SPELL_AURA_TRIGGER_SPELL_ON_HEALTH_PCT) when falling below 30% health.
    // The health-pct trigger bypasses the proc system and ignores spell cooldowns, so the 2 min ICD
    // is enforced through aura 374609. The ICD and the 8 sec buff (454871) are applied here synchronously
    // to close the 200 ms window of the DB2 delayed trigger.
    class spell_dk_blood_draw_drain : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::BloodDrawIcd, Spells::BloodDrawBuff });
        }

        SpellCastResult CheckIcd()
        {
            if (GetCaster()->HasAura(Spells::BloodDrawIcd))
                return SPELL_FAILED_DONT_REPORT;

            return SPELL_CAST_OK;
        }

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            CastSpellExtraArgs args(CastSpellExtraArgsInit
            {
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = GetSpell()
            });

            caster->CastSpell(caster, Spells::BloodDrawIcd, args);
            caster->CastSpell(caster, Spells::BloodDrawBuff, args);
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_dk_blood_draw_drain::CheckIcd);
            AfterCast += SpellCastFn(spell_dk_blood_draw_drain::HandleAfterCast);
        }
    };

    // 454871 - Blood Draw (buff)
    // Talent 374598 EFFECT_3 also triggers this directly on the same 30% crossing.
    // Reject the raw talent trigger; the buff is cast by spell_dk_blood_draw_drain instead so it shares the ICD.
    class spell_dk_blood_draw_buff : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::BloodDraw });
        }

        SpellCastResult CheckTriggerSource()
        {
            if (SpellInfo const* triggeringSpell = GetTriggeringSpell())
                if (triggeringSpell->Id == Spells::BloodDraw)
                    return SPELL_FAILED_DONT_REPORT;

            return SPELL_CAST_OK;
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_dk_blood_draw_buff::CheckTriggerSource);
        }
    };
}

void AddSC_custom_deathknight_spell_fixes()
{
    using namespace Scripts::Custom::DeathKnight;

    RegisterSpellScript(spell_dk_blood_draw_drain);
    RegisterSpellScript(spell_dk_blood_draw_buff);
}
