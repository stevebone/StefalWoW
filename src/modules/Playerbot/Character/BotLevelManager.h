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
#include "Threading/LockHierarchy.h"
#include "SharedDefines.h"
#include "ObjectGuid.h"
#include "../Equipment/BotGearFactory.h"  // Need full definition for std::unique_ptr<GearSet>
#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <functional>

class Player;

namespace Playerbot
{

// Forward declarations
struct LevelBracket;
struct TalentLoadout;
struct ZonePlacement;
class BotLevelDistribution;
class BotGearFactory;
class BotTalentManager;
class BotWorldPositioner;

/**
 * Bot Creation Task
 * Data prepared by worker thread, applied by main thread
 */
struct BotCreationTask
{
    // Bot identity
    ObjectGuid botGuid;
    uint32 accountId;
    ::std::string botName;

    // Character data (prepared in worker thread)
    uint8 race;
    uint8 cls;
    uint8 gender;
    TeamId faction;

    // Level data (prepared in worker thread)
    uint32 targetLevel;
    LevelBracket const* levelBracket;

    // Specialization data (prepared in worker thread)
    uint8 primarySpec;
    uint8 secondarySpec;  // For dual-spec (level 10+)
    bool useDualSpec;

    // Gear data (prepared in worker thread)
    ::std::unique_ptr<GearSet> gearSet;

    // Zone data (prepared in worker thread)
    ZonePlacement const* zonePlacement;

    // Level change tracking (set during ApplyLevel)
    bool levelChanged = false;  // True if level was actually modified (up or down)

    // Task metadata
    uint64 taskId;
    ::std::chrono::steady_clock::time_point createdAt;
    ::std::chrono::steady_clock::time_point preparedAt;

    BotCreationTask()
        : accountId(0), race(0), cls(0), gender(0), faction(TEAM_ALLIANCE)
        , targetLevel(0), levelBracket(nullptr), primarySpec(0), secondarySpec(0)
        , useDualSpec(false), zonePlacement(nullptr), taskId(0)
    {
        createdAt = ::std::chrono::steady_clock::now();
    }
};

/**
 * Bot Level Manager - Orchestrator for Automated World Population
 *
 * Purpose: Coordinate all systems for instant bot creation and level-up
 *
 * Orchestrated Systems:
 * 1. BotLevelDistribution - Level bracket selection
 * 2. BotGearFactory - Gear generation
 * 3. BotTalentManager - Spec/talent application
 * 4. BotWorldPositioner - Zone placement
 *
 * Two-Phase Bot Creation:
 * Phase 1 (Worker Thread):
 *   - Select level bracket
 *   - Choose specialization(s)
 *   - Generate gear set
 *   - Select zone placement
 *   - NO Player API calls
 *
 * Phase 2 (Main Thread):
 *   - GiveLevel() to target level
 *   - Apply specialization
 *   - Apply talents (InitTalentForLevel, LearnTalent)
 *   - Equip gear (EquipItem)
 *   - Teleport to zone (TeleportTo)
 *   - Save to database
 *
 * Thread Safety:
 * - Worker threads prepare data using lock-free caches
 * - Main thread applies data using Player API
 * - Task queue protected by mutex (low contention)
 * - Atomic counters for statistics
 *
 * Performance:
 * - Worker thread prep: <5ms per bot
 * - Main thread apply: <50ms per bot (throttled to 10/update)
 * - Memory: ~1KB per queued task
 * - Scales to 5000+ bots
 *
 * Throttling:
 * - Maximum 10 bots processed per server update (configurable)
 * - Prevents server stalls from bulk bot creation
 * - Queue drains naturally over time
 *
 * Distribution Monitoring:
 * - Tracks bot distribution across level brackets
 * - Automatic rebalancing (future enhancement)
 * - Statistics reporting
 */
/**
 * @brief Snapshot of statistics for the level manager (copyable)
 */
struct LevelManagerStatsSnapshot
{
    uint64 botsCreated{0};
    uint64 botsProcessed{0};
    uint64 tasksPrepared{0};
    uint64 tasksApplied{0};
    uint64 tasksQueued{0};
    uint64 levelUps{0};
    uint64 gearSetsApplied{0};
    uint64 talentsApplied{0};
    uint64 zonesAssigned{0};
    uint64 errors{0};

    LevelManagerStatsSnapshot() = default;
};

/**
 * @brief Thread-safe statistics for the level manager
 */
struct LevelManagerStats
{
    ::std::atomic<uint64> botsCreated{0};
    ::std::atomic<uint64> botsProcessed{0};
    ::std::atomic<uint64> tasksPrepared{0};
    ::std::atomic<uint64> tasksApplied{0};
    ::std::atomic<uint64> tasksQueued{0};
    ::std::atomic<uint64> levelUps{0};
    ::std::atomic<uint64> gearSetsApplied{0};
    ::std::atomic<uint64> talentsApplied{0};
    ::std::atomic<uint64> zonesAssigned{0};
    ::std::atomic<uint64> errors{0};
    // Task tracking
    ::std::atomic<uint64> totalTasksSubmitted{0};
    ::std::atomic<uint64> totalTasksCompleted{0};
    ::std::atomic<uint64> totalTasksFailed{0};
    ::std::atomic<uint64> totalApplyTimeMs{0};
    ::std::atomic<uint32> averageApplyTimeMs{0};
    // Prep time tracking
    ::std::atomic<uint64> totalPrepTimeMs{0};
    ::std::atomic<uint32> averagePrepTimeMs{0};
    // Operations tracking
    ::std::atomic<uint64> totalLevelUps{0};
    ::std::atomic<uint64> totalTalentApplications{0};
    ::std::atomic<uint64> totalGearApplications{0};
    ::std::atomic<uint64> totalTeleports{0};
    // Failure tracking
    ::std::atomic<uint64> levelUpFailures{0};
    ::std::atomic<uint64> talentFailures{0};
    ::std::atomic<uint64> gearFailures{0};
    ::std::atomic<uint64> teleportFailures{0};
    // Queue tracking
    ::std::atomic<uint32> currentQueueSize{0};
    ::std::atomic<uint32> peakQueueSize{0};

    LevelManagerStats() = default;

    // Reset all statistics to zero
    void Reset()
    {
        botsCreated.store(0, ::std::memory_order_relaxed);
        botsProcessed.store(0, ::std::memory_order_relaxed);
        tasksPrepared.store(0, ::std::memory_order_relaxed);
        tasksApplied.store(0, ::std::memory_order_relaxed);
        tasksQueued.store(0, ::std::memory_order_relaxed);
        levelUps.store(0, ::std::memory_order_relaxed);
        gearSetsApplied.store(0, ::std::memory_order_relaxed);
        talentsApplied.store(0, ::std::memory_order_relaxed);
        zonesAssigned.store(0, ::std::memory_order_relaxed);
        errors.store(0, ::std::memory_order_relaxed);
        totalTasksSubmitted.store(0, ::std::memory_order_relaxed);
        totalTasksCompleted.store(0, ::std::memory_order_relaxed);
        totalTasksFailed.store(0, ::std::memory_order_relaxed);
        totalApplyTimeMs.store(0, ::std::memory_order_relaxed);
        averageApplyTimeMs.store(0, ::std::memory_order_relaxed);
        totalPrepTimeMs.store(0, ::std::memory_order_relaxed);
        averagePrepTimeMs.store(0, ::std::memory_order_relaxed);
        totalLevelUps.store(0, ::std::memory_order_relaxed);
        totalTalentApplications.store(0, ::std::memory_order_relaxed);
        totalGearApplications.store(0, ::std::memory_order_relaxed);
        totalTeleports.store(0, ::std::memory_order_relaxed);
        levelUpFailures.store(0, ::std::memory_order_relaxed);
        talentFailures.store(0, ::std::memory_order_relaxed);
        gearFailures.store(0, ::std::memory_order_relaxed);
        teleportFailures.store(0, ::std::memory_order_relaxed);
        currentQueueSize.store(0, ::std::memory_order_relaxed);
        peakQueueSize.store(0, ::std::memory_order_relaxed);
    }

    LevelManagerStatsSnapshot GetSnapshot() const
    {
        LevelManagerStatsSnapshot snapshot;
        snapshot.botsCreated = botsCreated.load(::std::memory_order_relaxed);
        snapshot.botsProcessed = botsProcessed.load(::std::memory_order_relaxed);
        snapshot.tasksPrepared = tasksPrepared.load(::std::memory_order_relaxed);
        snapshot.tasksApplied = tasksApplied.load(::std::memory_order_relaxed);
        snapshot.tasksQueued = tasksQueued.load(::std::memory_order_relaxed);
        snapshot.levelUps = levelUps.load(::std::memory_order_relaxed);
        snapshot.gearSetsApplied = gearSetsApplied.load(::std::memory_order_relaxed);
        snapshot.talentsApplied = talentsApplied.load(::std::memory_order_relaxed);
        snapshot.zonesAssigned = zonesAssigned.load(::std::memory_order_relaxed);
        snapshot.errors = errors.load(::std::memory_order_relaxed);
        return snapshot;
    }
};

class TC_GAME_API BotLevelManager final
{
public:
    static BotLevelManager* instance();

    // IBotLevelManager interface implementation

    /**
     * Initialize all subsystems
     * MUST be called before any bot operations
     * Single-threaded execution required
     */
    bool Initialize();

    /**
     * Shutdown all subsystems
     * Called during server shutdown
     */
    void Shutdown();

    /**
     * Check if manager is ready
     */
    bool IsReady() const
    {
        return _initialized.load(::std::memory_order_acquire);
    }

    // ====================================================================
    // BOT CREATION API (Two-Phase Workflow)
    // ====================================================================

    /**
     * Create bot with instant level-up (async)
     * Returns task ID for tracking
     *
     * Workflow:
     * 1. Submit task to ThreadPool (worker thread)
     * 2. Worker prepares all data (level, gear, talents, zone)
     * 3. Task queued for main thread
     * 4. Main thread applies data on next update
     *
     * @param bot           Bot player object
     * @return              Task ID (0 if failed)
     */
    uint64 CreateBotAsync(Player* bot);

    /**
     * Create multiple bots in batch (async)
     * More efficient than individual creation
     *
     * @param bots          Vector of bot player objects
     * @return              Number of tasks submitted
     */
    uint32 CreateBotsAsync(::std::vector<Player*> const& bots);

    /**
     * Process queued bot creation tasks (main thread only)
     * Called from server update loop
     *
     * Throttling: Processes up to maxBots per call
     *
     * @param maxBots       Maximum bots to process (default: 10)
     * @return              Number of bots processed
     */
    uint32 ProcessBotCreationQueue(uint32 maxBots = 10);

    // ====================================================================
    // DISTRIBUTION MANAGEMENT
    // ====================================================================

    /**
     * Get target level bracket for new bot
     * Thread-safe, uses weighted selection
     *
     * @param faction       TEAM_ALLIANCE or TEAM_HORDE
     * @return              Level bracket, or nullptr if none available
     */
    LevelBracket const* SelectLevelBracket(TeamId faction);

    /**
     * Check distribution balance
     * Returns true if all brackets within tolerance (±15%)
     */
    bool IsDistributionBalanced() const;

    /**
     * Get distribution deviation percentage
     * 0% = perfect balance, >15% = needs rebalancing
     */
    float GetDistributionDeviation() const;

    /**
     * Force rebalance distribution
     * Redistributes bots to match target percentages
     * Analyzes over/underpopulated brackets and coordinates spawning
     */
    void RebalanceDistribution();

private:
    /**
     * Rebalance distribution for a specific faction
     * @param faction The faction to rebalance (TEAM_ALLIANCE or TEAM_HORDE)
     */
    void RebalanceFaction(TeamId faction);

    // ====================================================================
    // STATISTICS & MONITORING
    // ====================================================================

    LevelManagerStatsSnapshot GetStats() const { return _stats.GetSnapshot(); }
    void PrintReport() const;
    ::std::string GetSummary() const;

    // ====================================================================
    // CONFIGURATION
    // ====================================================================

    /**
     * Set maximum bots to process per update
     * Default: 10
     */
    void SetMaxBotsPerUpdate(uint32 maxBots)
    {
        _maxBotsPerUpdate.store(maxBots, ::std::memory_order_release);
    }

    uint32 GetMaxBotsPerUpdate() const
    {
        return _maxBotsPerUpdate.load(::std::memory_order_acquire);
    }

    /**
     * Enable/disable verbose logging
     */
    void SetVerboseLogging(bool enabled)
    {
        _verboseLogging.store(enabled, ::std::memory_order_release);
    }

private:
    BotLevelManager() = default;
    ~BotLevelManager() = default;
    BotLevelManager(BotLevelManager const&) = delete;
    BotLevelManager& operator=(BotLevelManager const&) = delete;

    // ====================================================================
    // WORKER THREAD TASKS (Phase 1: Data Preparation)
    // ====================================================================

    /**
     * Prepare bot creation data (worker thread)
     * NO Player API calls allowed
     */
    void PrepareBot_WorkerThread(::std::shared_ptr<BotCreationTask> task);

    /**
     * Generate bot character data
     */
    void GenerateCharacterData(BotCreationTask* task);

    /**
     * Select level and bracket
     */
    void SelectLevel(BotCreationTask* task);

    /**
     * Select specializations (primary + secondary for dual-spec)
     */
    void SelectSpecializations(BotCreationTask* task);

    /**
     * Generate gear set
     */
    void GenerateGear(BotCreationTask* task);

    /**
     * Select zone placement
     */
    void SelectZone(BotCreationTask* task);

    // ====================================================================
    // MAIN THREAD TASKS (Phase 2: Player API Application)
    // ====================================================================

    /**
     * Apply bot creation data (main thread only)
     * Uses Player API
     */
    bool ApplyBot_MainThread(BotCreationTask* task);

    /**
     * Apply level-up to target level
     */
    bool ApplyLevel(Player* bot, BotCreationTask* task);

    /**
     * Apply specialization and talents
     */
    bool ApplyTalents(Player* bot, BotCreationTask* task);

    /**
     * Equip gear set
     */
    bool ApplyGear(Player* bot, BotCreationTask* task);

    /**
     * Teleport to zone
     */
    bool ApplyZone(Player* bot, BotCreationTask* task);

    /**
     * Apply professions for bot (level 10+)
     * Learns 2 main professions + cooking/fishing and levels up skills
     */
    bool ApplyProfessions(Player* bot, BotCreationTask* task);

    /**
     * Apply riding skills and mounts for bot (level 10+)
     * Level thresholds: 10 (Apprentice), 20 (Journeyman), 30 (Expert/Flying), 40 (Artisan), 80 (Master)
     */
    bool ApplyRiding(Player* bot, BotCreationTask* task);

    // ====================================================================
    // TASK QUEUE MANAGEMENT
    // ====================================================================

    /**
     * Queue task for main thread processing
     */
    void QueueMainThreadTask(::std::shared_ptr<BotCreationTask> task);

    /**
     * Get next task from queue (main thread only)
     */
    ::std::shared_ptr<BotCreationTask> DequeueTask();

    // ====================================================================
    // SUBSYSTEM REFERENCES
    // ====================================================================

    BotLevelDistribution* _distribution;
    BotGearFactory* _gearFactory;
    BotTalentManager* _talentManager;
    BotWorldPositioner* _positioner;

    // ====================================================================
    // TASK QUEUE
    // ====================================================================

    ::std::queue<::std::shared_ptr<BotCreationTask>> _mainThreadQueue;
    Playerbot::OrderedMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _queueMutex;

    // ====================================================================
    // STATISTICS
    // ====================================================================

    LevelManagerStats _stats;
    ::std::atomic<uint64> _nextTaskId{1};

    // ====================================================================
    // CONFIGURATION
    // ====================================================================

    ::std::atomic<uint32> _maxBotsPerUpdate{10};
    ::std::atomic<bool> _verboseLogging{false};
    ::std::atomic<bool> _initialized{false};
};

} // namespace Playerbot

#define sBotLevelManager Playerbot::BotLevelManager::instance()
