/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * STRING INTERNING POOL
 *
 * Eliminates duplicate string allocations across all bot instances by
 * maintaining a shared pool of unique strings. When 500 bots each store
 * the same spell name, class name, zone name, or log message, only one
 * copy exists in memory.
 *
 * Architecture:
 *   - Global singleton, thread-safe via shared_mutex (read-heavy)
 *   - FNV-1a hash for O(1) lookup
 *   - Returns std::string_view to interned strings (zero-copy)
 *   - Strings are never freed (permanent interning for repeated strings)
 *   - Optional category tracking for memory profiling
 *
 * Memory Savings Estimate:
 *   - 500 bots x ~200 unique strings x avg 32 bytes = ~3.2MB saved
 *   - Plus std::string overhead (~32 bytes per string) = ~3.2MB more
 *   - Total estimated savings: ~6.4MB for 500 bots
 *
 * Usage:
 *   // Instead of storing std::string copies:
 *   std::string_view name = sStringInterningPool.Intern("Fireball");
 *   std::string_view zone = sStringInterningPool.Intern("Stormwind City");
 *
 *   // With category tracking:
 *   auto name = sStringInterningPool.Intern("Fireball", StringCategory::SPELL_NAME);
 *
 * Thread Safety:
 *   - Intern() is thread-safe (write lock only on first insert)
 *   - Returned string_views are valid for the lifetime of the pool
 *   - Read operations use shared lock for concurrent access
 */

#pragma once

#include "Define.h"
#include <array>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>

namespace Playerbot
{

// ============================================================================
// STRING CATEGORIES (for memory profiling)
// ============================================================================

enum class StringCategory : uint8
{
    UNCATEGORIZED   = 0,
    SPELL_NAME      = 1,
    ITEM_NAME       = 2,
    CLASS_NAME      = 3,
    SPEC_NAME       = 4,
    ZONE_NAME       = 5,
    NPC_NAME        = 6,
    QUEST_NAME      = 7,
    LOG_MESSAGE     = 8,
    CONFIG_KEY      = 9,
    COMMAND         = 10,
    AURA_NAME       = 11,
    TALENT_NAME     = 12,
    CHAT_MESSAGE    = 13,
    MISC            = 14,
    CATEGORY_COUNT  = 15
};

// ============================================================================
// STRING INTERNING STATISTICS
// ============================================================================

struct StringInterningStats
{
    ::std::atomic<uint64> totalInternRequests{0};   // Total Intern() calls
    ::std::atomic<uint64> cacheHits{0};             // Times string already existed
    ::std::atomic<uint64> newInsertions{0};          // Times a new string was inserted
    ::std::atomic<uint64> totalBytesInterned{0};     // Total bytes in the pool
    ::std::atomic<uint32> uniqueStrings{0};          // Number of unique strings

    // Per-category counts
    ::std::array<::std::atomic<uint32>, static_cast<size_t>(StringCategory::CATEGORY_COUNT)> categoryCounts{};

    float GetHitRate() const
    {
        uint64 total = totalInternRequests.load(::std::memory_order_relaxed);
        if (total == 0) return 0.0f;
        return static_cast<float>(cacheHits.load(::std::memory_order_relaxed)) / static_cast<float>(total);
    }

    uint64 GetEstimatedSavings(uint32 botCount) const
    {
        // Each bot would have its own copy without interning
        // Savings = (botCount - 1) * totalBytesInterned + uniqueStrings * 32 (std::string overhead)
        uint64 bytes = totalBytesInterned.load(::std::memory_order_relaxed);
        uint32 strings = uniqueStrings.load(::std::memory_order_relaxed);
        return (botCount > 1) ? (botCount - 1) * (bytes + strings * 32) : 0;
    }
};

// ============================================================================
// STRING INTERNING POOL
// ============================================================================

class TC_GAME_API StringInterningPool
{
public:
    static StringInterningPool& Instance()
    {
        static StringInterningPool instance;
        return instance;
    }

    // ========================================================================
    // CORE: INTERN A STRING
    // ========================================================================

    /// Intern a string. Returns a string_view to the pooled copy.
    /// If the string already exists in the pool, returns the existing copy.
    /// Thread-safe. Returned view is valid for the lifetime of the pool.
    /// @param str String to intern
    /// @param category Optional category for profiling
    /// @return string_view to the interned string
    ::std::string_view Intern(::std::string_view str, StringCategory category = StringCategory::UNCATEGORIZED);

    /// Intern a C-string
    ::std::string_view Intern(char const* str, StringCategory category = StringCategory::UNCATEGORIZED);

    /// Intern a std::string
    ::std::string_view Intern(::std::string const& str, StringCategory category = StringCategory::UNCATEGORIZED);

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Check if a string is already interned
    bool Contains(::std::string_view str) const;

    /// Get the number of unique strings in the pool
    uint32 GetSize() const;

    /// Get total memory used by the pool (approximate)
    uint64 GetMemoryUsage() const;

    /// Get statistics
    StringInterningStats const& GetStats() const { return _stats; }

    /// Get a debug summary string
    ::std::string GetDebugSummary() const;

    /// Get per-category statistics
    uint32 GetCategoryCount(StringCategory category) const;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /// Clear the pool (WARNING: invalidates all returned string_views)
    /// Only call during shutdown when no string_views are in use.
    void Clear();

    /// Pre-intern common strings (spell names, class names, etc.)
    /// Call during module initialization for optimal performance.
    void PreInternCommonStrings();

private:
    StringInterningPool() = default;
    ~StringInterningPool() = default;
    StringInterningPool(StringInterningPool const&) = delete;
    StringInterningPool& operator=(StringInterningPool const&) = delete;

    // ========================================================================
    // INTERNAL STATE
    // ========================================================================

    /// The actual storage. Using unordered_set<std::string> which owns the strings.
    /// string_view into these strings remains valid as long as the set isn't rehashed
    /// with the string removed. Since we never remove, views are permanently valid.
    ///
    /// We use a custom hash/equal that works with string_view for lookups
    /// to avoid constructing temporary std::string for Contains/find operations.
    struct StringHash
    {
        using is_transparent = void;

        ::std::size_t operator()(::std::string_view sv) const noexcept
        {
            // FNV-1a hash
            ::std::size_t hash = 14695981039346656037ULL;
            for (char c : sv)
            {
                hash ^= static_cast<::std::size_t>(c);
                hash *= 1099511628211ULL;
            }
            return hash;
        }

        ::std::size_t operator()(::std::string const& s) const noexcept
        {
            return operator()(::std::string_view(s));
        }
    };

    struct StringEqual
    {
        using is_transparent = void;

        bool operator()(::std::string const& a, ::std::string const& b) const noexcept
        {
            return a == b;
        }

        bool operator()(::std::string const& a, ::std::string_view b) const noexcept
        {
            return a == b;
        }

        bool operator()(::std::string_view a, ::std::string const& b) const noexcept
        {
            return a == b;
        }
    };

    ::std::unordered_set<::std::string, StringHash, StringEqual> _pool;

    /// Read-write lock: reads are concurrent, writes are exclusive
    mutable ::std::shared_mutex _mutex;

    /// Statistics
    mutable StringInterningStats _stats;
};

/// Global accessor macro
#define sStringInterningPool Playerbot::StringInterningPool::Instance()

} // namespace Playerbot
