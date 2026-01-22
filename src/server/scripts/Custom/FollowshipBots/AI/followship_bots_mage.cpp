

#include "followship_bots_mage.h"
#include "followship_bots_utils_spells.h"

std::vector<FSBSpellDefinition> MageSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask
    { SPELL_MAGE_FROSTBOLT,    FSBSpellType::Damage,            0.f,        0.f,            100.f,           40.f,          false,       1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_FIREBALL,     FSBSpellType::Damage,            0.f,        0.f,            100.f,           40.f,          false,       1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_ARCANE_MISSILES,    FSBSpellType::Damage,            0.f,        0.f,            100.f,           30.f,          false,       1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
};
