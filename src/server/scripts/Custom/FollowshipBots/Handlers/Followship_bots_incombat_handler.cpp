#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_warlock.h"

#include "Followship_bots_combat_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_incombat_handler.h"

namespace FSBIC
{
    bool BotICActions(Creature* bot, bool& botManaPotionUsed, bool& botHealthPotionUsed, uint32& globalCooldown, bool& botCastedCombatBuffs, const std::vector<Unit*>& botGroup)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!bot->IsInCombat())
            return false;

        if (bot->HasUnitState(UNIT_STATE_CASTING))
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto& resTargetGuid = baseAI->botResurrectTargetGuid;

        //1. IC Potions
        // These can be cast instant with no GCD
        if (BotICPotions(bot, botManaPotionUsed, botHealthPotionUsed))
            return true;

        //2. IC (initial)Buffs
        // These are cast when combat starts
        if (BotICInitialBuffs(bot, globalCooldown, botCastedCombatBuffs, botGroup))
            return true;

        //3. IC Resurrect - Druid
        if (BotICResurrect(bot, resTargetGuid))
            return true;

        //4. IC Go melee when OOM
        if (BotICMeleeMode(bot))
            return true;

        return false;
    }

    bool BotICMeleeMode(Creature* bot)
    {
        if (!bot)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto& meleeMode = baseAI->botMeleeMode;

        if (FSBCombat::ShouldSwitchToMelee(bot))
        {
            meleeMode = true;
            FSBCombat::EnterMeleeMode(bot);
            return true;
        }
        else meleeMode = false;

        return false;
    }

    bool BotICInitialBuffs(Creature* bot, uint32 globalCooldown, bool& botCastedCombatBuffs, const std::vector<Unit*>& botGroup)
    {
        if (!bot)
            return false;

        uint32 now = getMSTime();

        if (!FSBUtilsSpells::CanCastNow(bot, now, globalCooldown))
            return false;

        if (!bot->IsInCombat())
            return false;

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        FSB_Roles botRole = FSBUtils::GetRole(bot);

        switch (botClass)
        {
        case FSB_Class::Priest:
            if(FSBPriest::BotInitialCombatSpells(bot, globalCooldown, botCastedCombatBuffs, botRole, botGroup))
                return true;
            break;
        case FSB_Class::Druid:
            if (FSBDruid::BotInitialCombatSpells(bot, globalCooldown, botCastedCombatBuffs, botRole, botGroup))
                return true;
            break;
        default:
            return false;
        }

        return false;
    }

    bool BotICPotions(Creature* bot, bool& botManaPotionUsed, bool& botHealthPotionUsed)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!bot->IsInCombat())
            return false;

        if (bot->HasUnitState(UNIT_STATE_CASTING))
            return false;

        if (!FollowshipBotsConfig::configFSBUseICPotions)
            return false;

        // 1. Generic mana potions for bots with mana
        if (bot->GetPowerType() == POWER_MANA && bot->GetPowerPct(POWER_MANA) < BOT_IC_THRESHOLD_POTION_MP)
        {
            if (!botManaPotionUsed)
            {
                uint32 ManaPotionSpellId = FSBSpellsUtils::GetManaPotionSpellForLevel(bot->GetLevel());

                if (ManaPotionSpellId)
                {
                    // Global Cooldown does NOT apply for potions
                    // Limit of 1 potion per type (MP or HP) per combat 
                    bot->CastSpell(bot, ManaPotionSpellId, false);
                    botManaPotionUsed = true;

                    if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                    {
                        std::string spellName = FSBSpellsUtils::GetSpellName(ManaPotionSpellId);
                        std::string msg = FSBUtilsTexts::BuildNPCSayText("", NULL, FSBSayType::CombatMana, spellName);
                        bot->Say(msg, LANG_UNIVERSAL);
                    }

                    TC_LOG_DEBUG("scripts.ai.fsb", "FSB: IC Action mana potion used by bot: {} with spell id: {}", bot->GetName(), ManaPotionSpellId);
                    return true;
                }
            }

        }

        // 2. Generic health potions for non healer bots 
        if (bot->GetHealthPct() < BOT_IC_THRESHOLD_POTION_HP)
        {
            if (FSBUtils::BotIsHealerClass(bot) || botHealthPotionUsed)
                return false;

            uint32 HealthPotionSpellId = 0;
            FSB_Class cls = FSBUtils::GetBotClassForEntry(bot->GetEntry());

            if (cls == FSB_Class::Warlock)
            {
                HealthPotionSpellId = SPELL_WARLOCK_HEALTHSTONE;
            }
            else HealthPotionSpellId = FSBSpellsUtils::GetHealthPotionSpellForLevel(bot->GetLevel());

            bot->CastSpell(bot, HealthPotionSpellId, false);
            botHealthPotionUsed = true;

            std::string spellName = FSBSpellsUtils::GetSpellName(HealthPotionSpellId);
            if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
            {
                std::string msg = FSBUtilsTexts::BuildNPCSayText("", NULL, FSBSayType::CombatHealth, spellName);
                bot->Say(msg, LANG_UNIVERSAL);
            }

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: IC Action health potion used by bot: {} with spell: {}", bot->GetName(), spellName);
            return true;
        }
        return false;
    }

    bool BotICResurrect(Creature* bot, ObjectGuid& resTargetGuid)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!resTargetGuid)
            return false;

        FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_RESURRECT_TARGET, 3s, 5s);

        return true;
    }
}
