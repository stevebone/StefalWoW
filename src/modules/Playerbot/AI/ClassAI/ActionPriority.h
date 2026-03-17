/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Unit.h"
#include <queue>
#include <mutex>
#include <memory>
#include <vector>
#include "GameTime.h"

namespace Playerbot
{

// Action priority levels - lower values = higher priority
enum class ActionPriority : uint8
{
    EMERGENCY = 0,      // Defensive cooldowns, health potions, life-saving abilities
    SURVIVAL = 1,       // Heals, defensive abilities, dispels
    INTERRUPT = 2,      // Interrupt enemy casts, counterspells
    BURST = 3,          // Offensive cooldowns, major damage abilities
    ROTATION = 4,       // Normal rotation abilities, standard attacks
    MOVEMENT = 5,       // Positioning adjustments, gap closers
    BUFF = 6,           // Maintain buffs, weapon imbues
    IDLE = 7            // Out of combat activities, food/drink
};

// Represents a prioritized action that can be executed
struct PrioritizedAction
{
    uint32 spellId;             // Spell to cast
    ActionPriority priority;    // Priority level
    float score;                // Dynamic scoring within priority level
    ::Unit* target;             // Target for the spell (can be nullptr for self-cast)
    uint32 timestamp;           // When this action was created (for aging)

    PrioritizedAction() : spellId(0), priority(ActionPriority::IDLE), score(0.0f), target(nullptr), timestamp(0) {}

    PrioritizedAction(uint32 spell, ActionPriority prio, float sc, ::Unit* tgt = nullptr)
        : spellId(spell), priority(prio), score(sc), target(tgt), timestamp(GameTime::GetGameTimeMS()) {}

    // Comparison operator for priority queue (higher priority = lower enum value)
    bool operator<(const PrioritizedAction& other) const
    {
        if (priority != other.priority)
            return priority > other.priority; // Reverse comparison for min-heap behavior

        if (score != other.score)
            return score < other.score; // Higher score = higher priority within same level

        // Tie-breaker: newer actions have slight priority
        return timestamp < other.timestamp;
    }

    // Check if this action is still valid (not too old)
    bool IsValid(uint32 maxAgeMs = 5000) const
    {
        return (GameTime::GetGameTimeMS() - timestamp) <= maxAgeMs;
    }
};

// Thread-safe priority queue for managing bot actions
class TC_GAME_API ActionPriorityQueue
{
public:
    ActionPriorityQueue();
    ~ActionPriorityQueue() = default;

    // Add an action to the queue
    void AddAction(uint32 spellId, ActionPriority priority, float score, ::Unit* target = nullptr);

    // Get the highest priority action (removes it from queue)
    bool GetNextAction(PrioritizedAction& action);

    // Peek at the highest priority action without removing it
    bool PeekNextAction(PrioritizedAction& action) const;

    // Check if queue has any actions
    bool HasActions() const;

    // Get number of actions in queue
    size_t Size() const;

    // Clear all actions
    void Clear();

    // Remove old/invalid actions
    void CleanupOldActions(uint32 maxAgeMs = 5000);

    // Add multiple actions at once (more efficient)
    void AddActions(const ::std::vector<PrioritizedAction>& actions);

    // Get all actions of a specific priority level
    ::std::vector<PrioritizedAction> GetActionsByPriority(ActionPriority priority) const;

    // Check if queue contains action for specific spell
    bool ContainsSpell(uint32 spellId) const;

private:
    mutable ::std::priority_queue<PrioritizedAction> _queue;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _queueMutex;
    mutable ::std::atomic<size_t> _size{0};

    // Internal helper to validate action
    bool IsActionValid(const PrioritizedAction& action) const;
};

// Utility class for creating common action priorities
class TC_GAME_API ActionPriorityHelper
{
public:
    // Emergency actions
    static PrioritizedAction CreateEmergencyHeal(uint32 spellId, ::Unit* target, float healthPct);
    static PrioritizedAction CreateEmergencyDefensive(uint32 spellId, float threatLevel);
    static PrioritizedAction CreateEmergencyEscape(uint32 spellId, ::Unit* target);

    // Combat actions
    static PrioritizedAction CreateInterrupt(uint32 spellId, ::Unit* target, uint32 enemySpellId);
    static PrioritizedAction CreateBurst(uint32 spellId, ::Unit* target, float damageModifier);
    static PrioritizedAction CreateRotation(uint32 spellId, ::Unit* target, float rotationPriority);

    // Utility actions
    static PrioritizedAction CreateBuff(uint32 spellId, ::Unit* target, uint32 remainingDuration);
    static PrioritizedAction CreateMovement(uint32 spellId, ::Unit* target, float distance);

    // Scoring helpers
    static float CalculateHealScore(::Unit* target);
    static float CalculateDamageScore(::Unit* target, uint32 spellId);
    static float CalculateBuffScore(::Unit* target, uint32 spellId);
    static float CalculateInterruptScore(::Unit* target, uint32 enemySpellId);

private:
    // Internal scoring algorithms
    static float GetHealthPriorityMultiplier(float healthPct);
    static float GetThreatPriorityMultiplier(::Unit* unit);
    static float GetDistancePriorityMultiplier(float distance);
};

// Object pool for efficient action allocation
class TC_GAME_API ActionPool
{
public:
    static ActionPool& Instance();

    // Get a reusable action object
    ::std::unique_ptr<PrioritizedAction> Acquire();

    // Return an action object to the pool
    void Release(::std::unique_ptr<PrioritizedAction> action);

    // Cleanup unused objects
    void Cleanup();

private:
    ActionPool() = default;
    ~ActionPool() = default;

    ::std::vector<::std::unique_ptr<PrioritizedAction>> _pool;
    Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _poolMutex;
    static constexpr size_t MAX_POOL_SIZE = 1000;
};

} // namespace Playerbot