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

namespace Scripts::Custom::Warlock
{
    namespace Spells
    {
        static constexpr uint32 AntoranArmaments = 1250921;

        static constexpr uint32 CallDreadstalkers = 104316; // Main Spell Script
        static constexpr uint32 RippedThroughThePortal = 387485;
        static constexpr uint32 CallDreadstalkersSummon = 364750;
        static constexpr uint32 DreadstalkersCharge = 194247;
        static constexpr uint32 SharpenedDreadfangs = 215111;

        static constexpr uint32 ImplosionDamage = 196278;
        static constexpr uint32 WildImpCounter = 296553;
        static constexpr uint32 FelFirebolt = 104318;

        static constexpr uint32 DemonicConsumption = 267215;
        static constexpr uint32 DemonicConsumptionBuff = 267972;

        static constexpr uint32 SummonDemonicTyrant = 265187; // Main Spell Script
        static constexpr uint32 TyrantsOblation = 1276767;
        static constexpr uint32 ReignOfTyranny = 1276748;
        static constexpr uint32 TyrantsDemonfire = 270481;
        static constexpr uint32 TyrantsBurningCleave = 1264093;

        static constexpr uint32 SummonOverfiend = 434587;
        static constexpr uint32 SoulFire = 6353;

        static constexpr uint32 FireAndBrimstone = 196408;

        // Soulburn
        static constexpr uint32 Soulburn = 385899;
        static constexpr uint32 SoulburnBuff = 387626;
        static constexpr uint32 SoulburnDrainLifeBuff = 387630;
        static constexpr uint32 SoulburnDrainLifeAbsorb = 394810;
        static constexpr uint32 SoulburnHealthstoneBuff = 387636;
        static constexpr uint32 DrainLife = 234153;
        static constexpr uint32 Healthstone = 6262;

        static constexpr uint32 SoulburnDrainLifeAbsorbMaxPct = 30;
        static constexpr uint32 SoulburnHealthstoneHealPct = 30;

        // Demonic Gateway
        static constexpr uint32 DemonicGateway = 111771;
        static constexpr uint32 DemonicGatewaySummonGreen = 113886;
        static constexpr uint32 DemonicGatewaySummonPurple = 113890;
        static constexpr uint32 DemonicGatewayJumpGreen = 113896;
        static constexpr uint32 DemonicGatewayJumpPurple = 120729;
        static constexpr uint32 DemonicGatewayVisual = 113900;
        static constexpr uint32 DemonicGatewayDebuff = 113942;

        // Demonic Circle
        static constexpr uint32 DemonicCircleTeleport = 48020;
        static constexpr uint32 DemonicCircleSummon = 48018;
        static constexpr uint32 SoulburnDemonicCircle = 387633;

        // Soul Leech
        static constexpr uint32 SoulLeech = 108370;
        static constexpr uint32 SoulLeechShield = 108366;
        static constexpr uint32 DemonSkin = 219272;

        // Soul Link
        static constexpr uint32 SoulLink = 108415;
        static constexpr uint32 SoulLinkPetBuff = 108446;
        static constexpr uint32 GrimoireOfSacrificeBuff = 196099;

        // Darkglare
        static constexpr uint32 SummonDarkglare = 205180;
        static constexpr uint32 EyeLaser = 205231;

        // Misc
        static constexpr uint32 ArenaPreparation = 32727;
    }

    namespace Creatures
    {
        static constexpr uint32 WarlockDreadstalker = 98035;
        static constexpr uint32 WarlockInfernal = 89;
        static constexpr uint32 WarlockDoomguard = 11859;
        static constexpr uint32 WarlockDemonicTyrant1 = 135002;
        static constexpr uint32 WarlockDemonicTyrant2 = 250289;
        static constexpr uint32 WildImp = 55659;
        static constexpr uint32 DemonicGatewayGreen = 59262;
        static constexpr uint32 DemonicGatewayPurple = 59271;
        static constexpr uint32 Darkglare = 103673;
    }
}
