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

#include "DelvePackets.h"
#include "Log.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldSession.h"

void WorldSession::HandleDelveTeleportOut(WorldPackets::Delve::DelveTeleportOut& /*delveTeleportOut*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;
    Map* map = player->GetMap();
    if (!map)
        return;

    TC_LOG_DEBUG("network", "CMSG_DELVE_TELEPORT_OUT: Player {} requested Delve teleport out from Map {}",
        player->GetName(), map->GetId());

    switch (map->GetId())
    {
        case 2962: // Atal'Aman
            player->TeleportTo(0, 5121.0f, -5861.4f, 217.1f, 3.22f, TELE_TO_SEAMLESS);
            break;
        case 2952: // Shadow Enclave
            player->TeleportTo(0, 4781.7f, -4120.3f, 31.3f, 0.0f, TELE_TO_SEAMLESS);
            break;
        default:
            if (AreaTriggerTeleport const* at = sObjectMgr->GetGoBackTrigger(map->GetId()))
                player->TeleportTo(at->Loc);
            else
                player->TeleportTo(player->m_homebind);
            break;
    }
}

void WorldSession::HandleRequestPartyEligibilityForDelveTiers(WorldPackets::Delve::RequestPartyEligibilityForDelveTiers& /*request*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    TC_LOG_DEBUG("network", "CMSG_REQUEST_PARTY_ELIGIBILITY_FOR_DELVE_TIERS: Player {} requested tier eligibility",
        player->GetName());

    WorldPackets::Delve::PartyEligibilityForDelveTiersResponse response;
    response.IsPartyEligible = true;

    for (int32 tier = 1; tier <= 11; ++tier)
    {
        WorldPackets::Delve::PartyEligibilityForDelveTiersResponse::TierInfo tierInfo;
        tierInfo.Tier = tier;
        tierInfo.Eligible = true;
        response.Tiers.push_back(tierInfo);
    }

    SendPacket(response.Write());
}
