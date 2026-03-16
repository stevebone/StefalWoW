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
#include "SpellScript.h"
#include "Spell.h"
#include "SpellInfo.h"

 // 265187 - Summon Demonic Tyrant
class spell_warlock_summon_demonic_tyrant : public SpellScriptLoader
{
public:
    spell_warlock_summon_demonic_tyrant() : SpellScriptLoader("spell_warlock_summon_demonic_tyrant") { }

    class spell_warlock_summon_demonic_tyrant_SpellScript : public SpellScript
    {

        void HandleBeforeHit(SpellMissInfo missInfo)
        {
            // Prevent EFFECT_0 from summoning
            if (GetSpellInfo()->GetEffect(EFFECT_0).Effect == SPELL_EFFECT_SUMMON)
                PreventHitEffect(EFFECT_0);
        }

        void Register() override
        {
            BeforeHit += BeforeSpellHitFn(spell_warlock_summon_demonic_tyrant_SpellScript::HandleBeforeHit);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warlock_summon_demonic_tyrant_SpellScript();
    }
};

void AddSC_custom_warlock_spell_fixes()
{
    new spell_warlock_summon_demonic_tyrant();
}
