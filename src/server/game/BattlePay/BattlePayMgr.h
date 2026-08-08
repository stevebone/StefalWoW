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
#include <string>
#include <unordered_map>
#include <vector>

// A server-defined shop product: how it is paid for and what it grants. Keyed by the productID that the
// catalog advertises to the client (so a purchase packet's productID maps straight to grant + cost).
struct BattlePayProduct
{
    uint32 ProductID   = 0;
    uint64 CostMoney   = 0;   // copper; 0 = free
    uint32 CostItemId  = 0;   // token item id; 0 = none
    uint32 CostItemCount = 0;
    uint8  GrantType   = 0;   // 1 = item, 2 = spell (mount/toy/appearance learned as a spell)
    uint32 GrantId     = 0;
    uint32 GrantCount  = 1;
    std::string Name;
};

// In-game Shop (BattlePay / StoreUI) backend.
//
// The 12.0.7 GET_PRODUCT_LIST_RESPONSE catalog is a nested reflection bitstream whose per-field bit
// widths are not recoverable offline, so we cannot author a custom catalog field-by-field yet. For P0
// the manager loads a byte-exact catalog blob captured from a real 68275 client session and replays it
// verbatim, so the shop opens and shows real products. The purchase/deliver-for-gold path is layered on
// top later (tracked separately) once the buy flow is grounded.
class TC_GAME_API BattlePayMgr
{
public:
    static BattlePayMgr* instance();

    // Loads the captured catalog blob from <DataDir>/battlepay/product_list_68275.bin (if present).
    void Load();

    // Loads server-defined purchasable products from the world DB (battlepay_product).
    void LoadProducts();

    bool HasCatalog() const { return !_productListBlob.empty(); }
    std::vector<uint8> const& GetProductListBlob() const { return _productListBlob; }

    // Bumped every time the catalog blob is (re)built. A session serves the 58 KB blob at most once per
    // generation, so a client that polls GetProductList each shop open is not re-fed the blob until a
    // `.reload shop_catalog` changes it (anti-amplification without breaking restart-free rotation).
    uint32 GetCatalogGeneration() const { return _catalogGeneration; }

    // Distribution list: the client's StoreFrame_IsLoading gate blocks the shop panel until
    // HasDistributionList() is true, which only flips once it receives a
    // SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE. We replay a captured 68275 blob at session start.
    bool HasDistributionList() const { return !_distributionListBlob.empty(); }
    std::vector<uint8> const& GetDistributionListBlob() const { return _distributionListBlob; }

    BattlePayProduct const* GetProduct(uint32 productID) const;
    uint64 GeneratePurchaseID() { return ++_purchaseCounter; }

private:
    BattlePayMgr() = default;
    ~BattlePayMgr() = default;
    BattlePayMgr(BattlePayMgr const&) = delete;
    BattlePayMgr& operator=(BattlePayMgr const&) = delete;

    // Reads a raw blob file from <DataDir>/battlepay/<fileName>; returns true and fills out on success.
    bool LoadBlobFile(std::string const& fileName, std::vector<uint8>& out);

    std::vector<uint8> _productListBlob;
    std::vector<uint8> _distributionListBlob;
    std::unordered_map<uint32, BattlePayProduct> _products;
    uint64 _purchaseCounter = 0;
    uint32 _catalogGeneration = 0;
};

#define sBattlePayMgr BattlePayMgr::instance()

#endif // TRINITYCORE_BATTLE_PAY_MGR_H
