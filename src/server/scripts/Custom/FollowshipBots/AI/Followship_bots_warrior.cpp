

#include "followship_bots_warrior.h"

std::vector<FSBSpellDefinition> WarriorSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_WARRIOR_CHARGE,         FSBSpellType::Damage,            0.f,        0.f,            100.f,          25.f,           false,        20000,         FSB_RoleMask::FSB_ROLEMASK_ANY },

    // PROTECTION
    //{ SPELL_WARRIOR_DEVASTATE,      FSBSpellType::Damage,            0.f,        0.f,            90.f,          2.f,           false,        1000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_WARRIOR_REVENGE,        FSBSpellType::Damage,            0.f,        0.f,            90.f,          2.f,           false,        1000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_WARRIOR_SHIELD_SLAM,    FSBSpellType::Damage,            0.f,        0.f,            80.f,          2.f,           false,        9000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_WARRIOR_SHIELD_BLOCK,   FSBSpellType::Damage,            0.f,        0.f,            80.f,          2.f,           true,         16000,        FSB_RoleMask::FSB_ROLEMASK_TANK },

};
