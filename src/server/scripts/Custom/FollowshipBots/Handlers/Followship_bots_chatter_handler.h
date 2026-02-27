
struct FSBChatterReplyEntry
{
    std::string category;
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
    std::string GetRandomReply(Creature* bot, Unit* target, const std::string& category, FSB_ChatterType chatterType);
    void DemandTimedReply(Creature* bot, Unit* target, const std::string& category, FSB_ReplyType replyType, FSB_ChatterSource chatterSource);

    void ReplaceAll(std::string& text, const std::string& from, const std::string& to);
}
