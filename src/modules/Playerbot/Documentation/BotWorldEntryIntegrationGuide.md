# Bot World Entry Integration Guide

## Overview

This guide explains how to integrate the new Bot World Entry system with synchronous database login for TrinityCore PlayerBot module. The system is designed to handle 100+ concurrent bot logins with optimal performance and stability.

## Architecture Components

### 1. BotWorldEntry System
- **Location**: `Lifecycle/BotWorldEntry.h/cpp`
- **Purpose**: Manages the complete bot entry sequence from character load to fully active in world
- **Key Features**:
  - State machine for world entry phases
  - Performance metrics tracking
  - Error handling and recovery
  - Queue management for concurrent entries

### 2. BotLifecycleManager
- **Location**: `Lifecycle/BotLifecycleManager.h/cpp`
- **Purpose**: Manages bot lifecycle after successful world entry
- **Key Features**:
  - Activity tracking
  - Performance monitoring
  - State transitions (idle, combat, questing, etc.)
  - Global statistics

### 3. BotAIInitializer
- **Location**: `AI/BotAIInitializer.h`
- **Purpose**: Handles AI system initialization and activation
- **Key Features**:
  - Configurable AI behaviors
  - Role-based templates
  - Difficulty settings
  - Class-specific configurations

## Integration Steps

### Step 1: Replace Async Login with Synchronous Login

In your `BotSession` class, replace the async login callback pattern with the synchronous method:

```cpp
// OLD - Async callback pattern (FAILING)
void BotSession::LoginCharacter(ObjectGuid characterGuid)
{
    auto holder = std::make_shared<BotLoginQueryHolder>(accountId, characterGuid);
    holder->Initialize();

    _worldSession->GetQueryProcessor().AddCallback(
        CharacterDatabase.AsyncQuery(holder)
        .WithChainingPreparedCallback([this](QueryCallback& holder) {
            HandleBotPlayerLogin(holder); // FAILS with timeout
        }));
}

// NEW - Synchronous login with BotWorldEntry
bool BotSession::LoginCharacterSync(ObjectGuid characterGuid)
{
    // Create world entry handler
    auto worldEntry = std::make_shared<BotWorldEntry>(shared_from_this(), characterGuid);

    // Perform synchronous world entry (blocks until complete)
    if (!worldEntry->EnterWorldSync(30000)) // 30 second timeout
    {
        TC_LOG_ERROR("module.playerbot.session",
                    "Failed to complete world entry for bot {}",
                    characterGuid.ToString());
        return false;
    }

    TC_LOG_INFO("module.playerbot.session",
               "Bot {} successfully entered world",
               GetPlayer()->GetName());

    return true;
}
```

### Step 2: Implement Queued World Entry for Multiple Bots

For spawning multiple bots without overwhelming the server:

```cpp
// In BotSpawner or BotManager
void SpawnMultipleBots(std::vector<ObjectGuid> const& botGuids)
{
    for (auto const& guid : botGuids)
    {
        // Create bot session
        auto session = BotSession::Create(accountId);

        // Create world entry handler
        auto worldEntry = std::make_shared<BotWorldEntry>(session, guid);

        // Queue for world entry (non-blocking)
        uint32 queuePos = BotWorldEntryQueue::instance()->QueueEntry(worldEntry);

        TC_LOG_INFO("module.playerbot.spawner",
                   "Bot {} queued for world entry (position: {})",
                   guid.ToString(), queuePos);
    }
}

// In your world update loop
void UpdateBotSystems(uint32 diff)
{
    // Process up to 10 concurrent bot entries per update
    BotWorldEntryQueue::instance()->ProcessQueue(10);

    // Update all active bot lifecycles
    BotLifecycleManager::instance()->UpdateAll(diff);
}
```

### Step 3: Initialize Bot AI After World Entry

The AI is automatically initialized during the world entry process:

```cpp
// In BotWorldEntry::InitializeAI()
bool BotWorldEntry::InitializeAI()
{
    // Create AI initializer
    BotAIInitializer initializer(_player);

    // Determine AI configuration based on class/role
    BotAIConfig config = BotAITemplates::GetClassTemplate(_player->getClass());

    // Initialize AI with configuration
    if (!initializer.Initialize(config))
    {
        SetError("Failed to initialize AI");
        return false;
    }

    // Activate AI
    BotAI* ai = initializer.GetAI();
    ai->OnRespawn(); // Start AI processing

    // Store AI reference
    _player->SetAI(ai);

    return true;
}
```

### Step 4: Manage Bot Lifecycle

After successful world entry, the bot lifecycle is managed automatically:

```cpp
// Create lifecycle controller
auto lifecycle = BotLifecycleManager::instance()->CreateBotLifecycle(
    botGuid, session);

// Lifecycle automatically transitions through states:
// CREATED → LOGGING_IN → ACTIVE → (IDLE|COMBAT|QUESTING) → LOGGING_OUT → OFFLINE

// Monitor bot performance
auto metrics = lifecycle->GetMetrics();
TC_LOG_DEBUG("module.playerbot.lifecycle",
            "Bot {} - AI Update: {:.2f}ms avg, Actions: {}/s",
            botGuid.ToString(),
            metrics.avgAiUpdateTime,
            metrics.actionsExecuted);
```

## World Entry Sequence

The complete world entry sequence follows these phases:

1. **CHARACTER_LOADED**: Load character data from database
2. **PLAYER_CREATED**: Create and initialize Player object
3. **MAP_LOADED**: Load target map for player
4. **ADDING_TO_MAP**: Add player to map
5. **IN_WORLD**: Player successfully in world
6. **AI_INITIALIZING**: Initialize AI systems
7. **AI_ACTIVE**: AI fully operational
8. **FULLY_ACTIVE**: Bot completely ready

Each phase includes:
- Performance metrics tracking
- Error handling and retry logic
- Timeout protection
- State transition validation

## Performance Optimization

### Concurrent Login Limits

Configure maximum concurrent logins to prevent server overload:

```cpp
// Set in configuration or code
BotWorldEntryQueue::instance()->SetMaxConcurrent(10); // Process 10 bots at a time
BotLifecycleManager::instance()->SetMaxConcurrentLogins(10);
```

### Memory Management

Monitor memory usage per bot:

```cpp
auto stats = BotWorldEntryQueue::instance()->GetStats();
TC_LOG_INFO("module.playerbot.performance",
           "Bot Entry Stats - Active: {}, Completed: {}, Failed: {}, Avg Time: {:.2f}s",
           stats.activeEntries,
           stats.completedEntries,
           stats.failedEntries,
           stats.averageEntryTime);
```

### Update Intervals

Tune update intervals for performance:

```cpp
// Configure bot update frequency
BotLifecycleManager::instance()->SetUpdateInterval(100); // Update every 100ms

// Configure AI decision frequency
config.decisionIntervalMs = 200; // Make decisions every 200ms
config.reactionTimeMs = 500; // 500ms reaction time
```

## Error Handling

### Login Failures

Handle login failures gracefully:

```cpp
auto worldEntry = std::make_shared<BotWorldEntry>(session, guid);

worldEntry->BeginWorldEntry([](bool success, BotWorldEntryMetrics const& metrics) {
    if (!success)
    {
        TC_LOG_ERROR("module.playerbot",
                    "Bot world entry failed: {} (State: {})",
                    metrics.lastError,
                    static_cast<uint32>(metrics.failedState));

        // Retry or cleanup
        // ...
    }
    else
    {
        TC_LOG_INFO("module.playerbot",
                   "Bot entered world in {} ms",
                   metrics.totalTime / 1000);
    }
});
```

### Timeout Protection

All operations include timeout protection:

```cpp
// Synchronous entry with timeout
if (!worldEntry->EnterWorldSync(30000)) // 30 second timeout
{
    // Handle timeout
}

// Each phase has individual timeout
static constexpr auto PHASE_TIMEOUT = std::chrono::seconds(10);
```

### Recovery Mechanisms

Automatic retry on transient failures:

```cpp
static constexpr uint32 MAX_RETRY_COUNT = 3;

if (!LoadCharacterData())
{
    if (++_retryCount >= MAX_RETRY_COUNT)
    {
        HandleWorldEntryFailure("Maximum retry count exceeded");
        return false;
    }
    // Retry automatically
}
```

## Testing

### Unit Testing

Test individual components:

```cpp
TEST(BotWorldEntry, SuccessfulLogin)
{
    auto session = BotSession::Create(testAccountId);
    auto worldEntry = std::make_shared<BotWorldEntry>(session, testGuid);

    EXPECT_TRUE(worldEntry->EnterWorldSync(5000));
    EXPECT_EQ(worldEntry->GetState(), BotWorldEntryState::FULLY_ACTIVE);
}
```

### Load Testing

Test concurrent bot logins:

```cpp
// Spawn 100 bots concurrently
std::vector<ObjectGuid> botGuids = GenerateTestBots(100);

for (auto const& guid : botGuids)
{
    auto session = BotSession::Create(accountId);
    auto worldEntry = std::make_shared<BotWorldEntry>(session, guid);
    BotWorldEntryQueue::instance()->QueueEntry(worldEntry);
}

// Monitor performance
while (BotWorldEntryQueue::instance()->GetStats().activeEntries > 0)
{
    BotWorldEntryQueue::instance()->ProcessQueue(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

auto stats = BotWorldEntryQueue::instance()->GetStats();
EXPECT_GE(stats.completedEntries, 95); // At least 95% success rate
EXPECT_LE(stats.averageEntryTime, 5.0f); // Average under 5 seconds
```

## Troubleshooting

### Common Issues and Solutions

1. **Timeout during character loading**
   - Check database connection pool size
   - Verify character data integrity
   - Increase timeout values if needed

2. **Map loading failures**
   - Ensure map data is extracted correctly
   - Check map instance limits
   - Verify memory availability

3. **AI initialization failures**
   - Check spell data availability
   - Verify action/strategy definitions
   - Review class-specific configurations

4. **High memory usage**
   - Reduce concurrent login limit
   - Optimize AI update intervals
   - Enable memory pooling

## Configuration Examples

### Minimal Configuration

```cpp
// Basic bot with auto-detection
auto session = BotSession::Create(accountId);
auto worldEntry = std::make_shared<BotWorldEntry>(session, characterGuid);
worldEntry->EnterWorldSync();
```

### Advanced Configuration

```cpp
// Custom AI configuration
BotAIConfig config;
config.role = BotAIConfig::TANK;
config.behavior = BotAIConfig::AGGRESSIVE;
config.difficulty = BotAIConfig::EXPERT;
config.reactionTimeMs = 200; // Fast reactions
config.threatModifier = 1.5f; // High threat generation

// Initialize with custom config
BotAIInitializer initializer(player);
initializer.Initialize(config, [](bool success, BotAI* ai) {
    if (success)
    {
        TC_LOG_INFO("module.playerbot", "Custom AI initialized successfully");
    }
});
```

## Performance Metrics

Expected performance with the new system:

- **Login Time**: 500-2000ms per bot
- **Concurrent Logins**: 10-20 bots simultaneously
- **Memory Usage**: <10MB per bot
- **CPU Usage**: <0.1% per bot
- **Success Rate**: >95% for valid characters
- **AI Update Time**: <1ms average

## Migration from Legacy System

If migrating from the old async system:

1. Replace all async login calls with `LoginCharacterSync()`
2. Remove callback-based login handlers
3. Implement world entry queue for batch operations
4. Update error handling to use synchronous patterns
5. Add performance monitoring using provided metrics

## Support and Debugging

Enable detailed logging for troubleshooting:

```cpp
// In your config
TC_LOG_SetLogLevel("module.playerbot.worldentry", LOG_LEVEL_DEBUG);
TC_LOG_SetLogLevel("module.playerbot.session", LOG_LEVEL_DEBUG);
TC_LOG_SetLogLevel("module.playerbot.lifecycle", LOG_LEVEL_DEBUG);
```

Monitor system performance:

```cpp
// Print performance report
BotLifecycleManager::instance()->PrintPerformanceReport();

// Get detailed metrics
auto globalStats = BotLifecycleManager::instance()->GetGlobalStats();
TC_LOG_INFO("module.playerbot",
           "Global Stats - Total: {}, Active: {}, Combat: {}, Avg AI: {:.2f}ms",
           globalStats.totalBots,
           globalStats.activeBots,
           globalStats.combatBots,
           globalStats.avgAiUpdateTime);
```

## Conclusion

The new Bot World Entry system provides a robust, scalable solution for bot login and world entry. By replacing the failing async callbacks with synchronous operations and implementing proper state management, the system ensures reliable bot spawning with excellent performance characteristics.