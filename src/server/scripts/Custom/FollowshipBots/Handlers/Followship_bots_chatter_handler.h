
enum FSB_ChatterCategory
{
    chatter_none,
    targetKilled,
    botDismissed,
    botAcknowledge,
    botFollow,
    botStay,
    botMemberDied,
    botRevived,
    botDeathHired,
    botDeath,
    botHiredPermanent,
    botRevivedTarget,
    botHealTarget,
    botHealSelf,
    botCombatMana,
    botCombatHealth,
    botCombatSpell,
    botBuffSelf,
    botBuffTarget,
    botOOCRecovery,
    botOOCRecoveryHired,

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
    std::string GetRandomReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ChatterType chatterType, uint32 spellId);
    void DemandTimedReply(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource);
    void DemandBotChatter(Creature* bot, Unit* target, FSB_ChatterCategory category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource, uint32 spellId);

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to);
}
