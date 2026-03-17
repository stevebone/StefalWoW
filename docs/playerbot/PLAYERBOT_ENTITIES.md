# TrinityCore Playerbot Entities

**Document Version**: 1.0
**Last Updated**: 2025-10-16
**Status**: Active Development

## Table of Contents
1. [Core Entities](#core-entities)
2. [AI System Entities](#ai-system-entities)
3. [Combat System Entities](#combat-system-entities)
4. [Game System Entities](#game-system-entities)
5. [Performance System Entities](#performance-system-entities)
6. [Event System Entities](#event-system-entities)
7. [Session Management Entities](#session-management-entities)

---

## Core Entities

### Bot (extends Player)
**File**: `src/modules/Playerbot/Core/Bot.h`

**Purpose**: Core bot entity representing an AI-controlled player character.

**Key Characteristics**:
- Extends TrinityCore's `Player` class
- No network socket attached (socket-less session)
- Bot-specific flags (`IsBot()` returns true)
- Contains reference to `BotAI` instance

**Key Methods**:
```cpp
bool IsBot() const;                    // Returns true for bots
BotAI* GetBotAI() const;              // Get AI controller
void SetBotAI(BotAI* ai);             // Assign AI controller
void Update(uint32 diff) override;    // Bot-specific update logic
```

**Inheritance Hierarchy**:
```
Object → WorldObject → Unit → Player → Bot
```

**Bot-Specific Features**:
- No packet I/O overhead
- Optimized update path
- AI-driven behavior
- No network latency simulation

---

### BotAI
**File**: `src/modules/Playerbot/AI/BotAI.h`

**Purpose**: Main AI controller coordinating all bot actions and behaviors.

**Key Responsibilities**:
1. Decision-making engine
2. Strategy coordination
3. Action execution
4. State management
5. Event handling

**Core State Machine**:
```cpp
enum class BotAIState
{
    SOLO,        // Solo play mode (questing, gathering, autonomous combat)
    COMBAT,      // In active combat
    DEAD,        // Dead, awaiting resurrection
    TRAVELLING,  // Moving to destination
    QUESTING,    // Executing quest objectives
    GATHERING,   // Gathering resources
    TRADING,     // Interacting with vendor/player
    FOLLOWING,   // Following player/group
    FLEEING,     // Retreating from danger
    RESTING      // Regenerating health/mana
};
```

**Key Methods**:
```cpp
// Main update loop
virtual void UpdateAI(uint32 diff);

// Combat specialization hook
virtual void OnCombatUpdate(uint32 diff);

// Lifecycle events
virtual void OnDeath();
virtual void OnRespawn();
virtual void OnCombatStart(Unit* target);
virtual void OnCombatEnd();

// Strategy management
void AddStrategy(std::unique_ptr<Strategy> strategy);
void ActivateStrategy(std::string const& name);
void DeactivateStrategy(std::string const& name);

// Action execution
bool ExecuteAction(std::string const& actionName);
void QueueAction(std::shared_ptr<Action> action, ActionContext const& context);

// State management
BotAIState GetAIState() const;
void SetAIState(BotAIState state);

// Target management
void SetTarget(ObjectGuid guid);
Unit* GetTargetUnit() const;

// Movement control
void MoveTo(float x, float y, float z);
void Follow(Unit* target, float distance = 5.0f);
void StopMovement();

// Manager access
QuestManager* GetQuestManager();
TradeManager* GetTradeManager();
GatheringManager* GetGatheringManager();
AuctionManager* GetAuctionManager();
DeathRecoveryManager* GetDeathRecoveryManager();
```

**Event Handlers** (Phase 4):
```cpp
virtual void OnGroupEvent(GroupEvent const& event);
virtual void OnCombatEvent(CombatEvent const& event);
virtual void OnCooldownEvent(CooldownEvent const& event);
virtual void OnAuraEvent(AuraEvent const& event);
virtual void OnLootEvent(LootEvent const& event);
virtual void OnQuestEvent(QuestEvent const& event);
virtual void OnResourceEvent(ResourceEvent const& event);
virtual void OnSocialEvent(SocialEvent const& event);
virtual void OnAuctionEvent(AuctionEvent const& event);
virtual void OnNPCEvent(NPCEvent const& event);
virtual void OnInstanceEvent(InstanceEvent const& event);
```

**Performance Metrics**:
```cpp
struct PerformanceMetrics
{
    std::atomic<uint32> totalUpdates;
    std::atomic<uint32> actionsExecuted;
    std::atomic<uint32> triggersProcessed;
    std::atomic<uint32> strategiesEvaluated;
    std::chrono::microseconds averageUpdateTime;
    std::chrono::microseconds maxUpdateTime;
};
```

**Architecture Pattern**:
```
BotAI (Base Class)
    ├── UpdateAI() - Main update loop
    ├── OnCombatUpdate() - Virtual combat hook
    └── ClassAI (Derived Classes)
         ├── WarriorAI
         ├── MageAI
         ├── PriestAI
         └── [10 more classes]
```

---

### ClassAI (Abstract Base)
**File**: `src/modules/Playerbot/AI/ClassAI/ClassAI.h`

**Purpose**: Base class for class-specific AI implementations.

**Key Responsibilities**:
- Class-specific combat rotations
- Specialization-aware logic
- Resource management (mana, rage, energy, runes)
- Cooldown management
- Hero Talent support (War Within expansion)

**Specialization Hierarchy**:
```
ClassAI (Abstract)
    ├── WarriorAI
    │   ├── ArmsSpecialization
    │   ├── FurySpecialization
    │   └── ProtectionSpecialization
    ├── MageAI
    │   ├── ArcaneSpecialization
    │   ├── FireSpecialization
    │   └── FrostSpecialization
    ├── PriestAI
    │   ├── DisciplineSpecialization
    │   ├── HolySpecialization
    │   └── ShadowSpecialization
    └── [10 more classes × 3-4 specs each]
```

**Hero Talent Support**:
Each specialization has 2 Hero Talent options (War Within expansion):
```cpp
enum class HeroTalentTree
{
    // Warrior
    COLOSSUS,          // Arms/Protection
    MOUNTAIN_THANE,    // Fury/Protection
    SLAYER,            // Arms/Fury

    // Mage
    SUNFURY,
    SPELLSLINGER,
    FROSTFIRE,

    // ... [all 39 Hero Talent trees]
};
```

**Key Methods**:
```cpp
// Combat rotation
virtual void ExecuteCombatRotation(Unit* target);
virtual void ExecuteAoERotation(std::vector<Unit*> const& targets);

// Resource management
virtual void ManageResources();
virtual bool HasEnoughResource(uint32 spellId);

// Cooldown management
virtual void UseCooldowns(bool burst = false);
virtual bool ShouldUseCooldown(uint32 spellId);

// Specialization
virtual uint8 GetActiveSpecialization();
virtual void SwitchSpecialization(uint8 spec);

// Hero Talents
virtual HeroTalentTree GetActiveHeroTalent();
virtual void ActivateHeroTalent(HeroTalentTree tree);
```

---

## AI System Entities

### Strategy
**File**: `src/modules/Playerbot/AI/Strategy/Strategy.h`

**Purpose**: Encapsulates a behavior pattern (e.g., "Follow", "Combat", "Quest").

**Key Characteristics**:
- Strategy pattern implementation
- Composable behaviors
- Priority-based execution
- State-aware activation

**Key Methods**:
```cpp
virtual void OnActivate();
virtual void OnDeactivate();
virtual void Update(uint32 diff);
virtual bool ShouldActivate() const;
virtual uint32 GetPriority() const;
```

**Common Strategies**:
- `FollowStrategy`: Follow master/group leader
- `CombatStrategy`: Engage in combat
- `QuestStrategy`: Execute quest objectives
- `GatheringStrategy`: Gather resources
- `LootStrategy`: Loot corpses
- `RestStrategy`: Regenerate health/mana

---

### Action
**File**: `src/modules/Playerbot/AI/Actions/Action.h`

**Purpose**: Represents a concrete bot action (e.g., "CastSpell", "MoveTo", "UseItem").

**Key Characteristics**:
- Command pattern implementation
- Context-aware execution
- Precondition checking
- Result reporting

**Action Lifecycle**:
```
[QUEUED] → [EXECUTING] → [COMPLETED/FAILED]
```

**Key Methods**:
```cpp
virtual bool CanExecute(ActionContext const& context) const;
virtual ActionResult Execute(ActionContext const& context);
virtual void OnComplete(ActionResult result);
virtual uint32 GetPriority() const;
```

**Common Actions**:
- `CastSpellAction`: Cast a spell on target
- `MoveToAction`: Move to specific location
- `UseItemAction`: Use an item
- `LootAction`: Loot a corpse
- `AcceptQuestAction`: Accept a quest
- `TurnInQuestAction`: Turn in a quest

---

### Trigger
**File**: `src/modules/Playerbot/AI/Triggers/Trigger.h`

**Purpose**: Detects conditions that should activate actions.

**Key Characteristics**:
- Observer pattern implementation
- Event-driven activation
- Priority-based evaluation

**Key Methods**:
```cpp
virtual bool IsActive() const;
virtual float Evaluate() const;
virtual Action* CreateAction();
```

**Common Triggers**:
- `LowHealthTrigger`: Health < 30%
- `EnemyTrigger`: Hostile enemy nearby
- `LootAvailableTrigger`: Loot corpse nearby
- `QuestObjectiveTrigger`: Quest objective available
- `BuffExpiredTrigger`: Important buff expired

---

### BehaviorPriorityManager
**File**: `src/modules/Playerbot/AI/BehaviorPriorityManager.h`

**Purpose**: Resolves behavior conflicts and selects highest-priority action.

**Key Responsibilities**:
- Evaluate competing behaviors
- Apply priority rules
- Handle behavior transitions

**Priority Hierarchy**:
```
1. CRITICAL (10): Death, fleeing, resurrection
2. HIGH (8): Combat, healing critical target
3. MEDIUM (5): Questing, gathering, following
4. LOW (3): Buffing, idle behavior
5. IDLE (1): Waiting, resting
```

---

## Combat System Entities

### TargetScanner
**File**: `src/modules/Playerbot/AI/Combat/TargetScanner.h`

**Purpose**: Detects and scans for valid combat targets.

**Key Responsibilities**:
- Grid-based enemy detection
- Threat assessment
- Target prioritization
- Blacklist management

**Scan Modes**:
```cpp
enum class ScanMode
{
    AGGRESSIVE,  // Attack anything hostile
    DEFENSIVE,   // Only attack if threatened
    PASSIVE,     // Never auto-attack
    ASSIST       // Only attack group's target
};
```

**Key Methods**:
```cpp
Unit* FindNearestHostile(float range = 0.0f);
Unit* FindBestTarget(float range = 0.0f);
std::vector<Unit*> FindAllHostiles(float range = 0.0f);

bool IsValidTarget(Unit* target) const;
bool ShouldEngage(Unit* target) const;
uint8 GetTargetPriority(Unit* target) const;
float GetThreatValue(Unit* target) const;
```

**Scan Result**:
```cpp
struct ScanResult
{
    Unit* target = nullptr;
    float distance = 0.0f;
    float threat = 0.0f;
    uint8 priority = 0;
};
```

---

### TargetSelector
**File**: `src/modules/Playerbot/AI/Combat/TargetSelector.h`

**Purpose**: Selects the optimal target from scan results.

**Priority Logic**:
```cpp
enum TargetPriority
{
    PRIORITY_CRITICAL = 10,  // Attacking bot or allies
    PRIORITY_CASTER = 8,     // Casters and healers
    PRIORITY_ELITE = 6,      // Elite mobs
    PRIORITY_NORMAL = 4,     // Normal mobs
    PRIORITY_TRIVIAL = 2,    // Grey-level mobs
    PRIORITY_AVOID = 0       // Should not engage
};
```

**Selection Criteria**:
1. Threat level (attacking allies?)
2. Role priority (healers > casters > melee)
3. Crowd control status
4. Health percentage
5. Distance

---

### PositionManager
**File**: `src/modules/Playerbot/AI/Combat/PositionManager.h`

**Purpose**: Manages tactical positioning during combat.

**Key Responsibilities**:
- Role-based positioning (tank, healer, DPS)
- Line of sight checks
- Obstacle avoidance
- Formation maintenance

**Positioning Rules**:
```cpp
// Tank: 0-5 yards, facing target
Position CalculateTankPosition(Unit* target);

// Melee DPS: 0-5 yards, behind target
Position CalculateMeleeDPSPosition(Unit* target);

// Ranged DPS: 20-40 yards, LoS required
Position CalculateRangedDPSPosition(Unit* target);

// Healer: 15-40 yards, LoS to all group members
Position CalculateHealerPosition(Group* group);
```

---

### InterruptManager
**File**: `src/modules/Playerbot/AI/Combat/InterruptManager.h`

**Purpose**: Coordinates spell interruption across group.

**Key Responsibilities**:
- Detect interruptible casts
- Prioritize interrupt targets
- Coordinate group interrupts (avoid overlap)
- Track interrupt cooldowns

**Priority System**:
```cpp
enum class InterruptPriority
{
    CRITICAL = 10,  // Healing spells, dangerous AoE
    HIGH = 7,       // Damage spells, buffs
    MEDIUM = 4,     // Regular casts
    LOW = 2         // Non-threatening casts
};
```

---

### GridQueryProcessor
**File**: `src/modules/Playerbot/Combat/GridQueryProcessor.h`

**Purpose**: Optimized grid queries for entity detection.

**Key Optimizations**:
- Asynchronous execution (ThreadPool)
- Spatial indexing
- Early exit conditions
- Result caching

**Key Methods**:
```cpp
static std::vector<Unit*> FindEnemies(Player* bot, float range);
static std::vector<Player*> FindGroupMembers(Player* bot, float range);
static std::vector<GameObject*> FindGameObjects(Player* bot, uint32 entry, float range);

// Async variants
static void FindEnemiesAsync(Player* bot, float range, std::function<void(std::vector<Unit*>)> callback);
```

---

### PreScanCache
**File**: `src/modules/Playerbot/Combat/PreScanCache.h`

**Purpose**: Caches combat-relevant entities to reduce grid queries.

**Key Characteristics**:
- 500ms cache validity
- Thread-safe access
- Automatic refresh
- Performance tracking

**Cached Data**:
```cpp
struct CachedEntities
{
    std::vector<Unit*> enemies;
    std::vector<Player*> allies;
    std::vector<GameObject*> interactables;
    uint32 timestamp;
    bool valid;
};
```

---

## Game System Entities

### QuestManager
**File**: `src/modules/Playerbot/Game/QuestManager.h`

**Purpose**: Manages quest discovery, tracking, and completion.

**Key Responsibilities**:
- Quest discovery and prioritization
- Objective tracking
- Turn-in automation
- Reward selection

**Quest State Tracking**:
```cpp
enum class QuestState
{
    AVAILABLE,           // Can be accepted
    ACCEPTED,            // In quest log
    IN_PROGRESS,         // Working on objectives
    OBJECTIVES_COMPLETE, // Ready to turn in
    COMPLETED,           // Turned in
    FAILED               // Failed or abandoned
};
```

**Key Methods**:
```cpp
void DiscoverQuests();
void AcceptQuest(uint32 questId);
void TurnInQuest(uint32 questId);
void UpdateObjectives(uint32 diff);
Quest* SelectBestQuest();
```

---

### InventoryManager
**File**: `src/modules/Playerbot/Game/InventoryManager.h`

**Purpose**: Manages bot inventory, equipment, and consumables.

**Key Responsibilities**:
- Equipment management (auto-equip upgrades)
- Consumable tracking (food, water, potions)
- Bag organization
- Trash item identification

**Key Methods**:
```cpp
bool AutoEquipBestGear();
bool HasConsumable(ConsumableType type, uint32 minCount = 1);
void UseConsumable(ConsumableType type);
std::vector<Item*> GetJunkItems();
bool HasBagSpace(uint32 requiredSlots = 1);
```

---

### NPCInteractionManager
**File**: `src/modules/Playerbot/Game/NPCInteractionManager.h`

**Purpose**: Manages interactions with NPCs (vendors, trainers, quest givers).

**Key Responsibilities**:
- Vendor interactions (buy, sell, repair)
- Trainer interactions (learn spells)
- Quest giver interactions
- Flight master usage

**Key Methods**:
```cpp
void InteractWithVendor(Creature* vendor);
void SellJunkItems(Creature* vendor);
void RepairEquipment(Creature* vendor);
void BuyConsumables(Creature* vendor);
void LearnSpellsFromTrainer(Creature* trainer);
```

---

## Performance System Entities

### ThreadPool
**File**: `src/modules/Playerbot/Performance/ThreadPool/ThreadPool.h`

**Purpose**: Asynchronous task execution for performance optimization.

**Key Characteristics**:
- 50 worker threads (configurable)
- Lock-free task queue
- Task prioritization
- Deadlock prevention

**Key Methods**:
```cpp
template<typename F>
void Enqueue(F&& task);

template<typename F>
std::future<typename std::result_of<F()>::type> EnqueueWithResult(F&& task);

void Shutdown();
uint32 GetActiveThreadCount() const;
uint32 GetQueuedTaskCount() const;
```

**Common Use Cases**:
- Grid queries (TargetScanner)
- Pathfinding calculations
- Database queries
- Complex AI calculations

---

### BotPerformanceMonitor
**File**: `src/modules/Playerbot/Performance/BotPerformanceMonitor.h`

**Purpose**: Real-time performance tracking and alerting.

**Key Metrics**:
```cpp
struct PerformanceSnapshot
{
    uint32 botCount;
    float avgCpuUsagePercent;
    uint64 totalMemoryBytes;
    uint32 avgUpdateTimeMs;
    uint32 maxUpdateTimeMs;
    uint32 frameDrops;
};
```

**Key Methods**:
```cpp
void RecordMetric(std::string const& name, float value);
PerformanceSnapshot GetSnapshot() const;
void SetAlertThreshold(std::string const& metric, float threshold);
```

---

### BotMemoryManager
**File**: `src/modules/Playerbot/Performance/BotMemoryManager.h`

**Purpose**: Memory pooling and allocation optimization.

**Key Optimizations**:
- Object pools for frequent allocations
- Memory arena for temporary objects
- Fragmentation prevention

**Key Methods**:
```cpp
template<typename T>
T* Allocate();

template<typename T>
void Deallocate(T* obj);

void ResetArena();
uint64 GetTotalAllocated() const;
```

---

## Event System Entities

### CombatEventBus
**File**: `src/modules/Playerbot/Combat/CombatEventBus.h`

**Purpose**: Publishes combat-related events to subscribers.

**Key Events**:
```cpp
enum class CombatEventType
{
    SPELL_CAST_START,
    SPELL_CAST_SUCCESS,
    SPELL_INTERRUPT,
    DAMAGE_TAKEN,
    DAMAGE_DEALT,
    TARGET_DIED,
    COMBAT_START,
    COMBAT_END
};
```

**Key Methods**:
```cpp
void Subscribe(CombatEventObserver* observer);
void Unsubscribe(CombatEventObserver* observer);
void Publish(CombatEvent const& event);
```

---

### EventDispatcher (Phase 4)
**File**: `src/modules/Playerbot/Events/EventDispatcher.h`

**Purpose**: Centralized event routing to managers and AI.

**Supported Event Types**:
- `GroupEvent`: Group/raid events
- `CombatEvent`: Combat events
- `CooldownEvent`: Cooldown changes
- `AuraEvent`: Buff/debuff changes
- `LootEvent`: Loot availability
- `QuestEvent`: Quest state changes
- `ResourceEvent`: Health/power changes
- `SocialEvent`: Chat, emotes, guild
- `AuctionEvent`: Auction house events
- `NPCEvent`: NPC interactions
- `InstanceEvent`: Dungeon/raid events

---

## Session Management Entities

### BotWorldSession (extends WorldSession)
**File**: `src/modules/Playerbot/Session/BotWorldSession.h`

**Purpose**: Socket-less session for bot players.

**Key Characteristics**:
- No network socket attached
- No packet I/O overhead
- Minimal session logic
- Optimized for AI-driven behavior

**Key Differences from WorldSession**:
```cpp
// No packet processing
void ProcessIncomingPackets() override {} // No-op

// No network I/O
bool IsConnected() const override { return true; } // Always connected

// Optimized update
void Update(uint32 diff) override;
```

---

### BotWorldSessionMgr
**File**: `src/modules/Playerbot/Session/BotWorldSessionMgr.h`

**Purpose**: Manages all bot sessions and lifecycle.

**Key Responsibilities**:
- Session creation/destruction
- Update distribution (staggered)
- Resource management
- Performance monitoring

**Key Methods**:
```cpp
void AddSession(std::unique_ptr<BotWorldSession> session);
void RemoveSession(ObjectGuid guid);
BotWorldSession* GetSession(ObjectGuid guid);
void Update(uint32 diff);
uint32 GetSessionCount() const;
```

**Update Spreading**:
```cpp
// Stagger bot updates across frames to prevent CPU spikes
uint32 updateSlot = sessionId % UPDATE_INTERVAL;
if (currentTime % UPDATE_INTERVAL == updateSlot)
    session->Update(diff);
```

---

### BotLifecycleManager
**File**: `src/modules/Playerbot/Lifecycle/BotLifecycleManager.h`

**Purpose**: Manages bot creation, spawning, and deletion.

**Key Responsibilities**:
- Bot account creation
- Character creation
- Bot spawning
- Bot despawning
- Resource cleanup

**Key Methods**:
```cpp
bool CreateBot(std::string const& name, uint8 race, uint8 classId);
bool SpawnBot(ObjectGuid guid);
bool DespawnBot(ObjectGuid guid);
void DeleteBot(ObjectGuid guid);
```

---

## Summary

This document catalogs all major entities in the TrinityCore Playerbot system:

1. **Core Entities**: Bot, BotAI, ClassAI
2. **AI System**: Strategy, Action, Trigger, BehaviorPriorityManager
3. **Combat System**: TargetScanner, TargetSelector, PositionManager, InterruptManager, GridQueryProcessor, PreScanCache
4. **Game System**: QuestManager, InventoryManager, NPCInteractionManager
5. **Performance System**: ThreadPool, BotPerformanceMonitor, BotMemoryManager
6. **Event System**: CombatEventBus, EventDispatcher
7. **Session Management**: BotWorldSession, BotWorldSessionMgr, BotLifecycleManager

**Next Steps**: See [PLAYERBOT_FUNCTION_REFERENCE.md](PLAYERBOT_FUNCTION_REFERENCE.md) for detailed function documentation.
