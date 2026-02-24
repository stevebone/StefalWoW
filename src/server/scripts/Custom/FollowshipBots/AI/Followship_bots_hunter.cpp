#include "Followship_bots_utils.h"

#include "Followship_bots_hunter.h"

std::vector<FSBSpellDefinition> HunterSpellsTable =
{
    // Spell ID                             Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_HUMAN_WILL_TO_SURVIVE,          FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DWARF_STONEFORM,                FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DRAENEI_GIFT_NAARU,             FSBSpellType::Heal,     0.f,        50.f,           100.f,           30.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_HUNTER_STEADY_SHOT,             FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },

};

namespace FSBHunter
{

}
