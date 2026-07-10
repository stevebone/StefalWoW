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
#include "DatabaseEnv.h"
#include "Log.h"
#include "Timer.h"
#include "World.h"
#include <fstream>

BattlePayMgr* BattlePayMgr::instance()
{
    static BattlePayMgr instance;
    return &instance;
}

void BattlePayMgr::Load()
{
    uint32 const oldMSTime = getMSTime();

    _productListBlob.clear();

    std::string const path = sWorld->GetDataPath() + "battlepay/product_list_68275.bin";
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
    {
        TC_LOG_INFO("server.loading", "BattlePay: no catalog blob at '{}' - the in-game Shop will open empty.", path);
        return;
    }

    std::streamsize const size = in.tellg();
    if (size <= 0)
    {
        TC_LOG_ERROR("server.loading", "BattlePay: catalog blob '{}' is empty.", path);
        return;
    }

    in.seekg(0, std::ios::beg);
    _productListBlob.resize(static_cast<size_t>(size));
    if (!in.read(reinterpret_cast<char*>(_productListBlob.data()), size))
    {
        TC_LOG_ERROR("server.loading", "BattlePay: failed reading catalog blob '{}'.", path);
        _productListBlob.clear();
        return;
    }

    TC_LOG_INFO("server.loading", "BattlePay: loaded {}-byte in-game Shop catalog in {} ms.",
        _productListBlob.size(), GetMSTimeDiffToNow(oldMSTime));
}

void BattlePayMgr::LoadProducts()
{
    uint32 const oldMSTime = getMSTime();

    _products.clear();

    //                                             0          1          2           3              4          5        6           7
    QueryResult result = WorldDatabase.Query("SELECT productId, costMoney, costItemId, costItemCount, grantType, grantId, grantCount, name FROM battlepay_product");
    if (!result)
    {
        TC_LOG_INFO("server.loading", "BattlePay: loaded 0 shop products (table `battlepay_product` empty or missing).");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        BattlePayProduct product;
        product.ProductID     = fields[0].GetUInt32();
        product.CostMoney     = fields[1].GetUInt64();
        product.CostItemId    = fields[2].GetUInt32();
        product.CostItemCount = fields[3].GetUInt32();
        product.GrantType     = fields[4].GetUInt8();
        product.GrantId       = fields[5].GetUInt32();
        product.GrantCount    = fields[6].GetUInt32();
        product.Name          = fields[7].GetString();

        if (!product.GrantId || (product.GrantType != 1 && product.GrantType != 2))
        {
            TC_LOG_ERROR("sql.sql", "BattlePay: product {} has invalid grantType {} / grantId {} - skipped.",
                product.ProductID, product.GrantType, product.GrantId);
            continue;
        }
        if (!product.GrantCount)
            product.GrantCount = 1;

        _products[product.ProductID] = std::move(product);
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", "BattlePay: loaded {} shop products in {} ms.", _products.size(), GetMSTimeDiffToNow(oldMSTime));
}

BattlePayProduct const* BattlePayMgr::GetProduct(uint32 productID) const
{
    auto itr = _products.find(productID);
    return itr != _products.end() ? &itr->second : nullptr;
}
