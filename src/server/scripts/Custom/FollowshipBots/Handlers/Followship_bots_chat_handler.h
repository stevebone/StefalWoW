#pragma once

class Creature;

enum class ConversationRole
{
    Starter,
    Responder,
    Follower,
    Extra
};

struct ConversationLine
{
    ConversationRole role;
    std::string text;
};

struct ConversationTemplate
{
    std::string topic;
    std::vector<ConversationLine> lines;
};

struct ActiveConversation
{
    ConversationTemplate* tmpl = nullptr;
    std::vector<Creature*> participants;
    std::map<ConversationRole, size_t> roleMap;   // NEW
    size_t currentLine = 0;
    uint32 nextSpeakTime = 0;
};

enum class ChatChannelType
{
    General,
    Trade,
    LocalDefense,
    LFG,
    Custom
};

struct RandomChatTemplate
{
    uint32 zoneId;                     // 0 = any zone
    ChatChannelType channel;           // General, Trade, LFG, etc.
    Team team;                       // Alliance, Horde, Neutral
    uint32 spellId;
    uint32 itemId;
    std::string text;                  // The actual chat line
};

namespace FSBChat
{
    void BotSendGeneralChat(Creature* bot, std::string const& msg);
    void BotSendTradeChat(Creature* bot, std::string const& msg);
    void BotSendLocalDefenseChat(Creature* bot, std::string const& msg);
    void BotSendLFGChat(Creature* bot, std::string const& msg);

    extern std::vector<ConversationTemplate> ChatTables;
    extern std::vector<ActiveConversation> activeConversations;
    size_t CountUniqueRoles(const ConversationTemplate& tmpl);

    void StartBotConversation(Creature* starter);
    void UpdateBotConversations();
    bool IsBotInConversation(Creature* bot);

    extern std::vector<RandomChatTemplate> RandomChatTables;

    void StartBotRandomChat(Creature* bot, ChatChannelType channel);
}
