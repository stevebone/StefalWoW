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

namespace Scripts::Custom::Mardum
{
    namespace Creatures
    {
        static constexpr uint32 KaynSunfury = 98229;
        static constexpr uint32 SevisBrightflameAshtongue = 99916;
        static constexpr uint32 FelSpreader = 97142;
        static constexpr uint32 FirstFelSpreader = 97154;
        static constexpr uint32 LegionCommunicator1 = 102223;
        static constexpr uint32 ColossalInfernal = 96159;
        static constexpr uint32 ElmGeneralPurposeBunny = 23837;
    }

    namespace Maps
    {
        static constexpr uint32 Mardum = 1481;
    }

    namespace Areas
    {
        static constexpr uint32 DespairRidge = 7741;
        static constexpr uint32 MoltenShore  = 7740;
    }

    namespace GameObjects
    {
        static constexpr uint32 LegionCommunicator1 = 244439;
    }

    namespace Scenes
    {
        static constexpr uint32 TheInvasionBeginsBannerPlanted = 1116;
    }

    namespace Quests
    {
        static constexpr uint32 EnterTheIllidariAshtongue = 40378;
        static constexpr uint32 AssaultOnMardum          = 39279;
    }

    namespace Objectives
    {
        static constexpr uint32 FelSpreaderDestroyed      = 279930; // quest 39279
        static constexpr uint32 LegionCommunicator1    = 281333; // quest 39279
    }

    namespace Spells
    {
        static constexpr uint32 DestroyingFelSpreader  = 191827;
        static constexpr uint32 FelSpreaderExplosion   = 199617;
        static constexpr uint32 InfernalSummon         = 199216;
        static constexpr uint32 ColossalInfernalMeteor = 242915;
    }

    namespace Actions
    {
        static constexpr int32 ColossalInfernalMeteor = 1;
    }

    namespace StringIds
    {
        // Marks the Baleful-summoned Colossal Infernal taking part in the
        // Molten Shore meteor reveal (temp summon -> spawnId 0).
        static constexpr char const ColossalInfernal[] = "mardum_colossal_infernal";
    }

    namespace Conversations
    {
        static constexpr uint32 FelSpreaderDestroyed      = 581;
        static constexpr uint32 LegionCommunicatorReport  = 558;
    }

    namespace CreatureText
    {
        // Kayn Sunfury (98229) - creature_text GroupIDs (Mardum_CreatureText.sql)
        static constexpr uint8 KaynFindKeystone    = 2; // 'Cyana, Jace, Allari... find the keystone.'
        static constexpr uint8 KaynActivateGateway = 3; // 'Now, let's see about activating that gateway.'

        // Sevis Brightflame (99916) - creature_text GroupID
        static constexpr uint8 SevisAshtongueGreeting = 0;
    }

    namespace Positions
    {
        // Sky anchor above Molten Shore where the meteor's caster bunny hovers.
        static constexpr Position InfernalMeteorAttackerBunny = { 425.915f, 2550.287f, 200.0f };
    }

    namespace Misc
    {
        static constexpr float SearchRange        = 50.0f;
        static constexpr float TalkDistance       = 5.0f;
        static constexpr float MeteorTriggerRange = 350.0f; // Molten Shore player poll on the coloss AI
    }
}
