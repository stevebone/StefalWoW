/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Containers.h"
#include "DB2Stores.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "SpellHistory.h"
#include "SpellInfo.h"

#include "Followship_bots.h"
#include "Followship_bots_config.h"
#include "Followship_bots_utils.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_chatter_handler.h"
#include "Followship_bots_chat_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_outofcombat_handler.h"
#include "Followship_bots_pet_handler.h"
#include "Followship_bots_recovery_handler.h"
#include "Followship_bots_spells_handler.h"

namespace FSBOOC
{
    bool BotOOCActions(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& globalCooldown = baseAI->botGlobalCooldown;
        auto& buffTimer = baseAI->botBuffsTimer;
        auto& selfBuffTimer = baseAI->botSelfBuffsTimer;
        auto& botGroup = baseAI->botLogicalGroup;

        if (FSBCombatUtils::IsCombatActive(bot))
            return false;

        uint32 now = getMSTime();

        if (!FSBSpellsUtils::CanCastNow(bot, now, globalCooldown))
            return false;

        //Clear combat flags and states
        BotOOCClearFlagsStates(bot);

        // This check needs to be here because BotOOCClearFlagsStates will cancel the recovery if no longer needed
        // Moving this above the other will block the cancel check.
        if (FSBRecovery::BotHasRecoveryActive(bot))
            return false;

        //0 OOC Resurrect
        // If we resurrect someone then end tick
        if (BotOOCResurrect(bot))
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
        if (BotOOCSummonPetOrDemon(bot))
            return true;

        //6. Warlock SoulStone
        if (BotOOCBuffSoulstone(bot, globalCooldown, botGroup))
            return true;

        //7. Paladin Beacon
        if (BotOOCBuffBeacon(bot))
            return true;

        //8. Random event
        if (BotOOCDoRandomEvent(bot))
            return true;       

        return false; 
    }

    bool BotOOCDoRandomEvent(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        //TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} Triggered DoRandomEvent", bot->GetName());

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto& botRandomEventCooldown = baseAI->botRandomEventTimer;

        uint32 now = getMSTime();
        if (now < botRandomEventCooldown)
            return false;

        //TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} Triggered DoRandomEvent passed cooldown", bot->GetName());

        bool check = false;

        if (BotOOCActionPlayerAFK(bot, false))
        {
            //TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} Triggered DoRandomEvent action player afk", bot->GetName());
            check = true;
        }

        if (check)
        {
            //TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: check true, applied cooldown", bot->GetName());
            botRandomEventCooldown = now + RANDOM_EVENT_INTERVAL; 
            return true;
        }

        return false;
    }

    bool BotOOCActionPlayerAFK(Creature* bot, bool force)
    {
        if (urand(1, 100) > RANDOM_AFK_EVENT_CHANCE)
            return true; // if roll fails we return true so that cooldown is applied to reduce action spam

        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto botByFire = baseAI->botSitsByFire;
        auto& isDoingRandomEvent = baseAI->botDoingRandomEvent;
        auto botOwnerNotMoving = baseAI->botOwnerNotMovingTimer;
        auto hired = baseAI->botHired;

        if (!hired)
            return false;

        if (isDoingRandomEvent || botByFire)
            return false;
        //TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: Player AFK detection step", bot->GetName());
        Player* player = FSBMgr::Get()->GetBotOwner(bot);
        if (!player || !player->IsAlive())
            return false;

        uint32 now = getMSTime();
        uint32 ownerNotMovingTime = now - botOwnerNotMoving;
        
        bool playerAfk = player->isAFK() || ownerNotMovingTime > RANDOM_EVENT_INTERVAL;
        //TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: owner not moving for: {}", bot->GetName(), ownerNotMovingTime);
        if (force)
            playerAfk = true;

        if (playerAfk)
        {
            if (bot->HasAura(SPELL_ROGUE_STEALTH))
                bot->RemoveAurasDueToSpell(SPELL_ROGUE_STEALTH);

            //TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} detected owner AFK", bot->GetName());
            isDoingRandomEvent = true;
            uint8 randomAction = urand(0, FSB_AFK_MAX_ACTIONS - 1);

            if (BotOOCAFKAction(bot, randomAction))
            {
                FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_RANDOM_ACTION_FINISH, 30s, 45s);
                return true;
            }
        }

        return false;
    }

    bool BotOOCAFKAction(Creature* bot, uint8 action)
    {
        if (!bot || !bot->IsAlive())
            return false;

        Player* player = FSBMgr::Get()->GetBotOwner(bot);
        if (!player || !player->IsAlive())
            return false;

        switch (action)
        {
        case FSB_AFK_ACTION_SHAMAN_WOLF:
        {
            auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI)
                return false;

            if (baseAI->botClass != FSB_Class::Shaman)
                return false;

            if (FSBSpells::BotCastSpell(bot, SPELL_SHAMAN_GHOST_WOLF, bot))
                return true;

            return false;
        }
        case FSB_AFK_ACTION_MAKE_PICNIC:
        {
            if (bot->GetSpellHistory()->HasCooldown(SPELL_SPECIAL_ROMANTIC_PICNIC))
                return false;

            TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: picnic", bot->GetName());
            // 0. Check if there is a fire already
            GameObject* picnic = GetClosestGameObjectWithEntry(bot, 187267, 20.f);
            if (picnic)
                return false;

            if (bot->GetStandState() == UNIT_STAND_STATE_STAND)
            {
                // 1. Cast campfire spell
                bot->CastSpell(bot, SPELL_SPECIAL_ROMANTIC_PICNIC, false);
                bot->SetStandState(UNIT_STAND_STATE_SIT);
                return true;
            }

            return false;
        }

        case FSB_AFK_ACTION_MAKE_FIRE:
        {
            if (BotOOCActionCook(bot, SPELL_SPECIAL_CAMP_FIRE, 266354))
                return true;

            return false;
        }

        case FSB_AFK_ACTION_COOKING_POT:
        {
            if (BotOOCActionCook(bot, SPELL_SPECIAL_COOKING_POT, 379147))
                return true;

            return false;
        }

        case FSB_AFK_ACTION_COOK_SAUSAGES:
        {
            if (BotOOCActionCook(bot, SPELL_SPECIAL_COOK_SAUSAGES, 236110))
                return true;

            return false;
        }

        case FSB_AFK_ACTION_BEG:
        {
            if (player)
            {
                bot->SetFacingToObject(player, true);
                TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: beg", bot->GetName());
                std::string emote = bot->GetName() + " begs " + player->GetName() + ". How pathethic!";
                bot->TextEmote(emote, player);
                bot->HandleEmoteCommand(EMOTE_ONESHOT_BEG, player);
                return true;
            }
            return false;
        }

        case FSB_AFK_ACTION_DANCE:
        {
            if (player)
            {
                bot->SetFacingToObject(player, true);
                TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: dance", bot->GetName());
                std::string emote = bot->GetName() + " dances with " + player->GetName();
                bot->TextEmote(emote, player);
                bot->HandleEmoteCommand(EMOTE_STATE_DANCE, player);
                return true;
            }
            return false;
        }

        case FSB_AFK_ACTION_KISS:
        {
            if (BotOOCActionKiss(bot))
                return true;

            return false;
        }

        case FSB_AFK_ACTION_WHISTLE:
        {
            TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: whistle", bot->GetName());
            auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI)
                return false;
            auto botRace = baseAI->botRace;
            auto botGenger = FSBMgr::Get()->GetBotGenderForEntry(bot->GetEntry());
            auto soundInfo = sDB2Manager.GetTextSoundEmoteFor(TEXT_EMOTE_BORED, FSBUtils::BotRaceToTC(botRace), botGenger, 0);
            uint32 soundId = 0;
            if (soundInfo)
            {
                soundId = soundInfo->SoundID;
            }

            // 1. Emote text
            std::string emote = FSBChatter::GetRandomReply(bot, nullptr, FSB_ChatterCategory::emote_whistle, FSB_ChatterType::None, 0);
            if (!emote.empty())
                bot->TextEmote(emote);

            bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
            if (soundId)
                bot->PlayDistanceSound(soundId);
            else TC_LOG_WARN("scripts.ai.fsb", "FSB AFK Action Whistle: no sound found for race {}", botRace);

            return true;
        }

        case FSB_AFK_ACTION_FLIRT:
        {
            if (BotOOCActionFlirt(bot))
                return true;

            return false;
        }

        case FSB_AFK_ACTION_JOKE:
        {
            if (BotOOCActionJoke(bot))
                return true;

            return false;
        }

        case FSB_AFK_ACTION_SLEEP:
        {
            if (BotOOCActionSleep(bot))
                return true;

            return false;
        }

        case FSB_AFK_ACTION_SIGH:
        {
            if (BotOOCActionSigh(bot))
                return true;

            return false;
        }

        case FSB_AFK_ACTION_REST:
        {
            TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: rest", bot->GetName());
            bot->Say("Am gonna rest for a while...", LANG_UNIVERSAL);
            bot->SetStandState(UNIT_STAND_STATE_SIT);
            return true;
        }

        case FSB_AFK_ACTION_TALK:
        {
            TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: talk", bot->GetName());
            bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
            FSBChatter::DemandTimedReply(bot, nullptr, FSB_ChatterCategory::emote_talk, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
            
            return true;
        }

        case FSB_AFK_ACTION_WHISPER:
        {
            if (player)
            {
                TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: whisper", bot->GetName());
                bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                FSBChatter::DemandTimedReply(bot, player, FSB_ChatterCategory::whisper_afk, FSB_ReplyType::Whisper, FSB_ChatterSource::Bot);
                return true;
            }
            return false;
        }

        case FSB_AFK_ACTION_NOTHING:
        {
            TC_LOG_INFO("scripts.ai.fsb", "FSB Bot {} randomEvent: nothing", bot->GetName());
            return true;
        }

        case FSB_AFK_ACTION_COMPANION:
            if (BotOOCSpawnCompanion(bot))
                return true;
            return false;

        default:
            return false;
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

        int32 timerDiff = urandms(45, 60);

        if (FSBCombatUtils::IsOutOfCombatFor(bot, timerDiff) && !botHasCompanion)
        {
            baseAI->botHasCompanion = true;
            FSBSpells::BotCastSpell(bot, companionSpell, bot);
            return true;
        }

        return false;
    }

    bool BotOOCResurrect(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (!FSBUtils::BotIsHealerClass(bot))
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        if (!baseAI->botResurrectTargetGuid)
            return false;

        FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_RESURRECT_TARGET, 3s, 5s);

        return true;
    }

    bool BotOOCResurrectTarget(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& resurrectTargetGuid = baseAI->botResurrectTargetGuid;
        auto botClass = baseAI->botClass;
        Unit* target = ObjectAccessor::GetUnit(*bot, resurrectTargetGuid);
        bool canCombatRes = botClass == FSB_Class::Druid && bot->IsInCombat();

        if (!target || target->IsAlive())
        {
            return false;
        }

        if (target)
            TC_LOG_DEBUG("scripts.fsb.ooc", "FSB: BotOOCResurrectTarget Bot {} found dead unit {} for resurrection", bot->GetName(), target->GetName());

        if ((bot->IsInCombat() && !canCombatRes) || bot->HasUnitState(UNIT_STATE_CASTING))
        {
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_RESURRECT_TARGET, 5s);
            return false;
        }
        
        if (target && bot->GetMapId() == target->GetMapId() && bot->GetDistance(target) > 30.0f)
        {
            TC_LOG_DEBUG("scripts.fsb.ooc", "FSB: BotOOCResurrectTarget target {} too far from bot: {}", target->GetName(), bot->GetName());
            bot->GetMotionMaster()->Clear();
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
        case FSB_Class::Monk:
            spellId = SPELL_MONK_RESUSCITATE;
            break;
        case FSB_Class::Paladin:
            spellId = SPELL_PALADIN_REDEMPTION;
            break;
        case FSB_Class::Shaman:
            spellId = SPELL_SHAMAN_ANCESTRAL_SPIRIT;
            break;
        default:
            break;
        }

        if (spellId)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
            int32 castTimeMs = spellInfo->CalcCastTime(); // in milliseconds
            //FSBMovement::StopFollow(bot);

            if (FSBSpells::BotCastSpell(bot, spellId, target))
            {
                uint32 now = getMSTime();
                baseAI->botGlobalCooldown = now + 1500;
                TC_LOG_DEBUG("scripts.fsb.ooc", "FSB: BotOOCResurrectTarget Bot {} tried ressurect spell: {} on {}", bot->GetName(), spellId, target->GetName());

                if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
                {
                    // SAY after ressurect
                    FSBChatter::DemandBotChatter(bot, target, FSB_ChatterCategory::botRevivedTarget, FSB_ReplyType::Say, FSB_ChatterSource::None, 0);
                }

                baseAI->botResurrectTargetGuid.Clear();
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

    void BotOOCClearFlagsStates(Creature* bot)
    {
        if (!bot)
            return;

        if (bot->IsInCombat())
            return;

        FSBRecovery::BotCancelRecoveryAtFull(bot);

        if (FSBRecovery::BotHasRecoveryActive(bot))
            return;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return;

        if (!baseAI->botHired)
            return;

        baseAI->botAnnouncedLowMana = false;
        baseAI->botAnnouncedLowHealth = false;
        baseAI->botAnnouncedVeryLowHealth = false;

        baseAI->botManaPotionUsed = false;
        baseAI->botHealthPotionUsed = false;
        baseAI->botCastedCombatBuffs = false;

        // Set bot to follow if move state is follow but we are idle
        if (baseAI->botMoveState == FSB_MOVE_STATE_FOLLOWING && FSBMovement::GetMovementType(bot) != FOLLOW_MOTION_TYPE)
            FSBMovement::ResumeFollow(bot, baseAI->botFollowDistance, baseAI->botFollowAngle);

        auto& deadTargetGuid = baseAI->botResurrectTargetGuid;
        Unit* target = nullptr;
        if(!deadTargetGuid.IsEmpty())
            target = ObjectAccessor::GetUnit(*bot, deadTargetGuid);

        if (target && target->IsAlive())
        {
            baseAI->botResurrectTargetGuid.Clear();
            baseAI->botSayMemberDead = false;
        }
        
        
        Player* player = FSBMgr::Get()->GetBotOwner(bot);
        auto& botSitsByFire = baseAI->botSitsByFire;
        auto& botRandomEvent = baseAI->botDoingRandomEvent;
        auto fDistance = baseAI->botFollowDistance;
        auto fAngle = baseAI->botFollowAngle;

        auto& botOwnerNotMoving = baseAI->botOwnerNotMovingTimer;
        uint32 now = getMSTime();

        if (player && player->isMoving())
            baseAI->botOwnerNotMovingTimer = now;

        // check if we have a fire to go and sit around
        if (!botSitsByFire && player && (player->isAFK() || (now - botOwnerNotMoving) > RANDOM_EVENT_INTERVAL))
            FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_RANDOM_ACTION_MOVE_FIRE, 1s, 3s);

        // check if we need to make the bot stand again (after random sit event) and player is no longer afk
        if (player && (/*!player->isAFK() || */player->isMoving()))
        {
            if (botSitsByFire || botRandomEvent)
            {
                if (bot->HasAura(SPELL_SHAMAN_GHOST_WOLF))
                    bot->RemoveAurasDueToSpell(SPELL_SHAMAN_GHOST_WOLF);
                //bot->SetEmoteState(Emote(EMOTE_STATE_NONE));
                bot->HandleEmoteCommand(EMOTE_ONESHOT_YES);
                bot->SetStandState(UNIT_STAND_STATE_STAND);
                baseAI->botSitsByFire = false;
                baseAI->botDoingRandomEvent = false;
                FSBMovement::ResumeFollow(bot, fDistance, fAngle);
            }
        }

        
    }

    bool BotOOCBuffBeacon(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        if (bot->GetStandState() == UNIT_STAND_STATE_SIT)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto isDoingRandomEvent = baseAI->botDoingRandomEvent;

        if (isDoingRandomEvent)
            return false;

        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        if (cls != FSB_Class::Paladin)
            return false;

        if (FSBPaladin::BotOOCBuffBeacon(bot))
            return true;

        return false;
    }

    bool BotOOCBuffSoulstone(Creature* bot, uint32& globalCooldown, const std::vector<Unit*> botGroup)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        if (bot->GetStandState() == UNIT_STAND_STATE_SIT)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto isDoingRandomEvent = baseAI->botDoingRandomEvent;

        if (isDoingRandomEvent)
            return false;

        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        if (cls != FSB_Class::Warlock)
            return false;

        if (FSBWarlock::BotOOCBuffSoulstone(bot, globalCooldown, botGroup))
            return true;

        return false;
    }

    bool BotOOCSummonPetOrDemon(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->IsInCombat())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto isDoingRandomEvent = baseAI->botDoingRandomEvent;

        if (isDoingRandomEvent)
            return false;

        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        if (cls != FSB_Class::Warlock && cls != FSB_Class::Hunter)
            return false;

        if (FSBWarlock::BotSummonRandomDemon(bot))
            return true;

        if (FSBPet::BotSummonPet(bot))
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

        if (bot->GetStandState() == UNIT_STAND_STATE_SIT)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto isDoingRandomEvent = baseAI->botDoingRandomEvent;

        if (isDoingRandomEvent)
            return false;

        float pHealth = player->GetHealthPct();

        if (pHealth > 90)
            return false;

        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
        if(botClass == FSB_Class::Druid && (bot->HasAura(SPELL_DRUID_BEAR) || bot->HasAura(SPELL_DRUID_CAT) || bot->HasAura(SPELL_DRUID_TRAVEL)))
            return false;

        bool check = false;

        //FSBMovement::StopFollow(bot);

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
        case FSB_Class::Monk:
            if (FSBMonk::BotOOCHealOwner(bot))
                check = true;
            break;
        case FSB_Class::Shaman:
            if (FSBShaman::BotOOCHealOwner(bot))
                check = true;
            break;
        default:
            break;
        }

        if (check && player)
        {
            if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
            {
                FSBChatter::DemandBotChatter(bot, player, FSB_ChatterCategory::botHealTarget, FSB_ReplyType::Say, FSB_ChatterSource::Bot, 0);
            }

            return true;
        }

        return false;
    }

    bool BotOOCBuffSelf(Creature* bot, uint32& selfBuffTimer, uint32& globalCooldown)
    {
        if (!bot || !bot->IsAlive())
            return false;

        if (bot->GetStandState() == UNIT_STAND_STATE_SIT)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto isDoingRandomEvent = baseAI->botDoingRandomEvent;

        if (isDoingRandomEvent)
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
        case FSB_Class::Shaman:
            if (FSBShaman::BotOOCBuffSelf(bot, buffSpellId))
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
                    FSBChatter::DemandBotChatter(bot, nullptr, FSB_ChatterCategory::botBuffSelf, FSB_ReplyType::Say, FSB_ChatterSource::None, buffSpellId);
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

        if (bot->GetStandState() == UNIT_STAND_STATE_SIT)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        auto isDoingRandomEvent = baseAI->botDoingRandomEvent;

        if (isDoingRandomEvent)
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
        case FSB_Class::Shaman:
            buffSpellId = SPELL_SHAMAN_WATER_WALKING;
            if (baseAI->botRole == FSB_ROLE_HEALER)
                buffSpellId2 = SPELL_SHAMAN_EARTH_SHIELD;
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
            if (baseAI->botRole == FSB_ROLE_HEALER)
            {
                Unit* tank = FSBGroup::BotGetFirstGroupTank(botGroup);

                if (tank && !tank->HasAura(buffSpellId))
                    buffTargets.push_back(tank);
                else
                {
                    Player* owner = FSBMgr::Get()->GetBotOwner(bot);
                    if (owner && !owner->HasAura(buffSpellId))
                        buffTargets.push_back(owner);
                }
            }
                
            else GetBotBuffTargets(bot, buffSpellId, botGroup, 30.0f, buffTargets);
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
                        FSBChatter::DemandBotChatter(bot, nullptr, FSB_ChatterCategory::botBuffSelf, FSB_ReplyType::Say, FSB_ChatterSource::None, buffSpellId);
                    }
                    else
                    {
                        FSBChatter::DemandBotChatter(bot, target, FSB_ChatterCategory::botBuffTarget, FSB_ReplyType::Say, FSB_ChatterSource::Bot, buffSpellId);
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

    bool BotOOCActionSigh(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;
        auto botRace = baseAI->botRace;
        auto botGender = FSBMgr::Get()->GetBotGenderForEntry(bot->GetEntry());
        auto soundInfo = sDB2Manager.GetTextSoundEmoteFor(TEXT_EMOTE_SIGH, FSBUtils::BotRaceToTC(botRace), botGender, 0);
        uint32 soundId = 0;
        if (soundInfo)
        {
            soundId = soundInfo->SoundID;
        }

        TC_LOG_INFO("scripts.fsb.ooc", "FSB: BotOOCActionSigh started for Bot {}", bot->GetName());

        // 1. Emote text
        std::string emote = FSBChatter::GetRandomReply(bot, nullptr, FSB_ChatterCategory::emote_sigh, FSB_ChatterType::None, 0);
        if (!emote.empty())
            bot->TextEmote(emote);

        if (soundId)
            bot->PlayDistanceSound(soundId);
        else TC_LOG_WARN("scripts.fsb.ooc", "FSB: BotOOCActionSigh: no sound found for race {}", botRace);

        return true;
    }

    bool BotOOCActionSleep(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;
        auto botRace = baseAI->botRace;
        auto botGender = FSBMgr::Get()->GetBotGenderForEntry(bot->GetEntry());
        auto soundInfo = sDB2Manager.GetTextSoundEmoteFor(TEXT_EMOTE_YAWN, FSBUtils::BotRaceToTC(botRace), botGender, 0);
        uint32 soundId = 0;
        if (soundInfo)
        {
            soundId = soundInfo->SoundID;
        }

        TC_LOG_INFO("scripts.fsb.ooc", "FSB: BotOOCActionSleep started for Bot {}", bot->GetName());

        // 1. Emote text
        std::string emote = FSBChatter::GetRandomReply(bot, nullptr, FSB_ChatterCategory::emote_sleep, FSB_ChatterType::None, 0);
        if (!emote.empty())
            bot->TextEmote(emote);

        if (soundId)
            bot->PlayDistanceSound(soundId);
        else TC_LOG_WARN("scripts.fsb.ooc", "FSB: BotOOCActionSleep: no sound found for race {}", botRace);

        bot->SetStandState(UNIT_STAND_STATE_SLEEP);

        return true;
    }

    bool BotOOCActionJoke(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;
        auto& botGroup = baseAI->botLogicalGroup;
        auto botRace = baseAI->botRace;
        auto botGender = FSBMgr::Get()->GetBotGenderForEntry(bot->GetEntry());
        auto soundInfo = sDB2Manager.GetTextSoundEmoteFor(TEXT_EMOTE_JOKE, FSBUtils::BotRaceToTC(botRace), botGender, 0);
        uint32 soundId = 0;
        if (soundInfo)
        {
            soundId = soundInfo->SoundID;
        }

        Unit* randomUnit = nullptr;
        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        if (botGroup.size() >= 3)
        {
            do
            {
                randomUnit = Trinity::Containers::SelectRandomContainerElement(botGroup);
            } while (randomUnit == bot);
        }
        else if (player)
        {
            randomUnit = player;
        }
        else return false;

        if (randomUnit)
        {
            TC_LOG_INFO("scripts.fsb.ooc", "FSB: BotOOCActionJoke started for Bot {}", bot->GetName());
            // 1. Emote text
            std::string emote = FSBChatter::GetRandomReply(bot, randomUnit, FSB_ChatterCategory::emote_joke, FSB_ChatterType::None, 0);
            if (!emote.empty())
                bot->TextEmote(emote);

            bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
            if (soundId)
                bot->PlayDistanceSound(soundId);
            else TC_LOG_WARN("scripts.fsb.ooc", "FSB: BotOOCActionJoke: no sound found for race {}", botRace);

            if (!randomUnit->IsPlayer())
                FSBChatter::DemandTimedReply(bot, randomUnit, FSB_ChatterCategory::emote_joke, FSB_ReplyType::Say, FSB_ChatterSource::Target);

            return true;
        }

        return false;
    }

    bool BotOOCActionKiss(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        if (player)
        {
            auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
            if (!baseAI)
                return false;
            auto& botGroup = baseAI->botLogicalGroup;
            auto botRace = baseAI->botRace;
            auto botGender = FSBMgr::Get()->GetBotGenderForEntry(bot->GetEntry());
            auto soundInfo = sDB2Manager.GetTextSoundEmoteFor(TEXT_EMOTE_KISS, FSBUtils::BotRaceToTC(botRace), botGender, 0);
            uint32 soundId = 0;
            if (soundInfo)
            {
                soundId = soundInfo->SoundID;
            }

            Unit* randomUnit = nullptr;

            if (botGroup.size() >= 3)
            {
                do
                {
                    randomUnit = Trinity::Containers::SelectRandomContainerElement(botGroup);
                } while (randomUnit == bot);
            }
            else if (player)
            {
                randomUnit = player;
            }
            else return false;

            if (randomUnit)
            {
                TC_LOG_INFO("scripts.fsb.ooc", "FSB: BotOOCActionKiss started for Bot {}", bot->GetName());

                bot->SetFacingToObject(randomUnit, true);

                // 1. Emote text
                std::string emote = FSBChatter::GetRandomReply(bot, randomUnit, FSB_ChatterCategory::emote_kiss, FSB_ChatterType::None, 0);
                if (!emote.empty())
                    bot->TextEmote(emote, randomUnit);

                bot->HandleEmoteCommand(EMOTE_ONESHOT_KISS);
                if (soundId)
                    bot->PlayDistanceSound(soundId);
                else TC_LOG_WARN("scripts.fsb.ooc", "FSB: BotOOCActionKiss: no sound found for race {}", botRace);

                if (!randomUnit->IsPlayer())
                    FSBChatter::DemandTimedReply(bot, randomUnit, FSB_ChatterCategory::emote_kiss, FSB_ReplyType::Say, FSB_ChatterSource::Target);
                return true;
            }
        }
        return false;
    }

    bool BotOOCActionFlirt(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;
        auto& botGroup = baseAI->botLogicalGroup;
        auto botRace = baseAI->botRace;
        auto botGenger = FSBMgr::Get()->GetBotGenderForEntry(bot->GetEntry());
        auto soundInfo = sDB2Manager.GetTextSoundEmoteFor(TEXT_EMOTE_FLIRT, FSBUtils::BotRaceToTC(botRace), botGenger, 0);
        uint32 soundId = 0;
        if (soundInfo)
        {
            soundId = soundInfo->SoundID;
        }

        Unit* randomUnit = nullptr;
        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        if (botGroup.size() >= 3)
        {
            do
            {
                randomUnit = Trinity::Containers::SelectRandomContainerElement(botGroup);
            } while (randomUnit == bot);
        }
        else if (player)
        {
            randomUnit = player;
        }
        else return false;

        if (randomUnit)
        {
            TC_LOG_INFO("scripts.fsb.ooc", "FSB: BotOOCActionFlirt started for Bot {}", bot->GetName());

            bot->SetFacingToObject(randomUnit, true);

            // 1. Emote text
            std::string emote = FSBChatter::GetRandomReply(bot, randomUnit, FSB_ChatterCategory::emote_flirt, FSB_ChatterType::None, 0);
            if (!emote.empty())
                bot->TextEmote(emote);

            bot->HandleEmoteCommand(EMOTE_ONESHOT_SHY);
            if (soundId)
                bot->PlayDistanceSound(soundId);
            else TC_LOG_WARN("scripts.fsb.ooc", "FSB: BotOOCActionFlirt: no sound found for race {}", botRace);

            if (!randomUnit->IsPlayer())
                FSBChatter::DemandTimedReply(bot, randomUnit, FSB_ChatterCategory::emote_flirt, FSB_ReplyType::Say, FSB_ChatterSource::Target);

            return true;
        }

        return false;
    }

    bool BotOOCActionCook(Creature* bot, uint32 spellId, uint32 goId)
    {
        if (!bot || !bot->IsAlive())
            return false;

        // 0. Check if there is a fire already
        GameObject* gobject = GetClosestGameObjectWithEntry(bot, goId, 20.f);
        if (gobject)
            return false;

        if (bot->GetSpellHistory()->HasCooldown(spellId))
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto& botByFire = baseAI->botSitsByFire;

        if (!botByFire)
        {
            // 1. Cast campfire spell
            if (FSBSpells::BotCastSpell(bot, spellId, bot))
            {
                TC_LOG_INFO("scripts.fsb.ooc", "FSB: BotOOCActionCook started for bot {} with event {}", bot->GetName(), FSBSpellsUtils::GetSpellName(spellId));
                FSBChatter::DemandTimedReply(bot, nullptr, FSB_ChatterCategory::emote_cooking, FSB_ReplyType::Say, FSB_ChatterSource::Bot);
                return true;
            }
        }

        return false;
    }

    bool BotActionsNotHired(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        if (baseAI->botHired)
            return false;

        FSBChat::UpdateBotConversations();

        if (BotChatActionsNotHired(bot))
            return true;

        return false;
    }

    bool BotChatActionsNotHired(Creature* bot)
    {
        if (!FollowshipBotsConfig::configFSBUseChatChannels)
            return false;

        if (!bot || !bot->IsAlive())
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        if (baseAI->botHired)
            return false;

        uint32 now = getMSTime();

        // 1. If bot is already in a conversation, do nothing
        if (FSBChat::IsBotInConversation(bot))
            return false;

        // 2. Check timer correctly
        if (now < baseAI->botChatChannelsTimer)
            return false;

        int32 offset = urand(1000, 10000);

        // 3.1 Combat message when bot is attacked
        // This is in addition to the JustEngagedWith() trigger
        // Timer should ensure we are not spamming these
        if (bot->IsInCombat())
        {
            FSBChat::StartBotRandomChat(bot, ChatChannelType::CombatDefense);

            baseAI->botChatChannelsTimer = now + FollowshipBotsConfig::configFSBChatChannelsInterval + offset;

            return true;
        }

        // 3.2 Random chance to start a conversation or random chat
        if (urand(0, 99) > FollowshipBotsConfig::configFSBChatChannelsRate)
        {
            baseAI->botChatChannelsTimer = now + FollowshipBotsConfig::configFSBChatChannelsInterval + offset;
            return false;
        }

        int8 action = urand(1, 3);

        // 4. Start conversation
        if(action == 1)
            FSBChat::StartBotConversation(bot);

        // 5. Start Random Chat
        if (action == 2)
            FSBChat::StartBotRandomChat(bot, ChatChannelType::General);

        if (action == 3 && FSBUtils::IsBotInTradeCity(bot))
            FSBChat::StartBotRandomChat(bot, ChatChannelType::Trade);

        // 6. Set next allowed time (store future timestamp)
        baseAI->botChatChannelsTimer = now + FollowshipBotsConfig::configFSBChatChannelsInterval + offset;

        return true;
    }
}

