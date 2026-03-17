/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * ChatTemplateManager implementation.
 */

#include "ChatTemplateManager.h"
#include "Database/PlayerbotDatabase.h"
#include "Player.h"
#include "Log.h"
#include "Unit.h"
#include "GameTime.h"
#include "WorldSession.h"
#include <algorithm>
#include <numeric>

namespace Playerbot
{

// ============================================================================
// ChatTemplate / EmoteTemplate matching
// ============================================================================

bool ChatTemplate::MatchesBot(Player const* bot) const
{
    if (!bot)
        return false;
    if (filterClass != 0 && bot->GetClass() != filterClass)
        return false;
    if (filterRace != 0 && bot->GetRace() != filterRace)
        return false;
    if (filterMinLevel != 0 && bot->GetLevel() < filterMinLevel)
        return false;
    if (filterMaxLevel != 0 && bot->GetLevel() > filterMaxLevel)
        return false;
    return true;
}

bool EmoteTemplate::MatchesBot(Player const* bot) const
{
    if (!bot)
        return false;
    if (filterClass != 0 && bot->GetClass() != filterClass)
        return false;
    if (filterRace != 0 && bot->GetRace() != filterRace)
        return false;
    return true;
}

// ============================================================================
// Singleton
// ============================================================================

ChatTemplateManager& ChatTemplateManager::Instance()
{
    static ChatTemplateManager instance;
    return instance;
}

// ============================================================================
// Lifecycle
// ============================================================================

bool ChatTemplateManager::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("module.playerbot", "Loading chat/emote templates...");

    LoadChatTemplates();
    LoadEmoteTemplates();

    // If no DB templates found, load defaults
    if (_chatTemplates.empty() && _emoteTemplates.empty())
    {
        TC_LOG_INFO("module.playerbot", "No DB templates found, loading defaults");
        LoadDefaultTemplates();
    }

    uint32 chatCount = GetChatTemplateCount();
    uint32 emoteCount = GetEmoteTemplateCount();
    TC_LOG_INFO("module.playerbot", "Loaded {} chat templates and {} emote templates",
        chatCount, emoteCount);

    _initialized = true;
    return true;
}

void ChatTemplateManager::Shutdown()
{
    _chatTemplates.clear();
    _emoteTemplates.clear();

    std::lock_guard<std::mutex> lock(_cooldownMutex);
    _cooldownStates.clear();

    _initialized = false;
}

void ChatTemplateManager::ReloadTemplates()
{
    _chatTemplates.clear();
    _emoteTemplates.clear();

    LoadChatTemplates();
    LoadEmoteTemplates();

    if (_chatTemplates.empty() && _emoteTemplates.empty())
        LoadDefaultTemplates();

    TC_LOG_INFO("module.playerbot", "Reloaded {} chat and {} emote templates",
        GetChatTemplateCount(), GetEmoteTemplateCount());
}

// ============================================================================
// Template Loading
// ============================================================================

void ChatTemplateManager::LoadChatTemplates()
{
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT id, trigger_context, chat_type, message_text, "
        "filter_class, filter_race, filter_min_level, filter_max_level, "
        "weight, cooldown_ms, locale "
        "FROM playerbot_chat_templates ORDER BY trigger_context, weight DESC");

    if (!result)
        return;

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        ChatTemplate tmpl;
        tmpl.id             = fields[0].GetUInt32();
        tmpl.triggerContext  = fields[1].GetString();
        tmpl.chatType       = static_cast<TemplateChatType>(fields[2].GetUInt8());
        tmpl.messageText    = fields[3].GetString();
        tmpl.filterClass    = fields[4].GetUInt8();
        tmpl.filterRace     = fields[5].GetUInt8();
        tmpl.filterMinLevel = fields[6].GetUInt32();
        tmpl.filterMaxLevel = fields[7].GetUInt32();
        tmpl.weight         = fields[8].GetUInt32();
        tmpl.cooldownMs     = fields[9].GetUInt32();
        tmpl.locale         = fields[10].GetString();

        _chatTemplates[tmpl.triggerContext].push_back(std::move(tmpl));
        ++count;
    } while (result->NextRow());

    TC_LOG_DEBUG("module.playerbot", "Loaded {} chat templates from database", count);
}

void ChatTemplateManager::LoadEmoteTemplates()
{
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT id, trigger_context, emote_id, "
        "filter_class, filter_race, weight, cooldown_ms "
        "FROM playerbot_emote_templates ORDER BY trigger_context, weight DESC");

    if (!result)
        return;

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        EmoteTemplate tmpl;
        tmpl.id             = fields[0].GetUInt32();
        tmpl.triggerContext  = fields[1].GetString();
        tmpl.emoteId        = fields[2].GetUInt32();
        tmpl.filterClass    = fields[3].GetUInt8();
        tmpl.filterRace     = fields[4].GetUInt8();
        tmpl.weight         = fields[5].GetUInt32();
        tmpl.cooldownMs     = fields[6].GetUInt32();

        _emoteTemplates[tmpl.triggerContext].push_back(std::move(tmpl));
        ++count;
    } while (result->NextRow());

    TC_LOG_DEBUG("module.playerbot", "Loaded {} emote templates from database", count);
}

void ChatTemplateManager::LoadDefaultTemplates()
{
    // ================================================================
    // COMBAT TEMPLATES
    // ================================================================
    auto addChat = [this](std::string ctx, std::string text, TemplateChatType type = TemplateChatType::SAY,
                          uint32 weight = 100, uint32 cooldownMs = 60000, uint8 cls = 0)
    {
        ChatTemplate t;
        t.id = static_cast<uint32>(_chatTemplates.size()) + 1;
        t.triggerContext = std::move(ctx);
        t.chatType = type;
        t.messageText = std::move(text);
        t.filterClass = cls;
        t.weight = weight;
        t.cooldownMs = cooldownMs;
        _chatTemplates[t.triggerContext].push_back(std::move(t));
    };

    auto addEmote = [this](std::string ctx, uint32 emoteId, uint32 weight = 100, uint32 cooldownMs = 30000)
    {
        EmoteTemplate t;
        t.id = static_cast<uint32>(_emoteTemplates.size()) + 1;
        t.triggerContext = std::move(ctx);
        t.emoteId = emoteId;
        t.weight = weight;
        t.cooldownMs = cooldownMs;
        _emoteTemplates[t.triggerContext].push_back(std::move(t));
    };

    // Combat start
    addChat("combat_start", "Let's do this!", TemplateChatType::SAY, 100, 120000);
    addChat("combat_start", "Engaging!", TemplateChatType::SAY, 80, 120000);
    addChat("combat_start", "For the Alliance!", TemplateChatType::YELL, 40, 300000);
    addChat("combat_start", "For the Horde!", TemplateChatType::YELL, 40, 300000);
    addChat("combat_start", "Watch my back!", TemplateChatType::PARTY, 60, 120000);

    // Combat kill
    addChat("combat_kill", "Down!", TemplateChatType::SAY, 100, 60000);
    addChat("combat_kill", "One less to worry about.", TemplateChatType::SAY, 60, 60000);
    addChat("combat_kill", "That was close.", TemplateChatType::SAY, 40, 60000);

    // Combat death
    addChat("combat_death", "I need a rez!", TemplateChatType::PARTY, 100, 30000);
    addChat("combat_death", "I'm down!", TemplateChatType::PARTY, 80, 30000);

    // Low health
    addChat("low_health", "I need healing!", TemplateChatType::PARTY, 100, 15000);
    addChat("low_health", "Help!", TemplateChatType::YELL, 40, 30000);

    // Low mana / OOM
    addChat("low_mana", "Running low on mana.", TemplateChatType::PARTY, 100, 30000);
    addChat("oom", "OOM!", TemplateChatType::PARTY, 100, 20000);
    addChat("oom", "I'm out of mana!", TemplateChatType::PARTY, 80, 20000);

    // Greeting
    addChat("greeting", "Hey {target}!", TemplateChatType::SAY, 100, 300000);
    addChat("greeting", "Hello!", TemplateChatType::SAY, 80, 300000);
    addChat("greeting", "Good to see you, {target}.", TemplateChatType::SAY, 60, 300000);

    // Farewell
    addChat("farewell", "See you around, {target}!", TemplateChatType::SAY, 100, 300000);
    addChat("farewell", "Take care!", TemplateChatType::SAY, 80, 300000);
    addChat("farewell", "Later!", TemplateChatType::SAY, 60, 300000);

    // Thank you
    addChat("thank_you", "Thanks, {target}!", TemplateChatType::SAY, 100, 60000);
    addChat("thank_you", "Appreciate it!", TemplateChatType::SAY, 80, 60000);

    // Loot excitement
    addChat("loot_epic", "Wow, epic drop!", TemplateChatType::PARTY, 100, 30000);
    addChat("loot_rare", "Nice, a rare!", TemplateChatType::SAY, 80, 60000);

    // Ready check
    addChat("ready_check", "Ready!", TemplateChatType::PARTY, 100, 10000);
    addChat("ready_check", "Good to go.", TemplateChatType::PARTY, 80, 10000);

    // Buff request
    addChat("buff_request", "Can I get a buff?", TemplateChatType::PARTY, 100, 120000);

    // Resurrection request
    addChat("res_request", "Rez please!", TemplateChatType::PARTY, 100, 30000);

    // Group join/leave
    addChat("group_join", "Hey everyone!", TemplateChatType::PARTY, 100, 300000);
    addChat("group_leave", "Gotta go, thanks for the group!", TemplateChatType::PARTY, 100, 300000);

    // ================================================================
    // EMOTE TEMPLATES (Emote enum IDs from TrinityCore)
    // ================================================================

    // Greeting emotes
    addEmote("greeting", 3, 100, 60000);    // WAVE
    addEmote("greeting", 2, 60, 60000);     // BOW

    // Farewell emotes
    addEmote("farewell", 3, 100, 60000);    // WAVE
    addEmote("farewell", 2, 40, 60000);     // BOW

    // Thank you emotes
    addEmote("thank_you", 77, 100, 30000);  // THANKS
    addEmote("thank_you", 2, 60, 30000);    // BOW

    // Combat start emotes
    addEmote("combat_start", 15, 60, 120000);  // ROAR
    addEmote("combat_start", 71, 40, 120000);  // CHEER

    // Combat victory
    addEmote("combat_kill", 71, 80, 60000);    // CHEER
    addEmote("combat_kill", 10, 40, 60000);    // DANCE

    // Death emotes
    addEmote("combat_death", 18, 100, 30000);  // CRY

    // Idle emotes
    addEmote("idle_emote", 10, 60, 300000);    // DANCE
    addEmote("idle_emote", 8, 80, 300000);     // SIT
    addEmote("idle_emote", 11, 40, 300000);    // LAUGH
    addEmote("idle_emote", 3, 30, 300000);     // WAVE

    // City emotes
    addEmote("city_emote", 10, 60, 180000);    // DANCE
    addEmote("city_emote", 8, 80, 180000);     // SIT
    addEmote("city_emote", 16, 40, 180000);    // SLEEP
    addEmote("city_emote", 11, 30, 180000);    // LAUGH
}

// ============================================================================
// Template Selection
// ============================================================================

ChatTemplate const* ChatTemplateManager::SelectChatTemplate(
    Player const* bot, std::string const& context) const
{
    auto it = _chatTemplates.find(context);
    if (it == _chatTemplates.end() || it->second.empty())
        return nullptr;

    // Filter templates that match this bot
    std::vector<ChatTemplate const*> candidates;
    std::vector<uint32> weights;

    for (auto const& tmpl : it->second)
    {
        if (tmpl.MatchesBot(bot))
        {
            candidates.push_back(&tmpl);
            weights.push_back(tmpl.weight);
        }
    }

    if (candidates.empty())
        return nullptr;

    // Weighted random selection
    uint32 totalWeight = std::accumulate(weights.begin(), weights.end(), 0u);
    if (totalWeight == 0)
        return candidates[0];

    std::uniform_int_distribution<uint32> dist(0, totalWeight - 1);
    uint32 roll = dist(_rng);

    uint32 accumulated = 0;
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        accumulated += weights[i];
        if (roll < accumulated)
            return candidates[i];
    }

    return candidates.back();
}

EmoteTemplate const* ChatTemplateManager::SelectEmoteTemplate(
    Player const* bot, std::string const& context) const
{
    auto it = _emoteTemplates.find(context);
    if (it == _emoteTemplates.end() || it->second.empty())
        return nullptr;

    std::vector<EmoteTemplate const*> candidates;
    std::vector<uint32> weights;

    for (auto const& tmpl : it->second)
    {
        if (tmpl.MatchesBot(bot))
        {
            candidates.push_back(&tmpl);
            weights.push_back(tmpl.weight);
        }
    }

    if (candidates.empty())
        return nullptr;

    uint32 totalWeight = std::accumulate(weights.begin(), weights.end(), 0u);
    if (totalWeight == 0)
        return candidates[0];

    std::uniform_int_distribution<uint32> dist(0, totalWeight - 1);
    uint32 roll = dist(_rng);

    uint32 accumulated = 0;
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        accumulated += weights[i];
        if (roll < accumulated)
            return candidates[i];
    }

    return candidates.back();
}

// ============================================================================
// Variable Substitution
// ============================================================================

std::string ChatTemplateManager::SubstituteVariables(
    std::string text, Player const* bot, Player const* target)
{
    if (!bot)
        return text;

    // {name} - Bot's name
    auto replaceAll = [](std::string& str, std::string const& from, std::string const& to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos)
        {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    replaceAll(text, "{name}", bot->GetName());
    replaceAll(text, "{level}", std::to_string(bot->GetLevel()));

    if (target)
        replaceAll(text, "{target}", target->GetName());
    else
        replaceAll(text, "{target}", "friend");

    // {class} and {race} use TrinityCore class/race info strings
    // Use simple string fallbacks to avoid localization complexity
    static const char* classNames[] = {
        "", "Warrior", "Paladin", "Hunter", "Rogue", "Priest",
        "Death Knight", "Shaman", "Mage", "Warlock", "Monk",
        "Druid", "Demon Hunter", "Evoker"
    };
    uint8 cls = bot->GetClass();
    replaceAll(text, "{class}", cls < 14 ? classNames[cls] : "Adventurer");

    return text;
}

// ============================================================================
// Template Queries
// ============================================================================

ChatAction ChatTemplateManager::GetContextualAction(
    Player const* bot, std::string const& context, Player const* target) const
{
    ChatAction action;

    if (!_initialized || !bot)
        return action;

    // Check cooldown
    if (IsOnCooldown(bot->GetGUID(), context))
        return action;

    // Select chat template
    ChatTemplate const* chatTmpl = SelectChatTemplate(bot, context);
    if (chatTmpl)
    {
        action.hasChat = true;
        action.chatText = SubstituteVariables(chatTmpl->messageText, bot, target);
        action.chatType = chatTmpl->chatType;
    }

    // Select emote template
    EmoteTemplate const* emoteTmpl = SelectEmoteTemplate(bot, context);
    if (emoteTmpl)
    {
        action.hasEmote = true;
        action.emoteId = emoteTmpl->emoteId;
    }

    return action;
}

std::string ChatTemplateManager::GetChatMessage(
    Player const* bot, std::string const& context, Player const* target) const
{
    if (!_initialized || !bot || IsOnCooldown(bot->GetGUID(), context))
        return {};

    ChatTemplate const* tmpl = SelectChatTemplate(bot, context);
    if (!tmpl)
        return {};

    return SubstituteVariables(tmpl->messageText, bot, target);
}

uint32 ChatTemplateManager::GetEmote(Player const* bot, std::string const& context) const
{
    if (!_initialized || !bot || IsOnCooldown(bot->GetGUID(), context))
        return 0;

    EmoteTemplate const* tmpl = SelectEmoteTemplate(bot, context);
    return tmpl ? tmpl->emoteId : 0;
}

// ============================================================================
// Execution
// ============================================================================

bool ChatTemplateManager::ExecuteContextualChat(
    Player* bot, std::string const& context, Player* target)
{
    if (!_initialized || !bot || !bot->IsInWorld())
        return false;

    ChatAction action = GetContextualAction(bot, context, target);
    bool sent = false;

    if (action.hasChat && !action.chatText.empty())
    {
        sent = SendChat(bot, action.chatText, action.chatType, target);
    }

    if (action.hasEmote && action.emoteId > 0)
    {
        sent |= SendEmote(bot, action.emoteId);
    }

    if (sent)
    {
        // Record cooldown
        ChatTemplate const* chatTmpl = SelectChatTemplate(bot, context);
        uint32 cooldownMs = chatTmpl ? chatTmpl->cooldownMs : 30000;
        RecordCooldown(bot->GetGUID(), context, cooldownMs);
    }

    return sent;
}

bool ChatTemplateManager::SendChat(
    Player* bot, std::string const& message, TemplateChatType chatType, Player* target)
{
    if (!bot || !bot->IsInWorld() || message.empty())
        return false;

    switch (chatType)
    {
        case TemplateChatType::SAY:
            bot->Say(message, LANG_UNIVERSAL);
            return true;

        case TemplateChatType::YELL:
            bot->Yell(message, LANG_UNIVERSAL);
            return true;

        case TemplateChatType::EMOTE:
            bot->TextEmote(message);
            return true;

        case TemplateChatType::PARTY:
        case TemplateChatType::RAID:
            // Party/raid chat - say locally if not in group
            if (bot->GetGroup())
                bot->Say(message, LANG_UNIVERSAL); // TrinityCore handles group routing
            else
                bot->Say(message, LANG_UNIVERSAL);
            return true;

        case TemplateChatType::WHISPER:
            if (target && target->IsInWorld())
            {
                bot->Whisper(message, LANG_UNIVERSAL, target);
                return true;
            }
            return false;

        case TemplateChatType::GUILD:
            bot->Say(message, LANG_UNIVERSAL);
            return true;

        default:
            return false;
    }
}

bool ChatTemplateManager::SendEmote(Player* bot, uint32 emoteId)
{
    if (!bot || !bot->IsInWorld() || emoteId == 0)
        return false;

    bot->HandleEmoteCommand(static_cast<Emote>(emoteId));
    return true;
}

// ============================================================================
// Cooldown Management
// ============================================================================

bool ChatTemplateManager::IsOnCooldown(ObjectGuid botGuid, std::string const& context) const
{
    std::lock_guard<std::mutex> lock(_cooldownMutex);

    auto it = _cooldownStates.find(botGuid);
    if (it == _cooldownStates.end())
        return false;

    auto coolIt = it->second.contextCooldowns.find(context);
    if (coolIt == it->second.contextCooldowns.end())
        return false;

    auto now = std::chrono::steady_clock::now();
    return now < coolIt->second;
}

void ChatTemplateManager::RecordCooldown(ObjectGuid botGuid, std::string const& context, uint32 cooldownMs)
{
    std::lock_guard<std::mutex> lock(_cooldownMutex);

    auto& state = _cooldownStates[botGuid];
    state.contextCooldowns[context] =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(cooldownMs);
}

void ChatTemplateManager::ClearCooldowns(ObjectGuid botGuid)
{
    std::lock_guard<std::mutex> lock(_cooldownMutex);
    _cooldownStates.erase(botGuid);
}

BotCooldownState& ChatTemplateManager::GetCooldownState(ObjectGuid botGuid) const
{
    return _cooldownStates[botGuid];
}

// ============================================================================
// Statistics
// ============================================================================

uint32 ChatTemplateManager::GetChatTemplateCount() const
{
    uint32 count = 0;
    for (auto const& [ctx, vec] : _chatTemplates)
        count += static_cast<uint32>(vec.size());
    return count;
}

uint32 ChatTemplateManager::GetEmoteTemplateCount() const
{
    uint32 count = 0;
    for (auto const& [ctx, vec] : _emoteTemplates)
        count += static_cast<uint32>(vec.size());
    return count;
}

std::vector<std::string> ChatTemplateManager::GetSupportedContexts() const
{
    std::vector<std::string> contexts;
    for (auto const& [ctx, _] : _chatTemplates)
        contexts.push_back(ctx);
    for (auto const& [ctx, _] : _emoteTemplates)
    {
        if (std::find(contexts.begin(), contexts.end(), ctx) == contexts.end())
            contexts.push_back(ctx);
    }
    std::sort(contexts.begin(), contexts.end());
    return contexts;
}

} // namespace Playerbot
