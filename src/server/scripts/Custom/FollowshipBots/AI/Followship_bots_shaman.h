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

enum FSB_SHAMAN_TOTEM_SPELLS
{
    SPELL_SHAMAN_EARTH_TOTEM = 2484, //30s, 40f //ANY
    SPELL_SHAMAN_HEALING_STREAM_TOTEM = 5394, //30s, 0f //HEAL
    SPELL_SHAMAN_WIND_RUSH_TOTEM = 192077, //2m, 40f /DMG
    SPELL_SHAMAN_LIQUID_MAGMA_TOTEM = 192222, //30s, 40f /DMG
};

namespace FSBShaman
{
    bool BotOOCBuffSelf(Creature* bot, uint32& outSpellId);
    bool BotInitialCombatSpells(Creature* bot);
}
