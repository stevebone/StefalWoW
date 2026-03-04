#pragma once

#include "Followship_bots_spells_handler.h"

extern std::vector<FSBSpellDefinition> ShamanSpellsTable;

enum FSB_SHAMAN_GENERIC_SPELLS
{
    SPELL_SHAMAN_ANCESTRAL_SPIRIT = 2008, //RESS

    SPELL_SHAMAN_HEALING_SURGE = 8004, //0s, 40f, 10% mana

    SPELL_SHAMAN_SKYFURY = 462854, //buff self
    SPELL_SHAMAN_LIGHTNING_SHIELD = 192106, //buff self

    SPELL_SHAMAN_LIGHTNING_BOLT = 224721, //188196, //0s, 40f, 0.2% mana
    SPELL_SHAMAN_PRIMAL_STRIKE = 73899, //
};

namespace FSBShaman
{
    bool BotOOCBuffSelf(Creature* bot, uint32& outSpellId);
}
