#include "followship_bots_paladin.h"

std::vector<FSBSpellDefinition> PaladinSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    //ANY
    { SPELL_PALADIN_DIVINE_SHIELD,      FSBSpellType::Heal,   0.f,         20.f,            100.f,           0.f,          true,      300000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PALADIN_BLESSING_PROTECTION,FSBSpellType::Heal,   0.f,         20.f,            100.f,           40.f,          false,      300000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PALADIN_HAMMER_OF_JUSTICE,  FSBSpellType::Damage,   0.f,        0.f,            100.f,           10.f,          false,      45000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PALADIN_HAND_OF_RECKONING,  FSBSpellType::Damage,   0.f,        0.f,            100.f,           30.f,          false,      8000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PALADIN_CRUSADER_STRIKE,    FSBSpellType::Damage,   0.f,        0.f,            100.f,           2.f,          false,      6000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PALADIN_JUDGEMENT,          FSBSpellType::Damage,   0.f,        0.f,            100.f,           30.f,          false,      11000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PALADIN_CONSECRATION,       FSBSpellType::Damage,   0.02f,        0.f,            100.f,           2.f,          true,      9000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PALADIN_FLASH_OF_LIGHT,     FSBSpellType::Heal,     0.1f,        70.f,           100.f,           40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    // HOLY
    { SPELL_PALADIN_HOLY_LIGHT,         FSBSpellType::Heal,     0.f,        50.f,            100.f,           40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    //{ SPELL_PALADIN_HOLY_SHOCK,         FSBSpellType::Heal,     0.f,        60.f,            100.f,           40.f,          false,      6000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },

    { SPELL_PALADIN_HOLY_SHOCK,         FSBSpellType::Damage,     0.f,        50.f,            100.f,           20.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },

    // TANK
    { SPELL_PALADIN_AVENGER_SHIELD,         FSBSpellType::Damage,     0.f,        0.f,            100.f,           30.f,          false,      15000,           FSB_RoleMask::FSB_ROLEMASK_TANK },

    // DPS
    { SPELL_PALADIN_DIVINE_STORM,         FSBSpellType::Damage,     0.f,        0.f,            100.f,           2.5f,          true,      15000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_PALADIN_AVENGING_WRATH,         FSBSpellType::Damage,     0.f,        0.f,            100.f,           0.f,          true,      120000,           FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
};

namespace FSBPaladin
{
    bool BotHasDevotionAura(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->HasAura(SPELL_PALADIN_DEVOTION_AURA))
            return true;

        return false;

    }

    void HandleOnSpellCast(Creature* bot, uint32 spellId)
    {
        switch (spellId)
        {
        case SPELL_PALADIN_JUDGEMENT:
        case SPELL_PALADIN_CRUSADER_STRIKE:
            //bot->ModifyPower(POWER_HOLY_POWER, 1, false);
            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB bot {} casted spell and will receive holy power. Total HP: {}", bot->GetName(), bot->GetPower(POWER_HOLY_POWER));
            break;

        default:
            break;
        }
    }
}
