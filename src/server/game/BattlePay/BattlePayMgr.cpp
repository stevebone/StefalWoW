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

#include "BattlePayMgr.h"
#include "BattlePayCatalogWriter.h"
#include "Config.h"
#include "ConditionMgr.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Log.h"
#include "Player.h"
#include "Realm.h"
#include "RealmList.h"
#include "StringFormat.h"
#include "Timer.h"
#include "World.h"
#include <algorithm>
#include <fstream>

namespace
{
    constexpr uint32 DISPLAY_FLAG_HIDDEN_PRICE   = 8;
    constexpr uint32 DISPLAY_FLAG_HIDE_WHEN_OWNED = 256;

    bool InWindow(ShopProduct const& p, time_t now)
    {
        return (p.AvailableFrom == 0 || now >= p.AvailableFrom)
            && (p.AvailableUntil == 0 || now <= p.AvailableUntil);
    }
}

BattlePayMgr* BattlePayMgr::instance()
{
    static BattlePayMgr instance;
    return &instance;
}

bool BattlePayMgr::LoadBlobFile(std::string const& fileName, std::vector<uint8>& out)
{
    out.clear();

    std::string const path = sWorld->GetDataPath() + "battlepay/" + fileName;
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
    {
        TC_LOG_INFO("server.loading", "BattlePay: no blob at '{}'.", path);
        return false;
    }

    std::streamsize const size = in.tellg();
    if (size <= 0)
    {
        TC_LOG_ERROR("server.loading", "BattlePay: blob '{}' is empty.", path);
        return false;
    }

    in.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    if (!in.read(reinterpret_cast<char*>(out.data()), size))
    {
        TC_LOG_ERROR("server.loading", "BattlePay: failed reading blob '{}'.", path);
        out.clear();
        return false;
    }

    return true;
}

void BattlePayMgr::Load()
{
    uint32 const oldMSTime = getMSTime();

    if (LoadBlobFile("product_list_68275.bin", _templateBlob))
    {
        TC_LOG_INFO("server.loading", "BattlePay: loaded {}-byte catalog template in {} ms.",
            _templateBlob.size(), GetMSTimeDiffToNow(oldMSTime));

        // Trust the reskin path only if the writer reproduces the template byte-exact.
        if (!BattlePayCatalogWriter::SelfCheck(_templateBlob))
            TC_LOG_ERROR("server.loading", "BattlePay: catalog writer self-check FAILED; catalog will be served verbatim (no DB reskin).");
    }
    else
        TC_LOG_INFO("server.loading", "BattlePay: no catalog template - the in-game Shop will open empty.");

    // The distribution list unblocks the client's shop panel (StoreFrame_IsLoading). Replay the
    // captured 68275 blob; absence is non-fatal (the panel just keeps waiting on HasDistributionList).
    if (LoadBlobFile("distribution_list_68275.bin", _distributionListBlob))
        TC_LOG_INFO("server.loading", "BattlePay: loaded {}-byte distribution list.", _distributionListBlob.size());

    // Seed the persistent PurchaseID counter from the ledger so ids survive restarts and never repeat
    // (C-13). Namespace this realm's ids in the high 32 bits so multiple realms sharing one auth DB never
    // collide (C-22): this realm allocates only within [realmBase, realmBase + 0xFFFFFFFF].
    uint64 const realmBase = uint64(sRealmList->GetCurrentRealmId().Realm) << 32;
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BATTLEPAY_PURCHASE_MAXID);
    stmt->setUInt64(0, realmBase);
    stmt->setUInt64(1, realmBase | UI64LIT(0xFFFFFFFF));
    PreparedQueryResult maxIdResult = LoginDatabase.Query(stmt);
    if (maxIdResult && !(*maxIdResult)[0].IsNull())
        _purchaseCounter = (*maxIdResult)[0].GetUInt64();
    else
        _purchaseCounter = realmBase;
}

void BattlePayMgr::RecordPurchase(uint32 accountId, uint64 purchaseID, int32 status, int32 resultCode,
    uint32 productID, uint64 basePrice, uint64 userPrice)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_BATTLEPAY_PURCHASE);
    stmt->setUInt64(0, purchaseID);
    stmt->setUInt32(1, accountId);
    stmt->setUInt32(2, productID);
    stmt->setInt32(3, status);
    stmt->setInt32(4, resultCode);
    stmt->setUInt64(5, basePrice);
    stmt->setUInt64(6, userPrice);
    stmt->setInt64(7, GameTime::GetGameTime());
    stmt->setString(8, std::string());     // walletName: empty on this core (sent record-final on the wire)
    LoginDatabase.Execute(stmt);
}

void BattlePayMgr::LoadProducts()
{
    _products.clear();
    _slotOverrides.clear();

    //                                                    0          1        2     3            4         5      6            7               8             9             10       11        12        13                             14                              15        16          17           18                 19
    QueryResult result = WorldDatabase.Query("SELECT productId, enabled, name, description, currency, price, priceItemId, priceItemCount, displayPrice, displayFlags, groupId, ordering, featured, UNIX_TIMESTAMP(availableFrom), UNIX_TIMESTAMP(availableUntil), reqLevel, reqFaction, hideIfOwned, playerConditionId, comment FROM shop_product");
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            ShopProduct p;
            p.ProductID      = f[0].GetUInt32();
            p.Enabled        = f[1].GetUInt8() != 0;
            p.Name           = f[2].GetString();
            p.Description    = f[3].GetString();
            p.Currency       = f[4].GetUInt8();
            p.Price          = f[5].GetUInt64();
            p.PriceItemId    = f[6].GetUInt32();
            p.PriceItemCount = f[7].GetUInt32();
            if (!f[8].IsNull())
            {
                p.HasDisplayPrice = true;
                p.DisplayPrice = f[8].GetUInt64();
            }
            p.DisplayFlags       = f[9].GetUInt32();
            p.GroupId            = f[10].GetUInt32();
            p.Ordering           = f[11].GetInt32();
            p.Featured           = f[12].GetUInt8() != 0;
            if (!f[13].IsNull())
                p.AvailableFrom = time_t(f[13].GetInt64());
            if (!f[14].IsNull())
                p.AvailableUntil = time_t(f[14].GetInt64());
            p.ReqLevel           = f[15].GetUInt8();
            p.ReqFaction         = f[16].GetInt8();
            p.HideIfOwned        = f[17].GetUInt8() != 0;
            p.PlayerConditionId  = f[18].GetUInt32();
            p.Comment            = f[19].GetString();
            _products[p.ProductID] = std::move(p);
        }
        while (result->NextRow());

        if (QueryResult deliverables = WorldDatabase.Query("SELECT productId, seq, type, id, count FROM shop_product_deliverable ORDER BY productId, seq"))
        {
            do
            {
                Field* f = deliverables->Fetch();
                uint32 const productId = f[0].GetUInt32();
                auto itr = _products.find(productId);
                if (itr == _products.end())
                {
                    TC_LOG_ERROR("sql.sql", "BattlePay: shop_product_deliverable references unknown productId {} - skipped.", productId);
                    continue;
                }
                ShopDeliverable dv;
                dv.Type  = f[2].GetUInt8();
                dv.Id    = f[3].GetUInt32();
                dv.Count = f[4].GetUInt32();
                if (!dv.Count)
                    dv.Count = 1;
                itr->second.Deliverables.push_back(dv);
            }
            while (deliverables->NextRow());
        }
    }

    if (QueryResult overrides = WorldDatabase.Query("SELECT slotIndex, productId FROM shop_slot_override"))
    {
        do
        {
            Field* f = overrides->Fetch();
            _slotOverrides[f[0].GetUInt8()] = f[1].GetUInt32();
        }
        while (overrides->NextRow());
    }

    TC_LOG_INFO("server.loading", "BattlePay: loaded {} shop products, {} slot overrides.", _products.size(), _slotOverrides.size());
}

bool BattlePayMgr::AssembleCatalog(std::vector<uint8>& outBlob, std::unordered_map<uint32, uint32>& outRouting,
    std::string* report) const
{
    outRouting.clear();

    if (_templateBlob.empty())
        return false;

    std::vector<uint32> header;
    std::vector<BattlePayCatalogRecord> records;
    std::vector<uint8> remainder;
    if (!BattlePayCatalogWriter::Parse(_templateBlob, header, records, remainder))
    {
        TC_LOG_ERROR("server.loading", "BattlePay: catalog template did not parse; serving it verbatim.");
        outBlob = _templateBlob;
        return false;
    }

    time_t const now = GameTime::GetGameTime();
    std::string const placeholderName = std::string(sConfigMgr->GetStringDefault("Shop.PlaceholderName", "Currently unavailable"));

    // Products pinned to a specific slot are excluded from the automatic fill so they show once only.
    std::unordered_map<uint32, bool> pinned;
    for (auto const& [slot, productId] : _slotOverrides)
        if (productId)
            pinned[productId] = true;

    // Candidate set = enabled + in-window, not pinned; sorted featured DESC, ordering ASC, productId ASC.
    std::vector<ShopProduct const*> candidates;
    for (auto const& [id, product] : _products)
        if (product.Enabled && InWindow(product, now) && pinned.find(id) == pinned.end())
            candidates.push_back(&product);

    std::sort(candidates.begin(), candidates.end(), [](ShopProduct const* a, ShopProduct const* b)
    {
        if (a->Featured != b->Featured) return a->Featured > b->Featured;
        if (a->Ordering != b->Ordering) return a->Ordering < b->Ordering;
        return a->ProductID < b->ProductID;
    });

    auto reskin = [&](BattlePayCatalogRecord& rec, ShopProduct const& product)
    {
        uint64 displayPrice;
        if (product.HasDisplayPrice)
            displayPrice = product.DisplayPrice;
        else if (product.Currency == 1)                 // gold: copper -> shop fixed-point /100000
            displayPrice = (product.Price / 10000) * 100000;
        else
            displayPrice = 0;

        uint32 flags = product.DisplayFlags;
        if (!product.HasDisplayPrice && product.Currency != 0 && product.Currency != 1)
            flags |= DISPLAY_FLAG_HIDDEN_PRICE;         // non-gold currency w/o override: hide the price line
        if (product.HideIfOwned)
            flags |= DISPLAY_FLAG_HIDE_WHEN_OWNED;

        rec.Name         = product.Name;
        rec.Description  = product.Description;
        rec.NormalPrice  = displayPrice;
        rec.CurrentPrice = displayPrice;
        rec.Flags        = flags;
    };

    size_t candIdx = 0;
    for (size_t slot = 0; slot < records.size(); ++slot)
    {
        uint32 const slotProductId = records[slot].ProductID;
        ShopProduct const* assigned = nullptr;

        auto ovr = _slotOverrides.find(uint8(slot));
        if (ovr != _slotOverrides.end())
        {
            if (ovr->second != 0)                       // pinned; 0 = forced placeholder
                assigned = GetProduct(ovr->second);
        }
        else
        {
            if (candIdx < candidates.size())
                assigned = candidates[candIdx++];
        }

        if (assigned)
        {
            reskin(records[slot], *assigned);
            outRouting[slotProductId] = assigned->ProductID;
            if (report)
                report->append(Trinity::StringFormat("  slot {}: [{}] '{}' -> product {} (price {}, {}{})\n",
                    slot, slotProductId, assigned->Name, assigned->ProductID, assigned->Price,
                    assigned->Enabled ? "enabled" : "disabled", assigned->Featured ? ", featured" : ""));
        }
        else
        {
            records[slot].Name = placeholderName;
            records[slot].Description.clear();
            records[slot].NormalPrice = 0;
            records[slot].CurrentPrice = 0;
            records[slot].Flags |= DISPLAY_FLAG_HIDDEN_PRICE;
            if (report)
                report->append(Trinity::StringFormat("  slot {}: [{}] <placeholder - not purchasable>\n", slot, slotProductId));
        }
    }

    if (report && candIdx < candidates.size())
        report->append(Trinity::StringFormat("  OVERFLOW: {} enabled product(s) could not be shown (only {} slots).\n",
            candidates.size() - candIdx, records.size()));

    outBlob = BattlePayCatalogWriter::Serialize(header, records, remainder);
    return true;
}

void BattlePayMgr::LoadCatalog()
{
    uint32 const oldMSTime = getMSTime();

    LoadProducts();

    std::vector<uint8> blob;
    std::unordered_map<uint32, uint32> routing;
    if (_products.empty())
    {
        // No DB catalog: serve the raw template so the shop still opens (nothing purchasable).
        _productListBlob = _templateBlob;
        _slotRouting.clear();
        TC_LOG_INFO("server.loading", "BattlePay: no shop_product rows; serving the catalog template verbatim.");
    }
    else if (AssembleCatalog(blob, routing, nullptr))
    {
        _productListBlob = std::move(blob);
        _slotRouting = std::move(routing);
        TC_LOG_INFO("server.loading", "BattlePay: assembled {}-byte catalog ({} routed slots) in {} ms.",
            _productListBlob.size(), _slotRouting.size(), GetMSTimeDiffToNow(oldMSTime));
    }
    else
    {
        _productListBlob = _templateBlob;               // assembly failed: fall back to verbatim
        _slotRouting.clear();
    }

    ++_catalogGeneration;

    // Schedule the next automatic rebuild at the earliest future window boundary (restart-free rotation).
    time_t const now = GameTime::GetGameTime();
    _nextRebuildTime = 0;
    for (auto const& [id, product] : _products)
    {
        for (time_t boundary : { product.AvailableFrom, product.AvailableUntil })
            if (boundary > now && (_nextRebuildTime == 0 || boundary < _nextRebuildTime))
                _nextRebuildTime = boundary;
    }
}

void BattlePayMgr::Reload()
{
    LoadCatalog();
}

void BattlePayMgr::RebuildIfDue(time_t now)
{
    if (_nextRebuildTime != 0 && now >= _nextRebuildTime)
    {
        TC_LOG_INFO("server.loading", "BattlePay: availability window boundary reached; rebuilding catalog.");
        Reload();
    }
}

ShopProduct const* BattlePayMgr::GetProduct(uint32 adminProductId) const
{
    auto itr = _products.find(adminProductId);
    return itr != _products.end() ? &itr->second : nullptr;
}

ShopProduct const* BattlePayMgr::GetProductByAdvertisedId(uint32 advertisedProductId) const
{
    auto route = _slotRouting.find(advertisedProductId);
    if (route == _slotRouting.end())
        return nullptr;
    return GetProduct(route->second);
}

bool BattlePayMgr::IsAlreadyFullyOwned(ShopProduct const& product, Player* player)
{
    if (product.Deliverables.empty())
        return false;
    for (ShopDeliverable const& d : product.Deliverables)
    {
        if (d.Type != 2)                                // only spell-only bundles count as "ownable"
            return false;
        if (!player->HasSpell(d.Id))
            return false;
    }
    return true;
}

bool BattlePayMgr::IsPurchasable(ShopProduct const& product, Player* player, time_t now) const
{
    if (!player)
        return false;
    if (!product.Enabled || !InWindow(product, now))
        return false;
    if (product.ReqLevel && player->GetLevel() < product.ReqLevel)
        return false;
    if (product.ReqFaction >= 0 && int8(player->GetTeamId()) != product.ReqFaction)
        return false;
    if (product.HideIfOwned && IsAlreadyFullyOwned(product, player))
        return false;
    if (product.PlayerConditionId && !ConditionMgr::IsPlayerMeetingCondition(player, product.PlayerConditionId))
        return false;
    return true;
}

std::string BattlePayMgr::BuildStatusReport() const
{
    std::string report = Trinity::StringFormat("In-game Shop catalog: {} product(s), template {} bytes.\n",
        _products.size(), _templateBlob.size());

    std::vector<uint8> blob;
    std::unordered_map<uint32, uint32> routing;
    AssembleCatalog(blob, routing, &report);
    report.append(Trinity::StringFormat("Assembled blob: {} bytes, generation {}.", blob.size(), _catalogGeneration));
    return report;
}
