
#include "Followship_bots_priest.h"
#include "Followship_bots_utils_spells.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"

namespace FSB_PriestActionsSpells
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

    bool BotHealPlayerOOC(Creature* bot, uint32& _globalCooldownUntil, uint32& outSpellId)
    {
        uint32 now = getMSTime();

        Player* player = FSBMgr::GetBotOwner(bot);
        if (!player)
            return false;

        if (player->GetHealthPct() <= 50)
        {
            

            FSBUtilsMovement::StopFollow(bot);
            bot->CastSpell(player, SPELL_PRIEST_HEAL, false);
            _globalCooldownUntil = now + 1500;

            outSpellId = SPELL_PRIEST_HEAL;

            TC_LOG_DEBUG("scripts.ai.core", "FSB Out-of-combat: Player Heal < 50");

            return true;

        }
        else if (player->GetHealthPct() <= 70)
        {
            FSBUtilsMovement::StopFollow(bot);
            bot->CastSpell(player, SPELL_PRIEST_FLASH_HEAL, false);
            _globalCooldownUntil = now + 1500;

            outSpellId = SPELL_PRIEST_FLASH_HEAL;

            TC_LOG_DEBUG("scripts.ai.core", "FSB Out-of-combat: Player Heal < 70");

            return true;
        }

        return false;
    }
}
