/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * GuildTaskManager: Generates guild tasks (kill, gather, craft, fish, mine, herb,
 * dungeon, deliver, scout) and assigns them to guild bot members for autonomous
 * completion. Tasks are generated periodically based on guild needs and member
 * capabilities, providing meaningful guild participation for AI bots.
 *
 * Architecture:
 *   GuildTaskManager (singleton) ← SubsystemRegistry
 *       ├── TaskGenerator     - Creates tasks from templates + guild context
 *       ├── TaskAssigner      - Matches tasks to capable bots
 *       └── TaskTracker       - Monitors progress and completion
 *
 * Thread Safety: Main thread only (Update called from subsystem registry).
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>

class Player;
class Guild;

namespace Playerbot
{

/// Types of guild tasks bots can perform
enum class GuildTaskType : uint8
{
    KILL        = 0,   // Kill N creatures of a specific type
    GATHER      = 1,   // Gather N resource nodes (generic)
    CRAFT       = 2,   // Craft N items of a specific profession
    FISH        = 3,   // Catch N fish from fishing
    MINE        = 4,   // Mine N mineral nodes
    HERB        = 5,   // Pick N herb nodes
    SKIN        = 6,   // Skin N creatures
    DUNGEON     = 7,   // Complete a dungeon run
    DELIVER     = 8,   // Deliver gold/items to guild bank
    SCOUT       = 9,   // Explore/visit a specific zone

    MAX_TASK_TYPE
};

/// Task status lifecycle
enum class GuildTaskStatus : uint8
{
    AVAILABLE   = 0,   // Open for assignment
    ASSIGNED    = 1,   // Assigned to a bot
    IN_PROGRESS = 2,   // Bot is working on it
    COMPLETED   = 3,   // Successfully finished
    FAILED      = 4,   // Bot couldn't complete
    EXPIRED     = 5    // Deadline passed
};

/// Task difficulty affects reward scaling and assignment matching
enum class GuildTaskDifficulty : uint8
{
    EASY        = 0,   // Any bot can do this
    NORMAL      = 1,   // Requires appropriate level
    HARD        = 2,   // Requires good gear / specific spec
    ELITE       = 3    // Requires group or high-level bot
};

/// A single guild task definition
struct GuildTask
{
    uint32 taskId = 0;
    uint32 guildId = 0;
    GuildTaskType type = GuildTaskType::KILL;
    GuildTaskDifficulty difficulty = GuildTaskDifficulty::NORMAL;
    GuildTaskStatus status = GuildTaskStatus::AVAILABLE;

    std::string title;
    std::string description;

    // Task requirements
    uint32 targetEntry = 0;        // Creature/item/node entry ID (0 = any)
    uint32 targetCount = 0;        // How many to kill/gather/craft
    uint32 currentCount = 0;       // Progress
    uint32 requiredLevel = 0;      // Minimum bot level
    uint32 requiredSkill = 0;      // Required profession skill ID (0 = none)
    uint32 requiredSkillValue = 0; // Minimum skill value
    uint32 zoneId = 0;             // Target zone (0 = any)

    // Assignment
    ObjectGuid assigneeGuid;       // Bot assigned to this task
    uint32 assignedTime = 0;       // When assigned (GameTime)
    uint32 deadline = 0;           // When task expires (GameTime)

    // Rewards
    uint32 rewardGold = 0;         // Gold reward (in copper)
    uint32 rewardReputation = 0;   // Guild reputation reward
    uint32 rewardItemId = 0;       // Optional item reward
    uint32 rewardItemCount = 0;    // Item reward count

    // Metadata
    uint32 createdTime = 0;
    uint32 completedTime = 0;

    float GetProgressPercent() const
    {
        return targetCount > 0 ? static_cast<float>(currentCount) / targetCount * 100.0f : 0.0f;
    }

    bool IsComplete() const { return currentCount >= targetCount; }
    bool IsExpired(uint32 now) const { return deadline > 0 && now > deadline; }
};

/// Template for auto-generating tasks
struct GuildTaskTemplate
{
    uint32 templateId = 0;
    GuildTaskType type = GuildTaskType::KILL;
    GuildTaskDifficulty difficulty = GuildTaskDifficulty::NORMAL;

    std::string titleFormat;       // e.g., "Slay {} {}" (count, creature name)
    std::string descriptionFormat;

    uint32 targetEntry = 0;
    uint32 minCount = 0;
    uint32 maxCount = 0;
    uint32 requiredLevel = 0;
    uint32 requiredSkill = 0;
    uint32 requiredSkillValue = 0;
    uint32 zoneId = 0;

    uint32 baseGoldReward = 0;     // In copper
    uint32 baseRepReward = 0;
    uint32 durationHours = 24;     // Task lifetime in hours

    float weight = 1.0f;           // Selection weight for random generation
};

/// Per-guild task board state
struct GuildTaskBoard
{
    uint32 guildId = 0;
    std::vector<GuildTask> activeTasks;
    uint32 totalTasksGenerated = 0;
    uint32 totalTasksCompleted = 0;
    uint32 totalTasksFailed = 0;
    uint32 lastGenerationTime = 0;
    uint32 lastCleanupTime = 0;
};

/// Manager statistics
struct GuildTaskManagerMetrics
{
    uint32 totalGuildsTracked = 0;
    uint32 totalActiveTasks = 0;
    uint32 totalCompletedTasks = 0;
    uint32 totalFailedTasks = 0;
    uint32 totalExpiredTasks = 0;
    uint32 totalGoldAwarded = 0;
};

/**
 * @class GuildTaskManager
 * @brief Generates and manages guild tasks for bot members
 *
 * Singleton manager that:
 * 1. Periodically generates tasks for each guild with bot members
 * 2. Assigns tasks to idle bots based on capability matching
 * 3. Tracks progress and awards rewards on completion
 * 4. Cleans up expired/failed tasks
 */
class TC_GAME_API GuildTaskManager
{
public:
    static GuildTaskManager* instance()
    {
        static GuildTaskManager inst;
        return &inst;
    }

    /// Initialize: load templates from DB
    bool Initialize();

    /// Main update loop (called from subsystem registry)
    void Update(uint32 diff);

    /// Shutdown and cleanup
    void Shutdown();

    // ========================================================================
    // Task Board Queries
    // ========================================================================

    /// Get all active tasks for a guild
    std::vector<GuildTask> GetActiveTasks(uint32 guildId) const;

    /// Get tasks assigned to a specific bot
    std::vector<GuildTask> GetBotTasks(ObjectGuid botGuid) const;

    /// Get available (unassigned) tasks for a guild
    std::vector<GuildTask> GetAvailableTasks(uint32 guildId) const;

    /// Get task by ID
    GuildTask const* GetTask(uint32 taskId) const;

    // ========================================================================
    // Task Management
    // ========================================================================

    /// Manually create a task for a guild
    uint32 CreateTask(uint32 guildId, GuildTaskType type, uint32 targetEntry,
                      uint32 targetCount, uint32 durationHours = 24);

    /// Assign a task to a specific bot
    bool AssignTask(uint32 taskId, Player* bot);

    /// Report progress on a task (called by bot AI)
    void ReportProgress(uint32 taskId, uint32 incrementCount);

    /// Complete a task (awards rewards)
    void CompleteTask(uint32 taskId);

    /// Fail/abandon a task
    void AbandonTask(uint32 taskId);

    // ========================================================================
    // Metrics
    // ========================================================================

    GuildTaskManagerMetrics GetMetrics() const;

    /// Get count of templates loaded
    uint32 GetTemplateCount() const { return static_cast<uint32>(_templates.size()); }

private:
    GuildTaskManager();
    ~GuildTaskManager() = default;
    GuildTaskManager(GuildTaskManager const&) = delete;
    GuildTaskManager& operator=(GuildTaskManager const&) = delete;

    // Task generation
    void GenerateTasksForGuilds();
    void GenerateTasksForGuild(uint32 guildId);
    GuildTask GenerateTaskFromTemplate(uint32 guildId, GuildTaskTemplate const& tmpl);

    // Task assignment
    void AssignUnassignedTasks();
    bool CanBotDoTask(Player* bot, GuildTask const& task) const;
    float ScoreBotForTask(Player* bot, GuildTask const& task) const;

    // Task lifecycle
    void CleanupExpiredTasks();
    void AwardTaskRewards(GuildTask const& task);

    // Template loading
    void LoadTemplatesFromDB();
    void LoadDefaultTemplates();

    // State
    std::vector<GuildTaskTemplate> _templates;
    std::unordered_map<uint32, GuildTaskBoard> _guildBoards;  // guildId → board
    uint32 _nextTaskId = 1;

    // Timers
    uint32 _timeSinceGeneration = 0;
    uint32 _timeSinceAssignment = 0;
    uint32 _timeSinceCleanup = 0;

    bool _initialized = false;

    mutable std::mutex _mutex;

    // Configuration
    static constexpr uint32 GENERATION_INTERVAL_MS   = 300000;  // Generate new tasks every 5 min
    static constexpr uint32 ASSIGNMENT_INTERVAL_MS   =  30000;  // Try to assign every 30s
    static constexpr uint32 CLEANUP_INTERVAL_MS      =  60000;  // Cleanup every 60s
    static constexpr uint32 MAX_ACTIVE_TASKS_PER_GUILD = 10;
    static constexpr uint32 MAX_TASKS_PER_BOT          =  2;
    static constexpr uint32 DEFAULT_TASK_DURATION_HOURS = 24;
};

#define sGuildTaskManager GuildTaskManager::instance()

} // namespace Playerbot
