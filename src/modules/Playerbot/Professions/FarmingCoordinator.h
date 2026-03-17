/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * FARMING COORDINATOR SYSTEM FOR PLAYERBOT
 *
 * This system coordinates profession leveling with character progression:
 * - Monitors profession skill gap vs character level
 * - Triggers farming sessions when skills fall behind
 * - Selects optimal zones/nodes for current skill level
 * - Balances gathering vs crafting for efficient leveling
 * - Manages material stockpiles for auction house
 *
 * Skill Sync Algorithm:
 * - Target: Profession skill ≈ Character Level × 5
 * - Example: Level 20 character should have ~100 skill
 * - Trigger: If skill < (char_level × 5 - 25), start farming
 * - Duration: Farm until skill ≥ (char_level × 5)
 *
 * Integration:
 * - Works with ProfessionManager for skill tracking
 * - Uses GatheringAutomation for node harvesting
 * - Coordinates with auction house for material management
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include "ProfessionManager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace Playerbot
{

/**
 * @brief Farming session types
 */
enum class FarmingSessionType : uint8
{
    NONE = 0,
    SKILL_CATCHUP,          // Farm to catch up profession skill to character level
    MATERIAL_STOCKPILE,     // Farm materials for auction house
    CRAFTING_MATERIALS,     // Farm materials for crafting leveling
    QUEST_SUPPORT           // Farm materials for quest completion
};

/**
 * @brief Zone information for farming
 */
struct FarmingZoneInfo
{
    uint32 zoneId;
    uint32 areaId;
    std::string zoneName;
    uint16 minSkillLevel;           // Minimum profession skill
    uint16 maxSkillLevel;           // Maximum useful skill level
    ProfessionType profession;
    std::vector<uint32> nodeEntries; // GameObject entries for nodes in zone
    Position centerPosition;
    float zoneRadius;
    uint8 recommendedCharLevel;      // Character level for this zone
    bool isContested;                // PvP zone

    FarmingZoneInfo()
        : zoneId(0), areaId(0), minSkillLevel(0), maxSkillLevel(0),
          profession(ProfessionType::NONE), zoneRadius(0.0f),
          recommendedCharLevel(0), isContested(false) {}
};

/**
 * @brief Active farming session
 */
struct FarmingSession
{
    uint32 sessionId;
    uint32 playerGuid;
    FarmingSessionType sessionType;
    ProfessionType profession;
    FarmingZoneInfo zone;
    uint32 startTime;
    uint32 duration;                    // Expected duration (ms)
    uint16 startingSkill;
    uint16 targetSkill;
    uint32 nodesGathered;
    uint32 materialsCollected;
    bool isActive;
    Position originalPosition;          // Return position after session

    FarmingSession()
        : sessionId(0), playerGuid(0), sessionType(FarmingSessionType::NONE),
          profession(ProfessionType::NONE), startTime(0), duration(0),
          startingSkill(0), targetSkill(0), nodesGathered(0),
          materialsCollected(0), isActive(false) {}
};

/**
 * @brief Farming coordination profile per bot
 */
struct FarmingCoordinatorProfile
{
    bool autoFarm = true;                   // Enable automatic farming
    uint16 skillGapThreshold = 25;          // Skill gap before triggering farm (default: 25)
    float skillLevelMultiplier = 5.0f;      // Target = char_level × multiplier (default: 5.0)
    uint32 maxFarmingDuration = 1800000;    // Max farming session (30 minutes)
    uint32 minFarmingDuration = 300000;     // Min farming session (5 minutes)
    bool returnToOriginalPosition = true;   // Return after farming
    bool prioritizePrimaryProfessions = true; // Farm primary before secondary
    uint32 materialStockpileTarget = 100;   // Target stack size for AH materials

    // Session scheduling
    uint32 farmingCooldown = 600000;        // 10 minutes between sessions
    bool farmDuringDowntime = true;         // Farm when not questing/grouping
    bool farmWhileTraveling = true;         // Opportunistic gathering

    FarmingCoordinatorProfile() = default;
};

/**
 * @brief Farming statistics per bot
 */
struct FarmingStatistics
{
    std::atomic<uint32> sessionsCompleted{0};
    std::atomic<uint32> totalTimeSpent{0};          // Milliseconds
    std::atomic<uint32> totalNodesGathered{0};
    std::atomic<uint32> totalSkillPointsGained{0};
    std::atomic<uint32> totalMaterialsCollected{0};
    std::atomic<uint32> zonesVisited{0};

    void Reset()
    {
        sessionsCompleted = 0;
        totalTimeSpent = 0;
        totalNodesGathered = 0;
        totalSkillPointsGained = 0;
        totalMaterialsCollected = 0;
        zonesVisited = 0;
    }
};

/**
 * @brief Complete farming coordination system for profession leveling
 *
 * **Phase 5.2: Singleton → Per-Bot Instance Pattern**
 *
 * DESIGN PRINCIPLE: Per-bot instance owned by GameSystemsManager
 * - Each bot has its own FarmingCoordinator instance
 * - No mutex locking (per-bot isolation)
 * - Direct member access (no map lookups)
 * - Integrates with profession and gathering systems via facade
 * - Coordinates skill leveling with character progression
 *
 * **Ownership:**
 * - Owned by GameSystemsManager via std::unique_ptr
 * - Constructed per-bot with Player* reference
 * - Destroyed with bot cleanup
 */
class TC_GAME_API FarmingCoordinator final
{
public:
    /**
     * @brief Construct farming coordinator for bot
     * @param bot The bot player this manager serves
     */
    explicit FarmingCoordinator(Player* bot);

    /**
     * @brief Destructor - cleanup per-bot resources
     */
    ~FarmingCoordinator();

    // ============================================================================
    // CORE FARMING COORDINATION
    // ============================================================================

    /**
     * Initialize farming coordinator (called once per bot)
     */
    void Initialize();

    /**
     * Update farming coordination (called periodically)
     */
    void Update(::Player* player, uint32 diff);

    /**
     * Enable/disable farming coordination for this bot
     */
    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    /**
     * Get coordination profile for this bot
     */
    void SetCoordinatorProfile(FarmingCoordinatorProfile const& profile);
    FarmingCoordinatorProfile GetCoordinatorProfile() const;

    // ============================================================================
    // SKILL ANALYSIS
    // ============================================================================

    /**
     * Check if profession skill needs catch-up farming
     * Returns true if skill gap exceeds threshold
     */
    bool NeedsFarming(ProfessionType profession) const;

    /**
     * Calculate skill gap for profession
     * Returns: (target skill) - (current skill)
     * Positive = behind, Negative = ahead
     */
    int32 GetSkillGap(ProfessionType profession) const;

    /**
     * Get target skill level for character level
     * Formula: character_level × skillLevelMultiplier
     */
    uint16 GetTargetSkillLevel(ProfessionType profession) const;

    /**
     * Get professions that need farming (sorted by priority)
     */
    std::vector<ProfessionType> GetProfessionsNeedingFarm() const;

    /**
     * Calculate recommended farming duration based on skill gap
     */
    uint32 CalculateFarmingDuration(ProfessionType profession) const;

    // ============================================================================
    // FARMING SESSION MANAGEMENT
    // ============================================================================

    /**
     * Start farming session for profession
     */
    bool StartFarmingSession(ProfessionType profession, FarmingSessionType sessionType = FarmingSessionType::SKILL_CATCHUP);

    /**
     * Stop active farming session
     */
    void StopFarmingSession();

    /**
     * Get active farming session for this bot
     */
    FarmingSession const* GetActiveFarmingSession() const;

    /**
     * Check if this bot has active farming session
     */
    bool HasActiveFarmingSession() const;

    /**
     * Update farming session progress
     */
    void UpdateFarmingSession(uint32 diff);

    /**
     * Check if farming session should end
     */
    bool ShouldEndFarmingSession(FarmingSession const& session) const;

    // ============================================================================
    // ZONE SELECTION
    // ============================================================================

    /**
     * Get optimal farming zone for profession and skill level
     */
    FarmingZoneInfo const* GetOptimalFarmingZone(ProfessionType profession) const;

    /**
     * Get all suitable zones for skill level
     */
    std::vector<FarmingZoneInfo> GetSuitableZones(ProfessionType profession) const;

    /**
     * Calculate zone score based on:
     * - Distance from current position
     * - Node density
     * - Skill-up potential
     * - Safety (PvP risk)
     */
    float CalculateZoneScore(FarmingZoneInfo const& zone) const;

    // ============================================================================
    // MATERIAL MANAGEMENT
    // ============================================================================

    /**
     * Check if material stockpile target reached
     */
    bool HasReachedStockpileTarget(uint32 itemId) const;

    /**
     * Get current material count in inventory
     */
    uint32 GetMaterialCount(uint32 itemId) const;

    /**
     * Get materials needed for auction house target
     */
    std::vector<std::pair<uint32, uint32>> GetNeededMaterials(ProfessionType profession) const;

    // ============================================================================
    // STATISTICS
    // ============================================================================

    /**
     * Get statistics for this bot
     */
    FarmingStatistics const& GetStatistics() const;

    /**
     * Get global statistics across all bots
     */
    static FarmingStatistics const& GetGlobalStatistics();

    /**
     * Reset statistics for this bot
     */
    void ResetStatistics();

private:
    // Non-copyable
    FarmingCoordinator(FarmingCoordinator const&) = delete;
    FarmingCoordinator& operator=(FarmingCoordinator const&) = delete;

    // ============================================================================
    // INITIALIZATION HELPERS
    // ============================================================================

    static void LoadFarmingZones();  // Load shared zone database once
    static void InitializeZoneDatabase();
    static void InitializeMiningZones();
    static void InitializeHerbalismZones();
    static void InitializeSkinningZones();

    // ============================================================================
    // FARMING HELPERS
    // ============================================================================

    /**
     * Generate unique session ID
     */
    static uint32 GenerateSessionId();

    /**
     * Teleport/navigate bot to farming zone
     */
    bool TravelToFarmingZone(FarmingZoneInfo const& zone);

    /**
     * Return bot to original position after farming
     */
    void ReturnToOriginalPosition(FarmingSession const& session);

    /**
     * Check if farming conditions are met (not in combat, not in group, etc.)
     */
    bool CanStartFarming() const;

    /**
     * Validate farming session is still valid
     */
    bool ValidateFarmingSession(FarmingSession const& session) const;

    // ============================================================================
    // INTEGRATION HELPERS
    // ============================================================================

    /**
     * Get ProfessionManager via GameSystemsManager facade
     */
    class ProfessionManager* GetProfessionManager();

    /**
     * Get GatheringManager via GameSystemsManager facade
     */
    class GatheringManager* GetGatheringManager();

    // ============================================================================
    // PER-BOT INSTANCE DATA
    // ============================================================================

    Player* _bot;                               // Bot reference (non-owning)
    FarmingSession _activeSession;              // Active farming session for this bot
    FarmingCoordinatorProfile _profile;         // Coordination profile for this bot
    FarmingStatistics _statistics;              // Statistics for this bot
    uint32 _lastFarmingTime{0};                 // Last farming timestamp for cooldown
    bool _enabled{true};                        // Farming automation enabled

    // ============================================================================
    // SHARED STATIC DATA
    // ============================================================================

    // Farming zone database (profession -> zones sorted by skill level)
    // Shared across all bots, initialized once
    static std::unordered_map<ProfessionType, std::vector<FarmingZoneInfo>> _farmingZones;
    static bool _farmingZonesInitialized;

    // Global statistics across all bots
    static FarmingStatistics _globalStatistics;

    // Session ID generator (shared across all bots)
    static std::atomic<uint32> _nextSessionId;

    // Update intervals
    static constexpr uint32 FARMING_CHECK_INTERVAL = 10000;     // 10 seconds
    static constexpr uint32 SESSION_UPDATE_INTERVAL = 5000;     // 5 seconds
};

} // namespace Playerbot
