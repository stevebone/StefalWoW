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

#pragma once

#include "Define.h"
#include "Strategy/Strategy.h"
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <string_view>

namespace Playerbot {

// Forward declarations
class BotAI;

/**
 * @enum BehaviorPriority
 * @brief Priority levels for bot behaviors (higher = more important)
 *
 * This system ensures that only the highest-priority applicable behavior
 * runs at any given time, preventing conflicts like Issue #2 & #3.
 *
 * CRITICAL: Combat (100) > Follow (50) prevents simultaneous execution
 */
enum class BehaviorPriority : uint8_t {
    // Critical states (non-operational)
    DEAD = 0,
    ERROR_PRIORITY = 5,

    // Operational priorities
    COMBAT = 100,        // Highest - exclusive control during combat
    FLEEING = 90,        // Escape/survival
    CASTING = 80,        // Spell casting (blocks movement)
    FOLLOW = 50,         // Follow leader (only when not in combat)
    MOVEMENT = 45,       // General movement
    GATHERING = 40,      // Resource gathering
    TRADING = 30,        // Trade/merchant
    SOCIAL = 20,         // Chat/emotes
    SOLO = 10            // Lowest - solo play mode (autonomous questing/gathering/combat)
};

/**
 * @struct BehaviorMetadata
 * @brief Metadata about a behavior for priority system
 */
struct BehaviorMetadata {
    Strategy* strategy;
    BehaviorPriority priority;
    bool exclusive;              // If true, no other behaviors can run
    bool allowsLowerPriority;    // If true, allows lower priority behaviors to run
    ::std::set<BehaviorPriority> conflicts; // Priorities this conflicts with

    BehaviorMetadata()
        : strategy(nullptr)
        , priority(BehaviorPriority::SOLO)
        , exclusive(false)
        , allowsLowerPriority(true)
    {}
};

/**
 * @class BehaviorPriorityManager
 * @brief Coordinates strategy execution using priority system
 *
 * This class EXTENDS the existing BehaviorManager infrastructure by adding
 * priority-based coordination. It works WITH the existing throttling and
 * atomic flags from BehaviorManager, not against them.
 *
 * Key Features:
 * - Priority-based strategy selection
 * - Mutual exclusion rules (combat excludes follow)
 * - Handles transitions between priorities
 * - Integrates with existing BehaviorManager
 *
 * Integration with Phase 1:
 * - Uses BotStateMachine states for context
 * - Respects SafeObjectReference validity
 * - Works with event system skeleton
 *
 * Integration with OLD Phase 2:
 * - Uses BehaviorManager's IsEnabled() atomic flags
 * - Respects BehaviorManager's throttling
 * - Works with SoloStrategy observer pattern
 *
 * Fixes:
 * - Issue #2: Ensures combat has exclusive control (no follow interference)
 * - Issue #3: Ensures facing is set by combat, not follow
 */
class TC_GAME_API BehaviorPriorityManager {
public:
    explicit BehaviorPriorityManager(BotAI* ai);
    ~BehaviorPriorityManager();

    // Delete copy/move
    BehaviorPriorityManager(const BehaviorPriorityManager&) = delete;
    BehaviorPriorityManager& operator=(const BehaviorPriorityManager&) = delete;
    BehaviorPriorityManager(BehaviorPriorityManager&&) = delete;
    BehaviorPriorityManager& operator=(BehaviorPriorityManager&&) = delete;

    // ========================================================================
    // STRATEGY REGISTRATION
    // ========================================================================

    /**
     * @brief Register a strategy with priority
     * @param strategy Strategy to register
     * @param priority Priority level
     * @param exclusive If true, no other strategies can run simultaneously
     */
    void RegisterStrategy(
        Strategy* strategy,
        BehaviorPriority priority,
        bool exclusive = false
    );

    /**
     * @brief Unregister a strategy
     * @param strategy Strategy to remove
     */
    void UnregisterStrategy(Strategy* strategy);

    /**
     * @brief Add mutual exclusion rule
     * @param a First priority
     * @param b Second priority (cannot run with 'a')
     *
     * Example: AddExclusionRule(COMBAT, FOLLOW) prevents both from running
     */
    void AddExclusionRule(BehaviorPriority a, BehaviorPriority b);

    // ========================================================================
    // STRATEGY SELECTION
    // ========================================================================

    /**
     * @brief Select the highest priority active strategy
     * @param activeStrategies List of currently active strategies
     * @return Pointer to strategy that should execute, or nullptr
     *
     * Algorithm:
     * 1. Filter strategies by IsActive()
     * 2. Sort by priority (descending)
     * 3. Check mutual exclusion rules
     * 4. Return highest priority valid strategy
     */
    Strategy* SelectActiveBehavior(::std::vector<Strategy*>& activeStrategies);

    /**
     * @brief Get all active strategies sorted by priority
     * @return Vector of strategies (highest priority first)
     */
    ::std::vector<Strategy*> GetPrioritizedStrategies() const;

    /**
     * @brief Check if two behaviors can coexist
     * @param a First strategy
     * @param b Second strategy
     * @return true if both can run simultaneously
     */
    bool CanCoexist(Strategy* a, Strategy* b) const;

    /**
     * @brief Check if a strategy is currently allowed to run
     * @param strategy Strategy to check
     * @return true if no higher priority behaviors are active
     */
    bool IsAllowedToRun(Strategy* strategy) const;

    // ========================================================================
    // CONTEXT & STATE
    // ========================================================================

    /**
     * @brief Update priority context based on bot state
     *
     * This method checks:
     * - Bot's combat state (sets COMBAT priority if in combat)
     * - Bot's health/mana (sets FLEEING if low)
     * - Bot's group state (affects FOLLOW priority)
     */
    void UpdateContext();

    /**
     * @brief Get current active priority
     * @return Highest priority currently active
     */
    BehaviorPriority GetActivePriority() const;

    /**
     * @brief Check if a specific priority is currently active
     * @param priority Priority to check
     * @return true if active
     */
    bool IsPriorityActive(BehaviorPriority priority) const;

    // ========================================================================
    // DIAGNOSTICS
    // ========================================================================

    /**
     * @brief Dump current priority state to log
     */
    void DumpPriorityState() const;

    /**
     * @brief Get exclusion rules for a priority
     * @param priority Priority to query
     * @return Set of conflicting priorities
     */
    ::std::set<BehaviorPriority> GetConflicts(BehaviorPriority priority) const;

private:
    BotAI* m_ai;

    // Strategy metadata by priority
    ::std::map<BehaviorPriority, ::std::vector<BehaviorMetadata>> m_strategies;

    // Mutual exclusion rules
    ::std::map<BehaviorPriority, ::std::set<BehaviorPriority>> m_exclusionRules;

    // Current active priority
    BehaviorPriority m_activePriority{BehaviorPriority::SOLO};

    // Last selected strategy (for transition logging)
    Strategy* m_lastSelectedStrategy{nullptr};

    /**
     * @brief Internal: Check if priority is blocked by exclusion rules
     */
    bool IsBlockedByExclusion(
        BehaviorPriority priority,
        const ::std::vector<BehaviorPriority>& activePriorities
    ) const;

    /**
     * @brief Internal: Get metadata for a strategy
     */
    BehaviorMetadata* FindMetadata(Strategy* strategy);
    const BehaviorMetadata* FindMetadata(Strategy* strategy) const;
};

/**
 * @brief Helper: Convert priority to string
 */
TC_GAME_API ::std::string_view ToString(BehaviorPriority priority);

} // namespace Playerbot