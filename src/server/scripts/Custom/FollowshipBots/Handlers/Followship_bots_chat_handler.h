/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "ObjectGuid.h"
#include "LlamaAI/Followship_bots_channel_prompts.h"
#include "LlamaAI/Followship_bots_conversation_prompts.h"

class Channel;
class Creature;
class Player;

class FSBChatMgr
{
public:
    static FSBChatMgr* Get();

    // Active bot registry for /who and whisper routing
    void RegisterActiveBot(Creature* bot);
    void UnregisterActiveBot(Creature* bot);
    std::vector<Creature*> GetActiveBots();
    Creature* FindActiveBotByName(std::string const& name) const;
    Creature* FindActiveBotByGuid(ObjectGuid guid) const;
    static std::string SanitizeBotName(std::string const& name);

    // Bot channel membership
    void JoinBotChannels(Creature* bot);
    void LeaveBotChannels(Creature* bot);
    void UpdateBotChannels(Creature* bot);

    // Player chat reply handler
    void HandlePlayerGeneralChat(Player* player, Channel* channel, std::string const& msg);
    void HandleBotWhisper(Player* player, Creature* bot, std::string const& msg);

    // Global update (called from WorldScript::OnUpdate)
    void Update(uint32 diff);

private:
    std::unordered_map<ObjectGuid, Creature*> _activeBots;
    mutable std::mutex _activeBotsMutex;
    std::unordered_map<ObjectGuid, uint32> _botReplyCooldowns;
};

namespace FSBChat
{
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
        std::map<ConversationRole, size_t> roleMap;
        size_t currentLine = 0;
        uint32 nextSpeakTime = 0;

        // LlamaAI fields
        bool isLlamaGenerated = false;
        FSBConvPrompts::ConversationTopic topic;
        std::string topicDescription;
        std::vector<std::pair<std::string, std::string>> history; // {speakerName, line}
        size_t maxLines = 0;
        size_t currentSpeakerIndex = 0;
        std::shared_ptr<FSBConvPrompts::ConversationLlamaState> convLlamaState;
    };

    enum class ChatChannelType
    {
        General,
        Trade,
        LocalDefense,
        CombatDefense, // is basically for LocalDefense but specific to bot combat
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

    void BotSendGeneralChat(Creature* bot, std::string const& msg);
    void BotSendTradeChat(Creature* bot, std::string const& msg);
    void BotSendLocalDefenseChat(Creature* bot, std::string const& msg);
    void BotSendLFGChat(Creature* bot, std::string const& msg);

    extern std::vector<ConversationTemplate> ChatTables;
    extern std::vector<ActiveConversation> activeConversations;
    size_t CountUniqueRoles(const ConversationTemplate& tmpl);

    void StartBotConversation(Creature* starter);
    void StartBotLlamaConversation(Creature* starter);
    void UpdateBotConversations();
    bool IsBotInConversation(Creature* bot);

    extern std::vector<RandomChatTemplate> RandomChatTables;

    void StartBotRandomChat(Creature* bot, ChatChannelType channel);

    // used for chat initiated on combat
    void StartBotRandomChat(Creature* bot, ChatChannelType channel, Unit* attacker);

    std::string BuildItemLink(uint32 itemId, uint32 level = 0);
}
