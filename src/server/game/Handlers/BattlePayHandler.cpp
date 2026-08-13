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
#include "RealmList.h"
#include "World.h"
#include "WowTokenMgr.h"
#include <algorithm>
#include "Timer.h"

namespace
{
    // BattlepayPurchaseStatus / PurchaseResult (extracted from the client enum registrar).
    // STATUS_DONE = 6 per the live 68974 purchase list (TESTER_SNIFF2_LINDORMI_MINE,
    // dump_12.0.7.68974_2026-08-08_02-54-06): all completed purchases carry Status=6, not the
    // enum-registrar Done=3 we previously assumed; a failed VAS flow showed status=12/result=63.
    constexpr int32 STATUS_DONE = 6;
    constexpr int32 STATUS_FAILED = 4;
    // The client will not open the confirmation prompt, and PurchaseProductConfirm refuses to send the
    // response at all (Buy AND Cancel), unless a JamBattlePayPurchase record for this purchaseID exists
    // in its list with status == 9 and resultCode == 0. Verified at 0x13EB72C / 0x13EB736.
    constexpr int32 STATUS_CONFIRMATION_PENDING = 9;
    constexpr int32 RESULT_OK = 0;
    constexpr int32 RESULT_NOT_ENOUGH_BALANCE = 29;
    constexpr int32 RESULT_PRODUCT_NOT_PURCHASABLE = 57;

    // Distribution-specific PurchaseResult codes, taken from the client's own generated API
    // documentation (Blizzard_APIDocumentationGenerated/BattlepayConstantsDocumentation.lua), so these
    // values are the client's, not ours.
    constexpr int32 RESULT_NOT_ALLOWED_IN_GLUE_SCREEN = 27;   // ErrorBuyingProductNotAllowedInGlueScreen
    constexpr int32 RESULT_DISTRIBUTION_NOT_FOUND = 19;   // ErrorDistributionObjectNotFound
    constexpr int32 RESULT_DISTRIBUTION_INVALID_TARGET = 42;  // ErrorDistributionObjectInvalidTarget

    // A single distribution list must stay inside the wire's 11-bit count field; this is far below that
    // and keeps one account from ever pushing a multi-kilobyte list.
    constexpr size_t MAX_LISTED_ENTITLEMENTS = 64;

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

            // Persist completed purchases to the shared ledger so GetPurchaseList answers from real history
            // and the PurchaseID is durable across restarts (C-13/C-32). Failed attempts are not recorded.
            if (status == STATUS_DONE)
                sBattlePayMgr->RecordPurchase(GetAccountId(), purchaseID, status, resultCode, productID, price, price);
        };

    // Resolve the advertised (slot) productID to its admin product via the catalog routing map.
    // Placeholder / unrouted slots have no product -> not purchasable. The lookup itself needs no
    // player, so it also works at character select.
    ShopProduct const* product = sBattlePayMgr->GetProductByAdvertisedId(productID);
    if (!product)
    {
        // Most cards in the shipped catalog are UNTOUCHED retail records - only the slots we reskin
        // are routed to a real ShopProduct. Buying any other card lands here. Logged at INFO because
        // it is the difference between "the wire is broken" and "that item is not one of ours".
        TC_LOG_INFO("network", "BattlePay: {} tried to buy product {}, which is not routed to a shop_product "
            "(catalog card is an un-reskinned retail entry).", GetPlayerInfo(), productID);
        respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, 0);
        return;
    }

    time_t const now = GameTime::GetGameTime();
    bool const entitlementsEnabled = sWorld->getBoolConfig(CONFIG_SHOP_ENTITLEMENTS_ENABLED);

    // A product with no deliverables is display-only (e.g. the template's showcase mounts/pets): visible
    // in the catalog but not for sale, so it must never report a successful purchase.
    if (product->Deliverables.empty())
    {
        respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, 0);
        return;
    }

    // ---- Character select: no player, so nothing can be granted and nothing can be charged ----------
    //
    // This is the case the Shop hits when it is opened from the glue screen. Previously it always failed
    // (the product lookup was gated on `player`). With entitlements on, the purchase instead creates an
    // owned-but-unapplied entitlement the player assigns to a character later.
    //
    // Only free products can be bought here, and that is a real constraint rather than a shortcut: the
    // price is denominated in a character's gold, and at character select there is no character to take
    // it from. Anything with a price is refused with the client's own
    // ErrorBuyingProductNotAllowedInGlueScreen, which is exactly what that code is for.
    if (!player)
    {
        if (!entitlementsEnabled)
        {
            TC_LOG_INFO("network", "BattlePay: {} tried to buy product {} at character select, but "
                "Shop.Entitlements.Enabled is off.", GetPlayerInfo(), productID);
            respond(STATUS_FAILED, RESULT_NOT_ALLOWED_IN_GLUE_SCREEN, 0);
            return;
        }

        if (!product->Enabled || (product->AvailableFrom && now < product->AvailableFrom)
            || (product->AvailableUntil && now > product->AvailableUntil))
        {
            respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, 0);
            return;
        }

        if (product->Currency != 0 || product->Price || product->PriceItemId)
        {
            TC_LOG_INFO("network", "BattlePay: {} tried to buy priced product {} at character select; only "
                "free products can be bought without a character to charge.", GetPlayerInfo(), productID);
            respond(STATUS_FAILED, RESULT_NOT_ALLOWED_IN_GLUE_SCREEN, 0);
            return;
        }

        int32 const result = BattlePayCreateEntitlement(*product, purchaseID);
        respond(result == RESULT_OK ? STATUS_DONE : STATUS_FAILED, result, 0);
        return;
    }

    // Authoritative server-side gate for everything the wire cannot express (enabled/window/level/
    // faction/hideIfOwned/condition). Also refuse a spell-only product the player already fully owns
    // so a repeat purchase never takes gold for nothing (audit C-05), regardless of the HideIfOwned flag.
    if (!sBattlePayMgr->IsPurchasable(*product, player, now)
        || BattlePayMgr::IsAlreadyFullyOwned(*product, player))
    {
        respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, 0);
        return;
    }

    // A service deliverable (type 5) has no immediate form at all: its target is a character chosen
    // AFTER the purchase, which is precisely what an entitlement is for. With entitlements on, such a
    // product is sold as an entitlement even to a logged-in buyer.
    bool const deferred = entitlementsEnabled && BattlePayMgr::NeedsEntitlement(*product, player);

    // Reserved / unavailable deliverable types abort the whole purchase BEFORE charging: type 4 (game
    // time) is schema-reserved with no delivery impl, and type 5 (service) lands here only when
    // entitlements are off. Type 3 (WoW Token) is delivered through WowTokenMgr in the grant loop below.
    if (!deferred)
    {
        for (ShopDeliverable const& d : product->Deliverables)
        {
            if (d.Type < 1 || d.Type > 3)
            {
                TC_LOG_DEBUG("network", "BattlePay: product {} has unsupported deliverable type {} - refused.", productID, d.Type);
                respond(STATUS_FAILED, RESULT_PRODUCT_NOT_PURCHASABLE, 0);
                return;
            }
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

    // ---- Deferred (entitlement) purchase by a logged-in player -------------------------------------
    //
    // Charge first and commit it synchronously, THEN create the entitlement. This inverts the normal
    // grant-before-charge rule for the same reason the WoW Token path does: the entitlement is written
    // to the auth DB immediately and durably, while the buyer's gold would otherwise sit in Player
    // memory until the next periodic save. Charging last would mean a crash in that (minutes-long)
    // window left the account holding a paid-for entitlement that was never paid for - the server
    // losing value, which is the one outcome we never accept. The surviving failure is "charged, no
    // entitlement", and that path refunds explicitly below.
    if (deferred)
    {
        if (product->Currency == 1 && product->Price)
            player->ModifyMoney(-int64(product->Price));
        if (product->Currency == 2 && product->PriceItemId)
            player->DestroyItemCount(product->PriceItemId, product->PriceItemCount, true);

        {
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            player->SaveInventoryAndGoldToDB(trans);
            CharacterDatabase.DirectCommitTransaction(trans);
        }

        int32 const result = BattlePayCreateEntitlement(*product, purchaseID);
        if (result != RESULT_OK)
        {
            // Give the price back: the player paid and got nothing.
            if (product->Currency == 1 && product->Price)
                player->ModifyMoney(int64(product->Price));
            if (product->Currency == 2 && product->PriceItemId)
                BattlePayDeliverItem(player, product->PriceItemId, product->PriceItemCount);

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            player->SaveInventoryAndGoldToDB(trans);
            CharacterDatabase.DirectCommitTransaction(trans);

            TC_LOG_ERROR("network", "BattlePay: entitlement for product {} could not be created for {}; "
                "the price was refunded.", productID, GetPlayerInfo());
            respond(STATUS_FAILED, result, product->Price);
            return;
        }

        TC_LOG_INFO("network", "BattlePay: {} purchased product {} ({}) as an entitlement for {} (currency {}).",
            GetPlayerInfo(), productID, product->Name, product->Price, product->Currency);
        respond(STATUS_DONE, RESULT_OK, product->Price);
        return;
    }

    // Grant first; only charge if every deliverable succeeds so we never take payment without delivering.
    // Exception: a WoW Token (type 3) is charged *before* creation and committed synchronously - see the
    // anti-abuse note in that case; chargeSettled records that so the generic post-grant charge is skipped.
    bool granted = true;
    bool chargeSettled = false;
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
        case 3: // WoW Token - the retail acquisition path: bought from the Shop, then account-sellable.
            // This is THE shop<->token synergy: the catalog-admin deliverable drives WowTokenMgr.
            // ANTI-ABUSE (C-07, TK-5, the audit's #1 finding): a token is persisted to the AUTH DB
            // the instant it is created, while the buyer's gold otherwise stays in Player memory
            // until the next periodic character save. A crash in that (minutes-long) window would
            // leave the account holding the token with the gold never taken - a free token / free
            // gold duplication. Close the window: charge the cost and commit it to the character DB
            // *synchronously first*, then create the token. The only surviving crash outcome is
            // "gold taken, token not created" - a refundable player loss, never "keep gold AND
            // token" (the server never loses value).
            if (product->Currency == 1 && product->Price)
                player->ModifyMoney(-int64(product->Price));
            if (product->Currency == 2 && product->PriceItemId)
                player->DestroyItemCount(product->PriceItemId, product->PriceItemCount, true);
            {
                CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                player->SaveInventoryAndGoldToDB(trans);
                CharacterDatabase.DirectCommitTransaction(trans);
            }
            chargeSettled = true;

            for (uint32 i = 0; i < std::max<uint32>(d.Count, 1u); ++i)
                sWowTokenMgr->CreateToken(GetAccountId(), WOW_TOKEN_STATE_AUCTIONABLE);

            // Confirmed trigger for this push: the account's token holdings changed.
            SendCommerceTokenUpdate();
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

    // Types 1/2 are charged here after a successful grant; a type-3 token already settled its charge
    // synchronously above (chargeSettled), so it must not be charged again.
    if (!chargeSettled)
    {
        if (product->Currency == 1 && product->Price)
            player->ModifyMoney(-int64(product->Price));
        if (product->Currency == 2 && product->PriceItemId)
            player->DestroyItemCount(product->PriceItemId, product->PriceItemCount, true);
    }

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
    TC_LOG_INFO("network", "BattlePay: StartPurchase from {}: clientToken={} productID={} flag={}",
        GetPlayerInfo(), startPurchase.ClientToken, startPurchase.ProductID, uint32(startPurchase.Flag));

    // ANTI-ABUSE (C-13): collapse replayed / double-clicked purchases to a single charge. CMSG_START_PURCHASE
    // is craftable by any logged-in client and there is no client-supplied idempotency key. A lagged
    // double-click sends two CMSGs which the world thread runs back-to-back, so the first has already
    // completed (and charged) before the second begins - an in-flight flag alone cannot see it. A short
    // per-session throttle does: reject a second StartPurchase within BATTLEPAY_PURCHASE_THROTTLE_MS.
    // The duplicate is dropped silently (no charge, no ack) so the first purchase's response still drives
    // the UI; the in-flight flag additionally guards against any future re-entrancy. Applied to both the
    // direct and the two-step confirmation path (the timestamp is stamped before either runs).
    static constexpr uint32 BATTLEPAY_PURCHASE_THROTTLE_MS = 2000;
    uint32 const now = getMSTime();
    if (_battlePayPurchaseInFlight ||
        (_lastBattlePayPurchaseMSTime && getMSTimeDiff(_lastBattlePayPurchaseMSTime, now) < BATTLEPAY_PURCHASE_THROTTLE_MS))
    {
        TC_LOG_DEBUG("network", "BattlePay: throttled duplicate StartPurchase from {} (product {}).",
            GetPlayerInfo(), startPurchase.ProductID);
        return;
    }
    _lastBattlePayPurchaseMSTime = now;

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

        // ORDER MATTERS. The confirmation dialog reads the product name, wallet and both prices out of
        // the client's own purchase RECORD, not out of the confirm packet - GetConfirmationInfo returns
        // nil when no record for this purchaseID exists, and the Lua then hides the frame. So the record
        // has to arrive first: StartPurchaseResponse, then a PURCHASE_UPDATE carrying one record in
        // status 9, and only then the 12-byte confirm packet.
        // Sending the confirm packet on its own (what we did before) is why the client never answered.
        {
            WorldPackets::BattlePay::StartPurchaseResponse ack;
            ack.PurchaseID = purchaseID;
            ack.ResultB = uint32(RESULT_OK);
            SendPacket(ack.Write());
        }

        {
            // Prices are echoed back by the client from what it DISPLAYS, so keep them whole cents -
            // sub-cent precision can never compare equal on the way back.
            uint64 const price = product->Currency == 1 ? (product->Price / 10000) * 100 : 0;

            WorldPackets::BattlePay::PurchaseUpdate update;
            WorldPackets::BattlePay::PurchaseRecord& rec = update.Purchases.emplace_back();
            rec.PurchaseID = purchaseID;
            rec.Status = STATUS_CONFIRMATION_PENDING;
            rec.ResultCode = int32(RESULT_OK);
            rec.ProductID = startPurchase.ProductID;
            rec.BasePrice = price;
            rec.UserPrice = price;
            SendPacket(update.Write());
        }

        WorldPackets::BattlePay::ConfirmPurchase confirm;
        confirm.PurchaseID = purchaseID;
        confirm.ServerToken = _battlePayConfirmToken;
        SendPacket(confirm.Write());
        return;
    }

    _battlePayPurchaseInFlight = true;
    BattlePayProcessPurchase(startPurchase.ProductID);
    _battlePayPurchaseInFlight = false;
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

    // Retail answers CMSG_BATTLE_PAY_OPEN_CHECKOUT with SMSG_GENERATE_SSO_TOKEN_RESPONSE as a strict
    // 1:1 echo of the request's ClientToken (proven in all 8 captures: checkout #N -> response #N with
    // the same u32). Answering from here - rather than pushing the token unsolicited at login - is what
    // lets checkouts #2+ get a reply. See COMMERCE_AUDIT C-09 / WOW_TOKEN_RE_68275.md.
    SendGenerateSsoToken(openCheckout.ClientToken);
}

namespace
{
    // Renders one entitlement as the wire object the client parses. The structure is proven (see
    // DistributionObject in BattlePayPackets.h); what is OURS to choose is which values go in it.
    //
    // `DeliverableID` is set to the product id because this core has no separate deliverable-id
    // namespace - shop_product_deliverable is keyed (productId, seq). The client does not need it to
    // resolve anything: the full deliverable record is embedded inline right after it, exactly as in
    // the capture.
    void BuildDistributionObject(ShopEntitlement const& entitlement, WorldPackets::BattlePay::DistributionObject& out)
    {
        out.DistributionID = entitlement.DistributionID;
        out.Status = entitlement.Status;
        out.DeliverableID = entitlement.ProductID;
        out.PurchaseID = entitlement.PurchaseID;
        // LicenseGameAccountGUID / TargetPlayer stay empty: an unassigned entitlement has no target, and
        // the capture's licence guid has no server-side meaning we could reproduce honestly.

        ShopProduct const* product = sBattlePayMgr->GetProduct(entitlement.ProductID);
        if (!product || product->Deliverables.empty())
            return;                                 // hasDeliverable stays 0 - structurally valid

        WorldPackets::BattlePay::DistributionDeliverable& deliverable = out.Deliverable.emplace();
        deliverable.DeliverableID = entitlement.ProductID;
        deliverable.Name = product->Name.substr(0, 255);        // client buffer is char[256]

        // Map our deliverable vocabulary onto the catalog's. A type-5 (service) row carries the
        // catalog's own deliverable type in `id` (1 CharacterBoost, 5 NameChange, 6 FactionChange,
        // 8 RaceChange, 11 CharacterTransfer), so it passes straight through.
        ShopDeliverable const& first = product->Deliverables.front();
        switch (first.Type)
        {
        case 1:                                             // item
            deliverable.Type = 14;                          // Item/Toy
            deliverable.ItemID = first.Id;
            deliverable.Quantity = first.Count;
            break;
        case 2:                                             // spell (mount / toy / appearance)
            deliverable.Type = 3;                           // Mount
            deliverable.MountSpellID = first.Id;
            break;
        case 3:                                             // WoW Token
            deliverable.Type = 4;                           // WowToken
            deliverable.Quantity = first.Count;
            break;
        case 5:                                             // service
            deliverable.Type = first.Id;
            break;
        default:
            break;
        }
    }
}

// Sent unsolicited at session start (character select) and again at login. There is no CMSG that
// requests it - the client has no such opcode in this build. The client's StoreFrame_IsLoading gate
// keeps the Shop on "Loading, please wait" until C_StoreSecure.HasDistributionList() flips, which this
// response does.
//
// With entitlements off this replays the captured blob exactly as before. With entitlements on it
// answers from this account's real, unapplied entitlements - so the load has to finish first.
void WorldSession::SendBattlePayDistributionList()
{
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED))
        return;

    if (sWorld->getBoolConfig(CONFIG_SHOP_ENTITLEMENTS_ENABLED))
    {
        LoadBattlePayEntitlements(true);
        return;
    }

    if (!sBattlePayMgr->HasDistributionList())
        return;

    WorldPackets::BattlePay::GetDistributionListResponse response;
    response.RawData = &sBattlePayMgr->GetDistributionListBlob();
    SendPacket(response.Write());
}

// Refreshes this session's cached entitlements from the account store, optionally pushing the list
// afterwards. Async like the purchase list; the callback runs on the world thread via _queryProcessor,
// which is serviced at character select as well as in world.
void WorldSession::LoadBattlePayEntitlements(bool sendList)
{
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED) || !sWorld->getBoolConfig(CONFIG_SHOP_ENTITLEMENTS_ENABLED))
        return;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BATTLEPAY_ENTITLEMENT_ACCOUNT);
    stmt->setUInt32(0, GetAccountId());

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt)
        .WithPreparedCallback([this, sendList](PreparedQueryResult result)
            {
                _battlePayEntitlements.clear();
                if (result)
                {
                    do
                    {
                        Field* fields = result->Fetch();
                        ShopEntitlement& e = _battlePayEntitlements.emplace_back();
                        e.DistributionID = fields[0].GetUInt64();
                        e.ProductID = fields[1].GetUInt32();
                        e.ServiceType = fields[2].GetUInt8();
                        e.Status = fields[3].GetUInt8();
                        e.PurchaseID = fields[4].GetUInt64();
                        e.CreateTime = fields[5].GetInt64();
                    } while (result->NextRow());
                }

                if (sendList)
                    SendBattlePayDistributionListNow();
            }));
}

void WorldSession::SendBattlePayDistributionListNow()
{
    WorldPackets::BattlePay::GetDistributionListResponse response;
    response.BuildFromObjects = true;
    response.Result = RESULT_OK;

    for (ShopEntitlement const& e : _battlePayEntitlements)
    {
        if (e.Status != SHOP_ENTITLEMENT_AVAILABLE)
            continue;
        if (response.Distributions.size() >= MAX_LISTED_ENTITLEMENTS)
            break;
        BuildDistributionObject(e, response.Distributions.emplace_back());
    }

    SendPacket(response.Write());
}

// Pushes one entitlement so the client fires PRODUCT_DISTRIBUTIONS_UPDATED and refreshes its token row.
void WorldSession::SendBattlePayDistributionUpdate(ShopEntitlement const& entitlement)
{
    WorldPackets::BattlePay::DistributionUpdate update;
    BuildDistributionObject(entitlement, update.Distribution);
    SendPacket(update.Write());
}

// Turns a purchase into an owned-but-unapplied entitlement instead of an immediate grant. Returns the
// PurchaseResult to report; RESULT_OK means the entitlement exists and the caller may charge.
int32 WorldSession::BattlePayCreateEntitlement(ShopProduct const& product, uint64 purchaseID)
{
    // Cap what one account may hoard. Free products at character select are otherwise an unbounded row
    // generator (the 2 s purchase throttle limits the rate, not the total), and the wire cannot show
    // more than this anyway.
    size_t available = 0;
    for (ShopEntitlement const& e : _battlePayEntitlements)
        if (e.Status == SHOP_ENTITLEMENT_AVAILABLE)
            ++available;

    if (available >= MAX_LISTED_ENTITLEMENTS)
    {
        TC_LOG_INFO("network", "BattlePay: {} already holds {} unapplied entitlements; refusing another.",
            GetPlayerInfo(), available);
        return RESULT_PRODUCT_NOT_PURCHASABLE;
    }

    uint64 const distributionId = sBattlePayMgr->CreateEntitlement(GetAccountId(), product.ProductID,
        BattlePayMgr::GetServiceType(product), purchaseID);
    if (!distributionId)
        return RESULT_PRODUCT_NOT_PURCHASABLE;

    ShopEntitlement& e = _battlePayEntitlements.emplace_back();
    e.DistributionID = distributionId;
    e.ProductID = product.ProductID;
    e.ServiceType = BattlePayMgr::GetServiceType(product);
    e.Status = SHOP_ENTITLEMENT_AVAILABLE;
    e.PurchaseID = purchaseID;
    e.CreateTime = GameTime::GetGameTime();

    SendBattlePayDistributionUpdate(e);
    SendBattlePayDistributionListNow();
    return RESULT_OK;
}

// Applies one owned entitlement to a character the client has chosen.
//
// The request's four fields are structurally proven from the client's serializer, but the packet has
// never been seen on the wire, so nothing here trusts the field NAMES either: the DistributionID must
// be one this server issued to THIS account, and the target must be a character THIS session
// enumerated. A crafted or misread packet therefore cannot grant anything.
void WorldSession::HandleBattlePayDistributionAssignToTarget(WorldPackets::BattlePay::DistributionAssignToTarget& assign)
{
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED) || !sWorld->getBoolConfig(CONFIG_SHOP_ENTITLEMENTS_ENABLED))
        return;

    // Diagnostic first: this is the first build in which we answer this opcode at all, and this log is
    // what will confirm the field reading against a real client.
    TC_LOG_INFO("network", "BattlePay: DistributionAssignToTarget from {}: clientToken={} distributionID={} "
        "target={} productChoice={}", GetPlayerInfo(), assign.ClientToken, assign.DistributionID,
        assign.TargetCharacter.ToString(), assign.ProductChoice);

    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENTITLEMENT_ASSIGN_ENABLED))
    {
        TC_LOG_INFO("network", "BattlePay: assignment ignored - Shop.Entitlements.AssignEnabled is off.");
        return;
    }

    auto respond = [&](int32 result)
        {
            WorldPackets::BattlePay::StartDistributionAssignToTargetResponse response;
            response.Result = uint32(result);
            response.DistributionID = assign.DistributionID;
            SendPacket(response.Write());
        };

    // 1. Is it ours, and still unapplied? The cache is authoritative for "what this account owns"; the
    //    compare-and-swap below is authoritative for "nobody else took it first".
    ShopEntitlement const* wanted = nullptr;
    for (ShopEntitlement const& e : _battlePayEntitlements)
        if (e.Status == SHOP_ENTITLEMENT_AVAILABLE && e.DistributionID == assign.DistributionID)
            wanted = &e;

    if (!wanted)
    {
        TC_LOG_INFO("network", "BattlePay: {} named entitlement {}, which is not an available entitlement of "
            "this account ({} cached).", GetPlayerInfo(), assign.DistributionID, _battlePayEntitlements.size());
        respond(RESULT_DISTRIBUTION_NOT_FOUND);
        return;
    }

    // 2. Is the target really one of this account's characters? _legitCharacters is filled from the
    //    character enumeration, so this also rejects a guid belonging to somebody else entirely.
    if (assign.TargetCharacter.IsEmpty() || !IsLegitCharacterForAccount(assign.TargetCharacter))
    {
        TC_LOG_INFO("network", "BattlePay: {} tried to assign entitlement {} to {}, which is not a character "
            "of this account.", GetPlayerInfo(), assign.DistributionID, assign.TargetCharacter.ToString());
        respond(RESULT_DISTRIBUTION_INVALID_TARGET);
        return;
    }

    uint64 const distributionId = wanted->DistributionID;
    uint64 const purchaseId = wanted->PurchaseID;

    // 3. Claim it. This is the only step that may never run twice, and the compare-and-swap in the
    //    manager guarantees that: a replayed assign, or a second realm on the shared auth DB, loses the
    //    race and lands here with a failure instead of a second grant.
    uint64 claimToken = 0;
    ShopEntitlement claimed;
    int32 claimResult = 0;
    if (!sBattlePayMgr->ClaimEntitlement(GetAccountId(), distributionId, sRealmList->GetCurrentRealmId().Realm,
        assign.TargetCharacter.GetCounter(), claimToken, claimed, claimResult))
    {
        TC_LOG_INFO("network", "BattlePay: {} could not claim entitlement {} (result {}).",
            GetPlayerInfo(), distributionId, claimResult);
        respond(claimResult);
        LoadBattlePayEntitlements(true);            // resync: our cache was stale
        return;
    }

    // 4. Commit the claim as BOUND. The payload is handed over the next time that character logs in -
    //    the target is offline right now (this is character select), and delivering through the proven
    //    online path beats hand-writing another character's inventory.
    if (!sBattlePayMgr->TransitionEntitlement(distributionId, SHOP_ENTITLEMENT_CLAIMED, claimToken,
        SHOP_ENTITLEMENT_BOUND, 0))
    {
        // Could not finish the hand-off: put it back, so the player still owns what they paid for.
        sBattlePayMgr->TransitionEntitlement(distributionId, SHOP_ENTITLEMENT_CLAIMED, claimToken,
            SHOP_ENTITLEMENT_AVAILABLE, 0);
        TC_LOG_ERROR("network", "BattlePay: failed to bind entitlement {} to {}; returned it to the account.",
            distributionId, assign.TargetCharacter.ToString());
        respond(RESULT_PRODUCT_NOT_PURCHASABLE);
        LoadBattlePayEntitlements(true);
        return;
    }

    TC_LOG_INFO("network", "BattlePay: {} assigned entitlement {} (product {}) to {}; it will be delivered "
        "at that character's next login.", GetPlayerInfo(), distributionId, claimed.ProductID,
        assign.TargetCharacter.ToString());

    respond(RESULT_OK);

    // Tell the client the entitlement moved on, then resend the (now shorter) list.
    claimed.Status = SHOP_ENTITLEMENT_BOUND;
    claimed.PurchaseID = purchaseId;
    SendBattlePayDistributionUpdate(claimed);
    LoadBattlePayEntitlements(true);
}

// Delivers every entitlement bound to the character that just entered the world.
//
// Ordering is deliberately mark-then-deliver, the inverse of the normal grant-before-charge rule and for
// the same reason the WoW Token path inverts it: the entitlement is already paid for and already durable
// in the auth DB, so the only failure worth engineering against is delivering it twice. Consuming the row
// first means a crash mid-delivery costs the player one payload (refundable by a GM) instead of letting a
// relog loop mint goods forever.
void WorldSession::RedeemBattlePayEntitlements()
{
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED) || !sWorld->getBoolConfig(CONFIG_SHOP_ENTITLEMENTS_ENABLED))
        return;

    Player* player = GetPlayer();
    if (!player)
        return;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BATTLEPAY_ENTITLEMENT_PENDING_CHAR);
    stmt->setUInt32(0, sRealmList->GetCurrentRealmId().Realm);
    stmt->setUInt64(1, player->GetGUID().GetCounter());

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt)
        .WithPreparedCallback([this](PreparedQueryResult result)
            {
                if (!result)
                    return;

                Player* target = GetPlayer();
                if (!target || !target->IsInWorld())
                    return;

                do
                {
                    Field* fields = result->Fetch();
                    uint64 const distributionId = fields[0].GetUInt64();
                    uint32 const productId = fields[1].GetUInt32();
                    uint8 const serviceType = fields[2].GetUInt8();

                    // Consume first (see the ordering note above). A losing racer simply finds nothing to do.
                    if (!sBattlePayMgr->TransitionEntitlement(distributionId, SHOP_ENTITLEMENT_BOUND, 0,
                        SHOP_ENTITLEMENT_FINISHED, 0))
                    {
                        TC_LOG_INFO("network", "BattlePay: entitlement {} was already consumed; skipping.", distributionId);
                        continue;
                    }

                    if (serviceType != 0)
                    {
                        // A VAS service (boost, rename, faction/race change, transfer). This core implements none
                        // of them, so refuse loudly rather than silently swallowing a paid entitlement - and hand
                        // it straight back so nothing is lost.
                        sBattlePayMgr->TransitionEntitlement(distributionId, SHOP_ENTITLEMENT_FINISHED, 0,
                            SHOP_ENTITLEMENT_AVAILABLE, 0);
                        TC_LOG_ERROR("network", "BattlePay: entitlement {} names VAS service type {}, which this core "
                            "cannot perform; returned it to account {}.", distributionId, serviceType, GetAccountId());
                        continue;
                    }

                    ShopProduct const* product = sBattlePayMgr->GetProduct(productId);
                    if (!product || product->Deliverables.empty())
                    {
                        sBattlePayMgr->TransitionEntitlement(distributionId, SHOP_ENTITLEMENT_FINISHED, 0,
                            SHOP_ENTITLEMENT_AVAILABLE, 0);
                        TC_LOG_ERROR("network", "BattlePay: entitlement {} points at product {}, which no longer exists "
                            "or has no deliverables; returned it to account {}.", distributionId, productId, GetAccountId());
                        continue;
                    }

                    for (ShopDeliverable const& d : product->Deliverables)
                    {
                        switch (d.Type)
                        {
                        case 1:
                            BattlePayDeliverItem(target, d.Id, d.Count);
                            break;
                        case 2:
                            if (!target->HasSpell(d.Id))
                                target->LearnSpell(d.Id, false);
                            break;
                        case 3:
                            for (uint32 i = 0; i < std::max<uint32>(d.Count, 1u); ++i)
                                sWowTokenMgr->CreateToken(GetAccountId(), WOW_TOKEN_STATE_AUCTIONABLE);
                            SendCommerceTokenUpdate();
                            break;
                        default:
                            TC_LOG_ERROR("network", "BattlePay: entitlement {} carries unsupported deliverable type {}.",
                                distributionId, d.Type);
                            break;
                        }
                    }

                    TC_LOG_INFO("network", "BattlePay: delivered entitlement {} (product {} '{}') to {}.",
                        distributionId, productId, product->Name, target->GetName());
                } while (result->NextRow());
            }));
}

void WorldSession::HandleBattlePayGetPurchaseList(WorldPackets::BattlePay::GetPurchaseList& /*getPurchaseList*/)
{
    // The client polls this whenever the Shop is opened and blocks its purchase UI until it gets a reply.
    // Answer with this account's real purchase history from the shared ledger (C-13). ProductID may be 0
    // (a valid value, C-32) so it is never filtered. walletName is always sent empty/record-final.
    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLED))
        return;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BATTLEPAY_PURCHASE_ACCOUNT);
    stmt->setUInt32(0, GetAccountId());

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt)
        .WithPreparedCallback([this](PreparedQueryResult result)
            {
                WorldPackets::BattlePay::GetPurchaseListResponse response;
                response.Result = 0;    // PurchaseResult::Ok

                if (result)
                {
                    do
                    {
                        Field* fields = result->Fetch();
                        WorldPackets::BattlePay::PurchaseRecord& rec = response.Purchases.emplace_back();
                        rec.PurchaseID = fields[0].GetUInt64();
                        rec.Status = fields[1].GetInt32();
                        rec.ResultCode = fields[2].GetInt32();
                        rec.ProductID = fields[3].GetUInt32();
                        rec.BasePrice = fields[4].GetUInt64();
                        rec.UserPrice = fields[5].GetUInt64();
                        rec.TimeCreated = fields[6].GetInt64();
                    } while (result->NextRow());
                }

                SendPacket(response.Write());
            }));
}

// VAS (Value Added Services) status queries.
//
// These are the only two VAS opcodes a retail client actually sends unprompted - both appear at character
// select in every capture on this machine, and both have empty bodies. They were registered STATUS_IGNORED,
// which in this core is a bare `break;` in WorldSession: the packet was dropped without even a log line, so
// the client's VAS caches were never refreshed and never cleared.
//
// Answering with "nothing in flight" is the accurate state of this realm, and it is byte-identical to what
// retail sends when a player has no pending purchase.
void WorldSession::HandleUpdateVasPurchaseStates(WorldPackets::BattlePay::UpdateVasPurchaseStates& /*packet*/)
{
    WorldPackets::BattlePay::EnumVasPurchaseStatesResponse response;
    SendPacket(response.Write());
}

void WorldSession::HandleVasGetServiceStatus(WorldPackets::BattlePay::VasGetServiceStatus& /*packet*/)
{
    // ServiceStatus 0: this realm offers no paid character services. The low nibble's meaning is unknown,
    // so it is left 0 rather than given an invented value.
    WorldPackets::BattlePay::VasGetServiceStatusResponse response;
    SendPacket(response.Write());
}
