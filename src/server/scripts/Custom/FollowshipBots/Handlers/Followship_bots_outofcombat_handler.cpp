#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_utils_combat.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_outofcombat_handler.h"

#include "Followship_bots_mage.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_warrior.h"


namespace FSBOOC
{
    bool BotOOCActions(Creature* bot, uint32& globalCooldown, uint32& buffTimer, uint32& selfBuffTimer, std::vector<Unit*> botGroup)
    {
        if (!bot)
            return false;

        if (!bot->IsAlive())
            return false;

        if (FSBUtilsCombat::IsCombatActive(bot))
            return false;

        uint32 now = getMSTime();

        if (!FSBUtilsSpells::CanCastNow(bot, now, globalCooldown))
            return false;

        //1. Bot Heal Owner
        // If we heal owner this turn return and end tick
        Player* player = FSBMgr::GetBotOwner(bot);
        if (player && BotOOCHealOwner(bot, player, globalCooldown))
            return true;

        //2. Buff Group (bots and player)
        // If we buff someone this turn return and end tick
        if (now >= buffTimer && BotOOCBuffGroup(bot, botGroup, buffTimer, globalCooldown))
            return true;

        //3. Buff Self - buffs only for the bot
        // If we buff something this turn return and end tick
        if (now >= selfBuffTimer && BotOOCBuffSelf(bot, selfBuffTimer, globalCooldown))
            return true;

        return false; 
    }

    bool BotOOCHealOwner(Creature* bot, Player* player, uint32 globalCooldown)
    {
        if (!bot || !player || !player->IsAlive())
            return false;

        float pHealth = player->GetHealthPct();

        if (pHealth > 90)
            return false;

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());

        bool check = false;

        FSBUtilsMovement::StopFollow(bot);

        switch (botClass)
        {
        case FSB_Class::Priest:
            if (FSBPriest::BotOOCHealOwner(bot, player, globalCooldown))
                check = true;
            break;
        case FSB_Class::Druid:
            break;
        case FSB_Class::Paladin:
            if (FSBPaladin::BotOOCHealOwner(bot, player, globalCooldown))
                check = true;
            break;
        default:
            break;
        }

        if (check)
        {
            if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                bot->Say(FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::HealTarget, ""), LANG_UNIVERSAL);

            return true;
        }

        return false;
    }

    bool BotOOCBuffSelf(Creature* bot, uint32& selfBuffTimer, uint32& globalCooldown)
    {
        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        uint32 buffSpellId = 0;
        bool check = false;

        switch (botClass)
        {
        case FSB_Class::Paladin:
            if (FSBPaladin::BotOOCBuffSelf(bot, globalCooldown, selfBuffTimer, buffSpellId))
                check = true;
            break;
        default:
            return false;
        }

        if (check)
        {
            if (buffSpellId)
            {
                if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                {
                    std::string msg = FSBUtilsTexts::BuildNPCSayText("", 0, FSBSayType::BuffSelf, FSBUtilsSpells::GetSpellName(buffSpellId));
                    bot->Say(msg, LANG_UNIVERSAL);
                }
            }

            return true;
        }

        return false;
    }

    bool BotOOCBuffGroup(Creature* bot, std::vector<Unit*> botGroup, uint32& buffTimer, uint32& globalCooldown)
    {
        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        std::vector<Unit*> buffTargets;
        uint32 buffSpellId = 0;
        uint32 now = getMSTime();

        switch (botClass)
        {
        case FSB_Class::Warrior:
            buffSpellId = SPELL_WARRIOR_BATTLE_SHOUT;
            break;
        case FSB_Class::Priest:
            buffSpellId = SPELL_PRIEST_POWER_WORD_FORTITUDE;
            break;
        case FSB_Class::Mage:
            buffSpellId = SPELL_MAGE_ARCANE_INTELLECT;
            break;
        case FSB_Class::Paladin:
            buffSpellId = SPELL_PALADIN_BLESSING_KINGS;
            break;

        default:
            break;
        }

        if (!buffSpellId)
        {
            buffTimer = now + 300000;
            return false;
        }

        GetBotBuffTargets(bot, buffSpellId, botGroup, 30.0f, buffTargets);

        if (!buffTargets.empty())
        {
            Unit* target = buffTargets.front();

            bot->CastSpell(target, buffSpellId, false);
            globalCooldown = now + 1500;

            if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
            {
                if (target == bot)
                {
                    std::string msg = FSBUtilsTexts::BuildNPCSayText("", NULL, FSBSayType::BuffSelf, FSBUtilsSpells::GetSpellName(buffSpellId));
                    bot->Say(msg, LANG_UNIVERSAL);
                }

                else
                {
                    std::string msg = FSBUtilsTexts::BuildNPCSayText(target->GetName(), NULL, FSBSayType::BuffTarget, FSBUtilsSpells::GetSpellName(buffSpellId));
                    bot->Say(msg, LANG_UNIVERSAL);
                }


            }

            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Bot: {} Buffed target: {} with {}", bot->GetName(), target->GetName(), FSBUtilsSpells::GetSpellName(buffSpellId));

            return true;
        }
        else buffTimer = now + 120000;

        return false;
    }

    void GetBotBuffTargets(Creature* bot, uint32 buffSpellId, std::vector<Unit*> botGroup, float maxRange, std::vector<Unit*>& outTargets)
    {
        if (!bot)
            return;

        for (Unit* member : botGroup)
        {
            if (!member)
                continue;

            if (!member->IsAlive())
                continue;

            if (member->HasAura(buffSpellId))
                continue;

            if (!bot->IsWithinDistInMap(member, maxRange))
                continue;

            if (!bot->IsWithinLOSInMap(member))
                continue;

            outTargets.push_back(member);
        }
    }
}
