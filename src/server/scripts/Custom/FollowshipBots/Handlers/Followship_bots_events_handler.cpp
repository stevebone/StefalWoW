#include "Followship_bots.h"
#include "Followship_bots_ai_base.h"
#include "Followship_bots_mgr.h"

#include "Followship_bots_death_handler.h"
#include "Followship_bots_events_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_outofcombat_handler.h"

#include "Followship_bots_paladin.h"

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
    eventPayloads[eventId] = {
        replyType,
        chatterString,
        target ? target : nullptr };


    // Schedule event normally
    botEvents.ScheduleEvent(eventId, delay);
}


void FSB_BaseAI::HandleBotEvent(FSB_BaseAI* ai, uint32 eventId)
{
    auto it = eventPayloads.find(eventId);

    if (it != eventPayloads.end())
    {
        auto payload = it->second;
        eventPayloads.erase(it);

        // Resolve target
        Unit* target = nullptr;
        if (payload.unit)
            target = payload.unit;

        // Forward to the new overload
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
        // check if we need to move to a camp fire (if someone made one already)
        GameObject* campfire = GetClosestGameObjectWithEntry(bot, 266354, 20.f);
        GameObject* cookingpot = GetClosestGameObjectWithEntry(bot, 379147, 20.f);
        GameObject* sausages = GetClosestGameObjectWithEntry(bot, 236110, 20.f);
        if (!campfire && !cookingpot && !sausages)
            break;

        if (ai->botSitsByFire)
            break;

        if (campfire && bot->GetStandState() != UNIT_STAND_STATE_SIT)
        {
            ai->botSitsByFire = true;
            float offsetx = RAND(-2.f, 2.f);
            float offsety = frand(-2.f, 2.f);
            bot->GetMotionMaster()->MovePoint(FSB_MOVEMENT_POINT_NEAR_FIRE, campfire->GetPositionX() + offsetx, campfire->GetPositionY() + offsety, campfire->GetPositionZ());
            botEvents.ScheduleEvent(FSB_EVENT_RANDOM_ACTION_FINISH, 30s, 45s);
            break;
        }

        else if (cookingpot && bot->GetStandState() != UNIT_STAND_STATE_SIT)
        {
            ai->botSitsByFire = true;
            float offsetx = RAND(-2.f, 2.f);
            float offsety = frand(-2.f, 2.f);
            bot->GetMotionMaster()->MovePoint(FSB_MOVEMENT_POINT_NEAR_FIRE, cookingpot->GetPositionX() + offsetx, cookingpot->GetPositionY() + offsety, cookingpot->GetPositionZ());
            botEvents.ScheduleEvent(FSB_EVENT_RANDOM_ACTION_FINISH, 30s, 45s);
            break;
        }

        else if (sausages && bot->GetStandState() != UNIT_STAND_STATE_SIT)
        {
            ai->botSitsByFire = true;
            float offsetx = RAND(-2.f, 2.f);
            float offsety = frand(-2.f, 2.f);
            bot->GetMotionMaster()->MovePoint(FSB_MOVEMENT_POINT_NEAR_FIRE, sausages->GetPositionX() + offsetx, sausages->GetPositionY() + offsety, sausages->GetPositionZ());
            botEvents.ScheduleEvent(FSB_EVENT_RANDOM_ACTION_FINISH, 30s, 45s);
            break;
        }
        break;
    }

    case FSB_EVENT_RANDOM_ACTION_FINISH:
    {

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB Bot event: RANDOM ACTION FINISH for bot {}", bot->GetName());
        // cleanup event for random bot actions
        auto& botByFire = ai->botSitsByFire;
        auto& botDoingRandomEvent = ai->botDoingRandomEvent;
        auto fDistance = ai->botFollowDistance;
        auto fAngle = ai->botFollowAngle;

        if (!botDoingRandomEvent && !botByFire)
            break;

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB Bot event: RANDOM ACTION FINISH (cleanup) for bot {}", bot->GetName());
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

        FSBMovement::ResumeFollow(bot, fDistance, fAngle);

        botDoingRandomEvent = false;

        break;
    }

    default:
        break;
    }
}

void FSB_BaseAI::HandleBotEvent(FSB_BaseAI* ai, uint32 eventId, FSB_ReplyType replyType, std::string chatterReply, Unit* target)
{
    Creature* bot = ai->me;
    
    if (!bot)
        return;

    TC_LOG_DEBUG("scripts.ai.fsb", "FSB ChatterEvents: we got chatterString: {}", chatterReply);

    switch (eventId)
    {
    case FSB_EVENT_HIRED_TIMED_CHATTER_REPLY:
    {
        switch (replyType)
        {
        case FSB_ReplyType::Say:
            if(target)
                target->Say(chatterReply, LANG_UNIVERSAL);
            else bot->Say(chatterReply, LANG_UNIVERSAL);
            break;
        case FSB_ReplyType::Yell:
            if(target)
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
