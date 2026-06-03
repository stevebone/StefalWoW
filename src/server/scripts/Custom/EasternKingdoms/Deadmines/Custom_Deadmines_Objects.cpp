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
#include "GameObject.h"
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "WorldSession.h"

#include "Custom_Instance_Deadmines.h"
#include "GossipDef.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    struct go_defias_cannon : public GameObjectAI
    {
        go_defias_cannon(GameObject* go) : GameObjectAI(go) { }

        bool OnReportUse(Player* player) override
        {
            InstanceScript* instance = me->GetInstanceScript();

            if (!instance)
                return false;

            if (!player->HasItemCount(Items::DefiasGunpowder, 1))
                return false;

            instance->SetData(CannonEvent::DATA_EVENT, CannonEvent::STATE_CANNON_GUNPOWDER_USED);
            return true;
        }
    };

    // 208002 - Goblin Teleporter
    struct go_goblin_teleporter : public GameObjectAI
    {
        go_goblin_teleporter(GameObject* go) : GameObjectAI(go) { }

        bool OnGossipHello(Player* player) override
        {
            InstanceScript* instance = me->GetInstanceScript();

            if (!instance)
                return false;

            // Load the default gossip menu from database
            player->PlayerTalkClass->ClearMenus();
            player->PlayerTalkClass->GetGossipMenu().SetMenuId(Texts::GoblinTeleporterMenuID);

            Trinity::IteratorPair menuItemBounds = sObjectMgr->GetGossipMenuItemsMapBounds(Texts::GoblinTeleporterMenuID);

            for (auto const& [menuId, menuItem] : menuItemBounds)
            {
                // Check if this menu item should be shown based on boss state
                // OrderIndex 0 = Wood and Lumber (Helix), 1 = Metal and Scraps (Foe Reaper), 2 = Ship Parts (Ripsnarl)
                bool showItem = false;

                switch (menuItem.OrderIndex)
                {
                    case 0: // Wood and Lumber (Helix)
                        showItem = (instance->GetBossState(DataTypes::BOSS_HELIX_GEARBREAKER) == EncounterState::DONE);
                        break;
                    case 1: // Metal and Scraps (Foe Reaper)
                        showItem = (instance->GetBossState(DataTypes::BOSS_FOE_REAPER_5000) == EncounterState::DONE);
                        break;
                    case 2: // Ship Parts (Ripsnarl)
                        showItem = (instance->GetBossState(DataTypes::BOSS_ADMIRAL_RIPSNARL) == EncounterState::DONE);
                        break;
                    default:
                        // Keep other menu items (if any)
                        showItem = true;
                        break;
                }

                if (showItem)
                    player->PlayerTalkClass->GetGossipMenu().AddMenuItem(menuItem, menuItem.MenuID, menuItem.OrderIndex);
            }

            SendGossipMenuFor(player, Texts::GoblinTeleporterMenuID, me->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
        {
            CloseGossipMenuFor(player);

            GameObject* teleporter = nullptr;
            uint32 spawnId = 0;

            switch (gossipListId)
            {
                case 0: // Wood and Lumber (Helix)
                    spawnId = TeleporterSpawnIds::MastRoom;
                    break;
                case 1: // Metal and Scraps (Foe Reaper)
                    spawnId = TeleporterSpawnIds::FoundryRoom;
                    break;
                case 2: // Ship Parts (Ripsnarl)
                    spawnId = TeleporterSpawnIds::IronCladCove;
                    break;
                default:
                    return false;
            }

            teleporter = player->GetMap()->GetGameObjectBySpawnId(spawnId);
            if (!teleporter)
                return false;

            player->TeleportTo(player->GetMapId(), teleporter->GetPositionX(), teleporter->GetPositionY(), teleporter->GetPositionZ() + 1.f, teleporter->GetOrientation());

            // Cast visual spell at random interval (1-5s)
            uint32 delay = urand(1000, 5000);

            player->m_Events.AddEventAtOffset([player]()
            {
                if (player)
                    player->CastSpell(player, Spells::GoblinTeleporter, true);
            }, Milliseconds(delay));

            return true;
        }
    };
}

void AddSC_custom_deadmines_objects()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    RegisterGameObjectAI(go_defias_cannon);
    RegisterGameObjectAI(go_goblin_teleporter);
}
