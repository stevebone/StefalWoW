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

#ifdef BUILD_PLAYERBOT

#include "PlayerbotLog.h"
#include "PlayerbotConfig.h"
#include "Timer.h"
#include "GameTime.h"

PlayerbotPerformanceLogger::PlayerbotPerformanceLogger(std::string const& operation)
    : m_operation(operation), m_startTime(GameTime::GetGameTimeMS())
{
    TC_LOG_PLAYERBOT_PERF_DEBUG("Starting performance measurement for: {}", m_operation);
}

PlayerbotPerformanceLogger::~PlayerbotPerformanceLogger()
{
    uint32 duration = GetMSTimeDiffToNow(m_startTime);

    // Log performance with different levels based on duration
    if (duration > 100) // More than 100ms is concerning
    {
        TC_LOG_PLAYERBOT_PERF_WARN("{} took {} ms (performance concern)", m_operation, duration);
    }
    else if (duration > 50) // More than 50ms is noteworthy
    {
        TC_LOG_PLAYERBOT_PERF_INFO("{} took {} ms", m_operation, duration);
    }
    else
    {
        TC_LOG_PLAYERBOT_PERF_DEBUG("{} took {} ms", m_operation, duration);
    }
}

#endif // BUILD_PLAYERBOT