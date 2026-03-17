/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_PRIORITY_MANAGER_H
#define BOT_PRIORITY_MANAGER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <chrono>
#include <mutex>

class Player;

namespace Playerbot {

/**
 * ENTERPRISE-GRADE PRIORITY SYSTEM
 *
 * Manages bot update priorities for optimal performance at scale (5000+ bots)
 *
 * Priority Levels:
 * - EMERGENCY: Critical situations (stuck, error recovery) - every tick
 * - HIGH: Combat, group content - every tick
 * - MEDIUM: Active movement, questing - every 10 ticks
 * - LOW: Idle, resting, traveling - every 50 ticks
 * - SUSPENDED: Temporarily disabled for load shedding
 *
 * Performance Target: 181 bots/tick × 0.8ms = 145ms per tick
 */
enum class BotPriority : uint8
{
    EMERGENCY = 0,  // Critical situations requiring immediate attention
    HIGH = 1,       // Combat, group content, immediate response needed
    MEDIUM = 2,     // Active but non-critical (questing, active movement)
    LOW = 3,        // Idle, resting, background activities
    SUSPENDED = 4   // Temporarily suspended during load shedding
};

/**
 * Comprehensive metrics for each bot's update performance
 */
struct BotUpdateMetrics
{
    // Timing metrics
    uint32 lastUpdateTime{0};           // Game time of last update (milliseconds)
    uint32 lastUpdateDuration{0};       // How long the last update took (microseconds)
    uint32 averageUpdateDuration{0};    // Rolling average update time (microseconds)
    uint32 maxUpdateDuration{0};        // Peak update time (microseconds)

    // Update frequency
    uint32 updateCount{0};              // Total updates performed
    uint32 skippedUpdates{0};           // Updates skipped due to priority scheduling
    uint32 ticksSinceLastUpdate{0};     // For stall detection

    // Priority management
    BotPriority currentPriority{BotPriority::LOW};
    uint32 priorityChangeTime{0};       // Game time when priority last changed
    uint32 timeInCurrentPriority{0};    // Duration in current priority level

    // State tracking for auto-promotion/demotion
    bool wasInCombat{false};
    bool wasInGroup{false};
    bool wasMoving{false};
    uint32 idleStartTime{0};            // When bot became idle

    // Health monitoring
    uint32 errorCount{0};               // Update errors encountered
    uint32 lastErrorTime{0};            // Time of last error
    bool isStalled{false};              // Detected as not updating
};

/**
 * Thread-safe priority management for all bots
 *
 * Responsibilities:
 * - Track priority levels for all bots
 * - Auto-adjust priorities based on bot state
 * - Provide priority-based scheduling information
 * - Collect and aggregate performance metrics
 * - Detect and handle anomalies (stalls, errors)
 */
class TC_GAME_API BotPriorityManager final
{
public:
    static BotPriorityManager* instance()
    {
        static BotPriorityManager instance;
        return &instance;
    }

    // Initialization
    bool Initialize();
    void Shutdown();

    // Priority management
    void SetPriority(ObjectGuid botGuid, BotPriority priority);
    BotPriority GetPriority(ObjectGuid botGuid) const;
    void UpdatePriorityForBot(Player* bot, uint32 currentTime);

    // Automatic priority adjustment based on bot state
    void AutoAdjustPriority(Player* bot, uint32 currentTime);

    // Priority scheduling - determines if bot should update this tick
    bool ShouldUpdateThisTick(ObjectGuid botGuid, uint32 currentTick) const;
    uint32 GetUpdateInterval(BotPriority priority) const;

    // Metrics tracking
    void RecordUpdateStart(ObjectGuid botGuid, uint32 currentTime);
    void RecordUpdateEnd(ObjectGuid botGuid, uint32 durationMicros);
    void RecordUpdateSkipped(ObjectGuid botGuid);
    void RecordUpdateError(ObjectGuid botGuid, uint32 currentTime);

    // Metrics retrieval
    BotUpdateMetrics const* GetMetrics(ObjectGuid botGuid) const;
    uint32 GetBotCountByPriority(BotPriority priority) const;
    uint32 GetEstimatedBotsThisTick(uint32 currentTick) const;

    // Priority distribution for load balancing
    void GetPriorityDistribution(uint32& emergency, uint32& high, uint32& medium, uint32& low, uint32& suspended) const;

    // Load shedding - suspend low-priority bots under heavy load
    void SuspendLowPriorityBots(uint32 targetCount);
    void ResumeSuspendedBots(uint32 targetCount);

    // Health monitoring
    void DetectStalledBots(uint32 currentTime, uint32 stallThresholdMs);
    ::std::vector<ObjectGuid> GetStalledBots() const;

    // Administrative
    void RemoveBot(ObjectGuid botGuid);
    void Clear();

    // Configuration
    void SetMaxBotsPerPriority(BotPriority priority, uint32 maxBots);
    void SetUpdateInterval(BotPriority priority, uint32 intervalTicks);
    uint32 GetMaxBotsPerPriority(BotPriority priority) const;

    // Statistics and logging
    void LogPriorityDistribution() const;
    void LogPerformanceStatistics() const;

private:
    BotPriorityManager() = default;
    ~BotPriorityManager() = default;
    BotPriorityManager(const BotPriorityManager&) = delete;
    BotPriorityManager& operator=(const BotPriorityManager&) = delete;

    // Priority determination logic
    BotPriority DeterminePriority(Player* bot) const;
    bool IsInCriticalState(Player* bot) const;
    bool IsInHighPriorityActivity(Player* bot) const;
    bool IsInMediumPriorityActivity(Player* bot) const;

    // Metrics storage
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _metricsMutex;
    ::std::unordered_map<ObjectGuid, BotUpdateMetrics> _botMetrics;

    // Configuration (loaded from playerbots.conf)
    struct PriorityConfig
    {
        uint32 maxBotsPerTick{50};
        uint32 updateIntervalTicks{1};
    };

    ::std::array<PriorityConfig, 5> _priorityConfigs;

    // Default configuration (optimal for 5000 bots)
    void LoadDefaultConfiguration();

    // Initialization state
    ::std::atomic<bool> _initialized{false};
};

// Global instance accessor
#define sBotPriorityMgr BotPriorityManager::instance()

} // namespace Playerbot

#endif // BOT_PRIORITY_MANAGER_H
