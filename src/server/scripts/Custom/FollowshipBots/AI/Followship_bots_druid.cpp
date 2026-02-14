#include "Followship_bots_stats_handler.h"

#include "Followship_bots_druid.h"


std::vector<FSBSpellDefinition> DruidSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_DRUID_WRATH,                FSBSpellType::Damage,   0.f,        0.f,            100.f,            40.f,            false,       1000,        FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DRUID_BARKSKIN,             FSBSpellType::Heal,     0.f,        60.f,           100.f,            0.f,             true,        60000,       FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DRUID_REGROWTH,             FSBSpellType::Heal,     0.f,        70.f,           100.f,            30.f,            false,       1000,        FSB_RoleMask::FSB_ROLEMASK_ANY },

    // TANK
    { SPELL_DRUID_BEAR_GROWL,           FSBSpellType::Damage,   0.f,        0.f,            100.f,            40.f,             false,       8000,        FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_DRUID_BEAR_THRASH,          FSBSpellType::Damage,   0.f,        0.f,            100.f,            8.f,             false,        6000,        FSB_RoleMask::FSB_ROLEMASK_TANK },
    { SPELL_DRUID_BEAR_MANGLE,          FSBSpellType::Damage,   0.f,        0.f,            100.f,            2.5f,             false,       6000,        FSB_RoleMask::FSB_ROLEMASK_TANK },

    // MELEE
    { SPELL_DRUID_CAT_THRASH,           FSBSpellType::Damage,   0.2f,        0.f,            100.f,            8.f,             false,       6000,        FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_DRUID_CAT_SHRED,            FSBSpellType::Damage,   0.4f,        0.f,            100.f,            2.5f,             false,      1000,        FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_DRUID_CAT_MANGLE,           FSBSpellType::Damage,   0.4f,        0.f,            100.f,            2.5f,             false,      6000,        FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_DRUID_CAT_FEROCIOUS_BITE,   FSBSpellType::Damage,   0.25f,       0.f,            100.f,            2.5f,             false,      6000,        FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },

    // RANGED
    { SPELL_DRUID_SOLAR_BEAM,           FSBSpellType::Damage,   0.f,        0.f,             100.f,            40.f,            false,       60000,       FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_DRUID_WILD_MUSHROOM,        FSBSpellType::Damage,   0.05f,      0.f,             100.f,            40.f,            false,       30000,       FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_DRUID_NEW_MOON,             FSBSpellType::Damage,   0.f,        0.f,             100.f,            40.f,            false,       20000,       FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_DRUID_STARFALL,             FSBSpellType::Damage,   0.1f,       0.f,             100.f,            40.f,            false,       8000,        FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_DRUID_ROOTS,                FSBSpellType::Damage,   0.f,        0.f,             100.f,            30.f,            false,       1000,        FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_DRUID_MOONFIRE,             FSBSpellType::Damage,   0.f,        0.f,             100.f,            30.f,            false,       1000,        FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_DRUID_CELESTIAL_ALIGNMENT,  FSBSpellType::Damage,   0.f,        0.f,             50.f,             40.f,            true,        180000,      FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },

    // HEALER
    { SPELL_DRUID_NATURE_CURE,          FSBSpellType::Heal,     0.f,        100.f,           100.f,            40.f,           false,        8000,        FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_DRUID_REJUVENATION,         FSBSpellType::Heal,     0.f,        80.f,            100.f,            40.f,           false,        1000,        FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_DRUID_TREE_OF_LIFE,         FSBSpellType::Heal,     0.f,        50.f,            50.f,             0.f,            true,         180000,      FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_DRUID_IRONBARK,             FSBSpellType::Heal,     0.f,        50.f,            50.f,             0.f,            true,         90000,       FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_DRUID_LIFEBLOOM,            FSBSpellType::Heal,     0.f,        50.f,            50.f,             40.f,           false,        1000,        FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_DRUID_SWIFTMEND,            FSBSpellType::Heal,     0.f,        30.f,            50.f,             40.f,           false,        15000,       FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_DRUID_TRANQUILITY,          FSBSpellType::Heal,     0.f,        30.f,            50.f,             40.f,           true,         180000,      FSB_RoleMask::FSB_ROLEMASK_HEALER },
};

namespace FSBDruid
{
    bool BotOnAuraApplied(Creature* bot, AuraApplication const* aurApp, bool applied, FSBBotStats& botStats)
    {
        if (!bot)
            return false;

        if (!aurApp || !aurApp->GetBase())
            return false;

        switch (aurApp->GetBase()->GetId())
        {
        case SPELL_DRUID_BEAR:
        {
            float hpPct = bot->GetPctModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT);
            float armorPct = bot->GetPctModifierValue(UNIT_MOD_ARMOR, TOTAL_PCT);

            if (applied)
            {
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, hpPct + 0.25f);
                bot->SetStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, armorPct + 2.2f);
            }
            else if (!applied)
            {
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, hpPct - 0.25f);
                bot->SetStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, armorPct - 2.2f);
            }
            FSBStats::RecalculateStats(bot, false, false);
            return true;
        }
        case SPELL_DRUID_TREE_OF_LIFE:
        {
            float armorPct = bot->GetPctModifierValue(UNIT_MOD_ARMOR, TOTAL_PCT);

            if (applied)
            {
                botStats.spellPowerPct += 0.1f;
                bot->SetStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, armorPct + 1.2f);
            }

            if (!applied)
            {
                botStats.spellPowerPct -= 0.1f;
                bot->SetStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, armorPct - 1.2f);
            }
            FSBStats::RecalculateStats(bot, false, false);
            return true;
        }
        case SPELL_DRUID_MOONKIN:
        {
            float armorPct = bot->GetPctModifierValue(UNIT_MOD_ARMOR, TOTAL_PCT);

            if (applied)
            {
                botStats.spellPowerPct += 0.1f;
                bot->SetStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, armorPct + 1.25f);
            }

            if (!applied)
            {
                botStats.spellPowerPct -= 0.1f;
                bot->SetStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, armorPct - 1.25f);
            }
            FSBStats::RecalculateStats(bot, false, false);
            return true;
        }
        default:
            return false;
        }
    }

    void HandleOnSpellCast(Creature* bot, uint32 spellId)
    {
        switch (spellId)
        {
        case SPELL_DRUID_BEAR_MANGLE:
            bot->ModifyPower(POWER_RAGE, 120, false);
            break;
        case SPELL_DRUID_BEAR_THRASH:
            bot->ModifyPower(POWER_RAGE, 60, false);
            break;

        default:
            break;
        }
    }

    void BotSetRoleAuras(Creature* bot, FSB_Roles role)
    {
        if (!bot)
            return;

        switch (role)
        {
        case FSB_ROLE_TANK:
        {
            for (uint32 spellId : druidAurasToRemoveForTank)
            {
                if (bot->HasAura(spellId))
                    bot->RemoveAurasDueToSpell(spellId);
            }

            if (!bot->HasAura(SPELL_DRUID_BEAR))
                bot->CastSpell(bot, SPELL_DRUID_BEAR);
            break;
        }
        case FSB_ROLE_HEALER:
        {
            for (uint32 spellId : druidAurasToRemoveForHealer)
            {
                if (bot->HasAura(spellId))
                    bot->RemoveAurasDueToSpell(spellId);
            }

            if (!bot->HasAura(SPELL_DRUID_TREANT))
                bot->CastSpell(bot, SPELL_DRUID_TREANT);
            break;
        }
        case FSB_ROLE_MELEE_DAMAGE:
        {
            for (uint32 spellId : druidAurasToRemoveForMelee)
            {
                if (bot->HasAura(spellId))
                    bot->RemoveAurasDueToSpell(spellId);
            }

            if (!bot->HasAura(SPELL_DRUID_CAT))
                bot->CastSpell(bot, SPELL_DRUID_CAT);
            break;
        }
        case FSB_ROLE_RANGED_DAMAGE:
        {
            for (uint32 spellId : druidAurasToRemoveForDamage)
            {
                if (bot->HasAura(spellId))
                    bot->RemoveAurasDueToSpell(spellId);
            }

            if (!bot->HasAura(SPELL_DRUID_MOONKIN))
                bot->CastSpell(bot, SPELL_DRUID_MOONKIN);
            break;
        }
        default:
            break;
        }

        FSBStats::RecalculateStats(bot, false, true);
    }

    bool BotHasMarkWild(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->HasAura(SPELL_DRUID_MARK_WILD))
            return true;

        return false;

    }

    bool BotHasIronbark(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->HasAura(SPELL_DRUID_IRONBARK))
            return true;

        return false;

    }
}
