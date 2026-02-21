#include "Followship_bots_rogue.h"
#include "Followship_bots_utils_spells.h"


std::vector<FSBSpellDefinition> RogueSpellsTable =
{
    // Spell ID                             Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_HUMAN_WILL_TO_SURVIVE,          FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DWARF_STONEFORM,                FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DRAENEI_GIFT_NAARU,             FSBSpellType::Heal,     0.f,        50.f,           100.f,           30.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_ROGUE_CRIMSON_VIAL,             FSBSpellType::Heal,     0.f,        50.f,           100.f,           0.f,           true,       30000,          FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_ROGUE_KICK,                     FSBSpellType::Damage,   0.f,        0.f,            100.f,           2.f,           false,      15000,          FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_ROGUE_KIDNEY_SHOT,              FSBSpellType::Damage,   0.f,        0.f,            70.f,            2.f,           false,      30000,          FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_ROGUE_SLICE_DICE,               FSBSpellType::Damage,   0.f,        0.f,            70.f,            2.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_ROGUE_EVISCERATE,               FSBSpellType::Damage,   0.f,        0.f,            70.f,            2.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_ROGUE_SINISTER_STRIKE,          FSBSpellType::Damage,   0.f,        0.f,            50.f,            2.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    // ASSASSIN
    { SPELL_ROGUE_DEATHMARK,                FSBSpellType::Damage,   0.f,        0.f,            90.f,            2.f,           false,      120000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_ROGUE_GARROTE,                  FSBSpellType::Damage,   0.f,        0.f,            90.f,            2.f,           false,      6000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },

    { SPELL_ROGUE_FAN_KNIVES,               FSBSpellType::Damage,   0.f,        0.f,            80.f,            4.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_ROGUE_CRIMSON_TEMPEST,          FSBSpellType::Damage,   0.f,        0.f,            80.f,            5.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
};
