#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_utils_combat.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_movement_handler.h"
#include "Followship_bots_outofcombat_handler.h"
#include "Followship_bots_recovery_handler.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_mage.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"


namespace FSBOOC
{
    bool BotOOCActions(Creature* bot, uint32& globalCooldown, uint32& buffTimer, uint32& selfBuffTimer, const std::vector<Unit*> botGroup, bool& demonDead, bool& botManaPotionUsed, bool& botHealthPotionUsed)
    {
        if (!bot)
            return false;

        if (!bot->IsAlive())
            return false;

        if (FSBUtilsCombat::IsCombatActive(bot))
            return false;

        if (bot->HasUnitState(UNIT_STAND_STATE_SIT))
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

        //4. Recover HP/MP
        // One recover action at a time
        if (BotOOCRecovery(bot, globalCooldown))
            return true;

        //5. Warlock Demon Summon
        if (BotOOCSummonDemons(bot, globalCooldown, demonDead))
            return true;

        //6. Warlock SoulStone
        if (BotOOCBuffSoulstone(bot, globalCooldown, botGroup))
            return true;

        //7. Clear combat flags and states
        if (BotOOCClearCombatFlags(bot, botManaPotionUsed, botHealthPotionUsed))
            return true;

        return false; 
    }

    bool BotOOCClearCombatFlags(Creature* bot, bool& botManaPotionUsed, bool& botHealthPotionUsed)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        bool check = false;

        if (botManaPotionUsed)
        {
            botManaPotionUsed = false;
            check = true;
        }

        if (botHealthPotionUsed)
        {
            botHealthPotionUsed = false;
            check = true;
        }

        if (check)
            return true;

        return false;
    }

    bool BotOOCBuffSoulstone(Creature* bot, uint32& globalCooldown, const std::vector<Unit*> botGroup)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        FSB_Class cls = FSBUtils::GetBotClassForEntry(bot->GetEntry());

        if (cls != FSB_Class::Warlock)
            return false;

        if (FSBWarlock::BotOOCBuffSoulstone(bot, globalCooldown, botGroup))
            return true;

        return false;
    }

    bool BotOOCSummonDemons(Creature* bot, uint32& globalCooldown, bool& demonDead)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        FSB_Class cls = FSBUtils::GetBotClassForEntry(bot->GetEntry());

        if (cls != FSB_Class::Warlock)
            return false;

        if (FSBWarlock::BotSummonRandomDemon(bot, globalCooldown, demonDead))
            return true;

        return false;
    }

    bool BotOOCRecovery(Creature* bot, uint32& globalCooldown)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        BotRecoveryIntent intent = FSBRecovery::DetermineRecoveryIntent(bot);

        switch (intent)
        {
        case BotRecoveryIntent::None:
            return false;
        case BotRecoveryIntent::RecoverHealthAndMana:
        case BotRecoveryIntent::RecoverMana:
        case BotRecoveryIntent::RecoverHealth:
            return FSBRecovery::TryOOCRecovery(bot, intent, globalCooldown);
        default:
            return false;
        }

        return false;
    }

    bool BotOOCHealOwner(Creature* bot, Player* player, uint32& globalCooldown)
    {
        if (!bot || !player || !player->IsAlive())
            return false;

        float pHealth = player->GetHealthPct();

        if (pHealth > 90)
            return false;

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());

        bool check = false;

        FSBMovement::StopFollow(bot);

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

        if (check && player)
        {
            if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                bot->Say(FSBUtilsTexts::BuildNPCSayText(player->GetName(), NULL, FSBSayType::HealTarget, ""), LANG_UNIVERSAL);

            return true;
        }

        return false;
    }

    bool BotOOCBuffSelf(Creature* bot, uint32& selfBuffTimer, uint32& globalCooldown)
    {
        if (!bot || !bot->IsAlive())
            return false;

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        uint32 buffSpellId = 0;
        bool check = false;

        switch (botClass)
        {
        case FSB_Class::Paladin:
            if (FSBPaladin::BotOOCBuffSelf(bot, globalCooldown, selfBuffTimer, buffSpellId))
                check = true;
            break;
        case FSB_Class::Warlock:
            if (FSBWarlock::BotOOCBuffSelf(bot, globalCooldown, selfBuffTimer, buffSpellId))
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
                    std::string msg = FSBUtilsTexts::BuildNPCSayText("", 0, FSBSayType::BuffSelf, FSBSpellsUtils::GetSpellName(buffSpellId));
                    bot->Say(msg, LANG_UNIVERSAL);
                }
            }

            return true;
        }

        return false;
    }

    bool BotOOCBuffGroup(Creature* bot, const std::vector<Unit*> botGroup, uint32& buffTimer, uint32& globalCooldown)
    {
        if (!bot)
            return false;

        if (botGroup.empty())
            return false;

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        std::vector<Unit*> buffTargets;
        uint32 buffSpellId = 0;
        uint32 buffSpellId2 = 0;
        uint32 now = getMSTime();

        switch (botClass)
        {
        case FSB_Class::Druid:
            buffSpellId = SPELL_DRUID_MARK_WILD;
            buffSpellId2 = SPELL_DRUID_THORNS;
            break;
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
        case FSB_Class::Warlock:
            buffSpellId = SPELL_WARLOCK_UNENDING_BREATH;
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
        if (buffSpellId2 && buffTargets.empty())
        {
            buffSpellId = buffSpellId2;
            GetBotBuffTargets(bot, buffSpellId, botGroup, 30.0f, buffTargets);
        }

        if (!buffTargets.empty())
        {
            Unit* target = buffTargets.front();

            if (target)
            {
                SpellCastResult result = bot->CastSpell(target, buffSpellId, false);

                // Only continue if the cast succeeded
                if (result != SPELL_CAST_OK)
                {
                    // Optional: debug log
                     TC_LOG_DEBUG("scripts.ai.fsb", "Bot {} failed to cast {} on {} (reason {})",
                         bot->GetName(), buffSpellId, target->GetName(), result);

                    return false; // stop here, no chatter, no GCD
                }

                globalCooldown = now + 1500;

                if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                {
                    if (target == bot)
                    {
                        std::string msg = FSBUtilsTexts::BuildNPCSayText(
                            "", NULL, FSBSayType::BuffSelf, FSBSpellsUtils::GetSpellName(buffSpellId));
                        bot->Say(msg, LANG_UNIVERSAL);
                    }
                    else
                    {
                        std::string msg = FSBUtilsTexts::BuildNPCSayText(
                            target->GetName(), NULL, FSBSayType::BuffTarget, FSBSpellsUtils::GetSpellName(buffSpellId));
                        bot->Say(msg, LANG_UNIVERSAL);
                    }
                }
            }

            return true;
        }
        else buffTimer = now + 120000;

        return false;
    }

    void GetBotBuffTargets(Creature* bot, uint32 buffSpellId, const std::vector<Unit*> botGroup, float maxRange, std::vector<Unit*>& outTargets)
    {
        if (!bot)
            return;

        if (botGroup.empty())
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
