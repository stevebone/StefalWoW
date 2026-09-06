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
        static constexpr uint32 CrusadingStrikesProc      = 406833;
        static constexpr uint32 CrusadingStrikesEnergize  = 406834;
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

        // Avenging Wrath
        static constexpr uint32 AvengingWrath = 31884;
        static constexpr uint32 AvengingWrath2 = 384376;
        static constexpr uint32 AvengingWrath3 = 454351;
        static constexpr uint32 AvengingWrath4 = 454373;
        static constexpr uint32 AvengingWrathRet = 231895;

        // Radiant Glory (Retribution talent)
        static constexpr uint32 RadiantGlory = 458359;
        static constexpr uint32 CrusadeVariant = 384392;

        // Dawnlight (Herald of the Sun hero talent)
        static constexpr uint32 Dawnlight               = 431377;
        static constexpr uint32 DawnlightEmpower        = 431460;
        static constexpr uint32 DawnlightCharges        = 431522;
        static constexpr uint32 DawnlightDamage         = 431380;
        static constexpr uint32 DawnlightHeal           = 431381;
        static constexpr uint32 DawnlightRadiationHeal  = 431382;
        static constexpr uint32 DawnlightRadiationDamage = 431399;
        static constexpr uint32 DawnlightRadiateMeta    = 431581;

        // Morning Star (Herald of the Sun hero talent)
        static constexpr uint32 MorningStar             = 431482;
        static constexpr uint32 MorningStarBuff         = 431539;

        // Will of the Dawn (Herald of the Sun hero talent)
        static constexpr uint32 WillOfTheDawn           = 431406;
        static constexpr uint32 WillOfTheDawnSpeed      = 431462;
        static constexpr uint32 WillOfTheDawnBurst      = 431752;
        static constexpr uint32 WillOfTheDawnIcd        = 456779;

        // Sun's Avatar (Herald of the Sun hero talent)
        static constexpr uint32 SunsAvatar              = 431425;
        static constexpr uint32 SunsAvatarLink          = 431907;
        static constexpr uint32 SunsAvatarSelfLink      = 463073;
        static constexpr uint32 SunsAvatarDamage        = 431911;
        static constexpr uint32 SunsAvatarHeal          = 431939;
        static constexpr uint32 SunsAvatarSelfDamage    = 463075;

        // Dawnlight trigger spells
        static constexpr uint32 HolyPrism               = 114165;
        static constexpr uint32 DivineToll              = 375576;
        static constexpr uint32 WakeOfAshes             = 255937;

        // Beacon of Light (for Dawnlight exclusion)
        static constexpr uint32 BeaconOfLight           = 53563;
        static constexpr uint32 BeaconOfLightHeal       = 53652;
        static constexpr uint32 LightsBeacon            = 53651;

        // Expurgation
        static constexpr uint32 Expurgation             = 383344;
        static constexpr uint32 ExpurgationDamage       = 383346;

        // Grand Crusader
        static constexpr uint32 GrandCrusader           = 85043;
        static constexpr uint32 GrandCrusaderBuff       = 85416;
        static constexpr uint32 AvengersShield          = 31935;
        static constexpr uint32 HammerOfTheRighteous    = 53595;
        static constexpr uint32 BlessedHammer           = 204301;

        // Hammer of Wrath Talent
        static constexpr uint32 HammerOfWrathTalent     = 1241288;
        static constexpr uint32 Judgment                = 20271;
        static constexpr uint32 JudgmentProt            = 275779;
        static constexpr uint32 JudgmentHoly            = 275773;

        // Holy Prism
        static constexpr uint32 HolyPrismBeamVisual     = 121551;
        static constexpr uint32 HolyPrismTargetAlly     = 114871;
        static constexpr uint32 HolyPrismTargetEnemy    = 114852;

        // Judge, Jury and Executioner
        static constexpr uint32 JudgeJuryExecutioner    = 406157;
        static constexpr uint32 JudgeJuryExecutionerBuff = 1253174;
        static constexpr uint32 ExecutionSentence       = 343527;
        static constexpr uint32 ExecutionSentenceRadiate = 1260251;

        // Righteous Protector
        static constexpr uint32 RighteousProtector          = 204074;
        static constexpr uint32 GuardianOfAncientKings      = 86659;

        // Walk Into Light (Herald of the Sun)
        static constexpr uint32 WalkIntoLight               = 1263782;
        static constexpr uint32 BlessingOfAnshe             = 445206;
        static constexpr uint32 InfusionOfLightTalent       = 53576;

        // Light of the Titans
        static constexpr uint32 LightOfTheTitans            = 378405;
        static constexpr uint32 LightOfTheTitansHot         = 378412;
        static constexpr uint32 WordOfGlory                 = 85673;

        // Holy Ritual
        static constexpr uint32 HolyRitualTalent            = 199422;
        static constexpr uint32 HolyRitualHeal              = 199423;

        // Judgment of Justice
        static constexpr uint32 JudgmentOfJusticeTalent     = 403495;
        static constexpr uint32 JudgmentOfJustice           = 408383;

        // Light's Conviction (Holy Shock mana refund)
        static constexpr uint32 LightsConviction            = 414073;
    }
}

#endif // CUSTOM_PALADIN_DEFINES_H
