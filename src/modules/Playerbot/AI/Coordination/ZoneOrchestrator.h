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

#ifndef TRINITYCORE_ZONE_ORCHESTRATOR_H
#define TRINITYCORE_ZONE_ORCHESTRATOR_H

#include "RaidOrchestrator.h"
#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

class Player;

namespace Playerbot
{
namespace Coordination
{

/**
 * @brief Zone-wide activity type
 */
enum class ZoneActivity : uint8
{
    WORLD_BOSS,         // World boss engagement
    ZONE_EVENT,         // Zone-wide event (Invasions, etc.)
    CITY_RAID,          // PvP city raid
    RESOURCE_FARMING,   // Coordinated farming
    QUESTING,           // Zone questing coordination
    IDLE                // No specific activity
};

/**
 * @brief Zone threat level
 */
enum class ThreatLevel : uint8
{
    PEACEFUL,   // No threats
    LOW,        // Minor threats
    MODERATE,   // Some hostile NPCs
    HIGH,       // Dangerous zone
    CRITICAL    // Extreme danger (world bosses, raids)
};

/**
 * @brief Zone-wide objective
 */
struct ZoneObjective
{
    ::std::string objectiveType;      // "kill_boss", "defend_npc", "gather_resources"
    uint32 priority;                // 0-100
    uint32 assignedBots;            // Number of bots assigned
    uint32 requiredBots;            // Number of bots needed
    Position targetPosition;        // Objective location
    ObjectGuid targetGuid;          // Target entity (if applicable)
    uint32 timestamp;
    uint32 expirationTime;

    bool IsActive() const;
    bool IsComplete() const;
};

/**
 * @brief Zone Orchestrator
 * Coordinates 100-500 bots across zones with multiple raid groups
 *
 * Responsibilities:
 * - Manages multiple raid groups (up to 12 raids = 480 players)
 * - Zone-wide threat assessment
 * - Objective coordination (world bosses, events)
 * - Bot distribution and load balancing
 * - Cross-raid coordination
 * - Zone-level resource management
 */
class TC_GAME_API ZoneOrchestrator
{
public:
    ZoneOrchestrator(uint32 zoneId);
    ~ZoneOrchestrator() = default;

    /**
     * @brief Update zone coordination
     * @param diff Time since last update
     */
    void Update(uint32 diff);

    /**
     * @brief Get zone ID
     */
    uint32 GetZoneId() const { return _zoneId; }

    /**
     * @brief Register bot in zone
     * @param bot Player bot
     */
    void RegisterBot(Player* bot);

    /**
     * @brief Unregister bot from zone
     * @param botGuid Bot GUID
     */
    void UnregisterBot(ObjectGuid botGuid);

    /**
     * @brief Get bot count
     */
    uint32 GetBotCount() const { return static_cast<uint32>(_bots.size()); }

    /**
     * @brief Get all bots
     */
    ::std::vector<ObjectGuid> const& GetBots() const { return _bots; }

    /**
     * @brief Add raid orchestrator
     * @param raid Raid to add
     */
    void AddRaid(::std::unique_ptr<RaidOrchestrator> raid);

    /**
     * @brief Get raid count
     */
    uint32 GetRaidCount() const { return static_cast<uint32>(_raids.size()); }

    /**
     * @brief Get raid orchestrator by index
     * @param raidIndex Raid index
     */
    RaidOrchestrator* GetRaid(uint32 raidIndex);

    /**
     * @brief Set zone activity
     * @param activity Activity type
     */
    void SetActivity(ZoneActivity activity);

    /**
     * @brief Get current activity
     */
    ZoneActivity GetActivity() const { return _currentActivity; }

    /**
     * @brief Set threat level
     * @param level Threat level
     */
    void SetThreatLevel(ThreatLevel level);

    /**
     * @brief Get threat level
     */
    ThreatLevel GetThreatLevel() const { return _threatLevel; }

    /**
     * @brief Create zone objective
     * @param objective Objective to create
     */
    void CreateObjective(ZoneObjective const& objective);

    /**
     * @brief Get active objectives
     */
    ::std::vector<ZoneObjective> GetActiveObjectives() const;

    /**
     * @brief Complete objective
     * @param objectiveType Objective type to complete
     */
    void CompleteObjective(::std::string const& objectiveType);

    /**
     * @brief Assign bots to objective
     * @param objectiveType Objective type
     * @param botCount Number of bots to assign
     * @return Number of bots actually assigned
     */
    uint32 AssignBotsToObjective(::std::string const& objectiveType, uint32 botCount);

    /**
     * @brief Broadcast zone-wide message
     * @param message Message to broadcast
     * @param priority Message priority (0-100)
     */
    void BroadcastMessage(::std::string const& message, uint32 priority);

    /**
     * @brief Request zone-wide assembly
     * @param position Assembly position
     * @param radius Assembly radius
     * @return Number of bots responding
     */
    uint32 RequestAssembly(Position const& position, float radius);

    /**
     * @brief Balance bot distribution across raids
     */
    void BalanceBotDistribution();

    /**
     * @brief Get zone statistics
     */
    struct ZoneStats
    {
        uint32 totalBots;
        uint32 activeBots;
        uint32 idleBots;
        uint32 raidCount;
        uint32 activeObjectives;
        ThreatLevel threatLevel;
        ZoneActivity currentActivity;
        float avgBotLevel;
        uint32 totalDPS;
        uint32 totalHPS;
    };

    ZoneStats GetZoneStats() const;

private:
    void UpdateRaids(uint32 diff);
    void UpdateObjectives(uint32 diff);
    void UpdateThreatAssessment(uint32 diff);
    void UpdateBotActivity(uint32 diff);
    void UpdateLoadBalancing(uint32 diff);

    // Threat assessment
    void ScanForThreats();
    void DetectWorldBoss();
    void DetectZoneEvents();

    // Objective management
    void CleanupExpiredObjectives();
    void PrioritizeObjectives();

    // Bot distribution
    void AssignBotsToRaids();
    void RebalanceRaids();
    void OptimizeRaidComposition();

    uint32 _zoneId;
    ::std::vector<ObjectGuid> _bots;
    ::std::vector<::std::unique_ptr<RaidOrchestrator>> _raids;

    // Zone state
    ZoneActivity _currentActivity = ZoneActivity::IDLE;
    ThreatLevel _threatLevel = ThreatLevel::PEACEFUL;

    // Objectives
    ::std::vector<ZoneObjective> _objectives;

    // Statistics
    ZoneStats _cachedStats;
    uint32 _lastStatsUpdate = 0;

    // Performance
    uint32 _lastUpdateTime = 0;
    uint32 _updateInterval = 1000; // 1s update interval
};

/**
 * @brief Zone Orchestrator Manager
 * Manages orchestrators across all zones
 */
class TC_GAME_API ZoneOrchestratorManager
{
public:
    /**
     * @brief Get orchestrator for zone
     * @param zoneId Zone ID
     * @return Orchestrator or nullptr
     */
    static ZoneOrchestrator* GetOrchestrator(uint32 zoneId);

    /**
     * @brief Create orchestrator for zone
     * @param zoneId Zone ID
     * @return Created orchestrator
     */
    static ZoneOrchestrator* CreateOrchestrator(uint32 zoneId);

    /**
     * @brief Remove orchestrator for zone
     * @param zoneId Zone ID
     */
    static void RemoveOrchestrator(uint32 zoneId);

    /**
     * @brief Update all orchestrators
     * @param diff Time since last update
     */
    static void UpdateAll(uint32 diff);

    /**
     * @brief Get all orchestrators
     */
    static ::std::unordered_map<uint32, ::std::unique_ptr<ZoneOrchestrator>> const& GetAll();

    /**
     * @brief Clear all orchestrators
     */
    static void Clear();

    /**
     * @brief Get global statistics
     */
    struct GlobalStats
    {
        uint32 totalZones;
        uint32 totalBots;
        uint32 totalRaids;
        uint32 activeObjectives;
        uint32 criticalZones; // Zones with CRITICAL threat
    };

    static GlobalStats GetGlobalStats();

private:
    static ::std::unordered_map<uint32, ::std::unique_ptr<ZoneOrchestrator>> _orchestrators;
};

} // namespace Coordination
} // namespace Playerbot

#endif // TRINITYCORE_ZONE_ORCHESTRATOR_H
