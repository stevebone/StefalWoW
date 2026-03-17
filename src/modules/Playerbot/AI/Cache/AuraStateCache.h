/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Thread-Safe Aura State Cache for Worker Thread Access
 *
 * PROBLEM SOLVED:
 * Bot AI runs on ThreadPool worker threads for 7x performance improvement.
 * However, Unit::HasAura() accesses Unit::_appliedAuras which is also accessed
 * by the main thread (e.g., in AreaTrigger::Update). This causes race conditions
 * and ACCESS_VIOLATION crashes.
 *
 * SOLUTION:
 * This cache is populated ONLY from the main thread during BotActionProcessor
 * execution. Worker threads can safely read cached aura state without accessing
 * Unit internals.
 *
 * ARCHITECTURE:
 * Main Thread: BotActionProcessor::Update() -> AuraStateCache::UpdateBotAuras()
 * Worker Thread: BehaviorTree nodes -> AuraStateCache::HasCachedAura()
 *
 * PERFORMANCE:
 * - Cache size: O(bots * tracked_auras), typically <100KB for 1000 bots
 * - Update cost: O(tracked_auras) per bot per main thread tick
 * - Query cost: O(1) hash lookup
 * - Cache TTL: 1 second (configurable), auto-expires stale entries
 */

#ifndef AURA_STATE_CACHE_H
#define AURA_STATE_CACHE_H

#include "Define.h"
#include "ObjectGuid.h"
#include <atomic>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <vector>

class Player;
class Unit;

namespace Playerbot
{

/**
 * @brief Cached aura entry with expiration time
 */
struct CachedAuraEntry
{
    uint32 spellId;
    ObjectGuid casterGuid;        // Who applied the aura
    uint32 stacks;                // Stack count
    uint32 duration;              // Remaining duration in ms
    std::chrono::steady_clock::time_point cachedAt;
    std::chrono::steady_clock::time_point expiresAt;

    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() > expiresAt;
    }
};

/**
 * @brief Key for aura lookup: (UnitGUID, SpellID, CasterGUID)
 */
struct AuraCacheKey
{
    ObjectGuid unitGuid;
    uint32 spellId;
    ObjectGuid casterGuid;  // Empty = any caster

    bool operator==(AuraCacheKey const& other) const
    {
        return unitGuid == other.unitGuid &&
               spellId == other.spellId &&
               casterGuid == other.casterGuid;
    }
};

struct AuraCacheKeyHash
{
    std::size_t operator()(AuraCacheKey const& key) const
    {
        // Use std::hash specialization for ObjectGuid
        std::size_t h1 = std::hash<ObjectGuid>{}(key.unitGuid);
        std::size_t h2 = std::hash<uint32>{}(key.spellId);
        std::size_t h3 = std::hash<ObjectGuid>{}(key.casterGuid);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

/**
 * @brief Thread-safe aura state cache for bot AI worker threads
 *
 * Thread Safety:
 * - UpdateBotAuras(): MAIN THREAD ONLY
 * - UpdateTargetAuras(): MAIN THREAD ONLY
 * - HasCachedAura(): Thread-safe (uses shared_lock)
 * - GetCachedAura(): Thread-safe (uses shared_lock)
 *
 * Usage:
 * 1. Main thread calls UpdateBotAuras(bot) periodically
 * 2. Main thread calls UpdateTargetAuras(bot, target) when target changes
 * 3. Worker threads call HasCachedAura() instead of Unit::HasAura()
 */
class TC_GAME_API AuraStateCache
{
private:
    AuraStateCache();
    ~AuraStateCache() = default;

public:
    AuraStateCache(AuraStateCache const&) = delete;
    AuraStateCache& operator=(AuraStateCache const&) = delete;

    static AuraStateCache* instance();

    // ============================================================
    // MAIN THREAD ONLY - Cache Population
    // ============================================================

    /**
     * @brief Update cached auras for a bot (MAIN THREAD ONLY)
     *
     * Call this from BotActionProcessor::Update() or similar main-thread code.
     * Caches all auras on the bot that are commonly checked by AI.
     *
     * @param bot Player bot to cache auras for
     */
    void UpdateBotAuras(Player* bot);

    /**
     * @brief Update cached auras for a target unit (MAIN THREAD ONLY)
     *
     * Call this when bot's target changes or periodically for current target.
     * Caches auras applied BY the bot TO the target.
     *
     * @param bot Bot that applied the auras
     * @param target Target unit to check
     */
    void UpdateTargetAuras(Player* bot, Unit* target);

    /**
     * @brief Update a specific aura state (MAIN THREAD ONLY)
     *
     * @param unitGuid Unit that has the aura
     * @param spellId Spell ID of the aura
     * @param casterGuid Who cast the aura (Empty = any)
     * @param hasAura Whether the aura is present
     * @param stacks Stack count (default 1)
     * @param duration Remaining duration in ms (default 0)
     */
    void SetAuraState(ObjectGuid unitGuid, uint32 spellId, ObjectGuid casterGuid,
                      bool hasAura, uint32 stacks = 1, uint32 duration = 0);

    /**
     * @brief Invalidate all cached auras for a unit (MAIN THREAD ONLY)
     *
     * @param unitGuid Unit whose cache to invalidate
     */
    void InvalidateUnit(ObjectGuid unitGuid);

    /**
     * @brief Clear entire cache (MAIN THREAD ONLY)
     */
    void Clear();

    /**
     * @brief Cleanup expired entries (MAIN THREAD ONLY)
     *
     * Called periodically to remove stale cache entries.
     */
    void CleanupExpired();

    // ============================================================
    // THREAD-SAFE - Worker Thread Access
    // ============================================================

    /**
     * @brief Check if unit has cached aura (THREAD-SAFE)
     *
     * Safe to call from worker threads. Returns cached state.
     *
     * @param unitGuid Unit to check
     * @param spellId Spell ID to check
     * @param casterGuid Required caster (Empty = any caster)
     * @return true if aura is cached as present and not expired
     */
    bool HasCachedAura(ObjectGuid unitGuid, uint32 spellId, ObjectGuid casterGuid = ObjectGuid::Empty) const;

    /**
     * @brief Get cached aura details (THREAD-SAFE)
     *
     * @param unitGuid Unit to check
     * @param spellId Spell ID to check
     * @param casterGuid Required caster (Empty = any caster)
     * @param outEntry Output entry if found
     * @return true if aura is cached and not expired
     */
    bool GetCachedAura(ObjectGuid unitGuid, uint32 spellId, ObjectGuid casterGuid,
                       CachedAuraEntry& outEntry) const;

    /**
     * @brief Check if cache has any data for unit (THREAD-SAFE)
     *
     * @param unitGuid Unit to check
     * @return true if unit has cached aura data
     */
    bool HasCachedData(ObjectGuid unitGuid) const;

    /**
     * @brief Get cache freshness for unit (THREAD-SAFE)
     *
     * @param unitGuid Unit to check
     * @return Milliseconds since last cache update, or UINT32_MAX if never cached
     */
    uint32 GetCacheAge(ObjectGuid unitGuid) const;

    // ============================================================
    // Configuration
    // ============================================================

    /**
     * @brief Set cache TTL (time-to-live)
     * @param ttlMs TTL in milliseconds (default 1000ms)
     */
    void SetCacheTTL(uint32 ttlMs) { _cacheTTL = std::chrono::milliseconds(ttlMs); }

    /**
     * @brief Get cache TTL
     * @return TTL in milliseconds
     */
    uint32 GetCacheTTL() const { return static_cast<uint32>(_cacheTTL.count()); }

    /**
     * @brief Register spell IDs to track for bots
     *
     * Only registered spell IDs are cached to limit memory usage.
     *
     * @param spellIds Vector of spell IDs to track
     */
    void RegisterTrackedSpells(std::vector<uint32> const& spellIds);

    /**
     * @brief Add a single tracked spell
     * @param spellId Spell ID to track
     */
    void AddTrackedSpell(uint32 spellId);

    // ============================================================
    // Statistics (THREAD-SAFE)
    // ============================================================

    struct CacheStats
    {
        uint32 totalEntries;
        uint32 expiredEntries;
        uint32 cacheHits;
        uint32 cacheMisses;
        uint32 updateCount;
    };

    CacheStats GetStats() const;
    void ResetStats();

private:
    mutable std::shared_mutex _mutex;

    // Main cache storage: AuraCacheKey -> CachedAuraEntry
    std::unordered_map<AuraCacheKey, CachedAuraEntry, AuraCacheKeyHash> _cache;

    // Per-unit last update time for cache age queries
    std::unordered_map<ObjectGuid, std::chrono::steady_clock::time_point> _unitLastUpdate;

    // Tracked spell IDs (only these are cached to limit memory)
    std::unordered_set<uint32> _trackedSpells;

    // Configuration
    std::chrono::milliseconds _cacheTTL{1000};  // 1 second default

    // Statistics (atomic for thread-safe reads)
    mutable std::atomic<uint32> _cacheHits{0};
    mutable std::atomic<uint32> _cacheMisses{0};
    std::atomic<uint32> _updateCount{0};

    // Default tracked spells (common buffs/debuffs checked by AI)
    void RegisterDefaultTrackedSpells();
};

} // namespace Playerbot

#define sAuraStateCache Playerbot::AuraStateCache::instance()

#endif // AURA_STATE_CACHE_H
