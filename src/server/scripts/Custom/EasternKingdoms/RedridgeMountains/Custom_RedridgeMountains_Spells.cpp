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

#include "Player.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "TemporarySummon.h"

#include "Custom_RedridgeMountains_Defines.h"

namespace Scripts::EasternKingdoms::RedridgeMountains
{
    // 82580 - Bravo Company Field Kit
    class spell_bravo_company_field_kit : public SpellScript
    {
        SpellCastResult CheckCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return SPELL_FAILED_DONT_REPORT;

            if (caster->HasAura(Spells::BravoCompanyFieldKit))
            {
                caster->CastSpell(caster, Spells::CancelBravoCompanyFieldKit, true);
                return SPELL_FAILED_DONT_REPORT;
            }

            return SPELL_CAST_OK;
        }

        void HandleHit()
        {
            Player* player = GetCaster()->ToPlayer();
            if (!player)
                return;

            std::list<TempSummon*> minions;
            player->GetAllMinionsByEntry(minions, Creatures::JorgensenGuardian);
            for (TempSummon* minion : minions)
            {
                // Am using SetData to trigger the broadcast
                minion->AI()->SetData(1, 1);
            }
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_bravo_company_field_kit::CheckCast);
            OnHit += SpellHitFn(spell_bravo_company_field_kit::HandleHit);
        }
    };
}

void AddSC_custom_redridge_mountains_spells()
{
    using namespace Scripts::EasternKingdoms::RedridgeMountains;

    RegisterSpellScript(spell_bravo_company_field_kit);
}
