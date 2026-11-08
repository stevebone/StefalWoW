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
#include "Creature.h"
#include "CreatureAI.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "Unit.h"

#include "Custom_VaultWardens_Defines.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    // 70003 - custom serverside areatrigger covering the Vault of Silence.
    // Retail self-casts the CSA aura 138041 on the player as they cross the
    // room and Drelanim whispers ~150ms later. The aura (15min duration,
    // sniffed) is itself the per-player cooldown: the sniffer's second
    // whisper came ~16.8min after the first, and a second player's cast
    // fired independently. No server-side AT exists to hook, so this custom
    // AT drives it.
    struct at_drelanim_hold_the_line : AreaTriggerAI
    {
        at_drelanim_hold_the_line(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

        void OnUnitEnter(Unit* unit) override
        {
            if (!unit->IsPlayer())
                return;

            if (unit->HasAura(Spells::CsaAreaTriggerDummyTimerAuraB))
                return;

            unit->CastSpell(unit, Spells::CsaAreaTriggerDummyTimerAuraB, true);

            if (Creature* drelanim = GetClosestCreatureWithOptions(at, 50.0f, { .CreatureId = Creatures::DrelanimWhisperwind }))
                drelanim->AI()->Talk(Texts::SayDrelanimHoldTheLine, unit);
        }
    };
}

void AddSC_custom_vault_wardens_areatriggers()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    RegisterAreaTriggerAI(at_drelanim_hold_the_line);
}
