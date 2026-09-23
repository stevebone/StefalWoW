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

namespace Scripts::Custom::Mardum
{
    namespace Creatures
    {
        static constexpr uint32 KaynSunfury = 98229;
        static constexpr uint32 SevisBrightflameAshtongue = 99916;
        static constexpr uint32 FelSpreader = 97142;
        static constexpr uint32 FirstFelSpreader = 97154;
        static constexpr uint32 LegionCommunicator1 = 102223;
        static constexpr uint32 LegionCommunicator2 = 102224;
        static constexpr uint32 CyanaNightglaive    = 94377;
        static constexpr uint32 BelathDawnblade     = 94400;
        static constexpr uint32 SevisBrightflameCoilskar = 99917;
        static constexpr uint32 MannethrelDarkstar  = 93230;
        static constexpr uint32 IzalWhitemoon       = 93117;
        static constexpr uint32 AshtongueMystic     = 99914;
        static constexpr uint32 ELMGeneralPurposeBunny = 24021;
        static constexpr uint32 JaceDarkweaver      = 93759;
        static constexpr uint32 CoilskarSeaCaller   = 96884;
        static constexpr uint32 DoomCommanderBeliash = 93221;
        static constexpr uint32 QueenTyranna        = 95048; // RP version beside Beliash; 93802 is the boss at the Seat of Command
        static constexpr uint32 BeliashKillCredit   = 106003;
        static constexpr uint32 LegionDevastator    = 100161;
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
        static constexpr uint32 LegionCommunicator2 = 244440;
    }

    namespace Scenes
    {
        static constexpr uint32 TheInvasionBeginsBannerPlanted = 1116;
    }

    namespace Quests
    {
        static constexpr uint32 EnterTheIllidariAshtongue = 40378;
        static constexpr uint32 EnterTheIllidariCoilskar  = 40379;
        static constexpr uint32 EnterTheIllidariShivarra  = 38765;
        static constexpr uint32 AssaultOnMardum          = 39279;
        static constexpr uint32 SetThemFree              = 38759;
        static constexpr uint32 EyeOnThePrize            = 39049;
        static constexpr uint32 MeetingWithTheQueen      = 39050;
        static constexpr uint32 BeforeWereOverun         = 38766;
        static constexpr uint32 SevisSacrificeTracker    = 40087;
    }

    namespace Objectives
    {
        static constexpr uint32 FelSpreaderDestroyed      = 279930; // quest 39279
        static constexpr uint32 LegionCommunicator1       = 281333; // quest 39279
        static constexpr uint32 LegionCommunicator2       = 281334; // quest 39279
        static constexpr uint32 SoulSacrificed            = 280770; // quest 40379
        static constexpr uint32 CoilskarForces            = 280771; // quest 40379
    }

    namespace Spells
    {
        static constexpr uint32 DestroyingFelSpreader  = 191827;
        static constexpr uint32 FelSpreaderExplosion   = 199617;
        static constexpr uint32 AshtongueMysticSacrifice = 196724;
        static constexpr uint32 PermanentFeignDeath    = 159474;
        static constexpr uint32 ShivarraSoulMissiles02 = 191664;
        static constexpr uint32 FelChannelling         = 188485;
        static constexpr uint32 SummonCoilskarSeaCaller  = 191668;
        static constexpr uint32 SeaCallerLightningBolt   = 197745;
        static constexpr uint32 SeaCallerHealingWave     = 197744;
        static constexpr uint32 ShadowBlaze              = 195401;
        static constexpr uint32 ShadowBoltVolley         = 196403;
        static constexpr uint32 ShadowRetreat            = 196625;
        static constexpr uint32 BroodQueenTyrannaTeleport = 188658;
        static constexpr uint32 FelBombardment         = 194689;
    }

    namespace Conversations
    {
        static constexpr uint32 FelSpreaderDestroyed       = 581;
        static constexpr uint32 LegionCommunicatorReport1  = 558;
        static constexpr uint32 LegionCommunicatorReport2  = 583;
        static constexpr uint32 DoomCommanderBeliash       = 531;
        static constexpr uint32 FelBombardments            = 747;
    }

    namespace CreatureText
    {
        // Kayn Sunfury (98229) - creature_text GroupIDs (Mardum_CreatureText.sql)
        static constexpr uint8 KaynFindKeystone    = 2; // 'Cyana, Jace, Allari... find the keystone.'
        static constexpr uint8 KaynActivateGateway = 3; // 'Now, let's see about activating that gateway.'

        // Sevis Brightflame (99916) - creature_text GroupID
        static constexpr uint8 SevisAshtongueGreeting = 0;

        // Cyana Nightglaive (94377) - creature_text GroupID
        static constexpr uint8 CyanaCaptiveGreeting = 0;

        // Belath Dawnblade (94400) - creature_text GroupID
        static constexpr uint8 BelathCaptiveGreeting = 0;

        // Mannethrel Darkstar (93230) - creature_text GroupID
        static constexpr uint8 MannethrelCaptiveGreeting = 0;

        // Izal Whitemoon (93117) - creature_text GroupID
        static constexpr uint8 IzalCaptiveGreeting = 0;

        // Sevis Brightflame (99917) - creature_text GroupIDs
        static constexpr uint8 SevisCoilskarGreeting = 0;
        static constexpr uint8 SevisCoilskarFollowUp = 1;

        // Jace Darkweaver (93759) - creature_text GroupIDs (Mardum_CreatureText.sql)
        static constexpr uint8 JaceEyeOnThePrizeGreeting    = 0; // 'I sense greater power within you...'
        static constexpr uint8 JaceMeetingWithTheQueenAccept = 1; // 'Use the crucible to complete the ritual.'
        static constexpr uint8 JaceBeforeWereOverunAccept = 2; // 'Beliash is protected by...'
        static constexpr uint8 JaceBeforeWereOverunAccept2 = 3; // 'Good luck, $n. I'll see you up in the volcano.'

        // Coilskar Sea-Caller (96884) - creature_text GroupIDs
        static constexpr uint8 SeaCallerEngage   = 0; // 'Deal with these insects, Beliash.'
        static constexpr uint8 SeaCallerGreeting = 1; // 'Lady S'theno requested I join you.'

        // Doom Commander Beliash (93221) - creature_text GroupID
        static constexpr uint8 BeliashAggro = 0; // 'They will die.'
        static constexpr uint8 BeliashAggro2 = 1; // 'You wont survive'

        // Brood Queen Tyranna (95048) - creature_text GroupID
        static constexpr uint8 QueenTyrannaAggro = 0;
    }

    namespace Events
    {
        // Coilskar Sea-Caller (96884) - EventMap IDs
        static constexpr int8 SeaCallerGreeting      = 1;
        static constexpr int8 SeaCallerSpellRotation = 2;

        // Doom Commander Beliash (93221) - EventMap IDs
        static constexpr int8 BeliashShadowBlaze      = 1;
        static constexpr int8 BeliashShadowBoltVolley = 2;
        static constexpr int8 BeliashShadowRetreat    = 3;

        // Legion Devastator (100161) - EventMap IDs
        static constexpr int8 DevastatorBombardment             = 1;
        static constexpr int8 DevastatorBombardmentConversation = 2;
    }

    namespace Misc
    {
        static constexpr float SearchRange         = 50.0f;
        static constexpr float TalkDistance        = 5.0f;
        static constexpr float CaptiveGreetingRange = 10.0f;
        static constexpr float JaceGreetingRange    = 10.0f;
        static constexpr float BeliashConversationRange = 100.0f;
        static constexpr float BombardmentRange         = 450.0f;
        static constexpr uint32 MysticGossipMenu         = 19015;
        static constexpr uint32 MysticVisualKit          = 25111;
        static constexpr uint32 MysticAnimKit            = 9256;
        static constexpr uint32 SoulMissileTargetSpawnId = 6000619;
        static constexpr uint32 SoulMissileTargetEntry   = 24021;
    }
}
