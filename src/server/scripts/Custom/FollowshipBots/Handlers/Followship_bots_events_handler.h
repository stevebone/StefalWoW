enum FSB_BOT_EVENTS
{
    FSB_EVENT_HIRED_MOUNT_AURA = 1,
    FSB_EVENT_HIRED_RESURRECT_TARGET,
    FSB_EVENT_HIRED_RESUME_FOLLOW,
    FSB_EVENT_HIRED_DESPAWN_TEMP_BOT,
    FSB_EVENT_HIRED_CHECK_MEMBER_DEATH,
    FSB_EVENT_HIRED_SPELL_RESURRECT_STATE,
    FSB_EVENT_HIRED_TIMED_CHATTER_REPLY,
    FSB_EVENT_HIRED_TIMED_DUMMY_EMOTE,

    FSB_EVENT_GENERIC_CHECK_HIRED_TIME,

    FSB_EVENT_RANDOM_ACTION_SIT_BY_FIRE,
    FSB_EVENT_RANDOM_ACTION_MOVE_FIRE,
    FSB_EVENT_RANDOM_ACTION_FINISH,
};

namespace FSBEvents
{
    // Wrapper for scheduling a bot event directly from Creature*
    void ScheduleBotEvent(Creature* bot, uint32 eventId, std::chrono::milliseconds minTime, std::chrono::milliseconds maxTime = std::chrono::milliseconds(0));
    void ScheduleBotEventWithChatter(Creature* bot, uint32 eventId, std::chrono::milliseconds minTime, std::chrono::milliseconds maxTime = std::chrono::milliseconds(0), FSB_ReplyType replyType = FSB_ReplyType::None, std::string chatterReply = "", Unit* target = nullptr);
}
