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
#include "Player.h"
#include "QuestDef.h"
#include "Creature.h"
#include "GameObject.h"
#include "Position.h"
#include "../Core/Events/IEventHandler.h"
#include "QuestEvents.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <optional>
#include "GameTime.h"

class Player;
class Quest;
class Creature;
class GameObject;
class Group;
class Unit;

namespace Playerbot
{

/**
 * @brief Quest objective types matching TrinityCore's QuestObjectiveType enum
 *
 * These values MUST match 1:1 with TrinityCore's enum in QuestDef.h
 * to ensure proper objective parsing and handling.
 */
enum class QuestObjectiveType : uint8
{
    // ========================================================================
    // TrinityCore Standard Types (IDs 0-22)
    // ========================================================================
    MONSTER                 = 0,   // Kill creature (QUEST_OBJECTIVE_MONSTER)
    ITEM                    = 1,   // Collect item (QUEST_OBJECTIVE_ITEM)
    GAMEOBJECT              = 2,   // Use/interact with gameobject (QUEST_OBJECTIVE_GAMEOBJECT)
    TALKTO                  = 3,   // Talk to NPC (QUEST_OBJECTIVE_TALKTO)
    CURRENCY                = 4,   // Spend currency (QUEST_OBJECTIVE_CURRENCY)
    LEARNSPELL              = 5,   // Learn a spell (QUEST_OBJECTIVE_LEARNSPELL)
    MIN_REPUTATION          = 6,   // Reach minimum reputation (QUEST_OBJECTIVE_MIN_REPUTATION)
    MAX_REPUTATION          = 7,   // Reach maximum reputation (QUEST_OBJECTIVE_MAX_REPUTATION)
    MONEY                   = 8,   // Acquire gold/money (QUEST_OBJECTIVE_MONEY)
    PLAYERKILLS             = 9,   // Kill players in PvP (QUEST_OBJECTIVE_PLAYERKILLS)
    AREATRIGGER             = 10,  // Enter/exit area trigger (QUEST_OBJECTIVE_AREATRIGGER)
    WINPETBATTLEAGAINSTNPC  = 11,  // Win pet battle vs NPC (QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC)
    DEFEATBATTLEPET         = 12,  // Defeat battle pet (QUEST_OBJECTIVE_DEFEATBATTLEPET)
    WINPVPPETBATTLES        = 13,  // Win PvP pet battles (QUEST_OBJECTIVE_WINPVPPETBATTLES)
    CRITERIA_TREE           = 14,  // Complete criteria tree (QUEST_OBJECTIVE_CRITERIA_TREE)
    PROGRESS_BAR            = 15,  // Fill progress bar (QUEST_OBJECTIVE_PROGRESS_BAR)
    HAVE_CURRENCY           = 16,  // Have X currency on turn-in (QUEST_OBJECTIVE_HAVE_CURRENCY)
    OBTAIN_CURRENCY         = 17,  // Obtain X currency (QUEST_OBJECTIVE_OBTAIN_CURRENCY)
    INCREASE_REPUTATION     = 18,  // Gain X reputation (QUEST_OBJECTIVE_INCREASE_REPUTATION)
    AREA_TRIGGER_ENTER      = 19,  // Enter specific area trigger (QUEST_OBJECTIVE_AREA_TRIGGER_ENTER)
    AREA_TRIGGER_EXIT       = 20,  // Exit specific area trigger (QUEST_OBJECTIVE_AREA_TRIGGER_EXIT)
    KILL_WITH_LABEL         = 21,  // Kill creature with specific label (QUEST_OBJECTIVE_KILL_WITH_LABEL)
    UNK_1127                = 22,  // Unknown type from client 12.0.7+ (QUEST_OBJECTIVE_UNK_1127)

    // ========================================================================
    // Extended Playerbot Types (IDs 100+) - For bot-specific behaviors
    // ========================================================================
    CAST_SPELL              = 100, // Cast spell on target (playerbot-specific)
    EMOTE_AT_TARGET         = 101, // Perform emote at target (playerbot-specific)
    ESCORT_NPC              = 102, // Escort NPC to destination (playerbot-specific)
    DEFEND_AREA             = 103, // Defend area for duration (playerbot-specific)
    SURVIVE_TIME            = 104, // Survive for duration (playerbot-specific)
    WIN_BATTLEGROUND        = 105, // Win a battleground (playerbot-specific)
    COMPLETE_DUNGEON        = 106, // Complete dungeon instance (playerbot-specific)
    GAIN_EXPERIENCE         = 107, // Gain X experience points (playerbot-specific)

    // ========================================================================
    // Legacy Aliases (DO NOT USE - kept for backward compatibility)
    // ========================================================================
    KILL_CREATURE           = MONSTER,         // Alias for MONSTER
    COLLECT_ITEM            = ITEM,            // Alias for ITEM
    USE_GAMEOBJECT          = GAMEOBJECT,      // Alias for GAMEOBJECT (also direct interaction)
    TALK_TO_NPC             = TALKTO,          // Alias for TALKTO
    REACH_LOCATION          = AREATRIGGER,     // Alias for AREATRIGGER
    LEARN_SPELL             = LEARNSPELL,      // Alias for LEARNSPELL
    CUSTOM_OBJECTIVE        = 255              // Fallback for unknown types
};

enum class QuestCompletionStrategy : uint8
{
    EFFICIENT_COMPLETION    = 0,  // Complete objectives most efficiently
    SAFE_COMPLETION        = 1,  // Prioritize safety over speed
    GROUP_COORDINATION     = 2,  // Coordinate with group members
    SOLO_FOCUS            = 3,  // Complete independently
    EXPERIENCE_MAXIMIZING  = 4,  // Maximize experience gain
    SPEED_COMPLETION      = 5,  // Complete as fast as possible
    THOROUGH_EXPLORATION  = 6   // Explore thoroughly while completing
};

enum class ObjectiveStatus : uint8
{
    NOT_STARTED    = 0,
    IN_PROGRESS    = 1,
    COMPLETED      = 2,
    FAILED         = 3,
    BLOCKED        = 4,
    SKIPPED        = 5
};

struct QuestObjectiveData
{
    uint32 questId;
    uint32 objectiveIndex;
    QuestObjectiveType type;
    uint32 targetId;           // Creature ID, Item ID, etc.
    uint32 requiredCount;
    uint32 currentCount;
    Position targetLocation;
    float searchRadius;
    ObjectiveStatus status;
    uint32 lastUpdateTime;
    uint32 timeSpent;
    uint32 retryCount;
    bool isOptional;
    bool requiresGroup;
    std::string description;
    std::vector<uint32> alternativeTargets;

    QuestObjectiveData(uint32 qId, uint32 index, QuestObjectiveType t, uint32 tId, uint32 required)
        : questId(qId), objectiveIndex(index), type(t), targetId(tId), requiredCount(required)
        , currentCount(0), searchRadius(50.0f), status(ObjectiveStatus::NOT_STARTED)
        , lastUpdateTime(GameTime::GetGameTimeMS()), timeSpent(0), retryCount(0)
        , isOptional(false), requiresGroup(false) {}
};

struct QuestProgressData
{
    uint32 questId;
    uint32 botGuid;
    std::vector<QuestObjectiveData> objectives;
    QuestCompletionStrategy strategy;
    uint32 startTime;
    uint32 lastUpdateTime;
    uint32 estimatedCompletionTime;
    float completionPercentage;
    bool isStuck;
    uint32 stuckTime;
    uint32 consecutiveFailures;
    Position lastKnownLocation;
    std::vector<std::string> completionLog;
    bool requiresTurnIn;
    uint32 questGiverGuid;
    Position questGiverLocation;

    QuestProgressData(uint32 qId, uint32 bGuid) : questId(qId), botGuid(bGuid)
        , strategy(QuestCompletionStrategy::EFFICIENT_COMPLETION), startTime(GameTime::GetGameTimeMS())
        , lastUpdateTime(GameTime::GetGameTimeMS()), estimatedCompletionTime(1200000) // 20 minutes
        , completionPercentage(0.0f), isStuck(false), stuckTime(0)
        , consecutiveFailures(0), requiresTurnIn(true), questGiverGuid(0) {}
};

/**
 * @brief Cached quest progress from EventBus (real-time from packet sniffer)
 *
 * This cache is populated by QUEST_CREDIT_ADDED events from the packet sniffer,
 * providing real-time progress updates without expensive DB queries.
 *
 * Performance: <5μs cache hit vs 100-500ms DB poll
 */
struct CachedQuestProgress
{
    float progress;                 // 0.0 to 1.0
    uint32 lastUpdateTime;          // GameTime::GetGameTimeMS()
    bool isValid;                   // True if cache entry is usable
    uint32 questId;                 // Quest this progress belongs to

    CachedQuestProgress()
        : progress(0.0f), lastUpdateTime(0), isValid(false), questId(0) {}

    CachedQuestProgress(float prog, uint32 updateTime, uint32 qId)
        : progress(prog), lastUpdateTime(updateTime), isValid(true), questId(qId) {}

    /**
     * @brief Check if cache entry is still valid (5 second TTL)
     * @return true if entry was updated within the last 5 seconds
     */
    bool IsFresh() const
    {
        if (!isValid)
            return false;
        return (GameTime::GetGameTimeMS() - lastUpdateTime) < 5000;
    }
};

/**
 * @brief Quest POI data cached from QUEST_POI_RECEIVED events
 *
 * Contains server-provided objective locations for navigation optimization.
 */
struct CachedQuestPOI
{
    uint32 questId;
    std::vector<Position> objectiveLocations;
    uint32 lastUpdateTime;

    CachedQuestPOI() : questId(0), lastUpdateTime(0) {}
};

/**
 * @brief Quest completion manager with EventBus integration
 *
 * Handles quest progress tracking, objective execution, and turn-in management.
 * Integrates with QuestEventBus for real-time progress updates from packet sniffer.
 *
 * **EventBus Integration (via Callback Subscription):**
 * - Subscribes to QUEST_CREDIT_ADDED for real-time objective progress
 * - Subscribes to QUEST_COMPLETED for automatic turn-in triggering
 * - Subscribes to QUEST_POI_RECEIVED for navigation optimization
 *
 * **Performance:**
 * - <5μs progress queries (EventBus cache hit)
 * - <0.01% CPU per bot for quest tracking
 * - Zero DB polling for progress tracking
 */
class TC_GAME_API QuestCompletion final 
{
public:
    explicit QuestCompletion(Player* bot);
    ~QuestCompletion();
    QuestCompletion(QuestCompletion const&) = delete;
    QuestCompletion& operator=(QuestCompletion const&) = delete;

    // ========================================================================
    // EVENTBUS CALLBACK HANDLER
    // ========================================================================

    /**
     * @brief Handle quest events from EventBus (callback subscription)
     *
     * Called by QuestEventBus during ProcessEvents() via callback.
     * Dispatches to specific event handlers based on event type.
     *
     * @param event Quest event from packet sniffer
     */
    void HandleQuestEvent(QuestEvent const& event);

    // ========================================================================
    // CORE QUEST COMPLETION MANAGEMENT
    // ========================================================================
    bool StartQuestCompletion(uint32 questId, Player* bot);
    void UpdateQuestProgress(Player* bot);
    void CompleteQuest(uint32 questId, Player* bot);
    bool TurnInQuest(uint32 questId, Player* bot);

    // Objective tracking and execution
    void TrackQuestObjectives(Player* bot);
    void ExecuteObjective(Player* bot, QuestObjectiveData& objective);
    void UpdateObjectiveProgress(Player* bot, uint32 questId, uint32 objectiveIndex);
    bool IsObjectiveComplete(const QuestObjectiveData& objective);

    // Objective-specific handlers - Legacy (mapped to TrinityCore types)
    void HandleKillObjective(Player* bot, QuestObjectiveData& objective);
    void HandleCollectObjective(Player* bot, QuestObjectiveData& objective);
    void HandleTalkToNpcObjective(Player* bot, QuestObjectiveData& objective);
    void HandleLocationObjective(Player* bot, QuestObjectiveData& objective);
    void HandleGameObjectObjective(Player* bot, QuestObjectiveData& objective);
    void HandleSpellCastObjective(Player* bot, QuestObjectiveData& objective);
    void HandleEmoteObjective(Player* bot, QuestObjectiveData& objective);
    void HandleEscortObjective(Player* bot, QuestObjectiveData& objective);

    // ========================================================================
    // NEW TRINITYCORE OBJECTIVE HANDLERS (Phase 1 Quest System Completion)
    // ========================================================================

    /**
     * @brief Handle currency spending objectives (QUEST_OBJECTIVE_CURRENCY)
     * Bot needs to spend a specific amount of currency
     */
    void HandleCurrencyObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle spell learning objectives (QUEST_OBJECTIVE_LEARNSPELL)
     * Bot needs to learn a specific spell from a trainer
     */
    void HandleLearnSpellObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle minimum reputation objectives (QUEST_OBJECTIVE_MIN_REPUTATION)
     * Bot needs to reach a minimum reputation level with a faction
     */
    void HandleMinReputationObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle maximum reputation objectives (QUEST_OBJECTIVE_MAX_REPUTATION)
     * Bot needs to stay below a maximum reputation level with a faction
     */
    void HandleMaxReputationObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle money/gold objectives (QUEST_OBJECTIVE_MONEY)
     * Bot needs to have or acquire a specific amount of gold
     */
    void HandleMoneyObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle player kill objectives (QUEST_OBJECTIVE_PLAYERKILLS)
     * Bot needs to kill other players in PvP combat
     */
    void HandlePlayerKillsObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle area trigger objectives (QUEST_OBJECTIVE_AREATRIGGER)
     * Bot needs to enter or pass through a specific area trigger
     */
    void HandleAreaTriggerObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle pet battle vs NPC objectives (QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC)
     * Bot needs to win a pet battle against a specific NPC
     */
    void HandlePetBattleNPCObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle defeat battle pet objectives (QUEST_OBJECTIVE_DEFEATBATTLEPET)
     * Bot needs to defeat a specific wild battle pet
     */
    void HandleDefeatBattlePetObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle PvP pet battle objectives (QUEST_OBJECTIVE_WINPVPPETBATTLES)
     * Bot needs to win PvP pet battles against other players
     */
    void HandlePvPPetBattlesObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle criteria tree objectives (QUEST_OBJECTIVE_CRITERIA_TREE)
     * Bot needs to complete achievement-like criteria
     */
    void HandleCriteriaTreeObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle progress bar objectives (QUEST_OBJECTIVE_PROGRESS_BAR)
     * Bot needs to fill a progress bar through various actions
     */
    void HandleProgressBarObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle have currency objectives (QUEST_OBJECTIVE_HAVE_CURRENCY)
     * Bot needs to have X currency at turn-in (not consumed)
     */
    void HandleHaveCurrencyObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle obtain currency objectives (QUEST_OBJECTIVE_OBTAIN_CURRENCY)
     * Bot needs to obtain X currency after accepting quest (not kept)
     */
    void HandleObtainCurrencyObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle increase reputation objectives (QUEST_OBJECTIVE_INCREASE_REPUTATION)
     * Bot needs to gain X reputation with a specific faction
     */
    void HandleIncreaseReputationObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle area trigger enter objectives (QUEST_OBJECTIVE_AREA_TRIGGER_ENTER)
     * Bot needs to enter a specific area trigger zone
     */
    void HandleAreaTriggerEnterObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle area trigger exit objectives (QUEST_OBJECTIVE_AREA_TRIGGER_EXIT)
     * Bot needs to exit a specific area trigger zone
     */
    void HandleAreaTriggerExitObjective(Player* bot, QuestObjectiveData& objective);

    /**
     * @brief Handle kill with label objectives (QUEST_OBJECTIVE_KILL_WITH_LABEL)
     * Bot needs to kill creatures with a specific label/tag
     */
    void HandleKillWithLabelObjective(Player* bot, QuestObjectiveData& objective);

    // ========================================================================
    // HELPER METHODS FOR NEW OBJECTIVE HANDLERS
    // ========================================================================

    /**
     * @brief Find a trainer that can teach a specific spell
     * @param bot The bot player
     * @param spellId The spell ID to learn
     * @return Trainer creature or nullptr if not found
     */
    Creature* FindTrainerForSpell(Player* bot, uint32 spellId);

    /**
     * @brief Start PvP activity for player kills objective
     * @param bot The bot player
     */
    void StartPvPActivity(Player* bot);

    /**
     * @brief Start dungeon activity for dungeon objectives
     * @param bot The bot player
     */
    void StartDungeonActivity(Player* bot);

    /**
     * @brief Queue bot for a specific battleground
     * @param bot The bot player
     * @param bgTypeId Battleground type ID
     */
    void QueueForBattleground(Player* bot, uint32 bgTypeId);

    /**
     * @brief Find area trigger position by ID
     * @param areaTriggerIdOrObjectId Area trigger ID
     * @return Position of the area trigger
     */
    Position FindAreaTriggerPosition(uint32 areaTriggerIdOrObjectId);

    /**
     * @brief Check if bot has enough currency
     * @param bot The bot player
     * @param currencyId Currency type ID
     * @param amount Required amount
     * @return True if bot has enough
     */
    bool HasEnoughCurrency(Player* bot, uint32 currencyId, uint32 amount);

    /**
     * @brief Find best reputation source for a faction
     * @param bot The bot player
     * @param factionId Faction ID
     * @return Quest ID or creature ID that provides reputation
     */
    uint32 FindReputationSource(Player* bot, uint32 factionId);

    // Navigation and pathfinding
    void NavigateToObjective(Player* bot, const QuestObjectiveData& objective);
    bool FindObjectiveTarget(Player* bot, QuestObjectiveData& objective);
    std::vector<Position> GetObjectiveLocations(const QuestObjectiveData& objective);
    Position GetOptimalObjectivePosition(Player* bot, const QuestObjectiveData& objective);

    // Group coordination for quest completion
    void CoordinateGroupQuestCompletion(Group* group, uint32 questId);
    void ShareObjectiveProgress(Group* group, uint32 questId);
    void SynchronizeGroupObjectives(Group* group, uint32 questId);
    void HandleGroupObjectiveConflict(Group* group, uint32 questId, uint32 objectiveIndex);

    // Quest completion optimization
    void OptimizeQuestCompletionOrder(Player* bot);
    void OptimizeObjectiveSequence(Player* bot, uint32 questId);
    void FindEfficientCompletionPath(Player* bot, const std::vector<uint32>& questIds);
    void MinimizeTravelTime(Player* bot, const std::vector<QuestObjectiveData>& objectives);

    // Stuck detection and recovery
    void DetectStuckState(Player* bot, uint32 questId);
    void HandleStuckObjective(Player* bot, QuestObjectiveData& objective);
    void RecoverFromStuckState(Player* bot, uint32 questId);
    void SkipProblematicObjective(Player* bot, QuestObjectiveData& objective);

    // Quest turn-in management
    void ProcessQuestTurnIn(Player* bot, uint32 questId);
    bool FindQuestTurnInNpc(Player* bot, uint32 questId);
    void HandleQuestRewardSelection(Player* bot, uint32 questId);
    void CompleteQuestDialog(Player* bot, uint32 questId);

    // Performance monitoring
    struct QuestCompletionMetrics
    {
        std::atomic<uint32> questsStarted{0};
        std::atomic<uint32> questsCompleted{0};
        std::atomic<uint32> questsFailed{0};
        std::atomic<uint32> questsAbandoned{0};
        std::atomic<uint32> activeQuests{0};
        std::atomic<uint32> objectivesCompleted{0};
        std::atomic<uint32> stuckInstances{0};
        std::atomic<float> averageCompletionTime{1200000.0f}; // 20 minutes
        std::atomic<float> completionSuccessRate{0.85f};
        std::atomic<float> objectiveEfficiency{0.9f};
        std::atomic<uint32> totalDistanceTraveled{0};
        std::chrono::steady_clock::time_point lastUpdate;

        void Reset()
        {
            questsStarted = 0; questsCompleted = 0; questsFailed = 0;
            questsAbandoned = 0; activeQuests = 0;
            objectivesCompleted = 0; stuckInstances = 0; averageCompletionTime = 1200000.0f;
            completionSuccessRate = 0.85f; objectiveEfficiency = 0.9f;
            totalDistanceTraveled = 0;
            lastUpdate = std::chrono::steady_clock::now();
        }

        float GetCompletionRate() const
        {
            uint32 started = questsStarted.load();
            uint32 completed = questsCompleted.load();
            return started > 0 ? (float)completed / started : 0.0f;
        }

        // Delete copy operations (atomics are not copyable)
        QuestCompletionMetrics() = default;
        QuestCompletionMetrics(QuestCompletionMetrics const&) = delete;
        QuestCompletionMetrics& operator=(QuestCompletionMetrics const&) = delete;

        // Snapshot structure for returning metrics
        struct Snapshot {
            uint32 questsStarted;
            uint32 questsCompleted;
            uint32 questsFailed;
            uint32 objectivesCompleted;
            uint32 stuckInstances;
            float averageCompletionTime;
            float completionSuccessRate;
            float objectiveEfficiency;
            uint32 totalDistanceTraveled;
            std::chrono::steady_clock::time_point lastUpdate;

            float GetCompletionRate() const
            {
                return questsStarted > 0 ? (float)questsCompleted / questsStarted : 0.0f;
            }
        };

        // Create a snapshot of current metrics
        Snapshot CreateSnapshot() const
        {
            Snapshot snapshot;
            snapshot.questsStarted = questsStarted.load();
            snapshot.questsCompleted = questsCompleted.load();
            snapshot.questsFailed = questsFailed.load();
            snapshot.objectivesCompleted = objectivesCompleted.load();
            snapshot.stuckInstances = stuckInstances.load();
            snapshot.averageCompletionTime = averageCompletionTime.load();
            snapshot.completionSuccessRate = completionSuccessRate.load();
            snapshot.objectiveEfficiency = objectiveEfficiency.load();
            snapshot.totalDistanceTraveled = totalDistanceTraveled.load();
            snapshot.lastUpdate = lastUpdate;
            return snapshot;
        }
    };

    // Type alias for metrics snapshot
    using QuestCompletionMetricsSnapshot = QuestCompletionMetrics::Snapshot;

    QuestCompletionMetricsSnapshot GetBotCompletionMetrics(uint32 botGuid);
    QuestCompletionMetricsSnapshot GetGlobalCompletionMetrics();

    // Quest data analysis
    std::vector<uint32> GetActiveQuests(Player* bot);
    std::vector<uint32> GetCompletableQuests(Player* bot);
    uint32 GetHighestPriorityQuest(Player* bot);
    float CalculateQuestProgress(uint32 questId, Player* bot);

    // Configuration and settings
    void SetQuestCompletionStrategy(uint32 botGuid, QuestCompletionStrategy strategy);
    QuestCompletionStrategy GetQuestCompletionStrategy(uint32 botGuid);
    void SetMaxConcurrentQuests(uint32 botGuid, uint32 maxQuests);
    void EnableGroupCoordination(uint32 botGuid, bool enable);

    // Advanced quest completion features
    void HandleDungeonQuests(Player* bot, uint32 dungeonId);
    void HandlePvPQuests(Player* bot, uint32 battlegroundId);
    void HandleSeasonalQuests(Player* bot);
    void HandleDailyQuests(Player* bot);

    // Error handling and recovery
    void HandleQuestCompletionError(Player* bot, uint32 questId, const std::string& error);
    void RecoverFromCompletionFailure(Player* bot, uint32 questId);
    void AbandonUncompletableQuest(Player* bot, uint32 questId);
    void DiagnoseCompletionIssues(Player* bot, uint32 questId);

    // Update and maintenance
    void Update(uint32 diff);
    void UpdateBotQuestCompletion(Player* bot, uint32 diff);
    void CleanupCompletedQuests();
    void ValidateQuestStates();

    // ========================================================================
    // LIFECYCLE HOOK INTEGRATION (Phase 0)
    // ========================================================================

    /**
     * @brief Pause quest completion (called on death via hook)
     * @param botGuid Bot's GUID counter
     */
    void PauseQuestCompletion(uint32 botGuid);

    /**
     * @brief Resume quest completion (called on resurrection via hook)
     * @param botGuid Bot's GUID counter
     */
    void ResumeQuestCompletion(uint32 botGuid);

private:
    Player* _bot;

    // ========================================================================
    // EVENTBUS EVENT HANDLERS (Called by HandleEvent)
    // ========================================================================

    /**
     * @brief Handle QUEST_CREDIT_ADDED event - real-time objective progress
     * @param event Event containing questId, objectiveId, and exact count from packet
     */
    void OnQuestCreditAdded(QuestEvent const& event);

    /**
     * @brief Handle QUEST_COMPLETED event - trigger auto turn-in
     * @param event Event containing questId of completed quest
     */
    void OnQuestCompleted(QuestEvent const& event);

    /**
     * @brief Handle QUEST_FAILED event - trigger recovery
     * @param event Event containing questId of failed quest
     */
    void OnQuestFailed(QuestEvent const& event);

    /**
     * @brief Handle QUEST_POI_RECEIVED event - cache objective locations
     * @param event Event containing quest POI data from server
     */
    void OnQuestPOIReceived(QuestEvent const& event);

    /**
     * @brief Handle QUEST_OFFER_REWARD event - auto-trigger turn-in flow
     * @param event Event indicating quest reward is being offered
     */
    void OnQuestOfferReward(QuestEvent const& event);

    // ========================================================================
    // EVENTBUS PROGRESS CACHE (Real-time from packet sniffer)
    // ========================================================================

    /**
     * @brief Get cached progress for a quest (populated by EventBus events)
     * @param botGuid Bot's GUID
     * @param questId Quest ID to look up
     * @return Optional cached progress if available and fresh
     */
    std::optional<CachedQuestProgress> GetCachedQuestProgress(ObjectGuid botGuid, uint32 questId) const;

    /**
     * @brief Update progress cache with new data
     * @param botGuid Bot's GUID
     * @param questId Quest ID
     * @param progress Progress value (0.0 to 1.0)
     */
    void UpdateProgressCache(ObjectGuid botGuid, uint32 questId, float progress);

    /**
     * @brief Cache POI data for a quest
     * @param botGuid Bot's GUID
     * @param questId Quest ID
     */
    void CacheQuestPOIData(ObjectGuid botGuid, uint32 questId);

    /**
     * @brief Invalidate cache for a quest (on completion or abandonment)
     * @param botGuid Bot's GUID
     * @param questId Quest ID
     */
    void InvalidateProgressCache(ObjectGuid botGuid, uint32 questId);

    /**
     * @brief Get cached POI data for a quest
     * @param botGuid Bot's GUID
     * @param questId Quest ID
     * @return Optional cached POI data if available
     */
    std::optional<CachedQuestPOI> GetCachedQuestPOI(ObjectGuid botGuid, uint32 questId) const;

    // Progress cache: botGuidCounter -> questId -> cached progress
    mutable std::shared_mutex _progressCacheMutex;
    std::unordered_map<uint32, std::unordered_map<uint32, CachedQuestProgress>> _questProgressCache;

    // POI cache: botGuidCounter -> questId -> cached POI data
    mutable std::shared_mutex _poiCacheMutex;
    std::unordered_map<uint32, std::unordered_map<uint32, CachedQuestPOI>> _questPOICache;

    // Paused bots (due to death)
    // CRITICAL FIX: Changed to shared_mutex for read-heavy access pattern
    // _pausedBots is checked on EVERY quest event (read), but rarely modified (write)
    // With 100+ bots, std::mutex caused severe contention
    mutable std::shared_mutex _pausedBotsMutex;
    std::unordered_set<uint32> _pausedBots;

    // EventBus callback subscription ID (for unsubscription in destructor)
    uint32 _eventBusSubscriptionId{0};

    // ========================================================================
    // CORE DATA STRUCTURES
    // ========================================================================

    std::unordered_map<uint32, std::vector<QuestProgressData>> _botQuestProgress; // botGuid -> quests
    std::unordered_map<uint32, QuestCompletionStrategy> _botStrategies;
    std::unordered_map<uint32, QuestCompletionMetrics> _botMetrics;

    // Objective execution state
    std::unordered_map<uint32, uint32> _botCurrentObjective; // botGuid -> objectiveIndex
    std::unordered_map<uint32, uint32> _botLastObjectiveUpdate; // botGuid -> timestamp
    std::unordered_set<uint32> _botsInQuestMode; // bots actively completing quests

    // Group coordination data
    std::unordered_map<uint32, std::vector<uint32>> _groupQuestSharing; // groupId -> questIds
    std::unordered_map<uint32, std::unordered_map<uint32, uint32>> _groupObjectiveSync; // groupId -> questId -> syncTime

    // Pending reward selections: botGuidCounter -> (questId, rewardIndex)
    std::unordered_map<uint32, std::pair<uint32, uint32>> _pendingRewardSelections;

    // Group coordination mutex and data structures
    mutable std::mutex _groupCoordinationMutex;
    std::unordered_map<uint32, std::unordered_map<uint32, uint32>> _groupLootPriority; // groupId -> itemId -> playerGuidCounter
    struct GroupQuestCoordinationData
    {
        uint32 lastUpdate;
        uint32 memberCount;
        bool isCoordinated;
    };
    std::unordered_map<uint32, std::unordered_map<uint32, GroupQuestCoordinationData>> _groupQuestCoordination;

    // Quest order optimization
    // CRITICAL FIX: Changed to shared_mutex - quest order is read frequently (every update cycle)
    // but only written during optimization (rare). std::mutex caused ThreadPool delays.
    mutable std::shared_mutex _questOrderMutex;
    std::unordered_map<uint32, std::vector<uint32>> _botQuestOrder; // botGuidCounter -> ordered questIds

    // Objective order optimization
    // CRITICAL FIX: Changed to shared_mutex - same read-heavy pattern as quest order
    mutable std::shared_mutex _objectiveOrderMutex;
    std::unordered_map<uint32, std::unordered_map<uint32, std::vector<uint32>>> _botObjectiveOrder; // bot -> quest -> ordered objectives

    // Path optimization
    mutable std::mutex _pathMutex;
    std::unordered_map<uint32, std::vector<Position>> _botQuestPaths; // botGuidCounter -> path

    // Configuration: max concurrent quests per bot
    mutable std::mutex _configMutex;
    std::unordered_map<uint32, uint32> _botMaxConcurrentQuests; // botGuidCounter -> maxQuests
    std::unordered_set<uint32> _groupCoordinationEnabled; // questIds with group coordination enabled

    // Error tracking
    struct QuestErrorInfo
    {
        uint32 questId;
        std::string error;
        uint32 timestamp;
        uint32 recoveryAttempts;
    };
    mutable std::mutex _errorMutex;
    std::unordered_map<uint32, std::vector<QuestErrorInfo>> _botQuestErrors; // botGuidCounter -> errors

    // Quest diagnostics
    struct QuestDiagnostics
    {
        uint32 questId;
        std::vector<std::string> issues;
        uint32 lastDiagnosticTime;
        bool isBlocked;
        std::string blockReason;
    };
    std::unordered_map<uint32, std::unordered_map<uint32, QuestDiagnostics>> _questDiagnostics; // bot -> quest -> diagnostics

    // Performance tracking
    QuestCompletionMetrics _globalMetrics;

    // Helper functions
    void InitializeQuestProgress(Player* bot, uint32 questId);
    void ParseQuestObjectives(QuestProgressData& progress, const Quest* quest);
    void UpdateQuestObjectiveFromProgress(QuestObjectiveData& objective, Player* bot);
    bool CanExecuteObjective(Player* bot, const QuestObjectiveData& objective);
    void MoveToObjectiveTarget(Player* bot, const QuestObjectiveData& objective);
    bool IsInObjectiveRange(Player* bot, const QuestObjectiveData& objective);

    // Objective-specific implementations
    bool FindKillTarget(Player* bot, QuestObjectiveData& objective);
    bool FindCollectibleItem(Player* bot, QuestObjectiveData& objective);
    bool FindNpcTarget(Player* bot, QuestObjectiveData& objective);
    bool FindGameObjectTarget(Player* bot, QuestObjectiveData& objective);
    void InteractWithTarget(Player* bot, const QuestObjectiveData& objective, uint32 targetGuid);

    // Combat integration
    void HandleQuestCombat(Player* bot, const QuestObjectiveData& objective, Unit* target);
    void PrioritizeQuestTargets(Player* bot, const std::vector<Unit*>& enemies);
    bool ShouldEngageQuestTarget(Player* bot, Unit* target, const QuestObjectiveData& objective);

    // Pathfinding and navigation
    std::vector<Position> GenerateObjectivePath(Player* bot, const QuestObjectiveData& objective);
    Position FindNearestObjectiveLocation(Player* bot, const QuestObjectiveData& objective);
    bool ValidateObjectivePosition(const Position& position, const QuestObjectiveData& objective);

    // Group coordination helpers
    void BroadcastObjectiveProgress(Group* group, uint32 questId, uint32 objectiveIndex, Player* bot);
    void SynchronizeObjectiveStates(Group* group, uint32 questId);
    void CoordinateObjectiveExecution(Group* group, uint32 questId, uint32 objectiveIndex);

    // Strategy implementations
    void ExecuteEfficientStrategy(Player* bot, QuestProgressData& progress);
    void ExecuteSafeStrategy(Player* bot, QuestProgressData& progress);
    void ExecuteGroupStrategy(Player* bot, QuestProgressData& progress);
    void ExecuteSoloStrategy(Player* bot, QuestProgressData& progress);
    void ExecuteExperienceStrategy(Player* bot, QuestProgressData& progress);
    void ExecuteSpeedStrategy(Player* bot, QuestProgressData& progress);
    void ExecuteExplorationStrategy(Player* bot, QuestProgressData& progress);

    // Performance optimization
    void OptimizeQuestCompletionPerformance(Player* bot);
    void CacheObjectiveTargets(Player* bot);
    void PreloadQuestData(Player* bot);
    void BatchObjectiveUpdates();

    // Error handling
    void LogCompletionError(Player* bot, uint32 questId, const std::string& error);
    void HandleObjectiveTimeout(Player* bot, QuestObjectiveData& objective);
    void RecoverFromObjectiveFailure(Player* bot, QuestObjectiveData& objective);

    // Stuck detection helpers
    Position FindAlternativeSearchArea(Player* player, QuestObjectiveData const& objective);
    void DiagnoseObjectiveIssue(Player* player, QuestObjectiveData const& objective);
    static std::string GetObjectiveStatusName(ObjectiveStatus status);

    // Turn-in navigation helpers
    void NavigateToTurnInNpc(Player* player, Creature* npc);
    void NavigateToTurnInGO(Player* player, GameObject* go);

    // Item evaluation helpers
    bool CanBotUseItem(Player* player, ItemTemplate const* item);
    float EvaluateItemUpgrade(Player* player, ItemTemplate const* item);
    float EvaluateItemStats(Player* player, ItemTemplate const* item);

    // Priority calculation
    int32 CalculateQuestPriorityInternal(Player* player, Quest const* quest);

    // Constants
    static constexpr uint32 OBJECTIVE_UPDATE_INTERVAL = 2000; // 2 seconds
    static constexpr uint32 STUCK_DETECTION_TIME = 60000; // 1 minute
    static constexpr uint32 MAX_OBJECTIVE_RETRIES = 5;
    static constexpr float OBJECTIVE_COMPLETION_RADIUS = 10.0f;
    static constexpr uint32 QUEST_COMPLETION_TIMEOUT = 3600000; // 1 hour
    static constexpr uint32 GROUP_SYNC_INTERVAL = 5000; // 5 seconds
    static constexpr float MIN_PROGRESS_THRESHOLD = 0.01f; // 1% progress
    static constexpr uint32 MAX_CONCURRENT_OBJECTIVES = 3;
    static constexpr uint32 OBJECTIVE_SEARCH_RADIUS = 100; // 100 yards
    static constexpr uint32 TARGET_INTERACTION_TIMEOUT = 10000; // 10 seconds
};

} // namespace Playerbot