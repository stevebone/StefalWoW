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
Name: warfront_commandscript
%Complete: 100
Comment: GM/dev commands for the BfA Warfront cycle owner (WarfrontMgr): inspect and force-advance the state machine
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Creature.h"
#include "GameTime.h"
#include "InstanceScript.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RBAC.h"
#include "WarfrontMgr.h"

using namespace Trinity::ChatCommands;

// Dev commands for the Warfront cycle owner. WarfrontMgr drives a day-scale state machine per zone; these let a
// developer read the live state and force the next transition without waiting on contribution/queue/scenario wiring
// (which lands in P1-P3). See WARFRONTS_DESIGN.md §P0.
class warfront_commandscript : public CommandScript
{
public:
    warfront_commandscript() : CommandScript("warfront_commandscript") { }

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable warfrontCommandTable =
        {
            { "status",  HandleWarfrontStatusCommand,  rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
            { "advance", HandleWarfrontAdvanceCommand, rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
            { "join",    HandleWarfrontJoinCommand,    rbac::RBAC_PERM_COMMAND_GM, Console::No  },
            { "step",    HandleWarfrontStepCommand,    rbac::RBAC_PERM_COMMAND_GM, Console::No  },
            { "boss",    HandleWarfrontBossCommand,    rbac::RBAC_PERM_COMMAND_GM, Console::No  },
            { "goboss",  HandleWarfrontGoBossCommand,  rbac::RBAC_PERM_COMMAND_GM, Console::No  },
        };

        static ChatCommandTable commandTable =
        {
            { "warfront", warfrontCommandTable },
        };
        return commandTable;
    }

    static char const* StateName(WarfrontState state)
    {
        switch (state)
        {
            case WF_CONTRIBUTION: return "CONTRIBUTION";
            case WF_SIEGE:        return "SIEGE";
            case WF_FLIP:         return "FLIP";
            default:              return "UNKNOWN";
        }
    }

    static char const* TeamName(TeamId team)
    {
        return team == TEAM_ALLIANCE ? "Alliance" : (team == TEAM_HORDE ? "Horde" : "Neutral");
    }

    // .warfront status   - print each warfront's state, controlling/challenging team and any active timer.
    static bool HandleWarfrontStatusCommand(ChatHandler* handler)
    {
        auto const& warfronts = sWarfrontMgr->GetWarfronts();
        if (warfronts.empty())
        {
            handler->SendSysMessage("No warfronts are initialized.");
            return true;
        }

        time_t const now = GameTime::GetGameTime();
        handler->PSendSysMessage("Warfronts ({}):", uint32(warfronts.size()));
        for (auto const& [id, wf] : warfronts)
        {
            int64 const remaining = wf.PhaseEndTime ? int64(wf.PhaseEndTime) - int64(now) : 0;
            handler->PSendSysMessage("  [{}] {} | state={} | controlling={} | challenging={} | timer={}s",
                id, wf.Name, StateName(wf.State), TeamName(wf.ControllingTeam), TeamName(wf.ChallengingTeam),
                wf.PhaseEndTime ? int64(std::max<int64>(0, remaining)) : int64(0));
        }
        return true;
    }

    // .warfront advance <id>   - force the given warfront into its next transition (CONTRIBUTION->SIEGE, SIEGE->flip).
    static bool HandleWarfrontAdvanceCommand(ChatHandler* handler, uint32 warfrontId)
    {
        Warfront const* before = sWarfrontMgr->GetWarfront(warfrontId);
        if (!before)
        {
            handler->PSendSysMessage("Unknown warfront id {}.", warfrontId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        WarfrontState const oldState = before->State;
        sWarfrontMgr->AdvanceWarfront(warfrontId);

        Warfront const* after = sWarfrontMgr->GetWarfront(warfrontId);
        handler->PSendSysMessage("Warfront [{}] {}: {} -> {} (controlling now {}).",
            warfrontId, after->Name, StateName(oldState), StateName(after->State), TeamName(after->ControllingTeam));
        return true;
    }

    // .warfront join [id]   - one-shot dev join: makes your faction the attacker, forces the zone into SIEGE and
    // enrolls you, which (at the test min-player floor) immediately teleports you into the assault instance to fight
    // the boss. Defaults to warfront 1 (Stromgarde). Blizzlike queueing needs the native warfront opcodes (WIP).
    static bool HandleWarfrontJoinCommand(ChatHandler* handler, Optional<uint32> warfrontIdArg)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
        {
            handler->SendSysMessage("This command must be run in-game by a player.");
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 const warfrontId = warfrontIdArg.value_or(uint32(WARFRONT_STROMGARDE));
        std::string reason;
        if (!sWarfrontMgr->DevJoinAssault(player, warfrontId, &reason))
        {
            handler->PSendSysMessage("Could not join the assault: {}", reason.empty() ? "unknown error" : reason);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Joining the assault on warfront [{}] - you should be pulled into the battle now.", warfrontId);
        return true;
    }

    // Resolves the warfront battle instance the caller is standing in. Returns nullptr (and reports it) otherwise.
    static InstanceScript* GetCallerBattleInstance(ChatHandler* handler, Player** outPlayer = nullptr)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        InstanceMap* instMap = player ? player->GetMap()->ToInstanceMap() : nullptr;
        InstanceScript* instance = instMap ? instMap->GetInstanceScript() : nullptr;
        if (!instance)
        {
            handler->SendSysMessage("You are not inside a warfront battle instance.");
            handler->SetSentErrorMessage(true);
            return nullptr;
        }

        if (outPlayer)
            *outPlayer = player;
        return instance;
    }

    // Drives the warfront battle instance the caller is standing in (set Warfront.Battle.AutoAdvance = 0 to hold the
    // scenario for examination, then step it manually).
    static bool SendInstanceCommand(ChatHandler* handler, uint32 command, char const* okMsg)
    {
        InstanceScript* instance = GetCallerBattleInstance(handler);
        if (!instance)
            return false;

        instance->SetData(command, 1);
        handler->SendSysMessage(okMsg);
        return true;
    }

    // .warfront step   - advance the current battle instance's scenario by one step.
    static bool HandleWarfrontStepCommand(ChatHandler* handler)
    {
        return SendInstanceCommand(handler, WARFRONT_CMD_ADVANCE_STEP, "Advanced the warfront scenario one step.");
    }

    // .warfront boss   - muster the final boss RIGHT HERE, next to you, so the fight lands instantly for testing
    // instead of at the scripted muster point on the far side of the map. Reports where it actually went. If the boss
    // is already up this just reports its position (the muster is one-shot) - use `.warfront goboss` to go to it.
    static bool HandleWarfrontBossCommand(ChatHandler* handler)
    {
        Player* player = nullptr;
        InstanceScript* instance = GetCallerBattleInstance(handler, &player);
        if (!instance || !player)
            return false;

        bool const alreadyUp = !instance->GetGuidData(WARFRONT_DATA_FINAL_BOSS_GUID).IsEmpty();
        instance->SetGuidData(WARFRONT_CMD_MUSTER_BOSS_AT_PLAYER, player->GetGUID());

        Creature* boss = ObjectAccessor::GetCreature(*player, instance->GetGuidData(WARFRONT_DATA_FINAL_BOSS_GUID));
        if (!boss)
        {
            handler->SendSysMessage("Mustered the warfront final boss, but it could not be located afterwards - check the 'warfront' log.");
            return true;
        }

        handler->PSendSysMessage("{} {} is at {:.2f}, {:.2f}, {:.2f} ({:.0f} yd from you). Use '.warfront goboss' to jump to it.",
            alreadyUp ? "The warfront final boss was already mustered -" : "Mustered the warfront final boss here:",
            boss->GetName(), boss->GetPositionX(), boss->GetPositionY(), boss->GetPositionZ(), player->GetExactDist(boss));
        return true;
    }

    // .warfront goboss   - teleport to the final boss of the battle instance you are standing in, if it is already up.
    static bool HandleWarfrontGoBossCommand(ChatHandler* handler)
    {
        Player* player = nullptr;
        InstanceScript* instance = GetCallerBattleInstance(handler, &player);
        if (!instance || !player)
            return false;

        ObjectGuid const bossGuid = instance->GetGuidData(WARFRONT_DATA_FINAL_BOSS_GUID);
        Creature* boss = bossGuid.IsEmpty() ? nullptr : ObjectAccessor::GetCreature(*player, bossGuid);
        if (!boss)
        {
            handler->SendSysMessage("The warfront final boss has not been mustered yet - use '.warfront boss' to summon it here.");
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Land a few yards short of the boss rather than inside it.
        Position dest = boss->GetPosition();
        boss->MovePosition(dest, 5.0f, 0.0f);
        player->TeleportTo(boss->GetMapId(), dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(),
            dest.GetAbsoluteAngle(boss), TELE_TO_NONE, player->GetInstanceId());

        handler->PSendSysMessage("Teleporting you to {} at {:.2f}, {:.2f}, {:.2f}.",
            boss->GetName(), boss->GetPositionX(), boss->GetPositionY(), boss->GetPositionZ());
        return true;
    }
};

void AddSC_warfront_commandscript()
{
    new warfront_commandscript();
}
