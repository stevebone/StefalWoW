#pragma once

#include "Followship_bots_spells_handler.h"

enum FSB_ROGUE_SPECIAL_SPELLS
{
    // Heal
    SPELL_ROGUE_CRIMSON_VIAL = 185311, // 30s, 0f
};

enum FSB_ROGUE_POISON_SPELLS
{
    SPELL_ROGUE_INSTANT_POISON = 315584, // 1h
    SPELL_ROGUE_WOUND_POISON = 8679, // 1h

    SPELL_ROGUE_CRIPPLING_POISON = 3408, // 1h
};

enum FSB_ROGUE_COMBAT_SPELLS
{
    SPELL_ROGUE_SINISTER_STRIKE = 193315,

    SPELL_ROGUE_KIDNEY_SHOT = 1766, // 30s, 2f
    SPELL_ROGUE_SLICE_DICE = 5171, // 0s, 2f
    SPELL_ROGUE_EVISCERATE = 196819, // 0s, 2f

    SPELL_ROGUE_KICK = 1766, // 15s, 2f

    // Assassin
    SPELL_ROGUE_DEATHMARK = 360194, // 2m, 2f
    SPELL_ROGUE_GARROTE = 703, // 6s, 2f

    SPELL_ROGUE_CRIMSON_TEMPEST = 1247227, // 0s, 5f
    SPELL_ROGUE_FAN_KNIVES = 51723, // 0s, 4f
};

extern std::vector<FSBSpellDefinition> RogueSpellsTable;

namespace FSBRogue
{

}
