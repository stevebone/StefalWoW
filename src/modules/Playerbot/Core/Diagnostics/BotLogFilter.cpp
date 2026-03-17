/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotLogFilter.h"
#include "GameTime.h"
#include "Log.h"

#include <sstream>
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

BotLogFilter& BotLogFilter::Instance()
{
    static BotLogFilter instance;
    return instance;
}

// ============================================================================
// LOG LEVEL MANAGEMENT
// ============================================================================

void BotLogFilter::SetBotLogLevel(ObjectGuid botGuid, BotLogLevel level,
                                   ::std::string const& botName)
{
    ::std::unique_lock lock(_mutex);

    BotLogConfig config;
    config.level = level;
    config.categoryMask = static_cast<uint32>(BotLogCategory::ALL);
    config.botName = botName;
    config.enabledTime = GameTime::GetGameTimeMS();
    config.expiryTime = 0;  // Permanent

    _overrides[botGuid] = ::std::move(config);

    TC_LOG_INFO("module.playerbot", "BotLogFilter: Set log level for {} ({}) to {}",
        botName.empty() ? botGuid.ToString() : botName,
        botGuid.ToString(), LevelToString(level));
}

void BotLogFilter::SetBotLogLevel(ObjectGuid botGuid, BotLogLevel level,
                                   uint32 categoryMask,
                                   ::std::string const& botName)
{
    ::std::unique_lock lock(_mutex);

    BotLogConfig config;
    config.level = level;
    config.categoryMask = categoryMask;
    config.botName = botName;
    config.enabledTime = GameTime::GetGameTimeMS();
    config.expiryTime = 0;

    _overrides[botGuid] = ::std::move(config);

    TC_LOG_INFO("module.playerbot", "BotLogFilter: Set log level for {} to {} "
        "(categories: 0x{:08X})",
        botName.empty() ? botGuid.ToString() : botName,
        LevelToString(level), categoryMask);
}

void BotLogFilter::SetBotLogLevelTimed(ObjectGuid botGuid, BotLogLevel level,
                                        uint32 durationSec,
                                        ::std::string const& botName)
{
    ::std::unique_lock lock(_mutex);

    BotLogConfig config;
    config.level = level;
    config.categoryMask = static_cast<uint32>(BotLogCategory::ALL);
    config.botName = botName;
    config.enabledTime = GameTime::GetGameTimeMS();
    config.expiryTime = config.enabledTime + (durationSec * 1000);

    _overrides[botGuid] = ::std::move(config);

    TC_LOG_INFO("module.playerbot", "BotLogFilter: Set timed log level for {} to {} "
        "(expires in {} seconds)",
        botName.empty() ? botGuid.ToString() : botName,
        LevelToString(level), durationSec);
}

void BotLogFilter::ClearBotLogLevel(ObjectGuid botGuid)
{
    ::std::unique_lock lock(_mutex);

    auto it = _overrides.find(botGuid);
    if (it != _overrides.end())
    {
        TC_LOG_INFO("module.playerbot", "BotLogFilter: Cleared log override for {} ({})",
            it->second.botName.empty() ? botGuid.ToString() : it->second.botName,
            botGuid.ToString());
        _overrides.erase(it);
    }
}

void BotLogFilter::ClearAllOverrides()
{
    ::std::unique_lock lock(_mutex);
    uint32 count = static_cast<uint32>(_overrides.size());
    _overrides.clear();
    TC_LOG_INFO("module.playerbot", "BotLogFilter: Cleared all {} log overrides", count);
}

// ============================================================================
// LOG CHECK
// ============================================================================

bool BotLogFilter::ShouldLog(ObjectGuid botGuid, BotLogLevel level) const
{
    ::std::shared_lock lock(_mutex);

    auto it = _overrides.find(botGuid);
    if (it != _overrides.end())
    {
        // Check expiry
        if (it->second.expiryTime > 0 &&
            GameTime::GetGameTimeMS() > it->second.expiryTime)
        {
            // Expired - but we can't modify under shared lock; treat as default
            return level <= _defaultLevel;
        }
        return level <= it->second.level;
    }

    // No override - use default
    return level <= _defaultLevel;
}

bool BotLogFilter::ShouldLog(ObjectGuid botGuid, BotLogLevel level,
                              BotLogCategory category) const
{
    ::std::shared_lock lock(_mutex);

    auto it = _overrides.find(botGuid);
    if (it != _overrides.end())
    {
        // Check expiry
        if (it->second.expiryTime > 0 &&
            GameTime::GetGameTimeMS() > it->second.expiryTime)
        {
            return level <= _defaultLevel;
        }

        // Check both level and category
        if (level > it->second.level)
            return false;

        return (it->second.categoryMask & static_cast<uint32>(category)) != 0;
    }

    return level <= _defaultLevel;
}

BotLogLevel BotLogFilter::GetEffectiveLevel(ObjectGuid botGuid) const
{
    ::std::shared_lock lock(_mutex);

    auto it = _overrides.find(botGuid);
    if (it != _overrides.end())
    {
        if (it->second.expiryTime > 0 &&
            GameTime::GetGameTimeMS() > it->second.expiryTime)
        {
            return _defaultLevel;
        }
        return it->second.level;
    }

    return _defaultLevel;
}

// ============================================================================
// QUERIES
// ============================================================================

bool BotLogFilter::HasOverride(ObjectGuid botGuid) const
{
    ::std::shared_lock lock(_mutex);
    return _overrides.count(botGuid) > 0;
}

uint32 BotLogFilter::GetOverrideCount() const
{
    ::std::shared_lock lock(_mutex);
    return static_cast<uint32>(_overrides.size());
}

::std::vector<::std::pair<ObjectGuid, BotLogConfig>> BotLogFilter::GetAllOverrides() const
{
    ::std::shared_lock lock(_mutex);
    ::std::vector<::std::pair<ObjectGuid, BotLogConfig>> result;
    result.reserve(_overrides.size());
    for (auto const& [guid, config] : _overrides)
        result.emplace_back(guid, config);
    return result;
}

::std::string BotLogFilter::FormatStatus() const
{
    ::std::shared_lock lock(_mutex);

    ::std::ostringstream oss;
    oss << "=== Bot Log Filter Status ===\n";
    oss << "  Default Level: " << LevelToString(_defaultLevel) << "\n";
    oss << "  Active Overrides: " << _overrides.size() << "\n";

    if (!_overrides.empty())
    {
        oss << "  Overrides:\n";
        uint32 now = GameTime::GetGameTimeMS();
        for (auto const& [guid, config] : _overrides)
        {
            oss << "    " << (config.botName.empty() ? guid.ToString() : config.botName)
                << " -> " << LevelToString(config.level);
            if (config.categoryMask != static_cast<uint32>(BotLogCategory::ALL))
                oss << " (filtered categories)";
            if (config.expiryTime > 0)
            {
                if (now < config.expiryTime)
                    oss << " [expires in " << ((config.expiryTime - now) / 1000) << "s]";
                else
                    oss << " [EXPIRED]";
            }
            oss << "\n";
        }
    }

    return oss.str();
}

// ============================================================================
// MAINTENANCE
// ============================================================================

void BotLogFilter::CleanupExpired()
{
    ::std::unique_lock lock(_mutex);
    uint32 now = GameTime::GetGameTimeMS();
    uint32 removed = 0;

    for (auto it = _overrides.begin(); it != _overrides.end();)
    {
        if (it->second.expiryTime > 0 && now > it->second.expiryTime)
        {
            TC_LOG_DEBUG("module.playerbot", "BotLogFilter: Expired override for {} ({})",
                it->second.botName.empty() ? it->first.ToString() : it->second.botName,
                it->first.ToString());
            it = _overrides.erase(it);
            ++removed;
        }
        else
        {
            ++it;
        }
    }

    if (removed > 0)
    {
        TC_LOG_INFO("module.playerbot", "BotLogFilter: Cleaned up {} expired overrides", removed);
    }
}

// ============================================================================
// HELPERS
// ============================================================================

::std::string BotLogFilter::LevelToString(BotLogLevel level)
{
    switch (level)
    {
        case BotLogLevel::DISABLED:     return "DISABLED";
        case BotLogLevel::FATAL:        return "FATAL";
        case BotLogLevel::ERROR_LEVEL:  return "ERROR";
        case BotLogLevel::WARN:         return "WARN";
        case BotLogLevel::INFO:         return "INFO";
        case BotLogLevel::DEBUG:        return "DEBUG";
        case BotLogLevel::TRACE:        return "TRACE";
        default: return "UNKNOWN";
    }
}

BotLogLevel BotLogFilter::StringToLevel(::std::string const& str)
{
    if (str == "disabled" || str == "DISABLED" || str == "off")
        return BotLogLevel::DISABLED;
    if (str == "fatal" || str == "FATAL")
        return BotLogLevel::FATAL;
    if (str == "error" || str == "ERROR")
        return BotLogLevel::ERROR_LEVEL;
    if (str == "warn" || str == "WARN" || str == "warning")
        return BotLogLevel::WARN;
    if (str == "info" || str == "INFO")
        return BotLogLevel::INFO;
    if (str == "debug" || str == "DEBUG")
        return BotLogLevel::DEBUG;
    if (str == "trace" || str == "TRACE")
        return BotLogLevel::TRACE;
    return BotLogLevel::INFO;
}

::std::string BotLogFilter::CategoryToString(BotLogCategory category)
{
    switch (category)
    {
        case BotLogCategory::ALL:         return "ALL";
        case BotLogCategory::COMBAT:      return "COMBAT";
        case BotLogCategory::MOVEMENT:    return "MOVEMENT";
        case BotLogCategory::AI_DECISION: return "AI_DECISION";
        case BotLogCategory::SPELL_CAST:  return "SPELL_CAST";
        case BotLogCategory::TARGET_SELECT: return "TARGET_SELECT";
        case BotLogCategory::HEALING:     return "HEALING";
        case BotLogCategory::THREAT:      return "THREAT";
        case BotLogCategory::POSITIONING: return "POSITIONING";
        case BotLogCategory::COOLDOWNS:   return "COOLDOWNS";
        case BotLogCategory::PROCS:       return "PROCS";
        case BotLogCategory::CONSUMABLES: return "CONSUMABLES";
        case BotLogCategory::DUNGEON:     return "DUNGEON";
        case BotLogCategory::PVP:         return "PVP";
        case BotLogCategory::QUEST:       return "QUEST";
        case BotLogCategory::EQUIPMENT:   return "EQUIPMENT";
        case BotLogCategory::SOCIAL:      return "SOCIAL";
        case BotLogCategory::PROFESSION:  return "PROFESSION";
        case BotLogCategory::LIFECYCLE:   return "LIFECYCLE";
        default: return "UNKNOWN";
    }
}

BotLogCategory BotLogFilter::StringToCategory(::std::string const& str)
{
    if (str == "all" || str == "ALL") return BotLogCategory::ALL;
    if (str == "combat" || str == "COMBAT") return BotLogCategory::COMBAT;
    if (str == "movement" || str == "MOVEMENT") return BotLogCategory::MOVEMENT;
    if (str == "ai" || str == "AI_DECISION") return BotLogCategory::AI_DECISION;
    if (str == "spell" || str == "SPELL_CAST") return BotLogCategory::SPELL_CAST;
    if (str == "target" || str == "TARGET_SELECT") return BotLogCategory::TARGET_SELECT;
    if (str == "healing" || str == "HEALING") return BotLogCategory::HEALING;
    if (str == "threat" || str == "THREAT") return BotLogCategory::THREAT;
    if (str == "position" || str == "POSITIONING") return BotLogCategory::POSITIONING;
    if (str == "cooldown" || str == "COOLDOWNS") return BotLogCategory::COOLDOWNS;
    if (str == "proc" || str == "PROCS") return BotLogCategory::PROCS;
    if (str == "consumable" || str == "CONSUMABLES") return BotLogCategory::CONSUMABLES;
    if (str == "dungeon" || str == "DUNGEON") return BotLogCategory::DUNGEON;
    if (str == "pvp" || str == "PVP") return BotLogCategory::PVP;
    if (str == "quest" || str == "QUEST") return BotLogCategory::QUEST;
    if (str == "equipment" || str == "EQUIPMENT") return BotLogCategory::EQUIPMENT;
    if (str == "social" || str == "SOCIAL") return BotLogCategory::SOCIAL;
    if (str == "profession" || str == "PROFESSION") return BotLogCategory::PROFESSION;
    if (str == "lifecycle" || str == "LIFECYCLE") return BotLogCategory::LIFECYCLE;
    return BotLogCategory::ALL;
}

} // namespace Playerbot
