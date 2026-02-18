
#include "Followship_bots_priest.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_group_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_spells_handler.h"

std::vector<FSBSpellDefinition> PriestSpellsTable =
{
    // Spell ID                             Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask

    // ANY
    { SPELL_HUMAN_WILL_TO_SURVIVE,          FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       180000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DWARF_STONEFORM,                FSBSpellType::Heal,     0.f,        80.f,           100.f,           0.f,           true,       120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },
    { SPELL_DRAENEI_GIFT_NAARU,             FSBSpellType::Heal,     0.f,        50.f,           100.f,           30.f,          false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ANY },

    { SPELL_PRIEST_RENEW,               FSBSpellType::Heal,     0.f,        85.f,           75.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_FLASH_HEAL,          FSBSpellType::Heal,     0.f,        70.f,           70.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },

    { SPELL_PRIEST_SHADOW_WORD_PAIN,    FSBSpellType::Damage,   0.f,        0.f,            80.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_SMITE,               FSBSpellType::Damage,   0.f,        0.f,            50.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },
    
    // Healer
    { SPELL_PRIEST_PAIN_SUPPRESSION,    FSBSpellType::Heal,     0.f,        50.f,           80.f,           40.f,           false,      180000,          FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_POWER_WORD_RADIANCE, FSBSpellType::Heal,     0.f,        40.f,           80.f,           40.f,           false,      18000,          FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_DESPERATE_PRAYER,    FSBSpellType::Heal,     0.f,        20.f,           100.f,          0.f,            true,       90000,          FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_HEAL,                FSBSpellType::Heal,     0.f,        40.f,           80.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_PRAYER_HEALING,      FSBSpellType::Heal,     0.f,        60.f,           70.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_PRAYER_OF_MENDING,   FSBSpellType::Heal,     0.f,        70.f,           50.f,           40.f,           false,      12000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_HOLY_WORD_SERENITY,  FSBSpellType::Heal,     0.f,        60.f,          70.f,           40.f,           false,      60000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_HOLY_WORD_SANCTIFY,  FSBSpellType::Heal,     0.f,        40.f,          70.f,           40.f,           false,      60000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_GUARDIAN_SPIRIT,     FSBSpellType::Heal,     0.f,        30.f,          70.f,           40.f,           false,      180000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_DIVINE_HYMN,         FSBSpellType::Heal,     0.f,        30.f,          70.f,           40.f,           false,      180000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_POWER_WORD_SHIELD,   FSBSpellType::Heal,     0.f,        70.f,          70.f,           30.f,           false,      8000,           FSB_RoleMask::FSB_ROLEMASK_HEALER | FSB_RoleMask::FSB_ROLEMASK_ASSIST },

    { SPELL_PRIEST_HOLY_WORD_CHASTISE,  FSBSpellType::Damage,   0.f,        0.f,          70.f,           30.f,           false,      60000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },

    // Shadow
    { SPELL_PRIEST_VAMPIRIC_EMBRACE,    FSBSpellType::Heal ,    0.f,        50.f,           100.f,          0.f,           true,      120000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_DISPERSION,          FSBSpellType::Heal ,    0.f,        30.f,           100.f,          0.f,           true,      120000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },

    { SPELL_PRIEST_SHADOWFIEND,         FSBSpellType::Damage,   0.f,        0.f,            50.f,          40.f,           false,      180000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_SILENCE,             FSBSpellType::Damage,   0.f,        0.f,            100.f,          40.f,           false,      30000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_SHADOW_WORD_DEATH,   FSBSpellType::Damage,   0.f,        0.f,            70.f,           40.f,           false,      10000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_MIND_BLAST,          FSBSpellType::Damage,   0.f,        0.f,            70.f,           40.f,           false,      9000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_MIND_FLAY,           FSBSpellType::Damage,   0.2f,       0.f,            65.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_VOIDFORM,            FSBSpellType::Damage,   0.f,        0.f,            60.f,           40.f,           false,      120000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_PSYCHIC_SCREAM,      FSBSpellType::Damage,   0.f,        0.f,            80.f,           8.f,            false,       45000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE    },
    { SPELL_PRIEST_VOID_TORRENT,        FSBSpellType::Damage,   0.f,        0.f,            50.f,           40.f,           false,      30000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_VAMPIRIC_TOUCH,      FSBSpellType::Damage,   0.2f,        0.f,            80.f,           40.f,           false,     1000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_TENTACLE_SLAM,       FSBSpellType::Damage,   0.f,        0.f,            50.f,           40.f,           false,      15000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    
    
    
    
    { SPELL_PRIEST_DEVOURING_PLAGUE,    FSBSpellType::Damage,   0.5f,       0.f,            40.f,           40.f,           false,      10000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    

    // Hybrid
    { SPELL_PRIEST_POWER_INFUSION,      FSBSpellType::Heal,     0.f,        80.f,           80.f,           40.f,           false,      120000,         FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_PLEA,                FSBSpellType::Heal,     0.f,        70.f,           80.f,           40.f,           false,      9000,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_PENANCE,             FSBSpellType::Heal,     0.f,        60.f,           80.f,           40.f,           false,      9000,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_PAIN_SUPPRESSION,    FSBSpellType::Heal,     0.f,        50.f,           80.f,           40.f,           false,      180000,         FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_POWER_WORD_RADIANCE, FSBSpellType::Heal,     0.f,        40.f,           80.f,           40.f,           false,      18000,          FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_POWER_WORD_BARRIER,  FSBSpellType::Heal,     0.f,        30.f,           80.f,           40.f,           false,      180000,         FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_DESPERATE_PRAYER,    FSBSpellType::Heal,     0.f,        20.f,           100.f,          0.f,            true,       90000,          FSB_RoleMask::FSB_ROLEMASK_ASSIST },

    { SPELL_PRIEST_ULTIMATE_PENITENCE,  FSBSpellType::Damage,   0.f,       0.f,            80.f,           40.f,           false,       240000,         FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_HOLY_FIRE,           FSBSpellType::Damage,   0.f,       0.f,            60.f,           40.f,           false,       10000,          FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_PENANCE,             FSBSpellType::Damage,   0.f,       0.f,            55.f,           40.f,           false,       9000,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_HOLY_NOVA,           FSBSpellType::Damage,   0.f,       0.f,            80.f,           6.f,            true,        1000,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },
};


namespace FSBPriest
{
    bool BotInitialCombatSpells(Creature* bot, uint32& globalCooldown, bool& botCastedCombatBuffs, FSB_Roles botRole, const std::vector<Unit*>& botGroup)
    {
        if (botCastedCombatBuffs)
            return false;

        if (!bot || !bot->IsAlive())
            return false;

        if (!bot->IsInCombat())
            return false;

        Unit* target = nullptr;
        Unit* tank = FSBGroup::BotGetFirstGroupTank(botGroup);

        switch (botRole)
        {
        case FSB_ROLE_HEALER:
            if (tank)
                target = tank;
            else
            {
                Player* player = FSBMgr::Get()->GetBotOwner(bot);

                if (!player)
                    break;

                if (!player->IsAlive())
                    break;

                if (!player->IsInCombat())
                    break;

                target = player;
            }
            break;
        case FSB_ROLE_ASSIST:
            target = bot;
            break;
        default:
            target = bot;
            break;
        }

        if (target)
        {
            uint32 now = getMSTime();

            if (!target->HasAura(SPELL_PRIEST_POWER_WORD_SHIELD))
            {
                if (FSBSpells::BotCastSpell(bot, SPELL_PRIEST_POWER_WORD_SHIELD, target))
                {
                    globalCooldown = now + 1500;
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat Spell Cast: PWS on target: {}", target->GetName());
                    return true;
                }
            }
            else if(!target->HasAura(SPELL_PRIEST_RENEW))
            {
                if (FSBSpells::BotCastSpell(bot, SPELL_PRIEST_RENEW, target))
                {
                    globalCooldown = now + 1500;
                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat Spell Cast: Renew on target: {}", target->GetName());
                    botCastedCombatBuffs = true;
                    return true;
                }
            }
        }
        return false;

    }

    bool BotOOCHealOwner(Creature* bot, Player* player, uint32& globalCooldown)
    {
        if (!player || !bot)
            return false;

        uint32 now = getMSTime();

        if (player->GetHealthPct() <= 50)
        {
            bot->CastSpell(player, SPELL_PRIEST_HEAL, false);
            globalCooldown = now + 1500;

            TC_LOG_DEBUG("scripts.ai.core", "FSB Out-of-combat: Bot: {} Player Heal < 50", bot->GetName());

            return true;

        }
        else if (player->GetHealthPct() <= 90)
        {
            bot->CastSpell(player, SPELL_PRIEST_FLASH_HEAL, false);
            globalCooldown = now + 1500;

            TC_LOG_DEBUG("scripts.ai.core", "FSB Out-of-combat: Bot: {} Player Heal < 70", bot->GetName());

            return true;
        }

        return false;
    }

    bool BotHasPainSuppression(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->HasAura(SPELL_PRIEST_PAIN_SUPPRESSION))
            return true;

        return false;

    }
}
