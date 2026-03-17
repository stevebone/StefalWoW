# TrinityCore Playerbot Function Reference

**Document Version**: 1.0
**Last Updated**: 2025-10-16
**Status**: Active Development

## Table of Contents
1. [Core Functions](#core-functions)
2. [AI System Functions](#ai-system-functions)
3. [Combat System Functions](#combat-system-functions)
4. [Quest System Functions](#quest-system-functions)
5. [Performance System Functions](#performance-system-functions)
6. [Session Management Functions](#session-management-functions)
7. [Utility Functions](#utility-functions)

---

## Core Functions

### Bot Lifecycle

#### `BotLifecycleManager::CreateBot()`
```cpp
bool CreateBot(std::string const& name, uint8 race, uint8 classId, uint8 gender = GENDER_MALE)
```
**Purpose**: Creates a new bot character and account.

**Parameters**:
- `name`: Bot character name (must be unique)
- `race`: Race ID (1-36 for all races)
- `classId`: Class ID (1-13 for all classes)
- `gender`: Gender (GENDER_MALE or GENDER_FEMALE)

**Returns**: `true` if bot created successfully, `false` otherwise

**Process**:
1. Validate name uniqueness
2. Create bot account (max 10 characters per account)
3. Generate character data
4. Insert into characters database
5. Set bot flags

**Example**:
```cpp
if (BotLifecycleManager::CreateBot("BotWarrior", RACE_HUMAN, CLASS_WARRIOR))
    LOG_INFO("Bot created successfully");
```

---

#### `BotLifecycleManager::SpawnBot()`
```cpp
bool SpawnBot(ObjectGuid guid, Position const* spawnPos = nullptr)
```
**Purpose**: Spawns an existing bot character into the world.

**Parameters**:
- `guid`: Character GUID
- `spawnPos`: Optional spawn position (uses character's saved position if nullptr)

**Returns**: `true` if bot spawned successfully

**Process**:
1. Load character from database
2. Create BotWorldSession (socket-less)
3. Initialize BotAI
4. Add to world map
5. Start AI update loop

---

#### `BotLifecycleManager::DespawnBot()`
```cpp
void DespawnBot(ObjectGuid guid, bool saveToDb = true)
```
**Purpose**: Removes bot from world and saves state.

**Parameters**:
- `guid`: Bot character GUID
- `saveToDb`: Whether to save character state to database

**Process**:
1. Trigger `BotAI::OnBotRemoved()`
2. Save character to database
3. Remove from map
4. Destroy session
5. Release resources

---

### Session Management

#### `BotWorldSessionMgr::AddSession()`
```cpp
void AddSession(std::unique_ptr<BotWorldSession> session)
```
**Purpose**: Registers a bot session with the session manager.

**Parameters**:
- `session`: Unique pointer to bot session

**Process**:
1. Assign session ID
2. Calculate update slot (for staggered updates)
3. Add to session registry
4. Register with performance monitor

---

#### `BotWorldSessionMgr::Update()`
```cpp
void Update(uint32 diff)
```
**Purpose**: Updates all bot sessions (called by World::Update()).

**Parameters**:
- `diff`: Time since last update (milliseconds)

**Process**:
1. Iterate all active sessions
2. Check update slot (staggered updates)
3. Call `BotWorldSession::Update(diff)`
4. Track performance metrics

**Staggered Update Logic**:
```cpp
// Each session assigned an update slot
uint32 currentSlot = WorldTimer::GetMSTime() % UPDATE_INTERVAL;
for (auto& session : _sessions)
{
    if (session->GetUpdateSlot() == currentSlot)
        session->Update(diff);
}
```

---

## AI System Functions

### BotAI Core

#### `BotAI::UpdateAI()`
```cpp
virtual void UpdateAI(uint32 diff)
```
**Purpose**: Main AI update loop (single entry point).

**Parameters**:
- `diff`: Time since last update (milliseconds)

**Process**:
1. Update behavior context (combat state, health, resources)
2. Select active strategy (`SelectBestStrategy()`)
3. Process triggers (`ProcessTriggers()`)
4. Execute actions (`UpdateActions()`)
5. Update managers (`UpdateManagers()`)
6. Update movement (`UpdateMovement()`)
7. Update solo behaviors (`UpdateSoloBehaviors()`)
8. Check combat state (`UpdateCombatState()`)
9. If in combat, call `OnCombatUpdate(diff)` (virtual)

**Critical**: Runs every frame, no throttling.

---

#### `BotAI::OnCombatUpdate()`
```cpp
virtual void OnCombatUpdate(uint32 diff)
```
**Purpose**: Class-specific combat update (virtual hook for ClassAI).

**Parameters**:
- `diff`: Time since last update (milliseconds)

**Usage**:
```cpp
// In WarriorAI.cpp
void WarriorAI::OnCombatUpdate(uint32 diff)
{
    Unit* target = GetTargetUnit();
    if (!target)
        return;

    // Execute rotation
    ExecuteCombatRotation(target);
}
```

**Override Guidelines**:
- DO: Execute combat rotation
- DO: Manage class resources
- DO: Use cooldowns appropriately
- DON'T: Control movement (handled by strategies)
- DON'T: Throttle updates (causes issues)

---

#### `BotAI::ExecuteAction()`
```cpp
bool ExecuteAction(std::string const& actionName, ActionContext const& context = {})
```
**Purpose**: Executes a named action with context.

**Parameters**:
- `actionName`: Name of action to execute (e.g., "CastSpell", "MoveTo")
- `context`: Action execution context (target, position, spell ID, etc.)

**Returns**: `true` if action executed successfully

**Process**:
1. Look up action by name
2. Check preconditions (`Action::CanExecute()`)
3. Execute action (`Action::Execute()`)
4. Handle result (success, failure, in progress)

**Example**:
```cpp
ActionContext ctx;
ctx.target = enemy;
ctx.spellId = SPELL_MORTAL_STRIKE;

if (ExecuteAction("CastSpell", ctx))
    LOG_DEBUG("Cast Mortal Strike");
```

---

### Strategy Management

#### `BotAI::AddStrategy()`
```cpp
void AddStrategy(std::unique_ptr<Strategy> strategy)
```
**Purpose**: Registers a strategy with the AI.

**Parameters**:
- `strategy`: Unique pointer to strategy instance

**Example**:
```cpp
auto followStrategy = std::make_unique<FollowStrategy>(this);
AddStrategy(std::move(followStrategy));
```

---

#### `BotAI::ActivateStrategy()`
```cpp
void ActivateStrategy(std::string const& name)
```
**Purpose**: Activates a registered strategy by name.

**Parameters**:
- `name`: Strategy name (e.g., "Follow", "Combat", "Quest")

**Process**:
1. Find strategy by name
2. Call `Strategy::OnActivate()`
3. Add to active strategies list
4. Deactivate conflicting strategies

---

### Behavior Priority

#### `BehaviorPriorityManager::SelectBestBehavior()`
```cpp
BehaviorPriority SelectBestBehavior(BotAI* ai) const
```
**Purpose**: Selects highest-priority active behavior.

**Returns**: Priority enum representing selected behavior

**Priority Evaluation**:
```cpp
enum class BehaviorPriority : uint8_t
{
    CRITICAL = 10,  // Death, fleeing, resurrection
    HIGH = 8,       // Combat, healing critical target
    MEDIUM = 5,     // Questing, gathering, following
    LOW = 3,        // Buffing, idle behavior
    IDLE = 1        // Waiting, resting
};
```

**Logic**:
1. Evaluate all active behaviors
2. Apply context-specific rules
3. Resolve conflicts (combat > questing)
4. Return highest priority

---

## Combat System Functions

### Target Scanning

#### `TargetScanner::FindBestTarget()`
```cpp
Unit* FindBestTarget(float range = 0.0f)
```
**Purpose**: Finds the optimal combat target within range.

**Parameters**:
- `range`: Scan radius (0 = use class default)

**Returns**: Best target Unit*, or nullptr if none found

**Process**:
1. Check PreScanCache for valid targets
2. If cache invalid, submit async grid query
3. Prioritize targets:
   - Attacking bot/allies (PRIORITY_CRITICAL = 10)
   - Casters/healers (PRIORITY_CASTER = 8)
   - Elites (PRIORITY_ELITE = 6)
   - Normal mobs (PRIORITY_NORMAL = 4)
4. Apply blacklist filter
5. Return highest priority target

**Example**:
```cpp
Unit* target = _targetScanner->FindBestTarget();
if (target)
{
    SetTarget(target->GetGUID());
    OnCombatStart(target);
}
```

---

#### `TargetScanner::IsValidTarget()`
```cpp
bool IsValidTarget(Unit* target) const
```
**Purpose**: Validates if a unit is a valid combat target.

**Parameters**:
- `target`: Unit to validate

**Returns**: `true` if target is valid

**Validation Checks**:
- Is alive?
- Is hostile?
- Is attackable?
- Is in range?
- Is not blacklisted?
- Is not crowd-controlled (if applicable)?
- Has line of sight?

---

### Target Selection

#### `TargetSelector::SelectBestTarget()`
```cpp
Unit* SelectBestTarget(std::vector<ScanResult> const& results)
```
**Purpose**: Selects optimal target from scan results.

**Parameters**:
- `results`: Vector of scan results with priority scores

**Returns**: Selected target Unit*

**Selection Algorithm**:
```cpp
1. Filter by validity (IsValidTarget())
2. Sort by priority (descending)
3. If equal priority, sort by threat (descending)
4. If equal threat, sort by distance (ascending)
5. Return top result
```

---

### Position Management

#### `PositionManager::CalculateOptimalPosition()`
```cpp
Position CalculateOptimalPosition(Unit* target, CombatRole role) const
```
**Purpose**: Calculates optimal combat position based on role.

**Parameters**:
- `target`: Combat target
- `role`: Bot's role (TANK, HEALER, MELEE_DPS, RANGED_DPS)

**Returns**: Position struct with X, Y, Z coordinates

**Role-Based Logic**:

**Tank**:
```cpp
Position CalculateTankPosition(Unit* target)
{
    // Face target, 0-5 yards
    Position pos = target->GetPosition();
    pos.RelocateOffset(target->GetOrientation() + M_PI, 3.0f);
    return pos;
}
```

**Melee DPS**:
```cpp
Position CalculateMeleeDPSPosition(Unit* target)
{
    // Behind target, 0-5 yards (avoid parry/block)
    Position pos = target->GetPosition();
    float behindAngle = target->GetOrientation() + M_PI;
    pos.RelocateOffset(behindAngle, 2.0f);
    return pos;
}
```

**Ranged DPS**:
```cpp
Position CalculateRangedDPSPosition(Unit* target)
{
    // 20-40 yards, LoS required
    Position pos = GetBot()->GetPosition();
    float distance = GetBot()->GetExactDist(target);

    if (distance < 20.0f)
        // Move away to min range
        pos = CalculateRetreatPosition(target, 25.0f);
    else if (distance > 40.0f)
        // Move closer to max range
        pos = CalculateApproachPosition(target, 35.0f);

    return pos;
}
```

**Healer**:
```cpp
Position CalculateHealerPosition(Group* group)
{
    // 15-40 yards from group center, LoS to all members
    Position groupCenter = CalculateGroupCenter(group);
    Position pos = groupCenter;
    pos.RelocateOffset(GetBot()->GetOrientation(), 20.0f);

    // Ensure LoS to all group members
    if (!HasLosToAllGroupMembers(pos, group))
        pos = FindPositionWithLos(group);

    return pos;
}
```

---

### Interrupt Coordination

#### `InterruptManager::ShouldInterrupt()`
```cpp
bool ShouldInterrupt(Unit* target, Spell* spell) const
```
**Purpose**: Determines if a spell cast should be interrupted.

**Parameters**:
- `target`: Unit casting the spell
- `spell`: Spell being cast

**Returns**: `true` if interrupt should be attempted

**Priority Logic**:
```cpp
InterruptPriority GetInterruptPriority(Spell* spell)
{
    // Healing spells - always interrupt
    if (spell->IsHealSpell())
        return InterruptPriority::CRITICAL; // 10

    // Dangerous AoE - high priority
    if (spell->IsAoEDamage() && spell->GetMaxRadius() > 10.0f)
        return InterruptPriority::CRITICAL;

    // Large damage spells
    if (spell->GetBaseDamage() > GetBot()->GetMaxHealth() * 0.3f)
        return InterruptPriority::HIGH; // 7

    // Buffs
    if (spell->IsPositive())
        return InterruptPriority::MEDIUM; // 4

    return InterruptPriority::LOW; // 2
}
```

---

#### `InterruptManager::CoordinateInterrupt()`
```cpp
bool CoordinateInterrupt(Unit* target, Spell* spell)
```
**Purpose**: Coordinates interrupt with group to avoid overlap.

**Parameters**:
- `target`: Unit casting spell
- `spell`: Spell to interrupt

**Returns**: `true` if this bot should interrupt

**Coordination Logic**:
1. Check if anyone else already interrupting (via event bus)
2. Check own interrupt cooldown
3. Claim interrupt (publish intent to event bus)
4. Execute interrupt spell
5. Notify success/failure via event bus

---

### Grid Query Optimization

#### `GridQueryProcessor::FindEnemies()`
```cpp
static std::vector<Unit*> FindEnemies(Player* bot, float range)
```
**Purpose**: Optimized grid query for enemy units.

**Parameters**:
- `bot`: Bot player
- `range`: Search radius

**Returns**: Vector of enemy Units

**Optimization**:
- Submitted to ThreadPool (async execution)
- Uses spatial indexing
- Early exit on max results
- Result timeout (2 seconds max)

**Usage**:
```cpp
// Async variant
GridQueryProcessor::FindEnemiesAsync(bot, 40.0f, [this](std::vector<Unit*> enemies) {
    // Callback on main thread
    for (Unit* enemy : enemies)
        ProcessEnemy(enemy);
});
```

---

### PreScan Cache

#### `PreScanCache::GetCachedEnemies()`
```cpp
std::vector<Unit*> GetCachedEnemies() const
```
**Purpose**: Returns cached enemy list (if valid).

**Returns**: Vector of enemy Units, or empty if cache invalid

**Cache Validity**:
- Valid for 500ms
- Automatically refreshed on access if expired
- Thread-safe access

**Performance**:
- Cache hit: ~0.01ms (near-instant)
- Cache miss: ~5-10ms (grid query)

---

## Quest System Functions

### Quest Discovery

#### `QuestManager::DiscoverQuests()`
```cpp
void DiscoverQuests()
```
**Purpose**: Scans for available quests and prioritizes them.

**Process**:
1. Query quests from database
2. Filter by level (bot level ± 3)
3. Filter by prerequisites (previous quests, reputation)
4. Filter by class/race restrictions
5. Prioritize quests:
   - Group quests (if in group)
   - Nearby quests (minimize travel)
   - Chain quests (storyline continuation)
   - Rewarding quests (gear upgrades, gold)

**Called**: Every 30 seconds when solo, or on quest completion

---

#### `QuestManager::AcceptQuest()`
```cpp
bool AcceptQuest(uint32 questId)
```
**Purpose**: Accepts a quest from a quest giver.

**Parameters**:
- `questId`: Quest entry ID

**Returns**: `true` if quest accepted successfully

**Process**:
1. Find quest giver NPC
2. Pathfind to NPC (within 5 yards)
3. Open quest dialogue (`NPCInteractionManager::InteractWithQuestGiver()`)
4. Accept quest
5. Add to quest log
6. Initialize objective tracking

---

### Quest Tracking

#### `QuestManager::UpdateObjectives()`
```cpp
void UpdateObjectives(uint32 diff)
```
**Purpose**: Updates progress on active quest objectives.

**Parameters**:
- `diff`: Time since last update (milliseconds)

**Process**:
1. Iterate active quests
2. Check each objective type:
   - **Kill objectives**: Check if killed target matches quest requirement
   - **Collect objectives**: Count quest items in inventory
   - **Use item objectives**: Track item usage on GameObjects
   - **Talk objectives**: Track NPC dialogue completion
   - **Explore objectives**: Check if reached location
3. Mark completed objectives
4. If all objectives complete, trigger turn-in

---

#### `QuestManager::TurnInQuest()`
```cpp
bool TurnInQuest(uint32 questId)
```
**Purpose**: Turns in a completed quest.

**Parameters**:
- `questId`: Quest entry ID

**Returns**: `true` if quest turned in successfully

**Process**:
1. Find turn-in NPC (may differ from quest giver)
2. Pathfind to NPC
3. Open turn-in dialogue
4. Select reward (`SelectBestReward()`)
5. Complete quest
6. Receive experience, reputation, reward item

---

#### `QuestManager::SelectBestReward()`
```cpp
uint32 SelectBestReward(Quest const* quest)
```
**Purpose**: Selects the best reward from quest rewards.

**Parameters**:
- `quest`: Quest object

**Returns**: Reward choice index

**Selection Priority**:
1. Equipment upgrade (higher item level)
2. Primary stat priority (Strength for warriors, Intellect for mages, etc.)
3. Secondary stat priority (Haste, Crit, Mastery)
4. Gold (if no equipment upgrade)

---

## Performance System Functions

### ThreadPool

#### `ThreadPool::Enqueue()`
```cpp
template<typename F>
void Enqueue(F&& task)
```
**Purpose**: Submits a task for asynchronous execution.

**Parameters**:
- `task`: Callable object (lambda, function pointer, functor)

**Example**:
```cpp
ThreadPool::Instance()->Enqueue([this, bot]() {
    // Worker thread context
    auto enemies = PerformExpensiveGridQuery(bot);

    // Schedule callback on main thread
    bot->AddDelayedCallback([this, enemies]() {
        ProcessEnemies(enemies);
    });
});
```

---

#### `ThreadPool::EnqueueWithResult()`
```cpp
template<typename F>
std::future<typename std::result_of<F()>::type> EnqueueWithResult(F&& task)
```
**Purpose**: Submits a task and returns a future for the result.

**Parameters**:
- `task`: Callable object returning a value

**Returns**: `std::future<T>` where T is the task's return type

**Example**:
```cpp
auto future = ThreadPool::Instance()->EnqueueWithResult([bot]() -> std::vector<Unit*> {
    return GridQueryProcessor::FindEnemies(bot, 40.0f);
});

// Wait for result (max 100ms)
if (future.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready)
{
    auto enemies = future.get();
    ProcessEnemies(enemies);
}
```

---

### Performance Monitoring

#### `BotPerformanceMonitor::RecordMetric()`
```cpp
void RecordMetric(std::string const& name, float value)
```
**Purpose**: Records a performance metric for monitoring.

**Parameters**:
- `name`: Metric name (e.g., "bot_update_ms", "target_scan_ms")
- `value`: Metric value

**Example**:
```cpp
auto start = std::chrono::steady_clock::now();
ExecuteCombatRotation(target);
auto end = std::chrono::steady_clock::now();

auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
BotPerformanceMonitor::Instance()->RecordMetric("combat_rotation_us", duration.count());
```

---

#### `BotPerformanceMonitor::GetSnapshot()`
```cpp
PerformanceSnapshot GetSnapshot() const
```
**Purpose**: Returns current performance snapshot.

**Returns**: Struct containing current metrics

**Snapshot Contents**:
```cpp
struct PerformanceSnapshot
{
    uint32 botCount;              // Number of active bots
    float avgCpuUsagePercent;     // Average CPU usage
    uint64 totalMemoryBytes;      // Total memory used
    uint32 avgUpdateTimeMs;       // Average bot update time
    uint32 maxUpdateTimeMs;       // Max bot update time
    uint32 frameDrops;            // Frame drops in last minute
};
```

---

## Session Management Functions

### BotWorldSession

#### `BotWorldSession::Update()`
```cpp
void Update(uint32 diff) override
```
**Purpose**: Updates bot session (minimal overhead).

**Parameters**:
- `diff`: Time since last update (milliseconds)

**Process**:
1. Update session timers
2. Call `Bot::Update(diff)` (which calls `BotAI::UpdateAI()`)
3. Process delayed callbacks (from async tasks)

**Critical**: No packet processing overhead (socket-less session)

---

## Utility Functions

### ObjectCache

#### `ObjectCache::GetUnit()`
```cpp
Unit* GetUnit(ObjectGuid guid)
```
**Purpose**: Gets a Unit from cache (avoids ObjectAccessor deadlock).

**Parameters**:
- `guid`: Object GUID

**Returns**: Unit pointer, or nullptr if not found

**Deadlock Prevention**:
- Cache refreshed once per `BotAI::UpdateAI()` cycle
- Eliminates recursive `ObjectAccessor` calls
- Thread-safe access

---

### Configuration

#### `PlayerbotConfig::GetMaxBots()`
```cpp
static uint32 GetMaxBots()
```
**Purpose**: Returns maximum number of bots allowed server-wide.

**Returns**: Max bot count (from playerbots.conf)

**Config Example**:
```ini
Playerbot.MaxBots = 500
```

---

#### `PlayerbotConfig::IsEnabled()`
```cpp
static bool IsEnabled()
```
**Purpose**: Checks if playerbot system is enabled.

**Returns**: `true` if enabled

**Config Example**:
```ini
Playerbot.Enable = 1
```

---

## Summary

This function reference provides detailed documentation for:

1. **Core Functions**: Bot lifecycle, session management
2. **AI System**: UpdateAI, strategy management, action execution
3. **Combat System**: Target scanning, selection, positioning, interrupts
4. **Quest System**: Discovery, acceptance, tracking, turn-in
5. **Performance System**: ThreadPool, performance monitoring
6. **Session Management**: Session updates, lifecycle
7. **Utility Functions**: Caching, configuration

**Usage**: Reference this document when implementing new features or debugging existing systems.

**Next Steps**: See [WOW_11_2_FEATURE_MATRIX.md](WOW_11_2_FEATURE_MATRIX.md) for WoW 11.2 feature gap analysis.
