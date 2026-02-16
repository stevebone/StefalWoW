enum FSB_BOT_EVENTS
{
    FSB_EVENT_HIRED_MOUNT_AURA = 1,
    FSB_EVENT_HIRED_RESURRECT_TARGET = 2,
    FSB_EVENT_HIRED_RESUME_FOLLOW = 3,
    FSB_EVENT_HIRED_DESPAWN_TEMP_BOT = 4,

    FSB_EVENT_GENERIC_CHECK_HIRED_TIME = 5,
};

namespace FSBEvents
{
    // Wrapper for scheduling a bot event directly from Creature*
    void ScheduleBotEvent(Creature* bot, uint32 eventId, std::chrono::milliseconds minTime, std::chrono::milliseconds maxTime = std::chrono::milliseconds(0));
}
