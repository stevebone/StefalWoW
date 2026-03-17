# PHASE 2: BEHAVIOR PRIORITY SYSTEM - DETAILED IMPLEMENTATION SUBPLAN

## Executive Summary
**Duration**: 60-80 hours (1.5-2 weeks)
**Team Size**: 5-7 specialized agents
**Primary Goal**: Implement comprehensive behavior priority system to eliminate conflicting behaviors
**Critical Issues Addressed**: #2 (ranged combat), #3 (melee facing)

## Phase 2 Architecture Overview

### Core Components
```
BehaviorManager/
├── BehaviorManager.h/cpp           (850 lines) - Central orchestrator
├── BehaviorPriority.h              (100 lines) - Priority definitions
├── BehaviorContext.h/cpp           (350 lines) - Execution context
├── MutualExclusionRules.h/cpp      (350 lines) - Conflict resolution
├── BehaviorStateTransition.h/cpp    (300 lines) - State management
├── BehaviorMetrics.h/cpp            (250 lines) - Performance tracking
└── tests/                           (2000+ lines) - Comprehensive testing
```

## Detailed Task Breakdown

### Task 2.1: Behavior Priority Framework Design
**Duration**: 8 hours
**Assigned Agents**:
- Primary: cpp-architecture-optimizer (design patterns)
- Support: wow-bot-behavior-designer (behavior modeling)
**Dependencies**: Phase 1 complete
**Deliverables**:
```cpp
// BehaviorPriority.h
enum class BehaviorCategory : uint8_t {
    CRITICAL_SURVIVAL = 0,    // Death prevention, heal at <20% health
    COMBAT_OFFENSIVE = 1,      // Active combat engagement
    COMBAT_DEFENSIVE = 2,      // Defensive positioning, threat management
    MOVEMENT_TACTICAL = 3,     // Combat movement, positioning
    MOVEMENT_FOLLOW = 4,       // Following group/leader
    SOCIAL_INTERACTION = 5,    // Quest, vendor, bank interactions
    IDLE_ACTIVITY = 6         // Random movements, emotes
};

struct BehaviorPriority {
    float baseRelevance;           // 0.0 - 1.0
    float contextMultiplier;       // Dynamic adjustment
    uint32_t exclusionMask;        // Mutually exclusive behaviors
    std::chrono::milliseconds timeout;  // Max execution time

    float GetEffectiveRelevance() const {
        return baseRelevance * contextMultiplier;
    }
};
```

### Task 2.2: BehaviorManager Core Implementation
**Duration**: 12 hours
**Assigned Agents**:
- Primary: cpp-architecture-optimizer (system design)
- Support: concurrency-threading-specialist (thread safety)
**Dependencies**: Task 2.1
**Deliverables**:
```cpp
// BehaviorManager.h
class BehaviorManager {
public:
    using BehaviorId = uint32_t;
    using BehaviorFactory = std::function<std::unique_ptr<Behavior>()>;

    // Behavior registration
    void RegisterBehavior(BehaviorId id, BehaviorCategory category,
                         BehaviorFactory factory, BehaviorPriority priority);

    // Behavior execution
    void Update(BotAI* ai, uint32_t diff);
    BehaviorExecutionResult ExecuteHighestPriority(BotAI* ai);

    // Priority management
    void SetCombatMode(bool inCombat);
    void SetGroupMode(bool inGroup);
    void OverridePriority(BehaviorId id, float newRelevance);

    // Metrics & debugging
    BehaviorMetrics GetMetrics() const;
    std::vector<BehaviorDebugInfo> GetDebugInfo() const;

private:
    struct BehaviorEntry {
        BehaviorId id;
        BehaviorCategory category;
        BehaviorPriority priority;
        std::unique_ptr<Behavior> instance;
        BehaviorMetrics metrics;
        std::chrono::steady_clock::time_point lastExecution;
    };

    std::vector<BehaviorEntry> m_behaviors;
    std::array<std::vector<BehaviorId>, 7> m_categoryBuckets;
    mutable std::shared_mutex m_behaviorMutex;

    // Execution state
    BehaviorId m_currentBehavior{0};
    BehaviorContext m_context;

    // Performance optimization
    std::array<float, 7> m_categoryRelevanceCache;
    bool m_cacheValid{false};
};
```

### Task 2.3: Mutual Exclusion Rules System
**Duration**: 10 hours
**Assigned Agents**:
- Primary: wow-bot-behavior-designer (behavior rules)
- Support: code-quality-reviewer (rule validation)
**Dependencies**: Task 2.2
**Deliverables**:
```cpp
// MutualExclusionRules.h
class MutualExclusionRules {
public:
    struct ExclusionRule {
        std::string name;
        std::function<bool(const BehaviorContext&)> condition;
        std::set<BehaviorId> excludedBehaviors;
        float priorityOverride;
    };

    // Rule management
    void AddRule(ExclusionRule rule);
    void AddCategoryExclusion(BehaviorCategory cat1, BehaviorCategory cat2);

    // Rule evaluation
    std::set<BehaviorId> GetExcludedBehaviors(const BehaviorContext& context) const;
    float GetPriorityModifier(BehaviorId behavior, const BehaviorContext& context) const;

    // Predefined rules
    void InitializeCombatRules();
    void InitializeMovementRules();
    void InitializeSocialRules();

private:
    std::vector<ExclusionRule> m_rules;
    std::array<std::array<bool, 7>, 7> m_categoryExclusions;

    // Rule validation
    bool ValidateRule(const ExclusionRule& rule) const;
    void ResolveConflicts();
};

// Example rule implementation
void MutualExclusionRules::InitializeCombatRules() {
    // Critical: Follow has 0.0 relevance during active combat
    AddRule({
        .name = "combat_disables_follow",
        .condition = [](const BehaviorContext& ctx) {
            return ctx.IsInCombat() && ctx.HasValidTarget();
        },
        .excludedBehaviors = {BEHAVIOR_FOLLOW_LEADER, BEHAVIOR_FOLLOW_GROUP},
        .priorityOverride = 0.0f
    });
}
```

### Task 2.4: Behavior Context Management
**Duration**: 8 hours
**Assigned Agents**:
- Primary: wow-mechanics-expert (game state)
- Support: trinity-integration-tester (API validation)
**Dependencies**: Task 2.3
**Deliverables**:
```cpp
// BehaviorContext.h
class BehaviorContext {
public:
    // Combat state
    bool IsInCombat() const { return m_inCombat; }
    bool HasValidTarget() const { return m_targetGuid.IsCreature() || m_targetGuid.IsPlayer(); }
    float GetTargetDistance() const { return m_targetDistance; }
    bool IsRangedClass() const { return m_isRangedClass; }

    // Group state
    bool IsInGroup() const { return m_inGroup; }
    bool IsGroupLeader() const { return m_isGroupLeader; }
    ObjectGuid GetLeaderGuid() const { return m_leaderGuid; }

    // Position state
    Position GetCurrentPosition() const { return m_position; }
    float GetDistanceToLeader() const { return m_leaderDistance; }

    // Resource state
    float GetHealthPercent() const { return m_healthPercent; }
    float GetPowerPercent() const { return m_powerPercent; }

    // Update methods
    void UpdateFromBot(Bot* bot);
    void InvalidateCache();

private:
    // Cached state
    bool m_inCombat{false};
    bool m_inGroup{false};
    bool m_isGroupLeader{false};
    bool m_isRangedClass{false};

    ObjectGuid m_targetGuid;
    ObjectGuid m_leaderGuid;

    float m_targetDistance{0.0f};
    float m_leaderDistance{0.0f};
    float m_healthPercent{100.0f};
    float m_powerPercent{100.0f};

    Position m_position;

    // Cache management
    std::chrono::steady_clock::time_point m_lastUpdate;
    static constexpr auto CACHE_DURATION = std::chrono::milliseconds(100);
};
```

### Task 2.5: State Transition Management
**Duration**: 10 hours
**Assigned Agents**:
- Primary: wow-bot-behavior-designer (state machines)
- Support: cpp-server-debugger (transition debugging)
**Dependencies**: Task 2.4
**Deliverables**:
```cpp
// BehaviorStateTransition.h
class BehaviorStateTransition {
public:
    struct Transition {
        BehaviorId from;
        BehaviorId to;
        std::function<bool(const BehaviorContext&)> condition;
        std::chrono::milliseconds minDuration;
        float smoothingFactor;
    };

    // Transition management
    void RegisterTransition(Transition transition);
    bool CanTransition(BehaviorId from, BehaviorId to, const BehaviorContext& context) const;

    // Smooth transitions
    float GetTransitionProgress(BehaviorId from, BehaviorId to) const;
    void StartTransition(BehaviorId from, BehaviorId to);
    void CompleteTransition();

    // State persistence
    void SaveState(BehaviorId current);
    BehaviorId GetLastStableState() const;

private:
    std::unordered_map<BehaviorId, std::vector<Transition>> m_transitions;

    struct ActiveTransition {
        BehaviorId from;
        BehaviorId to;
        std::chrono::steady_clock::time_point startTime;
        float progress;
    };

    std::optional<ActiveTransition> m_activeTransition;
    std::deque<BehaviorId> m_stateHistory;
    static constexpr size_t MAX_HISTORY = 10;
};
```

### Task 2.6: Performance Metrics Implementation
**Duration**: 6 hours
**Assigned Agents**:
- Primary: resource-monitor-limiter (metrics)
- Support: windows-memory-profiler (performance)
**Dependencies**: Task 2.5
**Deliverables**:
```cpp
// BehaviorMetrics.h
class BehaviorMetrics {
public:
    struct MetricSnapshot {
        uint32_t executionCount{0};
        std::chrono::microseconds totalExecutionTime{0};
        std::chrono::microseconds avgExecutionTime{0};
        std::chrono::microseconds maxExecutionTime{0};
        float successRate{0.0f};
        uint32_t failureCount{0};
        uint32_t timeoutCount{0};
    };

    // Metric collection
    void RecordExecution(BehaviorId id, std::chrono::microseconds duration, bool success);
    void RecordTimeout(BehaviorId id);
    void RecordTransition(BehaviorId from, BehaviorId to);

    // Metric retrieval
    MetricSnapshot GetSnapshot(BehaviorId id) const;
    std::vector<MetricSnapshot> GetAllSnapshots() const;

    // Performance analysis
    std::vector<BehaviorId> GetSlowBehaviors(std::chrono::microseconds threshold) const;
    std::vector<BehaviorId> GetFrequentlyFailingBehaviors(float threshold) const;

    // Reporting
    std::string GenerateReport() const;
    void ExportToJson(const std::filesystem::path& path) const;

private:
    struct BehaviorMetricData {
        std::atomic<uint32_t> executionCount{0};
        std::atomic<uint64_t> totalTimeUs{0};
        std::atomic<uint64_t> maxTimeUs{0};
        std::atomic<uint32_t> successCount{0};
        std::atomic<uint32_t> failureCount{0};
        std::atomic<uint32_t> timeoutCount{0};
    };

    std::unordered_map<BehaviorId, BehaviorMetricData> m_metrics;
    mutable std::shared_mutex m_metricMutex;
};
```

### Task 2.7: Combat Behavior Integration
**Duration**: 12 hours
**Assigned Agents**:
- Primary: wow-mechanics-expert (combat mechanics)
- Support: pvp-arena-tactician (PvP scenarios)
**Dependencies**: Tasks 2.1-2.6
**Deliverables**:
- Ranged combat behavior with proper distancing
- Melee combat behavior with facing requirements
- Threat management behavior integration
- Combat rotation optimization
- PvP-specific behavior adaptations

### Task 2.8: Movement Behavior Refactoring
**Duration**: 10 hours
**Assigned Agents**:
- Primary: wow-bot-behavior-designer (movement AI)
- Support: wow-dungeon-raid-coordinator (group movement)
**Dependencies**: Task 2.7
**Deliverables**:
- Follow behavior with combat suppression
- Formation movement for groups
- Pathfinding integration
- Obstacle avoidance
- Mount/dismount logic

### Task 2.9: Integration Testing Suite
**Duration**: 8 hours
**Assigned Agents**:
- Primary: test-automation-engineer (test design)
- Support: trinity-integration-tester (server testing)
**Dependencies**: Tasks 2.7-2.8
**Deliverables**:
```cpp
// BehaviorManagerTests.cpp
class BehaviorManagerTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_manager = std::make_unique<BehaviorManager>();
        m_mockBot = std::make_unique<MockBot>();
        InitializeTestBehaviors();
    }

    void TestCombatFollowExclusion() {
        // Verify follow has 0.0 relevance during combat
        m_mockBot->SetInCombat(true);
        m_mockBot->SetValidTarget(true);

        auto followRelevance = m_manager->GetBehaviorRelevance(BEHAVIOR_FOLLOW_LEADER);
        EXPECT_FLOAT_EQ(followRelevance, 0.0f);
    }

    void TestRangedCombatDistancing() {
        // Verify ranged classes maintain proper distance
        m_mockBot->SetClass(CLASS_HUNTER);
        m_mockBot->SetTargetDistance(5.0f); // Too close

        auto activeBehavior = m_manager->GetHighestPriorityBehavior();
        EXPECT_EQ(activeBehavior, BEHAVIOR_RANGED_POSITIONING);
    }

    void TestMeleeFacingRequirement() {
        // Verify melee classes face target correctly
        m_mockBot->SetClass(CLASS_WARRIOR);
        m_mockBot->SetFacingAngle(45.0f); // Not facing

        auto activeBehavior = m_manager->GetHighestPriorityBehavior();
        EXPECT_EQ(activeBehavior, BEHAVIOR_FACE_TARGET);
    }
};
```

### Task 2.10: Documentation and Migration Tools
**Duration**: 6 hours
**Assigned Agents**:
- Primary: code-quality-reviewer (documentation)
- Support: test-automation-engineer (migration testing)
**Dependencies**: Task 2.9
**Deliverables**:
- API documentation with examples
- Migration guide from old system
- Behavior configuration templates
- Troubleshooting guide

## Testing Strategy

### Unit Testing Requirements
- 100% coverage of BehaviorManager core functions
- All mutual exclusion rules validated
- State transition edge cases covered
- Performance benchmarks for all behaviors

### Integration Testing Requirements
- Combat scenario testing (ranged and melee)
- Group movement scenarios
- PvP combat situations
- Dungeon/raid behavior validation
- Cross-behavior conflict resolution

### Performance Testing Requirements
- Behavior execution: <100μs average
- Priority calculation: <10μs
- Memory usage: <1KB per behavior
- Zero memory leaks over 24-hour test

## Risk Mitigation

### Technical Risks
1. **Behavior Oscillation**: Implement hysteresis and minimum duration
2. **Priority Deadlock**: Timeout mechanisms and fallback behaviors
3. **Performance Degradation**: Caching and lazy evaluation
4. **Memory Leaks**: Smart pointers and RAII throughout

### Integration Risks
1. **Legacy Behavior Compatibility**: Adapter pattern for old behaviors
2. **Core System Conflicts**: Event-based notifications instead of polling
3. **Database State Corruption**: Transactional updates only

## Success Criteria

### Functional Requirements
- ✅ Follow behavior has 0.0 relevance during combat
- ✅ Ranged classes maintain 20-30 yard distance
- ✅ Melee classes maintain proper facing
- ✅ No behavior conflicts or oscillation
- ✅ Smooth state transitions

### Performance Requirements
- ✅ <100μs average behavior execution
- ✅ <0.05% CPU usage per bot
- ✅ <1MB memory per bot behavior system
- ✅ Zero frame drops in combat

### Quality Requirements
- ✅ 100% unit test coverage
- ✅ Zero memory leaks
- ✅ Full API documentation
- ✅ CLAUDE.md compliance

## Agent Coordination Matrix

| Task | Primary Agent | Support Agents | Review Agent |
|------|--------------|----------------|--------------|
| 2.1 | cpp-architecture-optimizer | wow-bot-behavior-designer | code-quality-reviewer |
| 2.2 | cpp-architecture-optimizer | concurrency-threading-specialist | cpp-server-debugger |
| 2.3 | wow-bot-behavior-designer | code-quality-reviewer | cpp-architecture-optimizer |
| 2.4 | wow-mechanics-expert | trinity-integration-tester | wow-bot-behavior-designer |
| 2.5 | wow-bot-behavior-designer | cpp-server-debugger | code-quality-reviewer |
| 2.6 | resource-monitor-limiter | windows-memory-profiler | test-automation-engineer |
| 2.7 | wow-mechanics-expert | pvp-arena-tactician | trinity-integration-tester |
| 2.8 | wow-bot-behavior-designer | wow-dungeon-raid-coordinator | cpp-architecture-optimizer |
| 2.9 | test-automation-engineer | trinity-integration-tester | code-quality-reviewer |
| 2.10 | code-quality-reviewer | test-automation-engineer | cpp-architecture-optimizer |

## Rollback Strategy

### Phase 2 Rollback Points
1. **Pre-Integration**: Before connecting to BotAI
2. **Post-Unit-Test**: After unit testing passes
3. **Post-Integration**: After integration with Phase 1
4. **Production**: After performance validation

### Rollback Procedure
```bash
# Automated rollback script
git tag phase2-checkpoint-1  # Before integration
git tag phase2-checkpoint-2  # After unit tests
git tag phase2-checkpoint-3  # After integration
git tag phase2-complete      # Production ready

# Rollback command
git checkout phase2-checkpoint-[N]
```

## Deliverables Checklist

### Code Deliverables
- [ ] BehaviorManager.h/cpp (850 lines)
- [ ] BehaviorPriority.h (100 lines)
- [ ] BehaviorContext.h/cpp (350 lines)
- [ ] MutualExclusionRules.h/cpp (350 lines)
- [ ] BehaviorStateTransition.h/cpp (300 lines)
- [ ] BehaviorMetrics.h/cpp (250 lines)

### Test Deliverables
- [ ] Unit tests (1500+ lines)
- [ ] Integration tests (500+ lines)
- [ ] Performance benchmarks
- [ ] Stress test scenarios

### Documentation Deliverables
- [ ] API reference documentation
- [ ] Migration guide
- [ ] Configuration examples
- [ ] Troubleshooting guide

## Phase 2 Complete Validation

### Exit Criteria
1. All deliverables complete and reviewed
2. Issues #2 and #3 verified fixed
3. Performance targets met
4. Zero regression in existing functionality
5. Full documentation delivered

**Estimated Completion**: 70 hours (mid-range of 60-80 hour estimate)
**Confidence Level**: 95% (proven architecture patterns)