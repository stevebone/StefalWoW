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

#include "Log.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Spell.h"
#include "SpellInfo.h"

#include "Custom_Warlock_Defines.h"

 // 265187 - Summon Demonic Tyrant
class spell_warlock_summon_demonic_tyrant : public SpellScript
{
    void HandleBeforeCast()
    {
        TC_LOG_ERROR("scripts.ai.fsb", "DemonicTyrant: BeforeCast fired - script IS attached");
    }

    void HandleEffect0(SpellEffIndex effIndex)
    {
        Unit* caster = GetCaster();
        TC_LOG_ERROR("scripts.ai.fsb", "DemonicTyrant: HandleEffect0 fired. effIndex=%u, caster=%s, hasArmaments=%u",
            uint32(effIndex),
            caster ? caster->GetName().c_str() : "NULL",
            caster ? uint32(caster->HasAura(SPELL_WARLOCK_ANTORAN_ARMAMENTS)) : 0);

        if (!caster)
            return;

        if (caster->HasAura(SPELL_WARLOCK_ANTORAN_ARMAMENTS))
        {
            TC_LOG_ERROR("scripts.ai.fsb", "DemonicTyrant: BLOCKING EFFECT_0 (normal summon)");
            PreventHitEffect(effIndex);
            PreventHitDefaultEffect(effIndex);
        }
        else
        {
            TC_LOG_ERROR("scripts.ai.fsb", "DemonicTyrant: ALLOWING EFFECT_0 (normal summon)");
        }
    }

    void HandleEffect3(SpellEffIndex effIndex)
    {
        Unit* caster = GetCaster();
        TC_LOG_ERROR("scripts.ai.fsb", "DemonicTyrant: HandleEffect3 fired. effIndex=%u, caster=%s, hasArmaments=%u",
            uint32(effIndex),
            caster ? caster->GetName().c_str() : "NULL",
            caster ? uint32(caster->HasAura(SPELL_WARLOCK_ANTORAN_ARMAMENTS)) : 0);

        if (!caster)
            return;

        if (!caster->HasAura(SPELL_WARLOCK_ANTORAN_ARMAMENTS))
        {
            TC_LOG_ERROR("scripts.ai.fsb", "DemonicTyrant: BLOCKING EFFECT_3 (empowered summon)");
            PreventHitEffect(effIndex);
            PreventHitDefaultEffect(effIndex);
        }
        else
        {
            TC_LOG_ERROR("scripts.ai.fsb", "DemonicTyrant: ALLOWING EFFECT_3 (empowered summon)");
        }
    }

    void Register() override
    {
        BeforeCast += SpellCastFn(spell_warlock_summon_demonic_tyrant::HandleBeforeCast);
        OnEffectHitTarget += SpellEffectFn(spell_warlock_summon_demonic_tyrant::HandleEffect0, EFFECT_0, SPELL_EFFECT_SUMMON);
        OnEffectHitTarget += SpellEffectFn(spell_warlock_summon_demonic_tyrant::HandleEffect3, EFFECT_3, SPELL_EFFECT_SUMMON);
    }
};

void AddSC_custom_warlock_spell_fixes()
{
    new spell_warlock_summon_demonic_tyrant();
}
