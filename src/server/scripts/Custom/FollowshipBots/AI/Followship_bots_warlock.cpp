#include "Followship_bots_warlock.h"
#include "Followship_bots_utils.h"

std::vector<FSBSpellDefinition> WarlockSpellsTable =
{
    // Spell                                Spell                   Mana        HP % for    Chance        Dist/Range     SelfCast    Cooldown Ms     RoleMask
    // ID                                   Type                    Cost %      Heal

    //ANY
    { SPELL_WARLOCK_GIFT_NAARU,             FSBSpellType::Heal,     0.f,        50.f,       100.f,        40.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARLOCK_UNENDING_RESOLVE,       FSBSpellType::Damage,   0.f,        0.f,        50.f,         2.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARLOCK_SHADOW_BOLT,            FSBSpellType::Damage,   0.f,        0.f,        90.f,         40.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_WARLOCK_FEAR,                   FSBSpellType::Damage,   0.f,        0.f,        80.f,         20.f,          false,      8000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_WARLOCK_DRAIN_LIFE,             FSBSpellType::Damage,   0.f,        0.f,        80.f,         20.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    //{ SPELL_PALADIN_CONSECRATION,           FSBSpellType::Damage,   0.02f,      0.f,        100.f,        2.f,           true,       9000,           FSB_RoleMask::FSB_ROLEMASK_ANY },
    //{ SPELL_PALADIN_CRUSADER_STRIKE,        FSBSpellType::Damage,   0.f,        0.f,        90.f,         2.f,           false,      3000,           FSB_RoleMask::FSB_ROLEMASK_ANY },

    // AFFLICTION
    { SPELL_WARLOCK_CORRUPTION,               FSBSpellType::Damage,   0.f,        0.f,        90.f,         40.f,          false,      1000,         FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },
    { SPELL_WARLOCK_AGONY,                    FSBSpellType::Damage,   0.f,        0.f,        90.f,         40.f,          false,      1000,         FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },
    { SPELL_WARLOCK_CURSE_WEAKNESS,           FSBSpellType::Damage,   0.f,        0.f,        90.f,         40.f,          false,      1000,         FSB_RoleMask::FSB_ROLEMASK_RANGED_AFFLICTION },
    //{ SPELL_PALADIN_BLESSING_SACRIFICE,     FSBSpellType::Heal,     0.f,        60.f,       50.f,         40.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_HEALER },

    //{ SPELL_PALADIN_DIVINE_PROTECTION,      FSBSpellType::Heal,     0.f,        70.f,       50.f,         0.f,           true,       60000,          FSB_RoleMask::FSB_ROLEMASK_HEALER },
    //{ SPELL_PALADIN_SEAL_OF_THE_CRUSADER,   FSBSpellType::Heal,     0.05f,      60.f,       60.f,         40.f,          false,      12000,          FSB_RoleMask::FSB_ROLEMASK_HEALER },

    //{ SPELL_PALADIN_HOLY_SHOCK,             FSBSpellType::Damage,   0.f,        0.f,        90.f,         20.f,          false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },

    // TANK
    //{ SPELL_PALADIN_BLESSING_PROTECTION,    FSBSpellType::Heal,     0.f,        20.f,       100.f,        40.f,          false,      300000,         FSB_RoleMask::FSB_ROLEMASK_TANK },
    //{ SPELL_PALADIN_BLESSING_SALVATION,     FSBSpellType::Heal,     0.02f,      30.f,       100.f,        40.f,          false,      60000,          FSB_RoleMask::FSB_ROLEMASK_TANK },
    //{ SPELL_PALADIN_GUARDIAN_ANCIENT_KINGS, FSBSpellType::Heal,     0.02f,      60.f,       50.f,         0.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_TANK },

    //{ SPELL_PALADIN_REBUKE,                 FSBSpellType::Damage,   0.01f,      0.f,        100.f,        2.f,           false,      15000,          FSB_RoleMask::FSB_ROLEMASK_TANK },
    //{ SPELL_PALADIN_HAND_OF_RECKONING,      FSBSpellType::Damage,   0.f,        0.f,        100.f,        30.f,          false,      8000,           FSB_RoleMask::FSB_ROLEMASK_TANK },

    //{ SPELL_PALADIN_AVENGER_SHIELD,         FSBSpellType::Damage,   0.f,        0.f,        90.f,         30.f,          false,      15000,          FSB_RoleMask::FSB_ROLEMASK_TANK },
    //{ SPELL_PALADIN_HOLY_SHIELD,            FSBSpellType::Damage,   0.f,        0.f,        90.f,         2.f,           true,       10000,          FSB_RoleMask::FSB_ROLEMASK_TANK },
    //{ SPELL_PALADIN_BLESSED_HAMMER,         FSBSpellType::Damage,   0.01f,      0.f,        90.f,         40.f,          false,      3000,           FSB_RoleMask::FSB_ROLEMASK_TANK },
    //{ SPELL_PALADIN_HAMMER_OF_THE_RIGHT,    FSBSpellType::Damage,   0.f,        0.f,        90.f,         2.f,           false,      3000,           FSB_RoleMask::FSB_ROLEMASK_TANK },

    // DPS
    //{ SPELL_PALADIN_EXECUTION_SENTENCE,     FSBSpellType::Damage,   0.04f,      0.f,        100.f,        30.f,          false,      60000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    //{ SPELL_PALADIN_DIVINE_STORM,           FSBSpellType::Damage,   0.f,        0.f,        100.f,        2.5f,          true,       15000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    //{ SPELL_PALADIN_EXORCISM,               FSBSpellType::Damage,   0.02f,      0.f,        90.f,         30.f,          false,      15000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    //{ SPELL_PALADIN_HAMMER_OF_WRATH,        FSBSpellType::Damage,   0.04f,      0.f,        90.f,         40.f,          false,      12000,          FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    //{ SPELL_PALADIN_AVENGING_WRATH,         FSBSpellType::Damage,   0.f,        0.f,        50.f,         2.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },

};

namespace FSBWarlock
{
    bool BotSummonRandomDemon(Creature* bot, uint32& globalCooldown, bool& demonDead)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!demonDead)
            return false;

        FSB_Roles role = FSBUtils::GetRole(bot);

        uint32 spellId = SPELL_WARLOCK_SUMMON_IMP;

        if(role == FSB_ROLE_RANGED_DEMONOLOGY)
            spellId = RAND(SPELL_WARLOCK_SUMMON_IMP, SPELL_WARLOCK_SUMMON_SUCCUBUS, SPELL_WARLOCK_SUMMON_VOIDWALKER, SPELL_WARLOCK_SUMMON_FELHUNTER);

        //bot->SetPowerType(POWER_SOUL_SHARDS, true);
        //bot->SetMaxPower(POWER_SOUL_SHARDS, 50);
        //bot->SetPower(POWER_SOUL_SHARDS, 50, true);

        SpellCastResult result = bot->CastSpell(bot, spellId, false);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB BotSummonRandomDemon result {} for bot {}", result, bot->GetName());

        

        if (result == SPELL_CAST_OK)
        {
            uint32 now = getMSTime();

            //demonDead = false;
            globalCooldown = now + 1500;
            

            return true;
        }

        return false;
    }

    bool BotHasUnendingResolve(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->HasAura(SPELL_WARLOCK_UNENDING_RESOLVE))
            return true;

        return false;

    }
}
