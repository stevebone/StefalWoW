/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * ChatTemplateManager: Database-backed contextual chat and emote template system.
 *
 * Provides personality-driven, context-aware chat messages and emotes for bots.
 * Templates are loaded from the database on startup and cached in memory.
 * Supports class/race filtering, locale, weighted random selection, and
 * per-bot cooldown tracking.
 *
 * Trigger Contexts:
 *   - combat_start, combat_end, combat_kill, combat_death
 *   - low_health, low_mana, oom
 *   - quest_accept, quest_complete, quest_turnin
 *   - greeting, farewell, thank_you
 *   - loot_epic, loot_rare, loot_excited
 *   - ready_check, buff_request, res_request
 *   - group_join, group_leave
 *   - idle_emote, city_emote
 *
 * Variable Substitution:
 *   {name}     - Bot's name
 *   {target}   - Target's name
 *   {class}    - Bot's class name
 *   {race}     - Bot's race name
 *   {level}    - Bot's level
 *   {zone}     - Current zone name
 *   {spell}    - Last spell cast name
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <random>
#include <chrono>

class Player;

namespace Playerbot
{

/// Chat type for template messages
enum class TemplateChatType : uint8
{
    SAY     = 0,
    YELL    = 1,
    PARTY   = 2,
    RAID    = 3,
    GUILD   = 4,
    WHISPER = 5,
    EMOTE   = 6,    // /me style text emote
};

/// A single chat template entry loaded from database
struct ChatTemplate
{
    uint32 id = 0;
    std::string triggerContext;      // e.g. "combat_start", "greeting"
    TemplateChatType chatType = TemplateChatType::SAY;
    std::string messageText;        // With {variable} placeholders
    uint8 filterClass = 0;          // 0 = any class
    uint8 filterRace = 0;           // 0 = any race
    uint32 filterMinLevel = 0;      // 0 = no minimum
    uint32 filterMaxLevel = 0;      // 0 = no maximum
    uint32 weight = 100;            // Selection weight (higher = more likely)
    uint32 cooldownMs = 30000;      // Per-bot cooldown for this template
    std::string locale;             // "" = default locale, "deDE", "frFR", etc.

    bool MatchesBot(Player const* bot) const;
};

/// An emote template entry loaded from database
struct EmoteTemplate
{
    uint32 id = 0;
    std::string triggerContext;      // Same contexts as chat
    uint32 emoteId = 0;             // TrinityCore Emote enum value
    uint8 filterClass = 0;
    uint8 filterRace = 0;
    uint32 weight = 100;
    uint32 cooldownMs = 15000;

    bool MatchesBot(Player const* bot) const;
};

/// A chat+emote combination for delivery
struct ChatAction
{
    bool hasChat = false;
    std::string chatText;
    TemplateChatType chatType = TemplateChatType::SAY;

    bool hasEmote = false;
    uint32 emoteId = 0;
};

/// Per-bot cooldown tracking
struct BotCooldownState
{
    /// Map of context -> last trigger time
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> contextCooldowns;

    /// Map of template ID -> last use time
    std::unordered_map<uint32, std::chrono::steady_clock::time_point> templateCooldowns;
};

/// ChatTemplateManager - Singleton managing all chat/emote templates
class TC_GAME_API ChatTemplateManager
{
public:
    static ChatTemplateManager& Instance();

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /// Initialize: load templates from database
    bool Initialize();

    /// Shutdown: clear caches
    void Shutdown();

    /// Reload templates from database (hot reload)
    void ReloadTemplates();

    // ========================================================================
    // Template Queries
    // ========================================================================

    /// Get a contextual chat action for a bot.
    /// Returns a combined chat + emote action based on templates.
    /// Respects cooldowns and class/race/level filters.
    ChatAction GetContextualAction(Player const* bot, std::string const& context,
                                    Player const* target = nullptr) const;

    /// Get just a chat message for a context.
    /// Returns empty string if no template matches or on cooldown.
    std::string GetChatMessage(Player const* bot, std::string const& context,
                               Player const* target = nullptr) const;

    /// Get just an emote ID for a context.
    /// Returns 0 if no template matches or on cooldown.
    uint32 GetEmote(Player const* bot, std::string const& context) const;

    // ========================================================================
    // Execution
    // ========================================================================

    /// Execute a contextual chat action for a bot.
    /// Selects template, substitutes variables, sends message/emote.
    /// Returns true if something was sent.
    bool ExecuteContextualChat(Player* bot, std::string const& context,
                                Player* target = nullptr);

    /// Execute a specific chat message (bypasses template selection).
    static bool SendChat(Player* bot, std::string const& message,
                         TemplateChatType chatType, Player* target = nullptr);

    /// Execute a specific emote.
    static bool SendEmote(Player* bot, uint32 emoteId);

    // ========================================================================
    // Cooldown Management
    // ========================================================================

    /// Check if a context is on cooldown for a bot
    bool IsOnCooldown(ObjectGuid botGuid, std::string const& context) const;

    /// Record cooldown for a context
    void RecordCooldown(ObjectGuid botGuid, std::string const& context, uint32 cooldownMs);

    /// Clear all cooldowns for a bot (e.g. on login)
    void ClearCooldowns(ObjectGuid botGuid);

    // ========================================================================
    // Statistics
    // ========================================================================

    /// Get total loaded chat templates
    uint32 GetChatTemplateCount() const;

    /// Get total loaded emote templates
    uint32 GetEmoteTemplateCount() const;

    /// Get supported contexts
    std::vector<std::string> GetSupportedContexts() const;

private:
    ChatTemplateManager() = default;
    ~ChatTemplateManager() = default;
    ChatTemplateManager(ChatTemplateManager const&) = delete;
    ChatTemplateManager& operator=(ChatTemplateManager const&) = delete;

    /// Load chat templates from DB
    void LoadChatTemplates();

    /// Load emote templates from DB
    void LoadEmoteTemplates();

    /// Load default hardcoded templates (fallback if DB is empty)
    void LoadDefaultTemplates();

    /// Select a matching chat template for a bot and context
    ChatTemplate const* SelectChatTemplate(Player const* bot, std::string const& context) const;

    /// Select a matching emote template for a bot and context
    EmoteTemplate const* SelectEmoteTemplate(Player const* bot, std::string const& context) const;

    /// Perform variable substitution on a template string
    static std::string SubstituteVariables(std::string text, Player const* bot,
                                           Player const* target);

    /// Get or create cooldown state for a bot
    BotCooldownState& GetCooldownState(ObjectGuid botGuid) const;

    // Storage: context -> vector of templates
    std::unordered_map<std::string, std::vector<ChatTemplate>> _chatTemplates;
    std::unordered_map<std::string, std::vector<EmoteTemplate>> _emoteTemplates;

    // Per-bot cooldown tracking
    mutable std::unordered_map<ObjectGuid, BotCooldownState> _cooldownStates;
    mutable std::mutex _cooldownMutex;

    // RNG
    mutable std::mt19937 _rng{std::random_device{}()};

    bool _initialized = false;
};

} // namespace Playerbot
