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

enum Custom_Tirisfal_Glades_Areas
{
    AREA_DEATHKNELL = 154,
    
};

enum Custom_Deathknell_Generic_NPCs
{
    NPC_DEATHKNELL_SALTAIN = 1740,
    NPC_DEATHKNELL_SCARLET_CORPSE = 49340,
    NPC_DEATHKNELL_DARNELL = 49337,
};

enum Custom_Deathknell_Quests
{
    QUEST_FRESH_OUT_OF_THE_GRAVE = 24959, // on reward Darnell is summoned
    QUEST_RECRUITMENT = 26800, // on reward Darnell is despawned
    QUEST_THE_SHADOW_GRAVE = 28608, // on complete Darnell despawns
    QUEST_THOSE_THAT_COULDNT_BE_SAVED = 26799,
    QUEST_CARETAKER_CAICE = 28652,
    QUEST_THE_WAKENING = 24960, // on reward Darnell is summoned
    QUEST_BEYOND_THE_GRAVES = 25089, 
};

enum Custom_Deathknell_Spells
{
    SPELL_DEATHKNELL_SELECT_SCARLET_CORPSE = 91942,
    SPELL_DEATHKNELL_REVERSE_CAST_RIDE_VEHICLE = 91945, //258344, //84109, //46598, //,
    SPELL_DEATHKNELL_SLUMPED_OVER = 91935,
    SPELL_DEATHKNELL_FEIGN_DEATH = 29266,
    SPELL_DEATHKNELL_COSMETIC_YELLOW_ARROW = 92230,
    SPELL_DEATHKNELL_GRAB_PASSENGER = 46598, //61178, //193710,
    SPELL_DEATHKNELL_SUMMON_DARNELL = 91938,
};

enum Custom_DeathKnell_Events
{
    EVENT_DARNELL_PICKUP = 1,
    EVENT_DARNELL_RETURN = 2,
    EVENT_DARNELL_SAY_WAY_TO_DEATHKNELL = 3,
};

enum Custom_Deathknell_Darnell_Talk
{
    DEATHKNELL_DARNELL_SAY_HELLO = 0, // multiple options
    DEATHKNELL_DARNELL_SAY_WAY_DEATHKNELL = 1,
    DEATHKNELL_DARNELL_SAY_ASK_SALTAIN = 2,
    DEATHKNELL_DARNELL_SAY_CONFIRM_SALTAIN = 3,
    DEATHKNELL_DARNELL_SAY_GO_CORPSES = 4,
    DEATHKNELL_DARNELL_SAY_SEE_CORPSES = 5,
    DEATHKNELL_DARNELL_THIS_WAY = 6, // to the shadow grave multiple options
};
