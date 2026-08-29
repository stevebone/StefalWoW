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
#include "CreatureAIImpl.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"

#include "Custom_Stormwind_Defines.h"

namespace Scripts::EasternKingdoms::StormwindCity
{
    // 30987 - Joining The Alliance
    // 30988 - The Alliance Way
    // 30989 - An Old Pit Fighter
    class player_quest_30987_30988_30989_summon_on_relog : public PlayerScript
    {
    public:
        player_quest_30987_30988_30989_summon_on_relog() : PlayerScript("player_quest_30987_30988_30989_summon_on_relog") { }

        void OnLogin(Player* player, bool /*firstLogin*/) override
        {
            bool summonNeeded = player->IsActiveQuest(Quests::JoiningTheAlliance) ||
                player->IsActiveQuest(Quests::TheAllianceWay) || player->IsActiveQuest(Quests::AnOldPitFighter);

            if (!summonNeeded)
                return;

            if (Creature* aysa = player->SummonCreature(Creatures::AysaSpawnedAtGate, player->GetRandomNearPosition(3.f), TEMPSUMMON_MANUAL_DESPAWN, 0s, 0,
                Spells::SummonAysaAtStormwindGate, player->GetGUID()))
            {
                _aysaGUID = aysa->GetGUID();
                aysa->GetMotionMaster()->MoveFollow(player, 1.f);
            }

            if (Creature* jojo = player->SummonCreature(Creatures::JojoSpawnedAtGate, player->GetRandomNearPosition(3.f), TEMPSUMMON_MANUAL_DESPAWN, 0s, 0,
                Spells::SummonJojoAtStormwindGate, player->GetGUID()))
            {
                _jojoGUID = jojo->GetGUID();
                jojo->GetMotionMaster()->MoveFollow(player, 2.f, ChaseAngle(-(float(M_PI) / 2))); // right side
            }
        }

        void OnLogout(Player* player) override
        {
            if (Creature* aysa = ObjectAccessor::GetCreature(*player, _aysaGUID))
                aysa->DespawnOrUnsummon();

            if (Creature* jojo = ObjectAccessor::GetCreature(*player, _jojoGUID))
                jojo->DespawnOrUnsummon();
        }

    private:
        ObjectGuid _aysaGUID;
        ObjectGuid _jojoGUID;
    };
}

void AddSC_custom_stormwind_player()
{
    using namespace Scripts::EasternKingdoms::StormwindCity;

    new player_quest_30987_30988_30989_summon_on_relog();
}
