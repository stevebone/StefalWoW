// followship_bots_defines.h
#pragma once

enum class FSB_Class : uint8
{
    None = 0,
    Warrior = 1,
    Priest = 2,
    Mage = 3,
    Rogue = 4,
    Druid = 5,
    Paladin = 6,
    Hunter = 7,
    Warlock = 8,
};

enum FSB_Roles
{
    FSB_ROLE_NONE = 0,
    FSB_ROLE_MELEE_DAMAGE,
    FSB_ROLE_RANGED_DAMAGE,
    FSB_ROLE_HEALER,
    FSB_ROLE_ASSIST,
    FSB_ROLE_TANK
};

enum FSB_RoleMask : uint32
{
    FSB_ROLEMASK_ANY = 0,
    FSB_ROLEMASK_TANK = 1 << 0,
    FSB_ROLEMASK_HEALER = 1 << 1,
    FSB_ROLEMASK_RANGED_DAMAGE = 1 << 2,
    FSB_ROLEMASK_ASSIST = 1 << 3,
    FSB_ROLEMASK_MELEE_DAMAGE = 1 << 4
};

static uint32 RoleToMask(FSB_Roles role)
{
    switch (role)
    {
    case FSB_ROLE_TANK:             return FSB_ROLEMASK_TANK;
    case FSB_ROLE_HEALER:           return FSB_ROLEMASK_HEALER;
    case FSB_ROLE_RANGED_DAMAGE:    return FSB_ROLEMASK_RANGED_DAMAGE;
    case FSB_ROLE_ASSIST:           return FSB_ROLEMASK_ASSIST;
    case FSB_ROLE_MELEE_DAMAGE:     return FSB_ROLEMASK_MELEE_DAMAGE;
    default:                        return 0;
    }
}
