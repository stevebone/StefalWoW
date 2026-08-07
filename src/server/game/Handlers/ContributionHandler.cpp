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

#include "WorldSession.h"
#include "Chat.h"
#include "ContributionMgr.h"
#include "ContributionPackets.h"
#include "Log.h"
#include "Player.h"

void WorldSession::HandleContributionContribute(WorldPackets::Contribution::ContributionContribute& contribute)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    // The payload id is the contribution id (C_ContributionCollector.Contribute(contributionID) - the collector's
    // OrderIndex is a client-side Contribution.db2 lookup and never travels on the wire).
    ContributionResult const result = sContributionMgr->Contribute(player, contribute.CollectorGUID, contribute.ContributionID);

    // The client learns the outcome through CONTRIBUTION_COLLECTOR_PENDING, whose server message body is not
    // recovered, so the refusal is reported in chat instead of being silently swallowed.
    if (result != ContributionResult::Success)
        ChatHandler(this).SendSysMessage(ContributionMgr::GetResultText(result));

    // The bar itself moves through the realm-wide world-state broadcast off ManagedWorldStateMgr::PushProgress; this
    // only acks the collector's last-update timestamp. No-op unless Warfront.NativeUI.Enable = 1.
    sContributionMgr->SendLastUpdate(player, contribute.ContributionID);
}

// CMSG_CONTRIBUTION_LAST_UPDATE_REQUEST (0x3B00FE) - { uint32 ContributionID, uint32 ContributionGUID }. The layout is
// byte-exact from the 68275 client serializer at VA 0x7FF729154070 and matches shipped BfA-era server code
// (ContributionGetState::Read - two consecutive uint32).
void WorldSession::HandleContributionLastUpdateRequest(WorldPackets::Contribution::ContributionLastUpdateRequest& request)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    TC_LOG_INFO("warfront", "CMSG_CONTRIBUTION_LAST_UPDATE_REQUEST from {} ({}): ContributionID = {}, ContributionGUID = {} (0x{:08X})",
        player->GetName(), player->GetGUID().ToString(), request.ContributionID, request.ContributionGUID, request.ContributionGUID);

    // Twelve-byte timestamp ack, echoing the requested ids back. Still behind Warfront.NativeUI.Enable (default 0)
    // until a 12.0.7 sniff confirms the opcode is unchanged from the BfA shape.
    sContributionMgr->SendLastUpdate(player, request.ContributionID, request.ContributionGUID);
}
