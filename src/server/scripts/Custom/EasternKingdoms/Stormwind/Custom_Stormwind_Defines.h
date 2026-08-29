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

namespace Scripts::EasternKingdoms::StormwindCity
{
    namespace Creatures
    {
        static constexpr uint32 AysaAtStormwindGateKeep = 60566;
        static constexpr uint32 GeneralMarcusJonathan = 466;
        static constexpr uint32 AysaSpawnedAtGate = 61792;
        static constexpr uint32 JojoSpawnedAtGate = 61793;
        static constexpr uint32 AlynBlack = 61834;
        static constexpr uint32 MoniWiddlesprock = 61836;
        static constexpr uint32 LeriaNightwind = 61837;
        static constexpr uint32 GavinMarlsbury = 61838;
        static constexpr uint32 LucasSevering = 61839;
        static constexpr uint32 Naanae = 61840;
        static constexpr uint32 BrunnGoldenmug = 61841;
        static constexpr uint32 Marty = 61895;
        static constexpr uint32 Josie = 61896;
        static constexpr uint32 KingVarianPhased = 29611;
        static constexpr uint32 KingVarianSpawned = 61796;

        static constexpr uint32 CreditWalkWithKingVarian = 61798;
        static constexpr uint32 CreditFightWithKingVarian = 61824;
    }

    namespace Spawns
    {
        static constexpr uint32 StormwindCityGuardOnBridge = 313908;
    }

    namespace Quests
    {
        static constexpr uint32 JoiningTheAlliance = 30987;
        static constexpr uint32 TheAllianceWay = 30988;
        static constexpr uint32 AnOldPitFighter = 30989;
    }

    namespace Spells
    {
        static constexpr uint32 SummonAysaAtStormwindGate = 120344;
        static constexpr uint32 SummonJojoAtStormwindGate = 120345;
        static constexpr uint32 Stealth = 86603;
        static constexpr uint32 VariansPhaseInvisAura = 120418;
        static constexpr uint32 PandarenQuackingPalm = 107079;
    }

    namespace Positions
    {
        static constexpr Position AysaStormwindCityGeneralMarcus = { -8957.971f, 517.9630f, 96.3556f, 0.7743f };
        static constexpr Position AysaStormwindCityCenter = { -8826.584f, 627.582f, 94.3396f, 0.7692f };
        static constexpr Position AysaStormwindKeepVarianSpar = { -8326.8f, 301.036f, 156.833f, 5.23791f };
        static constexpr Position JojoStormwindKeepVarianSpar = { -8313.51f, 280.193f, 156.833f, 2.30782f };
    }

    namespace Paths
    {
        static constexpr uint32 StormwindKeepKingVarian = 6179600;
    }

    namespace Events
    {
        static constexpr int8 KingVarianSpawnedStartPath = 1;
        static constexpr int8 KingVarianSpawnedStartDialogue = 2;
        static constexpr int8 KingVarianDialogueStep = 3;
        static constexpr int8 KingVarianCombatStart = 4;
        static constexpr int8 KingVarianCombatEnd = 5;
        static constexpr int8 KingVarianSpawnedFinalPath = 6;
    }

    namespace Phases
    {
        static constexpr uint32 StormwindGateAysa = 1135;
        static constexpr uint32 StormwindGateJojo = 1136;
        static constexpr uint32 StormwindKeepKingVarian = 1139;
        static constexpr uint32 StormwindKeepKingVarianSpar = 1147;
    }

    namespace Misc
    {
        static constexpr int8 KingVarianSparScene = 2;
    }

    namespace AreaTriggers
    {
        static constexpr uint32 StormwindTradeDistrictMoniAlyn = 7990;
    }

    struct DialogueEntry
    {
        uint32 NpcEntry;
        uint8 TalkId;
        Milliseconds NextDelay;
        bool IsFinal = false;
    };

    namespace Dialogue
    {
        static constexpr DialogueEntry VarianDialogue[] =
        {
            // The time delay in seconds represents how long the line should take
            // Until the next one can fire
            { Creatures::KingVarianSpawned, 0, 4s }, // Walk with me. All three of you.
            { Creatures::KingVarianSpawned, 1, 5s }, // I understand you want to join the Alliance.
            { Creatures::KingVarianSpawned, 2, 8s }, // We always have need of allies. I'd be a fool to turn you away, especially at a time of war.
            { Creatures::KingVarianSpawned, 3, 6s }, // But there are a number of things I need to make clear to you.
            { Creatures::AysaSpawnedAtGate, 5, 3s }, // Yes. Go on.
            { Creatures::KingVarianSpawned, 4, 5s }, // First: the races of the Alliance look out for one another.
            { Creatures::KingVarianSpawned, 5, 10s }, // You are expected to provide aid to Alliance members in need, whether they be human, gnome, draenei... or pandaren like yourselves.
            { Creatures::KingVarianSpawned, 6, 7s }, // It is a simple guideline, but a meaningful one. Do you understand?
            { Creatures::AysaSpawnedAtGate, 6, 3s }, // Certainly
            { Creatures::KingVarianSpawned, 7, 5s }, // Good. Because that brings me to my second point.
            { Creatures::KingVarianSpawned, 8, 7s }, // Not all pandaren have chosen the same path as you three.
            { Creatures::KingVarianSpawned, 9, 5s }, // Some have sided with the Horde.
            { Creatures::KingVarianSpawned, 10, 7s }, // They, along with the other barbarian clans of the Horde, are your new enemies.
            { Creatures::KingVarianSpawned, 11, 10s }, // Those who you once considered friends, or even those you might have loved, are now your sworn adversaries.
            { Creatures::KingVarianSpawned, 12, 14s }, // I am deeply sorry, but the battle lines have been drawn. I will not tolerate any fraternizing with the enemy, as you could expose our Alliance to danger.
            { Creatures::KingVarianSpawned, 13, 4s }, // Do I make myself clear?
            { Creatures::AysaSpawnedAtGate, 7, 3s, true } // Yes... Of course.
        };
    }
}
