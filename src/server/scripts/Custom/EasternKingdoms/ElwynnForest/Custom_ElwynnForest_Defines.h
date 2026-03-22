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

enum Custom_Northshire_Generic_NPCs
{
    NPC_NORTHSHIRE_WORG = 49871,
    NPC_NORTHSHIRE_SPY = 49874,
    NPC_NORTHSHIRE_ASSASSIN = 50039,
    NPC_NORTHSHIRE_STORMWIND_INFANTRY = 49869,
};

enum Custom_Northshire_Generic_Spells
{
    SPELL_PAXTON_FLASH_HEAL = 17843,
    SPELL_PAXTON_PRAYER_OF_HEALING = 93091,
    SPELL_PAXTON_RENEW = 93094,
    SPELL_PAXTON_FORTITUDE = 13864,
};

enum Custom_Eastvale_Generic_NPCs
{
    NPC_EASTVALE_RAELEN = 10616,
    NPC_EASTVALE_PERRY = 1650,
    NPC_EASTVALE_LUMBERJACK = 1975,
    NPC_EASTVALE_PEASANT = 11328,
    NPC_EASTVALE_PROWLER = 118,
    NPC_EASTVALE_MARSHAL_PETTERSON = 42256
};

// Work points for lumberjacks
std::vector<Position> EastvaleWorkSpots =
{
    { -9446.307617f, -1269.940674f, 45.648872f, 3.255455f },
    { -9399.471680f, -1301.089722f, 52.264595f, 6.242860f },
    { -9411.202148f, -1227.282104f, 54.010494f, 0.760771f },
    { -9459.336914f, -1213.114502f, 47.256680f, 1.922275f },
    { -9529.320312f, -1323.670044f, 45.774399f, 2.709450f },
    { -9535.205078f, -1272.185059f, 43.205242f, 3.308480f },
    { -9437.122079f, -1188.336670f, 52.931740f, 1.904136f },
    { -9410.822266f, -1218.313965f, 54.994865f, 5.254884f },
    { -9387.245117f, -1248.24231f,  59.126514f, 0.065248f },
    { -9370.636719f, -1287.655151f, 58.614361f, 5.640986f },
    { -9371.529297f, -1293.885742f, 57.514565f, 0.667326f },
    { -9536.652344f, -1273.558716f, 43.550587f, 3.236280f },
    { -9352.536133f, -1311.200317f, 59.448452f, 2.071118f },
    { -9554.483398f, -1343.374634f, 48.531834f, 3.357558f },
    { -9479.631836f, -1405.977295f, 51.959297f, 5.796676f },
    { -9331.194336f, -1246.684814f, 66.264290f, 0.757329f },
    { -9326.047852f, -1291.486694f, 66.424393f, 5.354681f },
    { -9383.676758f, -1182.656616f, 63.201351f, 0.309986f },
    { -9417.114258f, -1445.833740f, 60.513313f, 4.425934f }
};

enum EastvaleLumberjackEvents
{
    EVENT_GO_TO_WORK_SPOT = 1,
    EVENT_GO_TO_SUPERVISOR,
    EVENT_CHOP_WOOD,
    EVENT_RETURN_HOME,
    EVENT_IDLE,
    EVENT_BREAK_NOT_ALLOWED,
    EVENT_BREAK_ALLOWED
};

// Say lines
std::vector<std::string> EastvaleLumberjackWorkLines =
{
    "Another day, another log.",
    "Hard work keeps the wolves away.",
    "Stormwind needs strong arms!",
    "This forest won't chop itself.",
    "Alright... alright, am working!"
};

std::vector<std::string> EastvalePanicLines =
{
    "Wolves! Help!",
    "Somebody, please help!",
    "I'm not paid enough for this!",
    "Guards! Wolves in the camp!",
    "Help! A wolf attack!",
};

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
    EVENT_BLOODY_STRIKE = 11
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

/*######
## npc_custom_hogger_minion
######*/

enum HoggerMinionSpellData
{
    SPELL_ADVENTURERS_RUSH = 87402
};
