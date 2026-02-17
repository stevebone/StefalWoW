#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_mage.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_warlock.h"

#include "Followship_bots_combat_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_incombat_handler.h"
#include "Followship_bots_powers_handler.h"
#include "Followship_bots_spells_handler.h"

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

        if (BotICHealSelf(bot))
            return true;

        if (BotICHealGroup(bot))
            return true;

        if (BotICTryDispel(bot))
            return true;

        if (BotICTryOffensiveDispel(bot))
            return true;

        if (BotICTryOffensiveSpell(bot))
            return true;

        return false;
    }

    bool BotICTryOffensiveSpell(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& globalCooldown = baseAI->botGlobalCooldown;
        uint32 now = getMSTime();

        if (!FSBSpellsUtils::CanCastNow(bot, now, globalCooldown))
            return false;

        auto& runtimeSpells = baseAI->botRuntimeSpells;

        auto spells = FSBSpells::BotGetAvailableSpells(bot, runtimeSpells, FSBSpellType::Damage, false);

        Unit* target = bot->GetVictim();
        if (!target || !target->IsInWorld() || target->IsDuringRemoveFromWorld() || !target->IsAlive())
            return false;

        FSBSpellRuntime* dmgSpell = FSBSpells::SelectBestDamageSpell(bot, spells, target);
        if (!dmgSpell)
            return false;

        if (dmgSpell && dmgSpell->def->isSelfCast)
            target = bot;

        float dist = bot->GetDistance(target);
        float spellDist = dmgSpell->def->dist;
        if (dmgSpell && spellDist && target)
        {
            if (dist > spellDist)
            {
                // Move into spell range
                bot->GetMotionMaster()->Clear();
                bot->GetMotionMaster()->MoveChase(target, spellDist);
                //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: SpellSkip - damage {} target too far ({}/{})", spell->spellId, dist, spell->dist);
                return false;
            }
        }

        
        // TO-DO move this to its own action before other combat related spells
        if (dmgSpell->def->spellId == SPELL_MAGE_POLYMORPH || dmgSpell->def->spellId == SPELL_WARLOCK_FEAR || dmgSpell->def->spellId == SPELL_DRUID_HIBERNATE)
            target = FSBUtilsCombat::GetRandomAttacker(bot);

        if (dmgSpell && target)
        {
            if (dmgSpell->def->manaCostOverride != 0.f && !FSBPowers::SpendPowerPct(bot, dmgSpell->def->manaCostOverride))
                return false; // not enough mana

            if (FSBSpells::BotCastSpell(bot, dmgSpell->def->spellId, target))
            {
                dmgSpell->nextReadyMs = now + dmgSpell->def->cooldownMs;
                globalCooldown = now + 1500;
                if (target == bot)
                    FSBUtilsCombat::SayCombatMessage(bot, bot, 0, FSBSayType::HealSelf, dmgSpell->def->spellId);
                else FSBUtilsCombat::SayCombatMessage(bot, target, 0, FSBSayType::SpellOnTarget, dmgSpell->def->spellId);
                return true;
                // Bot Say after spell cast - TO-DO transform this into its own method
            }
        }
        return false;
    }

    bool BotICHealGroup(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!FSBSpellsUtils::BotHasHealSpells(bot))
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& globalCooldown = baseAI->botGlobalCooldown;
        uint32 now = getMSTime();

        if (!FSBSpellsUtils::CanCastNow(bot, now, globalCooldown))
            return false;

        auto& runtimeSpells = baseAI->botRuntimeSpells;
        auto& botGroup = baseAI->botLogicalGroup;
        auto botRole = baseAI->botRole;

        float hpPct = 50.f;
        if (botRole == FSB_ROLE_HEALER) hpPct = 70.f;

        auto healTargets = FSBGroup::BotGetMembersToHeal(botGroup, hpPct);
        if (healTargets.empty())
            return false;

        if (botRole == FSB_ROLE_HEALER) FSBGroup::SortEmergencyTargets(healTargets);

        auto heals = FSBSpells::BotGetAvailableSpells(bot, runtimeSpells, FSBSpellType::Heal, false);

        Unit* target = nullptr;      

        if(!healTargets.empty())
            target = healTargets.front();

        FSBSpellRuntime* healSpell = FSBSpells::SelectBestHealSpell(heals, target);

        if (healSpell && target)
        {
            if (healSpell->def->isSelfCast && target != bot)
                return false;
            // check other requirements here

            if (FSBSpells::BotCastSpell(bot, healSpell->def->spellId, target))
            {
                healSpell->nextReadyMs = now + healSpell->def->cooldownMs;
                globalCooldown = now + 1500;
                if(target == bot)
                    FSBUtilsCombat::SayCombatMessage(bot, bot, 0, FSBSayType::HealSelf, healSpell->def->spellId);
                else FSBUtilsCombat::SayCombatMessage(bot, target, 0, FSBSayType::HealTarget, healSpell->def->spellId);
                return true;
                // Bot Say after spell cast - TO-DO transform this into its own method
            }
        }


        return false;
    }

    bool BotICHealSelf(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!FSBSpellsUtils::BotHasHealSpellsForSelf(bot))
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& globalCooldown = baseAI->botGlobalCooldown;
        uint32 now = getMSTime();

        if (!FSBSpellsUtils::CanCastNow(bot, now, globalCooldown))
            return false;

        auto& runtimeSpells = baseAI->botRuntimeSpells;

        auto selfHeals = FSBSpells::BotGetAvailableSpells(bot, runtimeSpells, FSBSpellType::Heal, true);

        FSBSpellRuntime* healSpell = FSBSpells::SelectBestHealSpell(selfHeals, bot);

        if (healSpell)
        {
            // check other requirements here

            if (FSBSpells::BotCastSpell(bot, healSpell->def->spellId, bot))
            {
                healSpell->nextReadyMs = now + healSpell->def->cooldownMs;
                globalCooldown = now + 1500;
                FSBUtilsCombat::SayCombatMessage(bot, bot, 0, FSBSayType::HealSelf, healSpell->def->spellId);
                return true;
                // Bot Say after spell cast - TO-DO transform this into its own method

                //else FSBUtilsCombat::SayCombatMessage(bot, target, 0, FSBSayType::HealTarget, def->spellId);
            }
        }


        return false;
    }

    bool BotICTryDispel(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->GetPower(POWER_MANA) < 10)
            return false;

        uint32 now = getMSTime();

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& globalCooldown = baseAI->botGlobalCooldown;

        if (!FSBSpellsUtils::CanCastNow(bot, now, globalCooldown))
            return false;

        if (FSBSpells::BotTryDispel(bot))
        {
            globalCooldown = now + 1500;
            return true;
        }

        return false;
    }

    bool BotICTryOffensiveDispel(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->GetPower(POWER_MANA) < 10)
            return false;

        uint32 now = getMSTime();

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& globalCooldown = baseAI->botGlobalCooldown;

        if (!FSBSpellsUtils::CanCastNow(bot, now, globalCooldown))
            return false;

        if (FSBSpells::BotTryOffensiveDispel(bot))
        {
            globalCooldown = now + 1500;
            return true;
        }

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

        if (!FSBSpellsUtils::CanCastNow(bot, now, globalCooldown))
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
