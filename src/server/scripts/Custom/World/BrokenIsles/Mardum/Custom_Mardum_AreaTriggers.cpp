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

#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "Unit.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    // 10195 - Carrion Storm areatrigger (create-properties 5494, visual spell 198947)
    // applies 199035 to enemies standing inside; cleared on exit or AT removal
    struct at_nefarious_carrion_storm : AreaTriggerAI
    {
        at_nefarious_carrion_storm(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

        void OnUnitEnter(Unit* unit) override
        {
            if (Unit* caster = at->GetCaster())
                if (caster->IsValidAttackTarget(unit))
                    caster->CastSpell(unit, Spells::NefariousCarrionStormAura, true);
        }

        void OnUnitExit(Unit* unit, AreaTriggerExitReason /*reason*/) override
        {
            unit->RemoveAurasDueToSpell(Spells::NefariousCarrionStormAura);
        }
    };
}

void AddSC_custom_mardum_areatriggers()
{
    using namespace Scripts::Custom::Mardum;

    RegisterAreaTriggerAI(at_nefarious_carrion_storm);
}
