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

#ifndef TRINITYCORE_SHARED_BLACKBOARD_H
#define TRINITYCORE_SHARED_BLACKBOARD_H

#include "Define.h"
#include "ObjectGuid.h"
#include <any>
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <vector>
#include <memory>

namespace Playerbot
{

/**
 * @brief Thread-safe shared blackboard for cross-bot communication
 *
 * Provides type-safe storage with read-write locking for concurrent access.
 * Supports event notifications on value changes.
 */
class TC_GAME_API SharedBlackboard
{
public:
    /**
     * @brief Blackboard change event
     */
    struct ChangeEvent
    {
        ::std::string key;
        ::std::any oldValue;
        ::std::any newValue;
        uint32 timestamp;
    };

    /**
     * @brief Change listener callback
     */
    using ChangeListener = ::std::function<void(ChangeEvent const&)>;

    SharedBlackboard() = default;
    ~SharedBlackboard() = default;

    /**
     * @brief Set value (thread-safe)
     * @param key Data key
     * @param value Value to store
     */
    template<typename T>
    void Set(::std::string const& key, T const& value)
    {
        ::std::unique_lock lock(_mutex);

        // Get old value for change event
        ::std::any oldValue;
        auto it = _data.find(key);
        if (it != _data.end())
            oldValue = it->second;

        _data[key] = value;

        // Notify listeners (outside lock to prevent deadlocks)
        lock.unlock();
        NotifyChange(key, oldValue, value);
    }

    /**
     * @brief Get value (thread-safe)
     * @param key Data key
     * @param outValue Output value
     * @return True if key exists and type matches
     */
    template<typename T>
    bool Get(::std::string const& key, T& outValue) const
    {
        ::std::shared_lock lock(_mutex);

        auto it = _data.find(key);
        if (it == _data.end())
            return false;

        try
        {
            outValue = ::std::any_cast<T>(it->second);
            return true;
        }
        catch (::std::bad_any_cast const&)
        {
            return false;
        }
    }

    /**
     * @brief Get value or default (thread-safe)
     * @param key Data key
     * @param defaultValue Default if key doesn't exist
     * @return Value or default
     */
    template<typename T>
    T GetOr(::std::string const& key, T const& defaultValue) const
    {
        T value;
        if (Get(key, value))
            return value;
        return defaultValue;
    }

    /**
     * @brief Check if key exists (thread-safe)
     * @param key Data key
     * @return True if key exists
     */
    bool Has(::std::string const& key) const
    {
        ::std::shared_lock lock(_mutex);
        return _data.find(key) != _data.end();
    }

    /**
     * @brief Remove key (thread-safe)
     * @param key Data key to remove
     */
    void Remove(::std::string const& key)
    {
        ::std::unique_lock lock(_mutex);
        _data.erase(key);
    }

    /**
     * @brief Clear all data (thread-safe)
     */
    void Clear()
    {
        ::std::unique_lock lock(_mutex);
        _data.clear();
    }

    /**
     * @brief Get all keys (thread-safe)
     * @return Vector of all keys
     */
    ::std::vector<::std::string> GetKeys() const
    {
        ::std::shared_lock lock(_mutex);

        ::std::vector<::std::string> keys;
        keys.reserve(_data.size());

        for (auto const& pair : _data)
            keys.push_back(pair.first);

        return keys;
    }

    /**
     * @brief Register change listener
     * @param key Key to watch (empty = watch all)
     * @param listener Callback function
     * @return Listener ID for unregistering
     */
    uint32 RegisterListener(::std::string const& key, ChangeListener listener);

    /**
     * @brief Unregister change listener
     * @param listenerId Listener ID from registration
     */
    void UnregisterListener(uint32 listenerId);

    /**
     * @brief Copy data from another blackboard (thread-safe)
     * @param other Source blackboard
     */
    void CopyFrom(SharedBlackboard const& other)
    {
        ::std::shared_lock otherLock(other._mutex);
        ::std::unique_lock thisLock(_mutex);

        _data = other._data;
    }

    /**
     * @brief Merge data from another blackboard (thread-safe)
     * @param other Source blackboard
     * @param overwrite Overwrite existing keys
     */
    void MergeFrom(SharedBlackboard const& other, bool overwrite = true)
    {
        ::std::shared_lock otherLock(other._mutex);
        ::std::unique_lock thisLock(_mutex);

        for (auto const& pair : other._data)
        {
            if (overwrite || _data.find(pair.first) == _data.end())
                _data[pair.first] = pair.second;
        }
    }

    /**
     * @brief Copy a specific key from another blackboard (thread-safe)
     * @param other Source blackboard
     * @param key Key to copy
     * @return True if key was found and copied
     */
    bool CopyKeyFrom(SharedBlackboard const& other, ::std::string const& key)
    {
        ::std::shared_lock otherLock(other._mutex);

        auto it = other._data.find(key);
        if (it == other._data.end())
            return false;

        ::std::any oldValue;
        {
            ::std::unique_lock thisLock(_mutex);

            // Get old value for change event
            auto existingIt = _data.find(key);
            if (existingIt != _data.end())
                oldValue = existingIt->second;

            _data[key] = it->second;
        }

        // Notify listeners outside lock
        NotifyChange(key, oldValue, it->second);
        return true;
    }

    /**
     * @brief Get raw std::any value for a key (thread-safe)
     * @param key Data key
     * @param outValue Output value
     * @return True if key exists
     */
    bool GetAny(::std::string const& key, ::std::any& outValue) const
    {
        ::std::shared_lock lock(_mutex);

        auto it = _data.find(key);
        if (it == _data.end())
            return false;

        outValue = it->second;
        return true;
    }

    /**
     * @brief Set raw std::any value (thread-safe)
     * @param key Data key
     * @param value Value to store
     */
    void SetAny(::std::string const& key, ::std::any const& value)
    {
        ::std::unique_lock lock(_mutex);

        ::std::any oldValue;
        auto it = _data.find(key);
        if (it != _data.end())
            oldValue = it->second;

        _data[key] = value;

        lock.unlock();
        NotifyChange(key, oldValue, value);
    }

private:
    void NotifyChange(::std::string const& key, ::std::any const& oldValue, ::std::any const& newValue);

    mutable ::std::shared_mutex _mutex;
    ::std::unordered_map<::std::string, ::std::any> _data;

    struct ListenerEntry
    {
        uint32 id;
        ::std::string key; // Empty = listen to all
        ChangeListener callback;
    };

    ::std::vector<ListenerEntry> _listeners;
    uint32 _nextListenerId = 1;
    mutable ::std::shared_mutex _listenerMutex;
};

/**
 * @brief Blackboard scope for hierarchical storage
 */
enum class BlackboardScope : uint8
{
    BOT,        // Per-bot blackboard
    GROUP,      // Shared among group (5-40 players)
    RAID,       // Shared among raid (40 players)
    ZONE        // Shared among zone (100-500 players)
};

/**
 * @brief Hierarchical Blackboard Manager
 * Manages blackboards at different scopes with automatic propagation
 */
class TC_GAME_API BlackboardManager
{
public:
    /**
     * @brief Get bot blackboard
     * @param botGuid Bot GUID
     * @return Bot's personal blackboard
     */
    static SharedBlackboard* GetBotBlackboard(ObjectGuid botGuid);

    /**
     * @brief Get group blackboard
     * @param groupId Group ID
     * @return Group shared blackboard
     */
    static SharedBlackboard* GetGroupBlackboard(uint32 groupId);

    /**
     * @brief Get raid blackboard
     * @param raidId Raid ID
     * @return Raid shared blackboard
     */
    static SharedBlackboard* GetRaidBlackboard(uint32 raidId);

    /**
     * @brief Get zone blackboard
     * @param zoneId Zone ID
     * @return Zone shared blackboard
     */
    static SharedBlackboard* GetZoneBlackboard(uint32 zoneId);

    /**
     * @brief Remove bot blackboard
     * @param botGuid Bot GUID
     */
    static void RemoveBotBlackboard(ObjectGuid botGuid);

    /**
     * @brief Remove group blackboard
     * @param groupId Group ID
     */
    static void RemoveGroupBlackboard(uint32 groupId);

    /**
     * @brief Remove raid blackboard
     * @param raidId Raid ID
     */
    static void RemoveRaidBlackboard(uint32 raidId);

    /**
     * @brief Remove zone blackboard
     * @param zoneId Zone ID
     */
    static void RemoveZoneBlackboard(uint32 zoneId);

    /**
     * @brief Clear all blackboards
     */
    static void ClearAll();

    /**
     * @brief Propagate value from bot to group
     * @param botGuid Bot GUID
     * @param groupId Group ID
     * @param key Key to propagate
     */
    static void PropagateToGroup(ObjectGuid botGuid, uint32 groupId, ::std::string const& key);

    /**
     * @brief Propagate value from group to raid
     * @param groupId Group ID
     * @param raidId Raid ID
     * @param key Key to propagate
     */
    static void PropagateToRaid(uint32 groupId, uint32 raidId, ::std::string const& key);

    /**
     * @brief Propagate value from raid to zone
     * @param raidId Raid ID
     * @param zoneId Zone ID
     * @param key Key to propagate
     */
    static void PropagateToZone(uint32 raidId, uint32 zoneId, ::std::string const& key);

private:
    static ::std::unordered_map<ObjectGuid, ::std::unique_ptr<SharedBlackboard>> _botBlackboards;
    static ::std::unordered_map<uint32, ::std::unique_ptr<SharedBlackboard>> _groupBlackboards;
    static ::std::unordered_map<uint32, ::std::unique_ptr<SharedBlackboard>> _raidBlackboards;
    static ::std::unordered_map<uint32, ::std::unique_ptr<SharedBlackboard>> _zoneBlackboards;

    static ::std::shared_mutex _botMutex;
    static ::std::shared_mutex _groupMutex;
    static ::std::shared_mutex _raidMutex;
    static ::std::shared_mutex _zoneMutex;
};

} // namespace Playerbot

#endif // TRINITYCORE_SHARED_BLACKBOARD_H
