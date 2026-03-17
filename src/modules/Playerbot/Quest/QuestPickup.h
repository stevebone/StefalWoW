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
#include "SharedDefines.h"
#include "Player.h"
#include "QuestDef.h"
#include "Creature.h"
#include "GameObject.h"
#include "Position.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>
#include "GameTime.h"

class Player;
class Quest;
class Creature;
class GameObject;
class Group;

namespace Playerbot
{

enum class QuestAcceptanceStrategy : uint8
{
    ACCEPT_ALL              = 0,  // Accept all available quests
    LEVEL_APPROPRIATE       = 1,  // Only accept level-appropriate quests
    ZONE_FOCUSED            = 2,  // Accept quests in current zone
    CHAIN_COMPLETION        = 3,  // Complete quest chains systematically
    EXPERIENCE_OPTIMAL      = 4,  // Maximize experience gain
    REPUTATION_FOCUSED      = 5,  // Focus on faction reputation
    GEAR_UPGRADE_FOCUSED    = 6,  // Prioritize quests with gear rewards
    GROUP_COORDINATION      = 7,  // Coordinate with group members
    SELECTIVE_QUALITY       = 8   // Only accept worthwhile quests
};

enum class QuestGiverType : uint8
{
    NPC_CREATURE    = 0,
    GAME_OBJECT     = 1,
    ITEM_USE        = 2,
    SPELL_EFFECT    = 3,
    AREA_TRIGGER    = 4,
    AUTO_COMPLETE   = 5
};

/**
 * @brief Metrics for quest pickup operations
 */
struct QuestPickupMetrics
{
    uint32 questsPickedUp{0};
    uint32 questsAttempted{0};
    uint32 pickupFailures{0};
    uint32 cacheHits{0};
    uint32 cacheMisses{0};
    uint32 npcInteractions{0};
    uint32 averagePickupTimeMs{0};
    uint32 lastPickupTime{0};
    uint32 pickupAttempts{0};
    uint32 successfulPickups{0};
    uint32 questsRejected{0};
    float averagePickupTime{0.0f};
    float questPickupEfficiency{0.0f};
    uint32 questGiversVisited{0};

    QuestPickupMetrics() = default;

    void Reset()
    {
        questsPickedUp = 0;
        questsAttempted = 0;
        pickupFailures = 0;
        cacheHits = 0;
        cacheMisses = 0;
        npcInteractions = 0;
        averagePickupTimeMs = 0;
        lastPickupTime = 0;
        pickupAttempts = 0;
        successfulPickups = 0;
        questsRejected = 0;
        averagePickupTime = 0.0f;
        questPickupEfficiency = 0.0f;
        questGiversVisited = 0;
    }

    float GetSuccessRate() const
    {
        return questsAttempted > 0 ? static_cast<float>(questsPickedUp) / questsAttempted * 100.0f : 0.0f;
    }
};

enum class QuestEligibility : uint8
{
    ELIGIBLE        = 0,
    LEVEL_TOO_LOW   = 1,
    LEVEL_TOO_HIGH  = 2,
    MISSING_PREREQ  = 3,
    ALREADY_HAVE    = 4,
    ALREADY_DONE    = 5,
    QUEST_LOG_FULL  = 6,
    FACTION_LOCKED  = 7,
    CLASS_LOCKED    = 8,
    RACE_LOCKED     = 9,
    SKILL_REQUIRED  = 10,
    ITEM_REQUIRED   = 11,
    NOT_AVAILABLE   = 12
};

struct QuestGiverInfo
{
    uint32 giverGuid;
    QuestGiverType type;
    Position location;
    uint32 zoneId;
    uint32 areaId;
    std::vector<uint32> availableQuests;
    uint32 lastInteractionTime;
    bool isActive;
    bool requiresMovement;
    float interactionRange;

    QuestGiverInfo() : giverGuid(0), type(QuestGiverType::NPC_CREATURE)
        , location(0.0f, 0.0f, 0.0f, 0.0f), zoneId(0), areaId(0)
        , lastInteractionTime(0), isActive(true), requiresMovement(true)
        , interactionRange(5.0f) {}

    QuestGiverInfo(uint32 guid, QuestGiverType t, const Position& pos)
        : giverGuid(guid), type(t), location(pos), zoneId(0), areaId(0)
        , lastInteractionTime(0), isActive(true), requiresMovement(true)
        , interactionRange(5.0f) {}
};

struct QuestPickupRequest
{
    uint32 questId;
    uint32 botGuid;
    uint32 questGiverGuid;
    QuestGiverType giverType;
    Position questGiverLocation;
    uint32 requestTime;
    uint32 priority;
    bool isGroupQuest;
    bool requiresMovement;
    std::string reason;

    QuestPickupRequest(uint32 qId, uint32 bGuid, uint32 gGuid, QuestGiverType gType)
        : questId(qId), botGuid(bGuid), questGiverGuid(gGuid), giverType(gType)
        , requestTime(GameTime::GetGameTimeMS()), priority(100), isGroupQuest(false)
        , requiresMovement(true) {}
};

struct QuestPickupFilter
{
    uint32 minLevel;
    uint32 maxLevel;
    uint32 maxLevelDifference;
    bool acceptGrayQuests;
    bool acceptEliteQuests;
    bool acceptDungeonQuests;
    bool acceptRaidQuests;
    bool acceptPvPQuests;
    bool acceptDailyQuests;
    bool acceptSeasonalQuests;
    std::unordered_set<uint32> acceptedQuestTypes;
    std::unordered_set<uint32> rejectedQuestTypes;
    std::unordered_set<uint32> preferredFactions;
    std::unordered_set<uint32> blacklistedQuests;
    float minRewardValue;
    bool requireQuestText;

    QuestPickupFilter()
        : minLevel(1), maxLevel(80), maxLevelDifference(5)
        , acceptGrayQuests(false), acceptEliteQuests(true)
        , acceptDungeonQuests(true), acceptRaidQuests(false)
        , acceptPvPQuests(true), acceptDailyQuests(true)
        , acceptSeasonalQuests(true), minRewardValue(0.0f)
        , requireQuestText(false) {}
};

class TC_GAME_API QuestPickup final 
{
public:
    explicit QuestPickup(Player* bot);
    ~QuestPickup();
    QuestPickup(QuestPickup const&) = delete;
    QuestPickup& operator=(QuestPickup const&) = delete;

    // Core quest pickup functionality
    bool PickupQuest(uint32 questId, Player* bot, uint32 questGiverGuid = 0);
    bool PickupQuestFromGiver(Player* bot, uint32 questGiverGuid, uint32 questId = 0);
    void PickupAvailableQuests(Player* bot);
    void PickupQuestsInArea(Player* bot, float radius = 50.0f);

    // Quest discovery and scanning
    std::vector<uint32> DiscoverNearbyQuests(Player* bot, float scanRadius = 100.0f);
    std::vector<QuestGiverInfo> ScanForQuestGivers(Player* bot, float scanRadius = 100.0f);
    std::vector<uint32> GetAvailableQuestsFromGiver(uint32 questGiverGuid, Player* bot);
    bool HasAvailableQuests(uint32 questGiverGuid, Player* bot);

    // Quest eligibility and validation
    QuestEligibility CheckQuestEligibility(uint32 questId, Player* bot);
    bool CanAcceptQuest(uint32 questId, Player* bot);
    bool MeetsQuestRequirements(uint32 questId, Player* bot);
    std::vector<std::string> GetEligibilityIssues(uint32 questId, Player* bot);

    // Quest filtering and prioritization
    std::vector<uint32> FilterQuests(const std::vector<uint32>& questIds, Player* bot, const QuestPickupFilter& filter);
    std::vector<uint32> PrioritizeQuests(const std::vector<uint32>& questIds, Player* bot, QuestAcceptanceStrategy strategy);
    uint32 GetNextQuestToPick(Player* bot);
    bool ShouldAcceptQuest(uint32 questId, Player* bot);

    // Quest giver interaction
    bool MoveToQuestGiver(Player* bot, uint32 questGiverGuid);
    bool InteractWithQuestGiver(Player* bot, uint32 questGiverGuid);
    bool IsInRangeOfQuestGiver(Player* bot, uint32 questGiverGuid);
    Position GetQuestGiverLocation(uint32 questGiverGuid);

    // Group quest coordination
    void CoordinateGroupQuestPickup(Group* group, uint32 questId);
    bool ShareQuestPickup(Group* group, uint32 questId, Player* initiator);
    void SynchronizeGroupQuestProgress(Group* group);
    std::vector<uint32> GetGroupCompatibleQuests(Group* group);

    // Automated quest pickup strategies
    void ExecuteStrategy(Player* bot, QuestAcceptanceStrategy strategy);
    void ProcessQuestPickupQueue(Player* bot);
    void ScheduleQuestPickup(const QuestPickupRequest& request);
    void CancelQuestPickup(uint32 questId, uint32 botGuid);

    // Quest chain management
    void TrackQuestChains(Player* bot);
    uint32 GetNextQuestInChain(uint32 currentQuestId);
    std::vector<uint32> GetQuestChainSequence(uint32 startingQuestId);
    void PrioritizeQuestChains(Player* bot);

    // Zone-based quest pickup
    void ScanZoneForQuests(Player* bot, uint32 zoneId);
    std::vector<uint32> GetZoneQuestGivers(uint32 zoneId);
    void OptimizeQuestPickupRoute(Player* bot, const std::vector<uint32>& questGivers);
    bool ShouldMoveToNextZone(Player* bot);

    // Quest pickup performance monitoring (QuestPickupMetrics defined in IQuestPickup.h interface)
    QuestPickupMetrics GetBotPickupMetrics(uint32 botGuid);
    QuestPickupMetrics GetGlobalPickupMetrics();

    // Configuration and settings
    void SetQuestAcceptanceStrategy(uint32 botGuid, QuestAcceptanceStrategy strategy);
    QuestAcceptanceStrategy GetQuestAcceptanceStrategy(uint32 botGuid);
    void SetQuestPickupFilter(uint32 botGuid, const QuestPickupFilter& filter);
    QuestPickupFilter GetQuestPickupFilter(uint32 botGuid);
    void EnableAutoQuestPickup(uint32 botGuid, bool enable);

    // Quest database integration
    void LoadQuestGiverData();
    void UpdateQuestGiverAvailability();
    void CacheQuestInformation();
    void RefreshQuestData();

    // Update and maintenance
    void Update(uint32 diff);
    void ProcessPickupQueue();
    void CleanupExpiredRequests();
    void ValidateQuestStates();

    // Performance optimization
    void OptimizeQuestPickupPerformance();
    void PreloadQuestData(Player* bot);
    void CacheFrequentlyAccessedQuests();
    void UpdateQuestPickupStatistics(uint32 botGuid, bool wasSuccessful, uint32 timeSpent);

    // Configuration constants
    // Note: MAX_QUEST_LOG_SIZE is defined in TrinityCore
    static constexpr uint32 QUEST_PICKUP_TIMEOUT = 10000; // 10 seconds
    static constexpr float DEFAULT_QUEST_GIVER_RANGE = 5.0f;
    static constexpr float QUEST_SCAN_RADIUS = 100.0f;
    static constexpr uint32 PICKUP_QUEUE_PROCESS_INTERVAL = 2000; // 2 seconds
    static constexpr uint32 QUEST_GIVER_UPDATE_INTERVAL = 30000; // 30 seconds
    static constexpr float MIN_QUEST_VALUE_THRESHOLD = 0.1f;
    static constexpr uint32 MAX_CONCURRENT_PICKUPS = 3;
    static constexpr uint32 QUEST_CHAIN_PRIORITY_BONUS = 50;
    static constexpr float GRAY_QUEST_VALUE_MULTIPLIER = 0.1f;
    static constexpr float ELITE_QUEST_VALUE_MULTIPLIER = 1.5f;
    static constexpr uint32 GROUP_QUEST_COORDINATION_TIMEOUT = 15000; // 15 seconds

private:
    Player* _bot;

    // Core data structures
    std::unordered_map<uint32, std::vector<QuestPickupRequest>> _botPickupQueues; // botGuid -> requests
    std::unordered_map<uint32, QuestAcceptanceStrategy> _botStrategies;
    std::unordered_map<uint32, QuestPickupFilter> _botFilters;
    std::unordered_map<uint32, QuestPickupMetrics> _botMetrics;
    

    // Quest giver database
    std::unordered_map<uint32, QuestGiverInfo> _questGivers; // giverGuid -> info
    std::unordered_map<uint32, std::vector<uint32>> _questToGivers; // questId -> giverGuids
    std::unordered_map<uint32, std::vector<uint32>> _zoneQuestGivers; // zoneId -> giverGuids
    

    // Quest chain data
    std::unordered_map<uint32, std::vector<uint32>> _questChains; // chainId -> questIds
    std::unordered_map<uint32, uint32> _questToChain; // questId -> chainId
    std::unordered_map<uint32, uint32> _questNextInChain; // questId -> nextQuestId

    // Performance tracking
    QuestPickupMetrics _globalMetrics;
    std::chrono::steady_clock::time_point _lastUpdate;

    // Helper functions
    void InitializeQuestGiverDatabase();
    void ScanCreatureQuestGivers();
    void ScanGameObjectQuestGivers();
    void ScanItemQuestStarters();
    void BuildQuestChainMapping();
    QuestGiverType DetermineQuestGiverType(uint32 questGiverGuid);
    bool ValidateQuestGiver(uint32 questGiverGuid);
    float CalculateQuestValue(uint32 questId, Player* bot);
    float CalculateQuestPriority(uint32 questId, Player* bot, QuestAcceptanceStrategy strategy);
    bool IsQuestAvailable(uint32 questId, Player* bot);
    void UpdateQuestGiverInteraction(uint32 questGiverGuid, Player* bot);
    void HandleQuestPickupFailure(uint32 questId, Player* bot, const std::string& reason);
    void NotifyQuestPickupSuccess(uint32 questId, Player* bot);

    // Strategy implementations
    void ExecuteAcceptAllStrategy(Player* bot);
    void ExecuteLevelAppropriateStrategy(Player* bot);
    void ExecuteZoneFocusedStrategy(Player* bot);
    void ExecuteChainCompletionStrategy(Player* bot);
    void ExecuteExperienceOptimalStrategy(Player* bot);
    void ExecuteReputationFocusedStrategy(Player* bot);
    void ExecuteGearUpgradeFocusedStrategy(Player* bot);
    void ExecuteGroupCoordinationStrategy(Player* bot);
    void ExecuteSelectiveQualityStrategy(Player* bot);

    // Navigation and pathfinding
    bool CanReachQuestGiver(Player* bot, uint32 questGiverGuid);
    std::vector<Position> GenerateQuestGiverRoute(Player* bot, const std::vector<uint32>& questGivers);
    void OptimizeQuestGiverVisitOrder(Player* bot, std::vector<uint32>& questGivers);
    float CalculateQuestGiverDistance(Player* bot, uint32 questGiverGuid);

    // Quest text and dialog handling
    void HandleQuestDialog(Player* bot, uint32 questGiverGuid, uint32 questId);
    void SelectQuestReward(Player* bot, uint32 questId);
    bool AcceptQuestDialog(Player* bot, uint32 questId);
    void HandleQuestGreeting(Player* bot, uint32 questGiverGuid);

    // Group coordination helpers
    void ShareQuestWithGroup(Group* group, uint32 questId, Player* sender);
    bool CanGroupMemberAcceptQuest(Player* member, uint32 questId);
    void WaitForGroupQuestDecisions(Group* group, uint32 questId, uint32 timeoutMs = 10000);
    void HandleGroupQuestConflict(Group* group, uint32 questId);
};

} // namespace Playerbot