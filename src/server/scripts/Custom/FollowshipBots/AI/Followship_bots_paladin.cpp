#include "followship_bots_paladin.h"

std::vector<FSBSpellDefinition> PaladinSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    //ANY
    { SPELL_PALADIN_CRUSADER_STRIKE,    FSBSpellType::Damage,   0.f,        0.f,            100.f,           2.f,          false,      6000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PALADIN_JUDGEMENT,          FSBSpellType::Damage,   0.f,        0.f,            100.f,           30.f,          false,      11000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_PALADIN_CONSECRATION,          FSBSpellType::Damage,   0.f,        0.f,            100.f,           2.f,          true,      9000,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    // HOLY
    { SPELL_PALADIN_HOLY_LIGHT,         FSBSpellType::Heal,     0.f,        50.f,            100.f,           40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    //{ SPELL_PALADIN_HOLY_SHOCK,         FSBSpellType::Heal,     0.f,        60.f,            100.f,           40.f,          false,      6000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },

    { SPELL_PALADIN_HOLY_SHOCK,         FSBSpellType::Damage,     0.f,        50.f,            100.f,           20.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },

};

namespace FSBPaladin
{
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
