

#include "followship_bots_mage.h"
#include "followship_bots_utils_spells.h"

std::vector<FSBSpellDefinition> MageSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask
    { SPELL_MAGE_ICE_BLOCK,    FSBSpellType::Damage,            0.f,        0.f,            100.f,          2.f,           true,        240000,         FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_FLURRY,       FSBSpellType::Damage,            0.f,        0.f,            90.f,           40.f,          false,       30000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_ICE_LANCE,    FSBSpellType::Damage,            0.04f,      0.f,            90.f,           40.f,          false,       1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_FROSTBOLT,    FSBSpellType::Damage,            0.04f,      0.f,            70.f,           40.f,          false,       1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_CONE_OF_COLD, FSBSpellType::Damage,            0.08f,      0.f,            70.f,           6.f,           false,       25000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_FROST_NOVA,   FSBSpellType::Damage,            0.04f,      0.f,            65.f,           6.f,           false,       30000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_ICE_BARRIER,  FSBSpellType::Damage,            0.f,        0.f,            65.f,           2.f,           true,        25000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_BLIZZARD,     FSBSpellType::Damage,            0.f,        0.f,            50.f,           30.f,          false,       12000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    { SPELL_MAGE_FROZEN_ORB,   FSBSpellType::Damage,            0.f,        0.f,            20.f,           40.f,          false,       60000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FROST },
    

    // FIRE
    { SPELL_MAGE_FIREBALL,     FSBSpellType::Damage,            0.f,        0.f,            100.f,           40.f,          false,       1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_FIRE },

    // ARCANE
    { SPELL_MAGE_ARCANE_MISSILES,   FSBSpellType::Damage,            0.f,        0.f,            50.f,           30.f,          false,       1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_ARCANE },
    { SPELL_MAGE_ARCANE_EXPLOSION,  FSBSpellType::Damage,            0.f,        0.f,            60.f,           6.f,          true,       1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_ARCANE },
    { SPELL_MAGE_ARCANE_BLAST,      FSBSpellType::Damage,            0.f,        0.f,            70.f,           30.f,          false,       1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_ARCANE },
    { SPELL_MAGE_ARCANE_BARRAGE,    FSBSpellType::Damage,            0.f,        0.f,            75.f,           30.f,          false,       1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_ARCANE },
    { SPELL_MAGE_SPELL_STEAL,    FSBSpellType::Damage,            0.f,        0.f,            75.f,           30.f,          false,       1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_ARCANE },
    { SPELL_MAGE_TIME_WARP,    FSBSpellType::Damage,            0.f,        0.f,            90.f,           2.f,          true,       300000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_ARCANE },
    { SPELL_MAGE_EVOCATION,    FSBSpellType::Damage,            0.f,        0.f,            90.f,           10.f,          true,       45000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_ARCANE },

    // ANY
    { SPELL_MAGE_POLYMORPH,    FSBSpellType::Damage,            0.f,        0.f,            30.f,           30.f,          false,       1000,          FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_MAGE_BLINK,        FSBSpellType::Damage,            0.f,        0.f,            55.f,           2.f,          true,       20000,          FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_MAGE_COUNTERSPELL, FSBSpellType::Damage,            0.f,        0.f,            100.f,           40.f,          true,       25000,          FSB_RoleMask::FSB_ROLEMASK_ANY },
};
