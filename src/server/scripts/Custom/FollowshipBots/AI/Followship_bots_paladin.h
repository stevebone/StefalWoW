#pragma once

#include "Followship_bots_utils_spells.h"

extern std::vector<FSBSpellDefinition> PaladinSpellsTable;

enum FSB_PALADIN_HEALING_SPELLS
{
    // HEALER
    SPELL_PALADIN_HOLY_LIGHT = 295698, //82326,
    SPELL_PALADIN_HOLY_SHOCK = 36340, //20473,

    SPELL_PALADIN_REDEMPTION = 7328,

    // DPS
    SPELL_PALADIN_CRUSADER_STRIKE = 35395,
    SPELL_PALADIN_JUDGEMENT = 20271,
    SPELL_PALADIN_CONSECRATION = 26573
};

enum FSB_PALADIN_AURAS
{
    SPELL_PALADIN_DEVOTION_AURA = 465,
    SPELL_PALADIN_RETRIBUTION_AURA = 8990,
    SPELL_PALADIN_CONCENTRATION_AURA = 79963
};

namespace FSBPaladin
{
    void HandleOnSpellCast(Creature* bot, uint32 spellId);
}
