#pragma once

class Player;
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
    ConversationTemplate* tmpl;
    std::vector<Creature*> participants;
    std::map<ConversationRole, size_t> roleMap;   // NEW
    size_t currentLine = 0;
    uint32 nextSpeakTime = 0;
};

namespace FSBChat
{
    Player* GetAnyPlayer();

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

}
