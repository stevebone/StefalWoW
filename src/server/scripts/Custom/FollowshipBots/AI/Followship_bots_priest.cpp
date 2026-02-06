
#include "Followship_bots_priest.h"
#include "Followship_bots_utils_spells.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"

std::vector<FSBSpellDefinition> PriestSpellsTable =
{
    // Spell ID                         Spell Type              ManaCost %  HP % for heal   Chance           Dist/Range     SelfCast    Cooldown Ms     RoleMask
    { SPELL_PRIEST_DESPERATE_PRAYER,    FSBSpellType::Heal,     0.f,        20.f,           100.f,          0.f,            true,       90000,          FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_HEAL,                FSBSpellType::Heal,     0.f,        40.f,           80.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_PRAYER_OF_MENDING,   FSBSpellType::Heal,     0.f,        60.f,           50.f,           40.f,           false,      9000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_FLASH_HEAL,          FSBSpellType::Heal,     0.f,        70.f,           70.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_RENEW,               FSBSpellType::Heal,     0.f,        85.f,           60.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_PURIFY,              FSBSpellType::Heal,     0.f,        100.f,          70.f,           40.f,           false,      8000,           FSB_RoleMask::FSB_ROLEMASK_HEALER },
    { SPELL_PRIEST_POWER_WORD_SHIELD,   FSBSpellType::Heal,     0.f,         70.f,          70.f,           30.f,           false,      8000,           FSB_RoleMask::FSB_ROLEMASK_HEALER | FSB_RoleMask::FSB_ROLEMASK_ASSIST },

    { SPELL_PRIEST_PSYCHIC_SCREAM,      FSBSpellType::Damage,   0.f,        0.f,            80.f,           8.f,            false,       45000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE    },
    { SPELL_PRIEST_SHADOW_WORD_PAIN,    FSBSpellType::Damage,   0.f,        0.f,            80.f,           30.f,           false,      15000,          FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_VAMPIRIC_TOUCH,      FSBSpellType::Damage,   0.f,        0.f,            80.f,           30.f,           false,      15000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_MIND_BLAST,          FSBSpellType::Damage,   0.f,        0.f,            70.f,           30.f,           false,      9000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_HOLY_FIRE,           FSBSpellType::Damage,   0.f,        0.f,            60.f,           30.f,           false,      10000,          FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_PENANCE,             FSBSpellType::Damage,   0.2f,       0.f,            55.f,           40.f,           false,      9000,           FSB_RoleMask::FSB_ROLEMASK_ASSIST },
    { SPELL_PRIEST_SMITE,               FSBSpellType::Damage,   0.f,        0.f,            50.f,           40.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_ANY    },
    { SPELL_PRIEST_DEVOURING_PLAGUE,    FSBSpellType::Damage,   0.5f,       0.f,            40.f,           40.f,           false,      10000,          FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },
    { SPELL_PRIEST_MIND_FLAY,           FSBSpellType::Damage,   0.2f,       0.f,            30.f,           35.f,           false,      1000,           FSB_RoleMask::FSB_ROLEMASK_RANGED_DAMAGE },

};


namespace FSBPriest
{
    bool BotInitialCombatSpells(Creature* bot, uint32& _globalCooldownUntil, bool& _ownerWasInCombat, uint8& _appliedInitialCBuffs, bool selfCast)
    {
        Unit* target = bot;

        if (!selfCast)
        {
            Player* player = FSBMgr::GetBotOwner(bot);

            if (!player)
                return false;

            if (!player->IsAlive())
                return false;

            if (!player->IsInCombat())
                return false;

            target = player;
        }

        // we need to default this in case bot does not have owner.
        // otherwise bot will never buff itself when not hired.
        if (selfCast)
            _ownerWasInCombat = bot->IsInCombat(); 


        uint32 now = getMSTime();

        if (FSBUtilsSpells::CanCastNow(bot, now, _globalCooldownUntil) && !target->HasAura(SPELL_PRIEST_POWER_WORD_SHIELD) && _ownerWasInCombat && _appliedInitialCBuffs == 0)
        {
            bot->CastSpell(target, SPELL_PRIEST_POWER_WORD_SHIELD);
            _globalCooldownUntil = now + 1500;
            _appliedInitialCBuffs = 1; // pws applied

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat Spell Cast: PWS on target: {}", target->GetName());

            return true;
        }
        else if (FSBUtilsSpells::CanCastNow(bot, now, _globalCooldownUntil) && !target->HasAura(SPELL_PRIEST_RENEW) && _appliedInitialCBuffs == 1)
        {
            bot->CastSpell(target, SPELL_PRIEST_RENEW);
            _globalCooldownUntil = now + 1500;
            _appliedInitialCBuffs = 2;

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Initial Combat Spell Cast: Renew on target: {}", target->GetName());

            return true;
        }

        return false;

    }

    bool BotHealSelfOOC(Creature* bot, uint32& _globalCooldownUntil, uint32& _60secondsCheckMs, uint32& outSpellId)
    {
        uint32 now = getMSTime();

        if (FSBUtilsSpells::CanCastNow(bot, now, _globalCooldownUntil))
        {
            if (bot->GetHealthPct() < 30 && !bot->HasAura(SPELL_PRIEST_RECUPERATE) && now >= _60secondsCheckMs)
            {
                FSBUtilsMovement::StopFollow(bot);

                bot->CastSpell(bot, SPELL_PRIEST_RECUPERATE, false);
                _60secondsCheckMs = now + 60000; // 1 minute
                _globalCooldownUntil = now + 10000; // use 10s cooldown to not interrup duration of channel spell

                outSpellId = SPELL_PRIEST_RECUPERATE;

                TC_LOG_DEBUG("scripts.ai.core", "FSB Priest Out-of-combat: Recuperate");

                return true;
            }
            else if (bot->GetHealthPct() < 80)
            {
                FSBUtilsMovement::StopFollow(bot);

                bot->CastSpell(bot, SPELL_PRIEST_HEAL, false);
                _globalCooldownUntil = now + 1500;

                outSpellId = SPELL_PRIEST_HEAL;

                TC_LOG_DEBUG("scripts.ai.core", "FSB Priest Out-of-combat: Heal");

                return true;
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
}
