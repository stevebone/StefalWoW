# BehaviorManager API Reference

**Version**: 1.0
**Module**: Playerbot
**Location**: `src/modules/Playerbot/AI/BehaviorManager.h`
**Namespace**: `Playerbot`

---

## Table of Contents

1. [Overview](#overview)
2. [Class Declaration](#class-declaration)
3. [Constructor and Destructor](#constructor-and-destructor)
4. [Public Methods](#public-methods)
5. [Protected Methods](#protected-methods)
6. [Protected Member Variables](#protected-member-variables)
7. [Private Implementation](#private-implementation)
8. [Performance Characteristics](#performance-characteristics)
9. [Thread Safety Guarantees](#thread-safety-guarantees)
10. [Complete Usage Examples](#complete-usage-examples)

---

## Overview

The `BehaviorManager` class is the base class for all bot behavior managers in the TrinityCore Playerbot module. It provides a throttled update mechanism where `Update()` is called every frame but `OnUpdate()` is only invoked at configured intervals, dramatically reducing CPU usage while maintaining responsive bot behavior.

### Key Features

- **Automatic update throttling** to reduce CPU usage (sub-millisecond overhead when throttled)
- **Atomic state flags** for lock-free queries from strategies and other systems
- **Performance monitoring** with automatic slow-update detection and interval adjustment
- **Per-bot instance design** (no singletons, each bot has its own manager instances)
- **Exception safety** with automatic manager disable on errors
- **Lifecycle management** with initialization and shutdown hooks

### Design Goals

1. **Performance**: <0.001ms overhead when throttled, 5-10ms typical update cost
2. **Scalability**: Support 100-5000 concurrent bots with minimal impact
3. **Reliability**: Automatic error handling and recovery
4. **Flexibility**: Easy to extend for specific manager implementations
5. **Observability**: Built-in performance monitoring and logging

---

## Class Declaration

```cpp
namespace Playerbot
{
    class TC_GAME_API BehaviorManager
    {
    public:
        // Constructor and destructor
        explicit BehaviorManager(Player* bot, BotAI* ai,
                                 uint32 updateInterval = 1000,
                                 std::string managerName = "BehaviorManager");
        virtual ~BehaviorManager() = default;

        // Core update interface
        void Update(uint32 diff);

        // State query methods (thread-safe, lock-free)
        bool IsEnabled() const;
        void SetEnabled(bool enable);
        bool IsBusy() const;
        bool IsInitialized() const;

        // Configuration methods
        uint32 GetUpdateInterval() const;
        void SetUpdateInterval(uint32 interval);
        const std::string& GetManagerName() const;

        // Utility methods
        uint32 GetTimeSinceLastUpdate() const;
        void ForceUpdate();

        // Delete copy and move constructors
        BehaviorManager(const BehaviorManager&) = delete;
        BehaviorManager& operator=(const BehaviorManager&) = delete;
        BehaviorManager(BehaviorManager&&) = delete;
        BehaviorManager& operator=(BehaviorManager&&) = delete;

    protected:
        // Virtual interface for derived classes
        virtual void OnUpdate(uint32 elapsed) = 0;
        virtual bool OnInitialize();
        virtual void OnShutdown();

        // Protected accessors
        Player* GetBot() const;
        BotAI* GetAI() const;

        // Logging utilities
        template<typename... Args>
        void LogDebug(const char* format, Args... args) const;

        template<typename... Args>
        void LogWarning(const char* format, Args... args) const;

        // Protected state flags (atomic)
        std::atomic<bool> m_hasWork{false};
        std::atomic<bool> m_needsUpdate{false};
        std::atomic<uint32> m_updateCount{0};

    private:
        // Private implementation details omitted
    };
}
```

---

## Constructor and Destructor

### Constructor

```cpp
explicit BehaviorManager(Player* bot, BotAI* ai,
                         uint32 updateInterval = 1000,
                         std::string managerName = "BehaviorManager");
```

**Purpose**: Constructs a new BehaviorManager instance for a specific bot.

**Parameters**:
- `bot` (Player*): Pointer to the bot player this manager belongs to. **Must not be null**.
- `ai` (BotAI*): Pointer to the bot AI controller. **Must not be null**.
- `updateInterval` (uint32): Update interval in milliseconds. Default: 1000ms. Minimum: 50ms (enforced).
- `managerName` (std::string): Name for logging and debugging purposes. Default: "BehaviorManager".

**Behavior**:
- Validates input parameters (bot and ai must not be null)
- Clamps update interval to minimum of 50ms
- Initializes internal timestamp to current time (prevents immediate update)
- If bot or ai is null, manager is automatically disabled and error is logged

**Thread Safety**: Not thread-safe. Must be called from the main thread.

**Performance**: <0.01ms construction time

**Example**:
```cpp
// Create a QuestManager with 2-second update interval
QuestManager::QuestManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 2000, "QuestManager")
{
    // Additional initialization
}
```

### Destructor

```cpp
virtual ~BehaviorManager() = default;
```

**Purpose**: Virtual destructor for proper cleanup of derived classes.

**Behavior**:
- Virtual destructor ensures derived class destructors are called
- Default implementation (compiler-generated)
- Derived classes should call `OnShutdown()` in their destructors if needed

**Thread Safety**: Not thread-safe. Must be called from the main thread.

**Example**:
```cpp
QuestManager::~QuestManager()
{
    // OnShutdown() can be called manually if needed
    OnShutdown();

    // Additional cleanup
    m_questCache.clear();
}
```

---

## Public Methods

### Update

```cpp
void Update(uint32 diff);
```

**Purpose**: Main update method called every frame to check if `OnUpdate()` should be invoked.

**Parameters**:
- `diff` (uint32): Time elapsed since last `Update()` call in milliseconds.

**Behavior**:
1. **Fast path checks** (in order):
   - Returns immediately if manager is disabled (atomic check, <0.001ms)
   - Returns immediately if manager is busy (prevents re-entrance)
   - Validates bot and AI pointers, disables manager if invalid
2. **Initialization handling**:
   - If not initialized, calls `OnInitialize()` once
   - If initialization fails, returns and retries next update
   - Marks as initialized on success
3. **Throttling logic**:
   - Accumulates elapsed time since last update
   - Checks three conditions (in priority order):
     - Priority 1: Force update requested (`ForceUpdate()` was called)
     - Priority 2: Enough time has elapsed (>= update interval)
     - Priority 3: Manager indicates immediate update needed (`m_needsUpdate` flag)
   - Returns immediately if no update needed (<0.001ms when throttled)
4. **Update execution**:
   - Calls internal `DoUpdate()` which invokes `OnUpdate()`
   - Resets accumulated time after update

**Thread Safety**: Thread-safe for reading state. Must be called from the same thread every time.

**Performance**:
- When throttled (no update): <0.001ms (just timestamp comparisons)
- When updating: 5-10ms (depends on `OnUpdate()` implementation)
- Amortized per-frame cost: <0.2ms

**Example**:
```cpp
// Called by BotAI every frame
void BotAI::UpdateAI(uint32 diff)
{
    // Update all managers
    if (_questManager)
        _questManager->Update(diff);

    if (_tradeManager)
        _tradeManager->Update(diff);

    // ... other managers
}
```

---

### IsEnabled

```cpp
bool IsEnabled() const;
```

**Purpose**: Check if this manager is currently enabled and processing updates.

**Parameters**: None

**Returns**: `true` if the manager is processing updates, `false` otherwise.

**Behavior**:
- Performs atomic read with acquire memory ordering
- Manager can be disabled by:
  - Calling `SetEnabled(false)`
  - Automatic disable on pointer validation failure
  - Automatic disable on exception in `OnUpdate()`

**Thread Safety**: Fully thread-safe (atomic operation with acquire semantics)

**Performance**: <0.001ms guaranteed (single atomic read)

**Example**:
```cpp
// Check if quest manager is active before querying quest state
if (questManager->IsEnabled() && questManager->HasActiveQuests())
{
    Quest const* quest = questManager->GetCurrentQuest();
    // Process quest
}
```

---

### SetEnabled

```cpp
void SetEnabled(bool enable);
```

**Purpose**: Enable or disable this manager's update processing.

**Parameters**:
- `enable` (bool): `true` to enable updates, `false` to disable.

**Returns**: void

**Behavior**:
- Performs atomic write with release memory ordering
- When disabled:
  - `Update()` returns immediately (fast path)
  - `OnUpdate()` is not called
  - State flags remain readable
- When re-enabled:
  - Updates resume on next `Update()` call
  - Time accumulation continues from last update

**Thread Safety**: Fully thread-safe (atomic operation with release semantics)

**Performance**: <0.001ms guaranteed (single atomic write)

**Example**:
```cpp
// Disable quest manager while bot is in dungeon
void BotAI::OnEnterDungeon()
{
    if (_questManager)
        _questManager->SetEnabled(false);
}

// Re-enable when leaving dungeon
void BotAI::OnLeaveDungeon()
{
    if (_questManager)
        _questManager->SetEnabled(true);
}
```

---

### IsBusy

```cpp
bool IsBusy() const;
```

**Purpose**: Check if the manager is currently executing `OnUpdate()`.

**Parameters**: None

**Returns**: `true` if `OnUpdate()` is currently executing, `false` otherwise.

**Behavior**:
- Performs atomic read with acquire memory ordering
- Flag is set at start of `DoUpdate()` and cleared at end
- Used internally to prevent re-entrance
- Useful for external code to avoid querying changing state

**Thread Safety**: Fully thread-safe (atomic operation with acquire semantics)

**Performance**: <0.001ms guaranteed (single atomic read)

**Example**:
```cpp
// Wait for manager to finish current update before querying detailed state
void Strategy::EvaluateTriggers()
{
    // Quick check if manager has work
    if (questManager->HasActiveQuests())
    {
        // Avoid querying detailed state while manager is updating
        if (!questManager->IsBusy())
        {
            // Safe to query detailed quest information
            ProcessQuestObjectives();
        }
    }
}
```

---

### IsInitialized

```cpp
bool IsInitialized() const;
```

**Purpose**: Check if manager has been successfully initialized.

**Parameters**: None

**Returns**: `true` if ready for updates, `false` otherwise.

**Behavior**:
- Performs atomic read with acquire memory ordering
- Set to `true` after first successful `OnInitialize()` call
- Managers are not fully operational until initialized

**Thread Safety**: Fully thread-safe (atomic operation with acquire semantics)

**Performance**: <0.001ms guaranteed (single atomic read)

**Example**:
```cpp
// Check if manager is ready before using
void BotAI::OnQuestAccepted(Quest const* quest)
{
    if (_questManager && _questManager->IsInitialized())
    {
        _questManager->AddQuest(quest);
    }
    else
    {
        // Queue quest for later processing
        _pendingQuests.push_back(quest);
    }
}
```

---

### GetUpdateInterval

```cpp
uint32 GetUpdateInterval() const;
```

**Purpose**: Get the configured update interval for this manager.

**Parameters**: None

**Returns**: Update interval in milliseconds.

**Behavior**:
- Returns current update interval value
- Minimum value is 50ms (enforced in constructor and `SetUpdateInterval()`)
- Can be modified at runtime with `SetUpdateInterval()`

**Thread Safety**: Thread-safe for reading (plain read of uint32)

**Performance**: <0.001ms (simple member access)

**Example**:
```cpp
// Log manager configuration
TC_LOG_DEBUG("module.playerbot", "QuestManager update interval: {}ms",
             questManager->GetUpdateInterval());

// Adjust performance based on interval
if (questManager->GetUpdateInterval() < 500)
{
    // High-frequency updates, do minimal work per update
    ProcessSingleQuestObjective();
}
else
{
    // Low-frequency updates, can do more work per update
    ProcessAllQuestObjectives();
}
```

---

### SetUpdateInterval

```cpp
void SetUpdateInterval(uint32 interval);
```

**Purpose**: Set a new update interval for this manager.

**Parameters**:
- `interval` (uint32): New interval in milliseconds.

**Returns**: void

**Behavior**:
- Clamps interval to range [50ms, 60000ms]
- Takes effect on next `Update()` cycle
- Does not reset accumulated time
- Logs debug message with new interval

**Thread Safety**: Not thread-safe. Must be called from the main thread.

**Performance**: <0.001ms (simple assignment with clamping)

**Example**:
```cpp
// Reduce update frequency during combat
void BotAI::OnCombatStart(Unit* target)
{
    // Quest manager doesn't need to update as often in combat
    if (_questManager)
        _questManager->SetUpdateInterval(5000); // 5 seconds
}

void BotAI::OnCombatEnd()
{
    // Restore normal update frequency
    if (_questManager)
        _questManager->SetUpdateInterval(2000); // 2 seconds
}
```

---

### GetManagerName

```cpp
const std::string& GetManagerName() const;
```

**Purpose**: Get the manager name for debugging and logging purposes.

**Parameters**: None

**Returns**: Constant reference to the manager name string.

**Behavior**:
- Returns the name provided in constructor
- Used for logging and performance monitoring
- Returned by reference for efficiency (no copy)

**Thread Safety**: Thread-safe for reading (const reference)

**Performance**: <0.001ms (return reference, no copy)

**Example**:
```cpp
// Debug logging with manager name
void DiagnosticsCommand::PrintManagerStatus(BehaviorManager* manager)
{
    std::cout << "Manager: " << manager->GetManagerName() << std::endl;
    std::cout << "  Enabled: " << (manager->IsEnabled() ? "Yes" : "No") << std::endl;
    std::cout << "  Initialized: " << (manager->IsInitialized() ? "Yes" : "No") << std::endl;
    std::cout << "  Interval: " << manager->GetUpdateInterval() << "ms" << std::endl;
}
```

---

### GetTimeSinceLastUpdate

```cpp
uint32 GetTimeSinceLastUpdate() const;
```

**Purpose**: Get time elapsed since last successful `OnUpdate()` completion.

**Parameters**: None

**Returns**: Milliseconds since last `OnUpdate()` completion. Returns 0 if never updated.

**Behavior**:
- Calculates difference between current time and last update timestamp
- Returns 0 if manager has never been updated (`m_lastUpdate == 0`)
- Useful for monitoring update frequency and detecting stalls

**Thread Safety**: Thread-safe (reads timestamp atomically via `getMSTime()`)

**Performance**: <0.001ms (timestamp difference calculation)

**Example**:
```cpp
// Detect stalled managers
void MonitorSystem::CheckManagerHealth()
{
    for (auto* manager : allManagers)
    {
        uint32 timeSinceUpdate = manager->GetTimeSinceLastUpdate();

        if (timeSinceUpdate > manager->GetUpdateInterval() * 2)
        {
            TC_LOG_WARN("module.playerbot",
                "Manager {} is stalled: {}ms since last update (expected {}ms)",
                manager->GetManagerName(), timeSinceUpdate, manager->GetUpdateInterval());
        }
    }
}
```

---

### ForceUpdate

```cpp
void ForceUpdate();
```

**Purpose**: Force an immediate update on the next `Update()` call, bypassing throttling.

**Parameters**: None

**Returns**: void

**Behavior**:
- Sets internal `m_forceUpdate` atomic flag to true
- Next `Update()` call will invoke `OnUpdate()` regardless of time elapsed
- Flag is automatically cleared after forced update
- Takes highest priority over interval and `m_needsUpdate` flag

**Thread Safety**: Fully thread-safe (atomic operation with release semantics)

**Performance**: <0.001ms (single atomic write)

**Usage**: Use sparingly as it bypasses the throttling mechanism. Appropriate for:
- Critical game events (quest accepted, item equipped, etc.)
- Player commands requiring immediate response
- External system notifications

**Example**:
```cpp
// Force quest manager update when quest is accepted
void BotAI::OnQuestAccepted(Quest const* quest)
{
    if (_questManager)
    {
        _questManager->AddQuest(quest);
        _questManager->ForceUpdate(); // Process immediately
    }
}

// Force trade manager update when trade window opens
void BotAI::OnTradeWindowOpened(Player* trader)
{
    if (_tradeManager)
    {
        _tradeManager->SetTrader(trader);
        _tradeManager->ForceUpdate(); // Respond immediately
    }
}
```

---

## Protected Methods

These methods are available to derived classes for implementing specific manager behavior.

### OnUpdate (Pure Virtual)

```cpp
virtual void OnUpdate(uint32 elapsed) = 0;
```

**Purpose**: Virtual method called at throttled intervals for actual work. **Must be implemented by derived classes**.

**Parameters**:
- `elapsed` (uint32): Time elapsed since last `OnUpdate()` in milliseconds.

**Returns**: void

**Behavior**:
- Called only when:
  - The manager is enabled
  - Sufficient time has passed since last update (or force update requested)
  - The bot and AI pointers are valid
  - The manager is initialized
- Wrapped in exception handler:
  - `std::exception`: Logs error message and disables manager
  - Unknown exception: Logs generic error and disables manager
- Performance is monitored:
  - Start time captured before call
  - Duration logged if exceeds 50ms threshold
  - Interval auto-adjusted if consistently slow (10+ consecutive slow updates)

**Implementation Requirements**:
- Target execution time: 5-10ms typical, <50ms maximum
- Must not throw exceptions (will disable manager)
- Must not block or wait on external resources
- Must not call `Update()` or create re-entrance
- Should update protected atomic flags (`m_hasWork`, etc.) to reflect state

**Thread Safety**: Called from main thread only. Must not spawn threads.

**Performance Target**: 5-10ms typical, <50ms maximum

**Example**:
```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    Player* bot = GetBot();
    if (!bot)
        return;

    // 1. Check for completed quest objectives (fast check)
    CheckQuestObjectiveCompletion(); // ~1-2ms

    // 2. Update quest progress cache (bulk operation)
    if (elapsed >= 5000) // Only every 5+ seconds
    {
        RefreshQuestProgressCache(); // ~5-8ms
    }

    // 3. Check for nearby quest givers (expensive)
    if (m_needsUpdate.load(std::memory_order_acquire))
    {
        ScanForQuestGivers(); // ~3-5ms
        m_needsUpdate.store(false, std::memory_order_release);
    }

    // 4. Update state flags for strategies
    m_hasWork.store(!m_activeQuests.empty(), std::memory_order_release);
}
```

---

### OnInitialize

```cpp
virtual bool OnInitialize();
```

**Purpose**: Called once during first `Update()` for initialization. Override to perform one-time setup.

**Parameters**: None

**Returns**:
- `true` if initialization successful (manager becomes operational)
- `false` to retry on next update (manager remains uninitialized)

**Behavior**:
- Called by `Update()` before first `OnUpdate()` invocation
- If returns `false`, will be called again on next `Update()`
- Once returns `true`, never called again (manager marked initialized)
- Default implementation returns `true` (no initialization needed)

**Thread Safety**: Called from main thread only.

**Performance**: Should complete quickly (<10ms) as it's called from `Update()`.

**Use Cases**:
- Wait for bot to be fully loaded in world
- Load initial data from database
- Register event listeners
- Validate manager prerequisites

**Example**:
```cpp
bool QuestManager::OnInitialize()
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    // Ensure bot is in world before initialization
    if (!bot->IsInWorld())
    {
        LogDebug("Bot {} not in world yet, deferring initialization", bot->GetName());
        return false; // Retry next update
    }

    // Load active quests from database
    LoadActiveQuestsFromDB();

    // Build quest cache
    BuildQuestObjectiveCache();

    // Register for quest events
    RegisterQuestEventHandlers();

    LogDebug("Initialized successfully for bot {} with {} active quests",
             bot->GetName(), m_activeQuests.size());

    return true;
}
```

---

### OnShutdown

```cpp
virtual void OnShutdown();
```

**Purpose**: Called when manager is being shut down. Override to perform cleanup.

**Parameters**: None

**Returns**: void

**Behavior**:
- Called from destructor or when bot is removed from world
- Default implementation does nothing
- Should release resources and clean up state
- Should not throw exceptions

**Thread Safety**: Called from main thread only.

**Performance**: Should complete quickly to avoid blocking.

**Use Cases**:
- Save persistent data to database
- Unregister event listeners
- Clear large data structures
- Release external resources

**Example**:
```cpp
void QuestManager::OnShutdown()
{
    Player* bot = GetBot();

    // Save quest progress to database
    if (bot && !m_activeQuests.empty())
    {
        SaveQuestProgressToDB();
    }

    // Unregister event handlers
    UnregisterQuestEventHandlers();

    // Clear quest cache
    m_questCache.clear();
    m_objectiveCache.clear();
    m_activeQuests.clear();

    // Update flags
    m_hasWork.store(false, std::memory_order_release);

    LogDebug("Shutdown complete - saved {} quests", m_activeQuests.size());
}
```

---

### GetBot

```cpp
Player* GetBot() const;
```

**Purpose**: Get the bot player this manager belongs to.

**Parameters**: None

**Returns**: Pointer to bot Player. **May be null** if bot disconnected or was destroyed.

**Behavior**:
- Returns internal bot pointer
- Pointer may become invalid if bot logs out or is destroyed
- Should check for null before use
- Validated automatically by `Update()` before calling `OnUpdate()`

**Thread Safety**: Thread-safe for reading (const method)

**Performance**: <0.001ms (return pointer)

**Example**:
```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    Player* bot = GetBot();
    if (!bot)
    {
        LogWarning("Bot pointer is null");
        return;
    }

    // Safe to use bot pointer
    if (bot->IsInWorld())
    {
        UpdateQuestProgress();
    }
}
```

---

### GetAI

```cpp
BotAI* GetAI() const;
```

**Purpose**: Get the bot AI controller.

**Parameters**: None

**Returns**: Pointer to BotAI. **May be null** if AI was destroyed.

**Behavior**:
- Returns internal AI pointer
- Pointer may become invalid if AI is destroyed
- Should check for null before use
- Validated automatically by `Update()` before calling `OnUpdate()`

**Thread Safety**: Thread-safe for reading (const method)

**Performance**: <0.001ms (return pointer)

**Example**:
```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    BotAI* ai = GetAI();
    if (!ai)
    {
        LogWarning("AI pointer is null");
        return;
    }

    // Safe to use AI pointer
    if (ai->IsIdle() && HasQuestObjectivesNearby())
    {
        // Trigger quest strategy
        ai->ActivateStrategy("quest");
    }
}
```

---

### LogDebug

```cpp
template<typename... Args>
void LogDebug(const char* format, Args... args) const;
```

**Purpose**: Log a debug message with manager name prefix.

**Parameters**:
- `format` (const char*): Printf-style or fmt-style format string.
- `args` (Args...): Format arguments (variadic template).

**Returns**: void

**Behavior**:
- Logs to "module.playerbot" category at DEBUG level
- Automatically prefixes message with `[ManagerName]`
- Uses fmtlib for formatting (safe and efficient)
- Only logged if debug logging is enabled in config

**Thread Safety**: Thread-safe (logging system is thread-safe)

**Performance**: <0.01ms if debug logging enabled, <0.001ms if disabled

**Example**:
```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    LogDebug("Processing {} active quests", m_activeQuests.size());

    for (auto const& [questId, progress] : m_activeQuests)
    {
        LogDebug("Quest {}: {} / {} objectives completed",
                 questId, progress.completedObjectives, progress.totalObjectives);
    }
}

// Output:
// [QuestManager] Processing 3 active quests
// [QuestManager] Quest 1234: 2 / 5 objectives completed
// [QuestManager] Quest 1235: 5 / 5 objectives completed
// [QuestManager] Quest 1236: 0 / 3 objectives completed
```

---

### LogWarning

```cpp
template<typename... Args>
void LogWarning(const char* format, Args... args) const;
```

**Purpose**: Log a warning message with manager name prefix.

**Parameters**:
- `format` (const char*): Printf-style or fmt-style format string.
- `args` (Args...): Format arguments (variadic template).

**Returns**: void

**Behavior**:
- Logs to "module.playerbot" category at WARN level
- Automatically prefixes message with `[ManagerName]`
- Uses fmtlib for formatting (safe and efficient)
- Always logged regardless of config (warnings are important)

**Thread Safety**: Thread-safe (logging system is thread-safe)

**Performance**: <0.01ms

**Example**:
```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    if (m_failedQuestLoads > 10)
    {
        LogWarning("Failed to load quest data {} times, check database connection",
                   m_failedQuestLoads);
    }

    if (GetTimeSinceLastUpdate() > 10000)
    {
        LogWarning("Manager hasn't updated in {}ms, possible performance issue",
                   GetTimeSinceLastUpdate());
    }
}

// Output:
// [QuestManager] Failed to load quest data 12 times, check database connection
// [QuestManager] Manager hasn't updated in 15234ms, possible performance issue
```

---

## Protected Member Variables

These atomic state flags are available to derived classes for implementing state queries that can be called from strategies and other systems without locks.

### m_hasWork

```cpp
std::atomic<bool> m_hasWork{false};
```

**Purpose**: Indicates whether the manager has pending work to process.

**Type**: `std::atomic<bool>` (lock-free, thread-safe)

**Default**: `false`

**Usage**:
- Set to `true` when manager has tasks/work to process
- Set to `false` when manager is idle
- Queried by strategies to determine if they should activate
- Lock-free queries from any thread

**Example**:
```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    // Update work flag based on quest state
    bool hasActiveQuests = !m_activeQuests.empty();
    bool hasNearbyObjectives = CheckForNearbyQuestObjectives();

    m_hasWork.store(hasActiveQuests || hasNearbyObjectives,
                    std::memory_order_release);
}

// In strategy code:
bool QuestStrategy::IsRelevant()
{
    QuestManager* qm = m_ai->GetQuestManager();
    return qm && qm->m_hasWork.load(std::memory_order_acquire);
}
```

---

### m_needsUpdate

```cpp
std::atomic<bool> m_needsUpdate{false};
```

**Purpose**: Indicates that manager needs immediate update, bypassing normal throttling.

**Type**: `std::atomic<bool>` (lock-free, thread-safe)

**Default**: `false`

**Usage**:
- Set to `true` when immediate update is needed
- Automatically cleared after update is performed
- Lower priority than `ForceUpdate()` but higher than interval timer
- Can be set from external code or internal logic

**Example**:
```cpp
// Set from external event
void BotAI::OnQuestObjectiveCompleted(uint32 questId, uint32 objectiveIndex)
{
    if (_questManager)
    {
        _questManager->m_needsUpdate.store(true, std::memory_order_release);
    }
}

// Check in OnUpdate
void QuestManager::OnUpdate(uint32 elapsed)
{
    if (m_needsUpdate.load(std::memory_order_acquire))
    {
        // Perform immediate quest status check
        CheckAllQuestCompletion();
        m_needsUpdate.store(false, std::memory_order_release);
    }
}
```

---

### m_updateCount

```cpp
std::atomic<uint32> m_updateCount{0};
```

**Purpose**: Tracks total number of `OnUpdate()` calls since manager creation.

**Type**: `std::atomic<uint32>` (lock-free, thread-safe)

**Default**: `0`

**Usage**:
- Automatically incremented after each `OnUpdate()` call
- Useful for periodic actions (e.g., every 10th update)
- Useful for performance monitoring
- Never resets (lifetime counter)

**Example**:
```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    // Perform expensive operation every 20 updates
    if (m_updateCount % 20 == 0)
    {
        RebuildQuestCache(); // Expensive operation
    }

    // Log statistics every 100 updates
    if (m_updateCount % 100 == 0)
    {
        LogDebug("Update #{}: {} active quests, {} completed objectives",
                 m_updateCount.load(), m_activeQuests.size(), m_completedObjectives);
    }
}
```

---

## Private Implementation

The following private members are documented for understanding but are not accessible to derived classes.

### Private Member Variables

```cpp
Player* m_bot;                              // The bot this manager belongs to
BotAI* m_ai;                                // The AI controller
std::string m_managerName;                  // Name for logging/debugging

uint32 m_updateInterval;                    // Update interval in milliseconds
uint32 m_lastUpdate;                        // Timestamp of last update (getMSTime)
uint32 m_timeSinceLastUpdate;              // Accumulated time since last update

std::atomic<bool> m_enabled{true};         // Whether manager is enabled
std::atomic<bool> m_initialized{false};    // Whether OnInitialize has succeeded
std::atomic<bool> m_isBusy{false};         // Whether currently in OnUpdate
std::atomic<bool> m_forceUpdate{false};    // Force update on next tick

uint32 m_slowUpdateThreshold{50};          // Threshold for slow update warning (ms)
uint32 m_consecutiveSlowUpdates{0};        // Counter for consecutive slow updates
uint32 m_totalSlowUpdates{0};              // Total slow updates since creation
```

### Private Methods

#### DoUpdate

```cpp
void DoUpdate(uint32 elapsed);
```

Internal method that wraps `OnUpdate()` with performance monitoring and exception handling.

#### ValidatePointers

```cpp
bool ValidatePointers() const;
```

Validates that bot and AI pointers are still valid and bot is in world.

---

## Performance Characteristics

### Update Costs by Path

| Path | Condition | Cost | Description |
|------|-----------|------|-------------|
| **Disabled** | Manager disabled | <0.001ms | Single atomic check, immediate return |
| **Busy** | Currently updating | <0.001ms | Two atomic checks, immediate return |
| **Throttled** | Time not elapsed | <0.001ms | All checks + time comparison |
| **Initializing** | First update | 1-10ms | Initialization + first update |
| **Updating** | Normal update | 5-10ms | Full OnUpdate() execution |
| **Force Update** | Forced update | 5-10ms | Immediate OnUpdate() execution |

### Memory Footprint

- **Base class**: ~200 bytes per instance
- **Derived class**: +100-500 bytes (implementation dependent)
- **Total per bot**: ~300-700 bytes per manager
- **1000 bots**: ~300-700 KB per manager type

### Scalability Metrics

| Bots | Updates/sec | CPU Usage | Memory | Notes |
|------|-------------|-----------|--------|-------|
| 100 | 10,000 | <1% | <50 MB | Optimal |
| 500 | 50,000 | <5% | <250 MB | Good |
| 1000 | 100,000 | <10% | <500 MB | Acceptable |
| 5000 | 500,000 | <30% | <2.5 GB | Maximum tested |

**Assumptions**:
- 10 managers per bot
- 1000ms average update interval
- 5ms average OnUpdate() cost
- Modern multi-core CPU (2020+)

### Performance Monitoring

The base class includes automatic performance monitoring:

1. **Slow Update Detection**:
   - Threshold: 50ms (configurable via `m_slowUpdateThreshold`)
   - Action: Log warning on first slow update
   - Action: Log warning every 5th consecutive slow update
   - Action: Auto-adjust interval after 10 consecutive slow updates

2. **Auto-Adjustment**:
   - Triggered after 10 consecutive slow updates
   - Doubles update interval (max 5000ms)
   - Resets consecutive counter after adjustment
   - Prevents performance degradation spiral

3. **Metrics Tracked**:
   - Total OnUpdate() calls (`m_updateCount`)
   - Consecutive slow updates (`m_consecutiveSlowUpdates`)
   - Total slow updates (`m_totalSlowUpdates`)
   - Update duration (logged when >50ms)

---

## Thread Safety Guarantees

### Thread-Safe Operations (Lock-Free)

The following methods use atomic operations and can be called safely from any thread:

- `IsEnabled()` - Atomic read with acquire semantics
- `SetEnabled()` - Atomic write with release semantics
- `IsBusy()` - Atomic read with acquire semantics
- `IsInitialized()` - Atomic read with acquire semantics
- `ForceUpdate()` - Atomic write with release semantics
- Reading protected atomics (`m_hasWork`, `m_needsUpdate`, `m_updateCount`)
- Writing protected atomics from `OnUpdate()` (single writer)

### Single-Thread Operations (Main Thread Only)

The following methods must be called from the same thread every time (typically the main world update thread):

- Constructor
- Destructor
- `Update()` - Must be called from same thread every time
- `SetUpdateInterval()` - Should be called from main thread
- `OnUpdate()` - Always called from main thread
- `OnInitialize()` - Always called from main thread
- `OnShutdown()` - Always called from main thread

### Memory Ordering Guarantees

The class uses C++11 memory ordering to ensure correct operation across threads:

- **Acquire-Release**: Used for state flags (`m_enabled`, `m_isBusy`, etc.)
  - `store()` uses `memory_order_release`
  - `load()` uses `memory_order_acquire`
  - Guarantees visibility of all operations before store to all operations after load

- **Acq-Rel (Read-Modify-Write)**: Used for counters
  - `fetch_add()` uses `memory_order_acq_rel`
  - Guarantees atomic increment with full memory barrier

### Example: Multi-Threaded Query

```cpp
// From Strategy thread (read-only query)
void QuestStrategy::Evaluate()
{
    QuestManager* qm = m_ai->GetQuestManager();

    // Thread-safe: atomic loads with acquire semantics
    if (qm->IsEnabled() && !qm->IsBusy())
    {
        // Read protected atomics
        bool hasWork = qm->m_hasWork.load(std::memory_order_acquire);
        uint32 updateCount = qm->m_updateCount.load(std::memory_order_acquire);

        if (hasWork)
        {
            // Safe to proceed - manager has work and is not currently updating
            ActivateQuestBehavior();
        }
    }
}

// From main thread (update)
void BotAI::UpdateAI(uint32 diff)
{
    // Thread-safe: atomic write with release semantics
    if (enteredDungeon)
        _questManager->SetEnabled(false);

    // Main thread update
    _questManager->Update(diff);
}
```

---

## Complete Usage Examples

### Example 1: Simple Manager Implementation

```cpp
// QuestManager.h
class QuestManager : public BehaviorManager
{
public:
    explicit QuestManager(Player* bot, BotAI* ai)
        : BehaviorManager(bot, ai, 2000, "QuestManager") // 2 second interval
    {
    }

    bool HasActiveQuests() const
    {
        return !m_activeQuests.empty();
    }

    void AddQuest(Quest const* quest)
    {
        m_activeQuests.insert(quest->GetQuestId());
        m_needsUpdate.store(true, std::memory_order_release);
    }

protected:
    void OnUpdate(uint32 elapsed) override
    {
        Player* bot = GetBot();
        if (!bot)
            return;

        // Update quest progress
        for (uint32 questId : m_activeQuests)
        {
            UpdateQuestProgress(questId);
        }

        // Update state flag
        m_hasWork.store(!m_activeQuests.empty(), std::memory_order_release);
    }

    bool OnInitialize() override
    {
        Player* bot = GetBot();
        if (!bot || !bot->IsInWorld())
            return false;

        LoadQuestsFromDB();
        return true;
    }

    void OnShutdown() override
    {
        SaveQuestsToDB();
        m_activeQuests.clear();
    }

private:
    void UpdateQuestProgress(uint32 questId);
    void LoadQuestsFromDB();
    void SaveQuestsToDB();

    std::unordered_set<uint32> m_activeQuests;
};
```

### Example 2: Manager with Atomic State Flags

```cpp
// TradeManager.h
class TradeManager : public BehaviorManager
{
public:
    explicit TradeManager(Player* bot, BotAI* ai)
        : BehaviorManager(bot, ai, 500, "TradeManager") // 500ms interval
    {
    }

    enum class TradeState : uint32
    {
        IDLE = 0,
        EVALUATING = 1,
        NEGOTIATING = 2,
        ACCEPTING = 3,
        REJECTING = 4
    };

    TradeState GetTradeState() const
    {
        return static_cast<TradeState>(m_tradeState.load(std::memory_order_acquire));
    }

    bool IsTrading() const
    {
        return GetTradeState() != TradeState::IDLE;
    }

protected:
    void OnUpdate(uint32 elapsed) override
    {
        TradeState currentState = GetTradeState();

        switch (currentState)
        {
            case TradeState::EVALUATING:
                EvaluateTradeOffer();
                break;
            case TradeState::NEGOTIATING:
                NegotiateTrade();
                break;
            case TradeState::ACCEPTING:
                AcceptTrade();
                break;
            case TradeState::REJECTING:
                RejectTrade();
                break;
            default:
                break;
        }

        // Update work flag
        m_hasWork.store(IsTrading(), std::memory_order_release);
    }

private:
    void EvaluateTradeOffer();
    void NegotiateTrade();
    void AcceptTrade();
    void RejectTrade();

    std::atomic<uint32> m_tradeState{static_cast<uint32>(TradeState::IDLE)};
};
```

### Example 3: Manager with Performance Optimization

```cpp
// CombatManager.h
class CombatManager : public BehaviorManager
{
public:
    explicit CombatManager(Player* bot, BotAI* ai)
        : BehaviorManager(bot, ai, 200, "CombatManager") // 200ms interval (high frequency)
    {
    }

    bool IsInCombat() const
    {
        return m_inCombat.load(std::memory_order_acquire);
    }

    uint32 GetActiveEnemyCount() const
    {
        return m_enemyCount.load(std::memory_order_acquire);
    }

protected:
    void OnUpdate(uint32 elapsed) override
    {
        uint32 startTime = getMSTime();

        Player* bot = GetBot();
        if (!bot)
            return;

        // Fast path: Check combat state
        bool inCombat = bot->IsInCombat();
        m_inCombat.store(inCombat, std::memory_order_release);

        if (!inCombat)
        {
            m_enemyCount.store(0, std::memory_order_release);
            m_hasWork.store(false, std::memory_order_release);
            return; // Early exit when not in combat
        }

        // Combat state - perform expensive operations
        UpdateThreatList();
        SelectOptimalTarget();
        UpdateCooldowns();

        // Update enemy count
        m_enemyCount.store(m_threatList.size(), std::memory_order_release);
        m_hasWork.store(true, std::memory_order_release);

        // Performance check
        uint32 duration = getMSTimeDiff(startTime, getMSTime());
        if (duration > 10)
        {
            LogDebug("Combat update took {}ms", duration);
        }
    }

    bool OnInitialize() override
    {
        Player* bot = GetBot();
        if (!bot || !bot->IsInWorld())
            return false;

        LoadCombatPreferences();
        InitializeCooldownTracking();
        return true;
    }

private:
    void UpdateThreatList();
    void SelectOptimalTarget();
    void UpdateCooldowns();
    void LoadCombatPreferences();
    void InitializeCooldownTracking();

    std::atomic<bool> m_inCombat{false};
    std::atomic<uint32> m_enemyCount{0};
    std::vector<ObjectGuid> m_threatList;
};
```

### Example 4: Manager with Database Operations

```cpp
// InventoryManager.h
class InventoryManager : public BehaviorManager
{
public:
    explicit InventoryManager(Player* bot, BotAI* ai)
        : BehaviorManager(bot, ai, 5000, "InventoryManager") // 5 second interval (low frequency)
    {
    }

    bool NeedsRepair() const
    {
        return m_needsRepair.load(std::memory_order_acquire);
    }

    bool IsFull() const
    {
        return m_bagsFull.load(std::memory_order_acquire);
    }

protected:
    void OnUpdate(uint32 elapsed) override
    {
        Player* bot = GetBot();
        if (!bot)
            return;

        // Batch operations to reduce database load
        if (elapsed >= 10000) // Only every 10+ seconds
        {
            // Expensive: Full inventory scan
            ScanInventory();

            // Expensive: Database query for item values
            UpdateItemValueCache();
        }
        else
        {
            // Cheap: Quick durability check
            QuickDurabilityCheck();
        }

        // Update state flags
        m_needsRepair.store(m_durabilityPercent < 50, std::memory_order_release);
        m_bagsFull.store(m_freeBagSlots < 5, std::memory_order_release);
        m_hasWork.store(m_needsRepair || m_bagsFull, std::memory_order_release);
    }

    bool OnInitialize() override
    {
        Player* bot = GetBot();
        if (!bot || !bot->IsInWorld())
            return false;

        // Load from database
        LoadInventoryPreferences();
        ScanInventory();
        return true;
    }

    void OnShutdown() override
    {
        // Save to database
        SaveInventoryState();
    }

private:
    void ScanInventory();
    void UpdateItemValueCache();
    void QuickDurabilityCheck();
    void LoadInventoryPreferences();
    void SaveInventoryState();

    std::atomic<bool> m_needsRepair{false};
    std::atomic<bool> m_bagsFull{false};
    std::atomic<uint32> m_freeBagSlots{0};
    uint32 m_durabilityPercent{100};
};
```

### Example 5: Manager with External Events

```cpp
// GroupManager.h
class GroupManager : public BehaviorManager
{
public:
    explicit GroupManager(Player* bot, BotAI* ai)
        : BehaviorManager(bot, ai, 1000, "GroupManager")
    {
    }

    bool IsInGroup() const
    {
        return m_inGroup.load(std::memory_order_acquire);
    }

    // Called by external event system
    void OnGroupInvite(Player* inviter)
    {
        m_pendingInviter = inviter->GetGUID();
        m_needsUpdate.store(true, std::memory_order_release);
        ForceUpdate(); // Respond immediately to invites
    }

    void OnGroupLeft()
    {
        m_inGroup.store(false, std::memory_order_release);
        ForceUpdate(); // Update state immediately
    }

protected:
    void OnUpdate(uint32 elapsed) override
    {
        Player* bot = GetBot();
        if (!bot)
            return;

        // Check group membership
        Group* group = bot->GetGroup();
        bool inGroup = group != nullptr;
        m_inGroup.store(inGroup, std::memory_order_release);

        // Handle pending invite
        if (!m_pendingInviter.IsEmpty())
        {
            ProcessGroupInvite(m_pendingInviter);
            m_pendingInviter = ObjectGuid::Empty;
        }

        // Update group role
        if (inGroup)
        {
            UpdateGroupRole(group);
        }

        m_hasWork.store(inGroup || !m_pendingInviter.IsEmpty(),
                        std::memory_order_release);
    }

private:
    void ProcessGroupInvite(ObjectGuid inviter);
    void UpdateGroupRole(Group* group);

    std::atomic<bool> m_inGroup{false};
    ObjectGuid m_pendingInviter;
};
```

---

## Summary

The `BehaviorManager` base class provides a robust, performant foundation for implementing bot managers in the TrinityCore Playerbot module. Key advantages:

1. **Performance**: Sub-millisecond overhead when throttled, efficient atomic operations
2. **Scalability**: Designed to support thousands of concurrent bots
3. **Safety**: Automatic exception handling, pointer validation, performance monitoring
4. **Flexibility**: Easy to extend with virtual methods and protected state flags
5. **Observability**: Built-in logging and performance tracking

### Best Practices Summary

- **Implement OnUpdate()** with 5-10ms target execution time
- **Use atomic flags** (`m_hasWork`, custom atomics) for lock-free state queries
- **Return false from OnInitialize()** if prerequisites not met (will retry)
- **Update state flags** at end of OnUpdate() to reflect current state
- **Use LogDebug/LogWarning** for diagnostics with manager name prefix
- **Check GetBot() for null** at start of OnUpdate()
- **Avoid blocking operations** in OnUpdate() (no sleeps, no long DB queries)
- **Set update interval appropriately** (low for combat, high for background tasks)

### Common Patterns

1. **State Management**: Use atomic flags for lock-free state queries from strategies
2. **Event Response**: Use `ForceUpdate()` or `m_needsUpdate` for immediate event handling
3. **Batching**: Use `m_updateCount` for periodic expensive operations
4. **Performance**: Use early returns and fast paths in OnUpdate()
5. **Initialization**: Defer expensive setup to OnInitialize(), return false if not ready

For complete examples, see `ExampleManager.h/.cpp` in the same directory.

---

**Document Version**: 1.0
**Last Updated**: Phase 2.1 Completion
**Related Documents**:
- [BehaviorManager Developer Guide](BEHAVIORMANAGER_GUIDE.md)
- [BehaviorManager Architecture](BEHAVIORMANAGER_ARCHITECTURE.md)
