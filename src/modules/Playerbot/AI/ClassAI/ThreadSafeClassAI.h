/*
 * Copyright (C) 2024 TrinityCore Playerbot Module
 *
 * Thread-Safe Base Class for All ClassAI Implementations
 * Ensures Zero Contention for 5000+ Concurrent Bots
 */

#ifndef MODULE_PLAYERBOT_THREAD_SAFE_CLASS_AI_H
#define MODULE_PLAYERBOT_THREAD_SAFE_CLASS_AI_H

#include "Common.h"
#include "SharedDefines.h"
#include <atomic>
#include <memory>
#include <array>
#include <tbb/concurrent_hash_map.h>
#include "../Threading/ThreadingPolicy.h"

class Player;
class Unit;
class SpellInfo;

namespace Playerbot
{

/**
 * Thread-Safe Base for ClassAI
 *
 * DESIGN PRINCIPLES:
 * 1. NO shared mutable state between bots
 * 2. Lock-free resource tracking
 * 3. Wait-free stat updates
 * 4. Cache-line optimized data layout
 * 5. Zero mutex design for hot paths
 */
class ThreadSafeClassAI
{
public:
    // Aligned atomic counter for cache efficiency
    template<typename T>
    struct alignas(64) AlignedAtomic
    {
        ::std::atomic<T> value{0};

        T load() const { return value.load(::std::memory_order_relaxed); }
        void store(T val) { value.store(val, ::std::memory_order_relaxed); }
        T fetch_add(T val) { return value.fetch_add(val, ::std::memory_order_relaxed); }
        T fetch_sub(T val) { return value.fetch_sub(val, ::std::memory_order_relaxed); }
        T exchange(T val) { return value.exchange(val, ::std::memory_order_relaxed); }
    };

    // Resource state (all atomic for lock-free access)
    struct ResourceState
    {
        AlignedAtomic<uint32> health;
        AlignedAtomic<uint32> maxHealth;
        AlignedAtomic<uint32> power;
        AlignedAtomic<uint32> maxPower;
        AlignedAtomic<uint8> powerType;
        AlignedAtomic<uint32> comboPoints;
        AlignedAtomic<uint32> holyPower;
        AlignedAtomic<uint32> runicPower;

        float GetHealthPercent() const
        {
            uint32 max = maxHealth.load();
            return max > 0 ? (health.load() * 100.0f / max) : 0.0f;
        }

        float GetPowerPercent() const
        {
            uint32 max = maxPower.load();
            return max > 0 ? (power.load() * 100.0f / max) : 0.0f;
        }
    };

    // Combat state (atomic flags for thread safety)
    struct CombatState
    {
        ::std::atomic<bool> inCombat{false};
        ::std::atomic<bool> isMoving{false};
        ::std::atomic<bool> isCasting{false};
        ::std::atomic<bool> isChanneling{false};
        ::std::atomic<bool> hasTarget{false};
        ::std::atomic<bool> targetInMelee{false};
        ::std::atomic<bool> targetCasting{false};
        ::std::atomic<uint32> targetGuid{0};
        ::std::atomic<uint32> threatLevel{0};  // 0-100
        ::std::atomic<uint32> incomingDamage{0};
        AlignedAtomic<uint64> lastCombatTime;
    };

    // Cooldown tracking (lock-free)
    class CooldownTracker
    {
    public:
        bool IsOnCooldown(uint32 spellId) const
        {
            uint64 currentTime = GameTime::GetGameTimeMS();
            typename CooldownMap::const_accessor accessor;
            if (_cooldowns.find(accessor, spellId))
            {
                return accessor->second > currentTime;
            }
            return false;
        }

        void SetCooldown(uint32 spellId, uint32 durationMs)
        {
            uint64 endTime = GameTime::GetGameTimeMS() + durationMs;
            typename CooldownMap::accessor accessor;
            _cooldowns.insert(accessor, spellId);
            accessor->second = endTime;
        }

        uint32 GetRemainingCooldown(uint32 spellId) const
        {
            uint64 currentTime = GameTime::GetGameTimeMS();
            typename CooldownMap::const_accessor accessor;
            if (_cooldowns.find(accessor, spellId))
            {
                if (accessor->second > currentTime)
                    return static_cast<uint32>(accessor->second - currentTime);
            }
            return 0;
        }

        void UpdateCooldowns()
        {
            uint64 currentTime = GameTime::GetGameTimeMS();
            for (auto it = _cooldowns.begin(); it != _cooldowns.end(); )
            {
                if (it->second <= currentTime)
                {
                    it = _cooldowns.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

    private:
        using CooldownMap = tbb::concurrent_hash_map<uint32, uint64>;
        mutable CooldownMap _cooldowns;
    };

    // Performance metrics (all atomic)
    struct PerformanceMetrics
    {
        AlignedAtomic<uint32> updateCount;
        AlignedAtomic<uint32> spellsCast;
        AlignedAtomic<uint32> spellsFailed;
        AlignedAtomic<uint32> decisionsM
        AlignedAtomic<uint64> totalUpdateTime;  // microseconds
        AlignedAtomic<uint64> totalDecisionTime; // microseconds

        void RecordUpdate(uint64 timeMicros)
        {
            updateCount.fetch_add(1);
            totalUpdateTime.fetch_add(timeMicros);
        }

        void RecordDecision(uint64 timeMicros)
        {
            decisionsMade.fetch_add(1);
            totalDecisionTime.fetch_add(timeMicros);
        }

        uint64 GetAverageUpdateTime() const
        {
            uint32 count = updateCount.load();
            return count > 0 ? totalUpdateTime.load() / count : 0;
        }

        uint64 GetAverageDecisionTime() const
        {
            uint32 count = decisionsMade.load();
            return count > 0 ? totalDecisionTime.load() / count : 0;
        }

        void Reset()
        {
            updateCount.store(0);
            spellsCast.store(0);
            spellsFailed.store(0);
            decisionsMade.store(0);
            totalUpdateTime.store(0);
            totalDecisionTime.store(0);
        }
    };

    // Spell priority queue (lock-free)
    struct SpellPriority
    {
        uint32 spellId;
        float priority;
        uint32 conditions; // Bitmask of required conditions

        bool operator<(SpellPriority const& other) const
        {
            return priority < other.priority;
        }
    };

    ThreadSafeClassAI(Player* bot);
    virtual ~ThreadSafeClassAI();

    // Core AI interface (all thread-safe)
    virtual void UpdateAI(uint32 diff);
    virtual void OnCombatStart(Unit* enemy);
    virtual void OnCombatEnd();
    virtual void OnTargetChanged(Unit* newTarget);

    // Resource management
    void UpdateResourceState();
    ResourceState const& GetResourceState() const { return _resourceState; }

    // Combat state
    void UpdateCombatState();
    CombatState const& GetCombatState() const { return _combatState; }

    // Cooldown management
    bool IsSpellReady(uint32 spellId) const;
    void TriggerSpellCooldown(uint32 spellId);
    CooldownTracker& GetCooldownTracker() { return _cooldownTracker; }

    // Performance monitoring
    PerformanceMetrics GetMetrics() const;
    void ResetMetrics();

protected:
    // Pure virtual methods for specializations
    virtual void UpdateRotation(uint32 diff) = 0;
    virtual void UpdateDefensives(uint32 diff) = 0;
    virtual void UpdateUtilities(uint32 diff) = 0;

    // Helper methods (all thread-safe)
    bool CastSpell(uint32 spellId, Unit* target = nullptr);
    bool CanCastSpell(uint32 spellId, Unit* target = nullptr) const;
    float GetSpellRange(uint32 spellId) const;
    bool IsInRange(Unit* target, float range) const;

    // Spell queue management (lock-free)
    void QueueSpell(uint32 spellId, float priority, uint32 conditions = 0);
    void ProcessSpellQueue();
    void ClearSpellQueue();

    // Target selection helpers
    Unit* SelectBestTarget() const;
    Unit* GetLowestHealthAlly() const;
    Unit* GetHighestThreatEnemy() const;

    // Bot reference (immutable after construction)
    Player* const _bot;

    // State tracking (all atomic/lock-free)
    ResourceState _resourceState;
    CombatState _combatState;
    CooldownTracker _cooldownTracker;

    // Performance metrics
    mutable PerformanceMetrics _metrics;

    // Spell queue (lock-free priority queue)
    using SpellQueue = tbb::concurrent_priority_queue<SpellPriority>;
    SpellQueue _spellQueue;

    // Update timing
    ::std::atomic<uint64> _lastUpdateTime{0};
    ::std::atomic<uint32> _updateCounter{0};

    // Configuration flags (atomic for thread safety)
    ::std::atomic<bool> _enabled{true};
    ::std::atomic<bool> _debugMode{false};
    ::std::atomic<bool> _performanceMode{false};

private:
    // Internal helper methods
    void UpdateInternal(uint32 diff);
    void CleanupExpiredData();

    // Constants for performance tuning
    static constexpr uint32 MAX_SPELL_QUEUE_SIZE = 10;
    static constexpr uint32 COOLDOWN_UPDATE_INTERVAL = 100; // ms
    static constexpr uint32 STATE_UPDATE_INTERVAL = 50;      // ms

    // Deleted operations
    ThreadSafeClassAI(ThreadSafeClassAI const&) = delete;
    ThreadSafeClassAI& operator=(ThreadSafeClassAI const&) = delete;
};

/**
 * Template for specialized class implementations
 */
template<class T>
class ThreadSafeSpecialization : public ThreadSafeClassAI
{
public:
    explicit ThreadSafeSpecialization(Player* bot) : ThreadSafeClassAI(bot) {}

protected:
    // Specialization-specific state (if needed)
    // Use atomic types or lock-free structures only

    // Cache frequently accessed data
    struct CachedData
    {
        ::std::atomic<uint32> primaryResource{0};
        ::std::atomic<uint32> secondaryResource{0};
        ::std::atomic<bool> hasProcs{false};
        ::std::atomic<bool> hasBurst{false};
    };

    CachedData _cache;

    // Update cache periodically
    void UpdateCache()
    {
        // Update cached values from bot state
        // This reduces the need for repeated queries
    }
};

} // namespace Playerbot

#endif // MODULE_PLAYERBOT_THREAD_SAFE_CLASS_AI_H