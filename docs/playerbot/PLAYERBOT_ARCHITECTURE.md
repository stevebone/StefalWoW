# TrinityCore Playerbot Architecture

**Document Version**: 1.0
**Last Updated**: 2025-10-16
**Status**: Active Development

## Table of Contents
1. [System Overview](#system-overview)
2. [Module Organization](#module-organization)
3. [Integration with TrinityCore](#integration-with-trinitycore)
4. [Threading Model & Concurrency](#threading-model--concurrency)
5. [Component Hierarchy](#component-hierarchy)
6. [Data Flow Diagrams](#data-flow-diagrams)
7. [Performance Architecture](#performance-architecture)

---

## System Overview

### Purpose
The TrinityCore Playerbot system transforms TrinityCore into a single-player capable MMORPG by integrating AI-controlled player bots. This enables offline gameplay without requiring internet connectivity or other human players.

### System Boundaries

```
┌─────────────────────────────────────────────────────────────┐
│                    TrinityCore Worldserver                   │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │         Playerbot Module (src/modules/Playerbot)      │  │
│  │                                                         │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │  │
│  │  │   Core   │  │    AI    │  │  Combat  │           │  │
│  │  └──────────┘  └──────────┘  └──────────┘           │  │
│  │                                                         │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │  │
│  │  │   Game   │  │  Social  │  │Performance│           │  │
│  │  └──────────┘  └──────────┘  └──────────┘           │  │
│  └───────────────────────────────────────────────────────┘  │
│                          ▲                                   │
│                          │ Integration Points               │
│                          ▼                                   │
│  ┌───────────────────────────────────────────────────────┐  │
│  │              TrinityCore Core Systems                  │  │
│  │  (Player, Map, Session, Combat, Spells, Movement)     │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Design Principles

1. **Module-First Architecture**: All bot functionality isolated in `src/modules/Playerbot/`
2. **Minimal Core Integration**: Use hooks/events pattern for core interaction
3. **Performance-Centric**: <0.1% CPU per bot, <10MB memory per bot
4. **Thread-Safe Operations**: Full concurrency support for multi-bot scenarios
5. **TrinityCore API Compliance**: No bypassing of existing systems

### Responsibilities

**Playerbot Module**:
- Bot lifecycle management (spawn, update, despawn)
- AI decision-making and behavior execution
- Combat coordination and positioning
- Quest tracking and completion
- Social interactions (group, raid, chat)
- Economy participation (vendors, auction house)
- Performance optimization and monitoring

**TrinityCore Core**:
- World state management
- Spell casting and effect application
- Movement and pathfinding
- Database persistence
- Network packet handling (for human players)
- Security and validation

---

## Module Organization

### Directory Structure

```
src/modules/Playerbot/
├── Core/                    # Bot lifecycle and session management
│   ├── Bot.h/cpp           # Core bot entity extending Player
│   ├── BotSession.h/cpp    # Socket-less bot session
│   └── Managers/           # Core manager infrastructure
│
├── AI/                      # Decision making and behavior systems
│   ├── BotAI.h/cpp         # Main AI controller
│   ├── ClassAI/            # Per-class specializations
│   │   ├── Warriors/
│   │   ├── Mages/
│   │   ├── Priests/
│   │   └── [13 classes total]
│   ├── Actions/            # Concrete bot actions
│   ├── Strategy/           # Behavior strategy pattern
│   └── Learning/           # Adaptive behavior learning
│
├── Combat/                  # Combat coordination and targeting
│   ├── TargetScanner.h/cpp         # Threat/enemy detection
│   ├── TargetSelector.h/cpp        # Target prioritization
│   ├── PositionManager.h/cpp       # Tactical positioning
│   ├── InterruptManager.h/cpp      # Spell interruption
│   ├── GridQueryProcessor.h/cpp    # Optimized grid queries
│   └── PreScanCache.h/cpp          # Combat entity caching
│
├── Game/                    # Quest, inventory, NPC interaction
│   ├── QuestManager.h/cpp          # Quest tracking
│   ├── InventoryManager.h/cpp      # Equipment/bag management
│   └── NPCInteractionManager.h/cpp # Vendor/trainer interaction
│
├── Social/                  # Group, raid, chat systems
│   ├── PlayerbotGroupManager.h/cpp
│   ├── TradeManager.h/cpp
│   └── GuildBankManager.h/cpp
│
├── Performance/             # ThreadPool, caching, optimization
│   ├── ThreadPool/         # Asynchronous task execution
│   ├── BotPerformanceMonitor.h/cpp
│   ├── BotMemoryManager.h/cpp
│   └── PerformanceManager.h/cpp
│
├── Infrastructure/          # Database, configuration, utilities
│   ├── Database/           # Bot-specific database operations
│   ├── Config/             # Configuration loading
│   └── Events/             # Event system
│
├── Advanced/                # Advanced features
│   ├── EconomyManager.h/cpp        # AH/vendor participation
│   ├── AdvancedBehaviorManager.h/cpp
│   └── SocialManager.h/cpp
│
├── Dungeon/                 # Dungeon/raid scripting
│   ├── DungeonScript.h/cpp
│   ├── EncounterStrategy.h/cpp
│   └── Scripts/            # Per-dungeon behavior
│
├── Lifecycle/               # Bot population and death recovery
│   ├── BotLifecycleManager.h/cpp
│   ├── BotPopulationManager.h/cpp
│   └── DeathRecoveryManager.h/cpp
│
├── LFG/                     # LFG/LFR integration
│   └── LFGBotManager.h/cpp
│
├── Movement/                # Advanced pathfinding
│   └── PathfindingManager.h/cpp
│
├── Professions/             # Crafting and gathering
│   ├── ProfessionManager.h/cpp
│   └── GatheringManager.h/cpp
│
├── PvP/                     # PvP combat AI
│   └── PvPCombatAI.h/cpp
│
├── Session/                 # Session management
│   ├── BotWorldSession.h/cpp
│   ├── BotWorldSessionMgr.h/cpp
│   └── BotSessionManager.h/cpp
│
└── conf/                    # Configuration files
    └── playerbots.conf.dist
```

### Subsystem Descriptions

#### Core/
**Purpose**: Foundation for bot existence and lifecycle
**Key Components**:
- `Bot`: Extends `Player` class, represents bot entity
- `BotSession`: Socket-less session for bots (no network I/O)
- `ManagerRegistry`: Centralized manager access

#### AI/
**Purpose**: Decision-making engine and behavior execution
**Key Components**:
- `BotAI`: Main AI controller, coordinates all bot actions
- `ClassAI`: Per-class specializations (Warriors, Mages, etc.)
  - Each class has 3+ specializations
  - Hero Talent support (War Within expansion)
- `BehaviorManager`: Strategy pattern for behavior selection
- `BehaviorPriorityManager`: Prioritizes competing behaviors

#### Combat/
**Purpose**: Combat effectiveness and coordination
**Key Components**:
- `TargetScanner`: Detects threats and valid targets
- `TargetSelector`: Prioritizes targets based on role/situation
- `PositionManager`: Tactical positioning (melee range, healing range, etc.)
- `InterruptManager`: Coordinates spell interruption
- `GridQueryProcessor`: Optimized queries for nearby entities
- `PreScanCache`: Caches combat-relevant entities to reduce queries

#### Game/
**Purpose**: Core gameplay system integration
**Key Components**:
- `QuestManager`: Quest discovery, tracking, completion
- `InventoryManager`: Equipment, consumables, bag management
- `NPCInteractionManager`: Vendor, trainer, quest giver interaction

#### Social/
**Purpose**: Multi-bot coordination and social features
**Key Components**:
- `PlayerbotGroupManager`: Group formation and coordination
- `TradeManager`: Bot-to-bot and bot-to-player trading
- `GuildBankManager`: Guild bank interaction

#### Performance/
**Purpose**: Optimization and resource management
**Key Components**:
- `ThreadPool`: Asynchronous task execution (50 threads)
- `BotPerformanceMonitor`: Real-time performance tracking
- `BotMemoryManager`: Memory pool and allocation optimization
- `PerformanceManager`: Centralized performance coordination

---

## Integration with TrinityCore

### Integration Philosophy

The Playerbot module follows a **minimal integration** approach:

1. **Observe, Don't Modify**: Use existing TrinityCore events
2. **Extend, Don't Replace**: Add functionality without changing core
3. **Hook, Don't Hack**: Use plugin/hook patterns for integration

### Integration Points

#### 1. Session Management
**Location**: `src/server/game/World/World.cpp`
**Pattern**: Hook pattern
**Purpose**: Bot sessions participate in world updates

```cpp
// Minimal hook in World::Update()
if (PlayerBotHooks::OnWorldUpdate)
    PlayerBotHooks::OnWorldUpdate(diff);
```

**Module Implementation**:
```cpp
// In src/modules/Playerbot/Session/BotWorldSessionMgr.cpp
void BotWorldSessionMgr::Update(uint32 diff)
{
    // Update all bot sessions
    for (auto& session : _botSessions)
        session->Update(diff);
}
```

#### 2. Player Lifecycle
**Location**: `src/server/game/Entities/Player/Player.cpp`
**Pattern**: Observer pattern
**Purpose**: Bot AI notified of player events

```cpp
// Hook in Player::Update()
if (IsBot() && GetBotAI())
    GetBotAI()->Update(diff);
```

#### 3. Combat Events
**Location**: `src/server/game/Combat/ThreatManager.cpp`
**Pattern**: Event subscription
**Purpose**: Bots react to threat/combat changes

```cpp
// Bots subscribe to combat events via CombatEventBus
// No core modification needed
```

#### 4. Spell Casting
**Location**: `src/server/game/Spells/Spell.cpp`
**Pattern**: Existing API usage
**Purpose**: Bots use standard spell casting

```cpp
// Bots use existing Player::CastSpell()
// No core modification needed
```

#### 5. Movement
**Location**: `src/server/game/Movement/MotionMaster.cpp`
**Pattern**: Existing API usage
**Purpose**: Bots use standard movement system

```cpp
// Bots use existing MotionMaster API
// No core modification needed
```

### Core Modifications Summary

**Total Core Files Modified**: ~5-10 files
**Total Lines Added to Core**: <200 lines
**Pattern**: All modifications use hook/observer pattern

**Modified Files**:
1. `World.cpp` - Bot session update hook
2. `Player.cpp` - Bot AI update integration
3. `CMakeLists.txt` - Module inclusion
4. `WorldSession.h` - Virtual methods for bot sessions
5. `Map.cpp` - Bot spawning hooks

**Justification**: Each modification enables essential bot participation in world systems while maintaining core stability.

---

## Threading Model & Concurrency

### Threading Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Main World Thread                         │
│  - Bot session updates (Update() calls)                      │
│  - AI decision-making                                         │
│  - Action execution                                           │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────┐
│              ThreadPool (50 worker threads)                  │
│  - Asynchronous tasks                                         │
│  - Grid queries (TargetScanner)                               │
│  - Database operations                                        │
│  - Pathfinding calculations                                   │
└─────────────────────────────────────────────────────────────┘
```

### Concurrency Model

#### Main Thread Operations (Synchronous)
- Bot AI decision-making (`BotAI::Update()`)
- Action execution (spell casting, movement)
- Session updates
- Event handling

#### Worker Thread Operations (Asynchronous)
- Grid queries for target scanning
- Database queries
- Complex pathfinding calculations
- Performance monitoring

### Thread Safety

**Thread-Safe Components**:
- `ThreadPool`: Lock-free task queue
- `PreScanCache`: Mutex-protected cache
- `GridQueryProcessor`: Atomic operations
- `BotPerformanceMonitor`: Lock-free metrics

**Thread-Unsafe (Main Thread Only)**:
- `BotAI`: Decision-making logic
- `Player` operations: Spell casting, movement
- World state modifications

### Synchronization Strategy

```cpp
// Example: GridQueryProcessor submits async task
void TargetScanner::ScanAsync(Bot* bot, std::function<void(std::vector<Unit*>)> callback)
{
    // Submit to thread pool
    ThreadPool::Instance()->Enqueue([bot, callback]() {
        // Worker thread: Query grid
        auto targets = GridQueryProcessor::FindTargets(bot);

        // Schedule callback on main thread
        bot->AddDelayedCallback([callback, targets]() {
            callback(targets);
        });
    });
}
```

### Deadlock Prevention

**Critical Fix (October 2025)**: ThreadPool deadlock resolution
- **Problem**: 60-second hang due to logging in worker threads
- **Solution**: Removed all logging from worker thread context
- **Result**: Zero deadlocks since fix

**Prevention Strategies**:
1. No logging in worker threads
2. No nested task submission
3. Timeout mechanisms (2-second max for queries)
4. Lock ordering discipline

---

## Component Hierarchy

### Layer 1: Core Foundation

```
┌───────────────────────────────────────────────────────────┐
│                      Bot Entity                            │
│  (extends Player)                                          │
│  - Bot-specific flags (IsBot())                            │
│  - BotAI reference                                          │
│  - No network socket                                        │
└───────────────────────────────────────────────────────────┘
                          ▲
                          │
┌───────────────────────────────────────────────────────────┐
│                   BotWorldSession                          │
│  (extends WorldSession)                                     │
│  - Socket-less session                                      │
│  - No packet I/O                                            │
│  - Minimal overhead                                         │
└───────────────────────────────────────────────────────────┘
```

### Layer 2: AI Decision Making

```
┌───────────────────────────────────────────────────────────┐
│                      BotAI                                 │
│  - Main AI controller                                       │
│  - Coordinates all bot actions                              │
└───────────────┬───────────────────────────────────────────┘
                │
                ├─► ClassAI (Warrior, Mage, Priest, etc.)
                │   - Class-specific logic
                │   - Specialization support
                │   - Hero Talents (War Within)
                │
                ├─► BehaviorManager
                │   - Strategy pattern
                │   - Context switching
                │
                └─► BehaviorPriorityManager
                    - Resolves behavior conflicts
                    - Priority queue
```

### Layer 3: Combat Systems

```
┌───────────────────────────────────────────────────────────┐
│                  Combat Subsystem                          │
├───────────────┬───────────────┬───────────────────────────┤
│ TargetScanner │ TargetSelector│ PositionManager           │
│ - Find targets│ - Prioritize  │ - Tactical positioning    │
│               │               │                            │
│InterruptMgr   │GridQueryProc  │ PreScanCache              │
│- Spell kicks  │- Optimized    │- Entity caching           │
│               │  queries      │                            │
└───────────────┴───────────────┴───────────────────────────┘
```

### Layer 4: Game Systems

```
┌───────────────────────────────────────────────────────────┐
│                   Game Subsystem                           │
├───────────────┬───────────────┬───────────────────────────┤
│ QuestManager  │InventoryMgr   │ NPCInteractionMgr         │
│ - Discovery   │- Equipment    │- Vendors                   │
│ - Tracking    │- Consumables  │- Trainers                  │
│ - Completion  │- Bag mgmt     │- Quest givers              │
└───────────────┴───────────────┴───────────────────────────┘
```

### Layer 5: Performance Optimization

```
┌───────────────────────────────────────────────────────────┐
│              Performance Subsystem                         │
├───────────────┬───────────────┬───────────────────────────┤
│ ThreadPool    │ PerformanceMon│ MemoryManager             │
│ - 50 workers  │- CPU/memory   │- Memory pools              │
│ - Async tasks │  tracking     │- Allocation opt            │
│               │               │                            │
│GridQueryProc  │ PreScanCache  │ Profilers                  │
│- Query opt    │- Data caching │- Bottleneck detection      │
└───────────────┴───────────────┴───────────────────────────┘
```

---

## Data Flow Diagrams

### 1. Bot Spawn → World Entry

```
[Account Creation]
        ↓
[Character Creation] (max 10 per account)
        ↓
[BotSpawner::SpawnBot()]
        ↓
[Create BotWorldSession (no socket)]
        ↓
[Load Character from DB]
        ↓
[Initialize BotAI]
        ↓
[Add to World]
        ↓
[BotAI::OnBotAdded()]
        ↓
[Start AI Update Loop]
```

### 2. Combat Engagement Flow

```
[TargetScanner] ──Async Query──► [GridQueryProcessor]
     ↓                                    ↓
[PreScanCache Check]              [Worker Thread Query]
     ↓                                    ↓
[Cache Hit? Return]              [Return Results]
     ↓                                    ↓
[TargetSelector]  ◄──────────────────────┘
     ↓
[Prioritize Targets]
 - Threat level
 - Role priority
 - Crowd control status
     ↓
[PositionManager]
 - Check range
 - LOS check
 - Tactical positioning
     ↓
[BotAI::ExecuteAction()]
 - Cast spell
 - Move to position
 - Use ability
     ↓
[TrinityCore Spell System]
     ↓
[Damage/Effect Application]
```

### 3. Quest Workflow

```
[QuestManager::DiscoverQuests()]
        ↓
[Query available quests]
 - Level appropriate
 - Prerequisites met
 - Not completed
        ↓
[QuestManager::AcceptQuest()]
        ↓
[NPCInteractionManager] ──► [Quest Giver NPC]
        ↓
[Quest Added to Log]
        ↓
[QuestManager::TrackObjectives()]
        ↓
[Objective Loop]
 - Kill targets
 - Collect items
 - Use GameObject
        ↓
[All Objectives Complete?]
        ↓
[QuestManager::TurnInQuest()]
        ↓
[NPCInteractionManager] ──► [Quest Giver NPC]
        ↓
[Reward Selection]
        ↓
[Quest Complete]
```

### 4. Group/Dungeon Flow

```
[LFGBotManager::QueueForDungeon()]
        ↓
[Wait in Queue]
        ↓
[Group Formed (5 players)]
 - 1 Tank
 - 1 Healer
 - 3 DPS
        ↓
[Teleport to Dungeon]
        ↓
[DungeonScript::OnEnter()]
        ↓
[Load Encounter Strategies]
        ↓
[Combat Loop]
 - Boss mechanics
 - Trash pulls
 - Role execution
        ↓
[Loot Distribution]
        ↓
[Dungeon Complete]
        ↓
[Teleport Out]
```

### 5. Economic Workflow

```
[EconomyManager::VendorInteraction()]
        ↓
[Sell Junk Items]
        ↓
[Repair Equipment]
        ↓
[Buy Consumables]
 - Food/Water
 - Potions
 - Reagents
        ↓
[EconomyManager::AuctionHouse()]
        ↓
[Sell Valuable Items]
        ↓
[Buy Upgrades]
        ↓
[Gold Management]
 - Track income/expenses
 - Budget allocation
```

---

## Performance Architecture

### Performance Targets

- **CPU Usage**: <0.1% per bot
- **Memory Usage**: <10MB per bot
- **Update Latency**: <50ms per bot update
- **Target Scanning**: <10ms per scan
- **Grid Queries**: <2ms average

### Optimization Strategies

#### 1. Update Spreading
**Problem**: All bots updating simultaneously causes CPU spikes
**Solution**: Stagger bot updates across frames

```cpp
// Each bot assigned update slot
uint32 updateSlot = botId % UPDATE_INTERVAL;
if (WorldTimer::GetMSTime() % UPDATE_INTERVAL == updateSlot)
    bot->Update(diff);
```

#### 2. PreScan Caching
**Problem**: Repeated grid queries for same entities
**Solution**: Cache combat-relevant entities

```cpp
// Cache valid for 500ms
if (cache.IsValid())
    return cache.GetEnemies();

// Refresh cache async
GridQueryProcessor::RefreshCacheAsync(cache);
```

#### 3. ThreadPool Async Execution
**Problem**: Expensive operations block main thread
**Solution**: Offload to worker threads

```cpp
// Submit pathfinding to worker
ThreadPool::Instance()->Enqueue([bot, destination]() {
    auto path = PathfindingManager::CalculatePath(bot, destination);
    bot->ApplyPath(path);
});
```

#### 4. Memory Pooling
**Problem**: Frequent allocations cause fragmentation
**Solution**: Pre-allocate object pools

```cpp
// Packet pool reduces allocations
WorldPacket* packet = PacketPoolManager::Acquire();
// ... use packet ...
PacketPoolManager::Release(packet);
```

#### 5. Grid Query Optimization
**Problem**: Naive grid iteration is expensive
**Solution**: Spatial indexing and early exit

```cpp
// Use grid search with early exit
for (auto& target : grid.GetTargets())
{
    if (target.IsValid() && target.InRange())
    {
        results.push_back(target);
        if (results.size() >= maxTargets)
            break; // Early exit
    }
}
```

### Performance Monitoring

```cpp
// Real-time metrics
BotPerformanceMonitor::Instance()->RecordMetric("bot_update_ms", duration);
BotPerformanceMonitor::Instance()->RecordMetric("target_scan_ms", scanTime);
BotPerformanceMonitor::Instance()->RecordMetric("memory_usage_mb", memory);

// Alert on threshold breach
if (cpuUsage > 80.0f)
    LOG_WARN("Playerbot CPU usage high: {}%", cpuUsage);
```

---

## Configuration Architecture

### Configuration Files

**Primary Config**: `playerbots.conf`

```ini
# Core Settings
Playerbot.Enable = 1
Playerbot.MaxBots = 500
Playerbot.MaxBotsPerAccount = 10

# Performance Settings
Playerbot.UpdateInterval = 100
Playerbot.ThreadPoolSize = 50

# Combat Settings
Playerbot.AI.CombatDelay = 200
Playerbot.AI.MovementDelay = 100

# Performance Monitoring
Playerbot.Performance.EnableMonitoring = 1
Playerbot.Performance.CPUWarningThreshold = 80
```

### Configuration Loading

```cpp
// Load on server start
PlayerbotConfig::LoadConfig();

// Access anywhere
uint32 maxBots = PlayerbotConfig::GetMaxBots();
bool enabled = PlayerbotConfig::IsEnabled();
```

---

## Summary

The TrinityCore Playerbot architecture is designed for:

1. **Modularity**: All bot code isolated in `src/modules/Playerbot/`
2. **Performance**: Multi-threaded, cached, optimized for 100-500 concurrent bots
3. **Maintainability**: Minimal core integration via hooks/events
4. **Extensibility**: Class-based AI, strategy patterns, plugin architecture
5. **Scalability**: Thread-safe, memory-efficient, CPU-conscious

**Next Steps**: See [PLAYERBOT_WORKFLOWS.md](PLAYERBOT_WORKFLOWS.md) for detailed workflow documentation.
