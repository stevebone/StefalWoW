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
        static constexpr uint32 MaievShadowsong    = 92718;
        static constexpr uint32 WardOfTheHunters   = 100168;
        static constexpr uint32 AltruisCell        = 103655;
        static constexpr uint32 KaynCell           = 103658;
        static constexpr uint32 AltruisTheSufferer = 99632;
        static constexpr uint32 KaynSunfury        = 99631;
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
        // creature_text GroupID 0
        static constexpr uint8 SayMaievFreed   = 0; // 92718
        static constexpr uint8 SayAltruisFreed = 0; // 99632
        static constexpr uint8 SayKaynFreed    = 0; // 99631
        // creature_text GroupID 1 - post-completion exchange at the rally point
        static constexpr uint8 SayKaynRally    = 1; // 99631
        static constexpr uint8 SayAltruisRally = 1; // 99632
    }

    namespace AnimKits
    {
        // "Breaking Out" (38672) - ward dissolve anim played by the personal ward clone
        static constexpr uint16 WardOpen = 8930;
        // prison cell cage-open anim played by the personal cell clone
        static constexpr uint16 CellOpen = 22328;
    }

    namespace Spells
    {
        // "Glaive Anim Replacement" - self-cast by the freed prisoner after
        // picking up their warglaives (sniffed SPELL_START on the private clone)
        static constexpr uint32 GlaiveAnimReplacement = 222589;
    }

    namespace KillCredits
    {
        // "Breaking Out" (38672) objectives - sniffed SMSG_QUEST_UPDATE_ADD_CREDIT ObjectIDs
        static constexpr uint32 AltruisFreed   = 92849;  // objective 278274 - granted when the cell-open cast ends
        static constexpr uint32 KaynFreed      = 92848;  // objective 278275 - granted when the cell-open cast ends
        static constexpr uint32 AltruisWeapons = 112277; // objective 286137 - Altruis's Warglaives (kneel)
        static constexpr uint32 KaynWeapons    = 112276; // objective 286136 - Kayn's Warglaives (kneel)
        static constexpr uint32 AltruisEnd     = 112287; // objective 286143 - Altruis reaches the rally point
        static constexpr uint32 KaynEnd        = 99326;  // objective 280526 - Kayn reaches the rally point
    }

    namespace Phases
    {
        // added when the freed prisoner reaches the rally point (sniffed phase swap)
        static constexpr uint32 AltruisFreed = 5124;
        static constexpr uint32 KaynFreed    = 5125;
    }

    namespace Paths
    {
        // "Breaking Out" (38672) - freed Maiev walks out of her cell (sniffed @MOVID+4)
        static constexpr uint32 MaievVaultOfSilenceFreed = 9271800;
        // freed prisoners: cell -> warglaives, then warglaives -> rally point
        static constexpr uint32 AltruisToWeapons = 9963200;
        static constexpr uint32 AltruisToRally   = 9963201;
        static constexpr uint32 KaynToWeapons    = 9963100;
        static constexpr uint32 KaynToRally      = 9963101;
    }

    namespace Waypoints
    {
        // last node id of each leg (multi-node paths fire MovementInform per node)
        static constexpr uint32 AltruisToWeaponsLastNode = 3;
        static constexpr uint32 KaynToWeaponsLastNode    = 4;
        static constexpr uint32 ToRallyLastNode          = 2;
    }
}
