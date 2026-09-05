/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CUSTOM_PALADIN_DEFINES_H
#define CUSTOM_PALADIN_DEFINES_H

#include "Define.h"

namespace Scripts::Custom::Paladin
{
    namespace Spells
    {
        // Art of War
        static constexpr uint32 ArtOfWar          = 406064;
        static constexpr uint32 ArtOfWarTriggered = 231843;
        static constexpr uint32 BladeOfJustice    = 184575;

        // Righteous Cause
        static constexpr uint32 RighteousCause    = 402912;
        static constexpr uint32 BladeOfJusticeAoE = 404358;

        // Consecrated Blade
        static constexpr uint32 Consecration              = 26573;
        static constexpr uint32 ConsecratedBlade          = 404834;
        static constexpr uint32 ConsecratedBladeOverride  = 462970;
        static constexpr uint32 ConsecratedBladeICD       = 407475;
        static constexpr uint32 ConsecratedBladeArtOfWar  = 382275;
    }
}

#endif // CUSTOM_PALADIN_DEFINES_H
