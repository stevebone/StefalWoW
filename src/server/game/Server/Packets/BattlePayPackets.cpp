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

#include "BattlePayPackets.h"
#include "StringFormat.h"
#include "Log.h"
#include "PacketOperators.h"

namespace WorldPackets::BattlePay
{
    WorldPacket const* ProductListResponse::Write()
    {
        if (RawData && !RawData->empty())
            _worldPacket.append(RawData->data(), RawData->size());

        return &_worldPacket;
    }

    namespace
    {
        // Writes a JamBattlePayDeliverable exactly as the client's parser reads it.
        //
        // The tail is a 16-bit MSB-first group, which is what produces the two trailing bytes the client
        // decomposes as B2/B3: alreadyOwns(1) + hasPetResult(1) + choicesCount(7) + hasDisplayInfo(1) +
        // petResult(6) = 16 bits. With everything zero this writes 00 00, matching the capture.
        void WriteDeliverable(ByteBuffer& buffer, DistributionDeliverable const& deliverable)
        {
            buffer << deliverable.DeliverableID;
            buffer << deliverable.Type;
            buffer << deliverable.ItemID;
            buffer << deliverable.Quantity;
            buffer << deliverable.MountSpellID;
            buffer << deliverable.BattlePetCreatureID;
            buffer << deliverable.BoostID;
            buffer << deliverable.Flags;
            buffer << deliverable.TransItemModifiedAppearanceID;
            buffer << deliverable.TransmogSetID;
            buffer << deliverable.CharTitleID;
            buffer << deliverable.SpellItemEnchantmentID;
            buffer << deliverable.WarbandSceneID;

            buffer << uint8(deliverable.Name.size());       // plain byte, read before the bit group
            buffer.WriteBit(deliverable.AlreadyOwns);
            buffer.WriteBit(false);                         // hasPetResult - we never grant a battle pet this way
            buffer.WriteBits(0u, 7);                        // choicesCount - no choice products
            buffer.WriteBit(false);                         // hasDisplayInfo - the struct is not decoded (see .h)
            buffer.WriteBits(0u, 6);                        // petResult
            buffer.FlushBits();

            if (!deliverable.Name.empty())
                buffer.append(deliverable.Name.data(), deliverable.Name.size());
        }

        // Writes a JamBattlePayDistributionObject. ObjectGuid streams as a PackedGuid (uint16 mask + the
        // non-zero bytes), which is exactly what the client's ReadPackedGuid consumes here.
        void WriteDistributionObject(ByteBuffer& buffer, DistributionObject const& distribution)
        {
            buffer << distribution.DistributionID;
            buffer << distribution.Status;
            buffer << distribution.DeliverableID;
            buffer << distribution.LicenseGameAccountGUID;
            buffer << distribution.TargetPlayer;
            buffer << distribution.TargetNativeRealm;
            buffer << distribution.TargetVirtualRealm;
            buffer << distribution.PurchaseID;
            buffer << distribution.ManualReview;             // precedes the flag byte on the wire

            buffer.WriteBit(distribution.Deliverable.has_value());
            buffer.WriteBit(distribution.Revoked);
            buffer.FlushBits();

            if (distribution.Deliverable)
                WriteDeliverable(buffer, *distribution.Deliverable);
        }
    }

    WorldPacket const* GetDistributionListResponse::Write()
    {
        if (!BuildFromObjects)
        {
            if (RawData && !RawData->empty())
                _worldPacket.append(RawData->data(), RawData->size());

            return &_worldPacket;
        }

        // Header proven byte-exact against the capture - see the class comment.
        _worldPacket << Result;
        _worldPacket.WriteBits(uint32(Distributions.size()), 11);
        _worldPacket.FlushBits();

        for (DistributionObject const& distribution : Distributions)
            WriteDistributionObject(_worldPacket, distribution);

        return &_worldPacket;
    }

    WorldPacket const* DistributionUpdate::Write()
    {
        WriteDistributionObject(_worldPacket, Distribution);

        return &_worldPacket;
    }

    void DistributionAssignToTarget::Read()
    {
        _worldPacket >> ClientToken;
        _worldPacket >> DistributionID;
        _worldPacket >> TargetCharacter;
        _worldPacket >> ProductChoice;
    }

    WorldPacket const* StartDistributionAssignToTargetResponse::Write()
    {
        _worldPacket << Result;
        _worldPacket << Unknown;
        _worldPacket << DistributionID;

        return &_worldPacket;
    }

    void StartPurchase::Read()
    {
        _worldPacket >> ClientToken;
        _worldPacket >> ProductID;
        _worldPacket >> Unused;
        Flag = _worldPacket.ReadBit();

        // Remainder is the platform string and the client's attestation blob; nothing here needs them,
        // and consuming the buffer keeps the "Unprocessed tail data" warning from firing every purchase.
        _worldPacket.rfinish();
    }



    void OpenCheckout::Read()
    {
        _worldPacket >> ClientToken;
    }

    WorldPacket const* StartPurchaseResponse::Write()
    {
        _worldPacket << ResultA;
        _worldPacket << ResultB;
        _worldPacket << PurchaseID;

        return &_worldPacket;
    }

    // INFERRED layout - see the ConfirmPurchase comment in the header. Gated off by default.
    WorldPacket const* ConfirmPurchase::Write()
    {
        _worldPacket << PurchaseID;     // +0
        _worldPacket << ServerToken;    // +8 - echoed back verbatim by the client

        return &_worldPacket;
    }



    void ConfirmPurchaseResponse::Read()
    {
        _worldPacket >> ServerToken;
        _worldPacket >> ClientPriceFixedPoint;
        Confirmed = _worldPacket.ReadBit();
    }



    // Record order proven against the live 68974 purchase list (TESTER_SNIFF2_LINDORMI_MINE, 458 B =
    // 8 + 10x45): { u64 PurchaseID, i32 Status, i32 ResultCode, u32 ProductID, u64 BasePrice,
    // u64 UserPrice, i64 TimeCreated, u8 walletNameLen }. walletName sits at the END of the record -
    // in all 10 live records the unix purchase time aligns at record offset 36 and byte 44 is the
    // empty-wallet 0; with the u8 after ProductID the time would start at 37, one byte late.
    // Shares the JamBattlePayPurchase record layout with PurchaseUpdate::Write (walletName length
    // record-final - see the comment there). Answered honestly-empty today (no purchase ledger yet).
    WorldPacket const* GetPurchaseListResponse::Write()
    {
        _worldPacket << Result;
        _worldPacket << uint32(Purchases.size());
        for (PurchaseRecord const& p : Purchases)
        {
            _worldPacket << p.PurchaseID;
            _worldPacket << p.Status;
            _worldPacket << p.ResultCode;
            _worldPacket << p.ProductID;
            _worldPacket << p.BasePrice;
            _worldPacket << p.UserPrice;
            _worldPacket << p.TimeCreated;
            _worldPacket << uint8(0);       // walletName: empty (8-bit length primitive, value 0), record-final
        }

        return &_worldPacket;
    }

    WorldPacket const* PurchaseUpdate::Write()
    {
        // NO leading Result here. SMSG_BATTLE_PAY_PURCHASE_UPDATE (0x420231) begins straight with the record
        // count: its ctor (client RVA 0x6090D0) performs exactly ONE ReadUInt32 and feeds it directly to
        // vector_resize, then parses that many records.
        //
        // Its sibling SMSG_BATTLE_PAY_GET_PURCHASE_LIST_RESPONSE (0x42021B, ctor 0x607DA0) DOES lead with a
        // Result and performs TWO ReadUInt32. The two messages share the record type but not the header, and
        // the client structs prove it: the record vector sits at +0x20 in this message and at +0x28 in that
        // one - displaced by exactly the 4 bytes of Result.
        //
        // Writing Result here made the client read our always-zero Result AS THE COUNT, so it parsed zero
        // records and returned immediately (merge handler 0x23CD340, cmp/je on count == 0) with no error
        // anywhere. That silently broke the entire purchase confirmation handshake - see the commit message.
        _worldPacket << uint32(Purchases.size());
        for (PurchaseRecord const& p : Purchases)
        {
            _worldPacket << p.PurchaseID;
            _worldPacket << p.Status;
            _worldPacket << p.ResultCode;
            _worldPacket << p.ProductID;
            _worldPacket << p.BasePrice;
            _worldPacket << p.UserPrice;
            _worldPacket << p.TimeCreated;
            _worldPacket << uint8(0);       // walletName: empty (8-bit length primitive, value 0), record-final
        }

        return &_worldPacket;
    }

    WorldPacket const* EnumVasPurchaseStatesResponse::Write()
    {
        // Six-bit count, then flush. With no purchases this is the single 0x00 byte retail sends.
        _worldPacket << Bits<6>(0);
        _worldPacket.FlushBits();

        return &_worldPacket;
    }

    WorldPacket const* VasGetServiceStatusResponse::Write()
    {
        _worldPacket << Bits<4>(ServiceStatus);
        _worldPacket << Bits<4>(Unknown);
        _worldPacket.FlushBits();

        return &_worldPacket;
    }
}
