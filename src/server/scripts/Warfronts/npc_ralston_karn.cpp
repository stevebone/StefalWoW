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

// Warfront recruitment officers - the "war table" NPCs that drive the whole warfront loop for the player:
// they report each warfront's live phase and contribution percentage, take war-effort donations while the zone is
// in CONTRIBUTION, and enroll the challenging faction for the assault once it reaches SIEGE.
//   Ralston Karn (142721, Alliance, Boralus,  gossip menu 23182)
//   Throk        (138949, Horde,    Zuldazar, gossip menu 23112)
// Both staff BOTH warfronts, exactly like the retail war table, so each one lists Stromgarde and Darkshore.
//
// The donate option is only offered where it can actually work: the recruiter must carry
// UNIT_NPC_FLAG_2_CONTRIBUTION_COLLECTOR and be authorized for the player's faction bar by CreatureXContribution
// (see sql/updates/world/master/2026_08_07_00_warfront_unlock.sql and the matching hotfixes update).

#include "ScriptMgr.h"
#include "Chat.h"
#include "ContributionMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameTime.h"
#include "GossipDef.h"
#include "Log.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "StringFormat.h"
#include "Util.h"
#include "WarfrontMgr.h"

enum WarfrontGossipData
{
    // The war-table recruiters.
    NPC_RALSTON_KARN            = 142721,   // Alliance
    NPC_THROK                   = 138949,   // Horde

    // Gossip actions (GOSSIP_SENDER_MAIN). The warfront id is folded into the action so one menu can drive both
    // warfronts: action = base + warfrontId.
    GOSSIP_ACTION_STATUS        = 100,      // re-print the current warfront status
    GOSSIP_ACTION_ENROLL        = 200,      // join the assault queue
    GOSSIP_ACTION_LEAVE         = 300,      // leave the assault queue
    GOSSIP_ACTION_DONATE        = 400,      // hand supplies to the war effort (advances the contribution bar)
    GOSSIP_ACTION_OPEN_FINDER   = 500,      // pop the client's native warfront finder (SMSG_OPEN_LFG_DUNGEON_FINDER)
    GOSSIP_ACTION_OPEN_COLLECTOR= 600,      // pop the client's native Contribution Collector frame
};

// Every warfront this recruiter staffs, in war-table order.
static constexpr uint32 RecruiterWarfronts[] = { WARFRONT_STROMGARDE, WARFRONT_DARKSHORE };

struct npc_warfront_recruiter : public CreatureAI
{
    npc_warfront_recruiter(Creature* creature) : CreatureAI(creature) { }

    void UpdateAI(uint32 /*diff*/) override { }

    bool OnGossipHello(Player* player) override
    {
        InitGossipMenuFor(player, 0);
        if (me->IsQuestGiver())
            player->PrepareQuestMenu(me->GetGUID());

        bool any = false;
        for (uint32 warfrontId : RecruiterWarfronts)
        {
            Warfront const* wf = sWarfrontMgr->GetWarfront(warfrontId);
            if (!wf)
                continue;

            any = true;

            // A status line the player can click to re-read (keeps the war-table framing without a custom UI).
            AddGossipItemFor(player, GossipOptionNpc::None, BuildStatusText(player, *wf),
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_STATUS + warfrontId);

            // A locked warfront gets the "why" line and nothing actionable - the player has to advance the war
            // campaign first. This is the Blizzlike gate, not a phase problem, so it is reported separately.
            std::string lockReason;
            if (!sWarfrontMgr->HasUnlockedWarfront(player, warfrontId, &lockReason))
            {
                AddGossipItemFor(player, GossipOptionNpc::None, lockReason,
                    GOSSIP_SENDER_MAIN, GOSSIP_ACTION_STATUS + warfrontId);
                continue;
            }

            // CONTRIBUTION: the war effort is gathering. Offer the donation that actually pushes the bar toward
            // the target - reaching it is what opens the assault, with no GM command involved.
            if (wf->State == WF_CONTRIBUTION)
            {
                if (uint32 const contributionId = GetContributionFor(player, *wf))
                {
                    AddGossipItemFor(player, GossipOptionNpc::None, BuildDonateText(contributionId),
                        GOSSIP_SENDER_MAIN, GOSSIP_ACTION_DONATE + warfrontId);

                    // The client's own Contribution Collector frame. Selecting a GossipOptionNpc::ContributionCollector
                    // option makes Player::OnGossipSelect start a PlayerInteractionType::ContributionCollector
                    // interaction and send SMSG_NPC_INTERACTION_OPEN_RESULT, which is what
                    // PlayerInteractionFrameManager maps to ContributionCollectionFrame. The frame then fills itself
                    // from C_ContributionCollector.GetActive(), i.e. from the CLIENT's CreatureXContribution.db2 -
                    // so it stays behind the same opt-in as the rest of the native UI, because that mapping only
                    // reaches the client once the creature_x_contribution hotfix is advertised in `hotfix_data`.
                    if (WarfrontMgr::IsNativeUiEnabled())
                        AddGossipItemFor(player, GossipOptionNpc::ContributionCollector, "Inspect the war effort.",
                            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_OPEN_COLLECTOR + warfrontId);
                }
            }

            // SIEGE: enroll option, only when this player's faction is the challenger.
            if (sWarfrontMgr->CanQueue(player, warfrontId))
            {
                AddGossipItemFor(player, GossipOptionNpc::None,
                    Trinity::StringFormat("Enroll in the assault: {}.", wf->Name),
                    GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ENROLL + warfrontId);
            }
            else if (WarfrontQueue const* queue = sWarfrontMgr->GetQueue(warfrontId))
            {
                if (queue->IsEnrolled(player->GetGUID()))
                    AddGossipItemFor(player, GossipOptionNpc::None,
                        Trinity::StringFormat("Withdraw from the assault: {}.", wf->Name),
                        GOSSIP_SENDER_MAIN, GOSSIP_ACTION_LEAVE + warfrontId);
            }
        }

        if (!any)
            return false;   // fall back to default gossip if no warfront is loaded

        // Native war table: pops the client's own dungeon-finder panel preselected to this warfront, whose
        // "Join Battle" button then round-trips CMSG_DF_JOIN back to WorldSession::HandleLfgJoinOpcode (which
        // intercepts warfront LFGDungeons ids and routes them here). SMSG_OPEN_LFG_DUNGEON_FINDER's body is
        // INFERRED, so the option only appears when the operator opted in via Warfront.NativeUI.Enable.
        if (WarfrontMgr::IsNativeUiEnabled())
        {
            AddGossipItemFor(player, GossipOptionNpc::None, "Open the Warfronts table.",
                GOSSIP_SENDER_MAIN, uint32(GOSSIP_ACTION_OPEN_FINDER) + uint32(WARFRONT_STROMGARDE));
        }

        SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        uint32 const action = GetGossipActionFor(player, gossipListId);
        uint32 const base = (action / 100) * 100;
        uint32 const warfrontId = action - base;

        switch (base)
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
            case GOSSIP_ACTION_DONATE:
            {
                Warfront const* wf = sWarfrontMgr->GetWarfront(warfrontId);
                uint32 const contributionId = wf ? GetContributionFor(player, *wf) : 0;
                if (!contributionId)
                {
                    Notify(player, "The war effort is not taking supplies here.");
                    CloseGossipMenuFor(player);
                    break;
                }

                ContributionResult const result = sContributionMgr->Contribute(player, me->GetGUID(), contributionId);
                Notify(player, ContributionMgr::GetResultText(result));

                // Re-open so the donor immediately sees the bar they just moved (and, if the donation filled it,
                // the assault option that just appeared).
                OnGossipHello(player);
                break;
            }
            case GOSSIP_ACTION_OPEN_COLLECTOR:
                // Hand the option back to Player::OnGossipSelect, whose generic GossipOptionNpc ->
                // PlayerInteractionType mapping is what actually opens the native collector frame.
                return false;
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

    // The contribution this recruiter collects for the player's own faction bar of this warfront (0 = none, which
    // is also what happens when the collector data has not been applied - the option is then simply not offered).
    uint32 GetContributionFor(Player const* player, Warfront const& wf) const
    {
        uint32 const managedWorldStateId = wf.GetContributionMWS(player->GetTeamId());
        if (!managedWorldStateId)
            return 0;

        uint32 const contributionId = sContributionMgr->GetContributionForCreatureAndManagedWorldState(me->GetEntry(), managedWorldStateId);
        if (!contributionId || !sContributionMgr->IsCollectorFor(me, contributionId))
            return 0;

        return contributionId;
    }

    static std::string BuildDonateText(uint32 contributionId)
    {
        ContributionMgr::ContributionCost cost;
        if (sContributionMgr->GetContributionCost(contributionId, cost) && cost.CurrencyID && cost.CurrencyAmount)
            return Trinity::StringFormat("Donate {} supplies to the war effort.", cost.CurrencyAmount);

        return "Donate supplies to the war effort.";
    }

    // "<Warfront>: held by X - war effort NN% (M more needed)" / "... - the assault is underway, N left".
    std::string BuildStatusText(Player const* player, Warfront const& wf) const
    {
        char const* controller = wf.ControllingTeam == TEAM_ALLIANCE ? "the Alliance" : "the Horde";
        time_t const now = GameTime::GetGameTime();

        if (wf.State == WF_SIEGE)
        {
            std::string remaining = (wf.PhaseEndTime > now)
                ? secsToTimeString(uint64(wf.PhaseEndTime - now), TimeFormat::ShortText, true)
                : std::string("moments");

            return Trinity::StringFormat("{}: held by {} - the assault is underway ({} left).",
                wf.Name, controller, remaining);
        }

        // CONTRIBUTION (and the transient FLIP): report the challenger's bar, because that is the one whose target
        // opens the assault, plus this player's own bar when they are not the challenger.
        float fraction = 0.0f;
        int32 progress = 0;
        int32 target = 0;
        if (sWarfrontMgr->GetContributionProgress(wf.Id, wf.ChallengingTeam, fraction, progress, target))
        {
            bool const mine = player->GetTeamId() == wf.ChallengingTeam;
            char const* whose = mine ? "Your" : (wf.ChallengingTeam == TEAM_ALLIANCE ? "The Alliance" : "The Horde");
            return Trinity::StringFormat("{}: held by {}. {} war effort stands at {}% - the assault begins at 100%.",
                wf.Name, controller, whose, uint32(fraction * 100.0f));
        }

        return Trinity::StringFormat("{}: held by {}. The war effort gathers strength; the assault is not yet ready.",
            wf.Name, controller);
    }
};

void AddSC_npc_warfront_recruiter()
{
    RegisterCreatureAI(npc_warfront_recruiter);
}
