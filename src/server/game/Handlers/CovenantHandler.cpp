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
#include "ConditionMgr.h"
#include "DB2Stores.h"
#include "Garrison.h"
#include "GameTime.h"
#include "Log.h"
#include "MiscPackets.h"
#include "Player.h"
#include "RestMgr.h"
#include "SharedDefines.h"
#include <algorithm>

// True while the player may change soulbinds: retail allows it in a rest area or inside the covenant sanctum.
// The exact sanctum area list is not derivable offline, so the sanctum test uses the maps that GarrSite 296
// publishes through GarrSiteLevel.db2 (no hardcoded map ids). This errs permissive on purpose - a false negative
// would lock a player out of their own soulbinds, which is far worse than a false positive.
static bool CanChangeSoulbind(Player const* player)
{
    if (player->GetRestMgr().HasRestFlag(REST_FLAG_IN_TAVERN)
        || player->GetRestMgr().HasRestFlag(REST_FLAG_IN_CITY)
        || player->GetRestMgr().HasRestFlag(REST_FLAG_IN_FACTION_AREA))
        return true;

    for (GarrSiteLevelEntry const* siteLevel : sGarrSiteLevelStore)
        if (siteLevel->GarrSiteID == GARR_SITE_COVENANT_SANCTUM && siteLevel->MapID == player->GetMapId())
            return true;

    return false;
}

void WorldSession::HandleActivateSoulbind(WorldPackets::Covenant::ActivateSoulbind& packet)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    auto fail = [&]
    {
        WorldPackets::Covenant::ActivateSoulbindFailed failed;
        failed.SoulbindID = packet.SoulbindID;
        SendPacket(failed.Write());
    };

    SoulbindEntry const* soulbind = sSoulbindStore.LookupEntry(packet.SoulbindID);
    if (!soulbind)
    {
        TC_LOG_DEBUG("network", "CMSG_ACTIVATE_SOULBIND: {} sent an unknown SoulbindID {}",
            player->GetGUID().ToString(), packet.SoulbindID);
        fail();
        return;
    }

    // A soulbind belongs to exactly one covenant and may only be activated by a member of it. Without this check a
    // client could send any soulbind id and (before the matching fix in Player::ActivateSoulbind) free-switch its
    // covenant - bypassing the whole covenant-choice flow, its costs and its cooldown.
    if (soulbind->CovenantID <= 0 || uint32(soulbind->CovenantID) != player->GetActiveCovenant())
    {
        TC_LOG_DEBUG("network", "CMSG_ACTIVATE_SOULBIND: {} tried to activate soulbind {} of covenant {} while in covenant {}",
            player->GetGUID().ToString(), soulbind->ID, soulbind->CovenantID, player->GetActiveCovenant());
        fail();
        return;
    }

    // Soulbinds unlock over the covenant campaign; Soulbind.db2 carries the unlock gate (PlayerConditionID 84407-84502).
    if (soulbind->PlayerConditionID > 0 && !ConditionMgr::IsPlayerMeetingCondition(player, uint32(soulbind->PlayerConditionID)))
    {
        TC_LOG_DEBUG("network", "CMSG_ACTIVATE_SOULBIND: {} tried to activate not-yet-unlocked soulbind {} (PlayerCondition {})",
            player->GetGUID().ToString(), soulbind->ID, soulbind->PlayerConditionID);
        fail();
        return;
    }

    if (soulbind->ID == player->GetActiveSoulbind())
        return;     // already active - nothing to do, and not an error

    if (!CanChangeSoulbind(player))
    {
        player->SendDirectMessage(WorldPackets::Misc::DisplayGameError(GameError::ERR_ACTIVATE_SOULBIND_FAILED_REST_AREA).Write());
        fail();
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

void WorldSession::HandleCovenantRenownRequestCatchupState(WorldPackets::Covenant::CovenantRenownRequestCatchupState& /*packet*/)
{
    // Core does not implement accelerated renown catch-up, so report it as inactive. Answers the client's query so
    // its renown UI stops waiting (mirrors the default-response pattern of the other covenant info handlers).
    WorldPackets::Covenant::CovenantRenownSendCatchupState response;
    response.IsActive = false;
    SendPacket(response.Write());
}
