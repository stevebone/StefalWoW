#include "followship_bots_paladin.h"

std::vector<FSBSpellDefinition> PaladinSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_PALADIN_HOLY_LIGHT,         FSBSpellType::Heal,     0.f,        50.f,            100.f,           40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },

};
