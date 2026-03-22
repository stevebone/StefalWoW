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
 
 /*######
## npc_custom_hogger
######*/

enum HoggerSpellData
{
    SPELL_SUMMON_MINIONS = 87366,
    SPELL_VICIOUS_SLICE = 87337,
    SPELL_EATING = 87351,
    SPELL_TELEPORT_VISUAL_ONLY_1 = 87459,
    SPELL_TELEPORT_VISUAL_ONLY_2 = 64446,
    SPELL_BLOODY_STRIKE = 87359
};

enum HoggerEventData
{
    EVENT_VICIOUS_SLICE = 1,
    EVENT_HAMMOND_GROUP_START_WALKING = 2,
    EVENT_DISMOUNT_HAMMOND_CLAY = 3,
    EVENT_HOGGER_SAY_GRR = 4,
    EVENT_CLAYS_EXPLAINATION = 5,
    EVENT_CLAY_SAYS_TAKE_HIM = 6,
    EVENT_HOGGER_SAYS_NOO = 7,
    EVENT_CLAY_SPEAKS_TO_ANDROMATH = 8,
    EVENT_TELEPORT_BACK = 9,
    EVENT_CHECK_EAT_RANGE = 10,
    EVENT_BLOODY_STRIKE = 11,
    EVENT_RAGAMUFFIN_LEAVE1 = 12,
    EVENT_RAGAMUFFIN_LEAVE2 = 13,
};

enum HoggerTextData
{
    SAY_HOGGER_AGGRO = 0,
    SAY_HOGGER_SUMMON_MINIONS = 1,
    SAY_EATING = 2,
    SAY_STUNNED = 3,
    SAY_BEG = 4,
    SAY_RAND = 5,
    SAY_FINAL = 6
};

enum HammondTextData
{
    YELL_OPENING = 0,
    SAY_EXPLAINATION = 11,
    SAY_TAKE_HIM = 12,
    SAY_TO_ANDROMATH = 13
};

enum RagamuffinTextData
{
    SAY_CLAY = 0,
    SAY_WOW = 1,
};

enum HoggerCreatures
{
    NPC_GENERAL_HAMMOND_CLAY = 65153,
    NPC_ANDROMATH = 46941,
    NPC_DUMAS = 46940,
    NPC_HOGGER = 448,
    NPC_EATING_TARGET = 45979,
    NPC_RAGAMUFFIN = 46943,
    NPC_HOGGER_MINION = 46932
};

enum HoggerMiscData
{
    DISPLAYID_GENERAL_HAMMOND_CLAYS_MOUNT = 2410
};

// validated positions
static const Position generalHammondClayPositions[4] =
{
    { -10125.35f, 650.7324f, 36.05776f },
    { -10128.3f,  656.4648f, 36.05776f },
    { -10131.25f, 662.1973f, 36.05776f },
    { -10135.73f, 668.389f,  35.74807f }
};

// validated positions
static const Position andromathPositions[3] =
{
    { -10119.2f, 647.913f, 36.36745f },
    { -10123.0f, 656.875f, 36.05776f },
    { -10126.8f, 665.837f, 35.74807f }
};

// validated positions
static const Position dumasPositions[3] =
{
    { -10130.1f, 647.7671f, 36.04665f },
    { -10132.9f, 653.5605f, 36.05776f },
    { -10135.7f, 659.354f,  36.06887f }
};

// validated positions
static const Position hoggerPositions[1] =
{
    { -10136.9f, 670.009f, 36.03682f }
};

// validated positions
static const Position ragamuffinPositions[2] =
{
    { -10127.00f, 651.0f, 36.05776f },
    { -10123.0f, 651.0f,  36.06887f }
};

// validated positions
static const Position ragamuffinPositionsLeave[1] =
{
    { -10090.936523f, 592.084351f, 39.357849f }
};

/*######
## npc_custom_hogger_minion
######*/

enum HoggerMinionSpellData
{
    SPELL_ADVENTURERS_RUSH = 87402
};