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

#include "Chat.h"
#include "ChatCommandTags.h"
#include "ChatCommand.h"
#include "DB2Stores.h"
#include "GridNotifiers.h"
#include "Group.h"
#include "MiscPackets.h"
#include "MovementGenerator.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include "RBAC.h"
#include "MotionMaster.h"
#include "Map.h"

class free_share_scripts : public CommandScript
{
public:
    free_share_scripts() : CommandScript("free_share_scripts") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> commandTable =
        {
            { "barbershop",      rbac::RBAC_PERM_COMMAND_BARBER,        false,      &HandleBarberCommand,       ""},
            { "castgroup",       rbac::RBAC_PERM_COMMAND_CAST_GROUP,    false,      &HandleCastGroupCommand,    ""},
            { "castgroupscene",  rbac::RBAC_PERM_COMMAND_CAST_SCENE,    false,      &HandleCastSceneCommand,    ""},
            { "npcmoveto",       rbac::RBAC_PERM_COMMAND_NPC_MOVE,      false,      &HandleNpcMoveTo,           ""},
            { "npcguidsay",      rbac::RBAC_PERM_COMMAND_NPC_SAY,       false,      &HandleNpcGuidSay,          ""},
            { "npcguidyell",     rbac::RBAC_PERM_COMMAND_NPC_YELL,      false,      &HandleNpcGuidYell,         ""},
        };

        return commandTable;
    }

    // custom command .barber
    static bool HandleBarberCommand(ChatHandler* handler)
    {
        WorldPackets::Misc::EnableBarberShop packet;
        handler->GetSession()->GetPlayer()->SendDirectMessage(WorldPackets::Misc::EnableBarberShop().Write());

        return true;
    }

    // custom command .castgroup
    static bool HandleCastGroupCommand(ChatHandler* handler, SpellInfo const* spellInfo)
    {
        uint32 spellId = spellInfo->Id;

        if (!spellId)
            return false;

        if (!handler->GetSession()->GetPlayer()->GetGroup())
            return false;

        for (GroupReference* itr = handler->GetSession()->GetPlayer()->GetGroup()->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* plr = itr->GetSource();
            if (!plr || !plr->GetSession())
                continue;

            plr->CastSpell(plr, spellId, false);
        }

        return true;
    }

    // custom command .castscene
    static bool HandleCastSceneCommand(ChatHandler* handler, char const* args)
    {

        if (!*args)
            return false;

        char const* scenePackageIdStr = strtok((char*)args, " ");
        char const* flagsStr = strtok(NULL, "");

        if (!scenePackageIdStr)
            return false;

        uint32 scenePackageId = atoi(scenePackageIdStr);
        uint32 flags = flagsStr ? atoi(flagsStr) : 0;

        if (!handler->GetSession()->GetPlayer()->GetGroup())
            return false;

        for (GroupReference* itr = handler->GetSession()->GetPlayer()->GetGroup()->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* plr = itr->GetSource();
            if (!plr || !plr->GetSession())
                continue;

            if (!sSceneScriptPackageStore.HasRecord(scenePackageId))
                return false;

            plr->GetSceneMgr().PlaySceneByPackageId(scenePackageId, SceneFlag(flags));
        }

        return true;
    }

    // custom command .npcmoveto
    static bool HandleNpcMoveTo(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* player = handler->GetSession()->GetPlayer();

        char* guid = strtok((char*)args, " ");

        if (!guid)
            return false;

        char* x = strtok(nullptr, " ");
        char* y = strtok(nullptr, " ");
        char* z = strtok(nullptr, " ");

        if (!x || !y || !z)
            return false;

        float x2, y2, z2;
        x2 = (float)atof(x);
        y2 = (float)atof(y);
        z2 = (float)atof(z);

        Creature* creature = nullptr;

        char* cId = handler->extractKeyFromLink((char*)guid, "Hcreature");

        ObjectGuid::LowType lowguid = Trinity::StringTo<ObjectGuid::LowType>(cId).value_or(UI64LIT(0));

        creature = handler->GetCreatureFromPlayerMapByDbGuid(lowguid);

        if (!creature)
        {
            return false;
        }

        uint32 MapIdPlayer = player->GetMapId();

        uint32 MapIdCreature = creature->GetMapId();

        if (MapIdPlayer != MapIdCreature)
        {
            return false;
        }
        else
        {
            creature->GetMotionMaster()->MovePoint(0, x2, y2, z2);
        }

        return true;
    }

    // custom command .npcguidsay
    static bool HandleNpcGuidSay(ChatHandler* handler, char const* args)
    {

        char* guid = strtok((char*)args, " ");

        if (!guid)
            return false;

        char* say = strtok(nullptr, "");

        if (!say)
            return false;

        Creature* creature = nullptr;

        char* cId = handler->extractKeyFromLink((char*)guid, "Hcreature");

        ObjectGuid::LowType lowguid = Trinity::StringTo<ObjectGuid::LowType>(cId).value_or(UI64LIT(0));

        creature = handler->GetCreatureFromPlayerMapByDbGuid(lowguid);

        if (!creature)
        {
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();
        uint32 MapIdPlayer = player->GetMapId();
        uint32 MapIdCreature = creature->GetMapId();

        if (MapIdPlayer != MapIdCreature)
        {
            return false;
        }
        else
        {
            creature->Say(say, LANG_UNIVERSAL);

            char lastchar = args[strlen(args) - 1];
            switch (lastchar)
            {
            case '?':   creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);      break;
            case '!':   creature->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);   break;
            default:    creature->HandleEmoteCommand(EMOTE_ONESHOT_TALK);          break;
            }
        }

        return true;
    }

    // custom command .npcguidyell
    static bool HandleNpcGuidYell(ChatHandler* handler, char const* args)
    {
        char* guid = strtok((char*)args, " ");

        if (!guid)
            return false;

        char* yell = strtok(nullptr, "");

        if (!yell)
            return false;

        Creature* creature = nullptr;

        char* cId = handler->extractKeyFromLink((char*)guid, "Hcreature");

        ObjectGuid::LowType lowguid = Trinity::StringTo<ObjectGuid::LowType>(cId).value_or(UI64LIT(0));

        creature = handler->GetCreatureFromPlayerMapByDbGuid(lowguid);

        if (!creature)
        {
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();
        uint32 MapIdPlayer = player->GetMapId();
        uint32 MapIdCreature = creature->GetMapId();

        if (MapIdPlayer != MapIdCreature)
        {
            return false;
        }
        else
        {
            creature->Yell(yell, LANG_UNIVERSAL);

            creature->HandleEmoteCommand(EMOTE_ONESHOT_SHOUT);
        }

        return true;
    }
};

void AddSC_free_share_scripts()
{
    new free_share_scripts();
}

