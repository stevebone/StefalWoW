/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLAYERBOT_LOG_H
#define PLAYERBOT_LOG_H

#include "Log.h"
#include "Timer.h"
#include <string>

// Playerbot logging macros
#define TC_LOG_PLAYERBOT_TRACE(format, ...) \
    TC_LOG_TRACE("module.playerbot", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_DEBUG(format, ...) \
    TC_LOG_DEBUG("module.playerbot", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_INFO(format, ...) \
    TC_LOG_INFO("module.playerbot", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_WARN(format, ...) \
    TC_LOG_WARN("module.playerbot", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_ERROR(format, ...) \
    TC_LOG_ERROR("module.playerbot", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_FATAL(format, ...) \
    TC_LOG_FATAL("module.playerbot", format, ##__VA_ARGS__)

// Specialized logging macros for different subsystems
#define TC_LOG_PLAYERBOT_AI_TRACE(format, ...) \
    TC_LOG_TRACE("module.playerbot.ai", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_AI_DEBUG(format, ...) \
    TC_LOG_DEBUG("module.playerbot.ai", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_AI_INFO(format, ...) \
    TC_LOG_INFO("module.playerbot.ai", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_AI_WARN(format, ...) \
    TC_LOG_WARN("module.playerbot.ai", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_AI_ERROR(format, ...) \
    TC_LOG_ERROR("module.playerbot.ai", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_PERF_INFO(format, ...) \
    TC_LOG_INFO("module.playerbot.performance", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_PERF_DEBUG(format, ...) \
    TC_LOG_DEBUG("module.playerbot.performance", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_PERF_WARN(format, ...) \
    TC_LOG_WARN("module.playerbot.performance", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_DB_TRACE(format, ...) \
    TC_LOG_TRACE("module.playerbot.database", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_DB_DEBUG(format, ...) \
    TC_LOG_DEBUG("module.playerbot.database", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_DB_INFO(format, ...) \
    TC_LOG_INFO("module.playerbot.database", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_DB_ERROR(format, ...) \
    TC_LOG_ERROR("module.playerbot.database", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_CHAR_TRACE(format, ...) \
    TC_LOG_TRACE("module.playerbot.character", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_CHAR_DEBUG(format, ...) \
    TC_LOG_DEBUG("module.playerbot.character", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_CHAR_INFO(format, ...) \
    TC_LOG_INFO("module.playerbot.character", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_CHAR_WARN(format, ...) \
    TC_LOG_WARN("module.playerbot.character", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_CHAR_ERROR(format, ...) \
    TC_LOG_ERROR("module.playerbot.character", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_ACCOUNT_DEBUG(format, ...) \
    TC_LOG_DEBUG("module.playerbot.account", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_ACCOUNT_INFO(format, ...) \
    TC_LOG_INFO("module.playerbot.account", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_ACCOUNT_WARN(format, ...) \
    TC_LOG_WARN("module.playerbot.account", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_ACCOUNT_ERROR(format, ...) \
    TC_LOG_ERROR("module.playerbot.account", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_NAMES_DEBUG(format, ...) \
    TC_LOG_DEBUG("module.playerbot.names", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_NAMES_INFO(format, ...) \
    TC_LOG_INFO("module.playerbot.names", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_NAMES_WARN(format, ...) \
    TC_LOG_WARN("module.playerbot.names", format, ##__VA_ARGS__)

#define TC_LOG_PLAYERBOT_NAMES_ERROR(format, ...) \
    TC_LOG_ERROR("module.playerbot.names", format, ##__VA_ARGS__)

// Performance metrics helper class
class TC_DATABASE_API PlayerbotPerformanceLogger
{
public:
    PlayerbotPerformanceLogger(std::string const& operation);
    ~PlayerbotPerformanceLogger();

private:
    std::string m_operation;
    uint32 m_startTime;
};

// Performance logging macro
#define PLAYERBOT_PERF_LOG(operation) \
    PlayerbotPerformanceLogger perfLogger(operation)

// Conditional performance logging (only if performance metrics enabled)
#define PLAYERBOT_PERF_LOG_COND(operation) \
    std::unique_ptr<PlayerbotPerformanceLogger> perfLogger; \
    if (sPlayerbotConfig->GetBool("Playerbot.Log.PerformanceMetrics", false)) \
        perfLogger = std::make_unique<PlayerbotPerformanceLogger>(operation)

#endif // PLAYERBOT_LOG_H