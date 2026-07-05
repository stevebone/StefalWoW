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

/* ScriptData
Name: conduit_commandscript
%Complete: 100
Comment: Soulbind conduit collection commands (grant conduits into a player's collection)
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "DB2Stores.h"
#include "Language.h"
#include "Player.h"
#include "RBAC.h"
#include "WorldSession.h"

using namespace Trinity::ChatCommands;

// Interim collection-population trigger. The Blizzlike source is collecting a conduit item in the world; that link
// (item -> SoulbindConduitID) is not yet recovered, so admins/tests grant conduits with this command. Once a conduit
// is owned it can be socketed into the active soulbind's tree (CMSG_GARRISON_SOCKET_TALENT) to apply its spell.
class conduit_commandscript : public CommandScript
{
public:
    conduit_commandscript() : CommandScript("conduit_commandscript") { }

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable conduitCommandTable =
        {
            { "add", HandleConduitAddCommand, rbac::RBAC_PERM_COMMAND_LEARN, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "conduit", conduitCommandTable },
        };
        return commandTable;
    }

    // .conduit add <conduitId> [rankIndex]   (rankIndex omitted => lowest defined rank for the conduit)
    static bool HandleConduitAddCommand(ChatHandler* handler, uint32 conduitId, Optional<int32> rankIndex)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
            return false;

        if (!sSoulbindConduitStore.LookupEntry(conduitId))
        {
            handler->PSendSysMessage("Unknown SoulbindConduit id {}.", conduitId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!target->CollectConduit(conduitId, rankIndex.value_or(-1)))
        {
            handler->PSendSysMessage("Could not grant conduit {} (already owned at an equal/higher rank, or no rank data).", conduitId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Granted conduit {} (rank {}) to {}.", conduitId, target->GetConduitRank(conduitId), target->GetName());
        return true;
    }
};

void AddSC_conduit_commandscript()
{
    new conduit_commandscript();
}
