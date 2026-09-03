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

#include "ContributionPackets.h"

namespace WorldPackets::Contribution
{
void ContributionContribute::Read()
{
    _worldPacket >> CollectorGUID;
    _worldPacket >> ContributionID;
}

void ContributionLastUpdateRequest::Read()
{
    _worldPacket >> ContributionID;
    _worldPacket >> ContributionGUID;
}

WorldPacket const* ContributionLastUpdateResponse::Write()
{
    // 8 + 4 + 4 = 16 bytes. The handler reads the timestamp as a qword; writing it as a dword shifts both keys
    // and makes the client read 4 bytes past the payload.
    _worldPacket << uint64(Data);
    _worldPacket << uint32(ContributionID);
    _worldPacket << uint32(ContributionGUID);

    return &_worldPacket;
}
}
