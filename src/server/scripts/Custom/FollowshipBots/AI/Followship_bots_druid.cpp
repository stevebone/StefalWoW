#include "Followship_bots_stats_handler.h"

#include "Followship_bots_druid.h"


std::vector<FSBSpellDefinition> DruidSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_DRUID_WRATH,                FSBSpellType::Damage,   0.f,        0.f,            100.f,            40.f,            false,       1000,        FSB_RoleMask::FSB_ROLEMASK_ANY },

    // TANK
    { SPELL_DRUID_BEAR_THRASH,               FSBSpellType::Damage,   0.f,        0.f,            100.f,            8.f,             false,       6000,        FSB_RoleMask::FSB_ROLEMASK_TANK },

    // MELEE
    { SPELL_DRUID_CAT_THRASH,               FSBSpellType::Damage,   0.2f,        0.f,            100.f,            8.f,             false,       6000,        FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },
    { SPELL_DRUID_CAT_SHRED,               FSBSpellType::Damage,   0.4f,        0.f,            100.f,            2.5f,             false,       1000,        FSB_RoleMask::FSB_ROLEMASK_MELEE_DAMAGE },

};

namespace FSBDruid
{
    bool BotOnAuraApplied(Creature* bot, AuraApplication const* aurApp, bool applied)
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
        default:
            return false;
        }
    }

    void HandleOnSpellCast(Creature* bot, uint32 spellId)
    {
        switch (spellId)
        {
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
}
