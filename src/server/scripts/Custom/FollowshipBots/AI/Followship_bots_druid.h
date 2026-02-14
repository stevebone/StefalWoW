#pragma once

#include "Followship_bots_utils.h"

extern std::vector<FSBSpellDefinition> DruidSpellsTable;

enum DRUID_SHAPESHIFT_FORMS
{
    SPELL_DRUID_BEAR = 18309,
    SPELL_DRUID_CAT = 197167,
    SPELL_DRUID_MOONKIN = 24858,
    SPELL_DRUID_TREANT = 176295
};

enum DRUID_BUFF_SPELLS
{
    SPELL_DRUID_MARK_WILD = 1126,
    SPELL_DRUID_THORNS = 418805
};

enum DRUID_SPECIAL_SPELLS
{
    SPELL_DRUID_CAT_PROWL = 5215,
};

enum DRUID_COMBAT_SPELLS
{
    SPELL_DRUID_WRATH = 5176, //76815,
    SPELL_DRUID_REGROWTH = 8936, //97426,
    SPELL_DRUID_BARKSKIN = 22812,

    // BEAR
    SPELL_DRUID_BEAR_GROWL = 182848,
    SPELL_DRUID_BEAR_THRASH = 292576,
    SPELL_DRUID_BEAR_MANGLE = 288266,

    // CAT
    SPELL_DRUID_CAT_THRASH = 172035,
    SPELL_DRUID_CAT_SHRED = 215442,
    SPELL_DRUID_CAT_MANGLE = 79828,
    SPELL_DRUID_CAT_FEROCIOUS_BITE = 27557,

    // RANGED
    SPELL_DRUID_MOONFIRE = 8921, //15798,
    SPELL_DRUID_ROOTS = 37823,
    SPELL_DRUID_FORCE_NATURE = 205636,
    SPELL_DRUID_SOLAR_BEAM = 78675,
    SPELL_DRUID_NEW_MOON = 274281,
    SPELL_DRUID_STARFALL = 1286243,
    SPELL_DRUID_WILD_MUSHROOM = 88747,
    SPELL_DRUID_CELESTIAL_ALIGNMENT = 194223,

    // HEALER
    SPELL_DRUID_TREE_OF_LIFE = 33891,
    SPELL_DRUID_IRONBARK = 102342,

    // COMBAT BUFFS
    
};

static const uint32 druidAurasToRemoveForHealer[] =
{
    SPELL_DRUID_BEAR,
    SPELL_DRUID_MOONKIN,
    SPELL_DRUID_CAT,
};

static const uint32 druidAurasToRemoveForTank[] =
{
    SPELL_DRUID_TREANT,
    SPELL_DRUID_MOONKIN,
    SPELL_DRUID_CAT,
};

static const uint32 druidAurasToRemoveForMelee[] =
{
    SPELL_DRUID_TREANT,
    SPELL_DRUID_MOONKIN,
    SPELL_DRUID_BEAR,
};

static const uint32 druidAurasToRemoveForDamage[] =
{
    SPELL_DRUID_TREANT,
    SPELL_DRUID_BEAR,
    SPELL_DRUID_CAT,
};

namespace FSBDruid
{
    bool BotOnAuraApplied(Creature* bot, AuraApplication const* aurApp, bool applied, FSBBotStats& botStats);
    void HandleOnSpellCast(Creature* bot, uint32 spellId);
    void BotSetRoleAuras(Creature* bot, FSB_Roles role);
    bool BotHasMarkWild(Creature* bot);
    bool BotHasIronbark(Creature* bot);
}
