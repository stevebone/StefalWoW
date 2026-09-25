/*
* This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ScriptMgr.h"
#include "GameObject.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Item.h"

enum InoculatingCrystal
{
    ITEM_INOCULATING_CRYSTAL              = 22962,
    NPC_NESTLEWOOD_OWLKIN                 = 16518,
    NPC_MUTATED_OWLKIN                    = 16537,
    SPELL_INOCULATE_NESTLEWOOD_OWLKIN     = 29528,
};

class item_inoculating_crystal : public ItemScript
{
public:
    item_inoculating_crystal() : ItemScript("item_inoculating_crystal") {}

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/, ObjectGuid castId) override
    {
        if (item->GetEntry() != ITEM_INOCULATING_CRYSTAL)
            return false;

        Unit* target = player->GetSelectedUnit();
        if (!target || target->GetTypeId() != TYPEID_UNIT ||
            (target->GetEntry() != NPC_NESTLEWOOD_OWLKIN && target->GetEntry() != NPC_MUTATED_OWLKIN))
        {
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_INOCULATE_NESTLEWOOD_OWLKIN, DIFFICULTY_NONE))
                Spell::SendCastResult(player, spellInfo, {}, castId, SPELL_FAILED_BAD_TARGETS);
            return true;
        }

        player->CastSpell(target, SPELL_INOCULATE_NESTLEWOOD_OWLKIN, true);
        return true;
    }
};

class npc_mutated_owlkin : public ScriptedAI
{
public:
    explicit npc_mutated_owlkin(Creature* creature) : ScriptedAI(creature) { }

    void OnAuraRemoved(AuraApplication const* aurApp) override
    {
        if (aurApp->GetBase()->GetId() == SPELL_INOCULATE_NESTLEWOOD_OWLKIN)
            if (Unit* caster = aurApp->GetBase()->GetCaster())
                AttackStart(caster);
    }
};

void AddSC_azuremyst_isle_custom()
{
    new item_inoculating_crystal();
    RegisterCreatureAI(npc_mutated_owlkin);
}
