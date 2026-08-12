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

#ifndef TRINITYCORE_BATTLE_PAY_MGR_H
#define TRINITYCORE_BATTLE_PAY_MGR_H

#include "Define.h"
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

// A single deliverable payload of a shop product (>1 per product = a bundle).
struct ShopDeliverable
{
    uint8  Type  = 0;   // 1 item | 2 spell | 3 WoW Token | 4 game-time (reserved) | 5 service (reserved)
    uint32 Id    = 0;   // itemId / spellId / 0 (token) / days / serviceType
    uint32 Count = 1;
};

// An admin-defined shop product (row of `shop_product` + its `shop_product_deliverable` rows).
// The catalog wire can only express name/description/price/flags; everything else here is enforced
// server-side at purchase time (IsPurchasable) or used to decide slot assignment.
struct ShopProduct
{
    uint32 ProductID = 0;           // admin id (routing frees it from the blob's fixed slot ids)
    bool   Enabled   = true;
    std::string Name;
    std::string Description;
    uint8  Currency  = 1;           // 0 free | 1 gold(copper) | 2 item-token | 3 custom-currency
    uint64 Price     = 0;           // copper (currency 1) or currency amount (3)
    uint32 PriceItemId = 0;         // currency 2: token item
    uint32 PriceItemCount = 0;
    bool   HasDisplayPrice = false; // wire fixed-point /100000 override (NULL in DB => derived)
    uint64 DisplayPrice = 0;
    uint32 DisplayFlags = 0;        // BattlepayDisplayFlags (8 HiddenPrice, 256 HideWhenOwned)
    uint32 GroupId   = 0;           // stored; rendered only once the ShopEntry region is cracked (SH-7)
    int32  Ordering  = 0;           // slot-assignment priority (lower = earlier slot)
    bool   Featured  = false;
    time_t AvailableFrom  = 0;      // 0 = always
    time_t AvailableUntil = 0;      // 0 = always
    uint8  ReqLevel  = 0;
    int8   ReqFaction = -1;         // -1 any, else TeamId (0 alliance, 1 horde)
    bool   HideIfOwned = false;
    uint32 PlayerConditionId = 0;
    std::string Comment;
    std::vector<ShopDeliverable> Deliverables;
};

// In-game Shop (BattlePay / StoreUI) backend + catalog administration.
//
// The captured 68275 GET_PRODUCT_LIST_RESPONSE blob is a TEMPLATE. LoadCatalog() reskins its 9
// simple-shape slots from the `shop_product` DB rows via the byte-exact BattlePayCatalogWriter and
// records a slot->product routing map, so the offer set is DB-driven and reloadable without a restart.
// Everything the wire cannot carry (enable/disable, windows, level/faction/owned/condition gates) is
// enforced at purchase time by IsPurchasable().
class TC_GAME_API BattlePayMgr
{
public:
    static BattlePayMgr* instance();

    // Loads the raw template + distribution blobs from <DataDir>/battlepay/.
    void Load();

    // Reads shop_product / shop_product_deliverable / shop_slot_override, assembles the catalog blob
    // from the template, and builds the routing map. Call after Load().
    void LoadCatalog();

    // Atomic re-run of LoadProducts + LoadCatalog on the world thread (.reload shop_catalog / .shop).
    void Reload();

    bool HasCatalog() const { return !_productListBlob.empty(); }
    std::vector<uint8> const& GetProductListBlob() const { return _productListBlob; }

    // Bumped every time the catalog blob is (re)built; drives the once-per-session send throttle.
    uint32 GetCatalogGeneration() const { return _catalogGeneration; }

    // Distribution list (see Load()); unblocks the client's shop panel.
    bool HasDistributionList() const { return !_distributionListBlob.empty(); }
    std::vector<uint8> const& GetDistributionListBlob() const { return _distributionListBlob; }

    // Purchase routing: the client buys by the advertised (slot) productID, which the assembly kept;
    // this resolves it to the admin ShopProduct. Unrouted (placeholder) slots return nullptr.
    ShopProduct const* GetProductByAdvertisedId(uint32 advertisedProductId) const;
    // Direct lookup by admin productId (used by the .shop commands).
    ShopProduct const* GetProduct(uint32 adminProductId) const;
    std::unordered_map<uint32, ShopProduct> const& GetProducts() const { return _products; }

    // Purchase-time authority for everything the wire cannot express.
    bool IsPurchasable(ShopProduct const& product, Player* player, time_t now) const;
    // True if the product's payload is entirely spells the player already knows (never charge for nothing).
    static bool IsAlreadyFullyOwned(ShopProduct const& product, Player* player);

    // Earliest future availability-window boundary (0 = none); World tick rebuilds when it passes.
    time_t GetNextRebuildTime() const { return _nextRebuildTime; }
    void RebuildIfDue(time_t now);

    // Dry-run assembly summary for `.shop preview` / `.shop list`.
    std::string BuildStatusReport() const;

    // Allocates the next PurchaseID. The counter is seeded from the ledger at startup (survives restart)
    // and namespaced by realm id in its high 32 bits, so PurchaseIDs never collide across realms sharing
    // one auth DB (C-13/C-22). One worldserver per realm means the in-memory increment is race-free.
    uint64 GeneratePurchaseID() { return ++_purchaseCounter; }

    // Persists a completed purchase to the shared account_battlepay_purchase ledger (auth DB), the home
    // that answers CMSG_BATTLE_PAY_GET_PURCHASE_LIST. ProductID may legitimately be 0 (C-32).
    void RecordPurchase(uint32 accountId, uint64 purchaseID, int32 status, int32 resultCode,
        uint32 productID, uint64 basePrice, uint64 userPrice);

private:
    BattlePayMgr() = default;
    ~BattlePayMgr() = default;
    BattlePayMgr(BattlePayMgr const&) = delete;
    BattlePayMgr& operator=(BattlePayMgr const&) = delete;

    bool LoadBlobFile(std::string const& fileName, std::vector<uint8>& out);
    void LoadProducts();                                   // fills _products from the DB
    // Assembles the catalog blob into `outBlob` and the routing into `outRouting`; returns false on
    // a fatal error (no template / self-check failure). Used by LoadCatalog and BuildStatusReport.
    bool AssembleCatalog(std::vector<uint8>& outBlob, std::unordered_map<uint32, uint32>& outRouting,
        std::string* report) const;

    std::vector<uint8> _templateBlob;
    std::vector<uint8> _productListBlob;
    std::vector<uint8> _distributionListBlob;
    std::unordered_map<uint32, ShopProduct> _products;         // admin productId -> product
    std::unordered_map<uint32, uint32> _slotRouting;           // advertised (slot) productId -> admin id
    std::unordered_map<uint8, uint32> _slotOverrides;          // slotIndex -> admin id (0 = placeholder)
    uint64 _purchaseCounter = 0;
    uint32 _catalogGeneration = 0;
    time_t _nextRebuildTime = 0;
};

#define sBattlePayMgr BattlePayMgr::instance()

#endif // TRINITYCORE_BATTLE_PAY_MGR_H
