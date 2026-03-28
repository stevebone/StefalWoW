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

#include "Position.h"

enum Custom_Westfall_Generic_NPCs
{
    NPC_WESTFALL_THUG = 42387,
    NPC_WESTFALL_Q26232_TRIGGER = 42562,
    NPC_WESTFALL_Q26232_CREDIT = 42417,

    NPC_WESTFALL_LIEUTENANT_HORATIO = 42308,

    NPC_WESTFALL_STEW = 42617,
    NPC_WESTFALL_HOMELESS_STORMWIND_CITIZEN = 42384,
    NPC_WESTFALL_HOMELESS_STORMWIND_CITIZEN_FEMALE = 42386,

    NPC_WESTFALL_KEARNEN = 7024,
    NPC_WESTFALL_ELITE_MERCENARY = 42656,
    NPC_WESTFALL_SHADOWY_FIGURE = 42662,
    NPC_WESTFALL_HELIX_MORTWAKE = 42655,
    NPC_WESTFALL_OAF_MORTWAKE = 42754
};

enum Custom_Westfall_Quests
{
    QUEST_LOUS_PARTING_THOUGHTS = 26232,
    QUEST_THE_DEFIAS_BROTHERHOOD_FINAL = 166,
    QUEST_FEEDING_THE_HUNGRY = 26271,
    QUEST_SECRETS_OF_THE_TOWER = 26290,
};

enum Custom_Westfall_Spells
{
    SPELL_WESTFALL_DETECT_QUEST_INVIS_1 = 79229,
    SPELL_WESTFALL_DETECT_QUEST_INVIS_2 = 79341,
    SPELL_WESTFALL_FULL_BELLY = 79451,
    SPELL_WESTFALL_POTION_SHROUDING = 79528,
    SPELL_WESTFALL_KILL_SHOT = 79526,
    SPELL_WESTFALL_TELEPORT_VISUAL = 64446,
    SPELL_WESTFALL_Q26290_CREDIT = 79534,
};

enum Custom_Westfall_Events
{
    EVENT_KEARNEN_KILL_SHOT = 1,
};

static Position const Q26232ThugPositions[4] =
{
    { -9859.36f, 1332.42f, 41.985f, 2.49f },
    { -9862.51f, 1332.08f, 41.985f, 0.85f },
    { -9863.49f, 1335.49f, 41.985f, 5.63f },
    { -9860.42f, 1335.46f, 41.985f, 4.11f },
};

constexpr int SAY_FURLBROW_RETURN = 42446;

enum Custom_Westfall_Q26232_TALK
{
    Q26232_SAY_THREATEN_PLAYER = 0,
    Q26232_SAY_ASK_MEET = 1,
    Q26232_SAY_NOTICE_PLAYER = 2,
    Q26232_SAY_CONFIRM_MEET = 3,
    Q26232_SAY_CONGRATULATE = 4,
    Q26232_SAY_ASK_IDENTITY = 5,
    Q26232_SAY_DIE = 6,  
};

enum Custom_Westfall_Q26290_TALK
{
    Q26232_SAY_HELIX_0 = 0,
    Q26232_SAY_HELIX_1 = 1,
    Q26232_SAY_HELIX_2 = 2,
    Q26232_SAY_HELIX_3 = 3,

    Q26232_SAY_SHADOWY_0 = 0,
    Q26232_SAY_SHADOWY_1 = 1,
    Q26232_SAY_SHADOWY_2 = 2,
    Q26232_SAY_SHADOWY_3 = 3,
    Q26232_SAY_SHADOWY_4 = 4,
};
