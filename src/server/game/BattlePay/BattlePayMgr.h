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
#include <vector>

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

    bool HasCatalog() const { return !_productListBlob.empty(); }
    std::vector<uint8> const& GetProductListBlob() const { return _productListBlob; }

private:
    BattlePayMgr() = default;
    ~BattlePayMgr() = default;
    BattlePayMgr(BattlePayMgr const&) = delete;
    BattlePayMgr& operator=(BattlePayMgr const&) = delete;

    std::vector<uint8> _productListBlob;
};

#define sBattlePayMgr BattlePayMgr::instance()

#endif // TRINITYCORE_BATTLE_PAY_MGR_H
