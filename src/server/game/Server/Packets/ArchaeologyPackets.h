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

#ifndef TRINITYCORE_ARCHAEOLOGY_PACKETS_H
#define TRINITYCORE_ARCHAEOLOGY_PACKETS_H

#include "Packet.h"

namespace WorldPackets
{
    namespace Archaeology
    {
        // Result of an Archaeology Survey cast: how many finds the current dig site has yielded and
        // needs, which research branch it feeds, and whether this cast revealed a find.
        class SurveyCast final : public ServerPacket
        {
        public:
            explicit SurveyCast() : ServerPacket(SMSG_ARCHAEOLOGY_SURVERY_CAST, 4 + 4 + 4 + 1) { }

            WorldPacket const* Write() override;

            uint32 TotalFinds = 0;
            uint32 NumFindsCompleted = 0;
            uint32 ResearchBranchID = 0;
            bool SuccessfulFind = false;
        };
    }
}

#endif // TRINITYCORE_ARCHAEOLOGY_PACKETS_H
