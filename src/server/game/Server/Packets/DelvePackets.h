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

#ifndef TRINITYCORE_DELVE_PACKETS_H
#define TRINITYCORE_DELVE_PACKETS_H

#include "Packet.h"
#include "ObjectGuid.h"

namespace WorldPackets::Delve
{
    class TC_GAME_API ShowDelvesDisplayUI final : public ServerPacket
    {
    public:
        ShowDelvesDisplayUI() : ServerPacket(SMSG_SHOW_DELVES_DISPLAY_UI) { }

        WorldPacket const* Write() override;

        int32 ScenarioID = 0;
    };

    class TC_GAME_API PartyEligibilityForDelveTiersResponse final : public ServerPacket
    {
    public:
        PartyEligibilityForDelveTiersResponse() : ServerPacket(SMSG_PARTY_ELIGIBILITY_FOR_DELVE_TIERS_RESPONSE) { }

        WorldPacket const* Write() override;

        struct TierInfo
        {
            int32 Tier = 0;
            bool Eligible = false;
        };

        std::vector<TierInfo> Tiers;
        bool IsPartyEligible = true;
    };

    class TC_GAME_API ShowDelvesCompanionConfigurationUI final : public ServerPacket
    {
    public:
        ShowDelvesCompanionConfigurationUI() : ServerPacket(SMSG_SHOW_DELVES_COMPANION_CONFIGURATION_UI) { }

        WorldPacket const* Write() override;

        int32 CompanionConfigValue = 0; //check this
    };

    class TC_GAME_API DelvesAccountDataElementChanged final : public ServerPacket
    {
    public:
        DelvesAccountDataElementChanged() : ServerPacket(SMSG_DELVES_ACCOUNT_DATA_ELEMENT_CHANGED) { }

        WorldPacket const* Write() override;

        int32 ElementIndex = 0;
        int32 NewValue = 0;
    };

    class RequestPartyEligibilityForDelveTiers final : public ClientPacket
    {
    public:
        RequestPartyEligibilityForDelveTiers(WorldPacket&& packet)
            : ClientPacket(CMSG_REQUEST_PARTY_ELIGIBILITY_FOR_DELVE_TIERS, std::move(packet)) { }

        void Read() override;

        int32 GossipOptionID = 0; // auto??
    };

    class DelveTeleportOut final : public ClientPacket
    {
    public:
        DelveTeleportOut(WorldPacket&& packet)
            : ClientPacket(CMSG_DELVE_TELEPORT_OUT, std::move(packet)) { }

        void Read() override;
    };
}

#endif // TRINITYCORE_DELVE_PACKETS_H
