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
#include "BattlePayMgr.h"
#include "BattlePayPackets.h"
#include "Log.h"
#include "Player.h"

namespace
{
    // BattlepayPurchaseStatus / PurchaseResult (extracted from the client enum registrar).
    constexpr int32 STATUS_DONE   = 3;
    constexpr int32 STATUS_FAILED = 4;
    constexpr int32 RESULT_OK                       = 0;
    constexpr int32 RESULT_NOT_ENOUGH_BALANCE       = 29;
    constexpr int32 RESULT_PRODUCT_NOT_PURCHASABLE  = 57;
}

// In-game Shop (BattlePay). P0: reply to the catalog request with the captured, client-validated
// product list so the shop opens and displays real products. If no catalog blob is loaded we send
// nothing (shop opens empty) rather than fabricating wire.
void WorldSession::HandleBattlePayGetProductList(WorldPackets::BattlePay::GetProductList& /*getProductList*/)
{
    if (!sBattlePayMgr->HasCatalog())
    {
        TC_LOG_DEBUG("network", "BattlePay: GetProductList from {} but no catalog loaded.", GetPlayerInfo());
        return;
    }

    WorldPackets::BattlePay::ProductListResponse response;
    response.RawData = &sBattlePayMgr->GetProductListBlob();
    SendPacket(response.Write());
}

// Drives the purchase to completion for a given productID: validate, charge (gold or a token item),
// grant, then send the client the start-purchase ack + a PurchaseUpdate (Done/Failed). The granted
// item/spell reaches the client over the normal item/collection packets regardless of the shop UI.
void WorldSession::BattlePayProcessPurchase(uint32 productID)
{
    Player* player = GetPlayer();
    uint64 const purchaseID = sBattlePayMgr->GeneratePurchaseID();

    auto respond = [&](int32 status, int32 resultCode, uint64 price)
    {
        WorldPackets::BattlePay::StartPurchaseResponse ack;
        ack.PurchaseID = purchaseID;
        ack.ResultB = uint32(resultCode);
        SendPacket(ack.Write());

        WorldPackets::BattlePay::PurchaseUpdate update;
        WorldPackets::BattlePay::PurchaseRecord& rec = update.Purchases.emplace_back();
        rec.PurchaseID = purchaseID;
        rec.Status = status;
        rec.ResultCode = resultCode;
        rec.ProductID = productID;
        rec.BasePrice = price;
        rec.UserPrice = price;
        SendPacket(update.Write());
    };

    BattlePayProduct const* product = player ? sBattlePayMgr->GetProduct(productID) : nullptr;
    if (!product)
    {
        TC_LOG_DEBUG("network", "BattlePay: purchase of unknown product {} by {}.", productID, GetPlayerInfo());
        respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, 0);
        return;
    }

    // Cost check (gold and/or a token item).
    if (product->CostMoney && !player->HasEnoughMoney(product->CostMoney))
    {
        respond(STATUS_FAILED, RESULT_NOT_ENOUGH_BALANCE, product->CostMoney);
        return;
    }
    if (product->CostItemId && !player->HasItemCount(product->CostItemId, product->CostItemCount))
    {
        respond(STATUS_FAILED, RESULT_NOT_ENOUGH_BALANCE, product->CostMoney);
        return;
    }

    // Grant first; only charge if the grant succeeds so we never take payment without delivering.
    bool granted = false;
    switch (product->GrantType)
    {
        case 1: // item
            granted = player->AddItem(product->GrantId, product->GrantCount);
            break;
        case 2: // spell (mount / toy / appearance)
            if (!player->HasSpell(product->GrantId))
                player->LearnSpell(product->GrantId, false);
            granted = true;
            break;
        default:
            break;
    }

    if (!granted)
    {
        TC_LOG_DEBUG("network", "BattlePay: grant failed for product {} ({}), {} not charged.",
            productID, product->Name, GetPlayerInfo());
        respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, product->CostMoney);
        return;
    }

    if (product->CostMoney)
        player->ModifyMoney(-int64(product->CostMoney));
    if (product->CostItemId)
        player->DestroyItemCount(product->CostItemId, product->CostItemCount, true);

    TC_LOG_INFO("network", "BattlePay: {} purchased product {} ({}) for {} copper / {}x item {}.",
        GetPlayerInfo(), productID, product->Name, product->CostMoney, product->CostItemCount, product->CostItemId);

    respond(STATUS_DONE, RESULT_OK, product->CostMoney);
}

void WorldSession::HandleBattlePayStartPurchase(WorldPackets::BattlePay::StartPurchase& startPurchase)
{
    // The productID scalar is a strong candidate (setter is Warden-obfuscated). Log all three fields so a
    // live purchase confirms which scalar is the productID; GetProduct() also guards against a wrong guess.
    TC_LOG_INFO("network", "BattlePay: StartPurchase from {}: u32={} u64={} flag={}",
        GetPlayerInfo(), startPurchase.ProductID, startPurchase.ScalarU64, uint32(startPurchase.Flag));

    BattlePayProcessPurchase(startPurchase.ProductID);
}

void WorldSession::HandleBattlePayOpenCheckout(WorldPackets::BattlePay::OpenCheckout& openCheckout)
{
    // Retail routes purchases here (-> Battle.net web-checkout) using a distributionID rather than a
    // productID. We cannot map that to a product without the distribution setup, so log it for diagnosis;
    // server-defined products are purchased via StartPurchase above.
    TC_LOG_INFO("network", "BattlePay: OpenCheckout from {} distributionID={} (web-checkout path, not handled).",
        GetPlayerInfo(), openCheckout.DistributionID);
}
