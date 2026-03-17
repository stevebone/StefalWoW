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

#ifndef TRINITYCORE_ROLE_COORDINATOR_H
#define TRINITYCORE_ROLE_COORDINATOR_H

#include "../../Advanced/GroupCoordinator.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace Playerbot
{

// Forward declaration for single authority delegation
// Note: InterruptCoordinator is a type alias for InterruptCoordinatorFixed
class InterruptCoordinatorFixed;
using InterruptCoordinator = InterruptCoordinatorFixed;

namespace Coordination
{

// Import GroupRole enum from Advanced namespace
using Advanced::GroupCoordinator;

/**
 * @brief Cached role query results to avoid O(N) iteration every frame
 * Refresh interval ensures data stays current without expensive per-frame recalculation
 */
struct RoleCache
{
    ::std::vector<ObjectGuid> tanks;
    ::std::vector<ObjectGuid> healers;
    ::std::vector<ObjectGuid> meleeDPS;
    ::std::vector<ObjectGuid> rangedDPS;
    uint32 lastRefreshTime = 0;
    bool dirty = true;

    static constexpr uint32 REFRESH_INTERVAL = 1000; // 1 second

    void MarkDirty() { dirty = true; }
    bool NeedsRefresh(uint32 currentTime) const
    {
        return dirty || (currentTime - lastRefreshTime) > REFRESH_INTERVAL;
    }
};

/**
 * @brief Base class for role-specific coordination
 * Each role (Tank, Healer, DPS) has unique coordination needs
 */
class TC_GAME_API RoleCoordinator
{
public:
    virtual ~RoleCoordinator() = default;

    /**
     * @brief Update role-specific coordination
     * @param group Group coordinator
     * @param diff Time since last update
     */
    virtual void Update(Advanced::GroupCoordinator* group, uint32 diff) = 0;

    /**
     * @brief Get role type
     */
    virtual GroupCoordinator::GroupCoordinator::GroupRole GetRole() const = 0;

protected:
    uint32 _lastUpdateTime = 0;
    uint32 _updateInterval = 200; // 200ms default
};

/**
 * @brief Tank Coordinator
 *
 * Responsibilities:
 * - Main tank designation
 * - Off-tank assignments
 * - Taunt rotation
 * - Threat management
 * - Tank swap coordination
 */
class TC_GAME_API TankCoordinator : public RoleCoordinator
{
public:
    void Update(GroupCoordinator* group, uint32 diff);
    void UpdateWithCache(GroupCoordinator* group, uint32 diff, RoleCache const& cache);
    GroupCoordinator::GroupRole GetRole() const override { return GroupCoordinator::GroupRole::TANK; }

    /**
     * @brief Get main tank GUID
     */
    ObjectGuid GetMainTank() const { return _mainTank; }

    /**
     * @brief Get off-tank GUID
     */
    ObjectGuid GetOffTank() const { return _offTank; }

    /**
     * @brief Get tank assigned to target
     * @param targetGuid Target GUID
     * @return Tank GUID or empty
     */
    ObjectGuid GetTankForTarget(ObjectGuid targetGuid) const;

    /**
     * @brief Request tank swap
     * Coordinates taunt between main and off-tank
     */
    void RequestTankSwap();

    /**
     * @brief Check if tank swap is needed
     * @param mainTankGuid Main tank GUID
     * @return True if swap recommended
     */
    bool NeedsTankSwap(ObjectGuid mainTankGuid) const;

private:
    void UpdateMainTank(GroupCoordinator* group, RoleCache const& cache);
    void UpdateTankAssignments(GroupCoordinator* group);
    void UpdateTauntRotation(GroupCoordinator* group);

    ObjectGuid _mainTank;
    ObjectGuid _offTank;
    ::std::unordered_map<ObjectGuid, ObjectGuid> _tankAssignments; // Target → Tank
    uint32 _lastTankSwapTime = 0;
    uint32 _tankSwapCooldown = 30000; // 30s between swaps
};

/**
 * @brief Healer Coordinator
 *
 * Responsibilities:
 * - Healing assignments (tanks, raid, group)
 * - Dispel assignments
 * - Cooldown rotation (Auras, Tranquility, etc.)
 * - Mana management coordination
 * - Resurrection priority
 */
class TC_GAME_API HealerCoordinator : public RoleCoordinator
{
public:
    void Update(GroupCoordinator* group, uint32 diff);
    void UpdateWithCache(GroupCoordinator* group, uint32 diff, RoleCache const& cache);
    GroupCoordinator::GroupRole GetRole() const override { return GroupCoordinator::GroupRole::HEALER; }

    /**
     * @brief Get healer assigned to tank
     * @param tankGuid Tank GUID
     * @return Healer GUID or empty
     */
    ObjectGuid GetHealerForTank(ObjectGuid tankGuid) const;

    /**
     * @brief Assign healer to tank
     * @param healerGuid Healer GUID
     * @param tankGuid Tank GUID
     */
    void AssignHealerToTank(ObjectGuid healerGuid, ObjectGuid tankGuid);

    /**
     * @brief Get next healer for cooldown rotation
     * @param cooldownType Cooldown type (e.g., "raid_heal")
     * @return Healer GUID or empty
     */
    ObjectGuid GetNextCooldownHealer(::std::string const& cooldownType) const;

    /**
     * @brief Use healing cooldown
     * @param healerGuid Healer using cooldown
     * @param cooldownType Cooldown type
     * @param durationMs Duration in milliseconds
     */
    void UseHealingCooldown(ObjectGuid healerGuid, ::std::string const& cooldownType, uint32 durationMs);

    /**
     * @brief Get resurrection priority list
     * @return Ordered list of dead players by priority
     */
    ::std::vector<ObjectGuid> GetResurrectionPriority() const;

private:
    void UpdateHealingAssignments(GroupCoordinator* group, RoleCache const& cache);
    void UpdateDispelCoordination(GroupCoordinator* group);
    void UpdateCooldownRotation(GroupCoordinator* group, RoleCache const& cache);
    void UpdateManaManagement(GroupCoordinator* group, RoleCache const& cache);

    struct HealingAssignment
    {
        ObjectGuid healerGuid;
        ObjectGuid targetGuid;
        ::std::string assignmentType; // "tank", "group", "raid"
        uint32 priority;
    };

    // O(1) lookup maps instead of O(N) vector searches
    ::std::unordered_map<ObjectGuid, HealingAssignment> _healerToAssignment; // Healer → Assignment
    ::std::unordered_map<ObjectGuid, ObjectGuid> _tankToHealer; // Tank → Healer (for GetHealerForTank O(1))

    // Legacy vector kept for iteration needs
    ::std::vector<HealingAssignment> _healingAssignments;
    ::std::unordered_map<ObjectGuid, ::std::unordered_map<::std::string, uint32>> _healerCooldowns; // Healer → Cooldown → ExpireTime

    // Mana conservation state
    bool _manaConservationMode = false;
    uint32 _manaConservationStartTime = 0;
    ::std::unordered_map<ObjectGuid, bool> _healerManaConservation; // Track per-healer conservation state

public:
    /**
     * @brief Check if healers should conserve mana
     * @return True if in mana conservation mode
     */
    bool IsInManaConservationMode() const { return _manaConservationMode; }

    /**
     * @brief Check if specific healer should conserve mana
     * @param healerGuid Healer to check
     * @return True if this healer should use mana-efficient spells
     */
    bool ShouldConserveMana(ObjectGuid healerGuid) const;
};

/**
 * @brief DPS Coordinator
 *
 * Responsibilities:
 * - Focus fire coordination
 * - Interrupt rotation
 * - Crowd control assignments
 * - Burst window coordination
 * - Add management
 */
class TC_GAME_API DPSCoordinator : public RoleCoordinator
{
public:
    void Update(GroupCoordinator* group, uint32 diff);
    void UpdateWithCache(GroupCoordinator* group, uint32 diff, RoleCache const& cache);
    GroupCoordinator::GroupRole GetRole() const override { return GroupCoordinator::GroupRole::DPS_MELEE; } // Handles both melee and ranged

    // ========================================================================
    // DEPENDENCY INJECTION - Single Authority Delegation
    // ========================================================================

    /**
     * @brief Set interrupt coordinator (single authority for interrupts)
     * Phase 2 Architecture: All interrupt coordination delegates to InterruptCoordinator
     */
    void SetInterruptCoordinator(InterruptCoordinator* ic) { _interruptCoordinator = ic; }

    /**
     * @brief Get focus target for DPS
     */
    ObjectGuid GetFocusTarget() const { return _focusTarget; }

    /**
     * @brief Set focus target
     */
    void SetFocusTarget(ObjectGuid targetGuid);

    /**
     * @brief Get next interrupter
     * @return DPS GUID with interrupt ready
     * Phase 2: Delegates to InterruptCoordinator
     */
    ObjectGuid GetNextInterrupter() const;

    /**
     * @brief Assign interrupt to DPS
     * @param dpsGuid DPS GUID
     * @param targetGuid Target GUID
     * Phase 2: Delegates to InterruptCoordinator
     */
    void AssignInterrupt(ObjectGuid dpsGuid, ObjectGuid targetGuid);

    /**
     * @brief Get CC assignment for DPS
     * @param dpsGuid DPS GUID
     * @return Target GUID or empty
     */
    ObjectGuid GetCCAssignment(ObjectGuid dpsGuid) const;

    /**
     * @brief Assign crowd control
     * @param dpsGuid DPS GUID
     * @param targetGuid Target GUID
     * @param ccType CC type (e.g., "polymorph", "trap", "hex")
     */
    void AssignCC(ObjectGuid dpsGuid, ObjectGuid targetGuid, ::std::string const& ccType);

    /**
     * @brief Request burst window
     * Coordinates offensive cooldowns for maximum burst
     * @param durationMs Burst window duration
     */
    void RequestBurstWindow(uint32 durationMs);

    /**
     * @brief Check if in burst window
     */
    bool InBurstWindow() const;

private:
    void UpdateFocusTarget(GroupCoordinator* group, RoleCache const& cache);
    void UpdateInterruptRotation(GroupCoordinator* group, RoleCache const& cache);
    void UpdateCCAssignments(GroupCoordinator* group);
    void UpdateBurstWindows(GroupCoordinator* group);

    struct InterruptAssignment
    {
        ObjectGuid dpsGuid;
        ObjectGuid targetGuid;
        uint32 assignedTime;
        uint32 cooldownExpire;
    };

    struct CCAssignment
    {
        ObjectGuid dpsGuid;
        ObjectGuid targetGuid;
        ::std::string ccType;
        uint32 assignedTime;
    };

    ObjectGuid _focusTarget;

    // Phase 2 Architecture: Delegate to single authority
    InterruptCoordinator* _interruptCoordinator = nullptr;

    // Legacy: Only used when _interruptCoordinator is null
    ::std::vector<InterruptAssignment> _interruptRotation;
    ::std::vector<CCAssignment> _ccAssignments;

    // Burst window coordination
    bool _inBurstWindow = false;
    uint32 _burstWindowStart = 0;
    uint32 _burstWindowDuration = 0;
};

/**
 * @brief Role Coordinator Manager
 * Manages all role-specific coordinators for a group
 *
 * Performance: Uses shared RoleCache to avoid O(N) group iteration per coordinator
 */
class TC_GAME_API RoleCoordinatorManager
{
public:
    RoleCoordinatorManager();
    ~RoleCoordinatorManager() = default;

    /**
     * @brief Update all role coordinators
     * @param group Group coordinator
     * @param diff Time since last update
     */
    void Update(GroupCoordinator* group, uint32 diff);

    /**
     * @brief Get tank coordinator
     */
    TankCoordinator* GetTankCoordinator() { return _tankCoordinator.get(); }

    /**
     * @brief Get healer coordinator
     */
    HealerCoordinator* GetHealerCoordinator() { return _healerCoordinator.get(); }

    /**
     * @brief Get DPS coordinator
     */
    DPSCoordinator* GetDPSCoordinator() { return _dpsCoordinator.get(); }

    /**
     * @brief Get cached role data (refreshed periodically)
     */
    RoleCache const& GetRoleCache() const { return _roleCache; }

    /**
     * @brief Invalidate role cache (call when group composition changes)
     */
    void InvalidateCache() { _roleCache.MarkDirty(); }

private:
    void RefreshRoleCache(GroupCoordinator* group);

    ::std::unique_ptr<TankCoordinator> _tankCoordinator;
    ::std::unique_ptr<HealerCoordinator> _healerCoordinator;
    ::std::unique_ptr<DPSCoordinator> _dpsCoordinator;

    // Shared cache for all coordinators - refreshed once per Update cycle
    RoleCache _roleCache;
};

} // namespace Coordination
} // namespace Playerbot

#endif // TRINITYCORE_ROLE_COORDINATOR_H
