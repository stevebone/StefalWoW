# PHASE 4: EVENT SYSTEM INTEGRATION - DETAILED IMPLEMENTATION SUBPLAN

## Executive Summary
**Duration**: 60-80 hours (1.5-2 weeks)
**Team Size**: 6-7 specialized agents
**Primary Goal**: Convert polling-based state checks to event-driven architecture
**Critical Issues Addressed**: #1 (login group behavior), performance optimization, reactive AI

## Phase 4 Architecture Overview

### Core Components
```
Events/
├── BotEventSystem.h/cpp            (750 lines) - Core event dispatcher
├── GroupEventObserver.h/cpp        (450 lines) - Group state events
├── CombatEventObserver.h/cpp       (450 lines) - Combat events
├── WorldEventObserver.h/cpp        (450 lines) - World state events
├── EventQueue.h/cpp                (400 lines) - Priority queue management
├── EventSubscription.h/cpp         (350 lines) - Subscription management
├── EventMetrics.h/cpp              (300 lines) - Performance monitoring
└── tests/                          (2500+ lines) - Comprehensive testing
```

## Detailed Task Breakdown

### Task 4.1: Event System Architecture Design
**Duration**: 10 hours
**Assigned Agents**:
- Primary: cpp-architecture-optimizer (event patterns)
- Support: concurrency-threading-specialist (async design)
**Dependencies**: Phase 3 complete
**Deliverables**:
```cpp
// BotEventSystem.h
class BotEventSystem {
public:
    // Event types
    enum class EventType : uint32_t {
        // Group events
        GROUP_CREATED = 1 << 0,
        GROUP_DISBANDED = 1 << 1,
        GROUP_MEMBER_ADDED = 1 << 2,
        GROUP_MEMBER_REMOVED = 1 << 3,
        GROUP_LEADER_CHANGED = 1 << 4,

        // Combat events
        COMBAT_STARTED = 1 << 5,
        COMBAT_ENDED = 1 << 6,
        TARGET_CHANGED = 1 << 7,
        THREAT_CHANGED = 1 << 8,
        DAMAGE_TAKEN = 1 << 9,
        DAMAGE_DEALT = 1 << 10,

        // World events
        PLAYER_LOGIN = 1 << 11,
        PLAYER_LOGOUT = 1 << 12,
        ZONE_CHANGED = 1 << 13,
        QUEST_ACCEPTED = 1 << 14,
        QUEST_COMPLETED = 1 << 15,
        ITEM_RECEIVED = 1 << 16,

        // Bot-specific events
        BOT_SPAWNED = 1 << 17,
        BOT_DESPAWNED = 1 << 18,
        BOT_BEHAVIOR_CHANGED = 1 << 19,
        BOT_STATE_CHANGED = 1 << 20
    };

    // Event data base
    struct EventData {
        EventType type;
        ObjectGuid sourceGuid;
        std::chrono::steady_clock::time_point timestamp;
        uint32_t priority;

        virtual ~EventData() = default;
    };

    // Specialized event data
    struct GroupEventData : EventData {
        ObjectGuid groupGuid;
        ObjectGuid memberGuid;
        GroupRole role;
    };

    struct CombatEventData : EventData {
        ObjectGuid targetGuid;
        uint32_t damage;
        uint32_t threat;
        SpellSchoolMask school;
    };

    // Event handler types
    using EventHandler = std::function<void(const EventData&)>;
    using EventFilter = std::function<bool(const EventData&)>;
    using SubscriptionId = uint64_t;

    // Singleton access
    static BotEventSystem& Instance();

    // Event publishing
    void PublishEvent(std::unique_ptr<EventData> event);
    void PublishEventDeferred(std::unique_ptr<EventData> event, std::chrono::milliseconds delay);
    void PublishEventBatch(std::vector<std::unique_ptr<EventData>> events);

    // Event subscription
    SubscriptionId Subscribe(EventType type, EventHandler handler,
                            EventFilter filter = nullptr,
                            uint32_t priority = 100);
    void Unsubscribe(SubscriptionId id);
    void UnsubscribeAll(ObjectGuid subscriber);

    // Event processing
    void ProcessEvents(uint32_t maxEvents = 100);
    void ProcessEventsFor(std::chrono::milliseconds duration);
    void FlushEvents();

    // Configuration
    void SetMaxQueueSize(size_t size);
    void SetProcessingThreadCount(uint32_t count);
    void EnableEventLogging(bool enable);

    // Metrics
    EventMetrics GetMetrics() const;
    void ResetMetrics();

private:
    BotEventSystem();
    ~BotEventSystem();

    struct Subscription {
        SubscriptionId id;
        EventType type;
        EventHandler handler;
        EventFilter filter;
        uint32_t priority;
        ObjectGuid owner;
    };

    // Event queue with priority
    std::priority_queue<std::unique_ptr<EventData>,
                       std::vector<std::unique_ptr<EventData>>,
                       EventComparator> m_eventQueue;

    // Subscriptions indexed by event type
    std::unordered_map<EventType, std::vector<Subscription>> m_subscriptions;

    // Deferred events
    std::multimap<std::chrono::steady_clock::time_point,
                  std::unique_ptr<EventData>> m_deferredEvents;

    // Thread safety
    mutable std::shared_mutex m_queueMutex;
    mutable std::shared_mutex m_subscriptionMutex;

    // Processing threads
    std::vector<std::thread> m_processingThreads;
    std::atomic<bool> m_shouldStop{false};

    // Metrics
    EventMetrics m_metrics;

    // ID generation
    std::atomic<SubscriptionId> m_nextSubscriptionId{1};
};

// Event type operators
inline EventType operator|(EventType a, EventType b) {
    return static_cast<EventType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool HasEventType(uint32_t mask, EventType type) {
    return (mask & static_cast<uint32_t>(type)) != 0;
}
```

### Task 4.2: Group Event Observer Implementation
**Duration**: 10 hours
**Assigned Agents**:
- Primary: wow-bot-behavior-designer (group behavior)
- Support: trinity-integration-tester (group hooks)
**Dependencies**: Task 4.1
**Deliverables**:
```cpp
// GroupEventObserver.h
class GroupEventObserver {
public:
    static void Initialize();
    static void Shutdown();

    // Core group event hooks
    static void OnGroupCreate(Group* group);
    static void OnGroupDisband(Group* group);
    static void OnMemberAdded(Group* group, ObjectGuid memberGuid);
    static void OnMemberRemoved(Group* group, ObjectGuid memberGuid);
    static void OnLeaderChanged(Group* group, ObjectGuid newLeaderGuid);

    // Role and assignment events
    static void OnRoleChanged(Group* group, ObjectGuid memberGuid, GroupRole newRole);
    static void OnLootMethodChanged(Group* group, LootMethod method);
    static void OnDifficultyChanged(Group* group, Difficulty difficulty);

    // Raid-specific events
    static void OnRaidTargetChanged(Group* group, uint8_t symbol, ObjectGuid targetGuid);
    static void OnReadyCheckInitiated(Group* group, ObjectGuid initiator);
    static void OnReadyCheckResponse(Group* group, ObjectGuid player, bool ready);

    // Bot-specific group handling
    static void UpdateBotGroupBehavior(Bot* bot, Group* group);
    static void HandleBotGroupInvite(Bot* bot, Player* inviter);
    static void HandleBotPromotionToLeader(Bot* bot);

private:
    // Event creation helpers
    static std::unique_ptr<GroupEventData> CreateGroupEvent(
        BotEventSystem::EventType type,
        Group* group,
        ObjectGuid memberGuid = ObjectGuid::Empty);

    // Bot response handlers
    static void HandleGroupFormation(Bot* bot, const GroupEventData& event);
    static void HandleLeaderFollowing(Bot* bot, const GroupEventData& event);
    static void HandleRoleAssignment(Bot* bot, const GroupEventData& event);

    // Subscription management
    static std::unordered_map<ObjectGuid, BotEventSystem::SubscriptionId> s_botSubscriptions;
    static std::shared_mutex s_subscriptionMutex;

    // Configuration
    static bool s_initialized;
    static uint32_t s_eventPriority;
};

// Integration with TrinityCore Group class
class GroupHooks {
public:
    static void InstallHooks();
    static void RemoveHooks();

private:
    // Hook implementations
    static void Hook_AddMember(Group* group, ObjectGuid guid);
    static void Hook_RemoveMember(Group* group, ObjectGuid guid);
    static void Hook_ChangeLeader(Group* group, ObjectGuid guid);
    static void Hook_Disband(Group* group);

    // Original function pointers
    static decltype(&Group::AddMember) Original_AddMember;
    static decltype(&Group::RemoveMember) Original_RemoveMember;
    static decltype(&Group::ChangeLeader) Original_ChangeLeader;
    static decltype(&Group::Disband) Original_Disband;
};
```

### Task 4.3: Combat Event Observer Implementation
**Duration**: 10 hours
**Assigned Agents**:
- Primary: wow-mechanics-expert (combat mechanics)
- Support: pvp-arena-tactician (PvP events)
**Dependencies**: Task 4.1
**Deliverables**:
```cpp
// CombatEventObserver.h
class CombatEventObserver {
public:
    static void Initialize();
    static void Shutdown();

    // Combat state events
    static void OnCombatStart(Unit* unit, Unit* target);
    static void OnCombatEnd(Unit* unit);
    static void OnTargetSwitch(Unit* unit, Unit* oldTarget, Unit* newTarget);

    // Damage events
    static void OnDamageDealt(Unit* dealer, Unit* victim, uint32_t damage, SpellInfo const* spellInfo);
    static void OnDamageTaken(Unit* victim, Unit* dealer, uint32_t damage, SpellInfo const* spellInfo);
    static void OnHealingDone(Unit* healer, Unit* target, uint32_t heal, SpellInfo const* spellInfo);

    // Threat events
    static void OnThreatUpdate(Unit* unit, Unit* victim, float threat);
    static void OnThreatClear(Unit* unit);
    static void OnTauntSuccess(Unit* taunter, Unit* target);

    // Spell events
    static void OnSpellCast(Unit* caster, SpellInfo const* spellInfo, Unit* target);
    static void OnSpellInterrupt(Unit* caster, Unit* interrupter, SpellInfo const* spellInfo);
    static void OnSpellResist(Unit* caster, Unit* target, SpellInfo const* spellInfo);

    // Death and resurrection
    static void OnDeath(Unit* unit, Unit* killer);
    static void OnResurrect(Unit* unit);

    // PvP-specific events
    static void OnHonorKill(Player* killer, Player* victim);
    static void OnDuelStart(Player* player1, Player* player2);
    static void OnDuelEnd(Player* winner, Player* loser);

    // Bot combat behavior updates
    static void UpdateBotCombatBehavior(Bot* bot, const CombatEventData& event);
    static void HandleBotThreatManagement(Bot* bot, const CombatEventData& event);
    static void HandleBotTargetSelection(Bot* bot, const CombatEventData& event);

private:
    struct CombatContext {
        ObjectGuid targetGuid;
        uint32_t totalDamageDealt{0};
        uint32_t totalDamageTaken{0};
        uint32_t totalHealing{0};
        std::chrono::steady_clock::time_point combatStartTime;
        ThreatManager threatInfo;
    };

    // Per-unit combat tracking
    static std::unordered_map<ObjectGuid, CombatContext> s_combatContexts;
    static std::shared_mutex s_contextMutex;

    // Event creation
    static std::unique_ptr<CombatEventData> CreateCombatEvent(
        BotEventSystem::EventType type,
        Unit* source,
        Unit* target,
        uint32_t value = 0);

    // Bot response strategies
    static void ApplyDefensiveStrategy(Bot* bot, const CombatEventData& event);
    static void ApplyOffensiveStrategy(Bot* bot, const CombatEventData& event);
    static void ApplySurvivalStrategy(Bot* bot, const CombatEventData& event);

    // Configuration
    static bool s_initialized;
    static uint32_t s_damageThreshold;
    static uint32_t s_threatThreshold;
};
```

### Task 4.4: World Event Observer Implementation
**Duration**: 10 hours
**Assigned Agents**:
- Primary: trinity-integration-tester (world hooks)
- Support: wow-economy-manager (economic events)
**Dependencies**: Task 4.1
**Deliverables**:
```cpp
// WorldEventObserver.h
class WorldEventObserver {
public:
    static void Initialize();
    static void Shutdown();

    // Player session events
    static void OnPlayerLogin(Player* player);
    static void OnPlayerLogout(Player* player);
    static void OnPlayerSave(Player* player);

    // Movement and location events
    static void OnZoneChange(Player* player, uint32_t newZone, uint32_t newArea);
    static void OnMapChange(Player* player, uint32_t mapId);
    static void OnTeleport(Player* player, LocationVector const& destination);

    // Quest events
    static void OnQuestAccept(Player* player, Quest const* quest);
    static void OnQuestComplete(Player* player, Quest const* quest);
    static void OnQuestAbandon(Player* player, Quest const* quest);
    static void OnQuestObjectiveComplete(Player* player, Quest const* quest, uint32_t objectiveId);

    // Item and loot events
    static void OnItemLooted(Player* player, Item* item);
    static void OnItemEquipped(Player* player, Item* item, uint8_t slot);
    static void OnItemDestroyed(Player* player, Item* item);
    static void OnCurrencyChange(Player* player, CurrencyTypes currency, int32_t change);

    // Social events
    static void OnWhisperReceived(Player* player, Player* sender, std::string const& message);
    static void OnGuildChat(Player* player, Guild* guild, std::string const& message);
    static void OnEmoteReceived(Player* player, Player* sender, uint32_t emoteId);

    // Achievement events
    static void OnAchievementComplete(Player* player, AchievementEntry const* achievement);
    static void OnAchievementProgress(Player* player, CriteriaEntry const* criteria);

    // Bot world interaction
    static void UpdateBotWorldInteraction(Bot* bot, const WorldEventData& event);
    static void HandleBotQuestBehavior(Bot* bot, Quest const* quest);
    static void HandleBotEconomicActivity(Bot* bot, const WorldEventData& event);

private:
    struct WorldEventData : BotEventSystem::EventData {
        uint32_t zoneId{0};
        uint32_t mapId{0};
        uint32_t questId{0};
        uint32_t itemId{0};
        std::string extraData;
    };

    // Event creation
    static std::unique_ptr<WorldEventData> CreateWorldEvent(
        BotEventSystem::EventType type,
        Player* player,
        uint32_t dataId = 0);

    // Bot behavioral responses
    static void HandleLoginProximityCheck(Bot* bot, Player* player);
    static void HandleQuestSharing(Bot* bot, Player* player, Quest const* quest);
    static void HandleZoneAppropriateBehavior(Bot* bot, uint32_t zoneId);

    // Subscription tracking
    static std::unordered_map<ObjectGuid, std::vector<BotEventSystem::SubscriptionId>> s_subscriptions;
    static std::shared_mutex s_subscriptionMutex;

    // Configuration
    static bool s_initialized;
    static uint32_t s_proximityCheckRadius;
    static bool s_enableQuestSharing;
};
```

### Task 4.5: Event Queue Management
**Duration**: 8 hours
**Assigned Agents**:
- Primary: concurrency-threading-specialist (queue design)
- Support: resource-monitor-limiter (memory management)
**Dependencies**: Tasks 4.2-4.4
**Deliverables**:
```cpp
// EventQueue.h
template<typename EventType>
class EventQueue {
public:
    struct QueueStats {
        size_t currentSize{0};
        size_t maxSize{0};
        size_t totalProcessed{0};
        size_t droppedEvents{0};
        std::chrono::microseconds avgProcessingTime{0};
    };

    explicit EventQueue(size_t maxSize = 10000)
        : m_maxSize(maxSize) {}

    // Queue operations
    bool Enqueue(std::unique_ptr<EventType> event, uint32_t priority = 100);
    std::unique_ptr<EventType> Dequeue();
    std::vector<std::unique_ptr<EventType>> DequeueBatch(size_t count);

    // Priority management
    void UpdatePriority(const std::function<bool(const EventType&)>& matcher,
                       uint32_t newPriority);
    void PurgeEvents(const std::function<bool(const EventType&)>& matcher);

    // Queue state
    bool IsEmpty() const;
    size_t Size() const;
    bool IsFull() const;

    // Statistics
    QueueStats GetStats() const;
    void ResetStats();

    // Configuration
    void SetMaxSize(size_t size);
    void SetOverflowPolicy(OverflowPolicy policy);

private:
    struct PrioritizedEvent {
        std::unique_ptr<EventType> event;
        uint32_t priority;
        uint64_t sequenceNumber;

        bool operator<(const PrioritizedEvent& other) const {
            if (priority != other.priority)
                return priority < other.priority;  // Higher priority first
            return sequenceNumber > other.sequenceNumber;  // FIFO for same priority
        }
    };

    std::priority_queue<PrioritizedEvent> m_queue;
    size_t m_maxSize;
    std::atomic<uint64_t> m_sequenceCounter{0};

    // Thread safety
    mutable std::mutex m_queueMutex;
    std::condition_variable m_notEmpty;

    // Statistics
    QueueStats m_stats;

    // Overflow handling
    enum class OverflowPolicy {
        DROP_OLDEST,
        DROP_NEWEST,
        DROP_LOWEST_PRIORITY,
        BLOCK
    };
    OverflowPolicy m_overflowPolicy{OverflowPolicy::DROP_LOWEST_PRIORITY};
};
```

### Task 4.6: Event Subscription Management
**Duration**: 8 hours
**Assigned Agents**:
- Primary: cpp-architecture-optimizer (subscription patterns)
- Support: code-quality-reviewer (API design)
**Dependencies**: Task 4.5
**Deliverables**:
```cpp
// EventSubscription.h
class EventSubscriptionManager {
public:
    using SubscriptionId = uint64_t;
    using EventHandler = std::function<void(const BotEventSystem::EventData&)>;

    struct Subscription {
        SubscriptionId id;
        ObjectGuid subscriberGuid;
        BotEventSystem::EventType eventMask;
        EventHandler handler;
        uint32_t priority;
        bool isActive;

        // Filtering
        std::optional<ObjectGuid> sourceFilter;
        std::optional<ObjectGuid> targetFilter;
        std::optional<uint32_t> zoneFilter;

        // Lifetime
        std::optional<std::chrono::steady_clock::time_point> expiration;
        std::optional<uint32_t> maxTriggers;
        uint32_t triggerCount{0};
    };

    // Subscription creation
    SubscriptionId Subscribe(const Subscription& subscription);

    template<typename EventDataType>
    SubscriptionId SubscribeTyped(
        ObjectGuid subscriber,
        std::function<void(const EventDataType&)> handler,
        uint32_t priority = 100);

    // Subscription management
    void Unsubscribe(SubscriptionId id);
    void UnsubscribeAll(ObjectGuid subscriber);
    void SuspendSubscription(SubscriptionId id);
    void ResumeSubscription(SubscriptionId id);

    // Batch operations
    std::vector<SubscriptionId> GetSubscriptions(ObjectGuid subscriber) const;
    void UpdateSubscriptionPriority(SubscriptionId id, uint32_t newPriority);

    // Event dispatch
    void DispatchEvent(const BotEventSystem::EventData& event);
    uint32_t GetSubscriberCount(BotEventSystem::EventType type) const;

    // Maintenance
    void CleanupExpiredSubscriptions();
    void ValidateSubscriptions();

private:
    std::unordered_map<SubscriptionId, Subscription> m_subscriptions;
    std::unordered_multimap<BotEventSystem::EventType, SubscriptionId> m_typeIndex;
    std::unordered_multimap<ObjectGuid, SubscriptionId> m_subscriberIndex;

    mutable std::shared_mutex m_subscriptionMutex;
    std::atomic<SubscriptionId> m_nextId{1};

    // Helper methods
    bool MatchesFilters(const Subscription& sub, const BotEventSystem::EventData& event) const;
    void ExecuteHandler(Subscription& sub, const BotEventSystem::EventData& event);
    void RemoveFromIndices(SubscriptionId id);
};
```

### Task 4.7: Event Metrics and Monitoring
**Duration**: 6 hours
**Assigned Agents**:
- Primary: resource-monitor-limiter (metrics design)
- Support: windows-memory-profiler (performance tracking)
**Dependencies**: Task 4.6
**Deliverables**:
```cpp
// EventMetrics.h
class EventMetrics {
public:
    struct EventTypeMetrics {
        uint64_t publishCount{0};
        uint64_t processCount{0};
        uint64_t dropCount{0};
        std::chrono::microseconds totalProcessingTime{0};
        std::chrono::microseconds avgProcessingTime{0};
        std::chrono::microseconds maxProcessingTime{0};
        size_t maxQueueDepth{0};
    };

    struct SubscriberMetrics {
        ObjectGuid subscriberGuid;
        uint64_t eventsReceived{0};
        uint64_t eventsProcessed{0};
        std::chrono::microseconds totalHandlingTime{0};
        std::unordered_map<BotEventSystem::EventType, uint64_t> eventBreakdown;
    };

    // Metric collection
    void RecordEventPublished(BotEventSystem::EventType type);
    void RecordEventProcessed(BotEventSystem::EventType type,
                              std::chrono::microseconds processingTime);
    void RecordEventDropped(BotEventSystem::EventType type);
    void RecordSubscriberHandling(ObjectGuid subscriber,
                                  BotEventSystem::EventType type,
                                  std::chrono::microseconds handlingTime);

    // Metric retrieval
    EventTypeMetrics GetEventTypeMetrics(BotEventSystem::EventType type) const;
    SubscriberMetrics GetSubscriberMetrics(ObjectGuid subscriber) const;
    std::vector<EventTypeMetrics> GetAllEventMetrics() const;

    // Performance analysis
    std::vector<BotEventSystem::EventType> GetSlowEvents(std::chrono::microseconds threshold) const;
    std::vector<ObjectGuid> GetSlowSubscribers(std::chrono::microseconds threshold) const;
    double GetEventLossRate() const;

    // Reporting
    std::string GenerateReport() const;
    void ExportToJson(const std::filesystem::path& path) const;
    void LogMetrics() const;

    // Reset
    void Reset();

private:
    std::unordered_map<BotEventSystem::EventType, EventTypeMetrics> m_eventMetrics;
    std::unordered_map<ObjectGuid, SubscriberMetrics> m_subscriberMetrics;

    mutable std::shared_mutex m_metricsMutex;

    // Helpers
    void UpdateAverages();
    std::string FormatDuration(std::chrono::microseconds duration) const;
};
```

### Task 4.8: Integration Testing Suite
**Duration**: 10 hours
**Assigned Agents**:
- Primary: test-automation-engineer (test design)
- Support: trinity-integration-tester (integration validation)
**Dependencies**: Tasks 4.1-4.7
**Deliverables**:
```cpp
// EventSystemTests.cpp
class EventSystemTests : public ::testing::Test {
protected:
    void TestLoginGroupBehaviorFix() {
        // Issue #1: Bot creates group on player login
        auto player = CreateTestPlayer();
        auto bot = CreateTestBot();

        // Set bot to follow player
        bot->SetFollowTarget(player->GetGUID());

        // Subscribe to login event
        bool groupCreated = false;
        auto subId = BotEventSystem::Instance().Subscribe(
            BotEventSystem::EventType::PLAYER_LOGIN,
            [&](const BotEventSystem::EventData& event) {
                // Bot should NOT create group automatically
                groupCreated = bot->GetGroup() != nullptr;
            });

        // Simulate player login
        WorldEventObserver::OnPlayerLogin(player);

        // Process events
        BotEventSystem::Instance().ProcessEvents();

        // Verify no automatic group creation
        EXPECT_FALSE(groupCreated);
        EXPECT_EQ(bot->GetGroup(), nullptr);

        BotEventSystem::Instance().Unsubscribe(subId);
    }

    void TestEventPropagation() {
        std::atomic<int> handlerCallCount{0};

        // Subscribe to combat event
        auto subId = BotEventSystem::Instance().Subscribe(
            BotEventSystem::EventType::COMBAT_STARTED,
            [&](const BotEventSystem::EventData& event) {
                handlerCallCount++;
            });

        // Trigger combat event
        auto unit = CreateTestUnit();
        auto target = CreateTestUnit();
        CombatEventObserver::OnCombatStart(unit, target);

        // Process events
        BotEventSystem::Instance().ProcessEvents();

        EXPECT_EQ(handlerCallCount, 1);

        BotEventSystem::Instance().Unsubscribe(subId);
    }

    void TestEventPriority() {
        std::vector<int> executionOrder;

        // Subscribe with different priorities
        auto sub1 = BotEventSystem::Instance().Subscribe(
            BotEventSystem::EventType::GROUP_CREATED,
            [&](const auto& e) { executionOrder.push_back(1); },
            nullptr, 50);  // Low priority

        auto sub2 = BotEventSystem::Instance().Subscribe(
            BotEventSystem::EventType::GROUP_CREATED,
            [&](const auto& e) { executionOrder.push_back(2); },
            nullptr, 150);  // High priority

        auto sub3 = BotEventSystem::Instance().Subscribe(
            BotEventSystem::EventType::GROUP_CREATED,
            [&](const auto& e) { executionOrder.push_back(3); },
            nullptr, 100);  // Medium priority

        // Trigger event
        auto group = CreateTestGroup();
        GroupEventObserver::OnGroupCreate(group);

        // Process events
        BotEventSystem::Instance().ProcessEvents();

        // Verify execution order (high to low priority)
        ASSERT_EQ(executionOrder.size(), 3);
        EXPECT_EQ(executionOrder[0], 2);  // High
        EXPECT_EQ(executionOrder[1], 3);  // Medium
        EXPECT_EQ(executionOrder[2], 1);  // Low
    }

    void TestMemoryLeaks() {
        const size_t iterations = 10000;
        const size_t initialMemory = GetMemoryUsage();

        for (size_t i = 0; i < iterations; ++i) {
            // Create and destroy subscriptions
            auto subId = BotEventSystem::Instance().Subscribe(
                BotEventSystem::EventType::DAMAGE_DEALT,
                [](const auto& e) { /* Do nothing */ });

            // Publish events
            auto event = std::make_unique<BotEventSystem::EventData>();
            event->type = BotEventSystem::EventType::DAMAGE_DEALT;
            BotEventSystem::Instance().PublishEvent(std::move(event));

            // Process and cleanup
            BotEventSystem::Instance().ProcessEvents();
            BotEventSystem::Instance().Unsubscribe(subId);
        }

        const size_t finalMemory = GetMemoryUsage();
        const size_t leaked = finalMemory - initialMemory;

        EXPECT_LT(leaked, 1024 * 1024);  // Less than 1MB
    }

    void TestConcurrentEventHandling() {
        std::atomic<uint32_t> eventCount{0};
        const uint32_t threadCount = 10;
        const uint32_t eventsPerThread = 1000;

        // Subscribe to events
        auto subId = BotEventSystem::Instance().Subscribe(
            BotEventSystem::EventType::DAMAGE_DEALT,
            [&](const auto& e) { eventCount++; });

        // Spawn threads to publish events
        std::vector<std::thread> threads;
        for (uint32_t t = 0; t < threadCount; ++t) {
            threads.emplace_back([&, t]() {
                for (uint32_t i = 0; i < eventsPerThread; ++i) {
                    auto event = std::make_unique<CombatEventData>();
                    event->type = BotEventSystem::EventType::DAMAGE_DEALT;
                    event->damage = t * 1000 + i;
                    BotEventSystem::Instance().PublishEvent(std::move(event));
                }
            });
        }

        // Wait for all threads
        for (auto& thread : threads) {
            thread.join();
        }

        // Process all events
        BotEventSystem::Instance().FlushEvents();

        // Verify all events were handled
        EXPECT_EQ(eventCount, threadCount * eventsPerThread);

        BotEventSystem::Instance().Unsubscribe(subId);
    }
};
```

### Task 4.9: Performance Optimization
**Duration**: 8 hours
**Assigned Agents**:
- Primary: resource-monitor-limiter (optimization)
- Support: concurrency-threading-specialist (parallelization)
**Dependencies**: Task 4.8
**Deliverables**:
- Lock-free event queue implementation
- Batch event processing optimization
- Memory pool for event objects
- CPU affinity for processing threads

### Task 4.10: Documentation and Migration
**Duration**: 6 hours
**Assigned Agents**:
- Primary: code-quality-reviewer (documentation)
- Support: test-automation-engineer (examples)
**Dependencies**: Task 4.9
**Deliverables**:
- Event system API documentation
- Migration guide from polling
- Event handler best practices
- Performance tuning guide

## Testing Strategy

### Unit Testing Requirements
- 100% coverage of event system core
- All event types tested
- Subscription management validation
- Queue overflow handling

### Integration Testing Requirements
- Group system integration
- Combat system integration
- World event integration
- Cross-module event flow

### Performance Testing Requirements
- 100,000 events/second throughput
- <100μs average event latency
- <1MB memory per 1000 events
- Zero event loss under load

### Stress Testing Requirements
- 24-hour continuous operation
- 10,000 concurrent subscriptions
- Memory leak detection
- Thread safety validation

## Risk Mitigation

### Technical Risks
1. **Event Storm**: Rate limiting and backpressure
2. **Memory Growth**: Event object pooling
3. **Deadlocks**: Lock-free structures where possible
4. **Event Loss**: Persistent queue option for critical events

### Integration Risks
1. **Hook Conflicts**: Careful hook installation/removal
2. **Performance Impact**: Async processing with dedicated threads
3. **Backward Compatibility**: Optional event system activation

## Success Criteria

### Functional Requirements
- ✅ Issue #1 (login group behavior) fixed
- ✅ All polling converted to events
- ✅ Real-time event propagation
- ✅ Zero event loss under normal operation
- ✅ Proper event ordering maintained

### Performance Requirements
- ✅ <100μs event processing latency
- ✅ 100,000 events/second throughput
- ✅ <0.1% CPU overhead
- ✅ <1MB memory per 1000 events

### Quality Requirements
- ✅ 100% test coverage
- ✅ Zero memory leaks
- ✅ Thread-safe operations
- ✅ Complete documentation

## Agent Coordination Matrix

| Task | Primary Agent | Support Agents | Review Agent |
|------|--------------|----------------|--------------|
| 4.1 | cpp-architecture-optimizer | concurrency-threading-specialist | code-quality-reviewer |
| 4.2 | wow-bot-behavior-designer | trinity-integration-tester | cpp-architecture-optimizer |
| 4.3 | wow-mechanics-expert | pvp-arena-tactician | trinity-integration-tester |
| 4.4 | trinity-integration-tester | wow-economy-manager | wow-bot-behavior-designer |
| 4.5 | concurrency-threading-specialist | resource-monitor-limiter | cpp-architecture-optimizer |
| 4.6 | cpp-architecture-optimizer | code-quality-reviewer | concurrency-threading-specialist |
| 4.7 | resource-monitor-limiter | windows-memory-profiler | test-automation-engineer |
| 4.8 | test-automation-engineer | trinity-integration-tester | code-quality-reviewer |
| 4.9 | resource-monitor-limiter | concurrency-threading-specialist | cpp-architecture-optimizer |
| 4.10 | code-quality-reviewer | test-automation-engineer | cpp-architecture-optimizer |

## Rollback Strategy

### Phase 4 Rollback Points
1. **Pre-Event**: Before event system implementation
2. **Core-Complete**: After core event system
3. **Observers-Ready**: After all observers implemented
4. **Production**: After full validation

### Rollback Procedure
```bash
# Checkpoint creation
git tag phase4-checkpoint-1  # Before event system
git tag phase4-checkpoint-2  # After core system
git tag phase4-checkpoint-3  # After observers
git tag phase4-complete      # Production ready

# Emergency rollback
./scripts/rollback_phase4.sh [checkpoint-number]
```

## Deliverables Checklist

### Code Deliverables
- [ ] BotEventSystem.h/cpp (750 lines)
- [ ] GroupEventObserver.h/cpp (450 lines)
- [ ] CombatEventObserver.h/cpp (450 lines)
- [ ] WorldEventObserver.h/cpp (450 lines)
- [ ] EventQueue.h/cpp (400 lines)
- [ ] EventSubscription.h/cpp (350 lines)
- [ ] EventMetrics.h/cpp (300 lines)

### Test Deliverables
- [ ] Unit tests (1500+ lines)
- [ ] Integration tests (1000+ lines)
- [ ] Performance benchmarks
- [ ] Stress test scenarios

### Documentation Deliverables
- [ ] API reference
- [ ] Migration guide
- [ ] Best practices guide
- [ ] Performance tuning guide

## Phase 4 Complete Validation

### Exit Criteria
1. Issue #1 completely resolved
2. All polling converted to events
3. Performance targets met
4. Zero regression in functionality
5. Full test coverage achieved

**Estimated Completion**: 70 hours (mid-range of 60-80 hour estimate)
**Confidence Level**: 92% (proven event-driven patterns)