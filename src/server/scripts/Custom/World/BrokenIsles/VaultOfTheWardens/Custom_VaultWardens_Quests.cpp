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
#include "EventProcessor.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"

#include "Custom_VaultWardens_Defines.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    using namespace Creatures;
    using namespace Quests;
    using namespace Texts;

    // 38672 - Breaking Out
    // Bound through quest_template_addon.ScriptName - only fires for this quest.
    class quest_vault_wardens_breaking_out : public QuestScript
    {
    public:
        quest_vault_wardens_breaking_out() : QuestScript("quest_vault_wardens_breaking_out") { }

        void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
        {
            // every status transition re-evaluates the area conditions
            // (cell phases 5964/5965, freed phases 5124/5125, Maiev/ward phases)
            PhasingHandler::OnConditionChange(player);

            if (newStatus == QUEST_STATUS_NONE)
            {
                // abandoning mid-scene: remove leftover personal clones so the
                // static/tracked spawns take over again (cell phases re-enable)
                for (uint32 entry : { AltruisCell, KaynCell, AltruisTheSufferer, KaynSunfury, MaievShadowsong, WardOfTheHunters })
                {
                    std::list<Creature*> creatures;
                    player->GetCreatureListWithEntryInGrid(creatures, entry, 150.0f);
                    for (Creature* creature : creatures)
                        if (creature->IsPrivateObject() && creature->GetPrivateObjectOwner() == player->GetGUID())
                            creature->DespawnOrUnsummon();
                }
            }
            else if (newStatus == QUEST_STATUS_COMPLETE)
            {
                // the freed Illidari react (creature_text group 1); the static
                // rally spawns talk, so private objects are excluded
                if (Creature* kayn = player->FindNearestCreatureWithOptions(40.0f, { .CreatureId = KaynSunfury, .IgnorePrivateObjects = true }))
                    kayn->AI()->Talk(SayKaynRally, player); // "Why did you let THAT one free?"

                player->m_Events.AddEventAtOffset([player]()
                {
                    if (Creature* altruis = player->FindNearestCreatureWithOptions(40.0f, { .CreatureId = AltruisTheSufferer, .IgnorePrivateObjects = true }))
                        altruis->AI()->Talk(SayAltruisRally, player); // "It has been a long time, Kayn..."
                }, 5s);
            }
        }
    };
}

void AddSC_custom_vault_wardens_quests()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    new quest_vault_wardens_breaking_out();
}
