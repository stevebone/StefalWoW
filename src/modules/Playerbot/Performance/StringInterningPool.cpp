/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "StringInterningPool.h"
#include "Log.h"
#include <sstream>

namespace Playerbot
{

// ============================================================================
// CORE: INTERN A STRING
// ============================================================================

::std::string_view StringInterningPool::Intern(::std::string_view str, StringCategory category)
{
    if (str.empty())
        return {};

    _stats.totalInternRequests.fetch_add(1, ::std::memory_order_relaxed);

    // Fast path: read lock to check if already interned
    {
        ::std::shared_lock<::std::shared_mutex> readLock(_mutex);
        auto it = _pool.find(str);
        if (it != _pool.end())
        {
            _stats.cacheHits.fetch_add(1, ::std::memory_order_relaxed);
            return ::std::string_view(*it);
        }
    }

    // Slow path: write lock to insert
    {
        ::std::unique_lock<::std::shared_mutex> writeLock(_mutex);

        // Double-check after acquiring write lock (another thread may have inserted)
        auto it = _pool.find(str);
        if (it != _pool.end())
        {
            _stats.cacheHits.fetch_add(1, ::std::memory_order_relaxed);
            return ::std::string_view(*it);
        }

        // Insert new string
        auto [insertIt, inserted] = _pool.emplace(str);
        if (inserted)
        {
            _stats.newInsertions.fetch_add(1, ::std::memory_order_relaxed);
            _stats.totalBytesInterned.fetch_add(static_cast<uint64>(str.size()), ::std::memory_order_relaxed);
            _stats.uniqueStrings.fetch_add(1, ::std::memory_order_relaxed);

            if (category != StringCategory::UNCATEGORIZED)
            {
                _stats.categoryCounts[static_cast<size_t>(category)].fetch_add(1, ::std::memory_order_relaxed);
            }
        }

        return ::std::string_view(*insertIt);
    }
}

::std::string_view StringInterningPool::Intern(char const* str, StringCategory category)
{
    if (!str || str[0] == '\0')
        return {};

    return Intern(::std::string_view(str), category);
}

::std::string_view StringInterningPool::Intern(::std::string const& str, StringCategory category)
{
    return Intern(::std::string_view(str), category);
}

// ============================================================================
// QUERIES
// ============================================================================

bool StringInterningPool::Contains(::std::string_view str) const
{
    ::std::shared_lock<::std::shared_mutex> readLock(_mutex);
    return _pool.find(str) != _pool.end();
}

uint32 StringInterningPool::GetSize() const
{
    ::std::shared_lock<::std::shared_mutex> readLock(_mutex);
    return static_cast<uint32>(_pool.size());
}

uint64 StringInterningPool::GetMemoryUsage() const
{
    ::std::shared_lock<::std::shared_mutex> readLock(_mutex);

    uint64 totalBytes = 0;
    // Each string in the set has: string data + std::string overhead (~32 bytes)
    // Plus hash table overhead (~8 bytes per bucket pointer)
    for (auto const& s : _pool)
    {
        totalBytes += s.size() + sizeof(::std::string) + 8; // data + overhead + bucket
    }

    return totalBytes;
}

::std::string StringInterningPool::GetDebugSummary() const
{
    ::std::ostringstream ss;
    ss << "StringInterningPool: "
       << "unique=" << _stats.uniqueStrings.load()
       << " requests=" << _stats.totalInternRequests.load()
       << " hits=" << _stats.cacheHits.load()
       << " hitRate=" << static_cast<int>(_stats.GetHitRate() * 100.0f) << "%"
       << " bytes=" << _stats.totalBytesInterned.load()
       << " memUsage=" << GetMemoryUsage() << "B";

    // Per-category breakdown
    static const char* categoryNames[] = {
        "uncategorized", "spell", "item", "class", "spec",
        "zone", "npc", "quest", "log", "config",
        "command", "aura", "talent", "chat", "misc"
    };

    bool hasCategories = false;
    for (size_t i = 1; i < static_cast<size_t>(StringCategory::CATEGORY_COUNT); ++i)
    {
        uint32 count = _stats.categoryCounts[i].load();
        if (count > 0)
        {
            if (!hasCategories)
            {
                ss << " [";
                hasCategories = true;
            }
            else
            {
                ss << ", ";
            }
            ss << categoryNames[i] << "=" << count;
        }
    }
    if (hasCategories)
        ss << "]";

    return ss.str();
}

uint32 StringInterningPool::GetCategoryCount(StringCategory category) const
{
    if (category >= StringCategory::CATEGORY_COUNT)
        return 0;
    return _stats.categoryCounts[static_cast<size_t>(category)].load(::std::memory_order_relaxed);
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void StringInterningPool::Clear()
{
    ::std::unique_lock<::std::shared_mutex> writeLock(_mutex);
    _pool.clear();

    // Reset all atomic stats individually (std::atomic is not copy-assignable)
    _stats.totalInternRequests.store(0, ::std::memory_order_relaxed);
    _stats.cacheHits.store(0, ::std::memory_order_relaxed);
    _stats.newInsertions.store(0, ::std::memory_order_relaxed);
    _stats.totalBytesInterned.store(0, ::std::memory_order_relaxed);
    _stats.uniqueStrings.store(0, ::std::memory_order_relaxed);
    for (size_t i = 0; i < static_cast<size_t>(StringCategory::CATEGORY_COUNT); ++i)
        _stats.categoryCounts[i].store(0, ::std::memory_order_relaxed);

    TC_LOG_INFO("module.playerbot", "StringInterningPool: Cleared all interned strings");
}

void StringInterningPool::PreInternCommonStrings()
{
    // Pre-intern class names
    static const char* classNames[] = {
        "Warrior", "Paladin", "Hunter", "Rogue", "Priest",
        "Death Knight", "Shaman", "Mage", "Warlock", "Monk",
        "Druid", "Demon Hunter", "Evoker"
    };
    for (auto name : classNames)
        Intern(name, StringCategory::CLASS_NAME);

    // Pre-intern role names
    static const char* roleNames[] = {
        "Tank", "Healer", "DPS", "Melee DPS", "Ranged DPS"
    };
    for (auto name : roleNames)
        Intern(name, StringCategory::MISC);

    // Pre-intern common spec names
    static const char* specNames[] = {
        "Arms", "Fury", "Protection",
        "Holy", "Retribution",
        "Beast Mastery", "Marksmanship", "Survival",
        "Assassination", "Outlaw", "Subtlety",
        "Discipline", "Shadow",
        "Blood", "Frost", "Unholy",
        "Elemental", "Enhancement", "Restoration",
        "Arcane", "Fire",
        "Affliction", "Demonology", "Destruction",
        "Brewmaster", "Mistweaver", "Windwalker",
        "Balance", "Feral", "Guardian",
        "Havoc", "Vengeance",
        "Devastation", "Preservation", "Augmentation"
    };
    for (auto name : specNames)
        Intern(name, StringCategory::SPEC_NAME);

    // Pre-intern common log prefixes
    static const char* logPrefixes[] = {
        "Combat", "Movement", "Healing", "Targeting", "AoE",
        "Positioning", "Interrupt", "Defensive", "Offensive",
        "Phase", "Burst", "Execute", "Opener", "Sustained"
    };
    for (auto prefix : logPrefixes)
        Intern(prefix, StringCategory::LOG_MESSAGE);

    // Pre-intern common resource names
    static const char* resourceNames[] = {
        "Mana", "Rage", "Energy", "Focus", "Runic Power",
        "Soul Shards", "Astral Power", "Insanity", "Maelstrom",
        "Chi", "Fury", "Pain", "Combo Points", "Holy Power",
        "Runes", "Essence"
    };
    for (auto name : resourceNames)
        Intern(name, StringCategory::MISC);

    TC_LOG_DEBUG("module.playerbot", "StringInterningPool: Pre-interned {} common strings",
        _stats.uniqueStrings.load());
}

} // namespace Playerbot
