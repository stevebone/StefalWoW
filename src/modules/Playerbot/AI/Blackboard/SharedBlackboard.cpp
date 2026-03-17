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

#include "SharedBlackboard.h"
#include "Log.h"
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// SharedBlackboard
// ============================================================================

uint32 SharedBlackboard::RegisterListener(::std::string const& key, ChangeListener listener)
{
    ::std::unique_lock lock(_listenerMutex);

    uint32 id = _nextListenerId++;

    ListenerEntry entry;
    entry.id = id;
    entry.key = key;
    entry.callback = listener;

    _listeners.push_back(entry);

    return id;
}

void SharedBlackboard::UnregisterListener(uint32 listenerId)
{
    ::std::unique_lock lock(_listenerMutex);

    _listeners.erase(
        ::std::remove_if(_listeners.begin(), _listeners.end(),
            [listenerId](ListenerEntry const& entry) {
                return entry.id == listenerId;
            }),
        _listeners.end()
    );
}

void SharedBlackboard::NotifyChange(::std::string const& key, ::std::any const& oldValue, ::std::any const& newValue)
{
    ::std::shared_lock lock(_listenerMutex);

    ChangeEvent event;
    event.key = key;
    event.oldValue = oldValue;
    event.newValue = newValue;
    event.timestamp = GameTime::GetGameTimeMS();

    for (auto const& listener : _listeners)
    {
        // Call listener if watching this key or watching all keys
    if (listener.key.empty() || listener.key == key)
        {
            try
            {
                listener.callback(event);
            }
            catch (...)
            {
                TC_LOG_ERROR("playerbot.blackboard", "Exception in blackboard change listener for key: {}", key);
            }
        }
    }
}

// ============================================================================
// BlackboardManager
// ============================================================================

::std::unordered_map<ObjectGuid, ::std::unique_ptr<SharedBlackboard>> BlackboardManager::_botBlackboards;
::std::unordered_map<uint32, ::std::unique_ptr<SharedBlackboard>> BlackboardManager::_groupBlackboards;
::std::unordered_map<uint32, ::std::unique_ptr<SharedBlackboard>> BlackboardManager::_raidBlackboards;
::std::unordered_map<uint32, ::std::unique_ptr<SharedBlackboard>> BlackboardManager::_zoneBlackboards;

::std::shared_mutex BlackboardManager::_botMutex;
::std::shared_mutex BlackboardManager::_groupMutex;
::std::shared_mutex BlackboardManager::_raidMutex;
::std::shared_mutex BlackboardManager::_zoneMutex;

SharedBlackboard* BlackboardManager::GetBotBlackboard(ObjectGuid botGuid)
{
    {
        ::std::shared_lock lock(_botMutex);
        auto it = _botBlackboards.find(botGuid);
        if (it != _botBlackboards.end())
            return it->second.get();
    }

    // Create new blackboard
    ::std::unique_lock lock(_botMutex);

    // Double-check after acquiring write lock
    auto it = _botBlackboards.find(botGuid);
    if (it != _botBlackboards.end())
        return it->second.get();

    auto blackboard = ::std::make_unique<SharedBlackboard>();
    SharedBlackboard* ptr = blackboard.get();
    _botBlackboards[botGuid] = ::std::move(blackboard);

    TC_LOG_TRACE("playerbot.blackboard", "Created bot blackboard for {}", botGuid.ToString());

    return ptr;
}

SharedBlackboard* BlackboardManager::GetGroupBlackboard(uint32 groupId)
{
    {
        ::std::shared_lock lock(_groupMutex);
        auto it = _groupBlackboards.find(groupId);
        if (it != _groupBlackboards.end())
            return it->second.get();
    }

    ::std::unique_lock lock(_groupMutex);

    auto it = _groupBlackboards.find(groupId);
    if (it != _groupBlackboards.end())
        return it->second.get();

    auto blackboard = ::std::make_unique<SharedBlackboard>();
    SharedBlackboard* ptr = blackboard.get();
    _groupBlackboards[groupId] = ::std::move(blackboard);

    TC_LOG_TRACE("playerbot.blackboard", "Created group blackboard for {}", groupId);

    return ptr;
}

SharedBlackboard* BlackboardManager::GetRaidBlackboard(uint32 raidId)
{
    {
        ::std::shared_lock lock(_raidMutex);
        auto it = _raidBlackboards.find(raidId);
        if (it != _raidBlackboards.end())
            return it->second.get();
    }

    ::std::unique_lock lock(_raidMutex);

    auto it = _raidBlackboards.find(raidId);
    if (it != _raidBlackboards.end())
        return it->second.get();

    auto blackboard = ::std::make_unique<SharedBlackboard>();
    SharedBlackboard* ptr = blackboard.get();
    _raidBlackboards[raidId] = ::std::move(blackboard);

    TC_LOG_TRACE("playerbot.blackboard", "Created raid blackboard for {}", raidId);

    return ptr;
}

SharedBlackboard* BlackboardManager::GetZoneBlackboard(uint32 zoneId)
{
    {
        ::std::shared_lock lock(_zoneMutex);
        auto it = _zoneBlackboards.find(zoneId);
        if (it != _zoneBlackboards.end())
            return it->second.get();
    }

    ::std::unique_lock lock(_zoneMutex);

    auto it = _zoneBlackboards.find(zoneId);
    if (it != _zoneBlackboards.end())
        return it->second.get();

    auto blackboard = ::std::make_unique<SharedBlackboard>();
    SharedBlackboard* ptr = blackboard.get();
    _zoneBlackboards[zoneId] = ::std::move(blackboard);

    TC_LOG_TRACE("playerbot.blackboard", "Created zone blackboard for {}", zoneId);

    return ptr;
}

void BlackboardManager::RemoveBotBlackboard(ObjectGuid botGuid)
{
    ::std::unique_lock lock(_botMutex);
    _botBlackboards.erase(botGuid);

    TC_LOG_TRACE("playerbot.blackboard", "Removed bot blackboard for {}", botGuid.ToString());
}

void BlackboardManager::RemoveGroupBlackboard(uint32 groupId)
{
    ::std::unique_lock lock(_groupMutex);
    _groupBlackboards.erase(groupId);

    TC_LOG_TRACE("playerbot.blackboard", "Removed group blackboard for {}", groupId);
}

void BlackboardManager::RemoveRaidBlackboard(uint32 raidId)
{
    ::std::unique_lock lock(_raidMutex);
    _raidBlackboards.erase(raidId);

    TC_LOG_TRACE("playerbot.blackboard", "Removed raid blackboard for {}", raidId);
}

void BlackboardManager::RemoveZoneBlackboard(uint32 zoneId)
{
    ::std::unique_lock lock(_zoneMutex);
    _zoneBlackboards.erase(zoneId);

    TC_LOG_TRACE("playerbot.blackboard", "Removed zone blackboard for {}", zoneId);
}

void BlackboardManager::ClearAll()
{
    {
        ::std::unique_lock lock(_botMutex);
        _botBlackboards.clear();
    }

    {
        ::std::unique_lock lock(_groupMutex);
        _groupBlackboards.clear();
    }

    {
        ::std::unique_lock lock(_raidMutex);
        _raidBlackboards.clear();
    }

    {
        ::std::unique_lock lock(_zoneMutex);
        _zoneBlackboards.clear();
    }

    TC_LOG_INFO("playerbot.blackboard", "Cleared all blackboards");
}

void BlackboardManager::PropagateToGroup(ObjectGuid botGuid, uint32 groupId, ::std::string const& key)
{
    SharedBlackboard* botBoard = GetBotBlackboard(botGuid);
    SharedBlackboard* groupBoard = GetGroupBlackboard(groupId);

    if (!botBoard || !groupBoard)
    {
        TC_LOG_TRACE("module.playerbot.blackboard",
            "PropagateToGroup: Bot or group blackboard not found (bot: {}, group: {})",
            botGuid.GetCounter(), groupId);
        return;
    }

    if (key.empty())
    {
        // Propagate all bot data to group
        groupBoard->MergeFrom(*botBoard, true);

        TC_LOG_DEBUG("module.playerbot.blackboard",
            "PropagateToGroup: Propagated all keys from bot {} to group {}",
            botGuid.GetCounter(), groupId);
    }
    else
    {
        // Propagate specific key using type-erased transfer
        // The SharedBlackboard stores std::any internally, so we can use CopyKeyFrom
        if (botBoard->Has(key))
        {
            groupBoard->CopyKeyFrom(*botBoard, key);

            TC_LOG_DEBUG("module.playerbot.blackboard",
                "PropagateToGroup: Propagated key '{}' from bot {} to group {}",
                key, botGuid.GetCounter(), groupId);
        }
        else
        {
            TC_LOG_TRACE("module.playerbot.blackboard",
                "PropagateToGroup: Key '{}' not found in bot {} blackboard",
                key, botGuid.GetCounter());
        }
    }
}

void BlackboardManager::PropagateToRaid(uint32 groupId, uint32 raidId, ::std::string const& /*key*/)
{
    SharedBlackboard* groupBoard = GetGroupBlackboard(groupId);
    SharedBlackboard* raidBoard = GetRaidBlackboard(raidId);

    if (!groupBoard || !raidBoard)
        return;

    // Merge group data into raid board
    raidBoard->MergeFrom(*groupBoard, false); // Don't overwrite existing raid data
}

void BlackboardManager::PropagateToZone(uint32 raidId, uint32 zoneId, ::std::string const& /*key*/)
{
    SharedBlackboard* raidBoard = GetRaidBlackboard(raidId);
    SharedBlackboard* zoneBoard = GetZoneBlackboard(zoneId);

    if (!raidBoard || !zoneBoard)
        return;

    // Merge raid data into zone board
    zoneBoard->MergeFrom(*raidBoard, false); // Don't overwrite existing zone data
}

} // namespace Playerbot
