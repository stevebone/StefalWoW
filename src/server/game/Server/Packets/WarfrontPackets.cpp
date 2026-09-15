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

#include "WarfrontPackets.h"

namespace WorldPackets::Warfront
{
WorldPacket const* WarfrontComplete::Write()
{
    // Byte-recovered layout, exactly 8 bytes - see the class comment for the derivation. The client reads both
    // dwords unconditionally, so a short body makes it read past the payload.
    _worldPacket << int32(MapID);
    _worldPacket << int32(Winner);

    return &_worldPacket;
}
}
