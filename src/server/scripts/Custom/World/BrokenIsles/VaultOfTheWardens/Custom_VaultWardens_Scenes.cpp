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

#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptMgr.h"

#include "Custom_VaultWardens_Defines.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    // 1016 - "Stop Gul'dan!" tomb cinematic (played by spell 187864 when the
    // quest-accept aura 223661 expires). The sniffed end-of-scene tick shows:
    // scene aura removed -> phase swap (+5128, -7385) -> bare
    // SMSG_MOVE_TELEPORT into the Vault of the Betrayer arena -> auto
    // SMSG_CANCEL_SCENE. There is no teleport spell in the sniff - the server
    // teleports directly, and the arena NPCs/portals are static spawns
    // revealed by phase 5128. Rewarding tracking quest 40922 ("Mephistos 2")
    // makes the phase_area conditions do the swap, persisting across relogs -
    // same pattern as 40921 for the Bastillax taunt. The core's
    // OnSceneComplete ordering (aura remove -> this hook -> auto cancel)
    // already matches the sniffed packet order.
    class scene_vault_stop_guldan : public SceneScript
    {
    public:
        scene_vault_stop_guldan() : SceneScript("scene_vault_stop_guldan") { }

        void OnSceneComplete(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
        {
            TransitionToArena(player);
        }

        void OnSceneCancel(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
        {
            TransitionToArena(player);
        }

    private:
        void TransitionToArena(Player* player)
        {
            if (player->GetMapId() != Maps::WardenPrisonDH)
                return;

            // reward first: its completion makes the phase_area conditions
            // drop 7385 / raise 5128 in the visibility pass below (neither
            // quest carries UPDATE_PHASESHIFT, so the explicit refresh is
            // required)
            if (!player->GetQuestRewardStatus(Quests::StopGuldanSceneTracking))
                player->CompleteQuest(Quests::StopGuldanSceneTracking);

            // sniffed order: phase shift -> INIT_WORLD_STATES (empty for this
            // map) -> START_LIGHTNING_STORM (id 0) -> MOVE_TELEPORT. The core
            // sends the storm packet itself from SendInitWorldStates, so the
            // pair is produced here just by calling it
            PhasingHandler::OnConditionChange(player);
            player->SendInitWorldStates(Zones::VaultOfTheWardens, Areas::VaultOfTheBetrayer);
            player->TeleportTo(WorldLocation(Maps::WardenPrisonDH, Positions::StopGuldanArenaTeleport));
        }
    };
}

void AddSC_custom_vault_wardens_scenes()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    new scene_vault_stop_guldan();
}
