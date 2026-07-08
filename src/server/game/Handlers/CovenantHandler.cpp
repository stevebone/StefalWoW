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
#include "CovenantPackets.h"
#include "DB2Stores.h"
#include "GameTime.h"
#include "Log.h"
#include "Player.h"
#include <algorithm>

void WorldSession::HandleActivateSoulbind(WorldPackets::Covenant::ActivateSoulbind& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    SoulbindEntry const* soulbind = sSoulbindStore.LookupEntry(packet.SoulbindID);
    if (!soulbind)
    {
        TC_LOG_DEBUG("network", "CMSG_ACTIVATE_SOULBIND: {} sent an unknown SoulbindID {}",
            player->GetGUID().ToString(), packet.SoulbindID);
        return;
    }

    player->ActivateSoulbind(soulbind);
}

void WorldSession::HandleRequestCovenantCallings(WorldPackets::Covenant::RequestCovenantCallings& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    // MaxCallings client constant (CovenantCallingsConstants.Callings.MaxCallings): at most 3 daily callings are offered.
    constexpr uint32 MaxCallings = 3;

    WorldPackets::Covenant::CovenantCallingsAvailabilityResponse response;

    uint32 covenantId = player->GetActiveCovenant();
    if (CovenantEntry const* covenant = covenantId ? sCovenantStore.LookupEntry(covenantId) : nullptr)
    {
        // A covenant with no bounty set offers no callings; otherwise callings unlock once the
        // set's locking quest (if any) is completed.
        response.CallingsUnlocked = covenant->BountySetID != 0;
        if (BountySetEntry const* bountySet = sBountySetStore.LookupEntry(covenant->BountySetID))
            if (bountySet->LockedQuestID > 0 && !player->IsQuestRewarded(bountySet->LockedQuestID))
                response.CallingsUnlocked = false;

        if (response.CallingsUnlocked)
        {
            if (std::vector<BountyEntry const*> const* bounties = sDB2Manager.GetBountiesForBountySet(covenant->BountySetID))
            {
                std::vector<BountyEntry const*> eligible;
                eligible.reserve(bounties->size());
                for (BountyEntry const* bounty : *bounties)
                    if (bounty->QuestID > 0)
                        eligible.push_back(bounty);

                std::sort(eligible.begin(), eligible.end(), [](BountyEntry const* left, BountyEntry const* right)
                {
                    return left->ID < right->ID;
                });

                // Offer up to MaxCallings real callings from the covenant's bounty pool, rotating each day so the
                // set is stable within a day and refreshes daily (Blizzlike daily callings behaviour).
                if (!eligible.empty())
                {
                    uint32 const poolSize = uint32(eligible.size());
                    uint32 const dayIndex = uint32(GameTime::GetGameTime() / DAY);
                    uint32 const offerCount = std::min<uint32>(MaxCallings, poolSize);
                    for (uint32 i = 0; i < offerCount; ++i)
                        response.BountyIDs.push_back(eligible[(dayIndex + i) % poolSize]->ID);
                }
            }
        }
    }

    SendPacket(response.Write());
}
