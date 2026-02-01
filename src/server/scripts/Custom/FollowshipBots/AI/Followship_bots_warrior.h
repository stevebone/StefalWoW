#pragma once

#include "Followship_bots_utils_spells.h"

constexpr auto FSB_GOSSIP_ITEM_ROLE_TANK = "Tank Role: take a beating";
constexpr auto FSB_GOSSIP_ITEM_ROLE_MELEE = "Melee DPS Role: use close range weapons and attacks";
//constexpr auto FSB_GOSSIP_ITEM_ROLE_FIRE = "Fire Damage Role: mostly fire spells";

extern std::vector<FSBSpellDefinition> WarriorSpellsTable;

enum FSB_WARRIOR_STANCES
{
    SPELL_WARRIOR_DEFENSIVE_STANCE = 386208
};

enum FSB_WARRIOR_COMBAT_SPELLS
{
    // PROTECTION
    SPELL_WARRIOR_DEVASTATE = 20243,
    SPELL_WARRIOR_REVENGE = 6572,
    SPELL_WARRIOR_SHIELD_SLAM = 23922,
    SPELL_WARRIOR_SHIELD_BLOCK = 2565,

    // SHARED
    SPELL_WARRIOR_CHARGE = 100
};
