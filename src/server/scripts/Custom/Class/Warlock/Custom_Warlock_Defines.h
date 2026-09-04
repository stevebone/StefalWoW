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
    }

    namespace Creatures
    {
        static constexpr uint32 WarlockDreadstalker = 98035;
        static constexpr uint32 WarlockInfernal = 89;
        static constexpr uint32 WarlockDoomguard = 11859;
        static constexpr uint32 WarlockDemonicTyrant1 = 135002;
        static constexpr uint32 WarlockDemonicTyrant2 = 250289;
        static constexpr uint32 WildImp = 55659;
    }
}
