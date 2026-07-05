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

#ifndef TRINITYCORE_COVENANT_PACKETS_H
#define TRINITYCORE_COVENANT_PACKETS_H

#include "Packet.h"

namespace WorldPackets
{
namespace Covenant
{
    // CMSG_ACTIVATE_SOULBIND (0x3A028F). Serializer sub_7FF72914B2B0 writes a single uint32 (the soulbind id).
    class ActivateSoulbind final : public ClientPacket
    {
    public:
        explicit ActivateSoulbind(WorldPacket&& packet) : ClientPacket(CMSG_ACTIVATE_SOULBIND, std::move(packet)) { }

        void Read() override;

        int32 SoulbindID = 0;
    };
}
}

#endif // TRINITYCORE_COVENANT_PACKETS_H
