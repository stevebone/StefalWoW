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
#include "ObjectGuid.h"
#include "Optional.h"
#include <string>
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

        // JamBattlePayDeliverable - what a product actually hands over. Layout recovered from the client's
        // own parser (sub_7FF729139460 @ image base 0x7FF728AA0000) and validated byte-exact against the
        // 68275 capture, where the embedded record decodes with zero remainder to catalog deliverable
        // 1161 { type = 1 CharacterBoost, boostID = 11, flags = 1620 } - the very values our decoded
        // catalog carries for that deliverable, which is what closes the loop on this struct.
        //
        // `Type` is the catalog's own deliverable vocabulary: 1 CharacterBoost, 2 BattlePet, 3 Mount,
        // 4 WowToken, 5 NameChange, 6 FactionChange, 8 RaceChange, 11 CharacterTransfer, 13 TransmogSet,
        // 14 Item/Toy, 18 GameUpgrade, 26 TransmogEnsemble.
        struct DistributionDeliverable
        {
            uint32 DeliverableID = 0;
            uint32 Type = 0;
            uint32 ItemID = 0;
            uint32 Quantity = 0;
            uint32 MountSpellID = 0;
            uint32 BattlePetCreatureID = 0;
            uint32 BoostID = 0;
            uint32 Flags = 0;
            uint32 TransItemModifiedAppearanceID = 0;
            uint32 TransmogSetID = 0;
            uint32 CharTitleID = 0;
            uint32 SpellItemEnchantmentID = 0;
            uint32 WarbandSceneID = 0;
            std::string Name;
            bool AlreadyOwns = false;
            // Choices and DisplayInfo are never emitted: DisplayInfo is a ~21 KB bit-packed struct that
            // has NOT been decoded (it is absent from both capture samples, has_displayInfo = 0), so we
            // always write the "no choices, no display info" form the captures show.
        };

        // JamBattlePayDistributionObject - one entitlement. Layout recovered from the client's parser
        // (sub_7FF729139990) and validated byte-exact: the captured 101-byte body decodes to
        // { distributionID = 0x1E828000009EECAC, status = 1, deliverableID = 1161,
        //   licenseGameAccountGUID = packed mask 0x800F, targetPlayer = empty, realms = 0,
        //   purchaseID = 0, manualReview = 0, flags = 0x80 (hasDeliverable) } + the 55-byte deliverable,
        // consuming all 101 bytes with nothing left over.
        struct DistributionObject
        {
            uint64 DistributionID = 0;
            uint32 Status = 0;              // 1 = available; the only value ever observed on the wire
            uint32 DeliverableID = 0;
            ObjectGuid LicenseGameAccountGUID;
            ObjectGuid TargetPlayer;        // empty while unassigned
            uint32 TargetNativeRealm = 0;
            uint32 TargetVirtualRealm = 0;
            uint64 PurchaseID = 0;
            uint32 ManualReview = 0;
            bool Revoked = false;
            Optional<DistributionDeliverable> Deliverable;
        };

        // Sent unsolicited at session start (character select) and again at login. There is no CMSG that
        // requests it - confirmed against the client's opcode table: no CMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST
        // exists in this build. The client's StoreFrame_IsLoading gate keeps the Shop on "Loading, please
        // wait" until C_StoreSecure.HasDistributionList() flips, which this response does.
        //
        // Header PROVEN: the captured body starts `00 00 00 00 00 20`, and `uint32 Result` followed by
        // WriteBits(1, 11) + FlushBits() reproduces those six bytes exactly. The client reads the count
        // back as `(b0 << 3) | (b1 >> 5)`, which is precisely TC's 11-bit MSB-first encoding. An empty
        // list is the 6-byte form, also observed live.
        //
        // RawData replays the captured blob instead, and is what we send while entitlements are off.
        class GetDistributionListResponse final : public ServerPacket
        {
        public:
            explicit GetDistributionListResponse() : ServerPacket(SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE) { }

            WorldPacket const* Write() override;

            std::vector<uint8> const* RawData = nullptr;

            bool BuildFromObjects = false;
            uint32 Result = 0;                              // PurchaseResult; 0 = Ok in every capture
            std::vector<DistributionObject> Distributions;
        };

        // Pushes a single changed entitlement: the body is exactly one DistributionObject with NO header
        // of any kind (the client's parser sub_7FF7290A8090 does nothing but read the object). Proven by
        // the 68275 capture, where this opcode appears 297 times with a 101-byte body byte-identical to
        // the object embedded in the 107-byte list response. Receiving it makes the client fire
        // PRODUCT_DISTRIBUTIONS_UPDATED, which refreshes the character-select token row and the Shop.
        class DistributionUpdate final : public ServerPacket
        {
        public:
            explicit DistributionUpdate() : ServerPacket(SMSG_BATTLE_PAY_DISTRIBUTION_UPDATE, 101) { }

            WorldPacket const* Write() override;

            DistributionObject Distribution;
        };

        // Client asks to apply one owned entitlement to a character it picked. Layout PROVEN by
        // disassembling the client's serializer (sub_7FF729079930): it writes the opcode and then
        // exactly uint32, uint64, PackedGuid, uint32. This packet appears in none of our captures, so the
        // field NAMES below are the plausible reading rather than a client-derived fact - which is why
        // the handler additionally validates both ids against values this server issued before acting.
        class DistributionAssignToTarget final : public ClientPacket
        {
        public:
            explicit DistributionAssignToTarget(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_DISTRIBUTION_ASSIGN_TO_TARGET, std::move(packet)) { }

            void Read() override;

            uint32 ClientToken = 0;
            uint64 DistributionID = 0;
            ObjectGuid TargetCharacter;
            uint32 ProductChoice = 0;       // a ChrSpecialization id for boosts, per the external fork
        };

        // Server's answer to an assign. Structure PROVEN by disassembling the client's parser
        // (sub_7FF7290A8F70): uint32, uint32, uint64. Never observed on the wire, so the field meanings
        // are inferred - the first uint32 is taken to be the PurchaseResult the client surfaces through
        // PRODUCT_ASSIGN_TO_TARGET_FAILED, and the middle uint32 is sent as 0 because we do not know it.
        class StartDistributionAssignToTargetResponse final : public ServerPacket
        {
        public:
            explicit StartDistributionAssignToTargetResponse() : ServerPacket(SMSG_BATTLE_PAY_START_DISTRIBUTION_ASSIGN_TO_TARGET_RESPONSE, 16) { }

            WorldPacket const* Write() override;

            uint32 Result = 0;
            uint32 Unknown = 0;
            uint64 DistributionID = 0;
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
            explicit ConfirmPurchase() : ServerPacket(SMSG_BATTLE_PAY_CONFIRM_PURCHASE, 8 + 4) { }

            WorldPacket const* Write() override;

            // EXACTLY 12 bytes - recovered from the client's handler (0x23D06D0 reads a u64 at +0 and a
            // u32 at +8, stores both to globals and fires STORE_CONFIRM_PURCHASE; the message class does
            // not field-parse, it takes a raw pointer to the payload). Nothing past +12 is ever read, so
            // the price and the bit-packed string we used to append were dead weight. Full RE:
            // c:\dumps\BATTLEPAY_CONFIRM_PURCHASE_WIRE_68275.md
            uint64 PurchaseID = 0;
            // The client echoes WHATEVER sits at +8 back to us as the token. We previously wrote ProductID
            // here, so the echo never matched the token we were comparing against.
            uint32 ServerToken = 0;
        };

        // Client's answer to the confirmation prompt. Layout byte-grounded from the 68275 client read of
        // CMSG_BATTLE_PAY_CONFIRM_PURCHASE_RESPONSE (0x4000fb): a u32 then a 1-bit bool. The u32 echoes
        // our ServerToken; the bool is confirm(true)/cancel(false).
        class ConfirmPurchaseResponse final : public ClientPacket
        {
        public:
            explicit ConfirmPurchaseResponse(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_CONFIRM_PURCHASE_RESPONSE, std::move(packet)) { }

            void Read() override;

            // 13 bytes, from the client's serializer (0x5D9FF0, class id via vtable 0x5DA060):
            //   u32 ServerToken @0  - echo of the SMSG's +8
            //   u64 price      @4  - dollars*10000 + cents*100, ZERO on cancel
            //   bits{1}        @12 - Confirmed, MSB-first then flush
            // The u64 used to be missing here, so ReadBit() read a bit of the price and Confirmed was
            // always false - i.e. even a working prompt would have been treated as a cancel.
            uint32 ServerToken = 0;
            uint64 ClientPriceFixedPoint = 0;
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

        // Server drives purchase progress/completion.
        //
        // Body = { uint32 Count, Count x JamBattlePayPurchase }. There is NO leading Result: the client
        // read ctor at RVA 0x6090D0 performs exactly ONE ReadUInt32 and passes it straight to
        // vector_resize. An earlier comment here claimed "u32 result, then a u32-counted vector", which
        // was wrong and cost a working shop - see PurchaseUpdate::Write.
        //
        // status = 6 signals completion (live 68974 value; see PurchaseRecord) and the record echoes the
        // productID delivered; status = 9 (ConfirmationPending) with resultCode 0 is what the
        // confirmation dialog needs before the client will send CMSG_BATTLE_PAY_CONFIRM_PURCHASE_RESPONSE.
        class PurchaseUpdate final : public ServerPacket
        {
        public:
            explicit PurchaseUpdate() : ServerPacket(SMSG_BATTLE_PAY_PURCHASE_UPDATE) { }

            WorldPacket const* Write() override;

            std::vector<PurchaseRecord> Purchases;
        };

        // Reply to CMSG_BATTLE_PAY_GET_PURCHASE_LIST. Body = { uint32 Result, uint32 Count,
        // Count x PurchaseRecord }.
        //
        // NOTE the header is NOT the same as SMSG_BATTLE_PAY_PURCHASE_UPDATE, which has no Result and
        // starts straight at Count. This message's ctor (RVA 0x607DA0) does TWO ReadUInt32; the other's
        // does one. The client structs confirm it: the record vector lives at +0x28 here and at +0x20
        // there, displaced by exactly these 4 bytes. Assuming the two were identical is what broke the
        // purchase flow.
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

        // ---------------------------------------------------------------------------------------------
        // VAS (Value Added Services) - paid character services: transfer, rename, faction/race change.
        //
        // Only the two opcodes a real client actually sends are answered here. CMSG_UPDATE_VAS_PURCHASE_STATES
        // is emitted at character select in every one of the 19 captures on this machine, and
        // CMSG_VAS_GET_SERVICE_STATUS alongside it. Both have EMPTY bodies (client serializers RVA 0x5DC6D0
        // and 0x5DD670 write the opcode header and nothing else).
        //
        // The replies below are the truthful complete answers for a realm that has no VAS purchases in
        // flight, not placeholders: retail sends the very same single 0x00 byte for
        // SMSG_ENUM_VAS_PURCHASE_STATES_RESPONSE every session when a player has no pending purchase, and
        // the client's handler (RVA 0x23D0140) CLEARS and rebuilds its whole VASPurchaseState cache from
        // it. Not answering leaves that cache holding whatever it had.
        // ---------------------------------------------------------------------------------------------

        // CMSG_UPDATE_VAS_PURCHASE_STATES (0x400123) - empty body.
        class UpdateVasPurchaseStates final : public ClientPacket
        {
        public:
            explicit UpdateVasPurchaseStates(WorldPacket&& packet) : ClientPacket(CMSG_UPDATE_VAS_PURCHASE_STATES, std::move(packet)) { }

            void Read() override { }
        };

        // SMSG_ENUM_VAS_PURCHASE_STATES_RESPONSE (0x42029B):
        //   Bits<6> Count; FlushBits; Count x VASPurchaseState
        //
        // Count is SIX BITS, not a uint32 - confirmed by the client ctor at RVA 0x60EB40 and by 19 live
        // captures, every one of which is a single 0x00 byte. There is no ClientToken gate on this message
        // (unlike 0x420297 / 0x420298 / 0x4202C3, which silently drop unless the token is echoed).
        //
        // The per-entry VASPurchaseState struct is recovered but deliberately not modelled yet: its State
        // field indexes the LE_VAS_PURCHASE_STATE_* enum whose numeric values are NOT recoverable offline
        // (they are Lua globals, not an AddEnumConstant registrar). Emitting an entry would mean guessing a
        // state value that drives the client's purchase UI, so entries wait for the phase that can prove
        // them. An empty list needs none of that and is exactly what retail sends.
        class EnumVasPurchaseStatesResponse final : public ServerPacket
        {
        public:
            explicit EnumVasPurchaseStatesResponse() : ServerPacket(SMSG_ENUM_VAS_PURCHASE_STATES_RESPONSE, 1) { }

            WorldPacket const* Write() override;
        };

        // CMSG_VAS_GET_SERVICE_STATUS (0x400137) - empty body.
        class VasGetServiceStatus final : public ClientPacket
        {
        public:
            explicit VasGetServiceStatus(WorldPacket&& packet) : ClientPacket(CMSG_VAS_GET_SERVICE_STATUS, std::move(packet)) { }

            void Read() override { }
        };

        // SMSG_VAS_GET_SERVICE_STATUS_RESPONSE (0x4202C0) - exactly 1 byte:
        //   Bits<4> ServiceStatus (high nibble); Bits<4> Unknown (low nibble); FlushBits
        //
        // Client ctor RVA 0x6115E0, handler 0x23D0410 stores both nibbles and fires a Lua event. 0x00 is a
        // legal, complete body. We send 0 because this realm offers no VAS services - that is the accurate
        // status, not a stand-in. The meaning of the low nibble is genuinely unknown, so it stays 0 rather
        // than being given an invented value.
        class VasGetServiceStatusResponse final : public ServerPacket
        {
        public:
            explicit VasGetServiceStatusResponse() : ServerPacket(SMSG_VAS_GET_SERVICE_STATUS_RESPONSE, 1) { }

            WorldPacket const* Write() override;

            uint8 ServiceStatus = 0;
            uint8 Unknown = 0;
        };
    }
}

#endif // TRINITYCORE_BATTLE_PAY_PACKETS_H
