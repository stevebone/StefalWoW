/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * BOT LOG FILTER
 *
 * Enables per-bot log level filtering so operators can enable verbose
 * debugging for a specific bot without flooding the log with output
 * from hundreds of other bots. This is critical for debugging individual
 * bot behaviors in production environments with many concurrent bots.
 *
 * Usage:
 *   // In a .bot command:
 *   BotLogFilter::Instance().SetBotLogLevel(botGuid, LOG_LEVEL_TRACE);
 *
 *   // In bot code:
 *   if (BotLogFilter::Instance().ShouldLog(botGuid, LOG_LEVEL_DEBUG))
 *       TC_LOG_DEBUG("module.playerbot", "Bot {} doing thing", bot->GetName());
 *
 *   // Convenience macro:
 *   BOT_LOG_DEBUG(bot, "module.playerbot", "Detailed info: {}", value);
 *
 * Architecture:
 *   - Singleton with per-GUID log level overrides
 *   - Default follows the global Trinity log level for module.playerbot
 *   - Individual bots can be elevated to TRACE/DEBUG without affecting others
 *   - Log categories can be filtered independently
 *   - Thread-safe via shared_mutex (read-heavy pattern)
 *   - Admin commands: .bot log <name> <level>, .bot log list
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <vector>

namespace Playerbot
{

// ============================================================================
// LOG LEVEL (mirrors Trinity's LogLevel)
// ============================================================================

enum class BotLogLevel : uint8
{
    DISABLED    = 0,    // No logging for this bot
    FATAL       = 1,    // Fatal errors only
    ERROR_LEVEL = 2,    // Errors
    WARN        = 3,    // Warnings
    INFO        = 4,    // Normal info
    DEBUG       = 5,    // Debug info
    TRACE       = 6     // Maximum verbosity
};

// ============================================================================
// LOG CATEGORY
// ============================================================================

enum class BotLogCategory : uint32
{
    ALL             = 0xFFFFFFFF,
    COMBAT          = 0x00000001,
    MOVEMENT        = 0x00000002,
    AI_DECISION     = 0x00000004,
    SPELL_CAST      = 0x00000008,
    TARGET_SELECT   = 0x00000010,
    HEALING         = 0x00000020,
    THREAT          = 0x00000040,
    POSITIONING     = 0x00000080,
    COOLDOWNS       = 0x00000100,
    PROCS           = 0x00000200,
    CONSUMABLES     = 0x00000400,
    DUNGEON         = 0x00000800,
    PVP             = 0x00001000,
    QUEST           = 0x00002000,
    EQUIPMENT       = 0x00004000,
    SOCIAL          = 0x00008000,
    PROFESSION      = 0x00010000,
    LIFECYCLE       = 0x00020000
};

// ============================================================================
// PER-BOT LOG CONFIGURATION
// ============================================================================

struct BotLogConfig
{
    BotLogLevel level{BotLogLevel::INFO};
    uint32 categoryMask{static_cast<uint32>(BotLogCategory::ALL)};
    ::std::string botName;
    uint32 enabledTime{0};      // When this override was enabled
    uint32 expiryTime{0};       // Auto-disable after this time (0 = permanent)
};

// ============================================================================
// BOT LOG FILTER (SINGLETON)
// ============================================================================

class BotLogFilter
{
public:
    static BotLogFilter& Instance();

    // ========================================================================
    // LOG LEVEL MANAGEMENT
    // ========================================================================

    /// Set log level for a specific bot (by GUID)
    void SetBotLogLevel(ObjectGuid botGuid, BotLogLevel level,
                        ::std::string const& botName = "");

    /// Set log level with category filter
    void SetBotLogLevel(ObjectGuid botGuid, BotLogLevel level,
                        uint32 categoryMask,
                        ::std::string const& botName = "");

    /// Set log level with auto-expiry (in seconds)
    void SetBotLogLevelTimed(ObjectGuid botGuid, BotLogLevel level,
                              uint32 durationSec,
                              ::std::string const& botName = "");

    /// Remove log level override for a specific bot
    void ClearBotLogLevel(ObjectGuid botGuid);

    /// Remove all log level overrides
    void ClearAllOverrides();

    // ========================================================================
    // LOG CHECK
    // ========================================================================

    /// Check if a specific bot should log at a given level
    bool ShouldLog(ObjectGuid botGuid, BotLogLevel level) const;

    /// Check if a specific bot should log at a given level + category
    bool ShouldLog(ObjectGuid botGuid, BotLogLevel level,
                   BotLogCategory category) const;

    /// Get the effective log level for a bot
    BotLogLevel GetEffectiveLevel(ObjectGuid botGuid) const;

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Is there a specific override for this bot?
    bool HasOverride(ObjectGuid botGuid) const;

    /// How many bots have overrides?
    uint32 GetOverrideCount() const;

    /// Get all current overrides
    ::std::vector<::std::pair<ObjectGuid, BotLogConfig>> GetAllOverrides() const;

    /// Get formatted status string
    ::std::string FormatStatus() const;

    // ========================================================================
    // MAINTENANCE
    // ========================================================================

    /// Clean up expired overrides
    void CleanupExpired();

    /// Set the default log level (from global config)
    void SetDefaultLevel(BotLogLevel level) { _defaultLevel = level; }

    /// Get default level
    BotLogLevel GetDefaultLevel() const { return _defaultLevel; }

    // ========================================================================
    // HELPERS
    // ========================================================================

    /// Convert level to string
    static ::std::string LevelToString(BotLogLevel level);

    /// Parse level from string
    static BotLogLevel StringToLevel(::std::string const& str);

    /// Convert category to string
    static ::std::string CategoryToString(BotLogCategory category);

    /// Parse category from string
    static BotLogCategory StringToCategory(::std::string const& str);

private:
    BotLogFilter() = default;
    ~BotLogFilter() = default;
    BotLogFilter(BotLogFilter const&) = delete;
    BotLogFilter& operator=(BotLogFilter const&) = delete;

    // Per-bot overrides
    ::std::unordered_map<ObjectGuid, BotLogConfig> _overrides;
    mutable ::std::shared_mutex _mutex;

    // Default level for bots without overrides
    BotLogLevel _defaultLevel{BotLogLevel::INFO};
};

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

// These macros check the bot-specific log level before generating log output,
// preventing string formatting overhead for bots that aren't being debugged.

#define BOT_LOG_TRACE(bot, channel, ...)                                        \
    do {                                                                        \
        if (bot && ::Playerbot::BotLogFilter::Instance().ShouldLog(             \
            bot->GetGUID(), ::Playerbot::BotLogLevel::TRACE))                   \
            TC_LOG_TRACE(channel, __VA_ARGS__);                                 \
    } while (0)

#define BOT_LOG_DEBUG(bot, channel, ...)                                        \
    do {                                                                        \
        if (bot && ::Playerbot::BotLogFilter::Instance().ShouldLog(             \
            bot->GetGUID(), ::Playerbot::BotLogLevel::DEBUG))                   \
            TC_LOG_DEBUG(channel, __VA_ARGS__);                                 \
    } while (0)

#define BOT_LOG_INFO(bot, channel, ...)                                         \
    do {                                                                        \
        if (bot && ::Playerbot::BotLogFilter::Instance().ShouldLog(             \
            bot->GetGUID(), ::Playerbot::BotLogLevel::INFO))                    \
            TC_LOG_INFO(channel, __VA_ARGS__);                                  \
    } while (0)

} // namespace Playerbot
