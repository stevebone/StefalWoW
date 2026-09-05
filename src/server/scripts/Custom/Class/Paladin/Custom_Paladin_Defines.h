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

        // Divine Storm
        static constexpr uint32 DivineStorm               = 53385;
        static constexpr uint32 DivineStormDamage         = 224239;
        static constexpr uint32 VisualKitDivineStorm      = 73892;

        // Empyrean Power
        static constexpr uint32 EmpyreanPower             = 326732;
        static constexpr uint32 EmpyreanPowerBuff         = 326733;

        // Tempest of the Lightbringer
        static constexpr uint32 TempestOfTheLightbringer   = 383396;
        static constexpr uint32 TempestOfTheLightbringerAT = 383427;

        // Empyrean Legacy
        static constexpr uint32 EmpyreanLegacy            = 387170;
        static constexpr uint32 EmpyreanLegacyBuff        = 387178;
        static constexpr uint32 EmpyreanLegacyICD         = 387441;

        // Second Sunrise
        static constexpr uint32 SecondSunrise             = 431474;
        static constexpr uint32 LightOfDawn               = 85222;
        static constexpr uint32 HammerOfWrath             = 1241413;
        static constexpr uint32 HammerOfWrathLegacy       = 24275;

        // Crusader Strike family (for Empyrean Power proc)
        static constexpr uint32 CrusaderStrike            = 35395;
        static constexpr uint32 CrusadingStrikesTalent    = 404542;
        static constexpr uint32 CrusadingStrikesDamage    = 408385;
        static constexpr uint32 TemplarStrikesTalent      = 406646;
        static constexpr uint32 TemplarStrike             = 407480;
        static constexpr uint32 TemplarSlash              = 406647;

        // Holy Shock (for Second Sunrise CustomArg forwarding)
        static constexpr uint32 HolyShock                 = 20473;
        static constexpr uint32 HolyShockDamage           = 25912;
        static constexpr uint32 HolyShockHealing          = 25914;

        // Single-target HP spenders (for Empyrean Legacy)
        static constexpr uint32 TemplarsVerdict           = 85256;
        static constexpr uint32 FinalVerdictCast          = 383328;
        static constexpr uint32 JusticarsVengeance        = 215661;
    }
}

#endif // CUSTOM_PALADIN_DEFINES_H
