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

/*
 * TACTICAL COORDINATOR - Combat Coordination Subsystem
 *
 * PURPOSE:
 * Handles tactical combat coordination for groups including interrupt rotation,
 * dispel assignments, cooldown coordination, and focus target management.
 * This is a subsystem of the main GroupCoordinator.
 *
 * DESIGN:
 * - Separated from main GroupCoordinator for better SRP (Single Responsibility Principle)
 * - Focuses purely on in-combat tactical decisions
 * - Used by both dungeon groups and raid groups
 * - Performance-optimized for <1ms update times
 *
 * INTEGRATION:
 * - Created and owned by GroupCoordinator
 * - Shared across all group members for coordinated decisions
 * - Thread-safe for concurrent access from multiple bots
 *
 * RESPONSIBILITIES:
 * - Interrupt rotation management
 * - Dispel assignment coordination
 * - Group cooldown tracking
 * - Focus target selection
 * - Priority target marking
 * - Crowd control assignment
 */

#ifndef TRINITYCORE_TACTICAL_COORDINATOR_H
#define TRINITYCORE_TACTICAL_COORDINATOR_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Threading/LockHierarchy.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <string>

// Forward declarations
class Unit;
class Player;
class Group;

namespace Playerbot
{

// Forward declaration for interrupt authority
// Note: InterruptCoordinator is a type alias for InterruptCoordinatorFixed
class InterruptCoordinatorFixed;
using InterruptCoordinator = InterruptCoordinatorFixed;

// Forward declaration for CC authority (single source of truth)
class CrowdControlManager;

namespace Advanced
{

/**
 * @brief Tactical assignment for a specific bot
 */
struct TacticalAssignment
{
    ObjectGuid targetGuid;          ///< Target of the assignment
    std::string taskType;           ///< "interrupt", "dispel", "focus", "cc", "taunt", etc.
    uint32 priority;                ///< Assignment priority (0-100, higher = more important)
    uint32 assignedTime;            ///< Game time when assigned (getMSTime)
    uint32 expirationTime;          ///< Game time when assignment expires
    ObjectGuid assignedBot;         ///< Bot this assignment belongs to

    TacticalAssignment()
        : priority(0), assignedTime(0), expirationTime(0) {}

    /**
     * @brief Check if assignment has expired
     */
    bool IsExpired(uint32 currentTime) const
    {
        return expirationTime > 0 && currentTime >= expirationTime;
    }

    /**
     * @brief Check if assignment is still valid
     */
    bool IsValid(uint32 currentTime) const
    {
        return !targetGuid.IsEmpty() && !IsExpired(currentTime);
    }
};

/**
 * @brief Group tactical state shared among all members
 */
struct GroupTacticalState
{
    // Target coordination
    ObjectGuid focusTarget;                                     ///< Current focus target for DPS
    std::vector<ObjectGuid> priorityTargets;                    ///< Priority kill order
    std::vector<ObjectGuid> crowdControlTargets;                ///< Targets marked for CC

    // Interrupt coordination
    std::unordered_map<ObjectGuid, uint32> interruptQueue;      ///< Bot GUID → next available interrupt time
    uint32 lastInterruptTime = 0;                               ///< Last time any interrupt was used
    ObjectGuid lastInterrupter;                                 ///< Last bot who interrupted

    // Dispel coordination
    std::unordered_map<ObjectGuid, ObjectGuid> dispelAssignments; ///< Bot GUID → Target GUID needing dispel
    uint32 lastDispelTime = 0;                                  ///< Last time any dispel was used

    // Cooldown coordination
    std::unordered_map<std::string, uint32> groupCooldowns;     ///< Cooldown name → expiration time

    // Combat state
    std::atomic<bool> inCombat{false};                          ///< Is group currently in combat
    uint32 combatStartTime = 0;                                 ///< When combat started (getMSTime)
    uint32 lastUpdateTime = 0;                                  ///< Last tactical update time

    GroupTacticalState() = default;

    // Atomic members need special copy handling
    GroupTacticalState(GroupTacticalState const& other)
        : focusTarget(other.focusTarget)
        , priorityTargets(other.priorityTargets)
        , crowdControlTargets(other.crowdControlTargets)
        , interruptQueue(other.interruptQueue)
        , lastInterruptTime(other.lastInterruptTime)
        , lastInterrupter(other.lastInterrupter)
        , dispelAssignments(other.dispelAssignments)
        , lastDispelTime(other.lastDispelTime)
        , groupCooldowns(other.groupCooldowns)
        , combatStartTime(other.combatStartTime)
        , lastUpdateTime(other.lastUpdateTime)
    {
        inCombat.store(other.inCombat.load());
    }

    GroupTacticalState& operator=(GroupTacticalState const& other)
    {
        if (this != &other)
        {
            focusTarget = other.focusTarget;
            priorityTargets = other.priorityTargets;
            crowdControlTargets = other.crowdControlTargets;
            interruptQueue = other.interruptQueue;
            lastInterruptTime = other.lastInterruptTime;
            lastInterrupter = other.lastInterrupter;
            dispelAssignments = other.dispelAssignments;
            lastDispelTime = other.lastDispelTime;
            groupCooldowns = other.groupCooldowns;
            inCombat.store(other.inCombat.load());
            combatStartTime = other.combatStartTime;
            lastUpdateTime = other.lastUpdateTime;
        }
        return *this;
    }
};

/**
 * @class TacticalCoordinator
 * @brief Handles combat tactical coordination for a group
 *
 * ARCHITECTURE:
 * - Subsystem of GroupCoordinator (composition pattern)
 * - Thread-safe for concurrent access from multiple bots
 * - Performance-optimized for minimal CPU usage (<1ms updates)
 *
 * USAGE PATTERN:
 * ```cpp
 * // In GroupCoordinator:
 * auto tactical = std::make_shared<TacticalCoordinator>(group);
 *
 * // Bots query for assignments:
 * if (auto assignment = tactical->GetAssignment(botGuid))
 * {
 *     if (assignment->taskType == "interrupt")
 *         PerformInterrupt(assignment->targetGuid);
 * }
 *
 * // Request interrupt assignment:
 * if (auto botGuid = tactical->AssignInterrupt(enemyGuid))
 *     // Bot with botGuid should interrupt enemyGuid
 * ```
 *
 * THREAD SAFETY:
 * - All public methods are thread-safe
 * - Uses lock hierarchy to prevent deadlocks
 * - Atomic operations for performance-critical queries
 *
 * PERFORMANCE:
 * - Update(): <1ms for 40-player raid groups
 * - Query methods: <0.001ms (lock-free where possible)
 * - Memory: ~8KB per 40-player group
 */
class TC_GAME_API TacticalCoordinator
{
public:
    /**
     * @brief Construct tactical coordinator for a group
     * @param group The group to coordinate (must not be null)
     */
    explicit TacticalCoordinator(Group* group);

    /**
     * @brief Destructor
     */
    ~TacticalCoordinator();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize the tactical coordinator
     */
    void Initialize();

    /**
     * @brief Update tactical state (called every 100-200ms)
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Reset tactical state (e.g., when combat ends)
     */
    void Reset();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    // ========================================================================
    // FOCUS TARGET COORDINATION
    // ========================================================================

    /**
     * @brief Get current focus target for DPS
     * @return GUID of focus target, or empty if none
     *
     * Thread-safe: Uses atomic operations
     * Performance: <0.001ms
     */
    ObjectGuid GetFocusTarget() const;

    /**
     * @brief Set focus target for group
     * @param targetGuid New focus target GUID
     *
     * Thread-safe: Uses locking
     * Updates all group members to focus this target
     */
    void SetFocusTarget(ObjectGuid targetGuid);

    /**
     * @brief Get priority target list (kill order)
     * @return Vector of target GUIDs in kill order
     *
     * Thread-safe: Returns copy
     */
    std::vector<ObjectGuid> GetPriorityTargets() const;

    /**
     * @brief Add priority target to kill list
     * @param targetGuid Target to add
     * @param priority Priority level (higher = kill sooner)
     */
    void AddPriorityTarget(ObjectGuid targetGuid, uint32 priority = 50);

    // ========================================================================
    // INTERRUPT COORDINATION
    // ========================================================================

    /**
     * @brief Assign interrupt to next available bot
     * @param targetGuid Target that needs interrupting
     * @return GUID of bot assigned to interrupt, or empty if none available
     *
     * Thread-safe: Uses locking
     *
     * LOGIC:
     * - Checks interrupt rotation queue
     * - Assigns to bot with interrupt off cooldown
     * - Updates last interrupt time to prevent overlap
     */
    ObjectGuid AssignInterrupt(ObjectGuid targetGuid);

    /**
     * @brief Register bot as having interrupt capability
     * @param botGuid Bot's GUID
     * @param cooldownMs Interrupt cooldown in milliseconds
     *
     * Called by bot during initialization to register interrupt capability
     */
    void RegisterInterrupter(ObjectGuid botGuid, uint32 cooldownMs);

    /**
     * @brief Report interrupt used by bot
     * @param botGuid Bot who used interrupt
     * @param cooldownMs Cooldown duration
     *
     * Updates rotation queue so next bot can be assigned
     */
    void ReportInterruptUsed(ObjectGuid botGuid, uint32 cooldownMs);

    /**
     * @brief Check if bot should interrupt next
     * @param botGuid Bot to check
     * @return True if this bot is next in rotation
     */
    bool IsNextInterrupter(ObjectGuid botGuid) const;

    // ========================================================================
    // DISPEL COORDINATION
    // ========================================================================

    /**
     * @brief Assign dispel to appropriate healer
     * @param targetGuid Target needing dispel
     * @return GUID of healer assigned to dispel, or empty if none available
     *
     * Thread-safe: Uses locking
     *
     * LOGIC:
     * - Prefers healers over other classes
     * - Checks dispel capability (magic, curse, poison, disease)
     * - Avoids double-assignment of same target
     */
    ObjectGuid AssignDispel(ObjectGuid targetGuid);

    /**
     * @brief Get current dispel assignment for bot
     * @param botGuid Bot's GUID
     * @return Target GUID to dispel, or empty if no assignment
     */
    ObjectGuid GetDispelAssignment(ObjectGuid botGuid) const;

    /**
     * @brief Report dispel completed by bot
     * @param botGuid Bot who dispelled
     * @param targetGuid Target that was dispelled
     *
     * Clears assignment so target won't be dispelled again
     */
    void ReportDispelCompleted(ObjectGuid botGuid, ObjectGuid targetGuid);

    // ========================================================================
    // CROWD CONTROL COORDINATION
    // ========================================================================

    /**
     * @brief Mark target for crowd control
     * @param targetGuid Target to CC
     * @param assignedBot Bot assigned to CC this target
     */
    void AssignCrowdControl(ObjectGuid targetGuid, ObjectGuid assignedBot);

    /**
     * @brief Get targets marked for crowd control
     * @return Vector of CC target GUIDs
     */
    std::vector<ObjectGuid> GetCrowdControlTargets() const;

    /**
     * @brief Check if target is currently being CC'd
     * @param targetGuid Target to check
     * @return True if target is marked for CC
     */
    bool IsTargetCrowdControlled(ObjectGuid targetGuid) const;

    // ========================================================================
    // GROUP COOLDOWN COORDINATION
    // ========================================================================

    /**
     * @brief Check if group cooldown is available
     * @param cooldownName Cooldown identifier (e.g., "Bloodlust", "Innervate")
     * @return True if cooldown is off cooldown
     *
     * Thread-safe: Uses locking
     * Performance: <0.01ms
     *
     * Use this to coordinate important group cooldowns to avoid overlap
     */
    bool IsGroupCooldownAvailable(std::string const& cooldownName) const;

    /**
     * @brief Use group cooldown
     * @param cooldownName Cooldown identifier
     * @param durationMs Cooldown duration in milliseconds
     *
     * Thread-safe: Uses locking
     *
     * Marks cooldown as used so other bots won't use it simultaneously
     */
    void UseGroupCooldown(std::string const& cooldownName, uint32 durationMs);

    /**
     * @brief Get remaining time on group cooldown
     * @param cooldownName Cooldown identifier
     * @return Remaining milliseconds, or 0 if available
     */
    uint32 GetGroupCooldownRemaining(std::string const& cooldownName) const;

    // ========================================================================
    // TACTICAL ASSIGNMENTS
    // ========================================================================

    /**
     * @brief Get current tactical assignment for bot
     * @param botGuid Bot's GUID
     * @return Pointer to assignment, or nullptr if no assignment
     *
     * Thread-safe: Returns const pointer
     *
     * Assignment types:
     * - "interrupt": Interrupt specific target
     * - "dispel": Dispel specific target
     * - "focus": Focus fire on target
     * - "cc": Crowd control target
     * - "taunt": Taunt target off someone
     */
    TacticalAssignment const* GetAssignment(ObjectGuid botGuid) const;

    /**
     * @brief Clear assignment for bot
     * @param botGuid Bot whose assignment to clear
     */
    void ClearAssignment(ObjectGuid botGuid);

    /**
     * @brief Clear all assignments
     *
     * Called when combat ends or group wipes
     */
    void ClearAllAssignments();

    // ========================================================================
    // COMBAT STATE
    // ========================================================================

    /**
     * @brief Check if group is in combat
     * @return True if any group member is in combat
     *
     * Thread-safe: Atomic read
     * Performance: <0.001ms
     */
    bool IsInCombat() const { return m_tacticalState.inCombat.load(std::memory_order_acquire); }

    /**
     * @brief Get time since combat started
     * @return Milliseconds since combat started, or 0 if not in combat
     */
    uint32 GetCombatDuration() const;

    /**
     * @brief Set combat state
     * @param inCombat True if entering combat, false if leaving
     *
     * Thread-safe: Atomic write
     */
    void SetCombatState(bool inCombat);

    // ========================================================================
    // TACTICAL STATE ACCESS
    // ========================================================================

    /**
     * @brief Get full tactical state (read-only)
     * @return Const reference to tactical state
     *
     * Thread-safe: Uses locking
     * Use sparingly - prefer specific query methods for better performance
     */
    GroupTacticalState const& GetTacticalState() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set update interval
     * @param intervalMs Update interval in milliseconds (default: 200ms)
     *
     * Lower values = more responsive but higher CPU usage
     * Recommended: 100ms for dungeons, 200ms for raids
     */
    void SetUpdateInterval(uint32 intervalMs) { m_updateInterval = intervalMs; }

    /**
     * @brief Get update interval
     */
    uint32 GetUpdateInterval() const { return m_updateInterval; }

    // ========================================================================
    // DEPENDENCY INJECTION - Single Authority Delegation
    // ========================================================================

    /**
     * @brief Set interrupt coordinator (single authority for interrupts)
     * @param ic InterruptCoordinator instance to delegate to
     *
     * Phase 2 Architecture: All interrupt coordination delegates to InterruptCoordinator
     */
    void SetInterruptCoordinator(InterruptCoordinator* ic) { _interruptCoordinator = ic; }

    /**
     * @brief Get interrupt coordinator
     */
    InterruptCoordinator* GetInterruptCoordinator() const { return _interruptCoordinator; }

    /**
     * @brief Set CC manager (single authority for crowd control)
     * Phase 2 Architecture: All CC coordination delegates to CrowdControlManager
     */
    void SetCCManager(CrowdControlManager* ccm) { _ccManager = ccm; }

    /**
     * @brief Get CC manager
     */
    CrowdControlManager* GetCCManager() const { return _ccManager; }

    // ========================================================================
    // STATISTICS & MONITORING
    // ========================================================================

    struct Statistics
    {
        std::atomic<uint32> totalUpdates{0};
        std::atomic<uint32> interruptsAssigned{0};
        std::atomic<uint32> dispelsAssigned{0};
        std::atomic<uint32> focusTargetChanges{0};
        std::atomic<uint32> cooldownsUsed{0};
        std::atomic<uint64> totalUpdateTimeUs{0};  ///< Total update time in microseconds

        Statistics() = default;

        Statistics(Statistics const& other)
        {
            totalUpdates.store(other.totalUpdates.load());
            interruptsAssigned.store(other.interruptsAssigned.load());
            dispelsAssigned.store(other.dispelsAssigned.load());
            focusTargetChanges.store(other.focusTargetChanges.load());
            cooldownsUsed.store(other.cooldownsUsed.load());
            totalUpdateTimeUs.store(other.totalUpdateTimeUs.load());
        }

        Statistics& operator=(Statistics const& other)
        {
            if (this != &other)
            {
                totalUpdates.store(other.totalUpdates.load());
                interruptsAssigned.store(other.interruptsAssigned.load());
                dispelsAssigned.store(other.dispelsAssigned.load());
                focusTargetChanges.store(other.focusTargetChanges.load());
                cooldownsUsed.store(other.cooldownsUsed.load());
                totalUpdateTimeUs.store(other.totalUpdateTimeUs.load());
            }
            return *this;
        }

        void Reset()
        {
            totalUpdates = 0;
            interruptsAssigned = 0;
            dispelsAssigned = 0;
            focusTargetChanges = 0;
            cooldownsUsed = 0;
            totalUpdateTimeUs = 0;
        }

        uint64 GetAverageUpdateTimeUs() const
        {
            uint32 updates = totalUpdates.load();
            return updates > 0 ? totalUpdateTimeUs.load() / updates : 0;
        }
    };

    /**
     * @brief Get statistics
     */
    Statistics GetStatistics() const { return m_statistics; }

    /**
     * @brief Reset statistics
     */
    void ResetStatistics() { m_statistics.Reset(); }

private:
    // ========================================================================
    // INTERNAL UPDATE METHODS
    // ========================================================================

    /**
     * @brief Update focus target based on threat and priority
     */
    void UpdateFocusTarget();

    /**
     * @brief Update interrupt rotation queue
     */
    void UpdateInterruptRotation();

    /**
     * @brief Update dispel assignments
     */
    void UpdateDispelAssignments();

    /**
     * @brief Clean up expired assignments and cooldowns
     */
    void CleanupExpiredData(uint32 currentTime);

    /**
     * @brief Find best focus target from current enemies
     * @return GUID of best target, or empty if none found
     */
    ObjectGuid FindBestFocusTarget() const;

    /**
     * @brief Get next bot in interrupt rotation
     * @return GUID of bot with interrupt ready, or empty if none
     */
    ObjectGuid GetNextInterrupter() const;

    /**
     * @brief Find best dispeller for target
     * @param targetGuid Target needing dispel
     * @return GUID of best dispeller, or empty if none found
     */
    ObjectGuid FindBestDispeller(ObjectGuid targetGuid) const;

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Group* m_group;                                             ///< Group being coordinated
    GroupTacticalState m_tacticalState;                         ///< Shared tactical state

    // Phase 2 Architecture: Delegate to single authorities
    InterruptCoordinator* _interruptCoordinator = nullptr;      ///< Single authority for interrupts
    CrowdControlManager* _ccManager = nullptr;                   ///< Single authority for crowd control

    // Assignments
    std::unordered_map<ObjectGuid, TacticalAssignment> m_assignments; ///< Bot GUID → Assignment

    // Update timing
    uint32 m_lastUpdateTime;                                    ///< Last update time (getMSTime)
    uint32 m_updateInterval;                                    ///< Update interval in ms

    // Statistics
    Statistics m_statistics;                                    ///< Performance statistics

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI> m_mutex;
};

} // namespace Advanced
} // namespace Playerbot

#endif // TRINITYCORE_TACTICAL_COORDINATOR_H
