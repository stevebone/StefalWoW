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
Name: garrison_commandscript
%Complete: 100
Comment: GM/dev commands for the garrison (force site-level upgrade, bypassing the client blueprint/quest gate)
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "DB2Structure.h"
#include "Garrison.h"
#include "Player.h"
#include "RBAC.h"
#include "WorldSession.h"

using namespace Trinity::ChatCommands;

// Dev commands for the WoD garrison. The retail upgrade path (Garrison Architect table) only unlocks the
// upgrade button once the client knows the "Garrison Blueprint: Level N" and the prerequisite quests are
// done; that client gate is not satisfied by completing the quests alone. This command drives the same
// server logic the architect handler uses (Garrison::Upgrade) directly, so a developer can level a garrison
// up on demand to test higher-level plots/buildings. Buildings are preserved by plot instance id.
class garrison_commandscript : public CommandScript
{
public:
    garrison_commandscript() : CommandScript("garrison_commandscript") { }

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable garrisonCommandTable =
        {
            { "upgrade", HandleGarrisonUpgradeCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "garrison", garrisonCommandTable },
        };
        return commandTable;
    }

    // .garrison upgrade   - advance the selected player's WoD garrison one site level (repeatable to max).
    static bool HandleGarrisonUpgradeCommand(ChatHandler* handler)
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

        Garrison* garrison = target->GetGarrison();
        if (!garrison)
        {
            handler->PSendSysMessage("{} has no WoD garrison.", target->GetName());
            handler->SetSentErrorMessage(true);
            return false;
        }

        GarrSiteLevelEntry const* before = garrison->GetSiteLevel();
        uint32 beforeLevel = before ? before->GarrLevel : 0;

        garrison->Upgrade();

        GarrSiteLevelEntry const* after = garrison->GetSiteLevel();
        uint32 afterLevel = after ? after->GarrLevel : 0;

        if (afterLevel <= beforeLevel)
        {
            handler->PSendSysMessage("Garrison is already at its maximum site level ({}).", beforeLevel);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Upgraded {}'s garrison: level {} -> {} (GarrSiteLevel {}). Buildings preserved.",
            target->GetName(), beforeLevel, afterLevel, after ? after->ID : 0);
        return true;
    }
};

void AddSC_garrison_commandscript()
{
    new garrison_commandscript();
}
