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
#include "CreatureAI.h"
#include "EventProcessor.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    // 1116 - The Invasion Begins (banner planted)
    class scene_the_invasion_begins_banner_planted : public SceneScript
    {
    public:
        scene_the_invasion_begins_banner_planted() : SceneScript("scene_the_invasion_begins_banner_planted") { }

        void OnSceneComplete(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
        {
            Creature* kayn = GetClosestCreatureWithOptions(player, Misc::SearchRange,
                { .CreatureId = Creatures::KaynSunfury, .IgnorePhases = true });
            if (!kayn)
                return;

            kayn->AI()->Talk(CreatureText::KaynFindKeystone, player);

            // 5s later, second line. Scheduled on the player's EventProcessor so it is
            // cleaned up if the player leaves; Kayn is re-resolved by GUID in case he despawned.
            player->m_Events.AddEventAtOffset([player, kaynGuid = kayn->GetGUID()]()
            {
                if (Creature* kayn = ObjectAccessor::GetCreature(*player, kaynGuid))
                    kayn->AI()->Talk(CreatureText::KaynActivateGateway, player);
            }, 5s);
        }
    };
}

void AddSC_custom_mardum_scenes()
{
    using namespace Scripts::Custom::Mardum;
    new scene_the_invasion_begins_banner_planted();
}
