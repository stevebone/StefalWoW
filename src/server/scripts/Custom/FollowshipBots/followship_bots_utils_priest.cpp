

#include "followship_bots_priest.h"
#include "followship_bots_utils_spells.h"


std::vector<FSBSpellDefinition> PriestSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask
    { SPELL_PRIEST_DESPERATE_PRAYER,    FSBSpellType::Heal,     0.f,        20.f,           100.f,          0.f,            true,       90000,          FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_HEAL,                FSBSpellType::Heal,     0.f,        40.f,           80.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_PRAYER_OF_MENDING,   FSBSpellType::Heal,     0.f,        60.f,           50.f,           40.f,           false,      9000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_FLASH_HEAL,          FSBSpellType::Heal,     0.f,        70.f,           70.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_RENEW,               FSBSpellType::Heal,     0.f,        85.f,           60.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_PURIFY,              FSBSpellType::Heal,     0.f,        100.f,          70.f,           40.f,           false,      8000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_POWER_WORD_SHIELD,   FSBSpellType::Heal,     0.f,         70.f,          70.f,           30.f,           false,      8000,           FSB_RoleMask::FSB_ROLEMASK_HEALER | FSB_RoleMask::FSB_ROLEMASK_ASSIST },

    { SPELL_PRIEST_PSYCHIC_SCREAM,      FSBSpellType::Damage,   0.f,        0.f,            80.f,           8.f,            true,       45000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE    },
    { SPELL_PRIEST_SHADOW_WORD_PAIN,    FSBSpellType::Damage,   0.f,        0.f,            80.f,           30.f,           false,      15000,          FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_VAMPIRIC_TOUCH,      FSBSpellType::Damage,   0.f,        0.f,            80.f,           30.f,           false,      15000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_MIND_BLAST,          FSBSpellType::Damage,   0.f,        0.f,            70.f,           30.f,           false,      9000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_HOLY_FIRE,           FSBSpellType::Damage,   0.f,        0.f,            60.f,           30.f,           false,      10000,          FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_PENANCE,             FSBSpellType::Damage,   0.2f,       0.f,            55.f,           40.f,           false,      9000,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_SMITE,               FSBSpellType::Damage,   0.f,        0.f,            50.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_DEVOURING_PLAGUE,    FSBSpellType::Damage,   0.5f,       0.f,            40.f,           40.f,           false,      10000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_MIND_FLAY,           FSBSpellType::Damage,   0.2f,       0.f,            30.f,           35.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },

};
