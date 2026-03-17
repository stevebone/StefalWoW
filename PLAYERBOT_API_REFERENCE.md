# TrinityCore Playerbot API Reference

**Version:** WoW 11.2 Compatible
**Target:** 5000 concurrent bots
**Module Location:** `src/modules/Playerbot/`

---

## Table of Contents

1. [Core APIs](#core-apis)
   - [BotSpawner](#botspawner)
   - [BotSession](#botsession)
   - [BotAI](#botai)
   - [ClassAI](#classai)
2. [Subsystems](#subsystems)
   - [Group Coordination](#group-coordination)
   - [Movement & Positioning](#movement--positioning)
   - [Combat Mechanics](#combat-mechanics)
   - [Database Integration](#database-integration)
   - [Performance Monitoring](#performance-monitoring)
3. [Configuration](#configuration)
4. [Extension Points](#extension-points)

---

## Core APIs

### BotSpawner

**File:** `src/modules/Playerbot/Lifecycle/BotSpawner.h`
**Singleton Access:** `sBotSpawner` or `BotSpawner::instance()`

Bot lifecycle manager responsible for creating, spawning, and despawning bots with support for 5000 concurrent bots.

#### Core Methods

##### `Initialize()`
```cpp
bool BotSpawner::Initialize()
```

Initializes the bot spawner system.

**Returns:** `true` on success, `false` on failure

**Example:**
```cpp
if (!sBotSpawner->Initialize()) {
    TC_LOG_ERROR("module.playerbot", "Failed to initialize BotSpawner");
    return false;
}
```

---

##### `SpawnBot(SpawnRequest const& request)`
```cpp
bool BotSpawner::SpawnBot(SpawnRequest const& request)
```

Spawns a single bot based on a spawn request.

**Parameters:**
- `request` - Spawn request containing zone, level, class requirements

**Returns:** `true` if spawn initiated successfully

**Thread Safety:** Thread-safe with internal mutex protection

**Example:**
```cpp
SpawnRequest request;
request.type = SpawnRequest::SPECIFIC_ZONE;
request.zoneId = 12; // Elwynn Forest
request.minLevel = 1;
request.maxLevel = 10;

if (sBotSpawner->SpawnBot(request)) {
    TC_LOG_INFO("module.playerbot", "Bot spawn request submitted");
}
```

**Spawn Request Types:**
```cpp
enum SpawnRequestType {
    RANDOM,          // Spawn random bot in any zone
    SPECIFIC_ZONE,   // Spawn in specific zone
    SPECIFIC_MAP,    // Spawn in specific map
    FOLLOW_PLAYER    // Spawn near player for following
};
```

---

##### `SpawnBots(std::vector<SpawnRequest> const& requests)`
```cpp
uint32 BotSpawner::SpawnBots(std::vector<SpawnRequest> const& requests)
```

Batch spawn multiple bots for better performance.

**Parameters:**
- `requests` - Vector of spawn requests

**Returns:** Number of successfully queued requests

**Performance:** Optimized for batch processing with configurable batch size (default: 10 bots/batch)

**Example:**
```cpp
std::vector<SpawnRequest> batch;
for (uint32 i = 0; i < 50; ++i) {
    SpawnRequest req;
    req.type = SpawnRequest::RANDOM;
    batch.push_back(req);
}

uint32 queued = sBotSpawner->SpawnBots(batch);
TC_LOG_INFO("module.playerbot", "Queued {} bot spawn requests", queued);
```

---

##### `DespawnBot(ObjectGuid guid, bool forced = false)`
```cpp
void BotSpawner::DespawnBot(ObjectGuid guid, bool forced = false)
```

Despawns a specific bot.

**Parameters:**
- `guid` - Character GUID of bot to despawn
- `forced` - If true, force immediate despawn regardless of state

**Thread Safety:** Thread-safe

**Example:**
```cpp
ObjectGuid botGuid = ...; // Get from player
sBotSpawner->DespawnBot(botGuid, false);
```

---

##### `GetActiveBotCount()`
```cpp
uint32 BotSpawner::GetActiveBotCount() const
```

Returns the total number of active bots.

**Returns:** Number of currently active bots

**Performance:** Lock-free atomic operation - safe to call frequently

**Example:**
```cpp
uint32 activeBots = sBotSpawner->GetActiveBotCount();
TC_LOG_INFO("module.playerbot", "Currently {} bots active", activeBots);
```

---

##### `CreateAndSpawnBot()`
```cpp
bool BotSpawner::CreateAndSpawnBot(
    uint32 masterAccountId,
    uint8 classId,
    uint8 race,
    uint8 gender,
    std::string const& name,
    ObjectGuid& outCharacterGuid)
```

Creates a new bot character and spawns it immediately.

**Parameters:**
- `masterAccountId` - Owner account ID
- `classId` - Character class (1-13)
- `race` - Character race
- `gender` - Character gender (0=male, 1=female)
- `name` - Character name
- `outCharacterGuid` - Output parameter for created character GUID

**Returns:** `true` on success, `false` on failure

**Used By:** `.bot spawn` command

**Example:**
```cpp
ObjectGuid characterGuid;
if (sBotSpawner->CreateAndSpawnBot(
    accountId,
    CLASS_WARRIOR,
    RACE_HUMAN,
    GENDER_MALE,
    "BotWarrior",
    characterGuid))
{
    TC_LOG_INFO("module.playerbot", "Created and spawned bot: {}", characterGuid.ToString());
}
```

---

##### `GetStats()`
```cpp
SpawnStats const& BotSpawner::GetStats() const
```

Retrieves spawn statistics for monitoring.

**Returns:** Reference to spawn statistics structure

**Statistics Available:**
```cpp
struct SpawnStats {
    std::atomic<uint32> totalSpawned;      // Total bots spawned
    std::atomic<uint32> totalDespawned;    // Total bots despawned
    std::atomic<uint32> currentlyActive;   // Currently active
    std::atomic<uint32> peakConcurrent;    // Peak concurrent count
    std::atomic<uint32> failedSpawns;      // Failed spawn attempts
    std::atomic<uint64> totalSpawnTime;    // Total spawn time (µs)
    std::atomic<uint32> spawnAttempts;     // Total attempts

    float GetAverageSpawnTime() const;     // Average time in ms
    float GetSuccessRate() const;          // Success rate %
};
```

**Example:**
```cpp
SpawnStats const& stats = sBotSpawner->GetStats();
TC_LOG_INFO("module.playerbot", "Bot Statistics:");
TC_LOG_INFO("module.playerbot", "  Total Spawned: {}", stats.totalSpawned.load());
TC_LOG_INFO("module.playerbot", "  Currently Active: {}", stats.currentlyActive.load());
TC_LOG_INFO("module.playerbot", "  Success Rate: {:.2f}%", stats.GetSuccessRate());
TC_LOG_INFO("module.playerbot", "  Avg Spawn Time: {:.2f}ms", stats.GetAverageSpawnTime());
```

---

#### Configuration

```cpp
struct SpawnConfig {
    uint32 maxBotsTotal = 500;           // Global bot limit
    uint32 maxBotsPerZone = 50;          // Per-zone limit
    uint32 maxBotsPerMap = 200;          // Per-map limit
    uint32 spawnBatchSize = 10;          // Batch processing size
    uint32 spawnDelayMs = 100;           // Delay between batches
    bool enableDynamicSpawning = true;   // Auto-spawn based on players
    bool respectPopulationCaps = true;   // Enforce limits
    float botToPlayerRatio = 2.0f;       // Bots per player
};
```

**Access:**
```cpp
SpawnConfig const& config = sBotSpawner->GetConfig();
```

**Modify:**
```cpp
sBotSpawner->SetMaxBots(1000);
sBotSpawner->SetBotToPlayerRatio(3.0f);
```

---

### BotSession

**File:** `src/modules/Playerbot/Session/BotSession.h`
**Inherits:** `WorldSession`

Represents a bot's world session without network socket.

#### Core Methods

##### `Create(uint32 bnetAccountId)`
```cpp
static std::shared_ptr<BotSession> BotSession::Create(uint32 bnetAccountId)
```

Factory method to create a bot session.

**Parameters:**
- `bnetAccountId` - Battle.net account ID

**Returns:** Shared pointer to created session

**Thread Safety:** Thread-safe

**Example:**
```cpp
auto session = BotSession::Create(accountId);
if (!session) {
    TC_LOG_ERROR("module.playerbot", "Failed to create bot session");
    return;
}
```

---

##### `LoginCharacter(ObjectGuid characterGuid)`
```cpp
bool BotSession::LoginCharacter(ObjectGuid characterGuid)
```

Asynchronously loads and logs in a bot character.

**Parameters:**
- `characterGuid` - Character GUID to login

**Returns:** `true` if login initiated successfully

**Async:** Uses TrinityCore's async query holder pattern

**Example:**
```cpp
if (botSession->LoginCharacter(characterGuid)) {
    TC_LOG_INFO("module.playerbot", "Bot login initiated for {}", characterGuid.ToString());
    // Login completes asynchronously via HandleBotPlayerLogin callback
}
```

---

##### `Update(uint32 diff, PacketFilter& updater)`
```cpp
bool BotSession::Update(uint32 diff, PacketFilter& updater)
```

Updates bot session (called every frame).

**Parameters:**
- `diff` - Time since last update (milliseconds)
- `updater` - Packet filter for processing

**Returns:** `true` if session is still active

**Called By:** `BotWorldSessionMgr::Update()`

**Performance:** Includes comprehensive memory safety checks

---

##### `GetAI()`
```cpp
BotAI* BotSession::GetAI() const
```

Retrieves the bot's AI controller.

**Returns:** Pointer to BotAI or nullptr

**Example:**
```cpp
if (BotAI* ai = botSession->GetAI()) {
    ai->SetAIState(BotAIState::COMBAT);
}
```

---

##### `SendPacket(WorldPacket const* packet, bool forced = false)`
```cpp
void BotSession::SendPacket(WorldPacket const* packet, bool forced = false) override
```

Sends a packet to the bot (stored in queue, no network).

**Parameters:**
- `packet` - Packet to send
- `forced` - Ignored for bots (no network)

**Special Handling:** Intercepts `SMSG_PARTY_INVITE` for automatic group joining

**Example:**
```cpp
WorldPacket packet(SMSG_PARTY_INVITE);
// ... build packet
botSession->SendPacket(&packet);
```

---

#### Login State Management

```cpp
enum class LoginState : uint8 {
    NONE,                // Not logging in
    LOGIN_IN_PROGRESS,   // Async login in progress
    LOGIN_COMPLETE,      // Login successful
    LOGIN_FAILED         // Login failed
};
```

**State Queries:**
```cpp
LoginState state = botSession->GetLoginState();
bool loggedIn = botSession->IsLoginComplete();
bool failed = botSession->IsLoginFailed();
```

---

### BotAI

**File:** `src/modules/Playerbot/AI/BotAI.h`
**Factory:** `sBotAIFactory->CreateAI(Player* bot)`

Base AI controller for all bots with strategy-based behavior system.

#### Core Methods

##### `UpdateAI(uint32 diff)`
```cpp
virtual void BotAI::UpdateAI(uint32 diff)
```

Main update method - SINGLE entry point for all AI updates.

**Parameters:**
- `diff` - Time since last update (milliseconds)

**Update Flow:**
1. Update core behaviors (strategies, movement, idle)
2. Check combat state transitions
3. If in combat, call `OnCombatUpdate()` for class-specific logic

**Performance:** Runs every frame - no throttling for smooth movement

**Override:** Do NOT override - use `OnCombatUpdate()` instead

---

##### `OnCombatUpdate(uint32 diff)`
```cpp
virtual void BotAI::OnCombatUpdate(uint32 diff)
```

Virtual method for class-specific COMBAT ONLY updates.

**Parameters:**
- `diff` - Time since last update (milliseconds)

**Called When:** Bot is in combat state

**Override:** ClassAI implementations should override for combat rotations

**Must Not:**
- Control movement (handled by strategies)
- Throttle updates (causes following issues)
- Call base UpdateAI (would cause recursion)

**Example (in ClassAI):**
```cpp
void WarriorAI::OnCombatUpdate(uint32 diff) {
    Unit* target = GetTargetUnit();
    if (!target)
        return;

    // Execute rotation
    UpdateRotation(target);

    // Update cooldowns
    UpdateCooldowns(diff);
}
```

---

##### Strategy Management

###### `AddStrategy(std::unique_ptr<Strategy> strategy)`
```cpp
void BotAI::AddStrategy(std::unique_ptr<Strategy> strategy)
```

Registers a new strategy with the AI.

**Parameters:**
- `strategy` - Unique pointer to strategy (ownership transferred)

**Example:**
```cpp
auto followStrategy = std::make_unique<LeaderFollowBehavior>();
botAI->AddStrategy(std::move(followStrategy));
```

---

###### `ActivateStrategy(std::string const& name)`
```cpp
void BotAI::ActivateStrategy(std::string const& name)
```

Activates a registered strategy.

**Parameters:**
- `name` - Strategy name

**Thread Safety:** Thread-safe with mutex

**Example:**
```cpp
botAI->ActivateStrategy("follow");
```

---

###### `DeactivateStrategy(std::string const& name)`
```cpp
void BotAI::DeactivateStrategy(std::string const& name)
```

Deactivates an active strategy.

**Parameters:**
- `name` - Strategy name

**Example:**
```cpp
botAI->DeactivateStrategy("follow");
```

---

##### Group Management

###### `OnGroupJoined(Group* group)`
```cpp
void BotAI::OnGroupJoined(Group* group)
```

Called when bot joins a group.

**Parameters:**
- `group` - Group joined

**Actions:**
- Activates "follow" strategy
- Activates "group_combat" strategy
- Sets AI state to FOLLOWING

**Example:**
```cpp
if (Group* group = bot->GetGroup()) {
    botAI->OnGroupJoined(group);
}
```

---

###### `OnGroupLeft()`
```cpp
void BotAI::OnGroupLeft()
```

Called when bot leaves a group.

**Actions:**
- Deactivates "follow" strategy
- Deactivates "group_combat" strategy
- Sets AI state to IDLE

---

##### State Management

###### `GetAIState()`
```cpp
BotAIState BotAI::GetAIState() const
```

Returns current AI state.

**AI States:**
```cpp
enum class BotAIState {
    IDLE,        // Doing nothing
    COMBAT,      // In combat
    DEAD,        // Dead
    TRAVELLING,  // Moving to destination
    QUESTING,    // Doing quests
    GATHERING,   // Gathering resources
    TRADING,     // At vendor/trading
    FOLLOWING,   // Following group leader
    FLEEING,     // Running away
    RESTING      // Resting for health/mana
};
```

---

###### `SetAIState(BotAIState state)`
```cpp
void BotAI::SetAIState(BotAIState state)
```

Sets AI state with logging.

**Parameters:**
- `state` - New AI state

---

##### Action System

###### `QueueAction(std::shared_ptr<Action> action, ActionContext const& context)`
```cpp
void BotAI::QueueAction(std::shared_ptr<Action> action, ActionContext const& context = {})
```

Queues an action for execution.

**Parameters:**
- `action` - Shared pointer to action
- `context` - Optional action context

**Example:**
```cpp
auto healAction = std::make_shared<HealAction>(targetGuid, spellId);
botAI->QueueAction(healAction);
```

---

###### `CancelCurrentAction()`
```cpp
void BotAI::CancelCurrentAction()
```

Cancels currently executing action.

---

##### Performance Metrics

```cpp
struct PerformanceMetrics {
    std::atomic<uint32> totalUpdates;
    std::atomic<uint32> actionsExecuted;
    std::atomic<uint32> triggersProcessed;
    std::atomic<uint32> strategiesEvaluated;
    std::chrono::microseconds averageUpdateTime;
    std::chrono::microseconds maxUpdateTime;
    std::chrono::steady_clock::time_point lastUpdate;
};
```

**Access:**
```cpp
PerformanceMetrics const& metrics = botAI->GetPerformanceMetrics();
TC_LOG_INFO("module.playerbot", "AI Performance:");
TC_LOG_INFO("module.playerbot", "  Total Updates: {}", metrics.totalUpdates.load());
TC_LOG_INFO("module.playerbot", "  Avg Update Time: {}µs", metrics.averageUpdateTime.count());
```

---

### ClassAI

**File:** `src/modules/Playerbot/AI/ClassAI/ClassAI.h`
**Inherits:** `BotAI`
**Factory:** `ClassAIFactory::CreateClassAI(Player* bot)`

Base class for class-specific combat AI implementations.

#### Design Principles

1. **Combat Only:** ClassAI handles ONLY combat specialization
2. **No Base Override:** Never override `UpdateAI()` - use `OnCombatUpdate()`
3. **No Movement Control:** Let BotAI strategies handle movement
4. **No Throttling:** Don't throttle updates - causes following issues

#### Pure Virtual Methods

##### `UpdateRotation(Unit* target)`
```cpp
virtual void ClassAI::UpdateRotation(::Unit* target) = 0
```

Executes class-specific combat rotation.

**Parameters:**
- `target` - Current combat target

**Must Implement:** All derived ClassAI classes

**Example (Warrior):**
```cpp
void WarriorAI::UpdateRotation(Unit* target) {
    // Execute warrior rotation
    if (CanUseAbility(SPELL_MORTAL_STRIKE) && IsSpellReady(SPELL_MORTAL_STRIKE)) {
        CastSpell(target, SPELL_MORTAL_STRIKE);
        return;
    }

    if (GetBot()->GetRage() > 30 && CanUseAbility(SPELL_HEROIC_STRIKE)) {
        CastSpell(target, SPELL_HEROIC_STRIKE);
    }
}
```

---

##### `UpdateBuffs()`
```cpp
virtual void ClassAI::UpdateBuffs() = 0
```

Applies class-specific buffs.

**Called:** When not in combat or between combats

**Example (Paladin):**
```cpp
void PaladinAI::UpdateBuffs() {
    // Maintain seal
    if (!HasAura(SPELL_SEAL_OF_RIGHTEOUSNESS)) {
        CastSpell(SPELL_SEAL_OF_RIGHTEOUSNESS);
    }

    // Apply blessing
    if (Group* group = GetBot()->GetGroup()) {
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next()) {
            Player* member = ref->GetSource();
            if (member && !HasAura(SPELL_BLESSING_OF_KINGS, member)) {
                CastSpell(member, SPELL_BLESSING_OF_KINGS);
            }
        }
    }
}
```

---

##### `HasEnoughResource(uint32 spellId)`
```cpp
virtual bool ClassAI::HasEnoughResource(uint32 spellId) = 0
```

Checks if bot has sufficient resources for a spell.

**Parameters:**
- `spellId` - Spell to check

**Returns:** `true` if sufficient resources

---

##### `GetOptimalRange(Unit* target)`
```cpp
virtual float ClassAI::GetOptimalRange(::Unit* target) = 0
```

Returns optimal combat range for the class.

**Parameters:**
- `target` - Combat target

**Returns:** Preferred combat range in yards

**Example:**
```cpp
float MageAI::GetOptimalRange(Unit* target) {
    return 30.0f; // Ranged caster - maintain distance
}

float RogueAI::GetOptimalRange(Unit* target) {
    return 5.0f; // Melee - close combat
}
```

---

#### Utility Methods

##### Spell Casting

```cpp
bool CastSpell(::Unit* target, uint32 spellId);  // Cast on target
bool CastSpell(uint32 spellId);                  // Self-cast
bool IsSpellReady(uint32 spellId);               // Check cooldown
bool IsInRange(::Unit* target, uint32 spellId);  // Check range
bool IsSpellUsable(uint32 spellId);              // Check usability
uint32 GetSpellCooldown(uint32 spellId);         // Get remaining CD
```

**Example:**
```cpp
if (IsSpellReady(SPELL_FIREBALL) && IsInRange(target, SPELL_FIREBALL)) {
    if (CastSpell(target, SPELL_FIREBALL)) {
        TC_LOG_DEBUG("module.playerbot", "Cast Fireball on {}", target->GetName());
    }
}
```

---

##### Target Selection

```cpp
::Unit* GetBestAttackTarget();                   // Best DPS target
::Unit* GetBestHealTarget();                     // Lowest health ally
::Unit* GetNearestEnemy(float maxRange = 30.0f); // Nearest enemy
::Unit* GetLowestHealthAlly(float maxRange = 40.0f); // For healing
```

**Example:**
```cpp
Unit* healTarget = GetBestHealTarget();
if (healTarget && healTarget->GetHealthPct() < 50.0f) {
    CastSpell(healTarget, SPELL_HEAL);
}
```

---

##### Aura Management

```cpp
bool HasAura(uint32 spellId, ::Unit* target = nullptr);
uint32 GetAuraStacks(uint32 spellId, ::Unit* target = nullptr);
uint32 GetAuraRemainingTime(uint32 spellId, ::Unit* target = nullptr);
```

**Example:**
```cpp
if (HasAura(SPELL_BLOODLUST)) {
    // Execute burst rotation
}

uint32 stacks = GetAuraStacks(SPELL_SUNDER_ARMOR, target);
if (stacks < 5) {
    CastSpell(target, SPELL_SUNDER_ARMOR);
}
```

---

## Subsystems

### Group Coordination

**File:** `src/modules/Playerbot/Group/GroupInvitationHandler.h`

#### GroupInvitationHandler

Handles automatic group invitation acceptance for bots.

```cpp
class GroupInvitationHandler {
public:
    bool HandleInvitation(WorldPackets::Party::PartyInvite const& invite);
    void Update(uint32 diff);
    void ProcessPendingInvitations();
};
```

**Usage:**
```cpp
if (BotAI* ai = botSession->GetAI()) {
    GroupInvitationHandler* handler = ai->GetGroupInvitationHandler();
    handler->Update(diff);
}
```

---

### Movement & Positioning

**File:** `src/modules/Playerbot/Movement/LeaderFollowBehavior.h`

#### LeaderFollowBehavior

Strategy for following group leader.

```cpp
class LeaderFollowBehavior : public Strategy {
public:
    void UpdateFollowBehavior(BotAI* ai, uint32 diff);
    float GetFollowDistance() const;
    void SetFollowDistance(float distance);
};
```

**Default Distances:**
- Melee: 5 yards
- Ranged: 15 yards
- Healers: 20 yards

**Usage:**
```cpp
auto* followStrategy = botAI->GetStrategy("follow");
if (auto* followBehavior = dynamic_cast<LeaderFollowBehavior*>(followStrategy)) {
    followBehavior->SetFollowDistance(10.0f);
}
```

---

### Combat Mechanics

**File:** `src/modules/Playerbot/AI/Combat/InterruptCoordinator.h`

#### InterruptCoordinator

Coordinates spell interrupts across multiple bots to prevent overlap.

```cpp
class InterruptCoordinator {
public:
    bool RegisterInterrupt(ObjectGuid botGuid, ObjectGuid targetGuid, uint32 spellId);
    bool CanInterrupt(ObjectGuid botGuid, ObjectGuid targetGuid);
    void ClearInterrupts(ObjectGuid targetGuid);
};
```

**Usage:**
```cpp
if (sInterruptCoordinator->CanInterrupt(bot->GetGUID(), target->GetGUID())) {
    CastSpell(target, SPELL_KICK);
    sInterruptCoordinator->RegisterInterrupt(bot->GetGUID(), target->GetGUID(), SPELL_KICK);
}
```

---

### Database Integration

**File:** `src/modules/Playerbot/Database/PlayerbotCharacterDBInterface.h`

#### PlayerbotCharacterDBInterface

Provides safe async/sync database access routing.

```cpp
class PlayerbotCharacterDBInterface {
public:
    PreparedQueryResult ExecuteSync(CharacterDatabasePreparedStatement* stmt);
    void ExecuteAsync(CharacterDatabasePreparedStatement* stmt, std::function<void(PreparedQueryResult)> callback);
    CharacterDatabaseTransaction BeginTransaction();
    void CommitTransaction(CharacterDatabaseTransaction transaction);
};
```

**Singleton:** `sPlayerbotCharDB`

**Example:**
```cpp
CharacterDatabasePreparedStatement* stmt = sPlayerbotCharDB->GetPreparedStatement(CHAR_SEL_CHARACTER);
stmt->setUInt64(0, characterGuid.GetCounter());

sPlayerbotCharDB->ExecuteAsync(stmt, [](PreparedQueryResult result) {
    if (result) {
        // Process result
    }
});
```

---

### Performance Monitoring

**File:** `src/modules/Playerbot/Performance/BotPerformanceMonitor.h`

#### BotPerformanceMonitor

Monitors bot performance metrics.

```cpp
class BotPerformanceMonitor {
public:
    void RecordBotUpdate(ObjectGuid botGuid, uint32 updateTimeMs);
    void RecordMemoryUsage(ObjectGuid botGuid, uint64 memoryBytes);
    PerformanceReport GenerateReport();
};
```

**Metrics Tracked:**
- Update time per bot
- Memory usage per bot
- CPU usage estimation
- Database query performance
- AI decision time

**Example:**
```cpp
auto report = sBotPerformanceMonitor->GenerateReport();
TC_LOG_INFO("module.playerbot", "Performance Report:");
TC_LOG_INFO("module.playerbot", "  Average Update Time: {:.2f}ms", report.avgUpdateTime);
TC_LOG_INFO("module.playerbot", "  Total Memory: {} MB", report.totalMemoryMB);
```

---

## Configuration

**File:** `playerbots.conf`
**C++ API:** `src/modules/Playerbot/Config/PlayerbotConfig.h`

### Configuration Access

```cpp
bool enabled = sPlayerbotConfig->GetBool("Playerbot.Enable", false);
uint32 maxBots = sPlayerbotConfig->GetUInt("Playerbot.MaxBotsPerAccount", 10);
float ratio = sPlayerbotConfig->GetFloat("Playerbot.BotToPlayerRatio", 2.0f);
std::string logFile = sPlayerbotConfig->GetString("Playerbot.Log.File", "Playerbot.log");
```

### Key Configuration Options

```ini
# Core Settings
Playerbot.Enable = 1
Playerbot.MaxBotsPerAccount = 10
Playerbot.GlobalMaxBots = 1000

# Performance
Playerbot.UpdateInterval = 100
Playerbot.AIDecisionTimeLimit = 50
Playerbot.SpawnBatchSize = 10

# Database
Playerbot.Database.Timeout = 30
Playerbot.Database.EnablePooling = 1

# Logging
Playerbot.Log.Level = 4
Playerbot.Log.File = "Playerbot.log"
```

### Performance Cache

Frequently accessed config values are cached for performance:

```cpp
template<typename T>
T GetCached(std::string const& key, T defaultValue) const;
```

**Example:**
```cpp
// Fast cached access - no mutex lock
uint32 maxBots = sPlayerbotConfig->GetCached<uint32>("Playerbot.MaxBotsPerAccount", 10);
```

---

## Extension Points

### Creating Custom Strategies

**File:** `src/modules/Playerbot/AI/Strategy/Strategy.h`

```cpp
class MyCustomStrategy : public Strategy {
public:
    std::string GetName() const override { return "my_custom"; }

    bool IsActive(BotAI const* ai) const override {
        // Determine if strategy should be active
        return true;
    }

    void UpdateBehavior(BotAI* ai, uint32 diff) override {
        // Implement custom behavior
    }

    void OnActivate(BotAI* ai) override {
        TC_LOG_INFO("module.playerbot", "Custom strategy activated");
    }

    void OnDeactivate(BotAI* ai) override {
        TC_LOG_INFO("module.playerbot", "Custom strategy deactivated");
    }
};

// Register and use
auto customStrategy = std::make_unique<MyCustomStrategy>();
botAI->AddStrategy(std::move(customStrategy));
botAI->ActivateStrategy("my_custom");
```

### Creating Custom Actions

**File:** `src/modules/Playerbot/AI/Actions/Action.h`

```cpp
class MyCustomAction : public Action {
public:
    bool IsPossible(BotAI* ai) override {
        // Check if action can be executed
        return true;
    }

    bool IsUseful(BotAI* ai) override {
        // Check if action should be executed
        return true;
    }

    ActionResult Execute(BotAI* ai, ActionContext const& context) override {
        // Execute action logic
        TC_LOG_INFO("module.playerbot", "Executing custom action");
        return ActionResult::SUCCESS;
    }
};

// Queue action
auto action = std::make_shared<MyCustomAction>();
botAI->QueueAction(action);
```

### Creating Custom Triggers

**File:** `src/modules/Playerbot/AI/Triggers/Trigger.h`

```cpp
class MyCustomTrigger : public Trigger {
public:
    bool Check(BotAI const* ai) override {
        // Fast pre-check
        return ai->GetBot()->GetHealthPct() < 50.0f;
    }

    TriggerResult Evaluate(BotAI* ai) override {
        TriggerResult result;
        result.triggered = true;
        result.urgency = 100.0f;
        result.suggestedAction = std::make_shared<MyCustomAction>();
        return result;
    }
};

// Register trigger
auto trigger = std::make_shared<MyCustomTrigger>();
botAI->RegisterTrigger(trigger);
```

---

## Performance Considerations

### Thread Safety

**Thread-Safe Components:**
- BotSpawner (internal mutex)
- BotSession (packet queue mutex)
- BotAI (strategy mutex)

**Lock-Free Components:**
- `GetActiveBotCount()` (atomic)
- Spawn statistics (atomic counters)
- Bot state tracking (atomic flags)

### Memory Optimization

**Target:** <10MB per bot

**Strategies:**
- Object pooling for frequently created objects
- Shared spell info caching
- Lazy loading of AI components
- Smart pointer usage for RAII cleanup

### Database Optimization

**Async Pattern:**
```cpp
// GOOD - Async with callback
sPlayerbotCharDB->ExecuteAsync(stmt, [](PreparedQueryResult result) {
    // Process result on callback thread
});

// BAD - Synchronous blocking
PreparedQueryResult result = CharacterDatabase.Query(stmt); // Blocks!
```

**Connection Pooling:**
- Dedicated connection pool for bot operations
- Configurable pool size
- Automatic connection recycling

---

## Error Handling

### Common Patterns

```cpp
// Validate inputs
if (!bot || !bot->IsInWorld()) {
    TC_LOG_ERROR("module.playerbot", "Invalid bot state");
    return false;
}

// Handle async failures
sPlayerbotCharDB->ExecuteAsync(stmt, [this](PreparedQueryResult result) {
    if (!result) {
        TC_LOG_ERROR("module.playerbot", "Database query failed");
        return;
    }
    // Process result
});

// Memory safety checks
if (!_active.load() || _destroyed.load()) {
    return; // Session being destroyed
}
```

### Logging Categories

```cpp
TC_LOG_ERROR("module.playerbot.spawner", "Spawn failed");
TC_LOG_WARN("module.playerbot.session", "Session timeout");
TC_LOG_INFO("module.playerbot.ai", "AI state changed");
TC_LOG_DEBUG("module.playerbot.combat", "Spell cast: {}", spellId);
TC_LOG_TRACE("module.playerbot.movement", "Position updated");
```

---

## Version History

**Current:** WoW 11.2 Compatible
**Last Updated:** 2025-10-03
**API Stability:** Stable (production-ready)

For bug reports and feature requests, see project issue tracker.
