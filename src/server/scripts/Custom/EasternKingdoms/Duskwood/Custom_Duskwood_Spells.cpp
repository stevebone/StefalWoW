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

#include "Creature.h"
#include "ScriptMgr.h"

#include "Custom_Duskwood_Defines.h"

namespace Scripts::EasternKingdoms::Duskwood
{
    /*######
    ## 82029 Call Stalvan
    ######*/

    class spell_call_stalvan : public SpellScript
    {
        bool IsEventRunning()
        {
            return GetCaster()->FindNearestCreature(Creatures::StalvanMistmantle, 20.f, true) != nullptr;
        }

        SpellCastResult CheckRequirement()
        {
            return SPELL_CAST_OK;
        }

        void HandleSendEvent(SpellEffIndex /*effIndex*/)
        {
            if (!GetCaster()->IsPlayer())
                return;

            if (GetCaster()->ToPlayer()->GetQuestStatus(Quests::MistmantlesRevenge) != QUEST_STATUS_INCOMPLETE)
                return;

            if (IsEventRunning())
                return;

            GetCaster()->SummonCreature(Creatures::StalvanMistmantle, Positions::StalvanSpawn, TEMPSUMMON_MANUAL_DESPAWN);
            GetCaster()->SummonCreature(Creatures::TobiasMistmantle, Positions::TobiasSpawn, TEMPSUMMON_MANUAL_DESPAWN);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_call_stalvan::HandleSendEvent, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
            OnCheckCast += SpellCheckCastFn(spell_call_stalvan::CheckRequirement);
        }
    };
}

void AddSC_custom_duskwood_spells()
{
    using namespace Scripts::EasternKingdoms::Duskwood;

    RegisterSpellScript(spell_call_stalvan);
}
