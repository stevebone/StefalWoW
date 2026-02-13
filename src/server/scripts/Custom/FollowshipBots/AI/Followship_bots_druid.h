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

enum DRUID_COMBAT_SPELLS
{
    SPELL_DRUID_WRATH = 76815,

    // BEAR
    SPELL_DRUID_BEAR_THRASH = 292576,

    // CAT
    SPELL_DRUID_CAT_THRASH = 172035,
    SPELL_DRUID_CAT_SHRED = 215442
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
    bool BotOnAuraApplied(Creature* bot, AuraApplication const* aurApp, bool applied);
    void BotSetRoleAuras(Creature* bot, FSB_Roles role);
    void BotSetRolePower(Creature* bot, FSB_Roles role);
}
