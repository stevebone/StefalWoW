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

// Warfront recruitment officers - the "war table" NPCs that show a warfront's status and let the challenging
// faction enroll for the assault once the zone is in SIEGE. See WARFRONTS_DESIGN.md §P2 / §3.
//   Ralston Karn (142721, Alliance, Boralus, gossip menu 23182)
//   Throk        (138949, Horde,    Zuldazar, gossip menu 23112)
// Both drive the Battle for Stromgarde (Arathi). The Darkshore war table (Nathanos 146323 / a Kul Tiran mirror)
// reuses this same script once its warfront is wired for queueing.

#include "ScriptMgr.h"
#include "Chat.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GossipDef.h"
#include "Log.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "StringFormat.h"
#include "WarfrontMgr.h"

enum WarfrontGossipData
{
    // The war-table recruiters.
    NPC_RALSTON_KARN            = 142721,   // Alliance
    NPC_THROK                   = 138949,   // Horde

    // Gossip actions (GOSSIP_SENDER_MAIN).
    GOSSIP_ACTION_STATUS        = 1,        // re-print the current warfront status
    GOSSIP_ACTION_ENROLL        = 2,        // join the assault queue
    GOSSIP_ACTION_LEAVE         = 3,        // leave the assault queue
    GOSSIP_ACTION_OPEN_FINDER   = 4,        // pop the client's native warfront finder (SMSG_OPEN_LFG_DUNGEON_FINDER)
};

// Maps a recruiter creature entry to the warfront it staffs. Both Stromgarde recruiters for now.
static uint32 GetWarfrontForRecruiter(uint32 /*creatureEntry*/)
{
    return WARFRONT_STROMGARDE;
}

struct npc_warfront_recruiter : public CreatureAI
{
    npc_warfront_recruiter(Creature* creature) : CreatureAI(creature) { }

    void UpdateAI(uint32 /*diff*/) override { }

    bool OnGossipHello(Player* player) override
    {
        uint32 const warfrontId = GetWarfrontForRecruiter(me->GetEntry());
        Warfront const* wf = sWarfrontMgr->GetWarfront(warfrontId);
        if (!wf)
            return false;   // fall back to default gossip if the warfront isn't loaded

        InitGossipMenuFor(player, 0);
        if (me->IsQuestGiver())
            player->PrepareQuestMenu(me->GetGUID());

        // A status line the player can click to re-read (keeps the war-table framing without a custom UI).
        AddGossipItemFor(player, GossipOptionNpc::None, BuildStatusText(*wf),
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_STATUS);

        // Enroll option: only when this player's faction is the challenger and the zone is in SIEGE.
        if (sWarfrontMgr->CanQueue(player, warfrontId))
        {
            AddGossipItemFor(player, GossipOptionNpc::None, "Enroll in the assault on Stromgarde.",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ENROLL);
        }
        else if (WarfrontQueue const* queue = sWarfrontMgr->GetQueue(warfrontId))
        {
            if (queue->IsEnrolled(player->GetGUID()))
                AddGossipItemFor(player, GossipOptionNpc::None, "Withdraw from the assault.",
                    GOSSIP_SENDER_MAIN, GOSSIP_ACTION_LEAVE);
        }

        // Native war table: pops the client's own dungeon-finder panel preselected to this warfront, whose
        // "Join Battle" button then round-trips CMSG_DF_JOIN back to WorldSession::HandleLfgJoinOpcode (which
        // intercepts warfront LFGDungeons ids and routes them here). SMSG_OPEN_LFG_DUNGEON_FINDER's body is
        // INFERRED, so the option only appears when the operator opted in via Warfront.NativeUI.Enable.
        if (WarfrontMgr::IsNativeUiEnabled())
        {
            AddGossipItemFor(player, GossipOptionNpc::None, "Open the Warfronts table.",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_OPEN_FINDER);
        }

        SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        uint32 const action = GetGossipActionFor(player, gossipListId);
        uint32 const warfrontId = GetWarfrontForRecruiter(me->GetEntry());

        switch (action)
        {
            case GOSSIP_ACTION_ENROLL:
            {
                std::string reason;
                if (sWarfrontMgr->EnqueuePlayer(player, warfrontId, &reason))
                    Notify(player, "You have joined the assault. Stand ready - you will be summoned when the war party musters.");
                else
                    Notify(player, reason.empty() ? "You cannot join the assault right now." : reason);
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_LEAVE:
            {
                if (WarfrontQueue* queue = sWarfrontMgr->GetQueue(warfrontId))
                    queue->Dequeue(player->GetGUID());
                Notify(player, "You have withdrawn from the assault.");
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_OPEN_FINDER:
            {
                if (!sWarfrontMgr->SendOpenLfgDungeonFinder(player, warfrontId))
                    Notify(player, "The war table is unavailable right now.");
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_STATUS:
            default:
                // Re-open the menu so the status refreshes in place.
                OnGossipHello(player);
                break;
        }
        return true;
    }

private:
    static void Notify(Player* player, std::string const& msg)
    {
        ChatHandler(player->GetSession()).SendSysMessage(msg.c_str());
    }

    static std::string BuildStatusText(Warfront const& wf)
    {
        char const* controller = wf.ControllingTeam == TEAM_ALLIANCE ? "the Alliance" : "the Horde";
        switch (wf.State)
        {
            case WF_SIEGE:
                return Trinity::StringFormat("Arathi is held by {} - the assault is underway! Speak with me to join the fight.", controller);
            case WF_CONTRIBUTION:
            default:
                return Trinity::StringFormat("Arathi is held by {}. The war effort gathers strength; the assault is not yet ready.", controller);
        }
    }
};

void AddSC_npc_warfront_recruiter()
{
    RegisterCreatureAI(npc_warfront_recruiter);
}
