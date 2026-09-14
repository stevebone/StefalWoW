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

#ifndef CUSTOM_DEATHKNIGHT_DEFINES_H
#define CUSTOM_DEATHKNIGHT_DEFINES_H

#include "Define.h"

namespace Scripts::Custom::DeathKnight
{
    namespace Spells
    {
        // Blood Draw
        static constexpr uint32 BloodDraw      = 374598;
        static constexpr uint32 BloodDrawDrain = 374606;
        static constexpr uint32 BloodDrawBuff  = 454871;
        static constexpr uint32 BloodDrawIcd   = 374609;

        // Vestigial Shell
        static constexpr uint32 AntiMagicShell       = 48707;
        static constexpr uint32 VestigialShellTalent = 454851;
        static constexpr uint32 VestigialShell        = 454863;

        // March of Darkness
        static constexpr uint32 MarchOfDarkness = 391546;
    }
}

#endif // CUSTOM_DEATHKNIGHT_DEFINES_H
