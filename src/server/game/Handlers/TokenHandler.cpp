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

#include "TokenPackets.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "WorldSession.h"
#include "WowTokenMgr.h"

void WorldSession::HandleCommerceTokenGetLog(WorldPackets::Token::CommerceTokenGetLog& commerceTokenGetLog)
{
    WorldPackets::Token::CommerceTokenGetLogResponse response;

    response.ClientToken = commerceTokenGetLog.ClientToken;
    response.Result = TOKEN_RESULT_ERROR_DISABLED;

    SendPacket(response.Write());
}

void WorldSession::HandleCommerceTokenGetMarketPrice(WorldPackets::Token::CommerceTokenGetMarketPrice& commerceTokenGetMarketPrice)
{
    WorldPackets::Token::CommerceTokenGetMarketPriceResponse response;

    response.ClientToken = commerceTokenGetMarketPrice.ClientToken;

    // The market price is the cheapest listing actually on the market, not a configured constant. With
    // nothing listed there is no price to quote, which is exactly ERROR_NONE_FOR_SALE.
    if (uint64 lowestPrice = sWowTokenMgr->GetLowestListingPrice())
    {
        response.Result = TOKEN_RESULT_SUCCESS;
        response.Price = lowestPrice;

        // Retail always sends 14400 (4 hours) here - it is a constant in all 9 captured responses, not a
        // statistic derived from sales history. Match it. See COMMERCE_AUDIT C-19.
        response.ExpectedSecondsUntilSold = 14400;
    }
    else
        response.Result = TOKEN_RESULT_ERROR_NONE_FOR_SALE;

    SendPacket(response.Write());
}

void WorldSession::HandleCommerceTokenGetCount(WorldPackets::Token::CommerceTokenGetCount& commerceTokenGetCount)
{
    WorldPackets::Token::CommerceTokenGetCountResponse response;

    response.ClientToken = commerceTokenGetCount.ClientToken;
    response.Result = TOKEN_RESULT_SUCCESS;
    response.AuctionableTokenIDs = sWowTokenMgr->GetAuctionableTokens(GetAccountId());
    response.ConsumableTokenIDs = sWowTokenMgr->GetConsumableTokens(GetAccountId());

    SendPacket(response.Write());
}

void WorldSession::HandleConsumableTokenCanVeteranBuy(WorldPackets::Token::ConsumableTokenCanVeteranBuy& consumableTokenCanVeteranBuy)
{
    // "Veteran buy" is the retail flow where a lapsed-subscription account buys a token with gold to
    // reactivate game time (AccountReactivate.lua). TrinityCore has no subscriptions, so no account is
    // ever eligible - which is exactly what retail reports here as well: all 22 captured responses are
    // {Result = ERROR_DISABLED, RemainingGold = 0}. Answer that verbatim as the default ineligible reply.
    //
    // The success path an eligible account would receive is {Result = SUCCESS (or SUCCESS_NO), gold},
    // where the trailing u64 feeds C_WowTokenGlue.GetAccountRemainingGoldAmount(); it has no server-side
    // counterpart here (no veteran/subscription concept) so it is deliberately not produced. See
    // COMMERCE_AUDIT C-21 / WOW_TOKEN_RE_68275.md.
    WorldPackets::Token::ConsumableTokenCanVeteranBuyResponse response;
    response.ClientToken = consumableTokenCanVeteranBuy.ClientToken;
    response.Result = TOKEN_RESULT_ERROR_DISABLED;
    response.RemainingGoldAmount = 0;
    SendPacket(response.Write());
}

void WorldSession::HandleCanRedeemTokenForBalance(WorldPackets::Token::CanRedeemTokenForBalance& /*canRedeemTokenForBalance*/)
{
    // Deliberately unanswered. SMSG_CAN_REDEEM_TOKEN_FOR_BALANCE_RESPONSE appears in none of the nine
    // 12.0.7 captures while the request appears 19 times, i.e. retail itself stays silent - replying
    // would be less correct than not replying, and Battle.net balance has no server-side counterpart
    // here anyway. The request is still parsed so it is not reported as an unhandled opcode.
}

void WorldSession::SendCommerceTokenUpdate()
{
    WorldPackets::Token::CommerceTokenUpdate tokenUpdate;

    tokenUpdate.AuctionableTokenIDs = sWowTokenMgr->GetAuctionableTokens(GetAccountId());
    tokenUpdate.ConsumableTokenIDs = sWowTokenMgr->GetConsumableTokens(GetAccountId());

    SendPacket(tokenUpdate.Write());
}

void WorldSession::SendGenerateSsoToken(uint32 clientToken)
{
    WorldPackets::Token::GenerateSsoTokenResponse ssoToken;

    // Echo the ClientToken from the CMSG_BATTLE_PAY_OPEN_CHECKOUT that triggered this (1:1 answer).
    ssoToken.ClientToken = clientToken;
    ssoToken.Result = TOKEN_RESULT_SUCCESS;
    ssoToken.Issued = GameTime::GetGameTime();
    ssoToken.Expires = GameTime::GetGameTime() + WowTokenMgr::SSO_TOKEN_DURATION;
    ssoToken.Token = WowTokenMgr::GenerateSsoToken(GetBattlenetAccountId());

    SendPacket(ssoToken.Write());
}
