#include "Followship_bots.h"
#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_events_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_outofcombat_handler.h"
#include "Followship_bots_recovery_handler.h"
#include "Followship_bots_spells_handler.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_mage.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_priest.h"
#include "Followship_bots_rogue.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"

std::vector<FSBAFKAction> afkActions =
{
    {
    [](Creature* bot)
    {
        // 0. Check if there is a fire already
        GameObject* campfire = GetClosestGameObjectWithEntry(bot, 266354, 20.f);
        if (campfire)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            //return false;

        baseAI->botSitsByFire = true;

        // 1. Cast campfire spell
        bot->CastSpell(bot, SPELL_SPECIAL_CAMP_FIRE, false);

        // 2. Move 2f left or right
        bot->GetMotionMaster()->MovePoint(FSB_MOVEMENT_POINT_OUT_FIRE, bot->GetPositionX() + 2.f, bot->GetPositionY(), bot->GetPositionZ());

        // continues in the MovementInform for the movePoint
        return true;
    },
    45 // 25% chance
},

    {
        // Sit down
        [](Creature* bot)
        {
            bot->Say("Am gonna rest for a while...", LANG_UNIVERSAL);
            bot->SetStandState(UNIT_STAND_STATE_SIT);
            return true;
        },
        10 // 40% chance
    },

    {
        // Beg
        [](Creature* bot)
        {
            Player* player = FSBMgr::Get()->GetBotOwner(bot);
            if (player)
            {
                bot->TextEmote(97, player, false);
                bot->HandleEmoteCommand(EMOTE_ONESHOT_BEG, player);
                return true;
            }

            return false;
        },
        50
    },

    {
        // Look around
        [](Creature* bot)
        {
            std::string emote = bot->GetName() + " looks around absentmindedly.";
            bot->TextEmote(emote);
            bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);

            return true;
        },
        10
    },

    {
        // Yawn
        [](Creature* bot)
        {
            bot->TextEmote("yawns loudly.");
            bot->HandleEmoteCommand(EMOTE_STATE_SLEEP);

            return true;
        },
        50
    }
};

namespace FSBOOC
{
    bool BotOOCActions(FSB_BaseAI* ai/*, const std::vector<Unit*> botGroup*/)
    {
        Creature* bot = ai->GetBot();
        auto& globalCooldown = ai->botGlobalCooldown;
        auto& buffTimer = ai->botBuffsTimer;
        auto& selfBuffTimer = ai->botSelfBuffsTimer;
        auto& botHasDemon = ai->botHasDemon;
        auto& botResTargetGuid = ai->botResurrectTargetGuid;
        auto botRole = ai->botRole;
        auto& botGroup = ai->botLogicalGroup;

        if (!bot)
            return false;

        if (!bot->IsAlive())
            return false;

        if (FSBUtilsCombat::IsCombatActive(bot))
            return false;

        if (FSBRecovery::BotHasRecoveryActive(bot))
            return false;

        uint32 now = getMSTime();

        if (!FSBSpellsUtils::CanCastNow(bot, now, globalCooldown))
            return false;

        //0 OOC Resurrect
        // If we resurrect someone then end tick
        if (BotOOCResurrect(bot, botResTargetGuid))
            return true;

        //1. Bot Heal Owner
        // If we heal owner this turn return and end tick
        Player* player = FSBMgr::Get()->GetBotOwner(bot);
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
        if (BotOOCSummonDemons(bot, globalCooldown, botHasDemon))
            return true;

        //6. Warlock SoulStone
        if (BotOOCBuffSoulstone(bot, globalCooldown, botGroup))
            return true;

        //7. Clear combat flags and states
        if (BotOOCClearFlagsStates(bot))
            return true;

        // Random event
        if (BotOOCDoRandomEvent(bot))
            return true;

        return false; 
    }

    bool BotOOCDoRandomEvent(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto& botRandomEventCooldown = baseAI->botRandomEventTimer;

        uint32 now = getMSTime();
        if (now < botRandomEventCooldown)
            return false;

        if (BotOOCSpawnCompanion(bot))
        {
            botRandomEventCooldown = now + 120000; // 2 min cooldown
            return true;
        }

        if (BotOOCActionPlayerAFK(bot))
        {
            botRandomEventCooldown = now + 120000; // 2 min cooldown
            return true;
        }

        botRandomEventCooldown = now + 120000; // 2 min cooldown
        return false;
    }

    bool BotOOCActionPlayerAFK(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        Player* player = FSBMgr::Get()->GetBotOwner(bot);
        if (!player || !player->IsAlive())
            return false;

        if (player->isAFK())
        {
            for (auto const& entry : afkActions)
            {
                if (urand(1, 100) <= entry.chance)
                {
                    if (entry.action(bot))
                        return true; // action executed
                    // else: skip and continue to next action
                }
            }

        }

        return false;
    }

    bool BotOOCSpawnCompanion(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto& botHasCompanion = baseAI->botHasCompanion;
        

        uint32 companionSpell = FSBSpellsUtils::GetBotCompanionSpellForEntry(bot->GetEntry());
        if (companionSpell == 0)
            return false;        

        // 30% chance to even consider doing this
        if (urand(1, 100) <= 30)
        {
            int32 timerDiff = urandms(45, 60);

            if (FSBCombatUtils::IsOutOfCombatFor(bot, timerDiff) && !botHasCompanion)
            {
                
                botHasCompanion = true;
                FSBSpells::BotCastSpell(bot, companionSpell, bot);
                
                return true;
            }
        }

        return false;
    }

    bool BotOOCResurrect(Creature* bot, ObjectGuid& resTargetGuid)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!FSBUtils::BotIsHealerClass(bot))
            return false;

        if (!resTargetGuid)
            return false;

        FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_RESURRECT_TARGET, 3s, 5s);

        return true;
    }

    bool BotOOCResurrectTarget(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());

        auto& resurrectTargetGuid = baseAI->botResurrectTargetGuid;
        auto botClass = baseAI->botClass;
        auto& globalCooldown = baseAI->botGlobalCooldown;
        Unit* target = ObjectAccessor::GetUnit(*bot, resurrectTargetGuid);
        bool canCombatRes = botClass == FSB_Class::Druid && bot->IsInCombat();

        if(target)
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: {} found dead unit {} for resurrection", bot->GetName(), target->GetName());

        if (!target || target->IsAlive())
        {
            return false;
        }

        if ((bot->IsInCombat() && !canCombatRes) || bot->HasUnitState(UNIT_STATE_CASTING))
        {
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_RESURRECT_TARGET, 5s);
            return false;
        }

        if (target && bot->GetMapId() == target->GetMapId() && bot->GetDistance(target) > 30.0f)
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Ressurect target {} too far from bot: {}", target->GetName(), bot->GetName());
            //bot->GetMotionMaster()->MoveChase(target, 28.f);
            bot->GetMotionMaster()->MoveCloserAndStop(4, target, 28.f);
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_RESURRECT_TARGET, 5s);
            return false;
        }

        uint32 spellId = 0;

        switch (botClass)
        {
        case FSB_Class::Priest:
            spellId = SPELL_PRIEST_RESURRECTION;
            break;
        case FSB_Class::Druid:
            if (canCombatRes)
                spellId = SPELL_DRUID_REBIRTH;
            else spellId = SPELL_DRUID_REVIVE;
            break;
        case FSB_Class::Paladin:
            spellId = SPELL_PALADIN_REDEMPTION;
            break;
        default:
            break;
        }

        if (spellId)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
            int32 castTimeMs = spellInfo->CalcCastTime(); // in milliseconds
            FSBMovement::StopFollow(bot);

            if (FSBSpells::BotCastSpell(bot, spellId, target))
            {
                uint32 now = getMSTime();
                globalCooldown = now + 1500;
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Ressurect Bot {} tried ressurect spell: {} on {}", bot->GetName(), spellId, target->GetName());

                if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                {
                    // SAY after ressurect
                    std::string msg = FSBUtilsTexts::BuildNPCSayText(target->GetName(), NULL, FSBSayType::Resurrect, "");
                    bot->Say(msg, LANG_UNIVERSAL);
                }

                resurrectTargetGuid.Clear();
            }
            else
            {
                FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_RESURRECT_TARGET, 5s);
                return false;
            }

            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_RESUME_FOLLOW, std::chrono::milliseconds(castTimeMs + 1000));
            return true;
        }

        return false;
    }

    bool BotOOCClearFlagsStates(Creature* bot)
    {
        if (!bot)
            return false;

        if (bot->IsInCombat())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& manaUsed = baseAI->botManaPotionUsed;
        auto& healthUsed = baseAI->botHealthPotionUsed;
        auto& buffsCasted = baseAI->botCastedCombatBuffs;

        bool check = false;

        if (manaUsed)
        {
            manaUsed = false;
            check = true;
        }

        if (healthUsed)
        {
            healthUsed = false;
            check = true;
        }

        if (buffsCasted)
        {
            buffsCasted = false;
            check = true;
        }       

        auto& botSayMemberDead = baseAI->botSayMemberDead;
        auto& deadTargetGuid = baseAI->botResurrectTargetGuid;
        Unit* target = nullptr;
        if(!deadTargetGuid.IsEmpty())
            target = ObjectAccessor::GetUnit(*bot, deadTargetGuid);

        if (target && target->IsAlive())
        {
            deadTargetGuid.Clear();
            botSayMemberDead = false;
            check = true;
        }

        // check if we need to make the bot stand again (after random sit event)
        auto& botSitsByFire = baseAI->botSitsByFire;
        auto fDistance = baseAI->botFollowDistance;
        auto fAngle = baseAI->botFollowAngle;

        Player* player = FSBMgr::Get()->GetBotOwner(bot);
        if (!player && bot->HasUnitState(UNIT_STAND_STATE_SIT))
        {
            bot->SetStandState(UNIT_STAND_STATE_STAND);
            if (botSitsByFire)
                botSitsByFire = false;
        }
        else if (player && bot->HasUnitState(UNIT_STAND_STATE_SIT) && (!player->isAFK() || player->isMoving()))
        {
            bot->SetStandState(UNIT_STAND_STATE_STAND);
            if (botSitsByFire)
                botSitsByFire = false;

            FSBMovement::ResumeFollow(bot, fDistance, fAngle);
        }

        

        if (!botSitsByFire)
        {
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_RANDOM_ACTION_MOVE_FIRE, 1s, 3s);
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

        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        if (cls != FSB_Class::Warlock)
            return false;

        if (FSBWarlock::BotOOCBuffSoulstone(bot, globalCooldown, botGroup))
            return true;

        return false;
    }

    bool BotOOCSummonDemons(Creature* bot, uint32& globalCooldown, bool& botHasDemon)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        if (cls != FSB_Class::Warlock)
            return false;

        if (FSBWarlock::BotSummonRandomDemon(bot, globalCooldown, botHasDemon))
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

        if (!FSBUtils::BotIsHealerClass(bot))
            return false;

        float pHealth = player->GetHealthPct();

        if (pHealth > 90)
            return false;

        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
        if(botClass == FSB_Class::Druid && (bot->HasAura(SPELL_DRUID_BEAR) || bot->HasAura(SPELL_DRUID_CAT) || bot->HasAura(SPELL_DRUID_TRAVEL)))
            return false;

        bool check = false;

        FSBMovement::StopFollow(bot);

        switch (botClass)
        {
        case FSB_Class::Priest:
            if (FSBPriest::BotOOCHealOwner(bot, player, globalCooldown))
                check = true;
            break;
        case FSB_Class::Druid:
            if (FSBDruid::BotOOCHealOwner(bot, player, globalCooldown))
                check = true;
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

        uint32 now = getMSTime();

        if (now < selfBuffTimer)
            return false;

        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
        uint32 buffSpellId = 0;
        bool check = false;

        switch (botClass)
        {
        case FSB_Class::Druid:
            if (FSBDruid::BotOOCBuffSelf(bot, globalCooldown, selfBuffTimer, buffSpellId))
                check = true;
            break;
        case FSB_Class::Paladin:
            if (FSBPaladin::BotOOCBuffSelf(bot, globalCooldown, selfBuffTimer, buffSpellId))
                check = true;
            break;
        case FSB_Class::Rogue:
            if (FSBRogue::BotOOCBuffSelf(bot, buffSpellId))
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
        selfBuffTimer = now + 60000; // 1 minute
        return false;
    }

    bool BotOOCBuffGroup(Creature* bot, const std::vector<Unit*>& botGroup, uint32& buffTimer, uint32& globalCooldown)
    {
        if (!bot)
            return false;

        if (botGroup.empty())
            return false;

        if (bot->HasAura(SPELL_DRUID_BEAR) || bot->HasAura(SPELL_DRUID_CAT) || bot->HasAura(SPELL_DRUID_TRAVEL))
            return false;

        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
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
                // we cannot buff bots with stealth aura
                if (target->HasAura(SPELL_ROGUE_STEALTH))
                    return false;

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

    void GetBotBuffTargets(Creature* bot, uint32 buffSpellId, const std::vector<Unit*>& botGroup, float maxRange, std::vector<Unit*>& outTargets)
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
