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

#ifndef TRINITYCORE_RAID_ORCHESTRATOR_H
#define TRINITYCORE_RAID_ORCHESTRATOR_H

#include "RoleCoordinator.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

// Forward declarations
namespace Playerbot { namespace Advanced { class GroupCoordinator; } }

// Forward declare GroupCoordinator as the concrete type (replacing IGroupCoordinator interface)
using GroupCoordinatorPtr = ::Playerbot::Advanced::GroupCoordinator;

class Group;

namespace Playerbot
{
namespace Coordination
{

/**
 * @brief Raid-wide formation positions
 */
enum class RaidFormation : uint8
{
    SPREAD,         // Spread out (AoE avoidance)
    STACKED,        // Stack together (healing efficiency)
    RANGED_SPLIT,   // Ranged split into groups
    MELEE_HEAVY,    // Melee close, ranged far
    DEFENSIVE       // Tanks front, healers back, DPS middle
};

/**
 * @brief Boss encounter phases
 */
enum class EncounterPhase : uint8
{
    NORMAL,         // Standard combat
    BURN,           // Burn/execute phase
    ADD_PHASE,      // Add management phase
    TRANSITION,     // Phase transition
    INTERMISSION,   // Boss intermission
    DEFENSIVE       // Defensive phase (survival focus)
};

/**
 * @brief Raid-level tactical directive
 */
struct RaidDirective
{
    ::std::string directiveType;      // "focus_adds", "lust", "defensive_cd", "spread", "stack"
    uint32 priority;                // 0-100
    uint32 timestamp;
    uint32 duration;                // How long directive is active (ms)
    ::std::unordered_map<::std::string, float> parameters; // Type-specific parameters

    bool IsActive() const;
};

/**
 * @brief Raid Orchestrator
 * Coordinates 40-player raids with hierarchical group management
 *
 * Responsibilities:
 * - Manages up to 8 groups (5 players each)
 * - Coordinates raid-wide tactics (bloodlust, defensive CDs)
 * - Boss encounter management
 * - Formation control
 * - Add priority targeting
 * - Raid-wide cooldown rotation
 */
class TC_GAME_API RaidOrchestrator
{
public:
    RaidOrchestrator(Group* raid);
    ~RaidOrchestrator() = default;

    /**
     * @brief Update raid coordination
     * @param diff Time since last update
     */
    void Update(uint32 diff);

    /**
     * @brief Get group count
     */
    uint32 GetGroupCount() const { return static_cast<uint32>(_groupCoordinators.size()); }

    /**
     * @brief Get group coordinator by index
     * @param groupIndex Group index (0-7)
     * @return Pointer to group coordinator, nullptr if invalid index
     */
    GroupCoordinatorPtr* GetGroupCoordinator(uint32 groupIndex);

    /**
     * @brief Get role coordinator manager
     */
    RoleCoordinatorManager* GetRoleCoordinatorManager() { return &_roleCoordinatorManager; }

    /**
     * @brief Issue raid-wide directive
     * @param directive Directive to issue
     */
    void IssueDirective(RaidDirective const& directive);

    /**
     * @brief Get active directives
     */
    ::std::vector<RaidDirective> GetActiveDirectives() const;

    /**
     * @brief Set raid formation
     * @param formation Formation type
     */
    void SetFormation(RaidFormation formation);

    /**
     * @brief Get current formation
     */
    RaidFormation GetFormation() const { return _currentFormation; }

    /**
     * @brief Set encounter phase
     * @param phase Encounter phase
     */
    void SetEncounterPhase(EncounterPhase phase);

    /**
     * @brief Get current encounter phase
     */
    EncounterPhase GetEncounterPhase() const { return _currentPhase; }

    /**
     * @brief Request raid-wide bloodlust/heroism
     * @return True if bloodlust was triggered
     */
    bool RequestBloodlust();

    /**
     * @brief Check if bloodlust is active
     */
    bool IsBloodlustActive() const;

    /**
     * @brief Request raid-wide defensive cooldown
     * @param cooldownType Type of defensive (e.g., "barrier", "aura")
     * @return True if cooldown was used
     */
    bool RequestRaidDefensiveCooldown(::std::string const& cooldownType);

    /**
     * @brief Designate add priority targets
     * @param targetGuids Ordered list of add GUIDs (highest to lowest priority)
     */
    void SetAddPriorities(::std::vector<ObjectGuid> const& targetGuids);

    /**
     * @brief Get add priority list
     */
    ::std::vector<ObjectGuid> GetAddPriorities() const { return _addPriorities; }

    /**
     * @brief Get raid-wide statistics
     */
    struct RaidStats
    {
        uint32 totalBots;
        uint32 aliveBots;
        uint32 deadBots;
        float avgHealthPct;
        float avgManaPct;
        uint32 combatDuration;
        uint32 totalDamageDone;
        uint32 totalHealingDone;
    };

    RaidStats GetRaidStats() const;

    /**
     * @brief Check if in combat
     */
    bool IsInCombat() const { return _inCombat; }

    /**
     * @brief Get combat duration
     */
    uint32 GetCombatDuration() const;

private:
    void UpdateGroupCoordinators(uint32 diff);
    void UpdateRoleCoordinators(uint32 diff);
    void UpdateDirectives(uint32 diff);
    void UpdateFormation(uint32 diff);
    void UpdateEncounterPhase(uint32 diff);
    void UpdateCombatState();
    void UpdateRaidStats();

    // Boss encounter detection
    void DetectBossEncounter();
    void HandleEncounterPhaseChange();

    // Cooldown management
    void RotateRaidDefensiveCooldowns();
    void CoordinateBloodlustTiming();

    // Add management
    void UpdateAddPriorities();
    void AssignDPSToAdds();

    Group* _raid;
    ::std::vector<::std::unique_ptr<GroupCoordinatorPtr>> _groupCoordinators;
    RoleCoordinatorManager _roleCoordinatorManager;

    // Raid state
    bool _inCombat = false;
    uint32 _combatStartTime = 0;
    RaidFormation _currentFormation = RaidFormation::DEFENSIVE;
    EncounterPhase _currentPhase = EncounterPhase::NORMAL;

    // Directives
    ::std::vector<RaidDirective> _activeDirectives;

    // Add management
    ::std::vector<ObjectGuid> _addPriorities;

    // Bloodlust/Heroism
    bool _bloodlustActive = false;
    uint32 _bloodlustTime = 0;
    uint32 _bloodlustCooldown = 600000; // 10 minutes

    // Raid-wide cooldowns
    ::std::unordered_map<::std::string, uint32> _raidCooldowns; // Type → Expire time

    // Statistics
    RaidStats _cachedStats;
    uint32 _lastStatsUpdate = 0;

    // Performance
    uint32 _lastUpdateTime = 0;
    uint32 _updateInterval = 500; // 500ms update interval
};

/**
 * @brief Boss Encounter Strategy
 * Defines tactics for specific boss encounters
 */
class TC_GAME_API BossEncounterStrategy
{
public:
    virtual ~BossEncounterStrategy() = default;

    /**
     * @brief Get boss entry ID
     */
    virtual uint32 GetBossEntry() const = 0;

    /**
     * @brief Execute strategy for current phase
     * @param orchestrator Raid orchestrator
     * @param phase Current encounter phase
     */
    virtual void Execute(RaidOrchestrator* orchestrator, EncounterPhase phase) = 0;

    /**
     * @brief Detect phase transitions
     * @param bossHealthPct Boss health percentage
     * @return New phase or current phase
     */
    virtual EncounterPhase DetectPhase(float bossHealthPct) const;
};

/**
 * @brief Boss Encounter Strategy Registry
 * Manages boss-specific strategies
 */
class TC_GAME_API BossStrategyRegistry
{
public:
    /**
     * @brief Register boss strategy
     * @param bossEntry Boss creature entry ID
     * @param strategy Strategy implementation
     */
    static void RegisterStrategy(uint32 bossEntry, ::std::shared_ptr<BossEncounterStrategy> strategy);

    /**
     * @brief Get strategy for boss
     * @param bossEntry Boss creature entry ID
     * @return Strategy or nullptr if not found
     */
    static ::std::shared_ptr<BossEncounterStrategy> GetStrategy(uint32 bossEntry);

    /**
     * @brief Clear all strategies
     */
    static void Clear();

private:
    static ::std::unordered_map<uint32, ::std::shared_ptr<BossEncounterStrategy>> _strategies;
};

/**
 * @brief Example boss strategy: Onyxia
 */
class TC_GAME_API OnyxiaStrategy : public BossEncounterStrategy
{
public:
    uint32 GetBossEntry() const override { return 10184; } // Onyxia

    void Execute(RaidOrchestrator* orchestrator, EncounterPhase phase);

    EncounterPhase DetectPhase(float bossHealthPct) const;
};

} // namespace Coordination
} // namespace Playerbot

#endif // TRINITYCORE_RAID_ORCHESTRATOR_H
