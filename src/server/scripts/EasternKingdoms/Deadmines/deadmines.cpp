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

#include "GameObject.h"
#include "GameObjectAI.h"
#include "ScriptMgr.h"
#include "PhasingHandler.h"
#include "deadmines.h"

struct go_defias_cannon : public GameObjectAI
{
    go_defias_cannon(GameObject* go) : GameObjectAI(go) { }

    bool OnReportUse(Player* /*player*/) override
    {
        InstanceScript* instance = me->GetInstanceScript();

        if (!instance)
            return false;

        instance->SetData(EVENT_STATE, CANNON_GUNPOWDER_USED);
        return true;
    }
};

class player_deadmines_phase : public PlayerScript
{
public:
    player_deadmines_phase() : PlayerScript("player_deadmines_phase") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        if (player->GetMapId() == 36)
        {
            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                //player->SetPhaseMask(1 | 10, true); // Visible to default (1) and Alliance phase (10)
                //PhasingHandler::AddPhaseGroup(player, 1364, true);
                PhasingHandler::AddPhase(player, 226, true);
                PhasingHandler::AddPhase(player, 169, true);
            }
            else if (player->GetTeamId() == TEAM_HORDE)
            {
                PhasingHandler::AddPhase(player, 170, true);
                PhasingHandler::AddPhase(player, 169, true);
            }
        }
    }

    //void OnMapChanged(Player* player) override
    void OnUpdateZone(Player* player, uint32 newZone, uint32 /*newArea*/) override
    {
        if (player->GetMapId() == 36 && newZone == 1581)
        {
            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                //player->SetPhaseMask(1 | 10, true); // Visible to default (1) and Alliance phase (10)
                //PhasingHandler::AddPhaseGroup(player, 1364, true);
                PhasingHandler::AddPhase(player, 226, true);
                PhasingHandler::AddPhase(player, 169, true);
            }
            else if (player->GetTeamId() == TEAM_HORDE)
            {
                PhasingHandler::AddPhase(player, 170, true);
                PhasingHandler::AddPhase(player, 169, true);
                //player->SetPhaseMask(1 | 20, true); // Visible to default (1) and Horde phase (20)
            }
        }
    }

    void OnMapChanged(Player* player) override
    {
        if (player->GetMapId() == 36)
        {
            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                //player->SetPhaseMask(1 | 10, true); // Visible to default (1) and Alliance phase (10)
                //PhasingHandler::AddPhaseGroup(player, 1364, true);
                PhasingHandler::AddPhase(player, 226, true);
                PhasingHandler::AddPhase(player, 169, true);
            }
            else if (player->GetTeamId() == TEAM_HORDE)
            {
                PhasingHandler::AddPhase(player, 170, true);
                PhasingHandler::AddPhase(player, 169, true);
                //player->SetPhaseMask(1 | 20, true); // Visible to default (1) and Horde phase (20)
            }
        }
    }
};

void AddSC_deadmines()
{
    RegisterGameObjectAI(go_defias_cannon);
    //new go_defias_cannon();

    new player_deadmines_phase();
}
