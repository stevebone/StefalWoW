#pragma once

#include "Followship_bots_spells_handler.h"

extern std::vector<FSBSpellDefinition> MonkSpellsTable;

enum FSB_MONK_GENERIC_SPELLS
{
    SPELL_MONK_BLACKOUT_KICK = 205523, //4s, 0f, //ANY
    SPELL_MONK_LEG_SWEEP = 119381, //1m, 6f, //ANY

    SPELL_MONK_RESUSCITATE = 157202, //10s 40f res
};

enum FSB_MONK_HEALER_SPELLS
{
    SPELL_MONK_VIVIFY = 116670, //2s, 40f, 3% mana
    SPELL_MONK_EXPEL_HARM = 322101, //15s, 0f, 1.4% mana
    SPELL_MONK_SPINNING_CRANE = 101546, //0s, 8f cc, 1% mana
};

enum FSB_MONK_DPS_SPELLS
{
    //SPELL_MONK_VIVFY = 116670, //2s, 40f, 30 energy
    //SPELL_MONK_EXPEL_HARM = 322101, //15s, 0f, 15 energy
    //SPELL_MONK_SPINNING_CRANE = 101546, //0s, 8f cc, 40 energy
    SPELL_MONK_TIGER_PALM = 100780, //0s, 2f, 50 energy
    SPELL_MONK_EXPLODING_KEG = 325153, //1m, 40f,
    SPELL_MONK_CHI_BURST = 123986, //30s, 40f
    SPELL_MONK_INVOKE_NIUZAO = 132578, //2m, 40f
    SPELL_MONK_KEG_SMASH = 121253, //8s, 15f
    SPELL_MONK_RUSHING_JADE_WIND = 261715, //6s, 8f cc
    SPELL_MONK_BREATH_FIRE = 115181, //15s, 2f

    SPELL_MONK_CELESTIAL_INFUSION = 1241059, //45s, 0f,
    SPELL_MONK_CELESTIAL_BREW = 322507, //45s, 0f
    SPELL_MONK_BLACK_OX_BREW = 115399, //2m, 0f energy refill
    SPELL_MONK_BLACK_OX_STATUE = 115315, //10s, 0f cc
    SPELL_MONK_PURIFYING_BREW = 119582, //10s, 0f, 8% hp

    SPELL_MONK_SPEAR_HAND_STRIKE = 116705, //15s, 2f interrupt
};

enum FSB_MONK_TANK_SPELLS
{
    SPELL_MONK_PROVOKE = 115546, //8s, 30f
    //SPELL_MONK_SPINNING_CRANE = 101546, //0s, 8f cc, 2 chi
    SPELL_MONK_RUSHING_WIND = 116847, //6s, 8f cc
};

namespace FSBMonk
{
    void HandleOnSpellCast(Creature* bot, uint32 spellId);
}
