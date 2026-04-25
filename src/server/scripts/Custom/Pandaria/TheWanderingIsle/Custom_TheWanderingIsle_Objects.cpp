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

#include "GameObject.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "ScriptMgr.h"

#include "Custom_TheWanderingIsle_Defines.h"

namespace Scripts::Custom::TheWanderingIsle
{
    // GO 210965 - Forest Door 1
    class go_210965_forest_door : public GameObjectScript
    {
    public:
        go_210965_forest_door() : GameObjectScript("go_210965_forest_door") { }

        struct go_210965_interactAI : public GameObjectAI
        {
            go_210965_interactAI(GameObject* go) : GameObjectAI(go) { }

            void Reset() override
            {
                //me->SetDynamicFlag(GO_DYNFLAG_LO_INTERACT_COND);
                me->SetFaction(114);
            }

            bool OnReportUse(Player* player) override
            {
                if (!player)
                    return false;

                player->KilledMonsterCredit(Npcs::credit_forest_door_one);

                if (Creature* aysa = player->FindNearestCreature(Npcs::npc_aysa_q29792, 40.0f))
                    aysa->AI()->SetData(1, 1);

                return true;
            }
        };

        GameObjectAI* GetAI(GameObject* go) const override
        {
            return new go_210965_interactAI(go);
        }
    };

    // GO 209626 - Gong
    class go_209626_gong : public GameObjectScript
    {
    public:
        go_209626_gong() : GameObjectScript("go_209626_gong") { }

        struct go_209626_gongAI : public GameObjectAI
        {
            go_209626_gongAI(GameObject* go) : GameObjectAI(go) { }

            void Reset() override
            {
            }

            bool OnReportUse(Player* player) override
            {
                if (!player)
                    return false;

                Creature* wugou = player->FindNearestCreature(Npcs::npc_wugou_farmstead, 20.0f);
                Creature* ji = player->FindNearestCreature(Npcs::npc_ji_farmstead, 30.0f);

                if (ji && wugou)
                {
                    CastSpellExtraArgs args(TRIGGERED_IGNORE_TARGET_CHECK | TRIGGERED_CAST_DIRECTLY | TRIGGERED_IS_TRIGGERED_MASK);

                    player->CastSpell(me, Spells::spell_break_gong_credit, args);
                    player->CastSpell(me, Spells::spell_break_gong_sound, true);

                    wugou->AI()->SetData(1, 1);

                    ji->AI()->SetData(2, 2);

                    return true;
                }

                return false;
            }
        };

        GameObjectAI* GetAI(GameObject* go) const override
        {
            return new go_209626_gongAI(go);
        }
    };
}

void AddSC_custom_the_wandering_isle_objects()
{
    using namespace Scripts::Custom::TheWanderingIsle;

    new go_210965_forest_door();
    new go_209626_gong();
}
