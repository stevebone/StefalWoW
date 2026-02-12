#include "Followship_bots_druid.h"

std::vector<FSBSpellDefinition> DruidSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_DRUID_WRATH,                FSBSpellType::Damage,   0.f,        0.f,            100.f,            40.f,            false,       1000,        FSB_RoleMask::FSB_ROLEMASK_ANY },

};
