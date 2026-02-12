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
    Shaman = 9
};

enum class FSB_Race : uint8
{
    None = 0,
    Human = 1,
    Dwarf = 2,
    NightElf = 3,
    Gnome = 4,
    Draenei = 5,
    Worgen = 6,
    Pandaren = 7,
    VoidElf = 8,
};

enum FSB_Roles
{
    FSB_ROLE_NONE = 0,
    FSB_ROLE_MELEE_DAMAGE,
    FSB_ROLE_RANGED_DAMAGE,
    FSB_ROLE_HEALER,
    FSB_ROLE_ASSIST,
    FSB_ROLE_TANK,
    FSB_ROLE_RANGED_FROST,
    FSB_ROLE_RANGED_ARCANE,
    FSB_ROLE_RANGED_FIRE,
    FSB_ROLE_RANGED_AFFLICTION,
    FSB_ROLE_RANGED_DEMONOLOGY,
    FSB_ROLE_RANGED_DESTRUCTION
};

enum FSB_RoleMask : uint32
{
    FSB_ROLEMASK_ANY = 0,
    FSB_ROLEMASK_TANK = 1 << 0,
    FSB_ROLEMASK_HEALER = 1 << 1,
    FSB_ROLEMASK_RANGED_DAMAGE = 1 << 2,
    FSB_ROLEMASK_ASSIST = 1 << 3,
    FSB_ROLEMASK_MELEE_DAMAGE = 1 << 4,
    FSB_ROLEMASK_RANGED_FROST = 1 << 5,
    FSB_ROLEMASK_RANGED_ARCANE = 1 << 6,
    FSB_ROLEMASK_RANGED_FIRE = 1 << 7,
    FSB_ROLEMASK_RANGED_AFFLICTION = 1 << 8,
    FSB_ROLEMASK_RANGED_DEMONOLOGY = 1 << 9,
    FSB_ROLEMASK_RANGED_DESTRUCTION = 1 << 10
};

static uint32 RoleToMask(FSB_Roles role)
{
    switch (role)
    {
    case FSB_ROLE_TANK:                 return FSB_ROLEMASK_TANK;
    case FSB_ROLE_HEALER:               return FSB_ROLEMASK_HEALER;
    case FSB_ROLE_RANGED_DAMAGE:        return FSB_ROLEMASK_RANGED_DAMAGE;
    case FSB_ROLE_ASSIST:               return FSB_ROLEMASK_ASSIST;
    case FSB_ROLE_MELEE_DAMAGE:         return FSB_ROLEMASK_MELEE_DAMAGE;
    case FSB_ROLE_RANGED_FROST:         return FSB_ROLEMASK_RANGED_FROST;
    case FSB_ROLE_RANGED_ARCANE:        return FSB_ROLEMASK_RANGED_ARCANE;
    case FSB_ROLE_RANGED_FIRE:          return FSB_ROLEMASK_RANGED_FIRE;
    case FSB_ROLE_RANGED_AFFLICTION:    return FSB_ROLEMASK_RANGED_AFFLICTION;
    case FSB_ROLE_RANGED_DEMONOLOGY:    return FSB_ROLEMASK_RANGED_DEMONOLOGY;
    case FSB_ROLE_RANGED_DESTRUCTION:   return FSB_ROLEMASK_RANGED_DESTRUCTION;
    default:                            return 0;
    }
}

enum FSB_MovementStates
{
    FSB_MOVE_STATE_IDLE = 0,
    FSB_MOVE_STATE_FOLLOWING = 1,
    FSB_MOVE_STATE_STAY = 2
};
