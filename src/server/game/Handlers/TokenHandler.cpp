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

        // Left at 0: how long a token takes to sell is a statistic retail derives from its own sales
        // history, and we have none to derive it from. 0 is honest here rather than an invented estimate.
        response.ExpectedSecondsUntilSold = 0;
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
    // The trailing uint64 is the account's remaining gold: the client's handler stores it straight into
    // the global that C_WowTokenGlue.GetAccountRemainingGoldAmount() returns, and AccountReactivate.lua
    // renders it as ACCOUNT_REACTIVATE_GOLD_REMAINING. This is answered at character select, so no
    // Player is loaded and the total has to come from the database.
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_TOTAL_MONEY);
    stmt->setUInt32(0, GetAccountId());

    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt)
        .WithPreparedCallback([this, clientToken = consumableTokenCanVeteranBuy.ClientToken](PreparedQueryResult result)
    {
        WorldPackets::Token::ConsumableTokenCanVeteranBuyResponse response;

        response.ClientToken = clientToken;

        // Buying a token for gold is only possible while somebody has one listed on the market, so
        // the answer is read out of the market rather than assumed. The client reads result == 0 as
        // "can buy" and treats ERROR_NONE_FOR_SALE as the other meaningful outcome.
        response.Result = sWowTokenMgr->GetListedTokenCount() ? TOKEN_RESULT_SUCCESS : TOKEN_RESULT_ERROR_NONE_FOR_SALE;
        response.RemainingGoldAmount = result ? (*result)[0].GetUInt64() : UI64LIT(0);

        SendPacket(response.Write());
    }));
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

void WorldSession::SendGenerateSsoToken()
{
    WorldPackets::Token::GenerateSsoTokenResponse ssoToken;

    ssoToken.Success = 1;
    ssoToken.Result = TOKEN_RESULT_SUCCESS;
    ssoToken.Issued = GameTime::GetGameTime();
    ssoToken.Expires = GameTime::GetGameTime() + WowTokenMgr::SSO_TOKEN_DURATION;
    ssoToken.Token = WowTokenMgr::GenerateSsoToken();

    SendPacket(ssoToken.Write());
}
