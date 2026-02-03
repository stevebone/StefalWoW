

#include "followship_bots_warrior.h"

std::vector<FSBSpellDefinition> WarriorSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_WARRIOR_CHARGE,         FSBSpellType::Damage,            0.f,        0.f,            100.f,          25.f,          false,        20000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARRIOR_TAUNT,          FSBSpellType::Damage,            0.f,        0.f,            100.f,          30.f,          false,        8000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARRIOR_PUMMEL,         FSBSpellType::Damage,            0.f,        0.f,            100.f,          2.f,           false,        15000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARRIOR_HEROIC_THROW,   FSBSpellType::Damage,            0.f,        0.f,            100.f,          30.f,          false,        6000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARRIOR_RECKLESSNESS,   FSBSpellType::Damage,            0.f,        0.f,            50.f,           2.f,           true,         90000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARRIOR_WHIRLWIND,      FSBSpellType::Damage,            0.f,        0.f,            50.f,           2.f,           true,         1000,         FSB_RoleMask::FSB_ROLEMASK_ANY },

    // PROTECTION
    { SPELL_WARRIOR_DISARM,         FSBSpellType::Damage,            0.f,        0.f,            90.f,          2.f,           false,        45000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_WARRIOR_EXECUTE,        FSBSpellType::Damage,            0.f,        0.f,            90.f,          2.f,           false,        6000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_WARRIOR_REVENGE,        FSBSpellType::Damage,            0.f,        0.f,            90.f,          2.f,           false,        1000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_WARRIOR_REND,           FSBSpellType::Damage,            0.f,        0.f,            80.f,          2.f,           false,        1000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_WARRIOR_DEVASTATE,      FSBSpellType::Damage,            0.f,        0.f,            70.f,          2.f,           false,        1000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    
    { SPELL_WARRIOR_SHIELD_SLAM,    FSBSpellType::Damage,            0.f,        0.f,            80.f,          2.f,           false,        9000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_WARRIOR_SHIELD_BLOCK,   FSBSpellType::Damage,            0.f,        0.f,            80.f,          2.f,           true,         16000,        FSB_RoleMask::FSB_ROLEMASK_TANK },

    { SPELL_WARRIOR_DEMO_SHOUT,     FSBSpellType::Damage,            0.f,        0.f,            60.f,          6.f,           true,        45000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_WARRIOR_IGNORE_PAIN,    FSBSpellType::Damage,            0.f,        0.f,            60.f,          2.f,           true,        1000,         FSB_RoleMask::FSB_ROLEMASK_TANK },

    // DPS
    { SPELL_WARRIOR_MORTAL_STRIKE,  FSBSpellType::Damage,            0.f,        0.f,            90.f,          2.f,           false,       6000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_WARRIOR_SLAM,           FSBSpellType::Damage,            0.f,        0.f,            90.f,          2.f,           false,       1000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_WARRIOR_BLOODTHIRST,    FSBSpellType::Damage,            0.f,        0.f,            90.f,          2.f,           false,       4500,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_WARRIOR_RAGING_BLOW,    FSBSpellType::Damage,            0.f,        0.f,            90.f,          2.f,           false,       8000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },

    { SPELL_WARRIOR_CLEAVE,         FSBSpellType::Damage,            0.f,        0.f,            80.f,          2.f,           false,        4500,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_WARRIOR_COLOSSUS_SMASH, FSBSpellType::Damage,            0.f,        0.f,            80.f,          2.f,           false,        45000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },

    { SPELL_WARRIOR_RAMPAGE,        FSBSpellType::Damage,            0.f,        0.f,            60.f,          2.f,           false,        1000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_WARRIOR_FURIOUS_SLASH,  FSBSpellType::Damage,            0.f,        0.f,            50.f,          2.f,           false,        1000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
};

namespace FSBWarrior
{
    bool BotHasDefensiveStance(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->HasAura(SPELL_WARRIOR_DEFENSIVE_STANCE))
            return true;

        return false;

    }

    bool BotHasBattleStance(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->HasAura(SPELL_WARRIOR_BATTLE_STANCE))
            return true;

        return false;

    }

    bool BotHasBerserkerStance(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->HasAura(SPELL_WARRIOR_BERSERKER_STANCE))
            return true;

        return false;

    }
}
