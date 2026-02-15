#include "Followship_bots_ai_base.h"

#include "Followship_bots_events_handler.h"

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
        // Example placeholder
    case FSB_EVENT_HIRED_MOUNT_AURA:
        bot->CastSpell(bot, SPELL_PALADIN_CRUSADER_AURA, false);
        bot->Say("This should help us go faster", LANG_UNIVERSAL);
        // Do delayed hire logic
        break;

    default:
        break;
    }
}

namespace FSBEvents
{
    // Wrapper for scheduling a bot event directly from Creature*
    void ScheduleBotEvent(Creature* bot, uint32 eventId, std::chrono::milliseconds minTime, std::chrono::milliseconds maxTime = std::chrono::milliseconds(0))
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
