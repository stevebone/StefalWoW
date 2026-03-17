# BotAI Game Systems Integration - Update Flow Documentation

## System Architecture Overview

The BotAI class now integrates 4 major game systems with carefully designed update priorities to maintain performance while providing responsive bot behavior.

```
┌─────────────────────────────────────────────────────────────┐
│                        BotAI CLASS                           │
├─────────────────────────────────────────────────────────────┤
│  Member Variables:                                           │
│  - std::unique_ptr<QuestManager> _questManager               │
│  - std::unique_ptr<InventoryManager> _inventoryManager       │
│  - std::unique_ptr<BotTradeManager> _tradeManager            │
│  - (AuctionManager accessed via singleton)                   │
│                                                               │
│  Throttle Timers:                                            │
│  - uint32 _lastQuestUpdate                                   │
│  - uint32 _lastAuctionUpdate                                 │
└─────────────────────────────────────────────────────────────┘
```

## Update Priority Strategy

### Priority Level 1: CRITICAL (Every Frame)
**Location:** `BotAI::UpdateGameSystems()`
**Called from:** `BotAI::UpdateAI()` main loop

```cpp
InventoryManager::Update(diff)
├── Looting corpses and containers
├── Auto-equipping better items
├── Managing bag space
└── Vendor trash detection
```

**Rationale:** Looting must be responsive. Missing loot windows causes items to be lost.

### Priority Level 2: HIGH (Active Only)
**Location:** `BotAI::UpdateGameSystems()`
**Called from:** `BotAI::UpdateAI()` main loop

```cpp
if (TradeManager::IsTrading())
    TradeManager::Update(diff)
    ├── Processing trade window updates
    ├── Evaluating trade fairness
    ├── Auto-accepting safe trades
    └── Managing trade state machine
```

**Rationale:** Active trades have timeouts. Must respond quickly to trade windows.

### Priority Level 3: THROTTLED (Idle Only)
**Location:** `BotAI::UpdateIdleBehaviors()`
**Called from:** `BotAI::UpdateAI()` when not in combat/following

```cpp
// Quest System - 5 second intervals
if (currentTime - _lastQuestUpdate > 5000)
    QuestManager::Update(diff)
    ├── Scanning for available quests
    ├── Tracking quest progress
    ├── Auto turn-in completed quests
    └── Selecting best quest rewards

// Auction System - 60 second intervals
if (currentTime - _lastAuctionUpdate > 60000)
    AuctionManager::Update(diff)
    ├── Market price analysis
    ├── Creating new auctions
    ├── Monitoring existing auctions
    └── Finding arbitrage opportunities
```

**Rationale:** Quest and auction operations involve expensive database queries and complex calculations.

## Update Flow Diagram

```
BotSession::Update(diff)
    │
    └──> BotAI::UpdateAI(diff)
            │
            ├──> [PHASE 1: Core Behaviors]
            │    ├── UpdateValues(diff)
            │    ├── UpdateStrategies(diff)
            │    ├── ProcessTriggers()
            │    ├── UpdateActions(diff)
            │    ├── UpdateMovement(diff)
            │    └── UpdateGameSystems(diff) ◄── NEW!
            │         ├── InventoryManager::Update() [EVERY FRAME]
            │         └── TradeManager::Update() [IF TRADING]
            │
            ├──> [PHASE 2: State Management]
            │    └── UpdateCombatState(diff)
            │
            ├──> [PHASE 3: Combat]
            │    └── if (IsInCombat()) OnCombatUpdate(diff)
            │
            ├──> [PHASE 4: Group]
            │    └── GroupInvitationHandler::Update(diff)
            │
            ├──> [PHASE 5: Idle Behaviors]
            │    └── if (!IsInCombat() && !IsFollowing())
            │         └── UpdateIdleBehaviors(diff) ◄── THROTTLED!
            │              ├── QuestManager::Update() [5s throttle]
            │              └── AuctionManager::Update() [60s throttle]
            │
            └──> [PHASE 6: Performance Tracking]
```

## Configuration Integration

All systems check PlayerbotConfig for enabled state:

```cpp
// Global enable check
if (!PlayerbotConfig::GetInstance()->IsGameSystemsEnabled())
    return;

// Per-system checks
if (PlayerbotConfig::GetInstance()->IsQuestSystemEnabled())
if (PlayerbotConfig::GetInstance()->IsInventorySystemEnabled())
if (PlayerbotConfig::GetInstance()->IsTradeSystemEnabled())
if (PlayerbotConfig::GetInstance()->IsAuctionSystemEnabled())
```

## Error Handling Strategy

Every system update is wrapped in try-catch for safety:

```cpp
try {
    _questManager->Update(diff);
} catch (std::exception const& e) {
    TC_LOG_ERROR("playerbots.ai.quest",
                "Quest update failed for bot {}: {}",
                _bot->GetName(), e.what());
}
```

This prevents a single system failure from crashing the entire bot.

## Memory Management

All managers use smart pointers for automatic cleanup:
- `std::unique_ptr<QuestManager>` - Owned by BotAI
- `std::unique_ptr<InventoryManager>` - Owned by BotAI
- `std::unique_ptr<BotTradeManager>` - Owned by BotAI
- `BotAuctionManager` - Singleton pattern, self-managed

## Performance Targets

| System | Update Frequency | CPU Budget | Memory Budget |
|--------|-----------------|------------|---------------|
| Inventory | Every frame (~50ms) | 0.03% | 200KB |
| Trade | When active | 0.02% | 100KB |
| Quest | 5 seconds | 0.01% | 500KB |
| Auction | 60 seconds | 0.05% | 1MB |
| **Total** | - | **<0.11%** | **<1.8MB** |

## Thread Safety

All managers implement internal thread safety:
- Inventory: Thread-local caches
- Trade: Mutex-protected state
- Quest: Read-write locks for cache
- Auction: Singleton with global mutex

## Testing Validation Points

1. **Memory Leak Testing**
   - Create/destroy 100 bots rapidly
   - Monitor memory usage over time

2. **Performance Testing**
   - Spawn 500 bots simultaneously
   - Measure CPU usage per system

3. **Crash Testing**
   - Force exceptions in each manager
   - Verify graceful error handling

4. **Integration Testing**
   - Verify all systems initialize correctly
   - Test configuration enable/disable
   - Validate update throttling works