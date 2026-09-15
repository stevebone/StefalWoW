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

    // The client renders the error string itself from the reason code, so nothing else needs to be sent.
    auto fail = [&](WorldPackets::Covenant::SoulbindActivationError reason)
    {
        WorldPackets::Covenant::ActivateSoulbindFailed failed;
        failed.Reason = reason;
        failed.SoulbindID = packet.SoulbindID;
        SendPacket(failed.Write());
    };
    using SoulbindError = WorldPackets::Covenant::SoulbindActivationError;

    SoulbindEntry const* soulbind = sSoulbindStore.LookupEntry(packet.SoulbindID);
    if (!soulbind)
    {
        TC_LOG_DEBUG("network", "CMSG_ACTIVATE_SOULBIND: {} sent an unknown SoulbindID {}",
            player->GetGUID().ToString(), packet.SoulbindID);
        fail(SoulbindError::CantDoThatRightNow);
        return;
    }

    // A soulbind belongs to exactly one covenant and may only be activated by a member of it. Without this check a
    // client could send any soulbind id and (before the matching fix in Player::ActivateSoulbind) free-switch its
    // covenant - bypassing the whole covenant-choice flow, its costs and its cooldown.
    if (soulbind->CovenantID <= 0 || uint32(soulbind->CovenantID) != player->GetActiveCovenant())
    {
        TC_LOG_DEBUG("network", "CMSG_ACTIVATE_SOULBIND: {} tried to activate soulbind {} of covenant {} while in covenant {}",
            player->GetGUID().ToString(), soulbind->ID, soulbind->CovenantID, player->GetActiveCovenant());
        fail(SoulbindError::CantDoThatRightNow);
        return;
    }

    // Soulbinds unlock over the covenant campaign; Soulbind.db2 carries the unlock gate (PlayerConditionID 84407-84502).
    if (soulbind->PlayerConditionID > 0 && !ConditionMgr::IsPlayerMeetingCondition(player, uint32(soulbind->PlayerConditionID)))
    {
        TC_LOG_DEBUG("network", "CMSG_ACTIVATE_SOULBIND: {} tried to activate not-yet-unlocked soulbind {} (PlayerCondition {})",
            player->GetGUID().ToString(), soulbind->ID, soulbind->PlayerConditionID);
        fail(SoulbindError::CantDoThatRightNow);
        return;
    }

    if (soulbind->ID == player->GetActiveSoulbind())
        return;     // already active - nothing to do, and not an error

    // Retail refuses a soulbind swap while dead or in combat, and requires a rest area or the sanctum. The client
    // owns the error text for each of these reasons, so only the reason code is sent.
    if (!player->IsAlive())
    {
        fail(SoulbindError::PlayerDead);
        return;
    }

    if (player->IsInCombat())
    {
        fail(SoulbindError::AffectingCombat);
        return;
    }

    if (!CanChangeSoulbind(player))
    {
        fail(SoulbindError::RestArea);
        return;
    }

    player->ActivateSoulbind(soulbind);
}

void WorldSession::HandleRequestCovenantCallings(WorldPackets::Covenant::RequestCovenantCallings& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    // The board is persistent per-character state with a real lifecycle - three slots, one new calling per daily
    // reset, three days of offer life - so it lives on the Player (Player::UpdateCovenantCallings) and is not
    // recomputed per request. The old code here derived the three ids from GameTime/DAY, which meant every member
    // of a covenant saw the same three callings, they changed at midnight UTC instead of at the realm's daily
    // reset, completing one did not replace it, and nothing about the board survived a restart.
    //
    // Rolling the board here as well as at login and at the daily reset costs nothing (it is a no-op once nothing
    // is due) and makes it impossible to answer with a board the reset has already moved past.
    player->UpdateCovenantCallings();
    player->SendCovenantCallingsUpdate();
}

void WorldSession::HandleCovenantRenownRequestCatchupState(WorldPackets::Covenant::CovenantRenownRequestCatchupState& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player)
        return;

    // The packet carries exactly one bit: is accelerated renown catch-up running for this player right now.
    // It is answered from Player::IsCovenantRenownCatchupActive, the single authority for that question - see
    // there for why the answer is currently always "no" and what would have to exist for it to be "yes".
    // Answering false is a true statement about this server rather than a placeholder: no code path grants
    // boosted renown, so claiming otherwise would make the client's renown UI advertise a bonus never paid.
    // The player's actual renown state is not carried by this packet at all - it reaches the client as
    // currency 1822, which Player::SyncCovenantRenownDisplayCurrency keeps equal to the active covenant's
    // renown currency (Covenant.db2 CurrencyTypesID).
    WorldPackets::Covenant::CovenantRenownSendCatchupState response;
    response.IsActive = player->IsCovenantRenownCatchupActive();
    SendPacket(response.Write());
}
