#include "Followship_bots_ai_base.h"
#include "Followship_bots_mgr.h"

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


void FSB_BaseAI::HandleBotEvent(FSB_BaseAI* ai, uint32 eventId)
{
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
}
