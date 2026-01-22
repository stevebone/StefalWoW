#pragma once

#include "followship_bots_utils_spells.h"

constexpr auto FSB_GOSSIP_ITEM_ROLE_ARCANE = "Arcane Damage Role: mostly arcane spells";
constexpr auto FSB_GOSSIP_ITEM_ROLE_FROST = "Frost Damage Dealer: mostly frost and ice spells";
constexpr auto FSB_GOSSIP_ITEM_ROLE_FIRE = "Fire Damage Role: mostly fire spells";

extern std::vector<FSBSpellDefinition> MageSpellsTable;

// Priest spell tables

enum FSB_MAGE_BUFF_SPELLS
{
    SPELL_MAGE_ARCANE_INTELLECT = 1459
};

enum FSB_MAGE_COMBAT_SPELLS
{
    // FROST
    SPELL_MAGE_FROSTBOLT = 12675, //164378 //116
    SPELL_MAGE_FIREBALL = 11921,
    SPELL_MAGE_ARCANE_MISSILES = 15791
};
