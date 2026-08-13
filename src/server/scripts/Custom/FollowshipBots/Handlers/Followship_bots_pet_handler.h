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

#include "SharedDefines.h"

enum FSB_PET_SPELLS
{
    //Combat
    SPELL_HUNTER_PET_FURIOUS_BITE = 263840, //10s //Wolf
    SPELL_HUNTER_PET_PRIMAL_RAGE = 264667, //6m  //Wolf
    SPELL_HUNTER_PET_PREDATOR_THIRST = 264663, //x  //Wolf

    SPELL_HUNTER_PET_BITE = 17253, //3s
    SPELL_HUNTER_PET_GORE = 1251585, //target
    SPELL_HUNTER_PET_AGILE_REFLEXES = 160011, //self
    SPELL_HUNTER_PET_MASTER_CALL = 53271, //self
    SPELL_HUNTER_PET_GROWL = 2649, //target
    SPELL_HUNTER_PET_CLAW = 16827, //target

    //BIRD OF PREY
    SPELL_HUNTER_PET_TALON_REND = 263852, //target

    //FOX
    SPELL_HUNTER_PET_SCRATCH = 256337, //target

    // BOAR
    SPELL_HUNTER_PET_BRISTLE = 263869, //self

    // HYENA
    SPELL_HUNTER_PET_RABIES = 3150, //target
    SPELL_HUNTER_PET_INFECTED_BITE = 263853, //target
};

static const std::unordered_map<CreatureFamily, std::vector<uint32>> CreatureFamilySpells =
{
    { CREATURE_FAMILY_WOLF,        { SPELL_HUNTER_PET_FURIOUS_BITE, SPELL_HUNTER_PET_BITE, SPELL_HUNTER_PET_PRIMAL_RAGE, SPELL_HUNTER_PET_PREDATOR_THIRST } },
    { CREATURE_FAMILY_FOX,         { SPELL_HUNTER_PET_SCRATCH, SPELL_HUNTER_PET_BITE, SPELL_HUNTER_PET_AGILE_REFLEXES, SPELL_HUNTER_PET_MASTER_CALL } },
    { CREATURE_FAMILY_BOAR,        { SPELL_HUNTER_PET_BITE, SPELL_HUNTER_PET_GORE, SPELL_HUNTER_PET_BRISTLE, SPELL_HUNTER_PET_MASTER_CALL } },
    { CREATURE_FAMILY_HYENA,       { SPELL_HUNTER_PET_BITE, SPELL_HUNTER_PET_INFECTED_BITE, SPELL_HUNTER_PET_RABIES, SPELL_HUNTER_PET_MASTER_CALL } },
    { CREATURE_FAMILY_BIRD_OF_PREY,{ SPELL_HUNTER_PET_CLAW, SPELL_HUNTER_PET_TALON_REND, SPELL_HUNTER_PET_GROWL, SPELL_HUNTER_PET_MASTER_CALL } },
};

namespace FSBPet
{
    bool BotSummonPet(Creature* bot);

    Unit* GetBotPet(Creature* owner);
    bool BotHasPet(Creature* owner);

    CreatureFamily GetPetFamily(uint32 entry);
    const std::vector<uint32>& GetFamilySpells(CreatureFamily family);

    bool DoAttackSpell(Creature* owner);

    bool SetBasePetInformation(Creature* owner, Creature* pet);
}
