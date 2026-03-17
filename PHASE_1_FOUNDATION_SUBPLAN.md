# PHASE 1: FOUNDATION - ENTERPRISE IMPLEMENTATION SUBPLAN

**Version**: 1.0.0
**Status**: READY FOR EXECUTION
**Total Estimated Hours**: 85 hours (2.1 weeks @ 40hrs/week)
**Total Lines of Code**: ~2,100 lines production + ~3,000 lines tests

## EXECUTIVE SUMMARY

Phase 1 establishes the architectural foundation for the PlayerBot refactoring, implementing a robust state machine framework with comprehensive safety guarantees. This foundation will support 5000 concurrent bots with <0.05% CPU overhead per bot through lock-free designs and cache-optimized data structures.

## 1. IMPLEMENTATION ORDER & TASK BREAKDOWN

### Critical Path Tasks (Must Complete in Order)

#### TASK 1.1: Core State Machine Types & Enums
**ID**: FSM-001
**Duration**: 3 hours
**Agent**: cpp-architecture-optimizer (Opus)
**Dependencies**: None
**Files to Create**:
- `src/modules/Playerbot/Core/StateMachine/BotStateTypes.h` (200 lines)

**Acceptance Criteria**:
- ✅ All bot states enumerated with proper categorization
- ✅ State transition matrix defined
- ✅ Event types comprehensive (30+ event types)
- ✅ Priority levels implemented (0-255 range)
- ✅ Thread-safe atomic types used

**Implementation**:
```cpp
// BotStateTypes.h
#pragma once
#include <atomic>
#include <cstdint>
#include <string_view>
#include <array>

namespace Playerbot::StateMachine {

// Primary bot states - mutually exclusive
enum class BotState : uint8_t {
    // Initialization states (0-15)
    UNINITIALIZED     = 0,
    INITIALIZING      = 1,
    LOADING_DATA      = 2,
    VALIDATING        = 3,

    // Operational states (16-31)
    IDLE              = 16,
    FOLLOWING         = 17,
    COMBAT            = 18,
    CASTING           = 19,
    LOOTING           = 20,
    TRADING           = 21,

    // Movement states (32-47)
    MOVING_TO_TARGET  = 32,
    PATHING           = 33,
    EVADING           = 34,
    FLEEING           = 35,

    // Group states (48-63)
    WAITING_FOR_GROUP = 48,
    FORMING_GROUP     = 49,
    IN_INSTANCE       = 50,

    // Error states (240-255)
    ERROR             = 240,
    RECOVERING        = 241,
    SHUTDOWN          = 254,
    TERMINATED        = 255
};

// State flags - can be combined
enum class StateFlags : uint32_t {
    NONE              = 0,
    IN_COMBAT         = (1 << 0),
    IS_DEAD           = (1 << 1),
    IS_CASTING        = (1 << 2),
    IS_MOVING         = (1 << 3),
    HAS_TARGET        = (1 << 4),
    IN_GROUP          = (1 << 5),
    IS_LEADER         = (1 << 6),
    NEEDS_MANA        = (1 << 7),
    NEEDS_HEALTH      = (1 << 8),
    HAS_AGGRO         = (1 << 9),
    IS_MOUNTED        = (1 << 10),
    IN_STEALTH        = (1 << 11),
    IS_FLYING         = (1 << 12),
    PVP_FLAGGED       = (1 << 13),
    IN_BATTLEGROUND   = (1 << 14),
    IN_ARENA          = (1 << 15)
};

// Event types that trigger state transitions
enum class EventType : uint16_t {
    // System events (0-31)
    SYSTEM_STARTUP    = 0,
    SYSTEM_SHUTDOWN   = 1,
    SYSTEM_ERROR      = 2,

    // Combat events (32-63)
    COMBAT_START      = 32,
    COMBAT_END        = 33,
    THREAT_GAINED     = 34,
    THREAT_LOST       = 35,
    DAMAGE_TAKEN      = 36,
    DAMAGE_DEALT      = 37,
    TARGET_DIED       = 38,
    TARGET_CHANGED    = 39,

    // Movement events (64-95)
    MOVEMENT_START    = 64,
    MOVEMENT_STOP     = 65,
    PATH_COMPLETE     = 66,
    PATH_FAILED       = 67,
    COLLISION         = 68,

    // Group events (96-127)
    GROUP_INVITE      = 96,
    GROUP_JOIN        = 97,
    GROUP_LEAVE       = 98,
    GROUP_DISBAND     = 99,
    LEADER_CHANGED    = 100,

    // Custom events (1000+)
    CUSTOM_BASE       = 1000
};

// Priority for state transitions (higher = more important)
using Priority = uint8_t;
constexpr Priority PRIORITY_CRITICAL = 255;
constexpr Priority PRIORITY_HIGH = 192;
constexpr Priority PRIORITY_NORMAL = 128;
constexpr Priority PRIORITY_LOW = 64;
constexpr Priority PRIORITY_IDLE = 0;

// Thread-safe state container
struct StateInfo {
    std::atomic<BotState> current{BotState::UNINITIALIZED};
    std::atomic<BotState> previous{BotState::UNINITIALIZED};
    std::atomic<StateFlags> flags{StateFlags::NONE};
    std::atomic<uint64_t> transitionCount{0};
    std::atomic<uint64_t> lastTransitionTime{0};
    std::atomic<Priority> currentPriority{PRIORITY_IDLE};
};

// State transition validation result
struct TransitionResult {
    bool allowed;
    BotState newState;
    std::string_view reason;
    Priority priority;
};

// State metadata for runtime introspection
struct StateMetadata {
    std::string_view name;
    std::string_view description;
    Priority defaultPriority;
    uint32_t allowedFlags;
    bool isTerminal;
    bool isError;
};

// Compile-time state metadata table
constexpr std::array<StateMetadata, 256> STATE_METADATA = {{
    {"UNINITIALIZED", "Bot not yet initialized", PRIORITY_IDLE, 0, false, false},
    {"INITIALIZING", "Bot initialization in progress", PRIORITY_HIGH, 0, false, false},
    // ... (complete for all states)
}};

} // namespace Playerbot::StateMachine
```

---

#### TASK 1.2: State Transition Matrix & Validators
**ID**: FSM-002
**Duration**: 5 hours
**Agent**: cpp-architecture-optimizer (Opus)
**Dependencies**: FSM-001
**Files to Create**:
- `src/modules/Playerbot/Core/StateMachine/StateTransitions.h` (150 lines)
- `src/modules/Playerbot/Core/StateMachine/StateValidators.h` (150 lines)
- `src/modules/Playerbot/Core/StateMachine/StateValidators.cpp` (300 lines)

**Acceptance Criteria**:
- ✅ Complete state transition matrix (all valid transitions)
- ✅ O(1) transition validation
- ✅ Thread-safe validation functions
- ✅ Comprehensive unit tests (100% coverage)
- ✅ Performance: <100ns per validation

**Implementation**:
```cpp
// StateTransitions.h
#pragma once
#include "BotStateTypes.h"
#include <bitset>

namespace Playerbot::StateMachine {

// Compile-time transition matrix using bitsets for O(1) lookup
class TransitionMatrix {
public:
    static constexpr size_t STATE_COUNT = 256;
    using TransitionBits = std::bitset<STATE_COUNT>;

    // Check if transition is valid in O(1)
    [[nodiscard]] static constexpr bool IsValidTransition(
        BotState from, BotState to) noexcept {
        return VALID_TRANSITIONS[static_cast<uint8_t>(from)]
            .test(static_cast<uint8_t>(to));
    }

    // Get all valid transitions from a state
    [[nodiscard]] static constexpr TransitionBits GetValidTransitions(
        BotState from) noexcept {
        return VALID_TRANSITIONS[static_cast<uint8_t>(from)];
    }

private:
    // Compile-time generation of transition matrix
    static constexpr std::array<TransitionBits, STATE_COUNT>
        GenerateTransitionMatrix() noexcept {
        std::array<TransitionBits, STATE_COUNT> matrix{};

        // Define all valid transitions
        // UNINITIALIZED -> INITIALIZING
        matrix[0].set(1);

        // INITIALIZING -> LOADING_DATA, ERROR
        matrix[1].set(2);
        matrix[1].set(240);

        // LOADING_DATA -> VALIDATING, ERROR
        matrix[2].set(3);
        matrix[2].set(240);

        // VALIDATING -> IDLE, ERROR
        matrix[3].set(16);
        matrix[3].set(240);

        // IDLE -> any operational state
        for (uint8_t i = 17; i <= 50; ++i) {
            matrix[16].set(i);
        }

        // Combat can transition to most states
        matrix[18].set(16);  // COMBAT -> IDLE
        matrix[18].set(20);  // COMBAT -> LOOTING
        matrix[18].set(34);  // COMBAT -> EVADING
        matrix[18].set(35);  // COMBAT -> FLEEING

        // Error recovery paths
        matrix[240].set(241); // ERROR -> RECOVERING
        matrix[241].set(16);  // RECOVERING -> IDLE
        matrix[241].set(254); // RECOVERING -> SHUTDOWN

        // Terminal states
        matrix[254].set(255); // SHUTDOWN -> TERMINATED

        return matrix;
    }

    static constexpr auto VALID_TRANSITIONS = GenerateTransitionMatrix();
};

// Priority-based transition rules
class TransitionRules {
public:
    // Check if transition should override current state based on priority
    [[nodiscard]] static bool ShouldOverride(
        Priority current, Priority incoming) noexcept {
        return incoming > current;
    }

    // Calculate transition priority based on event and state
    [[nodiscard]] static Priority CalculatePriority(
        EventType event, BotState currentState) noexcept;

    // Validate transition with context
    [[nodiscard]] static TransitionResult ValidateTransition(
        const StateInfo& state,
        BotState targetState,
        EventType event) noexcept;
};

} // namespace Playerbot::StateMachine
```

---

#### TASK 1.3: Core State Machine Implementation
**ID**: FSM-003
**Duration**: 8 hours
**Agent**: cpp-architecture-optimizer (Opus) + concurrency-threading-specialist (Opus)
**Dependencies**: FSM-001, FSM-002
**Files to Create**:
- `src/modules/Playerbot/Core/StateMachine/BotStateMachine.h` (300 lines)
- `src/modules/Playerbot/Core/StateMachine/BotStateMachine.cpp` (500 lines)

**Acceptance Criteria**:
- ✅ Lock-free state transitions using atomics
- ✅ Event queue with bounded capacity (1024 events)
- ✅ O(1) state queries
- ✅ Memory usage <1KB per state machine
- ✅ Thread-safe for 5000 concurrent instances
- ✅ Performance: <500ns per state transition

**Implementation**:
```cpp
// BotStateMachine.h
#pragma once
#include "BotStateTypes.h"
#include "StateTransitions.h"
#include <atomic>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>

namespace Playerbot::StateMachine {

// Forward declarations
class StateObserver;
class StateHistory;

// Lock-free state machine with event queue
class BotStateMachine {
public:
    using StateCallback = std::function<void(BotState, BotState)>;
    using EventHandler = std::function<void(EventType, const void*)>;
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    // Constructor/Destructor
    explicit BotStateMachine(ObjectGuid botGuid);
    ~BotStateMachine();

    // Non-copyable, non-movable (stateful)
    BotStateMachine(const BotStateMachine&) = delete;
    BotStateMachine& operator=(const BotStateMachine&) = delete;
    BotStateMachine(BotStateMachine&&) = delete;
    BotStateMachine& operator=(BotStateMachine&&) = delete;

    // State queries (lock-free, wait-free)
    [[nodiscard]] BotState GetCurrentState() const noexcept {
        return m_state.current.load(std::memory_order_acquire);
    }

    [[nodiscard]] BotState GetPreviousState() const noexcept {
        return m_state.previous.load(std::memory_order_acquire);
    }

    [[nodiscard]] StateFlags GetStateFlags() const noexcept {
        return m_state.flags.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool IsInState(BotState state) const noexcept {
        return GetCurrentState() == state;
    }

    [[nodiscard]] bool HasFlag(StateFlags flag) const noexcept {
        auto flags = GetStateFlags();
        return (static_cast<uint32_t>(flags) &
                static_cast<uint32_t>(flag)) != 0;
    }

    // State transitions (lock-free)
    [[nodiscard]] bool TransitionTo(
        BotState newState,
        EventType trigger = EventType::SYSTEM_STARTUP,
        Priority priority = PRIORITY_NORMAL);

    [[nodiscard]] bool TryTransitionTo(
        BotState newState,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // Event handling (lock-free queue)
    bool PostEvent(EventType event,
                   const void* data = nullptr,
                   Priority priority = PRIORITY_NORMAL);

    void ProcessEvents(uint32_t maxEvents = 10);

    // Flag management (atomic operations)
    void SetFlag(StateFlags flag) noexcept;
    void ClearFlag(StateFlags flag) noexcept;
    void ToggleFlag(StateFlags flag) noexcept;

    // Observer pattern for state changes
    void RegisterObserver(std::shared_ptr<StateObserver> observer);
    void UnregisterObserver(std::shared_ptr<StateObserver> observer);

    // Statistics and debugging
    [[nodiscard]] uint64_t GetTransitionCount() const noexcept {
        return m_state.transitionCount.load(std::memory_order_relaxed);
    }

    [[nodiscard]] TimePoint GetLastTransitionTime() const noexcept;
    [[nodiscard]] std::optional<StateHistory> GetHistory(size_t count) const;

    // Validation
    [[nodiscard]] bool ValidateState() const noexcept;
    [[nodiscard]] bool CanTransitionTo(BotState target) const noexcept;

private:
    // Core state storage (cache-line aligned)
    alignas(64) StateInfo m_state;

    // Bot identifier
    const ObjectGuid m_botGuid;

    // Lock-free event queue (MPSC)
    struct Event {
        EventType type;
        Priority priority;
        TimePoint timestamp;
        const void* data;
    };

    // Bounded lock-free queue implementation
    class EventQueue {
    public:
        static constexpr size_t CAPACITY = 1024;

        bool Push(const Event& event) noexcept;
        bool Pop(Event& event) noexcept;
        [[nodiscard]] bool IsEmpty() const noexcept;
        [[nodiscard]] size_t Size() const noexcept;

    private:
        alignas(64) std::atomic<size_t> m_head{0};
        alignas(64) std::atomic<size_t> m_tail{0};
        std::array<std::atomic<Event*>, CAPACITY> m_buffer{};
    };

    alignas(64) EventQueue m_eventQueue;

    // Observer management (RCU-style)
    mutable std::shared_mutex m_observerMutex;
    std::vector<std::weak_ptr<StateObserver>> m_observers;

    // State history ring buffer
    static constexpr size_t HISTORY_SIZE = 32;
    struct HistoryEntry {
        BotState state;
        EventType trigger;
        TimePoint timestamp;
        Priority priority;
    };

    alignas(64) std::array<HistoryEntry, HISTORY_SIZE> m_history;
    std::atomic<size_t> m_historyIndex{0};

    // Internal methods
    bool DoTransition(BotState newState, EventType trigger, Priority priority);
    void NotifyObservers(BotState oldState, BotState newState);
    void RecordHistory(BotState state, EventType trigger, Priority priority);
    void HandleEvent(const Event& event);

    // Performance counters
    alignas(64) struct {
        std::atomic<uint64_t> totalTransitions{0};
        std::atomic<uint64_t> failedTransitions{0};
        std::atomic<uint64_t> eventsProcessed{0};
        std::atomic<uint64_t> eventsDropped{0};
    } m_stats;
};

// State observer interface
class StateObserver {
public:
    virtual ~StateObserver() = default;
    virtual void OnStateChanged(ObjectGuid botGuid,
                                BotState oldState,
                                BotState newState) = 0;
    virtual void OnFlagChanged(ObjectGuid botGuid,
                               StateFlags oldFlags,
                               StateFlags newFlags) = 0;
};

} // namespace Playerbot::StateMachine
```

---

#### TASK 1.4: Initialization State Machine
**ID**: FSM-004
**Duration**: 6 hours
**Agent**: cpp-architecture-optimizer (Opus)
**Dependencies**: FSM-003
**Files to Create**:
- `src/modules/Playerbot/Core/StateMachine/BotInitStateMachine.h` (200 lines)
- `src/modules/Playerbot/Core/StateMachine/BotInitStateMachine.cpp` (400 lines)

**Acceptance Criteria**:
- ✅ Specialized initialization flow with checkpoints
- ✅ Rollback capability on failure
- ✅ Progress tracking (0-100%)
- ✅ Timeout handling for each phase
- ✅ Integration with TrinityCore Player initialization

---

#### TASK 1.5: Safe Reference System
**ID**: REF-001
**Duration**: 6 hours
**Agent**: concurrency-threading-specialist (Opus)
**Dependencies**: None (parallel with FSM tasks)
**Files to Create**:
- `src/modules/Playerbot/Core/Memory/SafeReference.h` (400 lines)
- `src/modules/Playerbot/Core/Memory/ReferenceValidator.h` (200 lines)
- `src/modules/Playerbot/Core/Memory/ReferenceValidator.cpp` (300 lines)

**Acceptance Criteria**:
- ✅ RAII-based reference management
- ✅ Automatic validation before access
- ✅ Zero overhead in release builds
- ✅ Thread-safe reference counting
- ✅ Integration with ObjectAccessor

---

#### TASK 1.6: Event System Framework
**ID**: EVT-001
**Duration**: 8 hours
**Agent**: concurrency-threading-specialist (Opus)
**Dependencies**: FSM-003
**Files to Create**:
- `src/modules/Playerbot/Core/Events/EventDispatcher.h` (300 lines)
- `src/modules/Playerbot/Core/Events/EventDispatcher.cpp` (500 lines)
- `src/modules/Playerbot/Core/Events/EventSubscriber.h` (150 lines)

**Acceptance Criteria**:
- ✅ Lock-free event dispatch
- ✅ Priority-based event processing
- ✅ Event batching for performance
- ✅ <1μs dispatch latency
- ✅ Support for 10,000 events/second

---

#### TASK 1.7: Behavior Interface Definitions
**ID**: BEH-001
**Duration**: 4 hours
**Agent**: cpp-architecture-optimizer (Opus)
**Dependencies**: FSM-003
**Files to Create**:
- `src/modules/Playerbot/Core/Behavior/IBehavior.h` (200 lines)
- `src/modules/Playerbot/Core/Behavior/BehaviorContext.h` (150 lines)
- `src/modules/Playerbot/Core/Behavior/BehaviorResult.h` (100 lines)

**Acceptance Criteria**:
- ✅ Pure virtual interfaces (no implementation)
- ✅ Context passing without allocation
- ✅ Result types for all outcomes
- ✅ Composable behavior chains
- ✅ Testability through mocking

---

#### TASK 1.8: State Machine Unit Tests
**ID**: TEST-001
**Duration**: 10 hours
**Agent**: test-automation-engineer (Sonnet)
**Dependencies**: FSM-001 through FSM-004
**Files to Create**:
- `src/modules/Playerbot/Tests/StateMachine/BotStateMachineTest.cpp` (800 lines)
- `src/modules/Playerbot/Tests/StateMachine/StateValidatorTest.cpp` (400 lines)
- `src/modules/Playerbot/Tests/StateMachine/TransitionMatrixTest.cpp` (300 lines)
- `src/modules/Playerbot/Tests/StateMachine/InitStateMachineTest.cpp` (500 lines)

**Acceptance Criteria**:
- ✅ 100% code coverage
- ✅ Thread safety tests (TSan clean)
- ✅ Performance benchmarks included
- ✅ Edge case coverage
- ✅ Mock implementations for dependencies

---

#### TASK 1.9: Safe Reference Unit Tests
**ID**: TEST-002
**Duration**: 6 hours
**Agent**: test-automation-engineer (Sonnet)
**Dependencies**: REF-001
**Files to Create**:
- `src/modules/Playerbot/Tests/Memory/SafeReferenceTest.cpp` (600 lines)
- `src/modules/Playerbot/Tests/Memory/ReferenceValidatorTest.cpp` (400 lines)

---

#### TASK 1.10: Event System Unit Tests
**ID**: TEST-003
**Duration**: 6 hours
**Agent**: test-automation-engineer (Sonnet)
**Dependencies**: EVT-001
**Files to Create**:
- `src/modules/Playerbot/Tests/Events/EventDispatcherTest.cpp` (600 lines)
- `src/modules/Playerbot/Tests/Events/EventSubscriberTest.cpp` (300 lines)

---

#### TASK 1.11: Integration Tests
**ID**: TEST-004
**Duration**: 8 hours
**Agent**: trinity-integration-tester (Sonnet)
**Dependencies**: All previous tasks
**Files to Create**:
- `src/modules/Playerbot/Tests/Integration/StateMachineIntegrationTest.cpp` (500 lines)
- `src/modules/Playerbot/Tests/Integration/EventSystemIntegrationTest.cpp` (400 lines)

---

#### TASK 1.12: Performance Profiling & Optimization
**ID**: PERF-001
**Duration**: 10 hours
**Agent**: windows-memory-profiler (Sonnet) + resource-monitor-limiter (Sonnet)
**Dependencies**: All implementation tasks
**Files to Create**:
- `src/modules/Playerbot/Tests/Performance/StateMachineBenchmark.cpp` (400 lines)
- `src/modules/Playerbot/Tests/Performance/MemoryProfileTest.cpp` (300 lines)

**Acceptance Criteria**:
- ✅ Memory usage <1KB per state machine
- ✅ State transition <500ns
- ✅ Event dispatch <1μs
- ✅ No memory leaks (Valgrind/ASan clean)
- ✅ 5000 concurrent instances stable

---

#### TASK 1.13: Documentation & Code Review
**ID**: DOC-001
**Duration**: 6 hours
**Agent**: code-quality-reviewer (Sonnet)
**Dependencies**: All tasks
**Files to Create**:
- `src/modules/Playerbot/Docs/StateMachineArchitecture.md` (500 lines)
- `src/modules/Playerbot/Docs/API/Phase1API.md` (400 lines)

---

## 2. COMPONENT SPECIFICATIONS

### 2.1 BotStateMachine Component

**Purpose**: Core state management with lock-free transitions and event processing

**Responsibility**:
- Maintain current bot state atomically
- Validate and execute state transitions
- Process events with priority ordering
- Notify observers of state changes
- Track state history for debugging

**Public API**:
```cpp
class BotStateMachine {
public:
    // State queries - O(1), lock-free
    BotState GetCurrentState() const noexcept;
    bool IsInState(BotState state) const noexcept;
    bool HasFlag(StateFlags flag) const noexcept;

    // State transitions - lock-free
    bool TransitionTo(BotState newState, EventType trigger, Priority priority);
    bool CanTransitionTo(BotState target) const noexcept;

    // Event handling
    bool PostEvent(EventType event, const void* data, Priority priority);
    void ProcessEvents(uint32_t maxEvents);

    // Observer pattern
    void RegisterObserver(std::shared_ptr<StateObserver> observer);
};
```

**Key Algorithms**:
- Lock-free MPSC queue for events (Dmitry Vyukov's algorithm)
- RCU-style observer management
- Cache-line aligned data structures
- Memory-ordered atomic operations

**TrinityCore 11.2 APIs**:
- `ObjectGuid` for bot identification
- `Player*` validation through `ObjectAccessor`
- `sWorld->GetGameTime()` for timestamps
- Thread primitives from `Common.h`

**Performance Requirements**:
- State transition: <500ns
- Memory per instance: <1KB
- Cache misses: <2 per transition
- Lock contention: 0 (lock-free)

**Error Handling**:
- Invalid transitions return false (no exceptions)
- Event queue overflow drops lowest priority
- Null checks on all external pointers
- Graceful degradation on memory pressure

**Thread Safety**:
- All public methods thread-safe
- Lock-free algorithms throughout
- Memory ordering specified explicitly
- No data races (TSan verified)

### 2.2 SafeReference Component

**Purpose**: RAII-based safe pointer management with automatic validation

**Responsibility**:
- Wrap TrinityCore pointers safely
- Validate before every access
- Automatic cleanup on scope exit
- Thread-safe reference counting
- Zero overhead in release mode

**Public API**:
```cpp
template<typename T>
class SafeRef {
public:
    explicit SafeRef(T* ptr);
    SafeRef(ObjectGuid guid);

    // Access with validation
    T* operator->() const;
    T& operator*() const;
    explicit operator bool() const noexcept;

    // Explicit operations
    bool IsValid() const noexcept;
    void Reset(T* ptr = nullptr);
    T* Release() noexcept;
};
```

**TrinityCore 11.2 APIs**:
- `ObjectAccessor::GetPlayer()`
- `ObjectAccessor::GetCreature()`
- `Map::GetPlayer()`
- GUID validation APIs

### 2.3 EventDispatcher Component

**Purpose**: High-performance event routing with priority and batching

**Responsibility**:
- Route events to subscribers
- Priority-based processing
- Event batching for efficiency
- Lock-free dispatch
- Subscriber lifecycle management

**Public API**:
```cpp
class EventDispatcher {
public:
    // Event posting
    void Post(EventType type, const void* data, Priority priority);
    void PostDelayed(EventType type, const void* data, uint32_t delayMs);

    // Subscription management
    SubscriptionId Subscribe(EventType type, EventHandler handler);
    void Unsubscribe(SubscriptionId id);

    // Processing
    void ProcessEvents(uint32_t maxEvents, uint32_t maxTimeMs);
    void ProcessPriority(Priority minPriority);
};
```

## 3. TESTING STRATEGY

### 3.1 Unit Test Requirements

**Per Component Test Coverage**:
- BotStateMachine: 50 tests minimum
  - All state transitions (30 tests)
  - Flag operations (5 tests)
  - Event queue operations (10 tests)
  - Edge cases (5 tests)

- SafeReference: 30 tests minimum
  - Valid/invalid pointers (10 tests)
  - Thread safety (10 tests)
  - RAII behavior (5 tests)
  - Performance benchmarks (5 tests)

- EventDispatcher: 40 tests minimum
  - Event routing (15 tests)
  - Priority handling (10 tests)
  - Subscription management (10 tests)
  - Performance under load (5 tests)

### 3.2 Integration Test Scenarios

1. **Multi-Bot State Coordination** (10 scenarios)
   - 100 bots transitioning simultaneously
   - Cascade state changes through events
   - Group state synchronization

2. **Memory Pressure Testing** (5 scenarios)
   - 5000 state machines concurrent
   - Memory allocation failures
   - Reference invalidation cascades

3. **Event Storm Handling** (5 scenarios)
   - 100,000 events/second
   - Priority inversion scenarios
   - Queue overflow recovery

### 3.3 Performance Benchmarks

**Required Benchmarks**:
```cpp
BENCHMARK(BM_StateTransition)->Range(1, 5000);
BENCHMARK(BM_EventDispatch)->Range(100, 100000);
BENCHMARK(BM_SafeRefAccess)->Range(1, 10000);
BENCHMARK(BM_ConcurrentTransitions)->Threads(16);
```

**Target Metrics**:
- State transition: <500ns (p99)
- Event dispatch: <1μs (p99)
- Memory per bot: <1KB
- CPU per bot: <0.05%

### 3.4 Mock/Stub Requirements

**Required Mocks**:
- `MockPlayer` - TrinityCore Player interface
- `MockObjectAccessor` - GUID lookups
- `MockMap` - Map operations
- `MockWorld` - Global state

## 4. CLAUDE.md COMPLIANCE CHECKLIST

### For Each Component

#### ✅ Complete Implementation
- [ ] No TODO comments
- [ ] No placeholder functions
- [ ] All error paths handled
- [ ] Full algorithm implementation
- [ ] Documentation complete

#### ✅ Module-Only Code
- [ ] All code in src/modules/Playerbot/
- [ ] No core file modifications
- [ ] Hook pattern for integration
- [ ] Clean module boundaries

#### ✅ TrinityCore 11.2 APIs
- [ ] All APIs verified to exist
- [ ] Correct parameter types
- [ ] Return value handling
- [ ] Version compatibility checked

#### ✅ Error Handling
- [ ] All pointers validated
- [ ] All returns checked
- [ ] Graceful degradation
- [ ] No uncaught exceptions
- [ ] Logging on errors

#### ✅ Performance
- [ ] Memory usage measured
- [ ] CPU usage profiled
- [ ] Cache efficiency verified
- [ ] Lock contention analyzed
- [ ] Benchmarks passing

## 5. RISK MITIGATION

### 5.1 Potential Blockers

**Risk 1: TrinityCore API Changes**
- **Probability**: Medium
- **Impact**: High
- **Mitigation**: Version lock to specific commit, abstraction layer
- **Rollback**: Previous API wrapper version

**Risk 2: Lock-Free Algorithm Bugs**
- **Probability**: Medium
- **Impact**: Critical
- **Mitigation**: Extensive testing, formal verification tools
- **Rollback**: Mutex-based fallback implementation

**Risk 3: Memory Overhead Exceeds Target**
- **Probability**: Low
- **Impact**: High
- **Mitigation**: Early profiling, object pooling
- **Rollback**: Reduce history buffer, compress state

### 5.2 Validation Checkpoints

**Checkpoint 1 (After Task 1.3)**:
- Basic state machine functional
- Memory usage within bounds
- Thread safety verified

**Checkpoint 2 (After Task 1.7)**:
- All interfaces defined
- API stability confirmed
- Integration points identified

**Checkpoint 3 (After Task 1.11)**:
- All tests passing
- Performance targets met
- No memory leaks

**Checkpoint 4 (After Task 1.13)**:
- Code review complete
- Documentation approved
- Ready for Phase 2

### 5.3 Rollback Points

1. **Before Task 1.4**: Can revert to simpler state machine
2. **Before Task 1.8**: Can adjust architecture
3. **After Task 1.11**: Can optimize before Phase 2

## 6. CODE EXAMPLES

### 6.1 Complete BotStateMachine Usage

```cpp
// Example: Bot combat initialization
void BotCombatManager::EnterCombat(Player* bot, Unit* target) {
    // Get bot's state machine
    auto& fsm = GetStateMachine(bot->GetGUID());

    // Validate transition
    if (!fsm.CanTransitionTo(BotState::COMBAT)) {
        LOG_WARN("Cannot enter combat from state: {}",
                 fsm.GetCurrentState());
        return;
    }

    // Set combat flags
    fsm.SetFlag(StateFlags::IN_COMBAT);
    fsm.SetFlag(StateFlags::HAS_TARGET);

    // Transition with high priority
    bool success = fsm.TransitionTo(
        BotState::COMBAT,
        EventType::COMBAT_START,
        PRIORITY_HIGH
    );

    if (success) {
        // Post target acquisition event
        CombatData data{target->GetGUID(), bot->GetDistance(target)};
        fsm.PostEvent(EventType::TARGET_CHANGED, &data, PRIORITY_HIGH);

        // Process immediate events
        fsm.ProcessEvents(5);
    }
}
```

### 6.2 SafeReference Pattern

```cpp
// Safe bot access pattern
class BotGroupManager {
public:
    void UpdateGroupPosition(ObjectGuid leaderGuid) {
        // Safe reference with automatic validation
        SafeRef<Player> leader(leaderGuid);
        if (!leader) {
            LOG_DEBUG("Leader {} not found", leaderGuid.ToString());
            return;
        }

        // Access is validated on each dereference
        Position leaderPos = leader->GetPosition();
        float orientation = leader->GetOrientation();

        // Iterate group members safely
        for (auto& botGuid : m_groupBots) {
            SafeRef<Player> bot(botGuid);
            if (bot && bot->IsAlive()) {
                // Calculate formation position
                Position formationPos = CalculateFormation(
                    leaderPos, orientation, bot->GetFormationIndex()
                );

                // Move bot (reference validated automatically)
                bot->GetMotionMaster()->MovePoint(
                    POINT_FORMATION, formationPos
                );
            }
        }
    } // References cleaned up automatically
};
```

### 6.3 Event Subscription Pattern

```cpp
// Event-driven behavior
class BotThreatManager : public EventSubscriber {
public:
    BotThreatManager(EventDispatcher& dispatcher)
        : m_dispatcher(dispatcher) {
        // Subscribe to combat events
        m_subs.push_back(
            dispatcher.Subscribe(EventType::THREAT_GAINED,
                [this](const void* data) { OnThreatGained(data); })
        );
        m_subs.push_back(
            dispatcher.Subscribe(EventType::COMBAT_START,
                [this](const void* data) { OnCombatStart(data); })
        );
    }

private:
    void OnThreatGained(const void* data) {
        auto* threatData = static_cast<const ThreatData*>(data);
        SafeRef<Player> bot(threatData->botGuid);

        if (bot && bot->IsAlive()) {
            // Update threat table
            UpdateThreat(bot.get(), threatData->sourceGuid,
                       threatData->amount);

            // Post state change if needed
            if (GetTotalThreat(bot.get()) > FLEE_THRESHOLD) {
                m_dispatcher.Post(EventType::MOVEMENT_START,
                                 nullptr, PRIORITY_HIGH);
            }
        }
    }
};
```

## 7. DELIVERY ARTIFACTS

### Phase 1 Completion Criteria

1. **Source Files** (14 files, ~2,100 lines)
   - All state machine components
   - Safe reference system
   - Event framework
   - Full documentation

2. **Test Files** (15 files, ~3,000 lines)
   - Unit tests with 100% coverage
   - Integration tests passing
   - Performance benchmarks met
   - Memory leak free

3. **Documentation** (2 files, ~900 lines)
   - Architecture overview
   - API reference
   - Usage examples
   - Migration guide

4. **Metrics Dashboard**
   - Memory usage: <1KB/bot ✓
   - CPU usage: <0.05%/bot ✓
   - State transition: <500ns ✓
   - Event dispatch: <1μs ✓
   - Thread safety: TSan clean ✓

## 8. PHASE 2 PREPARATION

### Handoff Requirements

1. **Stable APIs** - No breaking changes after Phase 1
2. **Test Coverage** - Minimum 95% for all components
3. **Performance Baseline** - All benchmarks documented
4. **Known Issues** - Complete list with workarounds
5. **Integration Points** - Documented for Phase 2

### Dependencies for Phase 2

Phase 2 teams can begin work when:
- State machine API stable (Task 1.3 complete)
- Behavior interfaces defined (Task 1.7 complete)
- Event system functional (Task 1.6 complete)

## EXECUTION COMMAND

To begin Phase 1 implementation:

```bash
# Create branch
git checkout -b phase1-foundation

# Create directory structure
mkdir -p src/modules/Playerbot/Core/StateMachine
mkdir -p src/modules/Playerbot/Core/Memory
mkdir -p src/modules/Playerbot/Core/Events
mkdir -p src/modules/Playerbot/Core/Behavior
mkdir -p src/modules/Playerbot/Tests/StateMachine
mkdir -p src/modules/Playerbot/Tests/Memory
mkdir -p src/modules/Playerbot/Tests/Events
mkdir -p src/modules/Playerbot/Tests/Integration
mkdir -p src/modules/Playerbot/Tests/Performance
mkdir -p src/modules/Playerbot/Docs/API

# Begin with Task 1.1
# Agent: cpp-architecture-optimizer
# File: src/modules/Playerbot/Core/StateMachine/BotStateTypes.h
```

## APPROVAL GATE

This plan requires explicit approval before implementation begins.

**Approval Checklist**:
- [ ] Architecture approved
- [ ] Resource allocation confirmed
- [ ] Agent availability verified
- [ ] TrinityCore APIs validated
- [ ] Performance targets agreed

---

**END OF PHASE 1 FOUNDATION SUBPLAN**

*Total: 85 hours, 14 implementation files, 15 test files, 2 documentation files*
*Delivers: Complete foundation for Phases 2-6 with zero technical debt*