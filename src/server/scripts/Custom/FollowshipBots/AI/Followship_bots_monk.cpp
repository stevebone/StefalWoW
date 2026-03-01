#include "Followship_bots_utils.h"

#include "Followship_bots_monk.h"


std::vector<FSBSpellDefinition> MonkSpellsTable =
{
    // Spell ID                             Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_HUMAN_WILL_TO_SURVIVE,          FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DWARF_STONEFORM,                FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DRAENEI_GIFT_NAARU,             FSBSpellType::Heal,     0.f,        50.f,           100.f,           30.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_MONK_BLACKOUT_KICK,             FSBSpellType::Damage,   0.f,        0.f,            100.f,           2.f,           false,      4000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_MONK_LEG_SWEEP,                 FSBSpellType::Damage,   0.f,        0.f,            100.f,           6.f,           false,      60000,          FSB_RoleMask::FSB_ROLEMASK_ANY },

    //DPS
    { SPELL_MONK_VIVIFY,                    FSBSpellType::Heal,     0.3f,       50.f,           100.f,           40.f,          false,      2000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_BLACK_OX_BREW,             FSBSpellType::Heal,     0.f,        30.f,           50.f,            0.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_CELESTIAL_INFUSION,        FSBSpellType::Heal,     0.f,        70.f,           50.f,            0.f,           true,       45000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_CELESTIAL_BREW,            FSBSpellType::Heal,     0.f,        30.f,           50.f,            0.f,           true,       45000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_EXPEL_HARM,                FSBSpellType::Heal,     0.15f,      40.f,           100.f,           0.f,           true,       15000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_PURIFYING_BREW,            FSBSpellType::Heal,     0.f,        70.f,           50.f,            0.f,           true,       10000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },

    // silence
    { SPELL_MONK_SPEAR_HAND_STRIKE,         FSBSpellType::Damage,   0.f,        0.f,            100.f,           2.f,           false,      15000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },

    // cc
    { SPELL_MONK_SPINNING_CRANE,            FSBSpellType::Damage,   0.4f,       0.f,            100.f,           6.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_BLACK_OX_STATUE,           FSBSpellType::Damage,   0.f,        0.f,            100.f,           2.f,           true,       10000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_RUSHING_JADE_WIND,         FSBSpellType::Damage,   0.f,        0.f,            100.f,           8.f,           false,      6000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    // dmg
    { SPELL_MONK_TIGER_PALM,                FSBSpellType::Damage,   0.5f,       0.f,            100.f,           2.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_INVOKE_NIUZAO,             FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_EXPLODING_KEG,             FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          false,      60000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_CHI_BURST,                 FSBSpellType::Damage,   0.f,        0.f,            100.f,           40.f,          false,      30000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_BREATH_FIRE,               FSBSpellType::Damage,   0.f,        0.f,            100.f,           2.f,           false,      15000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_MONK_KEG_SMASH,                 FSBSpellType::Damage,   0.f,        0.f,            100.f,           15.f,          false,      8000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
};

namespace FSBMonk
{
    void HandleOnSpellCast(Creature* bot, uint32 spellId)
    {
        switch (spellId)
        {
        case SPELL_MONK_PURIFYING_BREW:
            bot->ModifyHealth(bot->GetMaxHealth() * 0.08f);
            break;

        default:
            break;
        }
    }
}
