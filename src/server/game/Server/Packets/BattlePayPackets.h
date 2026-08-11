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

        // Sent unsolicited at session start (there is no matching CMSG). The client's shop panel gate
        // StoreFrame_IsLoading blocks until HasDistributionList() is true, which this response flips.
        // We replay a byte-exact 107 B blob captured from a real 68275 session (6 B header + 101 B
        // JamBattlePayDistributionObject record). RawData is the message BODY (opcode header prepended).
        class GetDistributionListResponse final : public ServerPacket
        {
        public:
            explicit GetDistributionListResponse() : ServerPacket(SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE) { }

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

            // Layout settled from live 505-byte packets (three clicks, two products):
            //   @0  u32 ClientToken - a per-session click counter (observed 1, 2, 3)
            //   @4  u32 ProductID   - the entry productID the client buys by; stable across two
            //                         attempts at the same pet (1448, 1448) and different for
            //                         another card (1061). This is entryInfo.productID, which
            //                         Blizzard_StoreUI passes to C_StoreSecure.PurchaseProduct().
            //   @8  u32 (always 0 so far)
            //   then bit-packed lengths + "win" (platform) + a ~480 char client attestation blob
            //   { "RGKY" : ..., "CPGE" : ... } which we do not need and do not parse.
            // The old reader took the FIRST scalar as the product, i.e. the click counter, so no
            // purchase ever resolved.
            uint32 ClientToken = 0;
            uint32 ProductID = 0;
            uint32 Unused = 0;
            bool Flag = false;
        };

        // Client opens the checkout. The u32 is the ClientToken the server must echo back verbatim in
        // SMSG_GENERATE_SSO_TOKEN_RESPONSE (proven 1:1 in all 8 captures - checkout #N -> response #N
        // with the same u32). It is not a distributionID. See COMMERCE_AUDIT C-09.
        class OpenCheckout final : public ClientPacket
        {
        public:
            explicit OpenCheckout(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_OPEN_CHECKOUT, std::move(packet)) { }

            void Read() override;

            uint32 ClientToken = 0;
        };

        // Server-driven purchase confirmation prompt (retail interposes this between StartPurchase and
        // completion; it clears the client's WaitingOnConfirmation and shows the confirm dialog, whose
        // C_StoreSecure.GetConfirmationInfo() reads productID, walletName and the current price).
        //
        // INFERRED LAYOUT - NOT byte-verified: the 68275 client read struct (sub_7FF7290A91A0, opcode
        // 0x420232) is an opaque nested reflection struct in the RE dump and this opcode never appears
        // on-wire in any of the 8 captures (retail hands purchases to web checkout). The field set below
        // is the classic JamBattlePayConfirmPurchase shape that GetConfirmationInfo consumes. Because a
        // malformed packet could disconnect a live client, sending this is gated behind the
        // Shop.PurchaseConfirmation config (default off) until a live client validates the layout; the
        // proven direct-grant path (StartPurchase -> grant/charge -> PurchaseUpdate) stays the default.
        class ConfirmPurchase final : public ServerPacket
        {
        public:
            explicit ConfirmPurchase() : ServerPacket(SMSG_BATTLE_PAY_CONFIRM_PURCHASE, 8 + 4 + 8 + 4 + 1) { }

            WorldPacket const* Write() override;

            uint64 PurchaseID = 0;
            uint32 ProductID = 0;
            uint64 CurrentPriceFixedPoint = 0;  // wire fixed-point /100000 (same scale as the catalog)
            uint32 ServerToken = 0;             // echoed back in the response so we can match the prompt
        };

        // Client's answer to the confirmation prompt. Layout byte-grounded from the 68275 client read of
        // CMSG_BATTLE_PAY_CONFIRM_PURCHASE_RESPONSE (0x4000fb): a u32 then a 1-bit bool. The u32 echoes
        // our ServerToken; the bool is confirm(true)/cancel(false).
        class ConfirmPurchaseResponse final : public ClientPacket
        {
        public:
            explicit ConfirmPurchaseResponse(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_CONFIRM_PURCHASE_RESPONSE, std::move(packet)) { }

            void Read() override;

            uint32 ServerToken = 0;
            bool Confirmed = false;
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

        // One JamBattlePayPurchase record. Wire order (68974 capture, TESTER_SNIFF2_LINDORMI_MINE):
        // fields below in declaration order, then a record-final u8 walletName length (sent empty, see .cpp).
        struct PurchaseRecord
        {
            uint64 PurchaseID = 0;
            int32 Status = 0;       // BattlepayPurchaseStatus: live 68974 completed purchases carry 6 (failed VAS: 12)
            int32 ResultCode = 0;   // PurchaseResult: Ok=0
            uint32 ProductID = 0;
            uint64 BasePrice = 0;
            uint64 UserPrice = 0;
            int64 TimeCreated = 0;
        };

        // Server drives purchase progress/completion. Layout from client read ctor (0x6090d0):
        // u32 result, then a u32-counted vector of JamBattlePayPurchase. status=6 signals completion
        // (live 68974 value; see PurchaseRecord) and the record echoes the productID delivered.
        class PurchaseUpdate final : public ServerPacket
        {
        public:
            explicit PurchaseUpdate() : ServerPacket(SMSG_BATTLE_PAY_PURCHASE_UPDATE) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            std::vector<PurchaseRecord> Purchases;
        };

        // Reply to CMSG_BATTLE_PAY_GET_PURCHASE_LIST. Body layout is identical to
        // SMSG_BATTLE_PAY_PURCHASE_UPDATE: { uint32 Result, uint32 Count, Count x PurchaseRecord }.
        // Proven against a live sniff: a retail account with 9 purchases produced a 413-byte body, and
        // 8 (header) + 9 * 45 (PurchaseRecord = u64+i32+i32+u32+u64+u64+i64+u8) == 413 exactly. The
        // record layout (walletName length record-final) matches the fixed PurchaseUpdate serializer.
        class GetPurchaseListResponse final : public ServerPacket
        {
        public:
            explicit GetPurchaseListResponse() : ServerPacket(SMSG_BATTLE_PAY_GET_PURCHASE_LIST_RESPONSE, 4 + 4) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            std::vector<PurchaseRecord> Purchases;
        };
    }
}

#endif // TRINITYCORE_BATTLE_PAY_PACKETS_H
