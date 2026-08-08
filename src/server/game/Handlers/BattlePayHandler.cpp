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
#include "DatabaseEnv.h"
#include "DBCEnums.h"
#include "GameTime.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "ItemEnchantmentMgr.h"
#include "Log.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include <algorithm>

namespace
{
    // BattlepayPurchaseStatus / PurchaseResult (extracted from the client enum registrar).
    // STATUS_DONE = 6 per the live 68974 purchase list (TESTER_SNIFF2_LINDORMI_MINE,
    // dump_12.0.7.68974_2026-08-08_02-54-06): all completed purchases carry Status=6, not the
    // enum-registrar Done=3 we previously assumed; a failed VAS flow showed status=12/result=63.
    constexpr int32 STATUS_DONE   = 6;
    constexpr int32 STATUS_FAILED = 4;
    constexpr int32 RESULT_OK                       = 0;
    constexpr int32 RESULT_NOT_ENOUGH_BALANCE       = 29;
    constexpr int32 RESULT_PRODUCT_NOT_PURCHASABLE  = 57;

    // Delivers `count` of `itemId` in full: as much as fits into the player's bags, the remainder by
    // mail (retail parity - a near-full inventory must never turn a full-price purchase into a partial
    // delivery, see audit C-06). Returns false only if the item template is invalid; once past that,
    // bags + mail together always take the whole quantity so the caller may charge safely.
    bool BattlePayDeliverItem(Player* player, uint32 itemId, uint32 count)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto || !count)
            return false;

        uint32 noSpaceCount = 0;
        ItemPosCountVec dest;
        InventoryResult const msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceCount);
        uint32 const toBags = (msg == EQUIP_ERR_OK) ? count : (count - noSpaceCount);

        if (toBags && !dest.empty())
        {
            if (Item* item = player->StoreNewItem(dest, itemId, true, GenerateItemRandomBonusListId(itemId)))
                player->SendNewItem(item, toBags, true, false);
        }

        uint32 remainder = count - toBags;
        if (remainder)
        {
            // Mail the overflow in max-stack chunks.
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            MailDraft draft("In-game Shop Purchase", "The items you purchased did not all fit in your bags; the remainder is attached.");
            uint32 const maxStack = std::max<uint32>(1u, proto->GetMaxStackSize());
            uint32 attached = 0;
            while (remainder && attached < MAX_MAIL_ITEMS)
            {
                uint32 const stackCount = std::min(remainder, maxStack);
                Item* mailItem = Item::CreateItem(itemId, stackCount, ItemContext::NONE, player);
                if (!mailItem)
                    break;
                mailItem->SaveToDB(trans);
                draft.AddItem(mailItem);
                remainder -= stackCount;
                ++attached;
            }
            draft.SendMailTo(trans, MailReceiver(player, player->GetGUID().GetCounter()), MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_GM));
            CharacterDatabase.CommitTransaction(trans);

            if (remainder)
            {
                // More than a full mail's worth left over - refuse rather than silently swallow the rest.
                TC_LOG_ERROR("network", "BattlePay: item {} x{} exceeded bag + mail capacity for {}; {} undelivered.",
                    itemId, count, player->GetName(), remainder);
                return false;
            }
        }
        return true;
    }
}

// In-game Shop (BattlePay). P0: reply to the catalog request with the captured, client-validated
// product list so the shop opens and displays real products. If no catalog blob is loaded we send
// nothing (shop opens empty) rather than fabricating wire.
void WorldSession::HandleBattlePayGetProductList(WorldPackets::BattlePay::GetProductList& /*getProductList*/)
{
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED))
        return;

    if (!sBattlePayMgr->HasCatalog())
    {
        TC_LOG_DEBUG("network", "BattlePay: GetProductList from {} but no catalog loaded.", GetPlayerInfo());
        return;
    }

    // Throttle the 58 KB blob: serve it at most once per catalog generation for this session. The client
    // re-requests on every shop open, so without this a STATUS_AUTHED session could pull it repeatedly.
    // A `.reload shop_catalog` bumps the generation, so a fresh catalog still reaches the next request.
    uint32 const generation = sBattlePayMgr->GetCatalogGeneration();
    if (_battlePayCatalogGeneration == generation)
    {
        TC_LOG_DEBUG("network", "BattlePay: GetProductList from {} already served catalog gen {}.", GetPlayerInfo(), generation);
        return;
    }

    WorldPackets::BattlePay::ProductListResponse response;
    response.RawData = &sBattlePayMgr->GetProductListBlob();
    SendPacket(response.Write());
    _battlePayCatalogGeneration = generation;
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

    // Resolve the advertised (slot) productID to its admin product via the catalog routing map.
    // Placeholder / unrouted slots have no product -> not purchasable.
    ShopProduct const* product = player ? sBattlePayMgr->GetProductByAdvertisedId(productID) : nullptr;
    if (!product)
    {
        TC_LOG_DEBUG("network", "BattlePay: purchase of unrouted product {} by {}.", productID, GetPlayerInfo());
        respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, 0);
        return;
    }

    // Authoritative server-side gate for everything the wire cannot express (enabled/window/level/
    // faction/hideIfOwned/condition). Also refuse a spell-only product the player already fully owns
    // so a repeat purchase never takes gold for nothing (audit C-05), regardless of the HideIfOwned flag.
    // A product with no deliverables is display-only (e.g. the template's showcase mounts/pets): visible
    // in the catalog but not for sale, so it must never report a successful purchase (audit parity - these
    // had no battlepay_product row before and returned 57).
    time_t const now = GameTime::GetGameTime();
    if (product->Deliverables.empty()
        || !sBattlePayMgr->IsPurchasable(*product, player, now)
        || BattlePayMgr::IsAlreadyFullyOwned(*product, player))
    {
        respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, 0);
        return;
    }

    // Reserved / unavailable deliverable types abort the whole purchase BEFORE charging: type 3 (WoW
    // Token) needs WowTokenMgr which lives on the wow-token branch; types 4 (game time) / 5 (service)
    // are schema-reserved with no delivery impl yet. Purchase result 57 until they land.
    for (ShopDeliverable const& d : product->Deliverables)
    {
        if (d.Type < 1 || d.Type > 2)
        {
            TC_LOG_DEBUG("network", "BattlePay: product {} has unsupported deliverable type {} - refused.", productID, d.Type);
            respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, 0);
            return;
        }
    }

    // Cost check by currency (1 = gold copper, 2 = item token).
    if (product->Currency == 1 && product->Price && !player->HasEnoughMoney(product->Price))
    {
        respond(STATUS_FAILED, RESULT_NOT_ENOUGH_BALANCE, product->Price);
        return;
    }
    if (product->Currency == 2 && product->PriceItemId && !player->HasItemCount(product->PriceItemId, product->PriceItemCount))
    {
        respond(STATUS_FAILED, RESULT_NOT_ENOUGH_BALANCE, product->Price);
        return;
    }

    // Grant first; only charge if every deliverable succeeds so we never take payment without delivering.
    bool granted = true;
    for (ShopDeliverable const& d : product->Deliverables)
    {
        switch (d.Type)
        {
            case 1: // item - full delivery to bags, overflow to mail (no partial-stack-at-full-price)
                if (!BattlePayDeliverItem(player, d.Id, d.Count))
                    granted = false;
                break;
            case 2: // spell (mount / toy / appearance) - LearnSpell routes it into the account-wide
                    // collection via CollectionMgr; LearnSpell no-ops if a bundled spell is already known
                if (!player->HasSpell(d.Id))
                    player->LearnSpell(d.Id, false);
                break;
            default:
                break;
        }
        if (!granted)
            break;
    }

    if (!granted)
    {
        TC_LOG_DEBUG("network", "BattlePay: grant failed for product {} ({}), {} not charged.",
            productID, product->Name, GetPlayerInfo());
        respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, product->Price);
        return;
    }

    if (product->Currency == 1 && product->Price)
        player->ModifyMoney(-int64(product->Price));
    if (product->Currency == 2 && product->PriceItemId)
        player->DestroyItemCount(product->PriceItemId, product->PriceItemCount, true);

    TC_LOG_INFO("network", "BattlePay: {} purchased product {} ({}) for {} (currency {}).",
        GetPlayerInfo(), productID, product->Name, product->Price, product->Currency);

    respond(STATUS_DONE, RESULT_OK, product->Price);
}

void WorldSession::HandleBattlePayStartPurchase(WorldPackets::BattlePay::StartPurchase& startPurchase)
{
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED))
        return;

    // The productID scalar is a strong candidate (setter is Warden-obfuscated). Log all three fields so a
    // live purchase confirms which scalar is the productID; GetProduct() also guards against a wrong guess.
    TC_LOG_INFO("network", "BattlePay: StartPurchase from {}: u32={} u64={} flag={}",
        GetPlayerInfo(), startPurchase.ProductID, startPurchase.ScalarU64, uint32(startPurchase.Flag));

    // Two-step retail confirmation flow (opt-in): stash the pending product, prompt the client, and
    // complete the purchase only when it answers CMSG_BATTLE_PAY_CONFIRM_PURCHASE_RESPONSE. Off by
    // default because the confirm packet layout is inferred (see ConfirmPurchase in BattlePayPackets.h);
    // the proven direct path runs otherwise.
    if (sWorld->getBoolConfig(CONFIG_SHOP_PURCHASE_CONFIRMATION))
    {
        Player* player = GetPlayer();
        ShopProduct const* product = player ? sBattlePayMgr->GetProductByAdvertisedId(startPurchase.ProductID) : nullptr;
        if (!product)
        {
            WorldPackets::BattlePay::StartPurchaseResponse ack;
            ack.PurchaseID = sBattlePayMgr->GeneratePurchaseID();
            ack.ResultB = uint32(RESULT_PRODUCT_NOT_PURCHASABLE);
            SendPacket(ack.Write());
            return;
        }

        uint64 const purchaseID = sBattlePayMgr->GeneratePurchaseID();
        _battlePayPendingProductID = startPurchase.ProductID;
        _battlePayConfirmToken = uint32(purchaseID) | 0x1u;     // non-zero token = a purchase is pending

        WorldPackets::BattlePay::ConfirmPurchase confirm;
        confirm.PurchaseID = purchaseID;
        confirm.ProductID = startPurchase.ProductID;
        confirm.CurrentPriceFixedPoint = product->Currency == 1 ? (product->Price / 10000) * 100000 : 0;  // copper -> shop fixed-point
        confirm.ServerToken = _battlePayConfirmToken;
        SendPacket(confirm.Write());
        return;
    }

    BattlePayProcessPurchase(startPurchase.ProductID);
}

void WorldSession::HandleBattlePayConfirmPurchaseResponse(WorldPackets::BattlePay::ConfirmPurchaseResponse& confirmPurchaseResponse)
{
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED))
        return;

    // Ignore stale/unsolicited responses (guards against a replayed confirm double-charging).
    if (!_battlePayConfirmToken || confirmPurchaseResponse.ServerToken != _battlePayConfirmToken)
    {
        TC_LOG_DEBUG("network", "BattlePay: ConfirmPurchaseResponse from {} with unexpected token {} (pending {}).",
            GetPlayerInfo(), confirmPurchaseResponse.ServerToken, _battlePayConfirmToken);
        return;
    }

    uint32 const productID = _battlePayPendingProductID;
    _battlePayConfirmToken = 0;          // consume the pending purchase before doing any work
    _battlePayPendingProductID = 0;

    if (!confirmPurchaseResponse.Confirmed)
    {
        TC_LOG_INFO("network", "BattlePay: {} cancelled purchase of product {}.", GetPlayerInfo(), productID);
        return;
    }

    BattlePayProcessPurchase(productID);
}

void WorldSession::HandleBattlePayOpenCheckout(WorldPackets::BattlePay::OpenCheckout& openCheckout)
{
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED))
        return;

    // Retail routes purchases here (-> Battle.net web-checkout) using a distributionID rather than a
    // productID. We cannot map that to a product without the distribution setup, so log it for diagnosis;
    // server-defined products are purchased via StartPurchase above.
    TC_LOG_INFO("network", "BattlePay: OpenCheckout from {} distributionID={} (web-checkout path, not handled).",
        GetPlayerInfo(), openCheckout.DistributionID);
}

// Replays the captured distribution-list blob unsolicited at session start. There is no CMSG that
// requests it; retail pushes it after the glue feature status, and the client's StoreFrame_IsLoading
// gate stays stuck until HasDistributionList() flips, which this response does.
void WorldSession::SendBattlePayDistributionList()
{
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED))
        return;

    if (!sBattlePayMgr->HasDistributionList())
        return;

    WorldPackets::BattlePay::GetDistributionListResponse response;
    response.RawData = &sBattlePayMgr->GetDistributionListBlob();
    SendPacket(response.Write());
}

void WorldSession::HandleBattlePayGetPurchaseList(WorldPackets::BattlePay::GetPurchaseList& /*getPurchaseList*/)
{
    // The client polls this whenever the Shop is opened (390 requests across our sniffs) and blocks its
    // purchase UI (HasPurchaseList) until it gets a reply. Answer with this account's real purchase
    // history: the Shop only grants products through BattlePayProcessPurchase, which does not persist a
    // purchase ledger, so the honest answer today is an empty list rather than fabricated entries.
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED))
        return;

    WorldPackets::BattlePay::GetPurchaseListResponse response;
    response.Result = 0;    // PurchaseResult::Ok
    SendPacket(response.Write());
}
