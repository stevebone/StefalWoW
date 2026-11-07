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

#include "Define.h"
#include "Position.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    namespace Creatures
    {
        // "Breaking Out" (38672) - Vault of Silence
        static constexpr uint32 MaievShadowsong  = 92718;
        static constexpr uint32 WardOfTheHunters = 100168;
    }

    namespace Maps
    {
        static constexpr uint32 WardenPrisonDH = 1468;
    }

    namespace Areas
    {
        static constexpr uint32 VaultOfSilence = 7873;
    }

    namespace Quests
    {
        static constexpr uint32 BreakingOut = 38672;
    }

    namespace Texts
    {
        // Maiev Shadowsong (92718) - creature_text GroupID 0
        static constexpr uint8 SayMaievFreed = 0;
    }

    namespace AnimKits
    {
        // "Breaking Out" (38672) - ward dissolve anim played by the personal ward clone
        static constexpr uint16 WardOpen = 8930;
    }

    namespace Paths
    {
        // "Breaking Out" (38672) - freed Maiev walks out of her cell (sniffed @MOVID+4)
        static constexpr uint32 MaievVaultOfSilenceFreed = 9271800;
    }
}
