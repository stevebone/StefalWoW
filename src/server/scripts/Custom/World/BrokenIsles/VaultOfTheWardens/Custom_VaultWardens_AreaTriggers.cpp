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
#include "Player.h"
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

    // 70004 - custom serverside areatrigger covering the mouth of the Tomb of
    // the Penitent. Sniffed: on crossing the tomb threshold (~4272.9,-300.1)
    // the player self-casts 192605 "Conversation: Illidan's Chamber Arrival",
    // creating private conversation 527 (Maiev + Kayn world-object actors).
    // The AT spawn lives in phase 7385 itself (phase_area 7871->7385 gated on
    // quests 38689/38690 complete), so the trigger only exists for players who
    // can see the trio. No sniffed server-side AT exists to reuse.
    struct at_tomb_of_the_penitent_conversation : AreaTriggerAI
    {
        at_tomb_of_the_penitent_conversation(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

        void OnUnitEnter(Unit* unit) override
        {
            if (!unit->IsPlayer())
                return;

            if (unit->HasAura(Spells::TombPenitentConversation))
                return;

            unit->CastSpell(unit, Spells::TombPenitentConversation, true);
        }
    };

    // 70005 - custom serverside areatrigger at the Illidari Ward / Tomb of
    // the Penitent boundary. Sniffed: crossing at ~(4326,-405) makes the
    // player self-cast 202116 "Mephistos 1", which creates private
    // conversation 1181 (Bastillax taunt) and completes tracking quest
    // 40921 - the quest's reward status is retail's own persistent
    // once-per-player gate. The spawn is phased to 7385 (phase_area
    // 7871->7385 gated on quests 38689/38690 complete), so the trigger only
    // exists during that exact window.
    struct at_bastillax_taunt : AreaTriggerAI
    {
        at_bastillax_taunt(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

        void OnUnitEnter(Unit* unit) override
        {
            if (!unit->IsPlayer())
                return;

            if (unit->ToPlayer()->GetQuestRewardStatus(Quests::BastillaxTauntTracking))
                return;

            unit->CastSpell(unit, Spells::BastillaxTaunt, true);
        }
    };
}

void AddSC_custom_vault_wardens_areatriggers()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    RegisterAreaTriggerAI(at_drelanim_hold_the_line);
    RegisterAreaTriggerAI(at_tomb_of_the_penitent_conversation);
    RegisterAreaTriggerAI(at_bastillax_taunt);
}
