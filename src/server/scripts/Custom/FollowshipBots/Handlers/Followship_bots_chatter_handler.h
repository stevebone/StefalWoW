
enum FSB_ChatterCategory
{
    chatter_none,
    targetKilled,
    botDismissed,
    botAcknowledge,
    botFollow,
    botStay,

    emote_oom,
    emote_heal,
    emote_help,
    emote_whistle,
    emote_sleep,
    emote_sigh,

    emote_kiss,
    emote_flirt,
    emote_joke,
    emote_talk,
    emote_cooking,

    whisper_afk,
    

};

struct FSBChatterReplyEntry
{
    FSB_ChatterCategory category;
    FSB_ChatterType chatterType;
    std::vector<std::string> lines;
};

enum FSB_ChatterSource
{
    None,
    Bot,
    Target
};

namespace FSBChatter
{
    std::string GetRandomReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ChatterType chatterType);
    void DemandTimedReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource);
    void OnKilledTargetChatter(Creature* bot, Unit* victim);

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to);
}
