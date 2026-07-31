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

#include <iterator>

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
            { "upgrade",   HandleGarrisonUpgradeCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "create",    HandleGarrisonCreateCommand,    rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "champions", HandleGarrisonChampionsCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "enter",     HandleGarrisonEnterCommand,     rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "exit",      HandleGarrisonExitCommand,      rbac::RBAC_PERM_COMMAND_GM, Console::No },
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

    // .garrison create <siteId>   - create a garrison of the given GarrSite id for the selected player.
    //   Legion Hunter Order Hall (Trueshot Lodge) = 161; WoD Alliance = 2, WoD Horde = 71.
    static bool HandleGarrisonCreateCommand(ChatHandler* handler, uint32 garrSiteId)
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

        target->CreateGarrison(garrSiteId);

        // Resolve which garrison (by type) the new site produced, so we can confirm it and report the map.
        Garrison* created = nullptr;
        for (GarrisonType t : { GARRISON_TYPE_GARRISON, GARRISON_TYPE_CLASS_ORDER, GARRISON_TYPE_WAR_CAMPAIGN, GARRISON_TYPE_COVENANT })
            if (Garrison* g = target->GetGarrison(t))
                if (g->GetSiteLevel() && g->GetSiteLevel()->GarrSiteID == garrSiteId)
                    created = g;

        if (!created)
        {
            handler->PSendSysMessage("Failed to create a garrison for GarrSite {} (unknown/invalid site id).", garrSiteId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Created garrison for {}: GarrSite {} -> type {}, map {}.",
            target->GetName(), garrSiteId, uint32(created->GetType()), created->GetSiteLevel()->MapID);
        return true;
    }

    // .garrison champions   - grant the Hunter Order Hall starting champions to the class-order garrison and seed
    // its mission board. Create the hall first with ".garrison create 161".
    static bool HandleGarrisonChampionsCommand(ChatHandler* handler)
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

        Garrison* garrison = target->GetGarrison(GARRISON_TYPE_CLASS_ORDER);
        if (!garrison)
        {
            handler->PSendSysMessage("{} has no Order Hall - create it first with '.garrison create 161'.", target->GetName());
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Hunter "Unseen Path" champions (GarrFollower ids, GarrType 3). Emmarel Shadewarden (593) is the hall leader.
        static constexpr uint32 hunterChampions[] = { 593, 642, 742, 743, 744, 745, 746, 747, 748 };
        for (uint32 followerId : hunterChampions)
            garrison->AddFollower(followerId);

        garrison->GenerateAvailableMissions();

        handler->PSendSysMessage("Granted {} Hunter champions and seeded missions to {}'s Order Hall.",
            uint32(std::size(hunterChampions)), target->GetName());
        return true;
    }

    // .garrison enter   - teleport to the Hunter Order Hall (Trueshot Lodge). The order hall is a phased location on
    // the Broken Isles continent (map 1220), not a separate instance - retail reaches it via the Dalaran Order Hall
    // portal; this teleports straight there.
    static bool HandleGarrisonEnterCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        target->TeleportTo(1220, 4529.88f, 5246.98f, 862.193f, 0.0f);
        handler->PSendSysMessage("Teleported {} to Trueshot Lodge (Hunter Order Hall).", target->GetName());
        return true;
    }

    // .garrison exit   - return from the Order Hall to Dalaran (Broken Isles).
    static bool HandleGarrisonExitCommand(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        target->TeleportTo(1220, -849.908f, 4461.17f, 735.661f, 0.0f);
        handler->PSendSysMessage("Returned {} to Dalaran.", target->GetName());
        return true;
    }
};

void AddSC_garrison_commandscript()
{
    new garrison_commandscript();
}
