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

#ifndef TRINITYCORE_WOW_TOKEN_MGR_H
#define TRINITYCORE_WOW_TOKEN_MGR_H

#include "Define.h"
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

enum WowTokenState : uint8
{
    // Owned by an account and sellable on the token market.
    WOW_TOKEN_STATE_AUCTIONABLE = 0,
    // Owned by an account and redeemable.
    WOW_TOKEN_STATE_CONSUMABLE  = 1,
    // Currently listed on the token market, so owned by nobody for display purposes.
    WOW_TOKEN_STATE_LISTED      = 2
};

struct WowToken
{
    uint64 Id            = 0;
    uint32 OwnerAccount  = 0;
    WowTokenState State  = WOW_TOKEN_STATE_AUCTIONABLE;
    uint64 Price         = 0;   // copper; only meaningful while WOW_TOKEN_STATE_LISTED
    time_t CreateTime    = 0;
};

// Account-level WoW Token holdings and the token market.
//
// Holdings live in the auth database because the client asks for them at character select, before any
// Player exists (CMSG_COMMERCE_TOKEN_GET_COUNT is answered at STATUS_AUTHED - see WOW_TOKEN_RE_68275.md
// for the sniff evidence). Everything the handlers put on the wire is read back out of this state; no
// count, price or eligibility answer is synthesised.
class TC_GAME_API WowTokenMgr
{
public:
    static WowTokenMgr* instance();

    void Load();

    // Token ids held by an account, split the way the client's two parallel uint64 lists expect.
    std::vector<uint64> GetAuctionableTokens(uint32 accountId) const;
    std::vector<uint64> GetConsumableTokens(uint32 accountId) const;

    // Market state.
    uint32 GetListedTokenCount() const;
    uint64 GetLowestListingPrice() const;

    WowToken const* GetToken(uint64 tokenId) const;

    // Creates a token owned by an account and persists it. Returns the new token id.
    uint64 CreateToken(uint32 accountId, WowTokenState state);

    // Moves a token between states (and optionally owners), persisting the change.
    bool SetTokenState(uint64 tokenId, WowTokenState state, uint32 ownerAccount, uint64 price = 0);

    // Generates a fresh single sign-on token for a session, of the form "<REGION>-<42 hex chars>",
    // matching the shape observed on retail. The value is random per call; it is not derived from
    // anything guessable.
    static std::string GenerateSsoToken();

    // Retail issues the SSO token with a four hour lifetime (observed expiry - issued == 14400s).
    static constexpr uint32 SSO_TOKEN_DURATION = 4 * 60 * 60;

private:
    WowTokenMgr() = default;
    ~WowTokenMgr() = default;
    WowTokenMgr(WowTokenMgr const&) = delete;
    WowTokenMgr& operator=(WowTokenMgr const&) = delete;

    std::unordered_map<uint64, WowToken> _tokens;
    uint64 _maxTokenId = 0;
};

#define sWowTokenMgr WowTokenMgr::instance()

#endif // TRINITYCORE_WOW_TOKEN_MGR_H
