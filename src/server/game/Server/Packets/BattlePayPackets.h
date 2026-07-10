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

#ifndef TRINITYCORE_BATTLE_PAY_PACKETS_H
#define TRINITYCORE_BATTLE_PAY_PACKETS_H

#include "Packet.h"
#include <vector>

namespace WorldPackets
{
    namespace BattlePay
    {
        // Client requests the shop catalog. Body carries a locale/region selector we do not need.
        class GetProductList final : public ClientPacket
        {
        public:
            explicit GetProductList(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_GET_PRODUCT_LIST, std::move(packet)) { }

            void Read() override { }
        };

        // Client requests the account purchase/distribution list.
        class GetPurchaseList final : public ClientPacket
        {
        public:
            explicit GetPurchaseList(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_GET_PURCHASE_LIST, std::move(packet)) { }

            void Read() override { }
        };

        // The 12.0.7 catalog is a nested reflection bitstream that cannot be re-serialized field-by-field
        // offline (see docs). For P0 we replay a byte-exact, client-validated catalog blob captured from a
        // real 68275 session, so the shop opens and displays real products. RawData is the message BODY
        // (opcode dword already stripped); the ServerPacket base prepends the opcode header.
        class ProductListResponse final : public ServerPacket
        {
        public:
            explicit ProductListResponse() : ServerPacket(SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE) { }

            WorldPacket const* Write() override;

            std::vector<uint8> const* RawData = nullptr;
        };

        // Client initiates an in-game purchase. Layout from the client Write method (0x5d9f90):
        // u32, u64, then a 1-bit bool. The u32 is the strong candidate for the productID (the setter is
        // Warden-obfuscated so the exact semantic is runtime-confirmed via the handler's diagnostic log).
        class StartPurchase final : public ClientPacket
        {
        public:
            explicit StartPurchase(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_START_PURCHASE, std::move(packet)) { }

            void Read() override;

            uint32 ProductID = 0;   // candidate (scalar_u32)
            uint64 ScalarU64 = 0;   // candidate: target character GUID
            bool Flag = false;
        };

        // Client opens the checkout for a previously-created distribution (sniff-confirmed: u32 distributionID).
        class OpenCheckout final : public ClientPacket
        {
        public:
            explicit OpenCheckout(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_OPEN_CHECKOUT, std::move(packet)) { }

            void Read() override;

            uint32 DistributionID = 0;
        };

        // Server ack for StartPurchase. Layout from the client read ctor (0x608ec0): u32, u32, u64.
        class StartPurchaseResponse final : public ServerPacket
        {
        public:
            explicit StartPurchaseResponse() : ServerPacket(SMSG_BATTLE_PAY_START_PURCHASE_RESPONSE, 16) { }

            WorldPacket const* Write() override;

            uint32 ResultA = 0;
            uint32 ResultB = 0;
            uint64 PurchaseID = 0;
        };

        // One JamBattlePayPurchase record (descriptor field order). walletName is sent empty (see .cpp).
        struct PurchaseRecord
        {
            uint64 PurchaseID = 0;
            int32 Status = 0;       // BattlepayPurchaseStatus: Done=3, Failed=4
            int32 ResultCode = 0;   // PurchaseResult: Ok=0
            uint32 ProductID = 0;
            uint64 BasePrice = 0;
            uint64 UserPrice = 0;
            int64 TimeCreated = 0;
        };

        // Server drives purchase progress/completion. Layout from client read ctor (0x6090d0):
        // u32 result, then a u32-counted vector of JamBattlePayPurchase. status=Done(3) signals completion
        // and the record echoes the productID delivered.
        class PurchaseUpdate final : public ServerPacket
        {
        public:
            explicit PurchaseUpdate() : ServerPacket(SMSG_BATTLE_PAY_PURCHASE_UPDATE) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            std::vector<PurchaseRecord> Purchases;
        };
    }
}

#endif // TRINITYCORE_BATTLE_PAY_PACKETS_H
