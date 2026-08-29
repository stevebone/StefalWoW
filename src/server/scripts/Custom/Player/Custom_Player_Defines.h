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

enum Custom_Player_Voucher_NPCs
{
    NPC_MERISSA_STILWELL = 11940,
    NPC_ORENTHIL_WHISPERWIND = 11942,
    NPC_YORI_CRACKHELM = 11941,
    NPC_JAELEIL = 16476,

    NPC_MAGGA = 11943,
    NPC_VORN_SKYSEER = 11944,
    NPC_CLAIRE_WILLOWER = 11945,
    NPC_MARSILLA_DAWNSTAR = 15493,
};

enum Custom_Player_Riding_NPCs
{
    NPC_RANDAL_HUNTER = 4732,
    NPC_JARTSAM = 4753,
    NPC_ULTHAM_IRONHORN = 4772,
    NPC_BINJY_FEATHERWHISTLE = 7954,
    NPC_AALUN = 20914,

    NPC_KAR_STORMWSINGER = 3690,
    NPC_XARTI = 7953,
    NPC_KILDAR = 4752,
    NPC_VELMA_WARNAM = 4773,
    NPC_PERASCAMIN = 16280,
};

enum Custom_Player_Chromie
{
    NPC_CHROMIE = 167032,
};

enum ChromieTalkGroup
{
    CHROMIE_TALK_BC           = 0,
    CHROMIE_TALK_WOTLK        = 1,
    CHROMIE_TALK_CATA         = 2,
    CHROMIE_TALK_MOP          = 3,
    CHROMIE_TALK_WOD          = 4,
    CHROMIE_TALK_LEGION       = 5,
    CHROMIE_TALK_BFA          = 6,
    CHROMIE_TALK_SHADOWLANDS  = 7,
    CHROMIE_TALK_DRAGONFLIGHT = 8,
};

struct ChromieIntroQuest
{
    int32 ExpansionId;
    uint32 AllianceQuest;
    uint32 HordeQuest;
    uint8 TalkGroupId;
};

static constexpr ChromieIntroQuest ChromieIntroQuests[] =
{
    {  5, 60891, 60887, CHROMIE_TALK_CATA         },
    {  6, 60959, 60961, CHROMIE_TALK_BC           },
    {  7, 60962, 60963, CHROMIE_TALK_WOTLK        },
    {  8, 60125, 60126, CHROMIE_TALK_MOP          },
    {  9, 60969, 60968, CHROMIE_TALK_WOD          },
    { 10, 60971, 60970, CHROMIE_TALK_LEGION       },
    { 14, 60545, 61874, CHROMIE_TALK_SHADOWLANDS  },
    { 15, 53370, 53372, CHROMIE_TALK_BFA          },
    { 16, 65436, 65435, CHROMIE_TALK_DRAGONFLIGHT },
};
