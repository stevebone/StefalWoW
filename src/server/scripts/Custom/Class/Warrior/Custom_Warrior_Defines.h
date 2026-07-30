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

#ifndef CUSTOM_WARRIOR_DEFINES_H
#define CUSTOM_WARRIOR_DEFINES_H

#include "Define.h"

namespace Scripts::Custom::Warrior
{
    namespace Spells
    {
        // Core warrior spells
        static constexpr uint32 BerserkerRage          = 18499;
        static constexpr uint32 Enrage                = 184362;
        static constexpr uint32 ShieldWall            = 871;
        static constexpr uint32 Rend                  = 772;
        static constexpr uint32 RendAura              = 388539;
        static constexpr uint32 Cleave                = 845;
        static constexpr uint32 ImprovedWhirlwind     = 12950;
        static constexpr uint32 WhirlwindCleaveAura   = 85739;
        static constexpr uint32 MeatCleaverTalent     = 280392;
        static constexpr uint32 Ravager              = 228920;
        static constexpr uint32 Execute              = 20647;

        // Sweeping Strikes
        static constexpr uint32 SweepingStrikes       = 260708;
        static constexpr uint32 CollateralDamage     = 334779;
        static constexpr uint32 CollateralDamageBuff = 334783;

        // Bladestorm
        static constexpr uint32 BladestormPeriodicWhirlwind = 50622;

        // Thirst for Battle
        static constexpr uint32 ThirstForBattle      = 199202;
        static constexpr uint32 ThirstForBattleBuff = 199203;

        // Slam / Fervor of Battle
        static constexpr uint32 SlamArms             = 1464;
        static constexpr uint32 FervorOfBattle       = 202316;

        // Wrecking Ball
        static constexpr uint32 WreckingBallEffect   = 215570;

        // Second Wind
        static constexpr uint32 SecondWind6pc        = 202147; // 202149;
        static constexpr uint32 SecondWind2pc        = 458245;

        // Indomitable
        static constexpr uint32 IndomitableHeal      = 316643;

        // Champion's Spear
        static constexpr uint32 ChampionsSpearAura         = 376080;
        static constexpr uint32 ChampionsSpearChainVisual = 376085;
        static constexpr uint32 ChampionsSpearPull         = 376084;

        // Ravager
        static constexpr uint32 RavagerRageGain       = 334934;
        static constexpr uint32 RavagerPeriodicDamage = 156287;
        static constexpr uint32 RavagerSummon         = 227876;

        // Honed Reflexes
        static constexpr uint32 HonedReflexes       = 391271;
        static constexpr uint32 HonedReflexesDebuff = 1271964;

        // Honed Reflexes affected spells
        static constexpr uint32 DieByTheSword       = 118038;
        static constexpr uint32 EnragedRegeneration = 184364;
        static constexpr uint32 Intervene           = 3411;
        static constexpr uint32 SpellReflection     = 23920;
        static constexpr uint32 StormBolt           = 107570;
        static constexpr uint32 Pummel              = 6552;

        // Crushing Combo
        static constexpr uint32 CrushingComboBuff = 1261189;

        // Dreadnaught
        static constexpr uint32 Dreadnaught       = 262150;
        static constexpr uint32 DreadnaughtDamage = 315961;

        // Fearless (Berserker Rage talent)
        static constexpr uint32 Fearless = 1271925;

        // Wrecking Throw
        static constexpr uint32 WreckingThrowDamage = 394354;

        // Defensive Stance
        static constexpr uint32 DefensiveStance          = 386208;
        static constexpr uint32 DefensiveStanceSpikeTalent = 1280961;

        // Shattering Throw
        static constexpr uint32 ShatteringThrowDispel = 64380;

        // Legendary - Unbreakable Will
        static constexpr uint32 UnbreakableWill     = 335629;
        static constexpr uint32 UnbreakableWillProc = 335635;

        // Misc Visuals
        static constexpr uint32 SpellVisualRavager = 36990;
    }
}

#endif // CUSTOM_WARRIOR_DEFINES_H
