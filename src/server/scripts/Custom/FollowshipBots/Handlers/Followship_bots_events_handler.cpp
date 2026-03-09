#include "DB2Stores.h"
#include "GameObject.h"
#include "Log.h"

#include "Followship_bots.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_death_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_outofcombat_handler.h"

#include "Followship_bots_paladin.h"
#include "Followship_bots_rogue.h"

void FSB_BaseAI::ScheduleBotEvent(uint32 eventId, Milliseconds time)
{
    botEvents.ScheduleEvent(eventId, time);
}

void FSB_BaseAI::ScheduleBotEvent(uint32 eventId, Milliseconds minTime, Milliseconds maxTime)
{
    if (minTime > maxTime)
        std::swap(minTime, maxTime);

    auto delay = std::chrono::milliseconds(urand(minTime.count(), maxTime.count()));

    botEvents.ScheduleEvent(eventId, delay);
}

void FSB_BaseAI::ScheduleBotEvent(uint32 eventId, Milliseconds minTime, Milliseconds maxTime, FSB_ReplyType replyType, const std::string& chatterString, Unit* target)
{
    if (minTime > maxTime)
        std::swap(minTime, maxTime);

    auto delay = std::chrono::milliseconds(urand(minTime.count(), maxTime.count()));

    // Store payload
    eventPayloads[eventId] = FSBEventPayload(replyType, chatterString, target);


    // Schedule event normally
    botEvents.ScheduleEvent(eventId, delay);
}


void FSB_BaseAI::HandleBotEvent(FSB_BaseAI* ai, uint32 eventId)
{
    auto it = eventPayloads.find(eventId);

    if (it != eventPayloads.end())
    {
        FSBEventPayload payload = it->second; // copy
        eventPayloads.erase(it);

        Unit* target = payload.unit; // allow nullptr

        HandleBotEvent(ai, eventId, payload.replyType, payload.chatterString, target);
        return;
    }



    Creature* bot = ai->me;
    if (!bot)
        return;

    switch (eventId)
    {
    case FSB_EVENT_GENERIC_CHECK_HIRED_TIME:
    {
        uint64 spawnId = bot->GetSpawnId();
        if (spawnId)
        {
            PlayerBotData* botData = FSBMgr::Get()->GetPersistentBotBySpawnId(spawnId);
            if (botData)
            {
                if (FSBMgr::Get()->IsPersistentBotExpired(botData->owner, botData->entry))
                    FSBMgr::Get()->RemovePersistentBot(botData->owner, botData->entry);
            }
        }
        ScheduleBotEvent(FSB_EVENT_GENERIC_CHECK_HIRED_TIME, 10s);
        break;
    }

    case FSB_EVENT_HIRED_MOUNT_AURA:
        bot->CastSpell(bot, SPELL_PALADIN_CRUSADER_AURA, false);
        bot->Say("This should help us go faster", LANG_UNIVERSAL);
        break;

    case FSB_EVENT_HIRED_RESURRECT_TARGET:
        FSBOOC::BotOOCResurrectTarget(bot);
        break;

    case FSB_EVENT_HIRED_RESUME_FOLLOW:
    {
        auto followDistance = ai->botFollowDistance;
        auto followAngle = ai->botFollowAngle;
        if(ai->botHired)
            FSBMovement::ResumeFollow(bot, followDistance, followAngle);
        break;
    }

    case FSB_EVENT_HIRED_DESPAWN_TEMP_BOT:
    {
        if (bot->GetSpawnId() == 0 && !FSBMgr::Get()->GetBotOwner(bot))
            bot->DespawnOrUnsummon(0s);
        else botEvents.ScheduleEvent(FSB_EVENT_HIRED_DESPAWN_TEMP_BOT, 1s);
        break;
    }

    case FSB_EVENT_HIRED_CHECK_MEMBER_DEATH:
        FSBDeath::CheckBotMemberDeath(bot);
        break;

    case FSB_EVENT_HIRED_SPELL_RESURRECT_STATE:
        FSBDeath::HandleSpellResurrectionDelayedAction(bot);
        break;

    case FSB_EVENT_RANDOM_ACTION_MOVE_FIRE:
    {
        GameObject* go = nullptr;

        // check if we need to move to a camp fire (if someone made one already)
        if(GameObject* campfire = GetClosestGameObjectWithEntry(bot, 266354, 15.f))
            go = campfire;

        if(GameObject* cookingpot = GetClosestGameObjectWithEntry(bot, 379147, 15.f))
            go = cookingpot;

        if(GameObject* sausages = GetClosestGameObjectWithEntry(bot, 236110, 15.f))
            go = sausages;

        if (!go)
            break;

        if (ai->botSitsByFire)
            break;

        if (bot->GetEmoteState() == EMOTE_STATE_DANCE || bot->GetEmoteState() == EMOTE_ONESHOT_SLEEP)
            break;

        if (bot->GetStandState() == UNIT_STAND_STATE_SIT || bot->GetStandState() == UNIT_STAND_STATE_SLEEP)
            break;

        if (go)
        {
            ai->botSitsByFire = true;
            float offsetx = RAND(-2.f, 2.f);
            float offsety = frand(-2.f, 2.f);
            bot->GetMotionMaster()->MovePoint(FSB_MOVEMENT_POINT_NEAR_FIRE, go->GetPositionX() + offsetx, go->GetPositionY() + offsety, go->GetPositionZ());
            botEvents.ScheduleEvent(FSB_EVENT_RANDOM_ACTION_FINISH, 30s, 45s);
            TC_LOG_DEBUG("scripts.fsb.events", "FSB: Event RANDOM_ACTION_MOVE_FIRE for bot {} finished. We found object {} and are moving to sit by", bot->GetName(), go->GetName());
            break;
        }

        break;
    }

    case FSB_EVENT_RANDOM_ACTION_SIT_BY_FIRE:
    {
        if (!bot || !bot->IsAlive())
            return;

        GameObject* go = nullptr;

        // 3. Find nearest gameobject camp fire /  cooking pot / sausages
        if(GameObject* campfire = GetClosestGameObjectWithEntry(me, 266354, 5.f))
            go = campfire;
        if(GameObject* cookingpot = GetClosestGameObjectWithEntry(me, 379147, 5.f))
            go = cookingpot;
        if(GameObject* sausages = GetClosestGameObjectWithEntry(me, 236110, 5.f))
            go = sausages;

        if(go)
            bot->SetFacingToObject(go);

        if (bot->HasAura(SPELL_ROGUE_STEALTH))
            bot->RemoveAurasDueToSpell(SPELL_ROGUE_STEALTH);

        // 5. Sit down
        me->SetStandState(UNIT_STAND_STATE_SIT);

        // Optional: emote text
        std::string emote = me->GetName() + " sits by the fire.";
        me->TextEmote(emote);

        if(go)
            TC_LOG_DEBUG("scripts.fsb.events", "FSB: Event RANDOM_ACTION_SIT_BY_FIRE for bot {} finished. We found object {} and are sitting down.", bot->GetName(), go->GetName());

        break;
    }

    case FSB_EVENT_RANDOM_ACTION_FINISH:
    {
        // cleanup event for random bot actions
        auto& botByFire = ai->botSitsByFire;
        auto& botDoingRandomEvent = ai->botDoingRandomEvent;
        auto fDistance = ai->botFollowDistance;
        auto fAngle = ai->botFollowAngle;
        auto moveState = ai->botMoveState;

        if (!botDoingRandomEvent && !botByFire)
            break;

        GameObject* campfire = GetClosestGameObjectWithEntry(bot, 266354, 5.f);
        GameObject* cookingpot = GetClosestGameObjectWithEntry(bot, 379147, 5.f);
        GameObject* sausages = GetClosestGameObjectWithEntry(bot, 236110, 5.f);
        if(campfire)
            campfire->DespawnOrUnsummon(0s);

        if (cookingpot)
            cookingpot->DespawnOrUnsummon(0s);

        if (sausages)
            sausages->DespawnOrUnsummon(0s);

        if (botByFire)
            botByFire = false;

        if (bot->GetStandState() == UNIT_STAND_STATE_SIT || bot->GetStandState() == UNIT_STAND_STATE_SLEEP)
            bot->SetStandState(UNIT_STAND_STATE_STAND);

        if (bot->HasAura(SPELL_SHAMAN_GHOST_WOLF))
            bot->RemoveAurasDueToSpell(SPELL_SHAMAN_GHOST_WOLF);

        //bot->SetEmoteState(Emote(EMOTE_STATE_NONE));
        bot->HandleEmoteCommand(EMOTE_ONESHOT_YES);

        if(moveState != FSB_MOVE_STATE_STAY)
            FSBMovement::ResumeFollow(bot, fDistance, fAngle);

        botDoingRandomEvent = false;

        TC_LOG_DEBUG("scripts.fsb.events", "FSB: Event RANDOM_ACTION_FINISH for bot {} ended. Cleaning up states and flags", bot->GetName());

        break;
    }

    case FSB_EVENT_HIRED_EXPIRED: // or dismissed
    {
        if (!bot->IsInCombat() && !bot->HasUnitState(UNIT_STAND_STATE_SIT) && bot->IsAlive())
        {
            botEvents.Reset();
            ai->botHired = false;
            FSBMgr::Get()->DismissPersistentBot(me);
            ScheduleBotEvent(FSB_EVENT_HIRED_LEAVE, 5s);
            break;
        }
        else ScheduleBotEvent(FSB_EVENT_HIRED_EXPIRED, 5s);
        break;
    }

    case FSB_EVENT_HIRED_LEAVE:
    {
        float x, y, z;
        me->GetRandomPoint(me->GetPosition(), 50.0f, x, y, z);
        me->GetMotionMaster()->MovePoint(1, x, y, z);
        me->DespawnOrUnsummon(10s);

        break;
    }

    default:
        break;
    }
}

void FSB_BaseAI::HandleBotEvent(FSB_BaseAI* ai, uint32 eventId, FSB_ReplyType replyType, std::string chatterReply, Unit* target)
{
    Creature* bot = ai->me;
    std::string tname = "";
    if (target)
        tname = target->GetName();
    
    if (!bot)
        return;

    switch (eventId)
    {
    case FSB_EVENT_HIRED_TIMED_CHATTER_REPLY:
    {
        TC_LOG_DEBUG("scripts.fsb.events", "FSB: Events TIMED_CHATTER_REPLY bot {} with target {} and chatterString: {}", bot->GetName(), tname, chatterReply);

        switch (replyType)
        {
        case FSB_ReplyType::Say:
            if(target && target->IsAlive())
                target->Say(chatterReply, LANG_UNIVERSAL);
            else bot->Say(chatterReply, LANG_UNIVERSAL);
            break;
        case FSB_ReplyType::Yell:
            if(target && target->IsAlive())
                target->Yell(chatterReply, LANG_UNIVERSAL);
            else bot->Yell(chatterReply, LANG_UNIVERSAL);
            break;
        case FSB_ReplyType::Whisper:
        {
            Player* player = FSBMgr::Get()->GetBotOwner(bot);
            if (!player)
                break;

            bot->Whisper(chatterReply, LANG_UNIVERSAL, player, false);

            break;
        }
        default:
            break;
        }
    }
        break;

    case FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE:
    {
        if (!target)
            target = bot;

        if (target->IsPlayer())
            break;

        auto targetAI = dynamic_cast<FSB_BaseAI*>(target->ToCreature()->AI());
        auto targetRace = targetAI->botRace;
        auto targetGender = FSBMgr::Get()->GetBotGenderForEntry(target->GetEntry());

        TextEmotes tEmote = TEXT_EMOTE_AGREE;
        Emote emote = EMOTE_ONESHOT_NONE;

        if (chatterReply == "emote:laugh")
        {
            tEmote = TEXT_EMOTE_LAUGH;
            emote = EMOTE_ONESHOT_LAUGH;
        }
        else if (chatterReply == "emote:rudeno")
        {
            tEmote = RAND(TEXT_EMOTE_NO, TEXT_EMOTE_RASP);
            emote = (tEmote == TEXT_EMOTE_NO) ? EMOTE_ONESHOT_NO : EMOTE_ONESHOT_RUDE;
        }
        else if (chatterReply == "emote:oom")
            tEmote = TEXT_EMOTE_OOM;
        else if (chatterReply == "emote:heal")
            tEmote = TEXT_EMOTE_HEALME;
        else if (chatterReply == "emote:help")
            tEmote = TEXT_EMOTE_HELPME;

        auto soundInfo = sDB2Manager.GetTextSoundEmoteFor(tEmote, FSBUtils::BotRaceToTC(targetRace), targetGender, 0);
        uint32 soundId = 0;
        if (soundInfo)
        {
            soundId = soundInfo->SoundID;
        }
        target->HandleEmoteCommand(emote);
        if (soundId)
            target->PlayDistanceSound(soundId);
        else TC_LOG_WARN("scripts.fsb.events", "FSB: Events AFK Action {}: no sound found for race {}", chatterReply, targetRace);

        break;
    }

    default:
        break;
    }
}

namespace FSBEvents
{
    // Wrapper for scheduling a bot event directly from Creature*
    void ScheduleBotEvent(Creature* bot, uint32 eventId, std::chrono::milliseconds minTime, std::chrono::milliseconds maxTime)
    {
        if (!bot)
            return;

        if (auto ai = bot->AI())
        {
            if (auto baseAI = dynamic_cast<FSB_BaseAI*>(ai))
            {
                if (maxTime.count() == 0) // only minTime provided
                    baseAI->ScheduleBotEvent(eventId, minTime);
                else
                    baseAI->ScheduleBotEvent(eventId, minTime, maxTime);
            }
        }
    }

    // Wrapper for scheduling a bot event directly from Creature*
    void ScheduleBotEventWithChatter(Creature* bot, uint32 eventId, std::chrono::milliseconds minTime, std::chrono::milliseconds maxTime, FSB_ReplyType replyType, std::string chatterString, Unit* target)
    {
        if (!bot)
            return;

        if (auto ai = bot->AI())
        {
            if (auto baseAI = dynamic_cast<FSB_BaseAI*>(ai))
            {
                if (maxTime.count() == 0) // only minTime provided
                    baseAI->ScheduleBotEvent(eventId, minTime);
                else
                    baseAI->ScheduleBotEvent(eventId, minTime, maxTime, replyType, chatterString, target);
            }
        }
    }
}
