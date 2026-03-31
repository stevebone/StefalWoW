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

#include "DelvePackets.h"
#include "PacketOperators.h"

namespace WorldPackets::Delve
{
    WorldPacket const* ShowDelvesDisplayUI::Write()
    {
        _worldPacket << int32(ScenarioID);

        return &_worldPacket;
    }

    WorldPacket const* PartyEligibilityForDelveTiersResponse::Write()
    {
        _worldPacket << Size<uint32>(Tiers);
        _worldPacket << Bits<1>(IsPartyEligible);
        _worldPacket.FlushBits();

        for (TierInfo const& tier : Tiers)
        {
            _worldPacket << int32(tier.Tier);
            _worldPacket << Bits<1>(tier.Eligible);
            _worldPacket.FlushBits();
        }

        return &_worldPacket;
    }

    WorldPacket const* ShowDelvesCompanionConfigurationUI::Write()
    {
        _worldPacket << int32(CompanionConfigValue);
        return &_worldPacket;
    }

    WorldPacket const* DelvesAccountDataElementChanged::Write()
    {
        _worldPacket << int32(ElementIndex);
        _worldPacket << int32(NewValue);

        return &_worldPacket;
    }

    void RequestPartyEligibilityForDelveTiers::Read()
    {
        _worldPacket >> GossipOptionID;
    }

    void DelveTeleportOut::Read()
    {
        // huh?
    }
}
